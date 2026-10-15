#include "cli_mode.h"
#include "os_compat.h"

#define APP_TITLE "YT-DLP Video Downloader v3.6"

int main(int argc, char *argv[]) {
    // 1. 初始化平台環境 (控制台編碼、視窗標題)
    os_init(APP_TITLE);

    // 2. 轉發至 CLI 路由器處理
    return cli_mode_run(argc, argv);
}
