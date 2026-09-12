param(
    [ValidateSet('Debug','Release')]
    [string]$Configuration = 'Release',
    [string]$SdkUrl = 'https://www.dev-c.com/files/ScriptHookRDR2_SDK_1.0.1207.73.zip',
    [switch]$SkipSdkDownload
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$sdkRoot = Join-Path $repo 'third_party\ScriptHookRDR2'
$incRoot = Join-Path $sdkRoot 'inc'
$libRoot = Join-Path $sdkRoot 'lib'
$requiredMain = Join-Path $incRoot 'main.h'
$requiredNatives = Join-Path $incRoot 'natives.h'
$requiredLib = Join-Path $libRoot 'ScriptHookRDR2.lib'

function Test-SdkReady {
    return (Test-Path $requiredMain) -and (Test-Path $requiredNatives) -and (Test-Path $requiredLib)
}

function Install-OfficialSdk {
    if ($SkipSdkDownload) {
        throw 'Script Hook RDR2 SDK is missing and -SkipSdkDownload was supplied.'
    }

    $work = Join-Path ([System.IO.Path]::GetTempPath()) ('nightwalker-sdk-' + [guid]::NewGuid().ToString('N'))
    $zip = Join-Path $work 'ScriptHookRDR2_SDK.zip'
    $expanded = Join-Path $work 'expanded'
    New-Item -ItemType Directory -Force -Path $expanded | Out-Null

    try {
        Write-Host "Downloading the official Script Hook RDR2 developer SDK from $SdkUrl"
        & curl.exe -L --fail --retry 3 --retry-delay 2 -A 'Mozilla/5.0' $SdkUrl -o $zip
        if ($LASTEXITCODE -ne 0) { throw "SDK download failed with exit code $LASTEXITCODE." }
        if (!(Test-Path $zip) -or (Get-Item $zip).Length -lt 1024) { throw 'SDK download was empty or unexpectedly small.' }

        Expand-Archive -LiteralPath $zip -DestinationPath $expanded -Force
        $main = Get-ChildItem -Path $expanded -Recurse -File -Filter 'main.h' | Select-Object -First 1
        $natives = Get-ChildItem -Path $expanded -Recurse -File -Filter 'natives.h' | Select-Object -First 1
        $lib = Get-ChildItem -Path $expanded -Recurse -File -Filter 'ScriptHookRDR2.lib' | Select-Object -First 1
        if (!$main -or !$natives -or !$lib) {
            throw 'Official SDK archive did not contain main.h, natives.h and ScriptHookRDR2.lib.'
        }

        Remove-Item $incRoot,$libRoot -Recurse -Force -ErrorAction SilentlyContinue
        New-Item -ItemType Directory -Force -Path $incRoot,$libRoot | Out-Null

        Copy-Item -Path (Join-Path $main.Directory.FullName '*') -Destination $incRoot -Recurse -Force
        if ($natives.Directory.FullName -ne $main.Directory.FullName) {
            Copy-Item -Path (Join-Path $natives.Directory.FullName '*') -Destination $incRoot -Recurse -Force
        }
        Copy-Item -LiteralPath $lib.FullName -Destination $requiredLib -Force
    }
    finally {
        Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue
    }

    if (!(Test-SdkReady)) { throw 'SDK staging failed: expected inc/main.h, inc/natives.h and lib/ScriptHookRDR2.lib.' }
}

Push-Location $repo
try {
    if (!(Test-SdkReady)) { Install-OfficialSdk }

    $msbuild = Get-Command msbuild.exe -ErrorAction SilentlyContinue
    if (!$msbuild) { $msbuild = Get-Command msbuild -ErrorAction SilentlyContinue }
    if (!$msbuild) { throw 'MSBuild was not found. Install Visual Studio 2022 Desktop C++ workload or use GitHub Actions.' }

    Write-Host "Building Nightwalker $Configuration x64..."
    & $msbuild.Source 'Nightwalker.vcxproj' '/m' '/t:Rebuild' "/p:Configuration=$Configuration" '/p:Platform=x64'
    if ($LASTEXITCODE -ne 0) { throw "MSBuild failed with exit code $LASTEXITCODE." }

    $asi = Join-Path $repo "bin\$Configuration\Nightwalker.asi"
    if (!(Test-Path $asi)) { throw "Build completed without producing $asi" }
    if ((Get-Item $asi).Length -le 0) { throw 'Nightwalker.asi was produced but is empty.' }

    $bytes = [System.IO.File]::ReadAllBytes($asi)
    if ($bytes.Length -lt 2 -or $bytes[0] -ne 0x4D -or $bytes[1] -ne 0x5A) {
        throw 'Nightwalker.asi is not a valid Windows PE image.'
    }

    $dist = Join-Path $repo 'dist\Nightwalker'
    Remove-Item $dist -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force -Path $dist | Out-Null
    Copy-Item -LiteralPath $asi -Destination (Join-Path $dist 'Nightwalker.asi') -Force
    Copy-Item -LiteralPath (Join-Path $repo 'config\Nightwalker.example.ini') -Destination (Join-Path $dist 'Nightwalker.ini') -Force
    if (Test-Path (Join-Path $repo 'content\Nightwalker.dialogue')) {
        Copy-Item -LiteralPath (Join-Path $repo 'content\Nightwalker.dialogue') -Destination (Join-Path $dist 'Nightwalker.dialogue') -Force
    }
    Copy-Item -LiteralPath (Join-Path $repo 'README.md') -Destination (Join-Path $dist 'README.md') -Force

    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $asi).Hash
    $size = (Get-Item $asi).Length
    "Nightwalker.asi SHA256=$hash" | Set-Content -Encoding ascii (Join-Path $dist 'SHA256.txt')

    Write-Host '====================================='
    Write-Host ' NIGHTWALKER ASI BUILD SUCCESSFUL'
    Write-Host '====================================='
    Write-Host "File: $asi"
    Write-Host "Size: $size bytes"
    Write-Host "SHA256: $hash"
}
finally {
    Pop-Location
}
