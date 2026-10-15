#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "cli_mode.h"
#include "interactive_mode.h"
#include "protocol_mode.h"
#include "download_flow.h"
#include "downloader.h"
#include "os_compat.h"
#include "event_protocol.h"
#include "url_parser.h"

#define AUTHOR_INFO "v3.6 by hray1413"
#define AUTHOR_EMAIL "videodownload@ss2256.cc.cd"

static void print_about(bool no_pause) {
    printf("==========================================\n");
    printf("   YT-DLP Video Downloader\n");
    printf("==========================================\n\n");
    printf("Author: hray1413\n");
    printf("Email: %s\n\n", AUTHOR_EMAIL);
    printf("==========================================\n");
    event_emit("about", EVENT_STATUS_OK, 0, "About info", AUTHOR_INFO);
    if (!no_pause) os_pause();
}

static void print_help(const char *prog_name) {
    printf("\n================================================================\n");
    printf("[Guide / Usage Instructions] YT-DLP Video Downloader\n");
    printf("================================================================\n\n");
    printf("Usage:\n");
    printf("   1. Interactive:       %s\n", prog_name);
    printf("   2. Download:          %s <URL> [Format 1-6]\n", prog_name);
    printf("   3. JSON Output:       %s --json-output <URL> [Format]\n", prog_name);
    printf("   4. Update Core:       %s -u\n", prog_name);
    printf("   5. Install Protocol:  %s --install-protocol\n", prog_name);
    printf("   6. Remove Protocol:   %s --uninstall-protocol\n", prog_name);
    printf("   7. About Info:        %s about\n", prog_name);
    printf("================================================================\n\n");
    event_emit("help", EVENT_STATUS_INFO, 0, "Help displayed", "");
}

int cli_mode_run(int argc, char *argv[]) {
    char app_dir[1024];
    os_get_app_dir(app_dir, sizeof(app_dir));

    bool cli_json = false;
    int arg_idx = 1;

    if (arg_idx < argc && _stricmp(argv[arg_idx], "--json-output") == 0) {
        cli_json = true;
        arg_idx++;
    }

    event_init(cli_json);

    bool no_pause = false;
    char *env_no_pause = getenv("VIDEODL_NO_PAUSE");
    if ((env_no_pause && env_no_pause[0] == '1') || cli_json || event_is_json_mode()) {
        no_pause = true;
    }

    char *env_out_dir = getenv("VIDEODL_OUTPUT_DIR");
    char *env_extra = getenv("VIDEODL_EXTRA_OPTS");

    event_emit("init", EVENT_STATUS_INFO, 0, "Initializing environment and core tools", "");

    // 檢測並自動下載核心組件
    if (!downloader_ensure_ytdlp(app_dir) || !downloader_ensure_ffmpeg(app_dir)) {
        event_emit("error", EVENT_STATUS_ERROR, 1, "Core tools check failed", "");
        if (!no_pause) os_pause();
        return 1;
    }
    event_emit("tool_check", EVENT_STATUS_OK, 0, "Tools verification succeeded", "OK");

    // 若無引數傳入，進入互動式 TUI 模式
    if (arg_idx >= argc && !getenv("VIDEODL_URL_FILE")) {
        return interactive_mode_run(app_dir, env_out_dir, env_extra);
    }

    // 處理管理型指令
    if (arg_idx < argc) {
        const char *arg = argv[arg_idx];

        if (_stricmp(arg, "about") == 0 || _stricmp(arg, "--about") == 0 || _stricmp(arg, "-about") == 0) {
            print_about(no_pause);
            return 0;
        }
        if (_stricmp(arg, "-u") == 0 || _stricmp(arg, "--update") == 0 || _stricmp(arg, "update") == 0) {
            char ytdlp_path[1024];
#ifdef _WIN32
            os_path_join(ytdlp_path, sizeof(ytdlp_path), app_dir, "yt-dlp.exe");
#else
            os_path_join(ytdlp_path, sizeof(ytdlp_path), app_dir, "yt-dlp");
#endif
            const char *update_argv[] = { "-U", NULL };
            event_emit("update", EVENT_STATUS_INFO, 0, "Updating yt-dlp binary", "");
            int res = os_spawn_process(ytdlp_path, update_argv);
            if (!no_pause) os_pause();
            return res;
        }
        if (_stricmp(arg, "-h") == 0 || _stricmp(arg, "--help") == 0 || _stricmp(arg, "/?") == 0 || _stricmp(arg, "help") == 0) {
            print_help(argv[0]);
            return 0;
        }
        if (_stricmp(arg, "--install-protocol") == 0) {
            int res = protocol_mode_install(app_dir);
            if (!no_pause) os_pause();
            return res;
        }
        if (_stricmp(arg, "--uninstall-protocol") == 0) {
            int res = protocol_mode_uninstall();
            if (!no_pause) os_pause();
            return res;
        }
    }

    char url[2048] = { 0 };
    char format_choice[32] = { 0 };
    char *env_url_file = getenv("VIDEODL_URL_FILE");

    // 讀取 URL
    if (env_url_file && *env_url_file) {
        FILE *fp = fopen(env_url_file, "r");
        if (fp) {
            if (fgets(url, sizeof(url), fp)) {
                char *nl = strpbrk(url, "\r\n");
                if (nl) *nl = '\0';
            }
            fclose(fp);
        }
        if (arg_idx < argc) {
            strncpy(format_choice, argv[arg_idx], sizeof(format_choice) - 1);
        }
    } else if (arg_idx + 1 < argc && os_file_exists(argv[arg_idx + 1]) && !url_is_valid(argv[arg_idx])) {
        // Electron 模式: dl <FORMAT> <URL_FILE>
        strncpy(format_choice, argv[arg_idx], sizeof(format_choice) - 1);
        FILE *fp = fopen(argv[arg_idx + 1], "r");
        if (fp) {
            if (fgets(url, sizeof(url), fp)) {
                char *nl = strpbrk(url, "\r\n");
                if (nl) *nl = '\0';
            }
            fclose(fp);
        }
    } else if (arg_idx < argc) {
        strncpy(url, argv[arg_idx], sizeof(url) - 1);
        if (arg_idx + 1 < argc) {
            strncpy(format_choice, argv[arg_idx + 1], sizeof(format_choice) - 1);
        }
    }

    // 檢查自訂協議前綴
    char extracted_url[2048];
    if (protocol_mode_extract(url, extracted_url, sizeof(extracted_url))) {
        strncpy(url, extracted_url, sizeof(url) - 1);
    } else {
        url_normalize(url, sizeof(url), format_choice);
    }

    if (!url_is_valid(url)) {
        printf("[Error] Invalid URL supplied: %s\n", url);
        event_emit("error", EVENT_STATUS_ERROR, 1, "Invalid URL supplied", url);
        print_help(argv[0]);
        if (!no_pause) os_pause();
        return 1;
    }

    event_emit("url_detected", EVENT_STATUS_INFO, 0, "Target URL recognized", url);

    DownloadRequest req;
    memset(&req, 0, sizeof(req));
    req.url = url;
    req.format_choice = format_choice;
    req.custom_output_dir = env_out_dir;
    req.extra_opts = env_extra;
    req.app_dir = app_dir;

    int exit_code = download_flow_execute(&req);
    if (!no_pause) os_pause();
    return exit_code;
}
