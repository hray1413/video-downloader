#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "protocol_mode.h"
#include "os_compat.h"
#include "event_protocol.h"
#include "url_parser.h"

#define PROTOCOL_SCHEME "videodl"

int protocol_mode_install(const char *app_dir) {
    char exe_path[1024];
#ifdef _WIN32
    os_path_join(exe_path, sizeof(exe_path), app_dir, "dl.exe");
#else
    os_path_join(exe_path, sizeof(exe_path), app_dir, "dl");
#endif

    bool ok = os_register_protocol(PROTOCOL_SCHEME, exe_path);
    if (ok) {
        printf("[Success] Protocol 'videodl://' registered successfully!\n");
        event_emit("protocol_install", EVENT_STATUS_OK, 0, "Protocol registered", PROTOCOL_SCHEME);
        return 0;
    } else {
        printf("[Error] Failed to register protocol 'videodl://'.\n");
        event_emit("protocol_install", EVENT_STATUS_ERROR, 1, "Failed to register protocol", PROTOCOL_SCHEME);
        return 1;
    }
}

int protocol_mode_uninstall(void) {
    bool ok = os_unregister_protocol(PROTOCOL_SCHEME);
    if (ok) {
        printf("[Success] Protocol 'videodl://' unregistered.\n");
        event_emit("protocol_uninstall", EVENT_STATUS_OK, 0, "Protocol unregistered", PROTOCOL_SCHEME);
        return 0;
    } else {
        printf("[Error] Failed to unregister protocol 'videodl://'.\n");
        event_emit("protocol_uninstall", EVENT_STATUS_ERROR, 1, "Failed to unregister protocol", PROTOCOL_SCHEME);
        return 1;
    }
}

bool protocol_mode_extract(const char *input_arg, char *out_url, size_t max_size) {
    if (!input_arg || !out_url || max_size == 0) return false;
    if (_strnicmp(input_arg, "videodl:", 8) != 0 && _strnicmp(input_arg, "videodl://", 10) != 0) {
        return false;
    }

    strncpy(out_url, input_arg, max_size - 1);
    out_url[max_size - 1] = '\0';
    url_normalize(out_url, max_size, NULL);
    return true;
}
