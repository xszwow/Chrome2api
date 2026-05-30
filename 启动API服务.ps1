param(
  [int]$Port = 11435,
  [string]$HostAddress = "127.0.0.1",
  [switch]$DebugPrompt
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$server = Join-Path $root "runtime\chromeml_api_server.cjs"

$nodeCommand = Get-Command node -ErrorAction SilentlyContinue
if ($nodeCommand) {
  $node = $nodeCommand.Source
} else {
  $fallbacks = @(
    "C:\Users\Public\codex-node-v24.15.0-win-x64\node.exe",
    (Join-Path $env:USERPROFILE ".cache\codex-runtimes\codex-primary-runtime\dependencies\node\bin\node.exe")
  )
  $node = $fallbacks | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
}

if (-not $node) {
  throw "node.exe was not found. Install Node.js or set PATH to a Node runtime."
}

$env:CHROMEML_API_HOST = $HostAddress
$env:CHROMEML_API_PORT = [string]$Port
if ($DebugPrompt) {
  $env:CHROMEML_API_DEBUG_PROMPT = "1"
}

Write-Host "ChromeML OpenAI-compatible API: http://$HostAddress`:$Port"
Write-Host "Node: $node"
& $node $server
