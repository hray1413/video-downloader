# YT-DLP Video Downloader v3.2 - UI 整合與參數調用指南

本專案提供彈性的命令列（CLI）與環境變數支援，方便開發者使用 Electron、Tauri 或其他前端框架來包裝圖形介面（GUI）。

---

## 🚀 啟動模式

`dl.bat` 支援三種運作模式：

1. **互動模式**：直接雙擊執行 `.bat`，透過終端機提示或剪貼簿自動偵測輸入網址。
2. **命令列模式（CLI）**：透過傳入參數快速執行下載。
3. **Electron / GUI 模式**：透過環境變數或暫存檔傳遞網址與格式，交由背景執行。

---

## ⚙️ 參數與調用方式

### 1. 命令列快速調用 (CLI Mode)

直接在命令列傳入「網址」與「格式代號」：

```cmd
dl.bat "https://www.youtube.com/watch?v=xxx" 1

```

### 2. Electron / GUI 模式參數

當你使用 Electron 的 `child_process.spawn` 調用時，推薦使用 **環境變數（`VIDEODL_URL_FILE`）** 的方式，避免命令列字元跳脫（Escape）或過長的問題：

* **環境變數**：`VIDEODL_URL_FILE`（指定一個存有目標網址的純文字檔案路徑）
* **命令列參數**：傳入格式代號（`%~1`）

**Node.js / Electron 實作範例：**

```javascript
const { spawn } = require('child_process');
const fs = require('fs');
const path = require('path');

// 1. 將網址寫入暫存檔
const urlFilePath = path.join(app.getPath('temp'), 'videodl_url.txt');
fs.writeFileSync(urlFilePath, 'https://www.youtube.com/watch?v=xxx', 'utf-8');

// 2. 帶入格式代號 '1'，並透過環境變數傳遞檔案路徑
const batProcess = spawn('cmd.exe', ['/c', 'dl.bat', '1'], {
    env: { ...process.env, VIDEODL_URL_FILE: urlFilePath }
});

batProcess.stdout.on('data', (data) => {
    console.log(`輸出: ${data}`);
});

```

亦支援直接傳入兩個參數的傳統檔案模式：

```cmd
dl.bat [格式代號] [網址檔案路徑]

```

---

## 📋 格式代號對照表 (`FORMAT_CHOICE`)

| 代號 | 格式與畫質說明 | 儲存路徑 |
| --- | --- | --- |
| **1** | 最佳畫質 (MP4, 自動最高 4K/2K/1080p, 內嵌字幕與封面) **[預設]** | `videos/` |
| **2** | 高清 1080p Max (MP4) | `videos/` |
| **3** | 標清 720p Max (MP4) | `videos/` |
| **4** | 高音質純音樂 (MP3 320k, 內嵌封面) | `videos/audio/mp3/` |
| **5** | 原生純音訊 (Best M4A, 內嵌封面) | `videos/audio/m4a/` |
| **6** | 整個播放清單 (Entire Playlist, MP4) | `videos/playlists/` |

---

## 🛠️ 其他常用指令

* **更新核心工具**：
```cmd
dl.bat -u

```


* **查看版本與作者資訊**：
```cmd
dl.bat about

```
