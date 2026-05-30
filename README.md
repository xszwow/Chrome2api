# Chrome2api

OpenAI-compatible local API wrapper for Chrome Gemini Nano / ChromeML.

This project is an experimental runner and API server around Chrome's on-device
Gemini Nano runtime. It does not include Google model weights or proprietary
Chrome runtime binaries.

## What This Is

Chrome2api provides:

- A bare ChromeML runner source file.
- A local OpenAI-compatible HTTP API server.
- PowerShell wrappers for Windows.
- Reference Chromium / Dawn headers used to understand the ABI.

The intended runtime chain is:

```text
OpenAI-compatible client
  -> http://127.0.0.1:11435/v1/chat/completions
  -> runtime/chromeml_api_server.cjs
  -> runtime/ChromeMLBareRunner.exe
  -> optimization_guide_internal.dll
  -> webgpu_dawn.dll
  -> weights.bin
```

## What Is Not Included

The following files are intentionally not included:

```text
runtime/ChromeMLBareRunner.exe
runtime/optimization_guide_internal.dll
runtime/webgpu_dawn.dll
model/OptGuideOnDeviceModel/2025.8.8.1141/weights.bin
```

These files are not authored by this project. If you use this project locally,
you must provide them yourself from a compatible local Chrome / Chrome component
installation and comply with the relevant licenses and terms.

Do not upload those files to a public GitHub repository or redistribute them as
part of this project. They may be large, proprietary, and subject to Google /
Chrome licensing terms. This repository only publishes the project-authored
wrapper code and BSD-licensed reference headers.

## Add Local Runtime Files

After cloning, copy your local runtime files into these exact paths:

```text
Chrome2api/
  runtime/
    optimization_guide_internal.dll
    webgpu_dawn.dll
  model/
    OptGuideOnDeviceModel/
      2025.8.8.1141/
        weights.bin
```

Typical source locations on a Windows machine with Chrome's on-device model
component installed may look like:

```text
%LOCALAPPDATA%\Google\Chrome\User Data\OptGuideOnDeviceModel\2025.8.8.1141\weights.bin
C:\Program Files\Google\Chrome\Application\<chrome-version>\optimization_guide_internal.dll
```

`webgpu_dawn.dll` must match the ChromeML runtime you are using. In the original
local experiment it was copied next to the runner as:

```text
runtime\webgpu_dawn.dll
```

The runner source is open, so `runtime/ChromeMLBareRunner.exe` should be built
locally instead of committed to the repository.

## Expected Layout

After adding local runtime files, the folder should look like:

```text
Chrome2api/
  runtime/
    ChromeMLBareRunner.exe
    ChromeMLBareRunner.cpp
    chromeml_api_server.cjs
    optimization_guide_internal.dll
    webgpu_dawn.dll
  model/
    OptGuideOnDeviceModel/
      2025.8.8.1141/
        weights.bin
  docs/
    OPENAI_COMPAT_API.md
  running scripts...
```

## Build Runner

The runner can be built with Zig's bundled clang on Windows:

```powershell
.\scripts\build_runner.ps1
```

Or manually:

```powershell
zig c++ -target x86_64-windows-gnu -std=c++17 -O2 -municode `
  -Iruntime runtime\ChromeMLBareRunner.cpp `
  -o runtime\ChromeMLBareRunner.exe `
  -lole32 -lwindowscodecs -lpsapi
```

## Run Bare Runner

```powershell
.\运行裸Runner.ps1 "Say exactly OK."
```

Multi-image input is supported by repeating `-Image`:

```powershell
.\运行裸Runner.ps1 "Compare these images." `
  -Image C:\images\a.png `
  -Image C:\images\b.jpg
```

## Run API Server

```powershell
.\启动API服务.ps1 -HostAddress 127.0.0.1 -Port 11435
```

Then test:

```powershell
curl.exe http://127.0.0.1:11435/v1/models
```

```powershell
curl.exe --% -s http://127.0.0.1:11435/v1/chat/completions -H "Content-Type: application/json" -d "{\"model\":\"chrome-gemini-nano\",\"messages\":[{\"role\":\"user\",\"content\":\"Say exactly OK.\"}],\"max_tokens\":16}"
```

See [docs/OPENAI_COMPAT_API.md](docs/OPENAI_COMPAT_API.md) for API details.

## Current Capabilities

- Text prompt.
- Stateless multi-turn `messages`.
- Image input through local file paths or file URLs.
- Multiple image inputs at the runner layer.
- One audio file input at the runner layer.
- `max_tokens` mapped to output length.
- `temperature`, `top_k`, and context-token options passed to the runner.
- Non-streaming and simulated SSE streaming responses.

## Compatibility Notes

Chrome2api is not Chrome Prompt API itself. It is a local compatibility layer
around the lower ChromeML runtime. It does not implement browser permissions,
Origin Trial logic, DOM input objects, or Chrome profile model registration.

## Legal / Licensing

Project-authored source is released under the MIT License. Chromium and Dawn
reference files remain under their original BSD-style licenses. Google model
weights and proprietary runtime binaries are not included and are not licensed
by this project.
