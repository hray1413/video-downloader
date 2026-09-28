# dl.bat — GUI Interface Integration Guide

> YT-DLP Video Downloader v3.5  
> Author: hray1413 | Email: videodownload@ss2256.cc.cd

---

## Table of Contents

1. [Overview](#overview)
2. [Environment Variables Reference](#environment-variables-reference)
3. [Exit Codes](#exit-codes)
4. [Output Event Format](#output-event-format)
5. [GUI Integration Modes Explained](#gui-integration-modes-explained)
   - [A. Electron / Desktop App](#a-electron--desktop-app)
   - [B. CLI / Script Mode](#b-cli--script-mode)
   - [C. WebSocket / Named Pipe Mode](#c-websocket--named-pipe-mode)
   - [D. JSON stdout Mode (Universal)](#d-json-stdout-mode-universal)
   - [E. Progress File Monitoring Mode](#e-progress-file-monitoring-mode)
   - [F. Custom Protocol videodl:// Handler](#f-custom-protocol-videodl-handler)
   - [G. Python / Tkinter / PyQt GUI](#g-python--tkinter--pyqt-gui)
   - [H. PowerShell / WPF GUI](#h-powershell--wpf-gui)
   - [I. Browser Extension (Native Messaging)](#i-browser-extension-native-messaging)
   - [J. REST API / HTTP Server Wrapper](#j-rest-api--http-server-wrapper)
6. [Format Options](#format-options)
7. [File Structure](#file-structure)
8. [Interactive Mode Commands](#interactive-mode-commands)

---

## Overview

`dl.bat` is a video downloading script powered by `yt-dlp`. In addition to providing an interactive CLI for end users, it reserves a complete set of GUI integration interfaces, allowing any frontend framework to consistently control downloading behavior and receive status feedback.

```
dl.bat [URL] [FORMAT]
dl.bat [FORMAT] [URL_FILE]         ← Electron mode
dl.bat --json-output [URL] [FORMAT]
```

---

## Environment Variables Reference

| Environment Variable | Type | Description |
| --- | --- | --- |
| `VIDEODL_URL_FILE` | Path string | Path to a text file containing the target URL (first line is URL). Setting this automatically triggers Electron mode. |
| `VIDEODL_JSON` | `1` | Enables JSON event output mode; all status messages are output as JSON lines. |
| `VIDEODL_PIPE` | Named pipe path | E.g., `\\.\pipe\videodl`, events are written to this pipe (each event is a single short-lived connection). |
| `VIDEODL_PIPE_FD` | `3` | When set to `3`, events are output via redirected file descriptor 3 (persistent long-lived connection pipe). |
| `VIDEODL_OUTPUT_DIR` | Path string | Overrides the default output directory (relative or absolute paths supported). |
| `VIDEODL_EXTRA_OPTS` | yt-dlp arg string | Additional arguments appended to the yt-dlp command line (advanced override). |
| `VIDEODL_NO_PAUSE` | `1` | Suppresses all `pause` prompts, suitable for any GUI integration. |
| `VIDEODL_SILENT` | `1` | *(Reserved)* For fully silent execution in future versions; currently recommended to use with JSON mode. |

> [!IMPORTANT]
> `VIDEODL_NO_PAUSE=1` is the **minimum required setting** for all GUI integrations; otherwise, the script will wait for a keypress upon completion and hang the child process.

---

## Exit Codes

| Code | Description |
| --- | --- |
| `0` | Download successful |
| `1` | Error occurred (missing tools, network failure, invalid URL, etc.) |

---

## Output Event Format

### General Text Output (Default)

All status lines are identified by the following prefixes, which GUIs can parse via regex:

```
[Info]     Informational message
[Success]  Operation successful
[Failed]   Download failed
[Error]    Severe error
[Warning]  Warning
[Progress] yt-dlp raw progress line (output directly by yt-dlp)
```

### JSON Output Mode (`VIDEODL_JSON=1`)

Each event outputs a single line of JSON:

```json
{"event":"init","data":"Script started, checking tools..."}
{"event":"tool_check","data":"Tools OK"}
{"event":"url_detected","data":"https://www.youtube.com/watch?v=xxx"}
{"event":"format_selected","data":"Format=1 OutputDir=videos"}
{"event":"success","data":"Saved to C:\\...\\videos"}
{"event":"failed","data":"Exit code 1"}
{"event":"update","data":"Updating yt-dlp..."}
{"event":"about","data":"v3.5 by hray1413"}
{"event":"error","data":"<Error description>"}
```

#### Full Event List

| Event Name | Trigger Timing |
| --- | --- |
| `init` | Script startup |
| `tool_check` | yt-dlp / ffmpeg verification complete |
| `url_detected` | URL validation passed |
| `format_selected` | Format and output directory determined |
| `progress` | *(Raw yt-dlp output; GUI must parse independently)* |
| `success` | Download complete |
| `failed` | Download failed |
| `update` | Executing update |
| `about` | Displaying version info |
| `error` | Any error occurs |

> [!TIP]
> **Strict JSON Specification & Automatic Character Escaping Guarantee:**  
> Automatic escape filtering is implemented inside `dl.bat`. When an event string contains Windows path backslashes `\` (e.g., `Saved to C:\videos`) or double quotes `"`, the script automatically converts them to `\\` and `\"`, ensuring every output line is 100% compliant with RFC 8259 JSON specifications. Strict parsers across languages (such as Python `json.loads`, Node.js `JSON.parse`, C# `JsonSerializer.Deserialize`) can safely parse them directly without throwing `Invalid \escape` exceptions.

> [!NOTE]
> **Known Limitation (`&` Character Interference):**  
> Since `&` is a native command separator in Windows batch scripting, text data containing unquoted `&` characters may cause syntax interference. For URLs containing complex parameters (such as multiple `&key=val`), **it is strongly recommended to prioritize Mode A (`VIDEODL_URL_FILE` file transfer)** to completely avoid Windows command-line parsing traps for special characters like `&`, `^`, and `%`.

---

## GUI Integration Modes Explained

---

### A. Electron / Desktop App

The most stable integration method, passing URLs via a file.

#### Method 1: Environment Variables (Recommended)

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
        VIDEODL_JSON: '1',       // Enable JSON events
      },
    });

    proc.stdout.on('data', (data) => {
      const lines = data.toString().split('\n');
      lines.forEach(line => {
        line = line.trim();
        if (line.startsWith('{')) {
          try {
            const event = JSON.parse(line);
            // Send to renderer process
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

#### Method 2: Command Line Arguments

```javascript
const proc = spawn('cmd.exe', ['/c', 'dl.bat', format, urlFilePath], {
  env: { ...process.env, VIDEODL_NO_PAUSE: '1' }
});
```

#### IPC Receiver Example (Renderer)

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

### B. CLI / Script Mode

Direct command-line invocation without any environment variables.

```batch
:: Download best quality
dl.bat "https://www.youtube.com/watch?v=xxx"

:: Download 1080p
dl.bat "https://www.youtube.com/watch?v=xxx" 2

:: Download MP3
dl.bat "https://www.youtube.com/watch?v=xxx" 4

:: No pause + JSON output (suitable for CI scripts)
set VIDEODL_NO_PAUSE=1
set VIDEODL_JSON=1
dl.bat "https://www.youtube.com/watch?v=xxx" 1
```

---

### C. WebSocket / Named Pipe Mode

Suitable for local IPC communication requiring real-time event pushing to desktop apps, resident services, or WebSocket servers.

> [!WARNING]
> **Important Behavior of Windows Batch Redirection into Named Pipes:**  
> When the batch script executes `echo {...} > "!VIDEODL_PIPE!"`, Windows **re-opens the pipe, writes a single line, and immediately closes the handle** on every `>` operation.  
> For Named Pipe servers (such as C#'s `NamedPipeServerStream`), this counts as a **"Per-Event Short Connection"**.  
> If the server calls `WaitForConnection()` only once and expects to continuously `ReadLine()` in a long connection, it will receive an EOF / pipe broken error after reading the first line, causing all subsequent events to be missed!

Depending on your architecture requirements, choose one of the following two implementation solutions:

---

#### Option 1: Short-Connection Loop Mode (Default `VIDEODL_PIPE`)

The batch script opens a pipe connection and writes a single line every time an event is triggered. The server must continuously **"Wait for connection ➔ Read ➔ Disconnect"** inside a loop.

##### Launch Batch Script:

```batch
set VIDEODL_PIPE=\\.\pipe\videodl
set VIDEODL_NO_PAUSE=1
dl.bat "https://..." 1
```

##### Server Implementation (PowerShell Example):

```powershell
# Create Named Pipe Server (configured for single-instance per connection)
$pipe = New-Object System.IO.Pipes.NamedPipeServerStream(
    'videodl',
    [System.IO.Pipes.PipeDirection]::In,
    1,                                              # Max instances
    [System.IO.Pipes.PipeTransmissionMode]::Byte,
    [System.IO.Pipes.PipeOptions]::Asynchronous
)

Write-Host "Named Pipe server started, waiting for events..."

try {
    while ($true) {
        # 1. Wait for batch script to establish short connection
        $pipe.WaitForConnection()
        
        # 2. Read one line of event JSON for this connection
        $reader = New-Object System.IO.StreamReader($pipe, [System.Text.Encoding]::UTF8)
        $line = $reader.ReadLine()
        
        if ($null -ne $line -and $line.Trim() -ne "") {
            Write-Host "Received event: $line"
            # Parse and forward to WebSocket or update UI...
            # $event = $line | ConvertFrom-Json
        }
        
        # 3. Important: Must disconnect this connection to prepare for the next event
        $pipe.Disconnect()
    }
} finally {
    $pipe.Dispose()
}
```

##### Server Implementation (C# / .NET Example):

```csharp
using System;
using System.IO;
using System.IO.Pipes;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

public class NamedPipeEventListener
{
    public static async Task StartListeningAsync(CancellationToken cancellationToken)
    {
        while (!cancellationToken.IsCancellationRequested)
        {
            using var server = new NamedPipeServerStream(
                "videodl",
                PipeDirection.In,
                1,
                PipeTransmissionMode.Byte,
                PipeOptions.Asynchronous);

            await server.WaitForConnectionAsync(cancellationToken);

            using var reader = new StreamReader(server);
            string? line = await reader.ReadLineAsync();

            if (!string.IsNullOrEmpty(line))
            {
                var evt = JsonSerializer.Deserialize<JsonElement>(line);
                Console.WriteLine($"[Pipe Event] {evt}");
            }

            // Close current connection; next loop iteration creates a new instance to wait for the next write
            server.Disconnect();
        }
    }
}
```

---

#### Option 2: Persistent Long-Connection Mode (`VIDEODL_PIPE_FD=3` File Descriptor Redirection)

If you want the Named Pipe server to maintain a **single uninterrupted connection**, allowing the server to continuously read all events using a single StreamReader:

> [!IMPORTANT]
> **Windows File Descriptor 3 Inheritance Mechanism and Prerequisites:**  
> Win32's `STARTUPINFO` natively only includes standard input (0), standard output (1), and standard error (2). Windows **does not** automatically create or inherit file descriptor 3 for child processes.  
> If file descriptor 3 is not created for the child process, executing `>&3` in the batch script will throw `ERROR_INVALID_HANDLE` (invalid handle), and the error message will be silently swallowed by `2>nul`, causing **all events to be silently lost**!  
>  
> **Simplest and Most Robust Solution:** Use `cmd.exe`'s native parenthetical redirection syntax to let `cmd.exe` automatically open the pipe and bind it to slot 3 when parsing the command line:  
> `cmd.exe /c "set VIDEODL_PIPE_FD=3 && (dl.bat "URL" FORMAT) 3>\\.\pipe\videodl"`

##### Correct Syntax for Initializing File Descriptor 3 Across Languages:

###### 1. Node.js (`child_process`)

```javascript
const { spawn } = require('child_process');

// Let cmd.exe parenthetical redirection bind \\.\pipe\videodl to file descriptor 3
const cmdString = `(dl.bat "${videoUrl}" ${format}) 3>\\\\.\\pipe\\videodl`;

const child = spawn('cmd.exe', ['/c', cmdString], {
    cwd: 'C:\\path\\to\\Video_downloader',
    env: { ...process.env, VIDEODL_NO_PAUSE: '1', VIDEODL_PIPE_FD: '3' }
});
```

###### 2. C# (.NET `System.Diagnostics.Process`)

```csharp
var psi = new ProcessStartInfo
{
    FileName = "cmd.exe",
    // Open handle 3 via cmd.exe parenthetical redirection
    Arguments = $"/c \"(dl.bat \"{url}\" {format}) 3>\\\\.\\pipe\\videodl\"",
    WorkingDirectory = @"C:\path\to\Video_downloader",
    UseShellExecute = false,
    CreateNoWindow = true
};
psi.EnvironmentVariables["VIDEODL_NO_PAUSE"] = "1";
psi.EnvironmentVariables["VIDEODL_PIPE_FD"] = "3";

using var process = Process.Start(psi);
```

###### 3. Python (`subprocess`)

```python
import subprocess, os

cmd = f'(dl.bat "{url}" {format}) 3>\\\\.\\pipe\\videodl'
env = {**os.environ, 'VIDEODL_NO_PAUSE': '1', 'VIDEODL_PIPE_FD': '3'}

proc = subprocess.Popen(
    ['cmd.exe', '/c', cmd],
    cwd=r'C:\path\to\Video_downloader',
    env=env
)
```

###### 4. PowerShell

```powershell
$cmd = "(dl.bat `"$url`" $format) 3>\\.\pipe\videodl"
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = "cmd.exe"
$psi.Arguments = "/c $cmd"
$psi.WorkingDirectory = "C:\path\to\Video_downloader"
$psi.UseShellExecute = $false
$psi.EnvironmentVariables["VIDEODL_NO_PAUSE"] = "1"
$psi.EnvironmentVariables["VIDEODL_PIPE_FD"] = "3"

[System.Diagnostics.Process]::Start($psi)
```

##### Server Persistent Long-Connection Implementation (C# / .NET Example):

```csharp
using var server = new NamedPipeServerStream("videodl", PipeDirection.In);
await server.WaitForConnectionAsync(); // Connect only once!

using var reader = new StreamReader(server, System.Text.Encoding.UTF8);
string? line;
// Continuously read from a single connection until the batch script ends and automatically closes the pipe
while ((line = await reader.ReadLineAsync()) != null)
{
    if (!string.IsNullOrWhiteSpace(line))
    {
        Console.WriteLine($"[Persistent Pipe Event] {line}");
    }
}
```

> [!TIP]
> **Architectural Recommendation**:  
> If the GUI and `dl.bat` are launched as a subprocess by the same native application, **"Mode D: JSON stdout Mode" is strongly recommended**. A subprocess's standard output is naturally a persistent, high-performance, concurrency-safe long data stream, completely eliminating the need to handle complex named pipe handle lifecycles.

---

### D. JSON stdout Mode (Universal)

Any framework capable of reading a child process's stdout can use this mode.

```batch
:: Enable via environment variables
set VIDEODL_JSON=1
set VIDEODL_NO_PAUSE=1
dl.bat "https://..." 1

:: Or enable via command-line flag
dl.bat --json-output "https://..." 1
```

```python
# Python Example
import subprocess, json, os

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

### E. Progress File Monitoring Mode

If the GUI framework (such as WPF or Windows Forms) has difficulty reading process stdout asynchronously, the caller can redirect stdout to a temporary log file upon launching the process, then read progress via a file monitor.

> [!NOTE]
> `dl.bat` has built-in `--newline` parameters ensuring yt-dlp progress is output via line-buffered flushing in real time. The script itself does not need a dedicated log path environment variable; instead, it is handled uniformly by the **caller (GUI) redirecting standard output directly** when spawning the process.

> [!TIP]
> This mode is suitable for desktop applications utilizing file system monitors (such as `FileSystemWatcher`) or periodic log polling.

```powershell
# PowerShell launch and monitor progress
$progressFile = "$env:TEMP\dl_progress.log"
$proc = Start-Process -FilePath "cmd.exe" `
    -ArgumentList "/c", "dl.bat", "1", "url.txt" `
    -RedirectStandardOutput $progressFile `
    -WorkingDirectory "C:\path\to\Video_downloader" `
    -NoNewWindow -PassThru `
    -Environment @{ VIDEODL_NO_PAUSE='1'; VIDEODL_JSON='1' }

# Monitor progress file
Get-Content $progressFile -Wait | ForEach-Object {
    if ($_ -match '^\{') {
        $event = $_ | ConvertFrom-Json
        Write-Host "Event: $($event.event) -> $($event.data)"
    }
}
```

```csharp
// C# FileSystemWatcher Example
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

### F. Custom Protocol videodl:// Handler

The `videodl://` protocol is registered in the system via `install-protocol.bat`, allowing direct invocation from browsers or any application.

```
videodl://https://www.youtube.com/watch?v=xxx
videodl://https://www.youtube.com/watch?v=xxx 2
```

> [!WARNING]
> **Browser Normalization (Dropping Colons) Common Trap:**  
> Mainstream browsers (like Chrome, Edge) often normalize URLs when invoking custom protocols via JavaScript, stripping or escaping the second colon immediately following the custom protocol. For example:  
> `videodl://https://www.youtube.com/...` might be converted by the browser core into `videodl://https//www.youtube.com/...` or even `videodl://https/...` before being passed to the system command line.  
>  
> **`dl.bat` has a built-in auto-tolerance mechanism**. During the `PARSE_URL` stage, the script automatically detects `https//`, `http//`, `https/`, and `http/` and re-appends the colon `://`. Callers can directly concatenate and invoke the full URL without manual encoding.

The script automatically strips protocol prefixes and handles the following variants:

| Input Format (Including Browser-Normalized Forms) | Automatically Corrected To |
| --- | --- |
| `videodl://https://...` | `https://...` |
| `videodl:///https://...` | `https://...` |
| `videodl:https://...` | `https://...` |
| `https//...` (Colon stripped) | `https://...` |
| `http//...` (Colon stripped) | `http://...` |

#### Invoking from JavaScript

```javascript
// Trigger protocol invocation in the browser
window.location.href = 'videodl://' + videoUrl;

// Or with format
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
        self.dl_dir = dl_dir  # Path to Video_downloader directory

    def download(self, url: str, format_choice: str = '1',
                 on_event=None, on_done=None):
        """
        on_event: Callable[[dict], None]  Receives JSON events
        on_done:  Callable[[int], None]   Receives exit code
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

# --- Usage Example (Tkinter) ---
import tkinter as tk

def on_event(event):
    print(f"[{event['event']}] {event['data']}")
    if event['event'] == 'success':
        label.config(text='✅ Download Complete!')
    elif event['event'] == 'failed':
        label.config(text='❌ Download Failed')

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

    # Asynchronous output reading
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

# --- Usage Example ---
Start-VideoDownload -Url 'https://...' -Format '1' -OnEvent {
    param($event)
    Write-Host "[$($event.event)] $($event.data)"
}
```

---

### I. Browser Extension (Native Messaging)

The extension invokes `dl.bat` via a Native Messaging Host.

#### Native Messaging Host (`host.js`)

```javascript
const { execFile } = require('child_process');

process.stdin.on('data', (data) => {
    // Chrome native messaging format: first 4 bytes represent length
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

#### Extension-Side Invocation

```javascript
// content_script.js or background.js
chrome.runtime.sendNativeMessage('com.videodl.host', {
    url: 'https://www.youtube.com/watch?v=xxx',
    format: '1'
}, (response) => {
    if (response.success) {
        console.log('Download successful');
    }
});
```

---

### J. REST API / HTTP Server Wrapper

Wrap `dl.bat` into an HTTP API using Flask or FastAPI, supporting SSE or WebSocket for progress pushing.

#### Flask + SSE Example

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

# Client (JavaScript)
# const es = new EventSource('/download?url=https://...&format=1');
# es.onmessage = e => { const evt = JSON.parse(e.data); console.log(evt); };
```

#### FastAPI + WebSocket Example

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

## Format Options

| Code | Description | Output Location |
| --- | --- | --- |
| `1` | Best quality MP4 (auto 4K/2K/1080p), embedded subtitles & thumbnail **[Default]** | `videos\` |
| `2` | 1080p Max MP4, embedded subtitles & thumbnail | `videos\` |
| `3` | 720p Max MP4, embedded subtitles & thumbnail | `videos\` |
| `4` | MP3 320k, embedded thumbnail | `videos\audio\mp3\` |
| `5` | Native pure audio (prioritizes downloading native M4A without re-encoding; automatically remuxes/converts if source lacks M4A), embedded thumbnail | `videos\audio\m4a\` |
| `6` | Entire playlist MP4, categorized by playlist folders | `videos\playlists\` |

---

## File Structure

```
Video_downloader\
├── dl.bat                  ← Main script (corresponds to this document)
├── yt-dlp.exe              ← Auto-downloaded (if missing)
├── ffmpeg.exe              ← Auto-downloaded (if missing)
├── ffprobe.exe             ← Auto-downloaded (if missing)
├── cookies.txt             ← Optional, login cookies
├── deno.exe                ← Optional, Deno JS Runtime
├── install-protocol.bat    ← Registers videodl:// protocol
├── electron-app\           ← Electron GUI frontend
├── browser-extension\      ← Browser extension
└── videos\                 ← Download output directory
    ├── audio\
    │   ├── mp3\
    │   └── m4a\
    └── playlists\
```

---

## Interactive Mode Commands

In interactive mode (launched without arguments), you can enter the following commands in the URL prompt:

| Input | Action |
| --- | --- |
| `u` / `update` / `--update` | Update yt-dlp |
| `about` / `--about` | Display version info |
| `help` / `--help` / `-h` | Display help instructions |
| Blank Enter | Exit program |

---

> [!TIP]
> It is recommended to configure both `VIDEODL_NO_PAUSE=1` and `VIDEODL_JSON=1` for all GUI integrations. This prevents `pause` commands from hanging the child process while enabling structured status information to be retrieved from the JSON event stream.

---

> [!WARNING]
> The `videodl://` protocol requires writing your own handler, and the invoked application must be your own; otherwise, it will not function.  
> Below is the `.reg` file configuration:

```ini
Windows Registry Editor Version 5.00

[HKEY_CURRENT_USER\Software\Classes\videodl]
@="URL:Video Downloader Protocol"
"URL Protocol"=""

[HKEY_CURRENT_USER\Software\Classes\videodl\shell\open\command]
@="\"C:\\Users\\Administrator\\Desktop\\video_downloader\\electron-app\\dist\\win-unpacked\\VideoDownloader.exe\" \"%1\"" // This must be changed to your application directory
```

你可以直接把上面這段內容存成 `readme-en-us.md` 使用。

需要我再幫你調整什麼嗎？
