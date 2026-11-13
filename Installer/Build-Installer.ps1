[CmdletBinding()]
param(
    [switch]$SkipBuild,
    [string]$BuildDirectory,
    [string]$OutputDirectory,
    [string]$JuceSourceDirectory,
    [string]$IsccPath
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($env:OS -ne 'Windows_NT') { throw 'Build the Windows installer on Windows.' }
$root = Split-Path -Parent $PSScriptRoot
if (-not $BuildDirectory) { $BuildDirectory = Join-Path $root 'build' }
$build = [IO.Path]::GetFullPath($BuildDirectory)
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $build 'installer' }
$output = [IO.Path]::GetFullPath($OutputDirectory)

if (-not $IsccPath) {
    $compiler = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($compiler) { $IsccPath = $compiler.Source }
    else {
        foreach ($base in @(${env:ProgramFiles(x86)}, $env:ProgramFiles)) {
            if ($base) {
                $candidate = Join-Path $base 'Inno Setup 6\ISCC.exe'
                if (Test-Path -LiteralPath $candidate -PathType Leaf) { $IsccPath = $candidate; break }
            }
        }
    }
}
if (-not $IsccPath -or -not (Test-Path -LiteralPath $IsccPath -PathType Leaf)) {
    throw 'Install Inno Setup 6.7.4 from https://jrsoftware.org/isdl.php or provide -IsccPath.'
}
if (-not $SkipBuild) {
    & cmake -S $root -B $build -A x64
    if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
    & cmake --build $build --config Release --parallel 2
    if ($LASTEXITCODE -ne 0) { throw 'Windows build failed.' }
}
$match = [regex]::Match((Get-Content (Join-Path $root 'CMakeLists.txt') -Raw), 'project\(RaSampler VERSION (\d+\.\d+\.\d+) LANGUAGES')
if (-not $match.Success) { throw 'Cannot read the RaSampler version.' }
$version = $match.Groups[1].Value
$bundle = Join-Path $build 'RaSampler_artefacts\Release\VST3\RaSampler.vst3'
$module = Join-Path $bundle 'Contents\x86_64-win\RaSampler.vst3'
$standalone = Join-Path $build 'RaSampler_artefacts\Release\Standalone\RaSampler.exe'

function Assert-WindowsX64([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Missing build file: $Path" }
    $reader = [IO.BinaryReader]::new([IO.File]::OpenRead($Path))
    try {
        if ($reader.ReadUInt16() -ne 0x5a4d) { throw "Not a Windows executable: $Path" }
        $reader.BaseStream.Position = 0x3c
        $offset = $reader.ReadUInt32()
        $reader.BaseStream.Position = $offset
        if ($reader.ReadUInt32() -ne 0x4550 -or $reader.ReadUInt16() -ne 0x8664) {
            throw "Not a Windows x64 PE file: $Path"
        }
    } finally { $reader.Dispose() }
}
Assert-WindowsX64 $module
Assert-WindowsX64 $standalone
$metadata = Get-Content (Join-Path $bundle 'Contents\Resources\moduleinfo.json') -Raw
# The SDK emits JSON with trailing commas, so check the relevant string fields
# without relying on the different PS5/PS7 JSON parser behaviours.
foreach ($pattern in @('"Name"\s*:\s*"RaSampler"', '"Vendor"\s*:\s*"Independent Audio"',
                       ('"Version"\s*:\s*"' + [regex]::Escape($version) + '"'),
                       '"CID"\s*:\s*"ABCDEF019182FAEB567973715261736D"')) {
    if ($metadata -notmatch $pattern) { throw "Incorrect VST3 identity or metadata: $pattern" }
}
foreach ($file in @($module, $standalone)) {
    $info = [Diagnostics.FileVersionInfo]::GetVersionInfo($file)
    if ($info.CompanyName -ne 'Independent Audio' -or $info.ProductVersion -ne $version) {
        throw "Incorrect Windows publisher or version in $file"
    }
}
if (-not $JuceSourceDirectory) { $JuceSourceDirectory = Join-Path $build '_deps\juce-src' }
$juce = [IO.Path]::GetFullPath($JuceSourceDirectory)
$licences = @{
    'JUCE-LICENSE.md' = Join-Path $juce 'LICENSE.md'
    'VST3-SDK-LICENSE.txt' = Join-Path $juce 'modules\juce_audio_processors\format_types\VST3_SDK\LICENSE.txt'
    'VST3-Pluginterfaces-LICENSE.txt' = Join-Path $juce 'modules\juce_audio_processors\format_types\VST3_SDK\pluginterfaces\LICENSE.txt'
    'VST3-Base-LICENSE.txt' = Join-Path $juce 'modules\juce_audio_processors\format_types\VST3_SDK\base\LICENSE.txt'
    'VST3-Public-SDK-LICENSE.txt' = Join-Path $juce 'modules\juce_audio_processors\format_types\VST3_SDK\public.sdk\LICENSE.txt'
}
foreach ($path in $licences.Values) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing third-party licence: $path" }
}
$stage = Join-Path $build 'installer-stage'
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Path (Join-Path $stage 'Licenses') -Force | Out-Null
Copy-Item -LiteralPath $bundle -Destination $stage -Recurse
Copy-Item -LiteralPath $standalone -Destination $stage
foreach ($name in @('README.md','THIRD_PARTY.md')) { Copy-Item -LiteralPath (Join-Path $root $name) -Destination $stage }
foreach ($name in $licences.Keys) { Copy-Item -LiteralPath $licences[$name] -Destination (Join-Path $stage "Licenses\$name") }
New-Item -ItemType Directory -Path $output -Force | Out-Null
$installer = Join-Path $output "RaSampler-$version-Windows-x64-Setup.exe"
if (Test-Path -LiteralPath $installer) { Remove-Item -LiteralPath $installer -Force }
& $IsccPath "/DAppVersion=$version" "/DStageDir=$stage" "/DOutputDir=$output" (Join-Path $PSScriptRoot 'RaSampler.iss')
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $installer -PathType Leaf)) { throw 'Inno Setup compilation failed.' }
Assert-WindowsX64 $module
# Inno Setup's bootstrapper is a Windows executable; its payload is x64.
$hash = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText("$installer.sha256", "$hash  $([IO.Path]::GetFileName($installer))`r`n", [Text.Encoding]::ASCII)
$commit = & git -C $root rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Cannot read the source commit.' }
[ordered]@{
    product = 'RaSampler'; version = $version; publisher = 'Independent Audio'
    platform = 'Windows x64'; sourceCommit = $commit; installer = [IO.Path]::GetFileName($installer)
    sha256 = $hash; vst3ClassId = 'ABCDEF019182FAEB567973715261736D'
    vst3Path = 'C:\Program Files\Common Files\VST3\RaSampler.vst3'
    standaloneOptional = $true
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'release-info.json') -Encoding UTF8
Write-Host "WINDOWS INSTALLER READY: $installer"
