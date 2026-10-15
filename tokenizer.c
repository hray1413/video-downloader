#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "tokenizer.h"

bool cmdline_tokenize(const char *cmd_line, char ***out_argv, int *out_argc) {
    if (!cmd_line || !out_argv || !out_argc) return false;

    *out_argv = NULL;
    *out_argc = 0;

    int capacity = 16;
    char **argv = (char **)malloc(sizeof(char *) * capacity);
    if (!argv) return false;

    int argc = 0;
    const char *p = cmd_line;

    char token_buf[4096];
    int token_len = 0;

    while (*p != '\0') {
        // 跳過空白
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
            p++;
        }
        if (*p == '\0') break;

        token_len = 0;
        char in_quote = 0; // 0, '"', 或 '\''

        while (*p != '\0') {
            if (in_quote) {
                if (*p == in_quote) {
                    in_quote = 0;
                    p++;
                } else if (*p == '\\' && in_quote == '\"' && (*(p + 1) == '\"' || *(p + 1) == '\\')) {
                    if (token_len < (int)sizeof(token_buf) - 1) {
                        token_buf[token_len++] = *(p + 1);
                    }
                    p += 2;
                } else {
                    if (token_len < (int)sizeof(token_buf) - 1) {
                        token_buf[token_len++] = *p;
                    }
                    p++;
                }
            } else {
                if (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
                    break;
                } else if (*p == '\"' || *p == '\'') {
                    in_quote = *p;
                    p++;
                } else if (*p == '\\' && (*(p + 1) == '\"' || *(p + 1) == '\'' || *(p + 1) == ' ' || *(p + 1) == '\\')) {
                    if (token_len < (int)sizeof(token_buf) - 1) {
                        token_buf[token_len++] = *(p + 1);
                    }
                    p += 2;
                } else {
                    if (token_len < (int)sizeof(token_buf) - 1) {
                        token_buf[token_len++] = *p;
                    }
                    p++;
                }
            }
        }

        token_buf[token_len] = '\0';

        if (argc >= capacity - 1) {
            capacity *= 2;
            char **new_argv = (char **)realloc(argv, sizeof(char *) * capacity);
            if (!new_argv) {
                cmdline_free_tokens(argv, argc);
                return false;
            }
            argv = new_argv;
        }

        argv[argc] = _strdup(token_buf);
        if (!argv[argc]) {
            cmdline_free_tokens(argv, argc);
            return false;
        }
        argc++;
    }

    argv[argc] = NULL;
    *out_argv = argv;
    *out_argc = argc;
    return true;
}

void cmdline_free_tokens(char **argv, int argc) {
    if (!argv) return;
    for (int i = 0; i < argc; ++i) {
        if (argv[i]) free(argv[i]);
    }
    free(argv);
}
