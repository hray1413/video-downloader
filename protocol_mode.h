#ifndef PROTOCOL_MODE_H
#define PROTOCOL_MODE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 註冊 videodl:// 協議至作業系統 (Windows 註冊表 / Linux .desktop)
int protocol_mode_install(const char *app_dir);

// 從作業系統解除註冊 videodl:// 協議
int protocol_mode_uninstall(void);

// 判斷是否為自定義協議調用並提取真實 URL
bool protocol_mode_extract(const char *input_arg, char *out_url, size_t max_size);

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_MODE_H
