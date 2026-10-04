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

bool downloader_fetch(const char *url, const char *dest_path) {
    if (!url || !dest_path) return false;

    // 優先策略 1: 系統 curl CLI
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
    // 優先策略 2 (Windows): WinINet API 原生下載
    HINTERNET hInternet = InternetOpenA("VideoDownloader/3.5", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) return false;

    HINTERNET hUrl = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return false;
    }

    FILE *fp = fopen(dest_path, "wb");
    if (!fp) {
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[8192];
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

    if (!success) {
        remove(dest_path);
    }
    return success;
#else
    return false;
#endif
}

bool downloader_ensure_ytdlp(const char *app_dir) {
    char ytdlp_path[1024];
#ifdef _WIN32
    snprintf(ytdlp_path, sizeof(ytdlp_path), "%syt-dlp.exe", app_dir);
    const char *url = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe";
#else
    snprintf(ytdlp_path, sizeof(ytdlp_path), "%syt-dlp", app_dir);
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
    snprintf(ffmpeg_path, sizeof(ffmpeg_path), "%sffmpeg.exe", app_dir);
    snprintf(ffprobe_path, sizeof(ffprobe_path), "%sffprobe.exe", app_dir);
#else
    snprintf(ffmpeg_path, sizeof(ffmpeg_path), "%sffmpeg", app_dir);
    snprintf(ffprobe_path, sizeof(ffprobe_path), "%sffprobe", app_dir);
#endif

    if (os_file_exists(ffmpeg_path) && os_file_exists(ffprobe_path)) {
        return true;
    }

    // 若系統中已存在 ffmpeg 與 ffprobe 則直接使用系統版本
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
    printf("[Error] Please install ffmpeg on your system: sudo apt install ffmpeg / brew install ffmpeg\n");
    return false;
#endif
}
