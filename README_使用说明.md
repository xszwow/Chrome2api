# Chrome Gemini Nano BareOnly

这是只保留裸 DLL runner 链路的版本，不包含 `chrome.exe`、Chrome Prompt API 测试页、profile 缓存和 CDP runner。

## 这个版本的意义

它验证并保留的是最短运行链路：

```text
ChromeMLBareRunner.exe
  -> optimization_guide_internal.dll
  -> webgpu_dawn.dll
  -> weights.bin
```

也就是说：不启动 Chrome 浏览器进程，不调用网页 `LanguageModel` Prompt API，不需要 DevTools/CDP，也不需要安装系统 Chrome。

## 文件内容

```text
runtime\ChromeMLBareRunner.exe
runtime\ChromeMLBareRunner.cpp
runtime\optimization_guide_internal.dll
runtime\webgpu_dawn.dll
model\README.md
data\red_test.png
data\blue_test.png
data\audio_test.wav
运行裸Runner.ps1
```

运行前需要把 `weights.bin` 放到下面的位置：

```text
model\OptGuideOnDeviceModel\2025.8.8.1141\weights.bin
```

`weights.bin` 是数 GB 的本地模型权重，超过 GitHub 普通仓库单文件限制，因此不提交到仓库。

## 运行

```powershell
cd "$env:USERPROFILE\Desktop\Chrome_Gemini_Nano_BareOnly"
.\运行裸Runner.ps1 "Say exactly OK."
```

默认是 4K 上下文，对齐裸 exe 和 ChromeML 内置执行器的 `max_tokens=4096` 路径。和 Chrome 内置 runner 一样，append 阶段默认留 2 个 token 的 safety reserve，所以实际输入上限按 `4094` 传入。这里是 2 token，不是 2K。

只有显式加 `-Short` / `--short` 才会切到 1K：

```powershell
.\运行裸Runner.ps1 "Say exactly OK." -Short
```

采样默认值也按当前 ChromeML 内置路径靠齐：

```text
temperature=0.0
top_k=3
```

如果要强制贪心 top-1，可以加：

```powershell
.\运行裸Runner.ps1 "Say exactly OK." -Top1
```

可调参数：

```powershell
.\运行裸Runner.ps1 "Say exactly OK." -ContextTokens 4096 -TopK 3 -Temperature 0 -MaxOutput 128
.\运行裸Runner.ps1 "Say exactly OK." -ContextTokens 1024 -NoSafetyReserve
```

图片：

```powershell
.\运行裸Runner.ps1 "What is the dominant color? Return one word." -Image .\data\red_test.png
```

多图：

```powershell
.\运行裸Runner.ps1 "There are two images. Return the dominant color of each image in order, separated by comma." -Image .\data\red_test.png -Image .\data\blue_test.png -MaxOutput 32
```

音频：

```powershell
.\运行裸Runner.ps1 "Transcribe this audio." -Audio .\data\audio_test.wav
```

图片加音频：

```powershell
.\运行裸Runner.ps1 "Answer with color then transcript, separated by a semicolon." -Image .\data\red_test.png -Audio .\data\audio_test.wav -MaxOutput 96
```

## OpenAI-compatible 本地 API

完整 API 参考见：

```text
docs\OPENAI_COMPAT_API.md
```

启动服务：

```powershell
cd "$env:USERPROFILE\Desktop\Chrome_Gemini_Nano_BareOnly"
.\启动API服务.ps1
```

默认监听：

```text
http://127.0.0.1:11435
```

模型列表：

```powershell
curl.exe http://127.0.0.1:11435/v1/models
```

文本 chat completions：

```powershell
curl.exe --% -s http://127.0.0.1:11435/v1/chat/completions -H "Content-Type: application/json" -d "{\"model\":\"chrome-gemini-nano\",\"messages\":[{\"role\":\"system\",\"content\":\"Answer briefly.\"},{\"role\":\"user\",\"content\":\"Say exactly OK.\"}],\"max_tokens\":16}"
```

流式 chat completions 使用 OpenAI 风格 SSE，返回 `chat.completion.chunk` 和最后的 `[DONE]`：

```powershell
curl.exe --% -sS -N http://127.0.0.1:11435/v1/chat/completions -H "Content-Type: application/json" -d "{\"model\":\"chrome-gemini-nano\",\"stream\":true,\"messages\":[{\"role\":\"user\",\"content\":\"Say exactly OK.\"}],\"max_tokens\":16}"
```

多轮是无状态的：客户端每次把完整 `messages` 发进来，服务端会拼成一次 prompt 后调用裸 runner。

可映射参数：

```text
max_tokens / max_completion_tokens -> -MaxOutput
temperature -> -Temperature
top_k -> -TopK
context_tokens -> -ContextTokens
audio_target_rate -> -AudioTargetRate
```

图片输入支持 OpenAI chat content array 里的多个 `image_url`，路径可以是 `file://` 或本地路径。相对路径按 BareOnly 根目录解析：

```powershell
curl.exe --% -s http://127.0.0.1:11435/v1/chat/completions -H "Content-Type: application/json" -d "{\"model\":\"chrome-gemini-nano\",\"messages\":[{\"role\":\"user\",\"content\":[{\"type\":\"text\",\"text\":\"What is the dominant color? Return one word.\"},{\"type\":\"image_url\",\"image_url\":{\"url\":\"file:///C:/Users/%E5%A4%8F/Desktop/Chrome_Gemini_Nano_BareOnly/data/red_test.png\"}}]}],\"max_tokens\":16}"
```

多图 API：

```powershell
curl.exe --% -s http://127.0.0.1:11435/v1/chat/completions -H "Content-Type: application/json" -d "{\"model\":\"chrome-gemini-nano\",\"messages\":[{\"role\":\"user\",\"content\":[{\"type\":\"text\",\"text\":\"There are two images. Return the dominant color of each image in order, separated by comma.\"},{\"type\":\"image_url\",\"image_url\":{\"url\":\"data\\red_test.png\"}},{\"type\":\"image_url\",\"image_url\":{\"url\":\"data\\blue_test.png\"}}]}],\"max_tokens\":32}"
```

流式多图同样支持，把上面的 JSON 加上 `"stream":true` 即可。

音频预留为本地 wav 路径，使用 `audio_url`：

```powershell
curl.exe --% -s http://127.0.0.1:11435/v1/chat/completions -H "Content-Type: application/json" -d "{\"model\":\"chrome-gemini-nano\",\"messages\":[{\"role\":\"user\",\"content\":[{\"type\":\"text\",\"text\":\"Transcribe this audio.\"},{\"type\":\"audio_url\",\"audio_url\":{\"url\":\"data\\audio_test.wav\"}}]}],\"max_tokens\":64}"
```

## 已验证结果

```text
text:        full_text=OK.
vision:      full_text=Red
multi-image: full_text=Red, Blue
audio:       full_text=Audio test 1 2 3.
image+audio: full_text=Red; Audio test one two three
```

4K 上下文也做过实际验证：1413 token 输入可正常 `append_ok=1` / `generate_ok=1`。

## 边界

这个版本不依赖 Chrome 进程，但仍依赖 Google 的 ChromeML runtime DLL：

```text
optimization_guide_internal.dll
webgpu_dawn.dll
```

它不是 GGUF/Ollama/Transformers 那种纯裸权重格式。要去掉这些 DLL，就需要重写 ChromeML/LiteRT-LM loader、模型资源解析和 GPU backend。
