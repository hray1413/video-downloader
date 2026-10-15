#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "downloader.h"
#include "os_compat.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#endif

// 策略 2：WinINet 帶 Redirect、TLS 1.2+ 與 HTTP 狀態碼檢驗之原子下載
#ifdef _WIN32
static bool wininet_download_atomic(const char *url, const char *dest_path) {
    HINTERNET hInternet = InternetOpenA("VideoDownloader/3.6 (Windows NT)", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) return false;

    DWORD timeout_ms = 20000;
    InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout_ms, sizeof(timeout_ms));
    InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout_ms, sizeof(timeout_ms));

    DWORD flags = INTERNET_FLAG_RELOAD | 
                  INTERNET_FLAG_NO_CACHE_WRITE | 
                  INTERNET_FLAG_SECURE | 
                  INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
                  INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;

    HINTERNET hUrl = InternetOpenUrlA(hInternet, url, NULL, 0, flags, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return false;
    }

    // 檢查 HTTP 狀態碼
    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    if (HttpQueryInfoA(hUrl, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusSize, NULL)) {
        if (statusCode < 200 || statusCode >= 400) {
            InternetCloseHandle(hUrl);
            InternetCloseHandle(hInternet);
            return false;
        }
    }

    char tmp_dest[MAX_PATH];
    snprintf(tmp_dest, sizeof(tmp_dest), "%s.tmp.%u", dest_path, (unsigned int)GetCurrentProcessId());

    FILE *fp = fopen(tmp_dest, "wb");
    if (!fp) {
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[16384];
    DWORD bytesRead = 0;
    bool success = true;

    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        if (fwrite(buffer, 1, bytesRead, fp) != bytesRead) {
            success = false;
            break;
        }
    }

    fclose(fp);
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    if (success) {
        remove(dest_path);
        if (rename(tmp_dest, dest_path) == 0) {
            return true;
        }
    }

    remove(tmp_dest);
    return false;
}
#endif

// 策略 3：PowerShell Fallback 下載
#ifdef _WIN32
static bool powershell_download_atomic(const char *url, const char *dest_path) {
    char ps_cmd[2048];
    snprintf(ps_cmd, sizeof(ps_cmd),
        "powershell -NoProfile -Command \"$ProgressPreference='SilentlyContinue'; "
        "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; "
        "(New-Object Net.WebClient).DownloadFile('%s', '%s')\"",
        url, dest_path);
    int code = system(ps_cmd);
    return (code == 0 && os_file_exists(dest_path));
}
#endif

bool downloader_fetch(const char *url, const char *dest_path) {
    if (!url || !dest_path) return false;

    // 階段 1: 系統 curl CLI
    if (os_command_exists("curl")) {
        const char *curl_argv[] = {
            "-L",
            "--retry", "3",
            "--connect-timeout", "15",
            "-o", dest_path,
            url,
            NULL
        };
        int code = os_spawn_process("curl", curl_argv);
        if (code == 0 && os_file_exists(dest_path)) {
            return true;
        }
    }

#ifdef _WIN32
    // 階段 2: WinINet 原子下載
    if (wininet_download_atomic(url, dest_path)) {
        return true;
    }

    // 階段 3: PowerShell 下載
    if (powershell_download_atomic(url, dest_path)) {
        return true;
    }
#endif

    return false;
}

bool downloader_ensure_ytdlp(const char *app_dir) {
    char ytdlp_path[1024];
#ifdef _WIN32
    os_path_join(ytdlp_path, sizeof(ytdlp_path), app_dir, "yt-dlp.exe");
    const char *url = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe";
#else
    os_path_join(ytdlp_path, sizeof(ytdlp_path), app_dir, "yt-dlp");
    const char *url = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp";
#endif

    if (os_file_exists(ytdlp_path)) return true;

    printf("[Info] yt-dlp binary not found!\nDownloading yt-dlp ...\n\n");
    if (downloader_fetch(url, ytdlp_path)) {
#ifndef _WIN32
        chmod(ytdlp_path, 0755);
#endif
        printf("[Success] yt-dlp downloaded!\n");
        return true;
    }

    printf("[Error] yt-dlp download failed, please check network.\n");
    printf("Manual download: https://github.com/yt-dlp/yt-dlp/releases/latest\n");
    return false;
}

bool downloader_ensure_ffmpeg(const char *app_dir) {
    char ffmpeg_path[1024];
    char ffprobe_path[1024];
#ifdef _WIN32
    os_path_join(ffmpeg_path, sizeof(ffmpeg_path), app_dir, "ffmpeg.exe");
    os_path_join(ffprobe_path, sizeof(ffprobe_path), app_dir, "ffprobe.exe");
#else
    os_path_join(ffmpeg_path, sizeof(ffmpeg_path), app_dir, "ffmpeg");
    os_path_join(ffprobe_path, sizeof(ffprobe_path), app_dir, "ffprobe");
#endif

    if (os_file_exists(ffmpeg_path) && os_file_exists(ffprobe_path)) {
        return true;
    }

    if (os_command_exists("ffmpeg") && os_command_exists("ffprobe")) {
        return true;
    }

#ifdef _WIN32
    printf("[Info] ffmpeg.exe or ffprobe.exe not found!\nDownloading ffmpeg archive...\n\n");

    char temp_dir[MAX_PATH];
    GetTempPathA(MAX_PATH, temp_dir);
    char zip_path[MAX_PATH];
    char extract_dir[MAX_PATH];
    snprintf(zip_path, sizeof(zip_path), "%sffmpeg-release_%u.zip", temp_dir, (unsigned int)GetTickCount());
    snprintf(extract_dir, sizeof(extract_dir), "%sffmpeg_extract_%u", temp_dir, (unsigned int)GetTickCount());

    const char *url = "https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip";
    if (!downloader_fetch(url, zip_path)) {
        printf("[Error] ffmpeg download failed, check network.\n");
        printf("Manual download: https://www.gyan.dev/ffmpeg/builds/\n");
        return false;
    }

    printf("Extracting ffmpeg.exe and ffprobe.exe...\n");
    char ps_extract[2048];
    snprintf(ps_extract, sizeof(ps_extract),
        "powershell -NoProfile -Command \"Add-Type -AssemblyName System.IO.Compression.FileSystem; "
        "[System.IO.Compression.ZipFile]::ExtractToDirectory('%s', '%s'); "
        "Get-ChildItem -Path '%s' -Filter 'ffmpeg.exe' -Recurse | Select-Object -First 1 | Copy-Item -Destination '%s' -Force; "
        "Get-ChildItem -Path '%s' -Filter 'ffprobe.exe' -Recurse | Select-Object -First 1 | Copy-Item -Destination '%s' -Force; "
        "Remove-Item -Recurse -Force '%s'; Remove-Item -Force '%s'\"",
        zip_path, extract_dir, extract_dir, ffmpeg_path, extract_dir, ffprobe_path, extract_dir, zip_path);

    system(ps_extract);

    if (os_file_exists(ffmpeg_path) && os_file_exists(ffprobe_path)) {
        printf("[Success] ffmpeg.exe & ffprobe.exe extracted!\n");
        return true;
    } else {
        printf("[Error] ffmpeg.exe extraction failed.\n");
        printf("Manual download: https://www.gyan.dev/ffmpeg/builds/\n");
        return false;
    }
#else
    printf("[Error] Please install ffmpeg on your system (e.g. sudo apt install ffmpeg / brew install ffmpeg)\n");
    return false;
#endif
}
