#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "download_flow.h"
#include "os_compat.h"
#include "event_protocol.h"

int download_flow_execute(DownloadRequest *req) {
    if (!req || !req->url || !*req->url) return 1;

    const char *fmt = (req->format_choice && *req->format_choice) ? req->format_choice : "1";
    char output_dir[1024] = "videos";
    char out_subpath[512] = "%(title)s [%(id)s].%(ext)s";
    bool is_playlist = false;
    bool is_audio_mp3 = false;
    bool is_audio_m4a = false;
    int max_res = 0;

    if (strcmp(fmt, "2") == 0) {
        max_res = 1080;
    } else if (strcmp(fmt, "3") == 0) {
        max_res = 720;
    } else if (strcmp(fmt, "4") == 0) {
        is_audio_mp3 = true;
        strcpy(output_dir, "videos/audio/mp3");
    } else if (strcmp(fmt, "5") == 0) {
        is_audio_m4a = true;
        strcpy(output_dir, "videos/audio/m4a");
    } else if (strcmp(fmt, "6") == 0) {
        is_playlist = true;
        strcpy(output_dir, "videos/playlists");
        strcpy(out_subpath, "%(playlist_title|Unknown)s/%(playlist_index|0)s - %(title)s [%(id)s].%(ext)s");
    }

    if (req->custom_output_dir && *req->custom_output_dir) {
        strncpy(output_dir, req->custom_output_dir, sizeof(output_dir) - 1);
    }

    char full_output_dir[1024];
    os_path_join(full_output_dir, sizeof(full_output_dir), req->app_dir, output_dir);
    os_mkdir_p(full_output_dir);
    strncpy(req->out_full_dir, full_output_dir, sizeof(req->out_full_dir) - 1);

    char event_data[512];
    snprintf(event_data, sizeof(event_data), "Format=%s OutputDir=%s", fmt, output_dir);
    event_emit("format_selected", EVENT_STATUS_INFO, 0, "Download profile selected", event_data);

    // 檢查 cookies
    char cookies_path[1024];
    os_path_join(cookies_path, sizeof(cookies_path), req->app_dir, "cookies.txt");
    bool has_cookies = os_file_exists(cookies_path);

    // 檢查 JS runtime
    char deno_local[1024];
#ifdef _WIN32
    os_path_join(deno_local, sizeof(deno_local), req->app_dir, "deno.exe");
#else
    os_path_join(deno_local, sizeof(deno_local), req->app_dir, "deno");
#endif
    char js_runtime_opt[1024] = { 0 };
    if (os_file_exists(deno_local)) {
        snprintf(js_runtime_opt, sizeof(js_runtime_opt), "deno:%s", deno_local);
    } else if (os_command_exists("deno")) {
        strcpy(js_runtime_opt, "deno");
    } else if (os_command_exists("node")) {
        strcpy(js_runtime_opt, "node");
    }

    // 建構 argv[] 陣列
    const char *args[128];
    int argc_pos = 0;

    char ytdlp_bin[1024];
#ifdef _WIN32
    os_path_join(ytdlp_bin, sizeof(ytdlp_bin), req->app_dir, "yt-dlp.exe");
#else
    os_path_join(ytdlp_bin, sizeof(ytdlp_bin), req->app_dir, "yt-dlp");
#endif

    if (has_cookies) {
        args[argc_pos++] = "--cookies";
        args[argc_pos++] = cookies_path;
    }

    if (js_runtime_opt[0] != '\0') {
        args[argc_pos++] = "--js-runtimes";
        args[argc_pos++] = js_runtime_opt;
    }

    if (is_playlist) {
        args[argc_pos++] = "--yes-playlist";
    } else {
        args[argc_pos++] = "--no-playlist";
    }

    char ffmpeg_loc[1024];
    strncpy(ffmpeg_loc, req->app_dir, sizeof(ffmpeg_loc) - 1);
    size_t flen = strlen(ffmpeg_loc);
    if (flen > 0 && (ffmpeg_loc[flen - 1] == '\\' || ffmpeg_loc[flen - 1] == '/')) {
        ffmpeg_loc[flen - 1] = '\0';
    }
    args[argc_pos++] = "--ffmpeg-location";
    args[argc_pos++] = ffmpeg_loc;

    if (is_audio_mp3) {
        args[argc_pos++] = "-f";
        args[argc_pos++] = "bestaudio";
        args[argc_pos++] = "--extract-audio";
        args[argc_pos++] = "--audio-format";
        args[argc_pos++] = "mp3";
        args[argc_pos++] = "--audio-quality";
        args[argc_pos++] = "0";
        args[argc_pos++] = "--embed-thumbnail";
        args[argc_pos++] = "--convert-thumbnails";
        args[argc_pos++] = "jpg";
    } else if (is_audio_m4a) {
        args[argc_pos++] = "-f";
        args[argc_pos++] = "ba[ext=m4a]/bestaudio";
        args[argc_pos++] = "-x";
        args[argc_pos++] = "--audio-format";
        args[argc_pos++] = "m4a";
        args[argc_pos++] = "--embed-thumbnail";
        args[argc_pos++] = "--convert-thumbnails";
        args[argc_pos++] = "jpg";
    } else {
        if (max_res == 1080) {
            args[argc_pos++] = "-f";
            args[argc_pos++] = "bv*[height<=1080]+ba/b[height<=1080]";
            args[argc_pos++] = "-S";
            args[argc_pos++] = "res:1080,ext:mp4:m4a";
        } else if (max_res == 720) {
            args[argc_pos++] = "-f";
            args[argc_pos++] = "bv*[height<=720]+ba/b[height<=720]";
            args[argc_pos++] = "-S";
            args[argc_pos++] = "res:720,ext:mp4:m4a";
        } else {
            args[argc_pos++] = "-f";
            args[argc_pos++] = "bv*+ba/b";
            args[argc_pos++] = "-S";
            args[argc_pos++] = "res,ext:mp4:m4a";
        }
        args[argc_pos++] = "--merge-output-format";
        args[argc_pos++] = "mp4";
        args[argc_pos++] = "--embed-thumbnail";
        args[argc_pos++] = "--embed-subs";
        args[argc_pos++] = "--sub-langs";
        args[argc_pos++] = "zh-Hans,zh-Hant,zh,en.*";
    }

    args[argc_pos++] = "--embed-metadata";
    args[argc_pos++] = "--windows-filenames";
    args[argc_pos++] = "--trim-filenames";
    args[argc_pos++] = "150";
    args[argc_pos++] = "-N";
    args[argc_pos++] = "4";
    args[argc_pos++] = "--retries";
    args[argc_pos++] = "10";
    args[argc_pos++] = "--fragment-retries";
    args[argc_pos++] = "10";
    args[argc_pos++] = "--newline";

    char extra_buf[2048];
    if (req->extra_opts && *req->extra_opts) {
        strncpy(extra_buf, req->extra_opts, sizeof(extra_buf) - 1);
        extra_buf[sizeof(extra_buf) - 1] = '\0';
        char *tok = strtok(extra_buf, " ");
        while (tok && argc_pos < 120) {
            args[argc_pos++] = tok;
            tok = strtok(NULL, " ");
        }
    }

    char out_template[1024];
    snprintf(out_template, sizeof(out_template), "%s/%s", full_output_dir, out_subpath);
    args[argc_pos++] = "-o";
    args[argc_pos++] = out_template;

    args[argc_pos++] = req->url;
    args[argc_pos] = NULL;

    if (!event_is_silent_mode()) {
        printf("\n==========================================\n");
        printf("Starting download...\n");
        printf("URL: %s\n", req->url);
        printf("Output Directory: %s\n", full_output_dir);
        printf("==========================================\n\n");
    }

    int exit_code = os_spawn_process(ytdlp_bin, args);

    if (exit_code == 0) {
        if (!event_is_silent_mode()) {
            printf("\n==========================================\n");
            printf("[Success] Download completed successfully!\n");
            printf("Saved to: %s\n", full_output_dir);
            printf("==========================================\n\n");
        }
        event_emit("success", EVENT_STATUS_OK, 0, "Download finished successfully", full_output_dir);
    } else {
        if (!event_is_silent_mode()) {
            printf("\n==========================================\n");
            printf("[Failed] Download encountered an error (code: %d)\n", exit_code);
            printf("==========================================\n\n");
        }
        char code_str[64];
        snprintf(code_str, sizeof(code_str), "Exit code %d", exit_code);
        event_emit("failed", EVENT_STATUS_ERROR, exit_code, "Download failed", code_str);
    }

    return exit_code;
}
