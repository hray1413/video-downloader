#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "url_parser.h"

bool url_is_valid(const char *raw_url) {
    if (!raw_url) return false;
    while (*raw_url == ' ' || *raw_url == '\t' || *raw_url == '\"' || *raw_url == '\'' || *raw_url == '\\') {
        raw_url++;
    }
    if (_strnicmp(raw_url, "http://", 7) == 0 ||
        _strnicmp(raw_url, "https://", 8) == 0 ||
        _strnicmp(raw_url, "www.", 4) == 0 ||
        _strnicmp(raw_url, "videodl:", 8) == 0 ||
        _strnicmp(raw_url, "videodl://", 10) == 0) {
        return true;
    }
    return false;
}

void url_normalize(char *url_buf, size_t max_size, char *out_format_choice) {
    if (!url_buf || max_size == 0) return;

    // 1. 去除首尾的引號與多餘空白
    int src = 0;
    while (url_buf[src] == ' ' || url_buf[src] == '\t' || url_buf[src] == '\"' || url_buf[src] == '\'' || url_buf[src] == '\\') {
        src++;
    }

    char clean[2048] = { 0 };
    int dst = 0;
    while (url_buf[src] != '\0' && dst < (int)sizeof(clean) - 1) {
        if (url_buf[src] != '\"' && url_buf[src] != '\'') {
            clean[dst++] = url_buf[src];
        }
        src++;
    }
    while (dst > 0 && (clean[dst - 1] == ' ' || clean[dst - 1] == '\t' || clean[dst - 1] == '\r' || clean[dst - 1] == '\n' || clean[dst - 1] == '\\')) {
        clean[--dst] = '\0';
    }

    // 2. 檢查尾部格式後綴 (例如 "... 2")
    if (dst >= 2 && clean[dst - 2] == ' ' && clean[dst - 1] >= '1' && clean[dst - 1] <= '6') {
        if (out_format_choice && out_format_choice[0] == '\0') {
            out_format_choice[0] = clean[dst - 1];
            out_format_choice[1] = '\0';
        }
        clean[dst - 2] = '\0';
        dst -= 2;
    }

    // 3. 剝除自定義 videodl:// 協議頭
    char *p = clean;
    if (_strnicmp(p, "videodl:///", 11) == 0) p += 11;
    else if (_strnicmp(p, "videodl://", 10) == 0) p += 10;
    else if (_strnicmp(p, "videodl:", 8) == 0) p += 8;
    while (*p == '/' || *p == '\\') p++;

    // 4. 修復瀏覽器/OS 遺失的協議冒號
    char fixed[2048] = { 0 };
    if (_strnicmp(p, "https//", 7) == 0) snprintf(fixed, sizeof(fixed), "https://%s", p + 7);
    else if (_strnicmp(p, "http//", 6) == 0) snprintf(fixed, sizeof(fixed), "http://%s", p + 6);
    else if (_strnicmp(p, "https/", 6) == 0) snprintf(fixed, sizeof(fixed), "https://%s", p + 6);
    else if (_strnicmp(p, "http/", 5) == 0) snprintf(fixed, sizeof(fixed), "http://%s", p + 5);
    else if (_strnicmp(p, "www.", 4) == 0) snprintf(fixed, sizeof(fixed), "https://%s", p);
    else strncpy(fixed, p, sizeof(fixed) - 1);

    strncpy(url_buf, fixed, max_size - 1);
    url_buf[max_size - 1] = '\0';
}
