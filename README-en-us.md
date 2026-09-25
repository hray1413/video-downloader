# 📥 My Video Downloader (YT-DLP Win Batch Wrapper)

A powerful, fully automatic, zero-dependency (no Python required!) Windows video batch downloader. Based on the [yt-dlp](https://github.com/yt-dlp/yt-dlp) and [FFmpeg](https://www.gyan.dev/ffmpeg/builds/) core, it's designed for users seeking ultimate efficiency and intuitive simplicity.

---

## ✨ Core Features

- **Zero-dependency environment**: No need to manually install Python. The first time it's run, it automatically downloads and updates the latest `yt-dlp.exe` and `ffmpeg.exe`.
- **Smart clipboard detection**: Automatically reads video URLs from the clipboard after enabling; simply press Enter to download directly—a boon for lazy users.
- **Multi-platform support**: Perfectly supports YouTube (including playlists, Shorts), Bilibili, TikTok, X (Twitter), Facebook, and other mainstream platforms.
- **Rich Format Selection**:
  - Supports automatic highest quality (4K/2K/1080p) merge with embedded subtitles and cover art.
  - Provides 1080p and 720p limitation options.
  - Supports high-quality audio extraction (MP3 320k / M4A) and full playlist bundling.
- **Advanced Features**:
  - Supports `cookies.txt` for downloading members-only or restricted videos.
  - Automatically detects and enables local or system Node.js / Deno runtimes (used to bypass complex anti-bot verifications).
  - Supports custom protocol invocation and CLI quick parameters.

---

## 🚀 Quick Start

1. Download `dl.bat` from this repository to your computer.
2. Double-click to run `dl.bat`.
3. The script will automatically check and download the required core tools (`yt-dlp` and `ffmpeg`).
4. Follow the prompt to paste a URL, or simply press Enter to use the URL in your clipboard for a seamless download!

---

## ⌨️ Command Line & Advanced Usage

If you prefer the command line (CLI), various shortcut arguments are supported:

:: 1. Interactive mode (run directly)
```batch
dl.bat
```
:: 2. Quick CLI download (URL + format option 1-6)
```batch
dl.bat "[https://www.youtube.com/watch?v=xxx](https://www.youtube.com/watch?v=xxx)" 1
```
:: 3. Update core tools (yt-dlp)
``` batch
dl.bat -u
```
:: 4. View help and full documentation
```batch
dl.bat --help
```

---
# GUI Contributors Welcome! My GUI skills are rusty, so feel free to pull request or integrate a frontend. Huge thanks! 🙏
