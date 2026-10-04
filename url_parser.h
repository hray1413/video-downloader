#ifndef URL_PARSER_H
#define URL_PARSER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 判斷是否為有效 URL (以 http://, https://, www. 或 videodl: 開頭)
bool url_is_valid(const char *raw_url);

// 正規化 URL 並安全提取可能附帶在結尾的 format 選項 (例如 "URL 2")
// 保證完整保留 URL 內部的 ?, &, =, #, % 等查詢字串與特殊字元
void url_normalize(char *url_buf, size_t max_size, char *out_format_choice);

#ifdef __cplusplus
}
#endif

#endif // URL_PARSER_H
