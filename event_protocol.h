#ifndef EVENT_PROTOCOL_H
#define EVENT_PROTOCOL_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EVENT_STATUS_INFO,
    EVENT_STATUS_OK,
    EVENT_STATUS_WARN,
    EVENT_STATUS_ERROR
} EventStatus;

// 初始化事件分發器 (解析 VIDEODL_JSON, VIDEODL_PIPE, VIDEODL_PIPE_FD 等)
void event_init(bool cli_json_flag);

// 發送標準化事件 (包含 event, status, code, message, data, timestamp)
void event_emit(const char *event_name, EventStatus status, int code, const char *message, const char *data_str);

// 簡化呼叫版本
void event_emit_simple(const char *event_name, EventStatus status, const char *message);

// 查詢目前是否啟用 JSON 模式
bool event_is_json_mode(void);

// 查詢目前是否為靜音模式
bool event_is_silent_mode(void);

#ifdef __cplusplus
}
#endif

#endif // EVENT_PROTOCOL_H
