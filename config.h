#ifndef CONFIG_H
#define CONFIG_H

// ==========================================
// 軟體版本與識別資訊
// ==========================================
#define APP_NAME            "YT-DLP Video Downloader"
#define APP_VERSION         "3.6.0"
#define APP_AUTHOR          "hray1413"
#define APP_EMAIL           "videodownload@ss2256.cc.cd"
#define PROTOCOL_SCHEME     "videodl"

// ==========================================
// 預設路徑與參數設定
// ==========================================
#define DEFAULT_OUTPUT_DIR  "videos"
#define DEFAULT_RETRY_COUNT 10
#define DEFAULT_CONCURRENT  4
#define FILENAME_MAX_LEN    150

// ==========================================
// 緩衝區上限規格
// ==========================================
#define MAX_URL_LENGTH      2048
#define MAX_PATH_LENGTH     1024
#define MAX_CMD_LENGTH      8192
#define MAX_EVENT_BUF_LEN   8192

#endif // CONFIG_H
