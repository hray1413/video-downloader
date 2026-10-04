#ifndef DOWNLOAD_FLOW_H
#define DOWNLOAD_FLOW_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *url;
    const char *format_choice;       // "1" 到 "6"
    const char *custom_output_dir;   // 可為 NULL
    const char *extra_opts;          // 自訂附加選項，可為 NULL
    const char *app_dir;             // 應用程式目錄
    char out_full_dir[1024];         // 輸出實際儲存資料夾路徑
} DownloadRequest;

// 執行影片/音樂下載流程，返回 exit_code (0 = 成功)
int download_flow_execute(DownloadRequest *req);

#ifdef __cplusplus
}
#endif

#endif // DOWNLOAD_FLOW_H
