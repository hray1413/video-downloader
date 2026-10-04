#ifndef OS_COMPAT_H
#define OS_COMPAT_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==========================================
// 1. 環境與主機初始化
// ==========================================
void os_init(const char *app_title);
void os_get_app_dir(char *out_dir, size_t max_size);
char os_path_separator(void);
void os_path_join(char *dest, size_t max_size, const char *dir, const char *subpath);

// ==========================================
// 2. 檔案系統與目錄
// ==========================================
bool os_file_exists(const char *path);
bool os_dir_exists(const char *path);
bool os_mkdir_p(const char *path);
bool os_command_exists(const char *cmd_name);
void os_open_folder(const char *folder_path);

// ==========================================
// 3. 剪貼簿操作
// ==========================================
bool os_get_clipboard_text(char *out_text, size_t max_size);

// ==========================================
// 4. 進程調度 (嚴格 argv[] 規格)
// ==========================================
int os_spawn_process(const char *exe_path, const char **argv);

// ==========================================
// 5. IPC 與管道輸出 (Named Pipe / Unix Socket / FD)
// ==========================================
bool os_pipe_write(const char *pipe_path, int pipe_fd, const char *data, size_t len);

// ==========================================
// 6. 自訂協議註冊 (videodl://)
// ==========================================
bool os_register_protocol(const char *scheme_name, const char *app_path);
bool os_unregister_protocol(const char *scheme_name);

// ==========================================
// 7. 終端輔助操作
// ==========================================
void os_pause(void);
void os_clear_screen(void);

#ifdef __cplusplus
}
#endif

#endif // OS_COMPAT_H
