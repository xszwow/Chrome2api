$ErrorActionPreference = "Continue"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$promptParts = @()
$images = @()
$audio = $null
$audioTargetRate = 16000
$contextTokens = $null
$maxOutput = 128
$topK = $null
$temperature = $null
$shortContext = $false
$top1 = $false
$noSafetyReserve = $false
$traceRuntime = $false

for ($i = 0; $i -lt $args.Count; $i++) {
  $arg = [string]$args[$i]
  if (($arg -eq "-Image" -or $arg -eq "--image") -and $i + 1 -lt $args.Count) {
    $images += [string]$args[++$i]
  } elseif (($arg -eq "-Audio" -or $arg -eq "--audio") -and $i + 1 -lt $args.Count) {
    $audio = [string]$args[++$i]
  } elseif (($arg -eq "-AudioTargetRate" -or $arg -eq "--audio-target-rate") -and $i + 1 -lt $args.Count) {
    $audioTargetRate = [int]$args[++$i]
  } elseif (($arg -eq "-ContextTokens" -or $arg -eq "--context-tokens" -or $arg -eq "--max-tokens") -and $i + 1 -lt $args.Count) {
    $contextTokens = [int]$args[++$i]
  } elseif (($arg -eq "-MaxOutput" -or $arg -eq "--max-output") -and $i + 1 -lt $args.Count) {
    $maxOutput = [int]$args[++$i]
  } elseif (($arg -eq "-TopK" -or $arg -eq "--top-k" -or $arg -eq "--topk") -and $i + 1 -lt $args.Count) {
    $topK = [int]$args[++$i]
  } elseif (($arg -eq "-Temperature" -or $arg -eq "--temperature") -and $i + 1 -lt $args.Count) {
    $temperature = [double]$args[++$i]
  } elseif ($arg -eq "-Short" -or $arg -eq "--short") {
    $shortContext = $true
  } elseif ($arg -eq "-Top1" -or $arg -eq "--top1") {
    $top1 = $true
  } elseif ($arg -eq "-NoSafetyReserve" -or $arg -eq "--no-safety-reserve") {
    $noSafetyReserve = $true
  } elseif ($arg -eq "-TraceRuntime" -or $arg -eq "--trace-runtime") {
    $traceRuntime = $true
  } else {
    $promptParts += $arg
  }
}

$prompt = $promptParts -join " "
if (-not $prompt) {
  if ($images.Count -gt 0 -and $audio) {
    $prompt = "Answer with image details and audio transcript."
  } elseif ($images.Count -gt 1) {
    $prompt = "Compare these images."
  } elseif ($images.Count -eq 1) {
    $prompt = "Describe this image."
  } elseif ($audio) {
    $prompt = "Transcribe this audio."
  } else {
    $prompt = "Say exactly OK."
  }
}

$runner = Join-Path $root "runtime\ChromeMLBareRunner.exe"
$chromeMl = Join-Path $root "runtime\optimization_guide_internal.dll"
$dawn = Join-Path $root "runtime\webgpu_dawn.dll"
$weights = Join-Path $root "model\OptGuideOnDeviceModel\2025.8.8.1141\weights.bin"

$runnerArgs = @(
  "--gpu",
  "--chat-input",
  "--no-cache",
  "--fast",
  "--prompt", $prompt,
  "--dll", $chromeMl,
  "--dawn-dll", $dawn,
  "--weights", $weights,
  "--max-output", $maxOutput
)

if ($contextTokens -ne $null) {
  $runnerArgs += @("--context-tokens", $contextTokens)
}
if ($shortContext) {
  $runnerArgs += "--short"
}
if ($topK -ne $null) {
  $runnerArgs += @("--top-k", $topK)
}
if ($top1) {
  $runnerArgs += "--top1"
}
if ($temperature -ne $null) {
  $runnerArgs += @("--temperature", $temperature)
}
if ($noSafetyReserve) {
  $runnerArgs += "--no-safety-reserve"
}
foreach ($image in $images) {
  $runnerArgs += @("--image", $image)
}
if ($audio) {
  $runnerArgs += @("--audio", $audio, "--audio-target-rate", $audioTargetRate)
}
if ($traceRuntime) {
  $runnerArgs += "--trace-runtime"
}

& $runner @runnerArgs
