param(
  [Parameter(Mandatory=$true)][string]$PluginPath,
  [string]$Version="1.0.0-rc1",
  [string]$RepositoryRoot=(Split-Path -Parent $PSScriptRoot),
  [string]$OutputDirectory=""
)
$ErrorActionPreference="Stop"
Set-StrictMode -Version Latest

$root=(Resolve-Path $RepositoryRoot).Path
$plugin=(Resolve-Path $PluginPath).Path
$pluginInfo=Get-Item $plugin
if ($pluginInfo.Length -lt 4096) { throw "Nightwalker.asi is too small for release packaging." }
$bytes=[System.IO.File]::ReadAllBytes($plugin)
if ($bytes[0] -ne 0x4D -or $bytes[1] -ne 0x5A) { throw "Nightwalker.asi is not a Windows PE image." }
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
  $OutputDirectory=Join-Path $root "artifacts"
} elseif (-not [System.IO.Path]::IsPathRooted($OutputDirectory)) {
  $OutputDirectory=Join-Path (Get-Location).Path $OutputDirectory
}
$OutputDirectory=[System.IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null

$stage=Join-Path $OutputDirectory "Nightwalker-$Version-stage"
$package=Join-Path $stage "Nightwalker"
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Path $package -Force | Out-Null

$files=@{
  "config/Nightwalker.example.ini"="Nightwalker.ini";
  "content/Nightwalker.dialogue"="Nightwalker.dialogue";
  "README.md"="README.md";
  "CHANGELOG.md"="CHANGELOG.md";
  "THIRD_PARTY_NOTICES.md"="THIRD_PARTY_NOTICES.md"
}
Copy-Item $plugin (Join-Path $package "Nightwalker.asi")
foreach ($source in $files.Keys) {
  $full=Join-Path $root $source
  if (-not (Test-Path $full -PathType Leaf)) { throw "Missing release input: $source" }
  Copy-Item $full (Join-Path $package $files[$source])
}

$expected=@("CHANGELOG.md","Nightwalker.asi","Nightwalker.dialogue","Nightwalker.ini","README.md","THIRD_PARTY_NOTICES.md") | Sort-Object
$actual=Get-ChildItem $package -File | ForEach-Object Name | Sort-Object
if (($expected -join "|") -ne ($actual -join "|")) { throw "Release package allowlist mismatch." }

$badExtensions=@(".pdb",".obj",".lib",".dll",".log",".exe",".ilk",".exp",".iobj",".ipdb")
foreach ($file in Get-ChildItem $package -File) {
  if ($badExtensions -contains $file.Extension.ToLowerInvariant()) { throw "Build junk in package: $($file.Name)" }
}
foreach ($file in Get-ChildItem $package -File | Where-Object Extension -ne ".asi") {
  $text=Get-Content $file.FullName -Raw
  if ($text -match 'third_party[\\/]ScriptHookRDR2|ScriptHookRdr2Root|[A-Za-z]:\\') { throw "Developer path leaked into $($file.Name)" }
}

$zip=Join-Path $OutputDirectory "Nightwalker-$Version-win64.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Push-Location $stage
try { Compress-Archive -Path "Nightwalker" -DestinationPath $zip -CompressionLevel Optimal }
finally { Pop-Location }
Remove-Item $stage -Recurse -Force
Write-Host "ARTIFACT_NAME=Nightwalker-$Version-win64.zip"
