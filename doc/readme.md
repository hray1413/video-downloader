# dl.bat — GUI 接口整合指南

> YT-DLP Video Downloader v3.3  
> 作者: hray1413 | 郵箱: videodownload@ss2256.cc.cd

---

## 目錄

1. [概述](#概述)
2. [環境變數一覽](#環境變數一覽)
3. [退出代碼](#退出代碼)
4. [輸出事件格式](#輸出事件格式)
5. [GUI 整合模式詳解](#gui-整合模式詳解)
   - [A. Electron / 桌面應用](#a-electron--桌面應用)
   - [B. CLI / 腳本模式](#b-cli--腳本模式)
   - [C. WebSocket / Named Pipe 模式](#c-websocket--named-pipe-模式)
   - [D. JSON stdout 模式（萬用）](#d-json-stdout-模式萬用)
   - [E. 進度檔案監視模式](#e-進度檔案監視模式)
   - [F. 自訂協議 videodl:// 處理器](#f-自訂協議-videodl-處理器)
   - [G. Python / Tkinter / PyQt GUI](#g-python--tkinter--pyqt-gui)
   - [H. PowerShell / WPF GUI](#h-powershell--wpf-gui)
   - [I. 瀏覽器擴充套件（Native Messaging）](#i-瀏覽器擴充套件native-messaging)
   - [J. REST API / HTTP Server 包裝器](#j-rest-api--http-server-包裝器)
6. [格式選項](#格式選項)
7. [檔案結構](#檔案結構)
8. [互動模式指令](#互動模式指令)

---

## 概述

`dl.bat` 是一個以 `yt-dlp` 為核心的影片下載腳本，除了提供給一般使用者的互動式 CLI，
也預留了完整的 GUI 整合接口，讓任何前端框架都能以一致的方式控制下載行為並接收狀態回饋。

```
dl.bat [URL] [FORMAT]
dl.bat [FORMAT] [URL_FILE]        ← Electron 模式
dl.bat --json-output [URL] [FORMAT]
```

---

## 環境變數一覽

| 環境變數 | 型態 | 說明 |
|---|---|---|
| `VIDEODL_URL_FILE` | 路徑字串 | 包含目標 URL 的文字檔路徑（第一行為 URL）。設定後自動進入 Electron 模式 |
| `VIDEODL_JSON` | `1` | 啟用 JSON 事件輸出模式，所有狀態訊息輸出為 JSON 行 |
| `VIDEODL_PIPE` | 命名管道路徑 | 如 `\\.\pipe\videodl`，事件寫入此管道 |
| `VIDEODL_PROGRESS_FILE` | 路徑字串 | GUI 應將 stdout 重導向至此檔案，然後監視該檔案獲取進度 |
| `VIDEODL_OUTPUT_DIR` | 路徑字串 | 覆蓋預設輸出目錄（相對或絕對路徑均可）|
| `VIDEODL_EXTRA_OPTS` | yt-dlp 參數字串 | 附加至 yt-dlp 命令列的額外參數（進階覆蓋）|
| `VIDEODL_NO_PAUSE` | `1` | 抑制所有 `pause` 提示，適合任何 GUI 整合 |
| `VIDEODL_SILENT` | `1` | *(預留)* 未來版本用於完全靜音，目前建議搭配 JSON 模式 |

> [!IMPORTANT]
> `VIDEODL_NO_PAUSE=1` 是所有 GUI 整合的**最低必要設定**，否則腳本會在完成後等待按鍵而卡住子行程。

---

## 退出代碼

| 代碼 | 說明 |
|---|---|
| `0` | 下載成功 |
| `1` | 發生錯誤（工具缺失、網路失敗、URL 無效等）|

---

## 輸出事件格式

### 一般文字輸出（預設）

所有狀態行以以下前綴標識，GUI 可正則解析：

```
[Info]     資訊訊息
[Success]  操作成功
[Failed]   下載失敗
[Error]    嚴重錯誤
[Warning]  警告
[Progress] yt-dlp 原始進度行（由 yt-dlp 自身輸出）
```

### JSON 輸出模式（`VIDEODL_JSON=1`）

每個事件輸出一行 JSON：

```json
{"event":"init","data":"Script started, checking tools..."}
{"event":"tool_check","data":"Tools OK"}
{"event":"url_detected","data":"https://www.youtube.com/watch?v=xxx"}
{"event":"format_selected","data":"Format=1 OutputDir=videos"}
{"event":"success","data":"Saved to C:\\...\\videos"}
{"event":"failed","data":"Exit code 1"}
{"event":"update","data":"Updating yt-dlp..."}
{"event":"about","data":"v3.3 by hray1413"}
{"event":"error","data":"<錯誤描述>"}
```

#### 完整事件列表

| 事件名稱 | 觸發時機 |
|---|---|
| `init` | 腳本啟動 |
| `tool_check` | yt-dlp / ffmpeg 確認完畢 |
| `url_detected` | URL 驗證通過 |
| `format_selected` | 格式與輸出目錄確定 |
| `progress` | *(由 yt-dlp 原始輸出，需 GUI 自行解析)* |
| `success` | 下載完成 |
| `failed` | 下載失敗 |
| `update` | 執行更新 |
| `about` | 顯示版本資訊 |
| `error` | 任何錯誤發生時 |

---

## GUI 整合模式詳解

---

### A. Electron / 桌面應用

最穩定的整合方式，透過 URL 檔案傳遞網址。

#### 方法一：環境變數（推薦）

```javascript
// main.js (Electron Main Process)
const { spawn } = require('child_process');
const fs = require('fs');
const path = require('path');
const os = require('os');

async function download(url, format = '1') {
  const urlFile = path.join(os.tmpdir(), `videodl_${Date.now()}.txt`);
  fs.writeFileSync(urlFile, url, 'utf8');

  return new Promise((resolve, reject) => {
    const proc = spawn('cmd.exe', ['/c', 'dl.bat', format], {
      cwd: 'C:\\path\\to\\Video_downloader',
      env: {
        ...process.env,
        VIDEODL_URL_FILE: urlFile,
        VIDEODL_NO_PAUSE: '1',
        VIDEODL_JSON: '1',       // 啟用 JSON 事件
      },
    });

    proc.stdout.on('data', (data) => {
      const lines = data.toString().split('\n');
      lines.forEach(line => {
        line = line.trim();
        if (line.startsWith('{')) {
          try {
            const event = JSON.parse(line);
            // 發送到 renderer process
            mainWindow.webContents.send('dl-event', event);
          } catch (_) {}
        }
      });
    });

    proc.on('close', (code) => {
      fs.unlinkSync(urlFile);
      code === 0 ? resolve() : reject(new Error(`Exit code: ${code}`));
    });
  });
}
```

#### 方法二：命令列參數

```javascript
const proc = spawn('cmd.exe', ['/c', 'dl.bat', format, urlFilePath], {
  env: { ...process.env, VIDEODL_NO_PAUSE: '1' }
});
```

#### IPC 接收範例（Renderer）

```javascript
// renderer.js
const { ipcRenderer } = require('electron');

ipcRenderer.on('dl-event', (_, event) => {
  if (event.event === 'success') showSuccess(event.data);
  if (event.event === 'failed')  showError(event.data);
  if (event.event === 'progress') updateProgressBar(event.data);
});
```

---

### B. CLI / 腳本模式

直接命令列呼叫，無需任何環境變數。

```batch
:: 下載最佳畫質
dl.bat "https://www.youtube.com/watch?v=xxx"

:: 下載 1080p
dl.bat "https://www.youtube.com/watch?v=xxx" 2

:: 下載 MP3
dl.bat "https://www.youtube.com/watch?v=xxx" 4

:: 無暫停 + JSON 輸出（適合 CI 腳本）
set VIDEODL_NO_PAUSE=1
set VIDEODL_JSON=1
dl.bat "https://www.youtube.com/watch?v=xxx" 1
```

---

### C. WebSocket / Named Pipe 模式

適合需要實時推送事件給 WebSocket 客戶端的本機服務。

```batch
:: 啟動時設定命名管道路徑
set VIDEODL_PIPE=\\.\pipe\videodl
set VIDEODL_NO_PAUSE=1
dl.bat "https://..." 1
```

#### PowerShell Named Pipe Server 範例

```powershell
# 建立命名管道伺服器
$pipe = New-Object System.IO.Pipes.NamedPipeServerStream(
    'videodl', 
    [System.IO.Pipes.PipeDirection]::In
)
$pipe.WaitForConnection()
$reader = New-Object System.IO.StreamReader($pipe)
while ($null -ne ($line = $reader.ReadLine())) {
    Write-Host "Event: $line"
    # 轉發至 WebSocket...
}
```

> [!NOTE]
> 命名管道適合本機 IPC。若需跨網路，請使用 REST API 模式（J）搭配 WebSocket。

---

### D. JSON stdout 模式（萬用）

任何能讀取子行程 stdout 的框架都可使用此模式。

```batch
:: 環境變數啟用
set VIDEODL_JSON=1
set VIDEODL_NO_PAUSE=1
dl.bat "https://..." 1

:: 或命令列旗標啟用
dl.bat --json-output "https://..." 1
```

```python
# Python 範例
import subprocess, json

proc = subprocess.Popen(
    ['dl.bat', 'https://...', '1'],
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    text=True,
    env={**os.environ, 'VIDEODL_JSON': '1', 'VIDEODL_NO_PAUSE': '1'},
    cwd=r'C:\path\to\Video_downloader'
)

for line in proc.stdout:
    line = line.strip()
    if line.startswith('{'):
        event = json.loads(line)
        print(f"[{event['event']}] {event['data']}")
```

---

### E. 進度檔案監視模式

GUI 將 stdout 重導向至檔案，然後監視（tail）該檔案獲取即時進度。

> [!TIP]
> 此模式適合使用檔案系統監視器（如 `FileSystemWatcher`）的 .NET/C# 應用。

```powershell
# PowerShell 啟動並監視進度
$progressFile = "$env:TEMP\dl_progress.log"
$proc = Start-Process -FilePath "cmd.exe" `
    -ArgumentList "/c", "dl.bat", "1", "url.txt" `
    -RedirectStandardOutput $progressFile `
    -WorkingDirectory "C:\path\to\Video_downloader" `
    -NoNewWindow -PassThru `
    -Environment @{ VIDEODL_NO_PAUSE='1'; VIDEODL_JSON='1' }

# 監視進度檔案
Get-Content $progressFile -Wait | ForEach-Object {
    if ($_ -match '^\{') {
        $event = $_ | ConvertFrom-Json
        Write-Host "Event: $($event.event) -> $($event.data)"
    }
}
```

```csharp
// C# FileSystemWatcher 範例
var watcher = new FileSystemWatcher(Path.GetTempPath(), "dl_progress.log");
watcher.Changed += (s, e) => {
    var lines = File.ReadAllLines(e.FullPath);
    foreach (var line in lines.Where(l => l.StartsWith("{"))) {
        var evt = JsonSerializer.Deserialize<DlEvent>(line);
        Dispatcher.Invoke(() => UpdateUI(evt));
    }
};
watcher.EnableRaisingEvents = true;
```

---

### F. 自訂協議 videodl:// 處理器

已透過 `install-protocol.bat` 在系統註冊 `videodl://` 協議，可從瀏覽器或任何應用直接喚起。

```
videodl://https://www.youtube.com/watch?v=xxx
videodl://https://www.youtube.com/watch?v=xxx 2
```

腳本自動剝離協議前綴並處理以下變體：

| 輸入格式 | 自動修正為 |
|---|---|
| `videodl:///https://...` | `https://...` |
| `videodl://https://...` | `https://...` |
| `videodl:https://...` | `https://...` |

#### 從 JavaScript 喚起

```javascript
// 在瀏覽器中觸發協議喚起
window.location.href = 'videodl://' + videoUrl;

// 或帶格式
window.location.href = `videodl://${videoUrl} 4`;  // 4=MP3
```

---

### G. Python / Tkinter / PyQt GUI

```python
import subprocess
import threading
import json
import os

class Downloader:
    def __init__(self, dl_dir: str):
        self.dl_dir = dl_dir  # Video_downloader 目錄路徑

    def download(self, url: str, format_choice: str = '1',
                 on_event=None, on_done=None):
        """
        on_event: Callable[[dict], None]  接收 JSON 事件
        on_done:  Callable[[int], None]   接收退出代碼
        """
        env = {
            **os.environ,
            'VIDEODL_NO_PAUSE': '1',
            'VIDEODL_JSON': '1',
        }

        def run():
            proc = subprocess.Popen(
                ['cmd.exe', '/c', 'dl.bat', url, format_choice],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                cwd=self.dl_dir,
                env=env,
            )
            for line in proc.stdout:
                line = line.strip()
                if line.startswith('{') and on_event:
                    try:
                        on_event(json.loads(line))
                    except json.JSONDecodeError:
                        pass
            proc.wait()
            if on_done:
                on_done(proc.returncode)

        threading.Thread(target=run, daemon=True).start()

# --- 使用範例 (Tkinter) ---
import tkinter as tk

def on_event(event):
    print(f"[{event['event']}] {event['data']}")
    if event['event'] == 'success':
        label.config(text='✅ 下載完成！')
    elif event['event'] == 'failed':
        label.config(text='❌ 下載失敗')

dl = Downloader(r'C:\path\to\Video_downloader')
dl.download('https://www.youtube.com/watch?v=xxx', '1', on_event=on_event)
```

---

### H. PowerShell / WPF GUI

```powershell
function Start-VideoDownload {
    param(
        [string]$Url,
        [string]$Format = '1',
        [string]$WorkDir = 'C:\path\to\Video_downloader',
        [scriptblock]$OnEvent
    )

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = 'cmd.exe'
    $psi.Arguments = "/c dl.bat `"$Url`" $Format"
    $psi.WorkingDirectory = $WorkDir
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.CreateNoWindow = $true
    $psi.EnvironmentVariables['VIDEODL_NO_PAUSE'] = '1'
    $psi.EnvironmentVariables['VIDEODL_JSON'] = '1'

    $proc = New-Object System.Diagnostics.Process
    $proc.StartInfo = $psi

    # 非同步讀取輸出
    $proc.OutputDataReceived += {
        param($s, $e)
        if ($e.Data -and $e.Data.StartsWith('{')) {
            $event = $e.Data | ConvertFrom-Json
            if ($OnEvent) { & $OnEvent $event }
        }
    }

    $proc.Start() | Out-Null
    $proc.BeginOutputReadLine()
    $proc.WaitForExit()
    return $proc.ExitCode
}

# --- 使用範例 ---
Start-VideoDownload -Url 'https://...' -Format '1' -OnEvent {
    param($event)
    Write-Host "[$($event.event)] $($event.data)"
}
```

---

### I. 瀏覽器擴充套件（Native Messaging）

擴充套件透過 Native Messaging Host 呼叫 `dl.bat`。

#### Native Messaging Host (`host.js`)

```javascript
const { execFile } = require('child_process');

process.stdin.on('data', (data) => {
    // Chrome 原生訊息格式：前4字節為長度
    const msg = JSON.parse(data.slice(4).toString('utf8'));
    const { url, format } = msg;

    const env = {
        ...process.env,
        VIDEODL_NO_PAUSE: '1',
        VIDEODL_JSON: '1',
    };

    execFile('cmd.exe', ['/c', 'dl.bat', url, format || '1'], {
        cwd: 'C:\\path\\to\\Video_downloader',
        env,
    }, (err, stdout, stderr) => {
        const result = { success: !err, stdout, exitCode: err ? err.code : 0 };
        sendNativeMessage(result);
    });
});

function sendNativeMessage(msg) {
    const json = JSON.stringify(msg);
    const buf = Buffer.alloc(4 + json.length);
    buf.writeUInt32LE(json.length, 0);
    buf.write(json, 4);
    process.stdout.write(buf);
}
```

#### 擴充套件端呼叫

```javascript
// content_script.js 或 background.js
chrome.runtime.sendNativeMessage('com.videodl.host', {
    url: 'https://www.youtube.com/watch?v=xxx',
    format: '1'
}, (response) => {
    if (response.success) {
        console.log('下載成功');
    }
});
```

---

### J. REST API / HTTP Server 包裝器

使用 Flask 或 FastAPI 將 `dl.bat` 包裝成 HTTP API，支援 SSE 或 WebSocket 推送進度。

#### Flask + SSE 範例

```python
from flask import Flask, Response, request, stream_with_context
import subprocess, json, os

app = Flask(__name__)
DL_DIR = r'C:\path\to\Video_downloader'

@app.route('/download')
def download():
    url = request.args.get('url')
    fmt = request.args.get('format', '1')

    def generate():
        proc = subprocess.Popen(
            ['cmd.exe', '/c', 'dl.bat', url, fmt],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            cwd=DL_DIR,
            env={**os.environ, 'VIDEODL_NO_PAUSE': '1', 'VIDEODL_JSON': '1'},
        )
        for line in proc.stdout:
            line = line.strip()
            if line.startswith('{'):
                yield f"data: {line}\n\n"
        proc.wait()
        yield f"data: {{\"event\":\"done\",\"code\":{proc.returncode}}}\n\n"

    return Response(
        stream_with_context(generate()),
        mimetype='text/event-stream',
        headers={'Cache-Control': 'no-cache', 'X-Accel-Buffering': 'no'}
    )

# 客戶端 (JavaScript)
# const es = new EventSource('/download?url=https://...&format=1');
# es.onmessage = e => { const evt = JSON.parse(e.data); console.log(evt); };
```

#### FastAPI + WebSocket 範例

```python
from fastapi import FastAPI, WebSocket
import asyncio, subprocess, json, os

app = FastAPI()
DL_DIR = r'C:\path\to\Video_downloader'

@app.websocket('/ws/download')
async def download_ws(ws: WebSocket):
    await ws.accept()
    data = await ws.receive_json()
    url, fmt = data['url'], data.get('format', '1')

    proc = await asyncio.create_subprocess_exec(
        'cmd.exe', '/c', 'dl.bat', url, fmt,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.STDOUT,
        cwd=DL_DIR,
        env={**os.environ, 'VIDEODL_NO_PAUSE': '1', 'VIDEODL_JSON': '1'},
    )

    async for line in proc.stdout:
        line = line.decode().strip()
        if line.startswith('{'):
            await ws.send_text(line)

    await proc.wait()
    await ws.send_json({'event': 'done', 'code': proc.returncode})
    await ws.close()
```

---

## 格式選項

| 代碼 | 說明 | 輸出位置 |
|---|---|---|
| `1` | 最佳畫質 MP4（自動 4K/2K/1080p），內嵌字幕與封面 **[預設]** | `videos\` |
| `2` | 1080p Max MP4，內嵌字幕與封面 | `videos\` |
| `3` | 720p Max MP4，內嵌字幕與封面 | `videos\` |
| `4` | MP3 320k，內嵌封面 | `videos\audio\mp3\` |
| `5` | Best M4A，內嵌封面 | `videos\audio\m4a\` |
| `6` | 整個播放清單 MP4，按清單資料夾分類 | `videos\playlists\` |

---

## 檔案結構

```
Video_downloader\
├── dl.bat                  ← 主腳本（本文件對應）
├── yt-dlp.exe              ← 自動下載（缺失時）
├── ffmpeg.exe              ← 自動下載（缺失時）
├── ffprobe.exe             ← 自動下載（缺失時）
├── cookies.txt             ← 可選，登入 Cookie
├── deno.exe                ← 可選，Deno JS Runtime
├── install-protocol.bat    ← 註冊 videodl:// 協議
├── electron-app\           ← Electron GUI 前端
├── browser-extension\      ← 瀏覽器擴充套件
└── videos\                 ← 下載輸出目錄
    ├── audio\
    │   ├── mp3\
    │   └── m4a\
    └── playlists\
```

---

## 互動模式指令

在互動模式（無參數啟動）的 URL 輸入框中，可輸入以下指令：

| 輸入 | 動作 |
|---|---|
| `u` / `update` / `--update` | 更新 yt-dlp |
| `about` / `--about` | 顯示版本資訊 |
| `help` / `--help` / `-h` | 顯示使用說明 |
| 空白 Enter | 結束程式 |

---

> [!TIP]
> 建議所有 GUI 整合都同時設定 `VIDEODL_NO_PAUSE=1` 與 `VIDEODL_JSON=1`，
> 這樣既不會因 `pause` 卡住子行程，又能從 JSON 事件流中獲取結構化的狀態資訊。
