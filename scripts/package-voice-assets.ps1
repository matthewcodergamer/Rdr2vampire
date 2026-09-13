param(
  [string]$RepositoryRoot=(Split-Path -Parent $PSScriptRoot),
  [string]$OutputDirectory=""
)
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest

$root=(Resolve-Path $RepositoryRoot).Path
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
  $OutputDirectory=Join-Path $root "artifacts"
} elseif (-not [System.IO.Path]::IsPathRooted($OutputDirectory)) {
  $OutputDirectory=Join-Path (Get-Location).Path $OutputDirectory
}
$OutputDirectory=[System.IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null

$manifest=Join-Path $root "content/Nightwalker.audio"
$dialogue=Join-Path $root "Nightwalker.voice.dialogue"
$voiceSource=Join-Path $root "content/audio"
foreach ($required in @($manifest,$dialogue)) {
  if (-not (Test-Path $required -PathType Leaf)) { throw "Missing voice-pack input: $required" }
}
if (-not (Test-Path $voiceSource -PathType Container)) { throw "Missing canonical voice directory: content/audio" }

$stage=Join-Path $OutputDirectory "Nightwalker-Voice-Assets-stage"
$package=Join-Path $stage "Nightwalker-Voice-Assets"
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Path $package -Force | Out-Null

Copy-Item $manifest (Join-Path $package "Nightwalker.audio")
Copy-Item $dialogue (Join-Path $package "Nightwalker.voice.dialogue")

$voiceDestination=Join-Path $package "audio"
New-Item -ItemType Directory -Path $voiceDestination -Force | Out-Null
$voiceFiles=@(Get-ChildItem $voiceSource -File -Filter "*.wav" | Sort-Object Name)
if ($voiceFiles.Count -ne 59) { throw "Expected 59 canonical voice WAVs, found $($voiceFiles.Count)." }
foreach ($voiceFile in $voiceFiles) {
  if ($voiceFile.Name -match 'voice_batch_1') { throw "Fallback voice filename survived cleanup: $($voiceFile.Name)" }
  Copy-Item $voiceFile.FullName (Join-Path $voiceDestination $voiceFile.Name)
}

$copied=@(Get-ChildItem $voiceDestination -File | Sort-Object Name)
if ($copied.Count -ne 59) { throw "Expected 59 packaged voice WAVs, found $($copied.Count)." }
foreach ($voiceFile in $copied) {
  if ($voiceFile.Extension.ToLowerInvariant() -ne ".wav") {
    throw "Unsupported voice asset: $($voiceFile.Name)"
  }
}

$zip=Join-Path $OutputDirectory "Nightwalker-Voice-Assets-All-Batches.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Push-Location $package
try { Compress-Archive -Path "*" -DestinationPath $zip -CompressionLevel Optimal }
finally { Pop-Location }
Remove-Item $stage -Recurse -Force
Write-Host "VOICE_ARTIFACT_NAME=Nightwalker-Voice-Assets-All-Batches.zip"
