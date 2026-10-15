#include "errors.h"

const char *vd_error_name(vd_error_t err) {
    switch (err) {
        case VD_OK:                     return "OK";
        case VD_ERR_INVALID_URL:        return "INVALID_URL";
        case VD_ERR_TOOL_YTDLP_MISSING: return "TOOL_YTDLP_MISSING";
        case VD_ERR_TOOL_FFMPEG_MISSING:return "TOOL_FFMPEG_MISSING";
        case VD_ERR_DOWNLOAD_FAILED:    return "DOWNLOAD_FAILED";
        case VD_ERR_YTDLP_EXEC_FAIL:    return "YTDLP_EXEC_FAIL";
        case VD_ERR_YTDLP_EXIT_ERROR:   return "YTDLP_EXIT_ERROR";
        case VD_ERR_IPC_FAILED:         return "IPC_FAILED";
        case VD_ERR_FS_IO:              return "FS_IO_ERROR";
        case VD_ERR_URL_FILE_NOT_FOUND: return "URL_FILE_NOT_FOUND";
        case VD_ERR_PROTOCOL_REG_FAIL:  return "PROTOCOL_REG_FAIL";
        default:                        return "UNKNOWN_ERROR";
    }
}

const char *vd_error_message(vd_error_t err) {
    switch (err) {
        case VD_OK:                     return "Success";
        case VD_ERR_INVALID_URL:        return "Invalid or unparseable video URL provided";
        case VD_ERR_TOOL_YTDLP_MISSING: return "yt-dlp core binary not found and auto-download failed";
        case VD_ERR_TOOL_FFMPEG_MISSING:return "ffmpeg/ffprobe core binary not found and auto-download failed";
        case VD_ERR_DOWNLOAD_FAILED:    return "Network error while fetching core tools archive";
        case VD_ERR_YTDLP_EXEC_FAIL:    return "Failed to spawn yt-dlp child process";
        case VD_ERR_YTDLP_EXIT_ERROR:   return "yt-dlp exited with non-zero status code";
        case VD_ERR_IPC_FAILED:         return "Failed to write event to specified Named Pipe or FD";
        case VD_ERR_FS_IO:              return "Filesystem I/O error (permission denied or path creation failed)";
        case VD_ERR_URL_FILE_NOT_FOUND: return "Specified URL file does not exist or cannot be opened";
        case VD_ERR_PROTOCOL_REG_FAIL:  return "Failed to register custom protocol in operating system";
        default:                        return "An unexpected error occurred";
    }
}

const char *vd_error_troubleshoot(vd_error_t err) {
    switch (err) {
        case VD_ERR_INVALID_URL:
            return "Please ensure the URL starts with http://, https://, or www.";
        case VD_ERR_TOOL_YTDLP_MISSING:
        case VD_ERR_TOOL_FFMPEG_MISSING:
        case VD_ERR_DOWNLOAD_FAILED:
            return "Check your internet connection/proxy settings or manually place binaries into app folder.";
        case VD_ERR_YTDLP_EXIT_ERROR:
            return "1. Run with '-u' to update yt-dlp core\n2. If video is restricted/member-only, place cookies.txt in app folder\n3. Check proxy or rate limits.";
        case VD_ERR_URL_FILE_NOT_FOUND:
            return "Verify the path passed to VIDEODL_URL_FILE exists and is readable.";
        default:
            return "Check program logs and system permissions.";
    }
}
