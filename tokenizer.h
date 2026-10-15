#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 將命令列字串解析為標準 argv[] 陣列
// 正確支援引號 ("...", '...'), 空白保護, 反斜線轉義
bool cmdline_tokenize(const char *cmd_line, char ***out_argv, int *out_argc);

// 釋放 tokenize 產生的 argv 陣列
void cmdline_free_tokens(char **argv, int argc);

#ifdef __cplusplus
}
#endif

#endif // TOKENIZER_H
