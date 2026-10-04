param([switch]$SmokeTest, [string]$SdkRoot = '', [string]$DataRoot = '', [string]$SdkHome = '')
$ErrorActionPreference = 'Stop'
$taskProject = Split-Path -Parent $PSScriptRoot
$taskSdk = & (Join-Path $PSScriptRoot 'SdkPaths.ps1') -SdkHome $SdkHome -SdkRoot $SdkRoot -DataRoot $DataRoot
$SdkRoot = $taskSdk.Root
$DataRoot = $taskSdk.Data
$taskExe = Join-Path $taskProject 'build\Release\MergenDemo.exe'
foreach ($taskFile in @($taskExe, "$SdkRoot\bin\sdk25-simVis.dll", "$SdkRoot\bin\osg161-osgText.dll", "$DataRoot\terrain\lowResEarth.mbtiles")) {
    if (!(Test-Path -LiteralPath $taskFile)) { throw "Required file missing: $taskFile" }
}
$taskOldPath=$env:PATH
$taskOldData=$env:SIMDIS_SDK_FILE_PATH
$env:PATH = "$SdkRoot\bin;" + $env:PATH
$env:SIMDIS_SDK_FILE_PATH = $DataRoot
Push-Location $taskProject
try {
    if ($SmokeTest) { & $taskExe --smoke-test --capture-dir "$taskProject\artifacts" }
    else { & $taskExe }
    if ($LASTEXITCODE -ne 0) { throw "Demo failed: exit code $LASTEXITCODE" }
} finally {
    Pop-Location
    $env:PATH=$taskOldPath
    $env:SIMDIS_SDK_FILE_PATH=$taskOldData
}
