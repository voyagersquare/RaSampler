$ErrorActionPreference = 'Stop'
$source = Join-Path $PSScriptRoot 'RaSampler.vst3'
if (!(Test-Path $source)) { throw 'RaSampler.vst3 must be next to this script. Extract the entire ZIP first.' }
$target = Join-Path $env:LOCALAPPDATA 'Programs\Common\VST3'
New-Item -ItemType Directory -Force $target | Out-Null
Copy-Item $source $target -Recurse -Force
Write-Host "Installed to $target. Add this folder to the DAW scan paths if needed, then rescan VST3 instruments."
