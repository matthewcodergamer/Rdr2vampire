param(
  [string]$RepositoryRoot=(Split-Path -Parent $PSScriptRoot),
  [Parameter(Mandatory=$true)][string]$OutputPath
)
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest

$root=(Resolve-Path $RepositoryRoot).Path
$partsRoot=Join-Path $root "content/voicepack"
$parts=@(Get-ChildItem $partsRoot -File -Filter "Nightwalker.voicepack.part*.b64" | Sort-Object Name)
if ($parts.Count -ne 34) { throw "Expected 34 Nightwalker voicepack source chunks, found $($parts.Count)." }

$builder=New-Object System.Text.StringBuilder
foreach ($part in $parts) {
  $text=(Get-Content $part.FullName -Raw).Trim()
  if ([string]::IsNullOrWhiteSpace($text)) { throw "Empty voicepack chunk: $($part.Name)" }
  [void]$builder.Append($text)
}

try { $bytes=[Convert]::FromBase64String($builder.ToString()) }
catch { throw "Nightwalker voicepack source chunks are not valid base64: $($_.Exception.Message)" }

$sha=[System.Security.Cryptography.SHA256]::Create()
try {
  $digest=($sha.ComputeHash($bytes) | ForEach-Object { $_.ToString("x2") }) -join ""
} finally { $sha.Dispose() }
$expected="b377d1ce81ac8c0f5b86f8fba15270721bba2ba430d5a5a940ea01f793fa4ba6"
if ($digest -ne $expected) { throw "Nightwalker voicepack SHA-256 mismatch: $digest" }

$fullOutput=[System.IO.Path]::GetFullPath($OutputPath)
$parent=Split-Path -Parent $fullOutput
if ($parent) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
[System.IO.File]::WriteAllBytes($fullOutput,$bytes)
Write-Host "VOICEPACK_SHA256=$digest"
Write-Host "VOICEPACK_PATH=$fullOutput"
