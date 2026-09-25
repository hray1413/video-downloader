# 📥 My Video Downloader (YT-DLP Win Batch Wrapper)

一個超強、全自動、零依賴（不用裝 Python！）的 Windows 影片下載批次檔工具。基於 [yt-dlp](https://github.com/yt-dlp/yt-dlp) 與 [FFmpeg](https://www.gyan.dev/ffmpeg/builds/) 核心，為追求極致效率與簡單直覺的使用者而生。

---

## ✨ 核心特色

- **零依賴環境**：不需要手動安裝 Python。第一次執行時會**自動下載與更新**最新的 `yt-dlp.exe` 與 `ffmpeg.exe`。
- **智慧剪貼簿偵測**：開啟後自動讀取剪貼簿中的影片網址，按個 Enter 就能直接下載，懶人福音。
- **多平台支援**：完美支援 YouTube（含播放清單、Shorts）、Bilibili、TikTok、X (Twitter)、Facebook 等主流平台。
- **豐富的格式選擇**：
  - 支援自動最高畫質（4K/2K/1080p）合併，並自動內嵌字幕與封面。
  - 提供 1080p、720p 限制選項。
  - 支援高音質純音樂提取（MP3 320k / M4A）與完整播放清單打包。
- **進階功能**：
  - 支援 `cookies.txt` 以下載會員或受限制影片。
  - 自動偵測並啟用本地或系統的 Node.js / Deno 執行環境（用於繞過複雜的反爬蟲驗證）。
  - 支援自訂協議喚起與命令列快速參數。

---

## 🚀 快速開始

1. 下載本專案的 `dl.bat` 到你的電腦中。
2. 雙擊執行 `dl.bat`。
3. 程式會自動檢查並下載必要的核心工具（`yt-dlp` 與 `ffmpeg`）。
4. 依提示貼上網址，或直接按下 Enter 使用剪貼簿中的網址，輕鬆完成下載！

---

## ⌨️ 命令列與進階用法

如果你喜歡用命令列（CLI），它也支援多種快捷參數：

:: 1. 互動模式（直接執行）
```batch
dl.bat
```

:: 2. 命令列快速下載（網址 + 格式選項 1-6）
```batch
dl.bat "[https://www.youtube.com/watch?v=xxx](https://www.youtube.com/watch?v=xxx)" 1
```
:: 3. 更新核心工具 (yt-dlp)
```batch
dl.bat -u
```
:: 4. 查看幫助與完整說明
```batch
dl.bat --help
```

---
# 歡迎大佬接入GUI，因為我GUI很爛，需要各位幫忙接入。萬分感謝!!!
