@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

:: ==========================================
:: Title and Initialization
:: ==========================================
title YT-DLP Video Downloader v3.3
echo ==========================================
echo    YT-DLP Video Downloader v3.3
echo ==========================================
echo.

:: ==========================================
:: GUI Interface Contract (All Modes)
:: ==========================================
:: This script exposes a stable interface for any GUI frontend.
:: Supported calling conventions:
::
::  [A] Electron / Desktop App Mode
::      - Method 1: Set env var VIDEODL_URL_FILE=<path_to_url_file>
::        then call: dl.bat <FORMAT_CHOICE>
::      - Method 2: dl.bat <FORMAT_CHOICE> <path_to_url_file>
::      - Exit codes: 0=success, 1=error (no interactive pause in this mode)
::      - Progress lines: prefixed with [Progress], [Success], [Failed], [Info], [Error]
::
::  [B] CLI / Script Mode
::      - dl.bat <URL> [FORMAT_CHOICE]
::      - dl.bat --json-output [URL] [FORMAT_CHOICE]   (enables JSON status lines)
::      - Exit codes: 0=success, 1=error
::
::  [C] WebSocket / Named-Pipe GUI Mode
::      - Set env var VIDEODL_PIPE=<named_pipe_path>  (e.g. \\.\pipe\videodl)
::      - Script will write JSON progress events to pipe in real-time
::      - Pipe message format: {"event":"progress","percent":42,"speed":"1.2MiB/s","eta":"00:30"}
::
::  [D] JSON Output Mode (any GUI that reads stdout)
::      - Activate by: set VIDEODL_JSON=1  OR pass --json-output as first arg
::      - All status lines output as: {"event":"...", "data":"..."}
::      - Events: init | tool_check | url_detected | format_selected |
::                progress | success | failed | update | about
::
::  [E] Electron IPC / Named-Pipe Progress Mode
::      - Set env var VIDEODL_PROGRESS_FILE=<path>
::      - Script appends each yt-dlp progress line to that file
::      - GUI polls or watches the file for real-time progress
::
::  [F] Custom Protocol (videodl://) Handler Mode
::      - Registered via install-protocol.bat
::      - Called as: dl.bat "videodl://https://..." [FORMAT]
::      - Already handled in PARSE_URL section
::
::  [G] Python / Tkinter / PyQt GUI Mode
::      - Launch via: python -c "import subprocess; subprocess.run(['dl.bat', url, fmt])"
::      - Or set VIDEODL_JSON=1 and parse stdout JSON for status updates
::
::  [H] PowerShell / WPF GUI Mode
::      - Launch as a Process with RedirectStandardOutput
::      - Set VIDEODL_JSON=1 to receive structured JSON from stdout
::
::  [I] Web / Browser Extension Mode
::      - Extension calls native messaging host which calls dl.bat
::      - Pass URL via VIDEODL_URL_FILE env var or CLI args
::      - Extension receives exit code and stdout from native host
::
::  [J] REST API / HTTP Server Mode (e.g. via Flask/FastAPI wrapper)
::      - HTTP server spawns dl.bat as subprocess
::      - Set VIDEODL_JSON=1 for structured progress on stdout
::      - HTTP server relays progress via SSE or WebSocket to web client
::
:: ==========================================
:: GUI Environment Variables Reference
:: ==========================================
::  VIDEODL_URL_FILE     Path to a .txt file containing the URL (one line)
::  VIDEODL_JSON         Set to "1" to enable JSON output mode on stdout
::  VIDEODL_PIPE         Named pipe path (short connection per event: open/write/close)
::  VIDEODL_PIPE_FD      Set to "3" to write events to handle 3 (persistent long connection)
::  VIDEODL_OUTPUT_DIR   Override default output directory
::  VIDEODL_EXTRA_OPTS   Append custom yt-dlp options (advanced override)
::  VIDEODL_NO_PAUSE     Set to "1" to suppress all pause prompts (useful for any GUI)
::  VIDEODL_SILENT       Set to "1" to suppress all echo output (JSON mode implied)
:: ==========================================

:: ==========================================
:: Mode Detection: GUI Environment Overrides
:: ==========================================
set "ELECTRON_MODE=0"
set "JSON_MODE=0"
set "PIPE_MODE=0"
set "PIPE_FD_MODE=0"
set "URL="
set "FORMAT_CHOICE="
set "URL_FILE="

:: [D] JSON Output Mode via env var
if /i "!VIDEODL_JSON!"=="1" set "JSON_MODE=1"

:: [C] Named Pipe Mode via env var (short connection per event)
if defined VIDEODL_PIPE set "PIPE_MODE=1"

:: [C2] Persistent Named Pipe Mode via FD 3
if /i "!VIDEODL_PIPE_FD!"=="3" set "PIPE_FD_MODE=1"
if /i "!VIDEODL_PIPE_FD!"=="1" set "PIPE_FD_MODE=1"

:: [H/G] No-pause override (any GUI should set this to avoid blocking)
set "NO_PAUSE=0"
if /i "!VIDEODL_NO_PAUSE!"=="1" set "NO_PAUSE=1"

:: [J] Custom output dir override
if defined VIDEODL_OUTPUT_DIR set "CUSTOM_OUTPUT_DIR=!VIDEODL_OUTPUT_DIR!"

:: Check about argument
if /i "%~1"=="about" goto SHOW_ABOUT
if /i "%~1"=="--about" goto SHOW_ABOUT
if /i "%~1"=="-about" goto SHOW_ABOUT
if /i "%~1"=="/about" goto SHOW_ABOUT

:: Check update arguments
if /i "%~1"=="-u" goto UPDATE_TOOLS
if /i "%~1"=="--update" goto UPDATE_TOOLS
if /i "%~1"=="/u" goto UPDATE_TOOLS

:: Check help arguments
if /i "%~1"=="-h" goto SHOW_INVALID_URL_HELP
if /i "%~1"=="--help" goto SHOW_INVALID_URL_HELP
if /i "%~1"=="/?" goto SHOW_INVALID_URL_HELP
if /i "%~1"=="help" goto SHOW_INVALID_URL_HELP

:: [D] JSON Output Mode via CLI flag --json-output
if /i "%~1"=="--json-output" (
    set "JSON_MODE=1"
    shift
)

:: [A] Electron Mode Detection (via VIDEODL_URL_FILE env or existing file in arg2)
if defined VIDEODL_URL_FILE (
    set "ELECTRON_MODE=1"
    set "NO_PAUSE=1"
    set "FORMAT_CHOICE=%~1"
    set "URL_FILE=%VIDEODL_URL_FILE%"
) else if not "%~2"=="" (
    if exist "%~2" (
        set "ELECTRON_MODE=1"
        set "NO_PAUSE=1"
        set "FORMAT_CHOICE=%~1"
        set "URL_FILE=%~2"
    ) else (
        rem CLI Mode: arg1=URL, arg2=FORMAT
        set "URL=%~1"
        set "FORMAT_CHOICE=%~2"
    )
) else if not "%~1"=="" (
    rem CLI Mode: arg1=URL, format interactive
    set "URL=%~1"
)

:: ==========================================
:: GUI Hook: INIT event
:: ==========================================
call :GUI_EVENT "init" "Script started, checking tools..."

:: ==========================================
:: 1. Check and Auto-download Core Tools
:: ==========================================
:CHECK_TOOLS

if not exist "%~dp0yt-dlp.exe" (
    call :DOWNLOAD_YTDLP
    if !errorlevel! neq 0 (
        call :GUI_EVENT "error" "Failed to get yt-dlp.exe"
        echo [Error] Failed to get yt-dlp.exe, exiting.
        if !NO_PAUSE! equ 0 pause
        exit /b 1
    )
)

if not exist "%~dp0ffmpeg.exe" (
    call :DOWNLOAD_FFMPEG
    if !errorlevel! neq 0 (
        call :GUI_EVENT "error" "Failed to get ffmpeg.exe"
        echo [Error] Failed to get ffmpeg.exe, exiting.
        if !NO_PAUSE! equ 0 pause
        exit /b 1
    )
)

call :GUI_EVENT "tool_check" "Tools OK"
goto CHECK_URL

:: ==========================================
:: Download yt-dlp.exe (subroutine)
:: ==========================================
:DOWNLOAD_YTDLP
echo [Info] yt-dlp.exe not found!
echo Downloading yt-dlp.exe ...
echo.

set "DL_SUCCESS=0"

:: Check if curl exists in system
where curl >nul 2>nul
if !errorlevel! equ 0 (
    echo [Info] curl found, downloading via curl...
    curl -L --retry 3 --connect-timeout 15 -o "%~dp0yt-dlp.exe" "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe"
    if exist "%~dp0yt-dlp.exe" set "DL_SUCCESS=1"
)

:: Fallback to PowerShell if curl is absent or download failed
if !DL_SUCCESS! equ 0 (
    echo [Info] Using PowerShell fallback download...
    powershell -Command "$ProgressPreference='SilentlyContinue'; [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; (New-Object Net.WebClient).DownloadFile('https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe', '%~dp0yt-dlp.exe')"
)

if exist "%~dp0yt-dlp.exe" (
    echo [Success] yt-dlp.exe downloaded!
    exit /b 0
) else (
    echo [Error] yt-dlp.exe download failed, please check network.
    echo Manual download: https://github.com/yt-dlp/yt-dlp/releases/latest
    exit /b 1
)

:: ==========================================
:: Download ffmpeg.exe and ffprobe.exe (subroutine)
:: ==========================================
:DOWNLOAD_FFMPEG
echo [Info] ffmpeg.exe not found!
echo Downloading ffmpeg archive...
echo.

set "TEMP_ZIP=%TEMP%\ffmpeg-release_%RANDOM%.zip"
set "EXTRACT_DIR=%TEMP%\ffmpeg_extract_%RANDOM%"
set "DL_SUCCESS=0"

:: Check if curl exists in system
where curl >nul 2>nul
if !errorlevel! equ 0 (
    echo [Info] curl found, downloading via curl...
    curl -L --retry 3 --connect-timeout 15 -o "%TEMP_ZIP%" "https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip"
    if exist "%TEMP_ZIP%" set "DL_SUCCESS=1"
)

:: Fallback to PowerShell if curl is absent or download failed
if !DL_SUCCESS! equ 0 (
    echo [Info] Using PowerShell fallback download...
    powershell -Command "$ProgressPreference='SilentlyContinue'; [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; (New-Object Net.WebClient).DownloadFile('https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip', '%TEMP_ZIP%')"
)

if not exist "%TEMP_ZIP%" (
    echo [Error] ffmpeg download failed, check network.
    echo Manual download: https://www.gyan.dev/ffmpeg/builds/
    exit /b 1
)

if exist "%EXTRACT_DIR%" rd /s /q "%EXTRACT_DIR%" 2>nul
echo Extracting ffmpeg.exe and ffprobe.exe...
powershell -Command "Add-Type -AssemblyName System.IO.Compression.FileSystem; [System.IO.Compression.ZipFile]::ExtractToDirectory('%TEMP_ZIP%', '%EXTRACT_DIR%')"

powershell -Command "Get-ChildItem -Path '%EXTRACT_DIR%' -Filter 'ffmpeg.exe' -Recurse | Select-Object -First 1 | Copy-Item -Destination '%~dp0ffmpeg.exe' -Force"
powershell -Command "Get-ChildItem -Path '%EXTRACT_DIR%' -Filter 'ffprobe.exe' -Recurse | Select-Object -First 1 | Copy-Item -Destination '%~dp0ffprobe.exe' -Force"

if exist "%EXTRACT_DIR%" rd /s /q "%EXTRACT_DIR%" 2>nul
if exist "%TEMP_ZIP%" del "%TEMP_ZIP%" 2>nul

if exist "%~dp0ffmpeg.exe" (
    echo [Success] ffmpeg.exe extracted!
    exit /b 0
) else (
    echo [Error] ffmpeg.exe extraction failed.
    echo Manual download: https://www.gyan.dev/ffmpeg/builds/
    echo Copy bin\ffmpeg.exe and bin\ffprobe.exe to this folder.
    exit /b 1
)

:: ==========================================
:: 2. Check URL
:: ==========================================
:CHECK_URL
if defined URL_FILE (
    rem [A] Electron/File mode: read URL from file
    if exist "!URL_FILE!" (
        set /p "URL=" < "!URL_FILE!"
    ) else (
        call :GUI_EVENT "error" "URL file not found: !URL_FILE!"
        echo [Error] URL file not found: !URL_FILE!
        if !NO_PAUSE! equ 0 pause
        exit /b 1
    )
) else if defined URL (
    rem Traditional mode: URL already provided via CLI args
) else (
    rem Interactive mode: prompt for input with clipboard auto-detection
    echo ==========================================
    echo Enter video URL, or run: %~nx0 [URL] [1-6]
    echo Tip: Enter [U] to update yt-dlp, [about] for author info.
    echo ==========================================
    echo.
    
    set "CLIP_URL="
    for /f "tokens=*" %%a in ('powershell -NoProfile -Command "Get-Clipboard" 2^>nul') do (
        if not defined CLIP_URL (
            set "TEMP_CLIP=%%a"
            if "!TEMP_CLIP:~0,4!"=="http" set "CLIP_URL=!TEMP_CLIP!"
            if "!TEMP_CLIP:~0,8!"=="videodl:" set "CLIP_URL=!TEMP_CLIP!"
        )
    )
    
    if defined CLIP_URL (
        echo [Detected URL in Clipboard]:
        echo !CLIP_URL!
        echo.
        set "USER_IN="
        set /p "USER_IN=Press [Enter] to use this URL, or paste a new URL: "
        if "!USER_IN!"=="" (
            set "URL=!CLIP_URL!"
        ) else (
            set "URL=!USER_IN!"
        )
    ) else (
        set /p "URL=Enter video URL: "
    )
)

:PARSE_URL
:: Strip double quotes if present
if defined URL set "URL=!URL:"=!"

:: Trim URL
if defined URL for /f "tokens=* delims= " %%a in ("!URL!") do set "URL=%%~a"

:: Check if empty
if "!URL!"=="" goto SHOW_INVALID_URL_HELP

:: Check if user requested special actions in interactive mode
if /i "!URL!"=="about" goto SHOW_ABOUT
if /i "!URL!"=="--about" goto SHOW_ABOUT
if /i "!URL!"=="-about" goto SHOW_ABOUT
if /i "!URL!"=="/about" goto SHOW_ABOUT
if /i "!URL!"=="u" goto UPDATE_TOOLS
if /i "!URL!"=="update" goto UPDATE_TOOLS
if /i "!URL!"=="--update" goto UPDATE_TOOLS
if /i "!URL!"=="help" goto SHOW_INVALID_URL_HELP
if /i "!URL!"=="--help" goto SHOW_INVALID_URL_HELP
if /i "!URL!"=="-h" goto SHOW_INVALID_URL_HELP
if /i "!URL!"=="/?" goto SHOW_INVALID_URL_HELP

:: Auto-strip videodl: protocol prefix if passed
if /i "!URL:~0,11!"=="videodl:///" set "URL=!URL:~11!"
if /i "!URL:~0,10!"=="videodl://" set "URL=!URL:~10!"
if /i "!URL:~0,8!"=="videodl:" set "URL=!URL:~8!"
if "!URL:~0,1!"=="/" set "URL=!URL:~1!"

:: Check for trailing format choice in input (e.g. "... 2")
set "LAST_TWO=!URL:~-2!"
if "!LAST_TWO!"==" 1" (
    if not defined FORMAT_CHOICE set "FORMAT_CHOICE=1"
    set "URL=!URL:~0,-2!"
) else if "!LAST_TWO!"==" 2" (
    if not defined FORMAT_CHOICE set "FORMAT_CHOICE=2"
    set "URL=!URL:~0,-2!"
) else if "!LAST_TWO!"==" 3" (
    if not defined FORMAT_CHOICE set "FORMAT_CHOICE=3"
    set "URL=!URL:~0,-2!"
) else if "!LAST_TWO!"==" 4" (
    if not defined FORMAT_CHOICE set "FORMAT_CHOICE=4"
    set "URL=!URL:~0,-2!"
) else if "!LAST_TWO!"==" 5" (
    if not defined FORMAT_CHOICE set "FORMAT_CHOICE=5"
    set "URL=!URL:~0,-2!"
) else if "!LAST_TWO!"==" 6" (
    if not defined FORMAT_CHOICE set "FORMAT_CHOICE=6"
    set "URL=!URL:~0,-2!"
)

:: Auto-restore missing colon caused by browser/OS URL normalization (https// -> https://, http// -> http://)
if /i "!URL:~0,7!"=="https//" set "URL=https://!URL:~7!"
if /i "!URL:~0,6!"=="http//" set "URL=http://!URL:~6!"
if /i "!URL:~0,6!"=="https/" set "URL=https://!URL:~6!"
if /i "!URL:~0,5!"=="http/" set "URL=http://!URL:~5!"

:: Check if valid URL format (starts with http://, https://, or www.)
set "IS_VALID_URL=0"
if "!URL:~0,7!"=="http://" set "IS_VALID_URL=1"
if "!URL:~0,8!"=="https://" set "IS_VALID_URL=1"
if "!URL:~0,4!"=="www." (
    set "URL=https://!URL!"
    set "IS_VALID_URL=1"
)

if "!IS_VALID_URL!"=="0" goto SHOW_INVALID_URL_HELP

call :GUI_EVENT "url_detected" "!URL!"

:: ==========================================
:: 3. Check Cookies & JS Runtime (Node/Deno)
:: ==========================================
set "COOKIE_OPTION="
if exist "%~dp0cookies.txt" (
    set COOKIE_OPTION=--cookies "%~dp0cookies.txt"
    echo [Info] cookies.txt found, downloading with login.
) else (
    echo [Info] No cookies.txt found, standard access mode.
)

:: Auto-detect JS Runtime: local deno.exe -> system deno -> system node
set "JS_PARAM="
if exist "%~dp0deno.exe" (
    set JS_PARAM=--js-runtimes "deno:%~dp0deno.exe"
    echo [Info] Local deno.exe found, enabling Deno JS runtime.
) else (
    where deno >nul 2>nul
    if !errorlevel! equ 0 (
        set "JS_PARAM=--js-runtimes deno"
        echo [Info] System Deno found, enabling Deno JS runtime.
    ) else (
        where node >nul 2>nul
        if !errorlevel! equ 0 (
            set "JS_PARAM=--js-runtimes node"
            echo [Info] System Node.js found, enabling Node.js runtime.
        )
    )
)
echo.

:: ==========================================
:: 4. Select Download Format
:: ==========================================
:SELECT_FORMAT
if defined FORMAT_CHOICE (
    goto APPLY_FORMAT
)

echo Select download format:
echo   [1] Video (MP4, Best quality - Auto 4K/2K/1080p, Embedded Subs & Cover) [Default]
echo   [2] Video (MP4, 1080p Max)
echo   [3] Video (MP4, 720p Max)
echo   [4] Audio only (MP3 320k, with Cover Art)
echo   [5] Audio only (Best M4A, with Cover Art)
echo   [6] Entire Playlist (MP4, organized in playlist folder)
echo.
set /p "FORMAT_CHOICE=Enter choice (1-6, default 1): "

for /f "tokens=* delims= " %%a in ("!FORMAT_CHOICE!") do set "FORMAT_CHOICE=%%a"
if "!FORMAT_CHOICE!"=="" set "FORMAT_CHOICE=1"

:APPLY_FORMAT
set "PLAYLIST_PARAM=--no-playlist"
set "EXTRA_OPTS=--embed-metadata --windows-filenames --trim-filenames 150 -N 4"

:: [J] Allow GUI to inject custom yt-dlp options via env var
if defined VIDEODL_EXTRA_OPTS set "EXTRA_OPTS=!EXTRA_OPTS! !VIDEODL_EXTRA_OPTS!"

if "!FORMAT_CHOICE!"=="1" (
    set FORMAT_PARAM=-f bv*+ba/b -S "res,ext:mp4:m4a"
    set "MERGE_PARAM=--merge-output-format mp4"
    set "OUTPUT_DIR=videos"
    set MEDIA_OPTS=--embed-thumbnail --embed-subs --sub-langs "zh-Hans,zh-Hant,zh,en.*"
) else if "!FORMAT_CHOICE!"=="2" (
    set FORMAT_PARAM=-f bv*[height<=1080]+ba/b[height<=1080] -S "res:1080,ext:mp4:m4a"
    set "MERGE_PARAM=--merge-output-format mp4"
    set "OUTPUT_DIR=videos"
    set MEDIA_OPTS=--embed-thumbnail --embed-subs --sub-langs "zh-Hans,zh-Hant,zh,en.*"
) else if "!FORMAT_CHOICE!"=="3" (
    set FORMAT_PARAM=-f bv*[height<=720]+ba/b[height<=720] -S "res:720,ext:mp4:m4a"
    set "MERGE_PARAM=--merge-output-format mp4"
    set "OUTPUT_DIR=videos"
    set MEDIA_OPTS=--embed-thumbnail --embed-subs --sub-langs "zh-Hans,zh-Hant,zh,en.*"
) else if "!FORMAT_CHOICE!"=="4" (
    set FORMAT_PARAM=-f bestaudio --extract-audio --audio-format mp3 --audio-quality 0
    set "MERGE_PARAM="
    set "OUTPUT_DIR=videos\audio\mp3"
    set MEDIA_OPTS=--embed-thumbnail --convert-thumbnails jpg
) else if "!FORMAT_CHOICE!"=="5" (
    rem Prefer native M4A (AAC) without re-encoding; fallback to best audio remuxed/converted to m4a
    set FORMAT_PARAM=-f "ba[ext=m4a]/bestaudio" -x --audio-format m4a
    set "MERGE_PARAM="
    set "OUTPUT_DIR=videos\audio\m4a"
    set MEDIA_OPTS=--embed-thumbnail --convert-thumbnails jpg
) else if "!FORMAT_CHOICE!"=="6" (
    set "PLAYLIST_PARAM=--yes-playlist"
    set FORMAT_PARAM=-f bv*+ba/b -S "res,ext:mp4:m4a"
    set "MERGE_PARAM=--merge-output-format mp4"
    set "OUTPUT_DIR=videos\playlists"
    set MEDIA_OPTS=--embed-thumbnail --embed-subs --sub-langs "zh-Hans,zh-Hant,zh,en.*"
) else (
    echo [Warning] Invalid choice, using default [1] Best quality.
    set "FORMAT_CHOICE=1"
    set FORMAT_PARAM=-f bv*+ba/b -S "res,ext:mp4:m4a"
    set "MERGE_PARAM=--merge-output-format mp4"
    set "OUTPUT_DIR=videos"
    set MEDIA_OPTS=--embed-thumbnail --embed-subs --sub-langs "zh-Hans,zh-Hant,zh,en.*"
)

:: [J] GUI output dir override takes precedence
if defined CUSTOM_OUTPUT_DIR set "OUTPUT_DIR=!CUSTOM_OUTPUT_DIR!"

set "OUT_SUBPATH=%%(title)s [%%(id)s].%%(ext)s"
if "!FORMAT_CHOICE!"=="6" set "OUT_SUBPATH=%%(playlist_title|Unknown)s\%%(playlist_index|0)s - %%(title)s [%%(id)s].%%(ext)s"

:: Create output folder
if not exist "%~dp0!OUTPUT_DIR!" mkdir "%~dp0!OUTPUT_DIR!" 2>nul

call :GUI_EVENT "format_selected" "Format=!FORMAT_CHOICE! OutputDir=!OUTPUT_DIR!"

:: ==========================================
:: 5. Execute Download
:: ==========================================
echo.
echo ==========================================
echo Starting download...
echo URL: !URL!
echo Output Directory: !OUTPUT_DIR!
echo ==========================================
echo.

:: Progress tracking: pass --newline so yt-dlp flushes stdout per line.
:: The GUI / caller should redirect stdout to a pipe or file externally if needed.
:: Example (PowerShell):
::   $proc = Start-Process dl.bat -ArgumentList "1","url.txt" -RedirectStandardOutput "progress.log" -NoNewWindow -PassThru
::   # Then tail progress.log in your GUI

:: Run yt-dlp
"%~dp0yt-dlp.exe" !COOKIE_OPTION! !JS_PARAM! !PLAYLIST_PARAM! --ffmpeg-location "%~dp0." !FORMAT_PARAM! !MERGE_PARAM! !MEDIA_OPTS! !EXTRA_OPTS! --retries 10 --fragment-retries 10 --newline -o "%~dp0!OUTPUT_DIR!\!OUT_SUBPATH!" "!URL!"

set "EXIT_CODE=!errorlevel!"

:: ==========================================
:: 6. Result Handling
:: ==========================================
if !EXIT_CODE! equ 0 goto DOWNLOAD_SUCCESS
goto DOWNLOAD_FAILED

:DOWNLOAD_SUCCESS
echo.
echo ==========================================
echo [Success] Download completed successfully!
echo Saved to: %~dp0!OUTPUT_DIR!
echo ==========================================
echo.

call :GUI_EVENT "success" "Saved to %~dp0!OUTPUT_DIR!"

if !NO_PAUSE! equ 1 goto DOWNLOAD_FINISH

echo Quick Actions:
echo   [O] Open videos folder
echo   [C] Continue downloading another video
echo   [Enter] Exit
echo.
set "POST_ACTION="
set /p "POST_ACTION=Select action (O/C/Enter): "
if /i "!POST_ACTION!"=="o" (
    start "" "%~dp0!OUTPUT_DIR!"
    goto DOWNLOAD_FINISH
) else if /i "!POST_ACTION!"=="c" (
    set "URL="
    set "FORMAT_CHOICE="
    cls
    goto CHECK_URL
)
goto DOWNLOAD_FINISH

:DOWNLOAD_FAILED
echo.
echo ==========================================
echo [Failed] Download encountered an error (code: !EXIT_CODE!)
echo ==========================================
echo.
echo Troubleshooting tips:
echo   1. Update core: run ".\dl.bat -u"
echo   2. Restricted/Member video: place cookies.txt in folder
echo   3. Check your network or proxy connection
echo.

call :GUI_EVENT "failed" "Exit code !EXIT_CODE!"

if !NO_PAUSE! equ 1 goto DOWNLOAD_FINISH

echo Quick Actions:
echo   [R] Retry download
echo   [Enter] Exit
echo.
set "POST_ACTION="
set /p "POST_ACTION=Select action (R/Enter): "
if /i "!POST_ACTION!"=="r" (
    cls
    goto APPLY_FORMAT
)
goto DOWNLOAD_FINISH

:DOWNLOAD_FINISH
if !NO_PAUSE! equ 0 pause
exit /b !EXIT_CODE!

:: ==========================================
:: 7. Update Tools (subroutine)
:: ==========================================
:UPDATE_TOOLS
echo ==========================================
echo    Checking and updating yt-dlp...
echo ==========================================
echo.
call :GUI_EVENT "update" "Updating yt-dlp..."
if exist "%~dp0yt-dlp.exe" (
    "%~dp0yt-dlp.exe" -U
) else (
    call :DOWNLOAD_YTDLP
)
echo.
if !NO_PAUSE! equ 0 pause
exit /b 0

:: ==========================================
:: 8. Show About Info
:: ==========================================
:SHOW_ABOUT
echo ==========================================
echo    YT-DLP Video Downloader
echo ==========================================
echo.
echo 製作者: hray1413
echo 郵箱: videodownload@ss2256.cc.cd
echo.
echo ==========================================
call :GUI_EVENT "about" "v3.3 by hray1413"
if !NO_PAUSE! equ 0 pause
exit /b 0

:: ==========================================
:: 9. Invalid URL Help / Usage Guide
:: ==========================================
:SHOW_INVALID_URL_HELP
echo.
echo ================================================================
echo [說明 / 使用指南] 請輸入有效的影片網址 (URL)！
echo ================================================================
echo.
echo 說明:
echo   您輸入的內容不是有效的網址，或未輸入任何網址。
echo   請確保網址以 http://、https:// 或 www. 開頭。
echo   若使用 videodl:// 協議喚起，系統已自動支援修復並補全冒號。
echo.
echo 支援平台:
echo   • YouTube (影片、播放清單、Shorts、音樂)
echo   • Bilibili (嗶哩嗶哩)
echo   • Facebook、Twitter (X)、TikTok、Instagram 等主流平台
echo.
echo 使用範例:
echo   1. 互動模式:
echo      直接執行 %~nx0，依提示貼上網址
echo   2. 命令列快速下載:
echo      %~nx0 "https://www.youtube.com/watch?v=xxx" [格式 1-6]
echo   3. 自訂協議喚起:
echo      %~nx0 "videodl://https://www.youtube.com/watch?v=xxx"
echo   4. 檢查並更新核心:
echo      %~nx0 -u
echo   5. 查看作者與版本資訊:
echo      %~nx0 about
echo   6. JSON 輸出模式 (GUI 整合):
echo      set VIDEODL_JSON=1 ^& %~nx0 "URL" [格式]
echo   7. 無暫停模式 (任何 GUI 皆適用):
echo      set VIDEODL_NO_PAUSE=1 ^& %~nx0 "URL" [格式]
echo.
echo 格式說明:
echo   [1] 最佳畫質 (MP4, 自動最高 4K/2K/1080p, 內嵌字幕海報) [預設]
echo   [2] 高清 1080p Max (MP4)
echo   [3] 標清 720p Max (MP4)
echo   [4] 高音質純音樂 (MP3 320k, 內嵌封面)
echo   [5] 原生純音訊 (Best M4A, 內嵌封面)
echo   [6] 整個播放清單 (Entire Playlist)
echo ================================================================
echo.
call :GUI_EVENT "error" "Invalid URL or help requested"
if !NO_PAUSE! equ 1 exit /b 1
if not "%~1"=="" (
    pause
    exit /b 1
)
rem Interactive mode: allow user to re-enter URL
set "RETRY_URL="
set /p "RETRY_URL=請重新輸入影片網址 (或直接按 Enter 結束): "
if "!RETRY_URL!"=="" exit /b 1
set "URL=!RETRY_URL!"
cls
goto PARSE_URL

:: ==========================================
:: GUI_EVENT Subroutine
:: ==========================================
:: Unified event emitter for all GUI integration modes.
:: Usage: call :GUI_EVENT <event_name> <data_string>
::
:: Outputs based on active modes:
::  JSON_MODE=1    -> prints {"event":"<name>","data":"<data>"} to stdout
::  PIPE_FD_MODE=1 -> writes same JSON to persistent handle 3 (long connection)
::  PIPE_MODE=1    -> writes same JSON to named pipe VIDEODL_PIPE (short connection)
::  default        -> no extra output (regular echo lines already present)
::
:GUI_EVENT
set "GE_NAME=%~1"
set "GE_DATA=%~2"

:: JSON escape: convert \ to \\ and " to \"
if defined GE_DATA (
    set "GE_DATA=!GE_DATA:\=\\!"
    set "GE_DATA=!GE_DATA:"=\"!"
)

if "!JSON_MODE!"=="1" (
    echo {"event":"!GE_NAME!","data":"!GE_DATA!"}
)

if "!PIPE_FD_MODE!"=="1" (
    rem Persistent long connection via file descriptor 3
    echo {"event":"!GE_NAME!","data":"!GE_DATA!"} >&3 2>nul
) else if "!PIPE_MODE!"=="1" (
    rem Short connection (per-event open and close)
    echo {"event":"!GE_NAME!","data":"!GE_DATA!"} > "!VIDEODL_PIPE!" 2>nul
)

exit /b 0
