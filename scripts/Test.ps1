param([string]$SdkRoot = '', [string]$SdkHome = '')
$ErrorActionPreference = 'Stop'
$taskProject=Split-Path -Parent $PSScriptRoot
$taskSdk = & (Join-Path $PSScriptRoot 'SdkPaths.ps1') -SdkHome $SdkHome -SdkRoot $SdkRoot
$SdkRoot = $taskSdk.Root
$taskVs = & 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$taskCtest=Join-Path $taskVs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
$taskOldPath=$env:PATH
$env:PATH="$SdkRoot\bin;"+$env:PATH
try {
    & $taskCtest --test-dir "$taskProject\build" -C Release --output-on-failure --timeout 30
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
} finally { $env:PATH=$taskOldPath }
