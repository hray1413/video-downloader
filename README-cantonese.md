# 📥 我嘅影片下載器 (YT-DLP Win Batch Wrapper)

一個超強、全自動、零依賴（唔使裝 Python！）嘅 Windows 影片下載批次檔工具。基於 [yt-dlp](https://github.com/yt-dlp/yt-dlp) 同 [FFmpeg](https://www.gyan.dev/ffmpeg/builds/) 核心，為追求極致效率同簡單直覺嘅使用者而生。

---

## ✨ 核心特色

- **零依賴環境**：唔需要手動安裝 Python。第一次執行時會自動下載同更新最新嘅 `yt-dlp.exe` 同 `ffmpeg.exe`[cite: 1]。
- **智慧剪貼簿偵測**：開啟後自動讀取剪貼簿中嘅影片網址，按個 Enter 就可以直接下載，懶人福音[cite: 1]。
- **多平台支援**：完美支援 YouTube（包括播放清單、Shorts）、Bilibili、TikTok、X (Twitter)、Facebook 等主流平台[cite: 1]。
- **豐富嘅格式選擇**：
  - 支援自動最高畫質（4K/2K/1080p）合併，並自動內嵌字幕同封面[cite: 1]。
  - 提供 1080p、720p 限制選項[cite: 1]。
  - 支援高音質純音樂提取（MP3 320k / M4A）同完整播放清單打包[cite: 1]。
- **進階功能**：
  - 支援 `cookies.txt` 以下載會員或受限制影片[cite: 1]。
  - 自動偵測並啟用本地或系統嘅 Node.js / Deno 執行環境（用於繞過複雜嘅反爬蟲驗證）[cite: 1]。
  - 支援自訂協議喚起同命令列快速參數[cite: 1]。

---

## 🚀 快速開始

1. 下載本專案嘅 `dl.bat` 去你嘅電腦入面。
2. 雙擊執行 `dl.bat`。
3. 程式會自動檢查並下載必要嘅核心工具（`yt-dlp` 同 `ffmpeg`）[cite: 1]。
4. 按提示貼上網址，或者直接按下 Enter 使用剪貼簿中嘅網址，輕鬆完成下載！

---

## ⌨️ 命令列同進階用法

如果你鍾意用命令列（CLI），佢亦支援多種快捷參數：

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
:: 4. 查看幫助同完整說明
```batch
dl.bat --help
```
---
# 歡迎大佬接入 GUI，因為我 GUI 好爛，需要各位幫手接入。萬分感謝！！！ 🙏

dl.bat
