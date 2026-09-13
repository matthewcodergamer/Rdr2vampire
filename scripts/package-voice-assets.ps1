param(
  [string]$RepositoryRoot=(Split-Path -Parent $PSScriptRoot),
  [string]$OutputDirectory="",
  [string]$VoiceAssetsDirectory="",
  [string]$VoicePackPath=""
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
foreach ($required in @($manifest,$dialogue)) {
  if (-not (Test-Path $required -PathType Leaf)) { throw "Missing voice-pack input: $required" }
}

$stage=Join-Path $OutputDirectory "Nightwalker-Voice-Assets-stage"
$package=Join-Path $stage "Nightwalker-Voice-Assets"
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Path $package -Force | Out-Null
Copy-Item $manifest (Join-Path $package "Nightwalker.audio")
Copy-Item $dialogue (Join-Path $package "Nightwalker.voice.dialogue")

$voiceSource=$null
if (-not [string]::IsNullOrWhiteSpace($VoiceAssetsDirectory) -and -not [string]::IsNullOrWhiteSpace($VoicePackPath)) {
  throw "Specify either VoiceAssetsDirectory or VoicePackPath, not both."
}
if (-not [string]::IsNullOrWhiteSpace($VoiceAssetsDirectory)) {
  $candidate=$VoiceAssetsDirectory
  if (-not [System.IO.Path]::IsPathRooted($candidate)) { $candidate=Join-Path (Get-Location).Path $candidate }
  if (-not (Test-Path $candidate -PathType Container)) { throw "VoiceAssetsDirectory does not exist: $candidate" }
  $voiceSource=(Resolve-Path $candidate).Path
} elseif (-not [string]::IsNullOrWhiteSpace($VoicePackPath)) {
  $candidate=$VoicePackPath
  if (-not [System.IO.Path]::IsPathRooted($candidate)) { $candidate=Join-Path (Get-Location).Path $candidate }
  if (-not (Test-Path $candidate -PathType Leaf)) { throw "VoicePackPath does not exist: $candidate" }
  Add-Type -AssemblyName System.IO.Compression.FileSystem
  $expanded=Join-Path $stage "expanded"
  New-Item -ItemType Directory -Path $expanded -Force | Out-Null
  [System.IO.Compression.ZipFile]::ExtractToDirectory((Resolve-Path $candidate).Path, $expanded)
  $direct=Join-Path $expanded "audio"
  $nested=Join-Path $expanded "Nightwalker-Voice-Assets/audio"
  if (Test-Path $direct -PathType Container) { $voiceSource=$direct }
  elseif (Test-Path $nested -PathType Container) { $voiceSource=$nested }
  else { throw "VoicePackPath is missing an audio/ directory." }
} else {
  $candidate=Join-Path $root "content/audio"
  if (Test-Path $candidate -PathType Container) { $voiceSource=(Resolve-Path $candidate).Path }
}

if ($null -eq $voiceSource) {
  throw "No voice payload found. Supply -VoiceAssetsDirectory or -VoicePackPath, or add content/audio/."
}

$voiceDestination=Join-Path $package "audio"
New-Item -ItemType Directory -Path $voiceDestination -Force | Out-Null
Copy-Item (Join-Path $voiceSource "*") $voiceDestination -Recurse -Force
$voiceFiles=@(Get-ChildItem $voiceDestination -File | Sort-Object Name)
if ($voiceFiles.Count -ne 25) { throw "Expected 25 unique voice files, found $($voiceFiles.Count)." }
foreach ($voiceFile in $voiceFiles) {
  $extension=$voiceFile.Extension.ToLowerInvariant()
  if ($extension -ne ".wav" -and $extension -ne ".mp3") {
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
