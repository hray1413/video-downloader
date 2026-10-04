#ifdef _WIN32

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <direct.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "os_compat.h"

void os_init(const char *app_title) {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    if (app_title) {
        SetConsoleTitleA(app_title);
    }
}

void os_get_app_dir(char *out_dir, size_t max_size) {
    if (!out_dir || max_size == 0) return;
    GetModuleFileNameA(NULL, out_dir, (DWORD)max_size);
    char *last_slash = strrchr(out_dir, '\\');
    if (last_slash) {
        *(last_slash + 1) = '\0';
    } else {
        strncpy(out_dir, ".\\", max_size - 1);
        out_dir[max_size - 1] = '\0';
    }
}

char os_path_separator(void) {
    return '\\';
}

void os_path_join(char *dest, size_t max_size, const char *dir, const char *subpath) {
    if (!dest || max_size == 0) return;
    dest[0] = '\0';
    if (!dir || !*dir) {
        if (subpath) strncpy(dest, subpath, max_size - 1);
        return;
    }
    strncpy(dest, dir, max_size - 1);
    size_t len = strlen(dest);
    if (len > 0 && dest[len - 1] != '\\' && dest[len - 1] != '/' && len < max_size - 1) {
        dest[len++] = '\\';
        dest[len] = '\0';
    }
    if (subpath && *subpath) {
        if (*subpath == '\\' || *subpath == '/') subpath++;
        strncat(dest, subpath, max_size - strlen(dest) - 1);
    }
}

bool os_file_exists(const char *path) {
    if (!path || !*path) return false;
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool os_dir_exists(const char *path) {
    if (!path || !*path) return false;
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool os_mkdir_p(const char *path) {
    if (!path || !*path) return false;
    char tmp[MAX_PATH];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (len > 0 && (tmp[len - 1] == '\\' || tmp[len - 1] == '/')) {
        tmp[len - 1] = 0;
    }
    for (p = tmp + 1; *p; p++) {
        if (*p == '\\' || *p == '/') {
            *p = 0;
            _mkdir(tmp);
            *p = '\\';
        }
    }
    return (_mkdir(tmp) == 0 || os_dir_exists(tmp));
}

bool os_command_exists(const char *cmd_name) {
    if (!cmd_name || !*cmd_name) return false;
    char check_cmd[MAX_PATH + 32];
    snprintf(check_cmd, sizeof(check_cmd), "where %s >nul 2>nul", cmd_name);
    return (system(check_cmd) == 0);
}

bool os_get_clipboard_text(char *out_text, size_t max_size) {
    if (!out_text || max_size == 0) return false;
    if (!OpenClipboard(NULL)) return false;

    HANDLE hData = GetClipboardData(CF_TEXT);
    if (!hData) {
        CloseClipboard();
        return false;
    }

    char *pszText = (char *)GlobalLock(hData);
    if (!pszText) {
        CloseClipboard();
        return false;
    }

    strncpy(out_text, pszText, max_size - 1);
    out_text[max_size - 1] = '\0';

    GlobalUnlock(hData);
    CloseClipboard();
    return true;
}

void os_open_folder(const char *folder_path) {
    if (!folder_path || !*folder_path) return;
    ShellExecuteA(NULL, "open", folder_path, NULL, NULL, SW_SHOWDEFAULT);
}

/*
 * ============================================================================
 * Windows 參數轉義規範 (Windows Command-Line Quoting Specification)
 * ============================================================================
 */
static void dynbuf_append_char(char **p_buf, size_t *p_len, size_t *p_cap, char c) {
    if (*p_len + 2 > *p_cap) {
        size_t new_cap = (*p_cap == 0) ? 256 : (*p_cap * 2);
        char *new_buf = (char *)realloc(*p_buf, new_cap);
        if (!new_buf) return;
        *p_buf = new_buf;
        *p_cap = new_cap;
    }
    (*p_buf)[(*p_len)++] = c;
    (*p_buf)[*p_len] = '\0';
}

static void dynbuf_append_str(char **p_buf, size_t *p_len, size_t *p_cap, const char *str) {
    if (!str) return;
    while (*str) {
        dynbuf_append_char(p_buf, p_len, p_cap, *str++);
    }
}

static void win32_append_arg(char **p_buf, size_t *p_len, size_t *p_cap, const char *arg, bool is_program_name) {
    if (!arg) return;

    if (*p_len > 0) {
        dynbuf_append_char(p_buf, p_len, p_cap, ' ');
    }

    if (is_program_name) {
        bool need_quote = (strpbrk(arg, " \t") != NULL) || (arg[0] == '\0');
        if (need_quote) dynbuf_append_char(p_buf, p_len, p_cap, '\"');
        dynbuf_append_str(p_buf, p_len, p_cap, arg);
        if (need_quote) dynbuf_append_char(p_buf, p_len, p_cap, '\"');
        return;
    }

    bool need_quote = (strpbrk(arg, " \t\n\v\"") != NULL) || (arg[0] == '\0');
    if (!need_quote) {
        dynbuf_append_str(p_buf, p_len, p_cap, arg);
        return;
    }

    dynbuf_append_char(p_buf, p_len, p_cap, '\"');

    for (const char *p = arg; *p != '\0'; ) {
        unsigned int backslash_count = 0;
        while (*p == '\\') {
            backslash_count++;
            p++;
        }

        if (*p == '\0') {
            for (unsigned int i = 0; i < backslash_count * 2; ++i) {
                dynbuf_append_char(p_buf, p_len, p_cap, '\\');
            }
            break;
        } else if (*p == '\"') {
            for (unsigned int i = 0; i < backslash_count * 2 + 1; ++i) {
                dynbuf_append_char(p_buf, p_len, p_cap, '\\');
            }
            dynbuf_append_char(p_buf, p_len, p_cap, '\"');
            p++;
        } else {
            for (unsigned int i = 0; i < backslash_count; ++i) {
                dynbuf_append_char(p_buf, p_len, p_cap, '\\');
            }
            dynbuf_append_char(p_buf, p_len, p_cap, *p++);
        }
    }

    dynbuf_append_char(p_buf, p_len, p_cap, '\"');
}

char *win32_build_command_line(const char *exe_path, const char **argv) {
    char *buf = NULL;
    size_t len = 0;
    size_t cap = 0;

    if (exe_path) {
        win32_append_arg(&buf, &len, &cap, exe_path, true);
    }

    if (argv) {
        for (int i = 0; argv[i] != NULL; ++i) {
            win32_append_arg(&buf, &len, &cap, argv[i], false);
        }
    }

    return buf;
}

int os_spawn_process(const char *exe_path, const char **argv) {
    char *cmd_buf = win32_build_command_line(exe_path, argv);
    if (!cmd_buf) return -1;

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    BOOL ok = CreateProcessA(
        NULL,
        cmd_buf,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    );

    free(cmd_buf);

    if (!ok) {
        DWORD err = GetLastError();
        return (int)err;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (int)exit_code;
}

bool os_pipe_write(const char *pipe_path, int pipe_fd, const char *data, size_t len) {
    if (!data || len == 0) return false;
    bool written = false;

    if (pipe_fd >= 0) {
        int res = _write(pipe_fd, data, (unsigned int)len);
        if (res > 0) written = true;
    }

    if (pipe_path && pipe_path[0] != '\0') {
        HANDLE hPipe = CreateFileA(pipe_path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hPipe != INVALID_HANDLE_VALUE) {
            DWORD dwWritten = 0;
            if (WriteFile(hPipe, data, (DWORD)len, &dwWritten, NULL)) {
                written = true;
            }
            CloseHandle(hPipe);
        }
    }

    return written;
}

// Windows 註冊表註冊 videodl:// 協議
bool os_register_protocol(const char *scheme_name, const char *app_path) {
    if (!scheme_name || !app_path) return false;

    char key_path[512];
    snprintf(key_path, sizeof(key_path), "Software\\Classes\\%s", scheme_name);

    HKEY hKey;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, key_path, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
        return false;
    }

    char desc[256];
    snprintf(desc, sizeof(desc), "URL:%s Protocol", scheme_name);
    RegSetValueExA(hKey, NULL, 0, REG_SZ, (const BYTE *)desc, (DWORD)(strlen(desc) + 1));
    RegSetValueExA(hKey, "URL Protocol", 0, REG_SZ, (const BYTE *)"", 1);

    HKEY hCmdKey;
    char cmd_path[512];
    snprintf(cmd_path, sizeof(cmd_path), "%s\\shell\\open\\command", key_path);
    if (RegCreateKeyExA(HKEY_CURRENT_USER, cmd_path, 0, NULL, 0, KEY_WRITE, NULL, &hCmdKey, NULL) == ERROR_SUCCESS) {
        char cmd_val[1024];
        snprintf(cmd_val, sizeof(cmd_val), "\"%s\" \"%%1\"", app_path);
        RegSetValueExA(hCmdKey, NULL, 0, REG_SZ, (const BYTE *)cmd_val, (DWORD)(strlen(cmd_val) + 1));
        RegCloseKey(hCmdKey);
    }

    RegCloseKey(hKey);
    return true;
}

bool os_unregister_protocol(const char *scheme_name) {
    if (!scheme_name) return false;
    char key_path[512];
    snprintf(key_path, sizeof(key_path), "Software\\Classes\\%s", scheme_name);
    return (RegDeleteTreeA(HKEY_CURRENT_USER, key_path) == ERROR_SUCCESS);
}

void os_pause(void) {
    system("pause");
}

void os_clear_screen(void) {
    system("cls");
}

#endif // _WIN32
