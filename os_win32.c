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

// 輔助函式：UTF-8 轉 UTF-16 (wchar_t)
static wchar_t *utf8_to_utf16(const char *utf8_str) {
    if (!utf8_str) return NULL;
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
    if (len <= 0) return NULL;
    wchar_t *wstr = (wchar_t *)malloc(sizeof(wchar_t) * len);
    if (!wstr) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, wstr, len);
    return wstr;
}

// 輔助函式：UTF-16 (wchar_t) 轉 UTF-8
static bool utf16_to_utf8(const wchar_t *wstr, char *out_utf8, size_t max_size) {
    if (!wstr || !out_utf8 || max_size == 0) return false;
    int res = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, out_utf8, (int)max_size, NULL, NULL);
    if (res <= 0) {
        out_utf8[0] = '\0';
        return false;
    }
    return true;
}

void os_init(const char *app_title) {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    if (app_title) {
        wchar_t *wtitle = utf8_to_utf16(app_title);
        if (wtitle) {
            SetConsoleTitleW(wtitle);
            free(wtitle);
        }
    }
}

void os_get_app_dir(char *out_dir, size_t max_size) {
    if (!out_dir || max_size == 0) return;
    wchar_t wpath[MAX_PATH];
    GetModuleFileNameW(NULL, wpath, MAX_PATH);
    wchar_t *last_slash = wcsrchr(wpath, L'\\');
    if (last_slash) {
        *(last_slash + 1) = L'\0';
    } else {
        wcscpy(wpath, L".\\");
    }
    utf16_to_utf8(wpath, out_dir, max_size);
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
    wchar_t *wpath = utf8_to_utf16(path);
    if (!wpath) return false;
    DWORD attr = GetFileAttributesW(wpath);
    free(wpath);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool os_dir_exists(const char *path) {
    if (!path || !*path) return false;
    wchar_t *wpath = utf8_to_utf16(path);
    if (!wpath) return false;
    DWORD attr = GetFileAttributesW(wpath);
    free(wpath);
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool os_mkdir_p(const char *path) {
    if (!path || !*path) return false;
    wchar_t *wpath = utf8_to_utf16(path);
    if (!wpath) return false;

    wchar_t tmp[MAX_PATH];
    wcsncpy(tmp, wpath, MAX_PATH - 1);
    tmp[MAX_PATH - 1] = L'\0';
    free(wpath);

    size_t len = wcslen(tmp);
    if (len > 0 && (tmp[len - 1] == L'\\' || tmp[len - 1] == L'/')) {
        tmp[len - 1] = 0;
    }
    for (wchar_t *p = tmp + 1; *p; p++) {
        if (*p == L'\\' || *p == L'/') {
            *p = 0;
            _wmkdir(tmp);
            *p = L'\\';
        }
    }
    return (_wmkdir(tmp) == 0 || os_dir_exists(path));
}

bool os_command_exists(const char *cmd_name) {
    if (!cmd_name || !*cmd_name) return false;
    char check_cmd[MAX_PATH + 32];
    snprintf(check_cmd, sizeof(check_cmd), "where %s >nul 2>nul", cmd_name);
    return (system(check_cmd) == 0);
}

// 原生 Win32 Unicode 剪貼簿讀取 (CF_UNICODETEXT -> UTF-8)
bool os_get_clipboard_text(char *out_text, size_t max_size) {
    if (!out_text || max_size == 0) return false;
    out_text[0] = '\0';
    if (!OpenClipboard(NULL)) return false;

    bool success = false;
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData) {
        wchar_t *pwText = (wchar_t *)GlobalLock(hData);
        if (pwText) {
            success = utf16_to_utf8(pwText, out_text, max_size);
            GlobalUnlock(hData);
        }
    } else {
        // Fallback 到 ANSI
        HANDLE hDataA = GetClipboardData(CF_TEXT);
        if (hDataA) {
            char *pText = (char *)GlobalLock(hDataA);
            if (pText) {
                strncpy(out_text, pText, max_size - 1);
                out_text[max_size - 1] = '\0';
                success = true;
                GlobalUnlock(hDataA);
            }
        }
    }

    CloseClipboard();
    return success;
}

void os_open_folder(const char *folder_path) {
    if (!folder_path || !*folder_path) return;
    wchar_t *wpath = utf8_to_utf16(folder_path);
    if (wpath) {
        ShellExecuteW(NULL, L"open", wpath, NULL, NULL, SW_SHOWDEFAULT);
        free(wpath);
    }
}

/*
 * ============================================================================
 * Windows 參數轉義規範 (MSDN CommandLineToArgvW 逆向規則)
 * ============================================================================
 */
static void dynbuf_append_wchar(wchar_t **p_buf, size_t *p_len, size_t *p_cap, wchar_t c) {
    if (*p_len + 2 > *p_cap) {
        size_t new_cap = (*p_cap == 0) ? 256 : (*p_cap * 2);
        wchar_t *new_buf = (wchar_t *)realloc(*p_buf, sizeof(wchar_t) * new_cap);
        if (!new_buf) return;
        *p_buf = new_buf;
        *p_cap = new_cap;
    }
    (*p_buf)[(*p_len)++] = c;
    (*p_buf)[*p_len] = L'\0';
}

static void dynbuf_append_wstr(wchar_t **p_buf, size_t *p_len, size_t *p_cap, const wchar_t *str) {
    if (!str) return;
    while (*str) {
        dynbuf_append_wchar(p_buf, p_len, p_cap, *str++);
    }
}

static void win32_append_arg_w(wchar_t **p_buf, size_t *p_len, size_t *p_cap, const wchar_t *arg, bool is_program_name) {
    if (!arg) return;

    if (*p_len > 0) {
        dynbuf_append_wchar(p_buf, p_len, p_cap, L' ');
    }

    if (is_program_name) {
        bool need_quote = (wcspbrk(arg, L" \t") != NULL) || (arg[0] == L'\0');
        if (need_quote) dynbuf_append_wchar(p_buf, p_len, p_cap, L'\"');
        dynbuf_append_wstr(p_buf, p_len, p_cap, arg);
        if (need_quote) dynbuf_append_wchar(p_buf, p_len, p_cap, L'\"');
        return;
    }

    bool need_quote = (wcspbrk(arg, L" \t\n\v\"") != NULL) || (arg[0] == L'\0');
    if (!need_quote) {
        dynbuf_append_wstr(p_buf, p_len, p_cap, arg);
        return;
    }

    dynbuf_append_wchar(p_buf, p_len, p_cap, L'\"');

    for (const wchar_t *p = arg; *p != L'\0'; ) {
        unsigned int backslash_count = 0;
        while (*p == L'\\') {
            backslash_count++;
            p++;
        }

        if (*p == L'\0') {
            for (unsigned int i = 0; i < backslash_count * 2; ++i) {
                dynbuf_append_wchar(p_buf, p_len, p_cap, L'\\');
            }
            break;
        } else if (*p == L'\"') {
            for (unsigned int i = 0; i < backslash_count * 2 + 1; ++i) {
                dynbuf_append_wchar(p_buf, p_len, p_cap, L'\\');
            }
            dynbuf_append_wchar(p_buf, p_len, p_cap, L'\"');
            p++;
        } else {
            for (unsigned int i = 0; i < backslash_count; ++i) {
                dynbuf_append_wchar(p_buf, p_len, p_cap, L'\\');
            }
            dynbuf_append_wchar(p_buf, p_len, p_cap, *p++);
        }
    }

    dynbuf_append_wchar(p_buf, p_len, p_cap, L'\"');
}

int os_spawn_process(const char *exe_path, const char **argv) {
    wchar_t *cmd_buf = NULL;
    size_t len = 0;
    size_t cap = 0;

    if (exe_path) {
        wchar_t *wexe = utf8_to_utf16(exe_path);
        if (wexe) {
            win32_append_arg_w(&cmd_buf, &len, &cap, wexe, true);
            free(wexe);
        }
    }

    if (argv) {
        for (int i = 0; argv[i] != NULL; ++i) {
            wchar_t *warg = utf8_to_utf16(argv[i]);
            if (warg) {
                win32_append_arg_w(&cmd_buf, &len, &cap, warg, false);
                free(warg);
            }
        }
    }

    if (!cmd_buf) return -1;

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // 使用 CreateProcessW，完整支援 Unicode 檔名與路徑
    BOOL ok = CreateProcessW(
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
        wchar_t *wpipe = utf8_to_utf16(pipe_path);
        if (wpipe) {
            HANDLE hPipe = CreateFileW(wpipe, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (hPipe != INVALID_HANDLE_VALUE) {
                DWORD dwWritten = 0;
                if (WriteFile(hPipe, data, (DWORD)len, &dwWritten, NULL)) {
                    written = true;
                }
                CloseHandle(hPipe);
            }
            free(wpipe);
        }
    }

    return written;
}

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
