#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "interactive_mode.h"
#include "download_flow.h"
#include "os_compat.h"
#include "event_protocol.h"
#include "url_parser.h"

int interactive_mode_run(const char *app_dir, const char *custom_out_dir, const char *extra_opts) {
    char url[2048] = { 0 };
    char format_choice[32] = { 0 };

MAIN_LOOP:
    url[0] = '\0';
    format_choice[0] = '\0';

    // 1. 剪貼簿偵測與 URL 輸入
    char clip[2048] = { 0 };
    bool has_clip = os_get_clipboard_text(clip, sizeof(clip)) && url_is_valid(clip);

    printf("==========================================\n");
    printf("Enter video URL, or paste from clipboard\n");
    printf("Tip: Enter [U] to update yt-dlp, [about] for author info.\n");
    printf("==========================================\n\n");

    if (has_clip) {
        printf("[Detected URL in Clipboard]:\n%s\n\n", clip);
        printf("Press [Enter] to use this URL, or paste a new URL: ");
        char input[2048] = { 0 };
        if (fgets(input, sizeof(input), stdin)) {
            char *nl = strpbrk(input, "\r\n");
            if (nl) *nl = '\0';
            if (strlen(input) == 0) {
                strncpy(url, clip, sizeof(url) - 1);
            } else {
                strncpy(url, input, sizeof(url) - 1);
            }
        }
    } else {
        printf("Enter video URL: ");
        if (fgets(url, sizeof(url), stdin)) {
            char *nl = strpbrk(url, "\r\n");
            if (nl) *nl = '\0';
        }
    }

    url_normalize(url, sizeof(url), format_choice);

    if (_stricmp(url, "about") == 0 || _stricmp(url, "--about") == 0) {
        event_emit_simple("about", EVENT_STATUS_OK, "v3.6 by hray1413");
        printf("\nAuthor: hray1413\nEmail: videodownload@ss2256.cc.cd\n\n");
        os_pause();
        os_clear_screen();
        goto MAIN_LOOP;
    }

    if (!url_is_valid(url)) {
        printf("[Error] Invalid URL entered. Please try again.\n\n");
        os_pause();
        os_clear_screen();
        goto MAIN_LOOP;
    }

    event_emit("url_detected", EVENT_STATUS_INFO, 0, "URL entered in interactive mode", url);

    // 2. 格式選擇
DOWNLOAD_FORMAT_LOOP:
    if (format_choice[0] == '\0') {
        printf("\nSelect download format:\n");
        printf("   [1] Video (MP4, Best quality - Auto 4K/2K/1080p, Embedded Subs & Cover) [Default]\n");
        printf("   [2] Video (MP4, 1080p Max)\n");
        printf("   [3] Video (MP4, 720p Max)\n");
        printf("   [4] Audio only (MP3 320k, with Cover Art)\n");
        printf("   [5] Audio only (Best M4A, with Cover Art)\n");
        printf("   [6] Entire Playlist (MP4, organized in playlist folder)\n\n");
        printf("Enter choice (1-6, default 1): ");

        char input[64] = { 0 };
        if (fgets(input, sizeof(input), stdin)) {
            char *nl = strpbrk(input, "\r\n");
            if (nl) *nl = '\0';
            char *p = input;
            while (*p == ' ') p++;
            if (*p >= '1' && *p <= '6') {
                format_choice[0] = *p;
                format_choice[1] = '\0';
            } else {
                strcpy(format_choice, "1");
            }
        } else {
            strcpy(format_choice, "1");
        }
    }

    // 3. 執行下載
    DownloadRequest req;
    memset(&req, 0, sizeof(req));
    req.url = url;
    req.format_choice = format_choice;
    req.custom_output_dir = custom_out_dir;
    req.extra_opts = extra_opts;
    req.app_dir = app_dir;

    int exit_code = download_flow_execute(&req);

    // 4. 下載後動作
    if (exit_code == 0) {
        printf("Quick Actions:\n");
        printf("   [O] Open videos folder\n");
        printf("   [C] Continue downloading another video\n");
        printf("   [Enter] Exit\n\n");
        printf("Select action (O/C/Enter): ");

        char action[32] = { 0 };
        if (fgets(action, sizeof(action), stdin)) {
            if (action[0] == 'o' || action[0] == 'O') {
                os_open_folder(req.out_full_dir);
                return 0;
            } else if (action[0] == 'c' || action[0] == 'C') {
                os_clear_screen();
                goto MAIN_LOOP;
            }
        }
    } else {
        printf("Troubleshooting tips:\n");
        printf("   1. Update core: run with -u\n");
        printf("   2. Restricted/Member video: place cookies.txt in folder\n");
        printf("   3. Check your network or proxy connection\n\n");

        printf("Quick Actions:\n");
        printf("   [R] Retry download\n");
        printf("   [C] Try another URL\n");
        printf("   [Enter] Exit\n\n");
        printf("Select action (R/C/Enter): ");

        char action[32] = { 0 };
        if (fgets(action, sizeof(action), stdin)) {
            if (action[0] == 'r' || action[0] == 'R') {
                goto DOWNLOAD_FORMAT_LOOP;
            } else if (action[0] == 'c' || action[0] == 'C') {
                os_clear_screen();
                goto MAIN_LOOP;
            }
        }
    }

    return exit_code;
}
