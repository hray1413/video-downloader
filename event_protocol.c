#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "event_protocol.h"
#include "os_compat.h"

static bool g_json_mode = false;
static bool g_silent = false;
static int  g_pipe_fd = -1;
static char g_pipe_path[512] = { 0 };

void event_init(bool cli_json_flag) {
    if (cli_json_flag) {
        g_json_mode = true;
    }

    char *env_json = getenv("VIDEODL_JSON");
    if (env_json && env_json[0] == '1') {
        g_json_mode = true;
    }

    char *env_silent = getenv("VIDEODL_SILENT");
    if (env_silent && env_silent[0] == '1') {
        g_silent = true;
        g_json_mode = true;
    }

    char *env_fd = getenv("VIDEODL_PIPE_FD");
    if (env_fd && *env_fd) {
        g_pipe_fd = atoi(env_fd);
    }

    char *env_pipe = getenv("VIDEODL_PIPE");
    if (env_pipe && *env_pipe) {
        strncpy(g_pipe_path, env_pipe, sizeof(g_pipe_path) - 1);
    }
}

bool event_is_json_mode(void) {
    return g_json_mode;
}

bool event_is_silent_mode(void) {
    return g_silent;
}

static void escape_json_string(char *dest, size_t dest_size, const char *src) {
    if (!dest || dest_size == 0) return;
    dest[0] = '\0';
    if (!src) return;

    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j < dest_size - 4; ++i) {
        if (src[i] == '\\' || src[i] == '\"') {
            dest[j++] = '\\';
            dest[j++] = src[i];
        } else if (src[i] == '\n') {
            dest[j++] = '\\';
            dest[j++] = 'n';
        } else if (src[i] == '\r') {
            continue;
        } else if (src[i] == '\t') {
            dest[j++] = '\\';
            dest[j++] = 't';
        } else {
            dest[j++] = src[i];
        }
    }
    dest[j] = '\0';
}

void event_emit(const char *event_name, EventStatus status, int code, const char *message, const char *data_str) {
    if (!event_name) return;

    const char *status_str = "info";
    switch (status) {
        case EVENT_STATUS_OK:    status_str = "ok"; break;
        case EVENT_STATUS_WARN:  status_str = "warn"; break;
        case EVENT_STATUS_ERROR: status_str = "error"; break;
        default:                 status_str = "info"; break;
    }

    char esc_msg[2048] = { 0 };
    char esc_data[4096] = { 0 };

    escape_json_string(esc_msg, sizeof(esc_msg), message ? message : "");
    escape_json_string(esc_data, sizeof(esc_data), data_str ? data_str : "");

    time_t now = time(NULL);
    char payload[8192];
    snprintf(payload, sizeof(payload),
             "{\"event\":\"%s\",\"status\":\"%s\",\"code\":%d,\"message\":\"%s\",\"data\":\"%s\",\"timestamp\":%lld}\n",
             event_name, status_str, code, esc_msg, esc_data, (long long)now);

    size_t payload_len = strlen(payload);

    // 1. 輸出至 Stdout
    if (g_json_mode && !g_silent) {
        printf("%s", payload);
        fflush(stdout);
    }

    // 2. 輸出至 Persistent FD 或 Named Pipe
    if (g_pipe_fd >= 0 || g_pipe_path[0] != '\0') {
        os_pipe_write(g_pipe_path, g_pipe_fd, payload, payload_len);
    }
}

void event_emit_simple(const char *event_name, EventStatus status, const char *message) {
    event_emit(event_name, status, (status == EVENT_STATUS_ERROR) ? 1 : 0, message, "");
}
