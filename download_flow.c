#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "download_flow.h"
#include "download_profiles.h"
#include "tokenizer.h"
#include "errors.h"
#include "os_compat.h"
#include "event_protocol.h"

/* -----------------------------------------------------------------------
 * Internal helpers
 * --------------------------------------------------------------------- */

/* Emit a structured "failed" event with error-code name + troubleshoot tip */
static void emit_failure(vd_error_t err, int ytdlp_exit) {
    char msg[256];
    char data[512];

    if (err == VD_ERR_YTDLP_EXIT_ERROR) {
        snprintf(msg, sizeof(msg), "yt-dlp exited with code %d", ytdlp_exit);
        snprintf(data, sizeof(data),
                 "{\"exit_code\":%d,\"hint\":\"%s\"}",
                 ytdlp_exit, vd_error_troubleshoot(err));
    } else {
        snprintf(msg, sizeof(msg), "%s", vd_error_message(err));
        snprintf(data, sizeof(data),
                 "{\"error\":\"%s\",\"hint\":\"%s\"}",
                 vd_error_name(err), vd_error_troubleshoot(err));
    }

    event_emit("failed", EVENT_STATUS_ERROR, (int)err, msg, data);

    if (!event_is_silent_mode()) {
        printf("\n==========================================\n");
        printf("[Failed] %s\n", msg);
        printf("\nTroubleshooting tips:\n");
        printf("   %s\n", vd_error_troubleshoot(err));
        printf("==========================================\n\n");
    }
}

/* -----------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------- */

vd_error_t download_flow_execute(DownloadRequest *req) {
    if (!req || !req->url || !*req->url) return VD_ERR_INVALID_URL;

    /* --- 1. Resolve profile ------------------------------------------ */
    const char *fmt = (req->format_choice && *req->format_choice)
                    ? req->format_choice : "1";
    const DownloadProfile *prof = profile_get(fmt);

    /* --- 2. Build output directory ------------------------------------ */
    char output_dir[1024];
    if (req->custom_output_dir && *req->custom_output_dir) {
        strncpy(output_dir, req->custom_output_dir, sizeof(output_dir) - 1);
        output_dir[sizeof(output_dir) - 1] = '\0';
    } else {
        strncpy(output_dir, prof->default_subfolder, sizeof(output_dir) - 1);
        output_dir[sizeof(output_dir) - 1] = '\0';
    }

    char full_output_dir[1024];
    os_path_join(full_output_dir, sizeof(full_output_dir), req->app_dir, output_dir);
    os_mkdir_p(full_output_dir);
    strncpy(req->out_full_dir, full_output_dir, sizeof(req->out_full_dir) - 1);

    char event_data[512];
    snprintf(event_data, sizeof(event_data),
             "{\"format\":\"%s\",\"profile\":\"%s\",\"output_dir\":\"%s\"}",
             fmt, prof->name, output_dir);
    event_emit("format_selected", EVENT_STATUS_INFO, 0,
                "Download profile selected", event_data);

    /* --- 3. Optional features (cookies, JS runtime) ------------------- */
    char cookies_path[1024];
    os_path_join(cookies_path, sizeof(cookies_path), req->app_dir, "cookies.txt");
    bool has_cookies = os_file_exists(cookies_path);

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
        strncpy(js_runtime_opt, "deno", sizeof(js_runtime_opt) - 1);
    } else if (os_command_exists("node")) {
        strncpy(js_runtime_opt, "node", sizeof(js_runtime_opt) - 1);
    }

    /* --- 4. Build argv[] ---------------------------------------------- */
    /*
     * We use a two-part design:
     *   args[]        = pointers into static strings or local buffers
     *   extra_argv[]  = heap-allocated tokens from tokenizer (freed after spawn)
     *
     * Max 128 slots total; VIDEODL_EXTRA_OPTS gets up to 120 slots.
     */
    const char *args[128];
    int argc_pos = 0;

    char ytdlp_bin[1024];
#ifdef _WIN32
    os_path_join(ytdlp_bin, sizeof(ytdlp_bin), req->app_dir, "yt-dlp.exe");
#else
    os_path_join(ytdlp_bin, sizeof(ytdlp_bin), req->app_dir, "yt-dlp");
#endif

    /* cookies */
    if (has_cookies) {
        args[argc_pos++] = "--cookies";
        args[argc_pos++] = cookies_path;
    }

    /* JS runtime */
    if (js_runtime_opt[0] != '\0') {
        args[argc_pos++] = "--js-runtimes";
        args[argc_pos++] = js_runtime_opt;
    }

    /* playlist vs. single */
    if (prof->is_playlist) {
        args[argc_pos++] = "--yes-playlist";
    } else {
        args[argc_pos++] = "--no-playlist";
    }

    /* ffmpeg location (strip trailing slash) */
    char ffmpeg_loc[1024];
    strncpy(ffmpeg_loc, req->app_dir, sizeof(ffmpeg_loc) - 1);
    ffmpeg_loc[sizeof(ffmpeg_loc) - 1] = '\0';
    size_t flen = strlen(ffmpeg_loc);
    if (flen > 0 && (ffmpeg_loc[flen - 1] == '\\' || ffmpeg_loc[flen - 1] == '/')) {
        ffmpeg_loc[flen - 1] = '\0';
    }
    args[argc_pos++] = "--ffmpeg-location";
    args[argc_pos++] = ffmpeg_loc;

    /* format-specific flags */
    if (strcmp(fmt, "4") == 0) {
        /* MP3 320k */
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
    } else if (strcmp(fmt, "5") == 0) {
        /* Best M4A */
        args[argc_pos++] = "-f";
        args[argc_pos++] = "ba[ext=m4a]/bestaudio";
        args[argc_pos++] = "-x";
        args[argc_pos++] = "--audio-format";
        args[argc_pos++] = "m4a";
        args[argc_pos++] = "--embed-thumbnail";
        args[argc_pos++] = "--convert-thumbnails";
        args[argc_pos++] = "jpg";
    } else {
        /* Video modes (1 / 2 / 3 / 6) */
        if (strcmp(fmt, "2") == 0) {
            args[argc_pos++] = "-f";
            args[argc_pos++] = "bv*[height<=1080]+ba/b[height<=1080]";
            args[argc_pos++] = "-S";
            args[argc_pos++] = "res:1080,ext:mp4:m4a";
        } else if (strcmp(fmt, "3") == 0) {
            args[argc_pos++] = "-f";
            args[argc_pos++] = "bv*[height<=720]+ba/b[height<=720]";
            args[argc_pos++] = "-S";
            args[argc_pos++] = "res:720,ext:mp4:m4a";
        } else {
            /* 1 (Best) and 6 (Playlist) */
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

    /* common flags */
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

    /* VIDEODL_EXTRA_OPTS — safe tokenizer (handles quoted paths/args) */
    char **extra_argv = NULL;
    int    extra_argc = 0;
    if (req->extra_opts && *req->extra_opts) {
        if (cmdline_tokenize(req->extra_opts, &extra_argv, &extra_argc)) {
            for (int i = 0; i < extra_argc && argc_pos < 120; ++i) {
                args[argc_pos++] = extra_argv[i];
            }
        } else {
            event_emit("warning", EVENT_STATUS_INFO, 0,
                       "VIDEODL_EXTRA_OPTS tokenization failed; option ignored", NULL);
        }
    }

    /* output template */
    char out_template[1024];
    snprintf(out_template, sizeof(out_template), "%s/%s",
             full_output_dir, prof->output_template);
    args[argc_pos++] = "-o";
    args[argc_pos++] = out_template;

    /* URL — always last */
    args[argc_pos++] = req->url;
    args[argc_pos]   = NULL;

    /* --- 5. Launch ----------------------------------------------------- */
    if (!event_is_silent_mode()) {
        printf("\n==========================================\n");
        printf("Starting download...\n");
        printf("URL: %s\n", req->url);
        printf("Output Directory: %s\n", full_output_dir);
        printf("==========================================\n\n");
    }

    event_emit("download_start", EVENT_STATUS_INFO, 0, "Spawning yt-dlp", NULL);

    int exit_code = os_spawn_process(ytdlp_bin, args);

    /* Free tokenizer heap now that spawn has returned */
    if (extra_argv) cmdline_free_tokens(extra_argv, extra_argc);

    /* --- 6. Result ------------------------------------------------------ */
    if (exit_code == 0) {
        if (!event_is_silent_mode()) {
            printf("\n==========================================\n");
            printf("[Success] Download completed successfully!\n");
            printf("Saved to: %s\n", full_output_dir);
            printf("==========================================\n\n");
        }
        event_emit("success", EVENT_STATUS_OK, 0,
                   "Download finished successfully", full_output_dir);
        return VD_OK;
    } else {
        emit_failure(VD_ERR_YTDLP_EXIT_ERROR, exit_code);
        return VD_ERR_YTDLP_EXIT_ERROR;
    }
}
