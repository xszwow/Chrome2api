"use strict";

const http = require("http");
const fs = require("fs");
const path = require("path");
const { spawn } = require("child_process");
const { StringDecoder } = require("string_decoder");
const { fileURLToPath } = require("url");

const ROOT_DIR = path.resolve(__dirname, "..");
const RUNNER_SCRIPT = path.join(ROOT_DIR, "运行裸Runner.ps1");
const DEFAULT_MODEL = "chrome-gemini-nano";
const HOST = process.env.CHROMEML_API_HOST || "127.0.0.1";
const PORT = parseInt(process.env.CHROMEML_API_PORT || "11435", 10);
const REQUEST_TIMEOUT_MS = parseInt(
  process.env.CHROMEML_API_TIMEOUT_MS || "120000",
  10,
);
const MAX_BODY_BYTES = parseInt(
  process.env.CHROMEML_API_MAX_BODY_BYTES || String(10 * 1024 * 1024),
  10,
);
const TEMP_MEDIA_DIR = path.join(ROOT_DIR, "runtime", "tmp", "media");
const DEBUG_PROMPT = process.env.CHROMEML_API_DEBUG_PROMPT === "1";

let runnerQueue = Promise.resolve();

function nowSeconds() {
  return Math.floor(Date.now() / 1000);
}

function makeId(prefix) {
  return `${prefix}-${Date.now().toString(36)}-${Math.random()
    .toString(36)
    .slice(2, 10)}`;
}

function sendJson(res, statusCode, payload) {
  const body = JSON.stringify(payload);
  res.writeHead(statusCode, {
    "content-type": "application/json; charset=utf-8",
    "content-length": Buffer.byteLength(body),
    "access-control-allow-origin": "*",
    "access-control-allow-methods": "GET,POST,OPTIONS",
    "access-control-allow-headers": "content-type,authorization",
  });
  res.end(body);
}

function sendError(res, statusCode, message, type = "invalid_request_error") {
  sendJson(res, statusCode, {
    error: {
      message,
      type,
      code: null,
    },
  });
}

function sendSseHeaders(res) {
  res.writeHead(200, {
    "content-type": "text/event-stream; charset=utf-8",
    "cache-control": "no-cache, no-transform",
    connection: "keep-alive",
    "access-control-allow-origin": "*",
    "access-control-allow-methods": "GET,POST,OPTIONS",
    "access-control-allow-headers": "content-type,authorization",
  });
  if (typeof res.flushHeaders === "function") {
    res.flushHeaders();
  }
}

function writeSseData(res, payload) {
  res.write(`data: ${JSON.stringify(payload)}\n\n`);
}

function writeSseDone(res) {
  res.write("data: [DONE]\n\n");
}

function readJsonBody(req) {
  return new Promise((resolve, reject) => {
    const chunks = [];
    let total = 0;

    req.on("data", (chunk) => {
      total += chunk.length;
      if (total > MAX_BODY_BYTES) {
        reject(Object.assign(new Error("Request body too large"), { status: 413 }));
        req.destroy();
        return;
      }
      chunks.push(chunk);
    });

    req.on("end", () => {
      try {
        const text = Buffer.concat(chunks).toString("utf8");
        resolve(text ? JSON.parse(text) : {});
      } catch (err) {
        reject(Object.assign(new Error("Invalid JSON request body"), { status: 400 }));
      }
    });

    req.on("error", reject);
  });
}

function asPositiveInt(value, name) {
  if (value === undefined || value === null) {
    return null;
  }
  const number = Number(value);
  if (!Number.isInteger(number) || number <= 0) {
    throw new HttpError(400, `${name} must be a positive integer`);
  }
  return number;
}

function asFiniteNumber(value, name) {
  if (value === undefined || value === null) {
    return null;
  }
  const number = Number(value);
  if (!Number.isFinite(number)) {
    throw new HttpError(400, `${name} must be a finite number`);
  }
  return number;
}

class HttpError extends Error {
  constructor(status, message) {
    super(message);
    this.status = status;
  }
}

function normalizeLocalPath(raw, label) {
  if (typeof raw !== "string" || raw.trim() === "") {
    throw new HttpError(400, `${label} must be a local path or file:// URL`);
  }

  let value = raw.trim();
  let localPath;

  if (/^file:/i.test(value)) {
    try {
      localPath = fileURLToPath(value);
    } catch (err) {
      if (/^file:\/\/[A-Za-z]:/i.test(value)) {
        localPath = decodeURIComponent(value.replace(/^file:\/\//i, ""));
      } else {
        throw new HttpError(400, `${label} file URL is invalid`);
      }
    }
  } else if (/^https?:/i.test(value)) {
    throw new HttpError(400, `${label} only supports local file paths in this build`);
  } else {
    localPath = value;
  }

  if (!path.isAbsolute(localPath)) {
    localPath = path.resolve(ROOT_DIR, localPath);
  }

  if (!fs.existsSync(localPath)) {
    throw new HttpError(400, `${label} does not exist: ${localPath}`);
  }

  return pathForRunner(localPath);
}

function decodeDataImageToTempFile(raw, label, media) {
  if (typeof raw !== "string") {
    return null;
  }

  const match = raw.match(/^data:image\/([a-zA-Z0-9.+-]+);base64,([\s\S]*)$/);
  if (!match) {
    return null;
  }

  const format = match[1].toLowerCase();
  const extensionMap = {
    jpeg: "jpg",
    jpg: "jpg",
    png: "png",
    gif: "gif",
    bmp: "bmp",
    webp: "webp",
    tiff: "tiff",
  };
  const extension = extensionMap[format];
  if (!extension) {
    throw new HttpError(400, `${label} data image format is not supported: ${format}`);
  }

  let bytes;
  try {
    bytes = Buffer.from(match[2].replace(/\s+/g, ""), "base64");
  } catch (err) {
    throw new HttpError(400, `${label} data image base64 is invalid`);
  }

  if (bytes.length === 0) {
    throw new HttpError(400, `${label} data image is empty`);
  }
  if (bytes.length > MAX_BODY_BYTES) {
    throw new HttpError(413, `${label} data image is too large`);
  }

  fs.mkdirSync(TEMP_MEDIA_DIR, { recursive: true });
  const filename = `image-${Date.now().toString(36)}-${Math.random()
    .toString(36)
    .slice(2, 10)}.${extension}`;
  const localPath = path.join(TEMP_MEDIA_DIR, filename);
  fs.writeFileSync(localPath, bytes);
  media.tempFiles.push(localPath);
  return pathForRunner(localPath);
}

function normalizeImageInput(raw, label, media) {
  return decodeDataImageToTempFile(raw, label, media) || normalizeLocalPath(raw, label);
}

function pathForRunner(localPath) {
  const relative = path.relative(ROOT_DIR, localPath);
  if (relative && !relative.startsWith("..") && !path.isAbsolute(relative)) {
    return relative;
  }
  return localPath;
}

function extractUrlish(value) {
  if (typeof value === "string") {
    return value;
  }
  if (value && typeof value === "object") {
    if (typeof value.url === "string") {
      return value.url;
    }
    if (typeof value.path === "string") {
      return value.path;
    }
    if (typeof value.file === "string") {
      return value.file;
    }
  }
  return null;
}

function roleLabel(message) {
  const role = typeof message.role === "string" ? message.role : "user";
  if (typeof message.name === "string" && message.name.trim()) {
    return `${role}(${message.name.trim()})`;
  }
  return role;
}

function extractContent(message, media) {
  const content = message.content;
  if (content === null || content === undefined) {
    return "";
  }

  if (typeof content === "string") {
    return content;
  }

  if (!Array.isArray(content)) {
    return JSON.stringify(content);
  }

  const textParts = [];
  for (const part of content) {
    if (typeof part === "string") {
      textParts.push(part);
      continue;
    }
    if (!part || typeof part !== "object") {
      continue;
    }

    const type = typeof part.type === "string" ? part.type : "";
    if (type === "text" || type === "input_text") {
      if (typeof part.text === "string") {
        textParts.push(part.text);
      }
      continue;
    }

    if (type === "image_url" || type === "input_image" || part.image_url) {
      const raw = extractUrlish(part.image_url || part.image || part);
      if (!raw) {
        throw new HttpError(400, "image_url must contain a URL or path");
      }
      const imageIndex = media.imagePaths.length + 1;
      media.imagePaths.push(normalizeImageInput(raw, `image_url[${imageIndex}]`, media));
      textParts.push(`[Image ${imageIndex} attached]`);
      continue;
    }

    if (
      type === "audio_url" ||
      type === "input_audio" ||
      type === "audio" ||
      part.audio_url
    ) {
      const raw = extractUrlish(part.audio_url || part.audio || part.input_audio || part);
      if (!raw) {
        throw new HttpError(400, "audio_url must contain a URL or path");
      }
      if (media.audioPath) {
        throw new HttpError(400, "Only one audio file is supported by this runner build");
      }
      media.audioPath = normalizeLocalPath(raw, "audio_url");
      textParts.push("[Audio attached]");
      continue;
    }

    if (typeof part.text === "string") {
      textParts.push(part.text);
    }
  }

  return textParts.join("\n");
}

function buildPrompt(messages, media) {
  const lines = [];
  for (const message of messages) {
    if (!message || typeof message !== "object") {
      throw new HttpError(400, "Each message must be an object");
    }
    const text = extractContent(message, media).trim();
    const role = roleLabel(message);
    if (text) {
      lines.push(`${role}:\n${text}`);
    } else {
      lines.push(`${role}:`);
    }
  }
  lines.push("assistant:");
  return lines.join("\n\n");
}

function extractFullText(stdout) {
  const stripTransportLineEndings = (text) => text.replace(/[\r\n]+$/g, "");
  const anchored = stdout.match(
    /generate_wait=\d+\s+complete_status=(-?\d+)\s+full_text=([\s\S]*?)\r?\ndestroyed_session=1/,
  );
  if (anchored) {
    return {
      status: Number(anchored[1]),
      text: stripTransportLineEndings(anchored[2]),
    };
  }

  const marker = "full_text=";
  const index = stdout.lastIndexOf(marker);
  if (index < 0) {
    return null;
  }

  const rest = stdout.slice(index + marker.length);
  const line = rest.split(/\r?\n/, 1)[0];
  return {
    status: null,
    text: stripTransportLineEndings(line),
  };
}

function extractGenerateOutputToken(line) {
  const match = line.match(/^generate_output status=(-?\d+) text=([\s\S]*?) tool_calls=\d+$/);
  if (!match || Number(match[1]) !== 0) {
    return null;
  }
  return match[2];
}

function runRunner(job, options = {}) {
  const args = [
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
    RUNNER_SCRIPT,
    job.prompt,
  ];

  for (const imagePath of job.imagePaths || []) {
    args.push("-Image", imagePath);
  }
  if (job.audioPath) {
    args.push("-Audio", job.audioPath);
  }
  if (job.audioTargetRate !== null) {
    args.push("-AudioTargetRate", String(job.audioTargetRate));
  }
  if (job.contextTokens !== null) {
    args.push("-ContextTokens", String(job.contextTokens));
  }
  if (job.maxOutput !== null) {
    args.push("-MaxOutput", String(job.maxOutput));
  }
  if (job.topK !== null) {
    args.push("-TopK", String(job.topK));
  }
  if (job.temperature !== null) {
    args.push("-Temperature", String(job.temperature));
  }
  if (job.short) {
    args.push("-Short");
  }
  if (job.top1) {
    args.push("-Top1");
  }
  if (job.noSafetyReserve) {
    args.push("-NoSafetyReserve");
  }

  if (DEBUG_PROMPT) {
    console.error("prompt:\n" + job.prompt);
    console.error("runner args:", args.map((value) => JSON.stringify(value)).join(" "));
  }

  return new Promise((resolve, reject) => {
    const child = spawn("powershell.exe", args, {
      cwd: ROOT_DIR,
      windowsHide: true,
    });

    const onToken = typeof options.onToken === "function" ? options.onToken : null;
    const stdoutDecoder = new StringDecoder("utf8");
    const stderrDecoder = new StringDecoder("utf8");
    let stdout = "";
    let stderr = "";
    let stdoutLineBuffer = "";
    let timedOut = false;

    const processStdoutText = (text) => {
      if (!text) {
        return;
      }
      stdout += text;
      if (!onToken) {
        return;
      }
      stdoutLineBuffer += text;
      let newlineIndex;
      while ((newlineIndex = stdoutLineBuffer.indexOf("\n")) !== -1) {
        const line = stdoutLineBuffer.slice(0, newlineIndex).replace(/\r$/, "");
        stdoutLineBuffer = stdoutLineBuffer.slice(newlineIndex + 1);
        const token = extractGenerateOutputToken(line);
        if (token !== null && token !== "") {
          onToken(token);
        }
      }
    };

    const timer = setTimeout(() => {
      timedOut = true;
      child.kill();
    }, REQUEST_TIMEOUT_MS);

    child.stdout.on("data", (chunk) => processStdoutText(stdoutDecoder.write(chunk)));
    child.stderr.on("data", (chunk) => {
      stderr += stderrDecoder.write(chunk);
    });
    child.on("error", reject);
    child.on("close", (code, signal) => {
      clearTimeout(timer);
      processStdoutText(stdoutDecoder.end());
      stderr += stderrDecoder.end();

      if (timedOut) {
        reject(new Error(`Runner timed out after ${REQUEST_TIMEOUT_MS} ms`));
        return;
      }

      if (code !== 0) {
        const detail = (stderr || stdout).trim().slice(-2000);
        reject(new Error(`Runner exited with code ${code}${detail ? `: ${detail}` : ""}`));
        return;
      }

      const parsed = extractFullText(stdout);
      if (!parsed) {
        const detail = stdout.trim().slice(-2000);
        reject(new Error(`Runner output did not contain full_text=${detail ? `: ${detail}` : ""}`));
        return;
      }

      resolve({
        text: parsed.text,
        status: parsed.status,
        stdout,
        stderr,
        signal,
      });
    });
  });
}

function cleanupTempFiles(files) {
  for (const file of files || []) {
    try {
      fs.unlinkSync(file);
    } catch (err) {
      if (DEBUG_PROMPT) {
        console.error(`failed to delete temp file ${file}:`, err.message);
      }
    }
  }
}

function enqueueRunner(job) {
  const current = runnerQueue.then(() => runRunner(job));
  runnerQueue = current.catch(() => {});
  return current;
}

function enqueueStreamingRunner(job, options) {
  const current = runnerQueue.then(() => runRunner(job, options));
  runnerQueue = current.catch(() => {});
  return current;
}

function createCompletionResponse(request, text) {
  const created = nowSeconds();
  const model = request.model || DEFAULT_MODEL;
  return {
    id: makeId("chatcmpl-local"),
    object: "chat.completion",
    created,
    model,
    choices: [
      {
        index: 0,
        message: {
          role: "assistant",
          content: text,
        },
        finish_reason: "stop",
      },
    ],
    usage: {
      prompt_tokens: null,
      completion_tokens: null,
      total_tokens: null,
    },
  };
}

function createCompletionChunk(id, created, model, delta, finishReason = null, usage = undefined) {
  const chunk = {
    id,
    object: "chat.completion.chunk",
    created,
    model,
    choices: [
      {
        index: 0,
        delta,
        finish_reason: finishReason,
      },
    ],
  };
  if (usage !== undefined) {
    chunk.usage = usage;
  }
  return chunk;
}

async function streamCompletionResponse(request, job, res) {
  const created = nowSeconds();
  const model = request.model || DEFAULT_MODEL;
  const id = makeId("chatcmpl-local");
  let streamedText = "";

  sendSseHeaders(res);
  writeSseData(res, createCompletionChunk(id, created, model, { role: "assistant" }));

  try {
    const result = await enqueueStreamingRunner(job, {
      onToken: (token) => {
        streamedText += token;
        writeSseData(res, createCompletionChunk(id, created, model, { content: token }));
      },
    });

    if (result.text && result.text !== streamedText) {
      const rest = result.text.startsWith(streamedText)
        ? result.text.slice(streamedText.length)
        : streamedText
          ? ""
          : result.text;
      if (rest) {
        writeSseData(res, createCompletionChunk(id, created, model, { content: rest }));
      }
    }

    const usage = request.stream_options && request.stream_options.include_usage
      ? {
          prompt_tokens: null,
          completion_tokens: null,
          total_tokens: null,
        }
      : undefined;
    writeSseData(res, createCompletionChunk(id, created, model, {}, "stop", usage));
    writeSseDone(res);
    res.end();
  } catch (err) {
    console.error(err);
    if (!res.destroyed) {
      writeSseData(res, {
        error: {
          message: err.message || "Internal server error",
          type: "server_error",
          code: null,
        },
      });
      writeSseDone(res);
      res.end();
    }
  }
}

function modelListResponse() {
  return {
    object: "list",
    data: [
      {
        id: DEFAULT_MODEL,
        object: "model",
        created: 0,
        owned_by: "local",
      },
    ],
  };
}

async function handleChatCompletions(req, res) {
  let body;
  try {
    body = await readJsonBody(req);
  } catch (err) {
    sendError(res, err.status || 400, err.message);
    return;
  }

  try {
    if (body.n !== undefined && body.n !== null && Number(body.n) !== 1) {
      throw new HttpError(400, "Only n=1 is supported");
    }
    if (!Array.isArray(body.messages) || body.messages.length === 0) {
      throw new HttpError(400, "messages must be a non-empty array");
    }

    const media = {
      imagePaths: [],
      audioPath: null,
      tempFiles: [],
    };
    const prompt = buildPrompt(body.messages, media);

    const job = {
      prompt,
      imagePaths: media.imagePaths,
      audioPath: media.audioPath,
      audioTargetRate: asPositiveInt(body.audio_target_rate, "audio_target_rate"),
      maxOutput: asPositiveInt(
        body.max_tokens ?? body.max_completion_tokens,
        "max_tokens",
      ),
      contextTokens: asPositiveInt(
        body.context_tokens ?? body.contextTokens,
        "context_tokens",
      ),
      topK: asPositiveInt(body.top_k ?? body.topK, "top_k"),
      temperature: asFiniteNumber(body.temperature, "temperature"),
      short: Boolean(body.short),
      top1: Boolean(body.top1 || body.top_1),
      noSafetyReserve: Boolean(body.no_safety_reserve || body.noSafetyReserve),
      tempFiles: media.tempFiles,
    };

    try {
      if (body.stream) {
        await streamCompletionResponse(body, job, res);
        return;
      }

      const result = await enqueueRunner(job);
      sendJson(res, 200, createCompletionResponse(body, result.text));
    } finally {
      cleanupTempFiles(job.tempFiles);
    }
  } catch (err) {
    if (err instanceof HttpError) {
      sendError(res, err.status, err.message);
      return;
    }
    console.error(err);
    sendError(res, 500, err.message || "Internal server error", "server_error");
  }
}

function handleRequest(req, res) {
  const url = new URL(req.url, `http://${HOST}:${PORT}`);

  if (req.method === "OPTIONS") {
    sendJson(res, 200, {});
    return;
  }

  if (req.method === "GET" && url.pathname === "/v1/models") {
    sendJson(res, 200, modelListResponse());
    return;
  }

  if (req.method === "POST" && url.pathname === "/v1/chat/completions") {
    handleChatCompletions(req, res);
    return;
  }

  sendError(res, 404, `Unknown route: ${req.method} ${url.pathname}`);
}

const server = http.createServer(handleRequest);
server.on("clientError", (err, socket) => {
  socket.end("HTTP/1.1 400 Bad Request\r\n\r\n");
});
server.on("error", (err) => {
  console.error(err.message);
  process.exit(1);
});
server.listen(PORT, HOST, () => {
  console.log(`ChromeML OpenAI-compatible API listening at http://${HOST}:${PORT}`);
  console.log(`Model: ${DEFAULT_MODEL}`);
});
