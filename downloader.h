#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 下載遠端檔案至本機路徑
// 自動優先使用系統 curl，若不可用則自動調用作業系統原生網路後端 (WinINet / POSIX)
bool downloader_fetch(const char *url, const char *dest_path);

// 檢測並自動下載 yt-dlp
bool downloader_ensure_ytdlp(const char *app_dir);

// 檢測並自動下載/解壓縮 ffmpeg 與 ffprobe
bool downloader_ensure_ffmpeg(const char *app_dir);

#ifdef __cplusplus
}
#endif

#endif // DOWNLOADER_H
