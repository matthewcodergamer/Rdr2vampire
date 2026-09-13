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

$sourcePack=Join-Path $root "content/Nightwalker.voicepack"
$manifest=Join-Path $root "content/Nightwalker.audio"
$dialogue=Join-Path $root "Nightwalker.voice.dialogue"
foreach ($required in @($sourcePack,$manifest,$dialogue)) {
  if (-not (Test-Path $required -PathType Leaf)) { throw "Missing voice-pack input: $required" }
}

$stage=Join-Path $OutputDirectory "Nightwalker-Voice-Assets-stage"
$package=Join-Path $stage "Nightwalker-Voice-Assets"
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Path $package -Force | Out-Null

Copy-Item $manifest (Join-Path $package "Nightwalker.audio")
Copy-Item $dialogue (Join-Path $package "Nightwalker.voice.dialogue")

Add-Type -AssemblyName System.IO.Compression.FileSystem
$expanded=Join-Path $stage "expanded"
New-Item -ItemType Directory -Path $expanded -Force | Out-Null
[System.IO.Compression.ZipFile]::ExtractToDirectory($sourcePack, $expanded)
$voiceSource=Join-Path $expanded "audio"
if (-not (Test-Path $voiceSource -PathType Container)) { throw "Nightwalker.voicepack is missing audio/." }
$voiceDestination=Join-Path $package "audio"
New-Item -ItemType Directory -Path $voiceDestination -Force | Out-Null
Copy-Item (Join-Path $voiceSource "*") $voiceDestination -Recurse -Force

$voiceFiles=Get-ChildItem $voiceDestination -File | Sort-Object Name
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
