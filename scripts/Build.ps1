param([string]$SdkRoot = '', [string]$SdkSource = '', [string]$SdkHome = '')
$ErrorActionPreference = 'Stop'
$taskProject = Split-Path -Parent $PSScriptRoot
$taskSdk = & (Join-Path $PSScriptRoot 'SdkPaths.ps1') -SdkHome $SdkHome -SdkRoot $SdkRoot -SdkSource $SdkSource
$SdkRoot = $taskSdk.Root
$SdkSource = $taskSdk.Source
$taskVswhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
$taskVs = & $taskVswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$taskVs) { throw 'Visual Studio C++ tools not found' }
$taskCmake = Join-Path $taskVs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
& $taskCmake -S $taskProject -B "$taskProject\build" -G 'Visual Studio 18 2026' -A x64 "-DSIMDIS_HOME=$($taskSdk.Home)" "-DSIMDIS_SDK_ROOT=$SdkRoot" "-DSIMDIS_SDK_SRC_DIR=$SdkSource" "-DSIMDIS_THIRD_PARTY_DIR=$SdkSource/3rd/win64_vc-14.5"
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed' }
& $taskCmake --build "$taskProject\build" --config Release --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
Write-Host 'Build complete. Start with Start-Demo.cmd.'
