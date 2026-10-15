#ifndef DOWNLOAD_PROFILES_H
#define DOWNLOAD_PROFILES_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *choice_id;           // "1" 到 "6"
    const char *name;                // 描述名稱
    const char *default_subfolder;   // 預設相對輸出目錄
    const char *output_template;     // 檔名模板
    bool is_playlist;                // 是否為播放清單模式
    bool is_audio_extract;           // 是否抽取音訊
} DownloadProfile;

// 取得指定編號 (1-6) 的格式設定檔 (若傳入無效則預設返回 1 號 Best Quality)
const DownloadProfile *profile_get(const char *choice_id);

// 印出互動模式的格式選單
void profile_print_menu(void);

#ifdef __cplusplus
}
#endif

#endif // DOWNLOAD_PROFILES_H
