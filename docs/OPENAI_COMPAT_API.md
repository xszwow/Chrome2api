# Chrome Gemini Nano BareOnly API Reference

本文件是 `Chrome_Gemini_Nano_BareOnly` 的 OpenAI-compatible 本地 API 说明。格式参考 OpenAI Chat Completions 文档，但内容以本项目当前实现为准。

## Base URL

```text
http://127.0.0.1:11435/v1
```

启动服务：

```powershell
cd "$env:USERPROFILE\Desktop\Chrome_Gemini_Nano_BareOnly"
.\启动API服务.ps1
```

自定义监听地址：

```powershell
.\启动API服务.ps1 -HostAddress 127.0.0.1 -Port 11435
```

## Authentication

本地服务默认不校验 API key。为了兼容 OpenAI SDK 或已有客户端，可以传任意 Bearer token：

```http
Authorization: Bearer local
```

服务端当前会忽略这个 header。

## Models

### List models

```http
GET /v1/models
```

Example:

```powershell
curl.exe http://127.0.0.1:11435/v1/models
```

Response:

```json
{
  "object": "list",
  "data": [
    {
      "id": "chrome-gemini-nano",
      "object": "model",
      "created": 0,
      "owned_by": "local"
    }
  ]
}
```

## Chat Completions

### Create chat completion

```http
POST /v1/chat/completions
```

最小文本请求：

```powershell
curl.exe --% -s http://127.0.0.1:11435/v1/chat/completions -H "Content-Type: application/json" -d "{\"model\":\"chrome-gemini-nano\",\"messages\":[{\"role\":\"user\",\"content\":\"Say exactly OK.\"}],\"max_tokens\":16}"
```

Response:

```json
{
  "id": "chatcmpl-local-...",
  "object": "chat.completion",
  "created": 1780130000,
  "model": "chrome-gemini-nano",
  "choices": [
    {
      "index": 0,
      "message": {
        "role": "assistant",
        "content": "OK."
      },
      "finish_reason": "stop"
    }
  ],
  "usage": {
    "prompt_tokens": null,
    "completion_tokens": null,
    "total_tokens": null
  }
}
```

## Request Body

Supported fields:

| Field | Type | Notes |
| --- | --- | --- |
| `model` | string | 推荐使用 `chrome-gemini-nano`。当前服务不按 model 路由，主要用于 OpenAI-compatible 响应回显。 |
| `messages` | array | 必填。完整对话历史。服务是无状态的，每次请求都要带完整 messages。 |
| `stream` | boolean | 可选。`true` 时返回 SSE streaming chunks。默认 `false`。 |
| `max_tokens` | integer | 可选。映射到 runner `-MaxOutput`。 |
| `max_completion_tokens` | integer | 可选。等价映射到 runner `-MaxOutput`。 |
| `temperature` | number | 可选。映射到 runner `-Temperature`。 |
| `top_k` | integer | 可选。映射到 runner `-TopK`。 |
| `context_tokens` | integer | 可选。映射到 runner `-ContextTokens`。 |
| `audio_target_rate` | integer | 可选。映射到 runner `-AudioTargetRate`。 |
| `n` | integer | 可选。当前只支持 `1`。 |
| `stream_options.include_usage` | boolean | 可选。streaming 最后一个 chunk 可带 null usage。 |

Accepted but not implemented as OpenAI semantics:

| Field | Current behavior |
| --- | --- |
| `top_p`, `presence_penalty`, `frequency_penalty`, `stop`, `seed`, `tools`, `tool_choice`, `response_format`, `logprobs` | 当前不会映射到底层 runner。需要这些能力时先不要依赖本服务。 |

## Messages

每个 message 至少包含：

```json
{
  "role": "user",
  "content": "Hello"
}
```

支持的 role：

```text
system
user
assistant
tool
```

服务端不会保存历史。多轮对话要由客户端把完整历史放到 `messages`：

```json
[
  { "role": "system", "content": "Answer briefly." },
  { "role": "user", "content": "Say exactly OK." },
  { "role": "assistant", "content": "OK." },
  { "role": "user", "content": "Now say READY." }
]
```

内部会转换成一次 prompt，再调用 `运行裸Runner.ps1`。

## Text Input

`content` 可以是字符串：

```json
{
  "role": "user",
  "content": "Say exactly OK."
}
```

也可以是 content array：

```json
{
  "role": "user",
  "content": [
    { "type": "text", "text": "Say exactly OK." }
  ]
}
```

## Image Input

支持 OpenAI chat content array 风格的 `image_url`。当前只支持本地路径：

- 相对路径：按 BareOnly 根目录解析，例如 `data\\red_test.png`
- 绝对路径：例如 `C:\\Users\\夏\\Desktop\\Chrome_Gemini_Nano_BareOnly\\data\\red_test.png`
- `file://` URL：例如 `file:///C:/Users/%E5%A4%8F/Desktop/Chrome_Gemini_Nano_BareOnly/data/red_test.png`

不支持远程 `http://` 或 `https://` 图片 URL。

Single image:

```powershell
curl.exe --% -s http://127.0.0.1:11435/v1/chat/completions -H "Content-Type: application/json" -d "{\"model\":\"chrome-gemini-nano\",\"messages\":[{\"role\":\"user\",\"content\":[{\"type\":\"text\",\"text\":\"What is the dominant color? Return one word.\"},{\"type\":\"image_url\",\"image_url\":{\"url\":\"data\\red_test.png\"}}]}],\"max_tokens\":16}"
```

Multiple images:

```powershell
curl.exe --% -s http://127.0.0.1:11435/v1/chat/completions -H "Content-Type: application/json" -d "{\"model\":\"chrome-gemini-nano\",\"messages\":[{\"role\":\"user\",\"content\":[{\"type\":\"text\",\"text\":\"There are two images. Return the dominant color of each image in order, separated by comma.\"},{\"type\":\"image_url\",\"image_url\":{\"url\":\"data\\red_test.png\"}},{\"type\":\"image_url\",\"image_url\":{\"url\":\"data\\blue_test.png\"}}]}],\"max_tokens\":32}"
```

Expected content:

```text
Red, Blue
```

## Audio Input

支持本地 wav 文件路径，使用 `audio_url`：

```powershell
curl.exe --% -s http://127.0.0.1:11435/v1/chat/completions -H "Content-Type: application/json" -d "{\"model\":\"chrome-gemini-nano\",\"messages\":[{\"role\":\"user\",\"content\":[{\"type\":\"text\",\"text\":\"Transcribe this audio.\"},{\"type\":\"audio_url\",\"audio_url\":{\"url\":\"data\\audio_test.wav\"}}]}],\"max_tokens\":64}"
```

当前限制：

- 只支持一个音频文件。
- 推荐 wav。
- 路径必须是本地文件路径或 `file://`。

## Streaming

设置 `stream: true` 时，接口返回 `text/event-stream`：

```powershell
curl.exe --% -sS -N http://127.0.0.1:11435/v1/chat/completions -H "Content-Type: application/json" -d "{\"model\":\"chrome-gemini-nano\",\"stream\":true,\"messages\":[{\"role\":\"user\",\"content\":\"Say exactly OK.\"}],\"max_tokens\":16}"
```

Example stream:

```text
data: {"id":"chatcmpl-local-...","object":"chat.completion.chunk","created":1780130000,"model":"chrome-gemini-nano","choices":[{"index":0,"delta":{"role":"assistant"},"finish_reason":null}]}

data: {"id":"chatcmpl-local-...","object":"chat.completion.chunk","created":1780130000,"model":"chrome-gemini-nano","choices":[{"index":0,"delta":{"content":"OK"},"finish_reason":null}]}

data: {"id":"chatcmpl-local-...","object":"chat.completion.chunk","created":1780130000,"model":"chrome-gemini-nano","choices":[{"index":0,"delta":{"content":"."},"finish_reason":null}]}

data: {"id":"chatcmpl-local-...","object":"chat.completion.chunk","created":1780130000,"model":"chrome-gemini-nano","choices":[{"index":0,"delta":{},"finish_reason":"stop"}]}

data: [DONE]
```

Streaming also works with image input and multiple images.

## OpenAI SDK Usage

Because the service is OpenAI-compatible, most clients can point their base URL at the local server.

JavaScript:

```javascript
import OpenAI from "openai";

const client = new OpenAI({
  apiKey: "local",
  baseURL: "http://127.0.0.1:11435/v1",
});

const completion = await client.chat.completions.create({
  model: "chrome-gemini-nano",
  messages: [{ role: "user", content: "Say exactly OK." }],
  max_tokens: 16,
});

console.log(completion.choices[0].message.content);
```

Python:

```python
from openai import OpenAI

client = OpenAI(
    api_key="local",
    base_url="http://127.0.0.1:11435/v1",
)

completion = client.chat.completions.create(
    model="chrome-gemini-nano",
    messages=[{"role": "user", "content": "Say exactly OK."}],
    max_tokens=16,
)

print(completion.choices[0].message.content)
```

Streaming in JavaScript:

```javascript
const stream = await client.chat.completions.create({
  model: "chrome-gemini-nano",
  stream: true,
  messages: [{ role: "user", content: "Say exactly OK." }],
  max_tokens: 16,
});

for await (const chunk of stream) {
  process.stdout.write(chunk.choices[0]?.delta?.content ?? "");
}
```

## Error Responses

Errors use an OpenAI-style shape:

```json
{
  "error": {
    "message": "messages must be a non-empty array",
    "type": "invalid_request_error",
    "code": null
  }
}
```

Common cases:

| HTTP status | Cause |
| --- | --- |
| `400` | Invalid JSON, missing `messages`, unsupported remote media URL, invalid parameter type. |
| `404` | Unknown route. |
| `413` | Request body exceeds the configured limit. |
| `500` | Runner failed, timed out, or did not produce `full_text`. |

## Runtime Behavior

- Requests are queued one at a time before calling the runner.
- Each request starts a runner process through `运行裸Runner.ps1`.
- The server parses `full_text=...` for non-streaming output.
- For streaming, the server forwards runner `generate_output status=0 text=...` fragments as SSE chunks.
- Usage token counts are currently `null`.

## Compatibility Matrix

| Feature | Status |
| --- | --- |
| `GET /v1/models` | Supported |
| `POST /v1/chat/completions` | Supported |
| Non-streaming text | Supported |
| Streaming text | Supported |
| Stateless multi-turn messages | Supported |
| Multiple local images | Supported |
| One local wav audio input | Supported |
| Remote image/audio URLs | Not supported |
| Tool calls | Not supported |
| JSON schema / structured outputs | Not supported |
| Embeddings, responses, assistants, files | Not implemented |

