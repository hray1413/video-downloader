#ifndef INTERACTIVE_MODE_H
#define INTERACTIVE_MODE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 執行互動式 TUI 主迴圈
int interactive_mode_run(const char *app_dir, const char *custom_out_dir, const char *extra_opts);

#ifdef __cplusplus
}
#endif

#endif // INTERACTIVE_MODE_H
