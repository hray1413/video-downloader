#ifndef _WIN32

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include "os_compat.h"

void os_init(const char *app_title) {
    if (app_title) {
        printf("\033]0;%s\007", app_title);
        fflush(stdout);
    }
}

void os_get_app_dir(char *out_dir, size_t max_size) {
    if (!out_dir || max_size == 0) return;
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count != -1) {
        path[count] = '\0';
        char *last_slash = strrchr(path, '/');
        if (last_slash) {
            *(last_slash + 1) = '\0';
            strncpy(out_dir, path, max_size - 1);
            out_dir[max_size - 1] = '\0';
            return;
        }
    }
    strncpy(out_dir, "./", max_size - 1);
    out_dir[max_size - 1] = '\0';
}

char os_path_separator(void) {
    return '/';
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
    if (len > 0 && dest[len - 1] != '/' && len < max_size - 1) {
        dest[len++] = '/';
        dest[len] = '\0';
    }
    if (subpath && *subpath) {
        if (*subpath == '/') subpath++;
        strncat(dest, subpath, max_size - strlen(dest) - 1);
    }
}

bool os_file_exists(const char *path) {
    if (!path || !*path) return false;
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

bool os_dir_exists(const char *path) {
    if (!path || !*path) return false;
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

bool os_mkdir_p(const char *path) {
    if (!path || !*path) return false;
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (len > 0 && tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    return (mkdir(tmp, 0755) == 0 || os_dir_exists(tmp));
}

bool os_command_exists(const char *cmd_name) {
    if (!cmd_name || !*cmd_name) return false;
    char check_cmd[PATH_MAX + 32];
    snprintf(check_cmd, sizeof(check_cmd), "command -v %s >/dev/null 2>&1", cmd_name);
    return (system(check_cmd) == 0);
}

bool os_get_clipboard_text(char *out_text, size_t max_size) {
    if (!out_text || max_size == 0) return false;
    FILE *fp = NULL;
    if (os_command_exists("wl-paste")) {
        fp = popen("wl-paste --no-newline 2>/dev/null", "r");
    } else if (os_command_exists("xclip")) {
        fp = popen("xclip -selection clipboard -o 2>/dev/null", "r");
    } else if (os_command_exists("pbpaste")) {
        fp = popen("pbpaste 2>/dev/null", "r");
    }

    if (!fp) return false;
    size_t n = fread(out_text, 1, max_size - 1, fp);
    out_text[n] = '\0';
    pclose(fp);
    return (n > 0);
}

void os_open_folder(const char *folder_path) {
    if (!folder_path || !*folder_path) return;
    char cmd[PATH_MAX + 32];
    #ifdef __APPLE__
    snprintf(cmd, sizeof(cmd), "open \"%s\"", folder_path);
    #else
    snprintf(cmd, sizeof(cmd), "xdg-open \"%s\" >/dev/null 2>&1 &", folder_path);
    #endif
    system(cmd);
}

int os_spawn_process(const char *exe_path, const char **argv) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        int arg_count = 0;
        if (argv) {
            while (argv[arg_count] != NULL) arg_count++;
        }
        char **full_argv = (char **)malloc(sizeof(char *) * (arg_count + 2));
        full_argv[0] = (char *)exe_path;
        for (int i = 0; i < arg_count; ++i) {
            full_argv[i + 1] = (char *)argv[i];
        }
        full_argv[arg_count + 1] = NULL;

        execvp(exe_path, full_argv);
        _exit(127);
    }

    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
}

bool os_pipe_write(const char *pipe_path, int pipe_fd, const char *data, size_t len) {
    if (!data || len == 0) return false;
    bool written = false;

    if (pipe_fd >= 0) {
        ssize_t res = write(pipe_fd, data, len);
        if (res > 0) written = true;
    }

    if (pipe_path && pipe_path[0] != '\0') {
        int fd = open(pipe_path, O_WRONLY | O_NONBLOCK);
        if (fd >= 0) {
            ssize_t res = write(fd, data, len);
            if (res > 0) written = true;
            close(fd);
        }
    }

    return written;
}

bool os_register_protocol(const char *scheme_name, const char *app_path) {
    char *home = getenv("HOME");
    if (!home) return false;

    char app_dir[PATH_MAX];
    snprintf(app_dir, sizeof(app_dir), "%s/.local/share/applications", home);
    os_mkdir_p(app_dir);

    char desktop_file[PATH_MAX];
    snprintf(desktop_file, sizeof(desktop_file), "%s/%s.desktop", app_dir, scheme_name);

    FILE *fp = fopen(desktop_file, "w");
    if (!fp) return false;

    fprintf(fp, "[Desktop Entry]\n");
    fprintf(fp, "Type=Application\n");
    fprintf(fp, "Name=%s Handler\n", scheme_name);
    fprintf(fp, "Exec=%s %%u\n", app_path);
    fprintf(fp, "StartupNotify=false\n");
    fprintf(fp, "MimeType=x-scheme-handler/%s;\n", scheme_name);
    fclose(fp);

    char cmd[PATH_MAX + 64];
    snprintf(cmd, sizeof(cmd), "xdg-mime default %s.desktop x-scheme-handler/%s 2>/dev/null", scheme_name, scheme_name);
    system(cmd);
    return true;
}

bool os_unregister_protocol(const char *scheme_name) {
    char *home = getenv("HOME");
    if (!home) return false;
    char desktop_file[PATH_MAX];
    snprintf(desktop_file, sizeof(desktop_file), "%s/.local/share/applications/%s.desktop", home, scheme_name);
    return (remove(desktop_file) == 0);
}

void os_pause(void) {
    printf("Press [Enter] to continue . . . ");
    fflush(stdout);
    getchar();
}

void os_clear_screen(void) {
    printf("\033[H\033[J");
    fflush(stdout);
}

#endif // !_WIN32
