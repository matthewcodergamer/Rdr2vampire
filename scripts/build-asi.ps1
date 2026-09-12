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

# Pinned public mirror used only when dev-c blocks the CI runner (for example HTTP 406).
# The files at this commit identify themselves as Alexander Blade's RDR2 Script Hook SDK.
$mirrorCommit = '14588ad1b05c0a0cafee6c8b7397cdf198d2f412'
$mirrorBase = "https://raw.githubusercontent.com/Halen84/RDR2-Native-Menu-Base/$mirrorCommit"
$mirrorHeaders = @('main.h','natives.h','nativeCaller.h','types.h','enums.h')

function Test-SdkReady {
    return (Test-Path $requiredMain) -and (Test-Path $requiredNatives) -and (Test-Path $requiredLib)
}

function Reset-SdkFolders {
    Remove-Item $incRoot,$libRoot -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force -Path $incRoot,$libRoot | Out-Null
}

function Install-PinnedSdkMirror {
    Write-Warning 'Official dev-c SDK download was unavailable. Falling back to the pinned public SDK mirror.'
    Reset-SdkFolders

    foreach ($name in $mirrorHeaders) {
        $url = "$mirrorBase/inc/$name"
        $out = Join-Path $incRoot $name
        & curl.exe -L --fail --retry 3 --retry-delay 2 $url -o $out
        if ($LASTEXITCODE -ne 0 -or !(Test-Path $out) -or (Get-Item $out).Length -le 0) {
            throw "Pinned SDK mirror failed while downloading $name."
        }
    }

    $libUrl = "$mirrorBase/lib/ScriptHookRDR2.lib"
    & curl.exe -L --fail --retry 3 --retry-delay 2 $libUrl -o $requiredLib
    if ($LASTEXITCODE -ne 0 -or !(Test-Path $requiredLib) -or (Get-Item $requiredLib).Length -lt 1024) {
        throw 'Pinned SDK mirror failed while downloading ScriptHookRDR2.lib.'
    }

    if (!(Test-SdkReady)) {
        throw 'Pinned SDK mirror staging failed: required Script Hook RDR2 SDK files are missing.'
    }
}

function Install-Sdk {
    if ($SkipSdkDownload) {
        throw 'Script Hook RDR2 SDK is missing and -SkipSdkDownload was supplied.'
    }

    $work = Join-Path ([System.IO.Path]::GetTempPath()) ('nightwalker-sdk-' + [guid]::NewGuid().ToString('N'))
    $zip = Join-Path $work 'ScriptHookRDR2_SDK.zip'
    $expanded = Join-Path $work 'expanded'
    New-Item -ItemType Directory -Force -Path $expanded | Out-Null

    $officialInstalled = $false
    try {
        Write-Host "Downloading the official Script Hook RDR2 developer SDK from $SdkUrl"
        & curl.exe -L --fail --retry 2 --retry-delay 2 -A 'Mozilla/5.0' -e 'https://www.dev-c.com/scripthookrdr2/' $SdkUrl -o $zip
        if ($LASTEXITCODE -eq 0 -and (Test-Path $zip) -and (Get-Item $zip).Length -ge 1024) {
            try {
                Expand-Archive -LiteralPath $zip -DestinationPath $expanded -Force
                $main = Get-ChildItem -Path $expanded -Recurse -File -Filter 'main.h' | Select-Object -First 1
                $natives = Get-ChildItem -Path $expanded -Recurse -File -Filter 'natives.h' | Select-Object -First 1
                $lib = Get-ChildItem -Path $expanded -Recurse -File -Filter 'ScriptHookRDR2.lib' | Select-Object -First 1
                if ($main -and $natives -and $lib) {
                    Reset-SdkFolders
                    Copy-Item -Path (Join-Path $main.Directory.FullName '*') -Destination $incRoot -Recurse -Force
                    if ($natives.Directory.FullName -ne $main.Directory.FullName) {
                        Copy-Item -Path (Join-Path $natives.Directory.FullName '*') -Destination $incRoot -Recurse -Force
                    }
                    Copy-Item -LiteralPath $lib.FullName -Destination $requiredLib -Force
                    $officialInstalled = Test-SdkReady
                }
            }
            catch {
                Write-Warning "Official SDK archive could not be staged: $($_.Exception.Message)"
            }
        }
        else {
            Write-Warning "Official SDK download was blocked or unavailable (curl exit $LASTEXITCODE)."
        }
    }
    finally {
        Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue
    }

    if (!$officialInstalled) { Install-PinnedSdkMirror }
}

Push-Location $repo
try {
    if (!(Test-SdkReady)) { Install-Sdk }

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
