param([string]$SdkHome = '', [string]$SdkRoot = '', [string]$SdkSource = '', [string]$DataRoot = '')
$ErrorActionPreference = 'Stop'
# All projects can share this installation; never infer it from the project folder.
if (!$SdkHome) { $SdkHome = $env:SIMDIS_SDK_HOME }
if (!$SdkHome) { $SdkHome = Join-Path $env:USERPROFILE 'Desktop\SimdisSDK' }
if (!$SdkRoot) { $SdkRoot = Join-Path $SdkHome 'install' }
if (!$SdkSource) { $SdkSource = Join-Path $SdkHome 'sdk' }
if (!$DataRoot) { $DataRoot = Join-Path $SdkHome 'data\SIMDIS_SDK-Data' }
[pscustomobject]@{
    Home = [System.IO.Path]::GetFullPath($SdkHome)
    Root = [System.IO.Path]::GetFullPath($SdkRoot)
    Source = [System.IO.Path]::GetFullPath($SdkSource)
    Data = [System.IO.Path]::GetFullPath($DataRoot)
}
