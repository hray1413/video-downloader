#ifndef ERRORS_H
#define ERRORS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VD_OK = 0,
    VD_ERR_INVALID_URL        = 1001,
    VD_ERR_TOOL_YTDLP_MISSING = 1002,
    VD_ERR_TOOL_FFMPEG_MISSING= 1003,
    VD_ERR_DOWNLOAD_FAILED    = 1004,
    VD_ERR_YTDLP_EXEC_FAIL    = 1005,
    VD_ERR_YTDLP_EXIT_ERROR   = 1006,
    VD_ERR_IPC_FAILED         = 1007,
    VD_ERR_FS_IO              = 1008,
    VD_ERR_URL_FILE_NOT_FOUND = 1009,
    VD_ERR_PROTOCOL_REG_FAIL  = 1010
} vd_error_t;

// 取得錯誤代碼字串名稱
const char *vd_error_name(vd_error_t err);

// 取得錯誤的人類可讀描述
const char *vd_error_message(vd_error_t err);

// 取得錯誤的具體排查診斷建議
const char *vd_error_troubleshoot(vd_error_t err);

#ifdef __cplusplus
}
#endif

#endif // ERRORS_H
