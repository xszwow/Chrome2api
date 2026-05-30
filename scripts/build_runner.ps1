param(
  [string]$Zig = "zig"
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$src = Join-Path $root "runtime\ChromeMLBareRunner.cpp"
$out = Join-Path $root "runtime\ChromeMLBareRunner.exe"

& $Zig c++ `
  -target x86_64-windows-gnu `
  -std=c++17 `
  -O2 `
  -municode `
  "-I$($root)\runtime" `
  $src `
  -o $out `
  -lole32 `
  -lwindowscodecs `
  -lpsapi

Write-Host "Built $out"

