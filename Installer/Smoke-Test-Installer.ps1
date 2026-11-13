[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Installer,
    [Parameter(Mandatory=$true)][string]$PluginvalPath,
    [Parameter(Mandatory=$true)][string]$ReportPath,
    [string]$BuildDirectory
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($env:GITHUB_ACTIONS -ne 'true' -or $env:OS -ne 'Windows_NT') {
    throw 'Run installation tests only on a disposable GitHub Actions Windows runner.'
}
$root = Split-Path -Parent $PSScriptRoot
if (-not $BuildDirectory) { $BuildDirectory = Join-Path $root 'build' }
$source = [IO.Path]::GetFullPath((Join-Path $BuildDirectory 'RaSampler_artefacts\Release\VST3\RaSampler.vst3'))
$sourceApp = Join-Path $BuildDirectory 'RaSampler_artefacts\Release\Standalone\RaSampler.exe'
$release = Get-Content (Join-Path (Split-Path -Parent $Installer) 'release-info.json') -Raw | ConvertFrom-Json
$version = $release.version
$bundle = Join-Path $env:CommonProgramW6432 'VST3\RaSampler.vst3'
$app = Join-Path $env:ProgramW6432 'Independent Audio\RaSampler'
$registry = 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{CED1643B-8D20-43C2-97D0-01746B7C50C0}_is1'
$menu = Join-Path ([Environment]::GetFolderPath('CommonPrograms')) 'Independent Audio\RaSampler'
$desktop = Join-Path ([Environment]::GetFolderPath('CommonDesktopDirectory')) 'RaSampler.lnk'
foreach ($path in @($bundle,$app,$registry,$menu,$desktop)) {
    if (Test-Path -LiteralPath $path) { throw "Previous installation exists; refusing to modify $path" }
}
if ((Get-FileHash -LiteralPath $Installer -Algorithm SHA256).Hash.ToLowerInvariant() -ne $release.sha256) {
    throw 'Installer checksum does not match release metadata.'
}
$presetFolder = Join-Path $env:APPDATA 'Independent Audio\RaSampler\Presets'
New-Item -ItemType Directory -Path $presetFolder -Force | Out-Null
$preset = Join-Path $presetFolder ('installer-preservation-' + [guid]::NewGuid().ToString() + '.rasampler')
[IO.File]::WriteAllText($preset, 'User preset preservation fixture. Never installed or removed by Setup.')
$presetHash = (Get-FileHash -LiteralPath $preset -Algorithm SHA256).Hash
function Assert-PresetPreserved {
    if (-not (Test-Path -LiteralPath $preset -PathType Leaf) -or
        (Get-FileHash -LiteralPath $preset -Algorithm SHA256).Hash -ne $presetHash) {
        throw 'The installer or uninstaller modified a user preset.'
    }
}
function Invoke-Setup([string]$Exe, [string]$Phase, [string[]]$Extra = @()) {
    $log = Join-Path $env:RUNNER_TEMP "RaSampler-install-$Phase.log"
    $arguments = @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-',"/LOG=`"$log`"") + $Extra
    $process = Start-Process -FilePath $Exe -ArgumentList $arguments -Wait -PassThru
    if ($process.ExitCode -ne 0) { throw "Setup $Phase failed: exit $($process.ExitCode). See $log" }
}
function Assert-Installed([bool]$Standalone, [bool]$DesktopShortcut = $false) {
    Get-ChildItem -LiteralPath $source -File -Recurse | ForEach-Object {
        $relative = $_.FullName.Substring($source.Length + 1)
        $installed = Join-Path $bundle $relative
        if (-not (Test-Path -LiteralPath $installed -PathType Leaf) -or
            (Get-FileHash -LiteralPath $_.FullName).Hash -ne (Get-FileHash -LiteralPath $installed).Hash) {
            throw "Installed VST3 differs from the tested build: $relative"
        }
    }
    $entry = Get-ItemProperty -LiteralPath $registry
    if ($entry.DisplayName -ne 'RaSampler' -or $entry.DisplayVersion -ne $version -or $entry.Publisher -ne 'Independent Audio') {
        throw 'Incorrect Windows Installed apps registration.'
    }
    foreach ($relative in @('Documentation\Install-Info.txt','Documentation\THIRD_PARTY.md','Licenses\JUCE-LICENSE.md','Licenses\VST3-SDK-LICENSE.txt','Uninstall\unins000.exe')) {
        if (-not (Test-Path -LiteralPath (Join-Path $app $relative) -PathType Leaf)) { throw "Missing $relative" }
    }
    if ($Standalone) {
        if (-not (Test-Path -LiteralPath (Join-Path $app 'RaSampler.exe') -PathType Leaf) -or
            (Get-FileHash -LiteralPath $sourceApp).Hash -ne (Get-FileHash -LiteralPath (Join-Path $app 'RaSampler.exe')).Hash) {
            throw 'The standalone application is missing or differs from the tested build.'
        }
        if (-not (Test-Path -LiteralPath (Join-Path $menu 'RaSampler.lnk'))) { throw 'Start menu application shortcut is missing.' }
    } else {
        if ((Test-Path -LiteralPath (Join-Path $app 'RaSampler.exe')) -or
            (Test-Path -LiteralPath (Join-Path $menu 'RaSampler.lnk'))) { throw 'VST3-only mode installed a standalone application.' }
    }
    if ((Test-Path -LiteralPath $desktop) -ne $DesktopShortcut) { throw 'Desktop shortcut selection was not respected.' }
    Assert-PresetPreserved
}
function Uninstall-And-Check([string]$Phase) {
    Invoke-Setup (Join-Path $app 'Uninstall\unins000.exe') $Phase
    for ($attempt = 0; $attempt -lt 100 -and (Test-Path -LiteralPath $app); ++$attempt) { Start-Sleep -Milliseconds 100 }
    foreach ($path in @($bundle,$app,$registry,$menu,$desktop)) {
        if (Test-Path -LiteralPath $path) { throw "Uninstall left behind $path" }
    }
    Assert-PresetPreserved
}
try {
    Invoke-Setup $Installer 'fresh' @('/TYPE=full','/TASKS=desktopicon')
    Assert-Installed $true $true
    # Validate the bundle at its actual installed DAW scan location.
    $validation = Start-Process -FilePath $PluginvalPath -ArgumentList @('--headless','--strictness-level','5','--timeout-ms','120000','--validate',"`"$bundle`"") -Wait -PassThru -NoNewWindow
    if ($validation.ExitCode -ne 0) { throw "Installed VST3 failed pluginval: $($validation.ExitCode)" }
    $running = Start-Process -FilePath (Join-Path $app 'RaSampler.exe') -PassThru
    try {
        Start-Sleep -Seconds 2
        $running.Refresh()
        if ($running.HasExited) { throw 'The installed standalone application exited during startup.' }
    } finally {
        if (-not $running.HasExited) {
            Stop-Process -Id $running.Id -Force
            $running.WaitForExit()
        }
    }
    # Model an older registered build needing repair without changing user data.
    Set-ItemProperty -LiteralPath $registry -Name DisplayVersion -Value '0.5.0'
    [IO.File]::WriteAllText((Join-Path $bundle 'Contents\x86_64-win\RaSampler.vst3'), 'Previous RaSampler build fixture')
    Invoke-Setup $Installer 'upgrade-repair' @('/TYPE=full','/TASKS=desktopicon')
    Assert-Installed $true $true
    Uninstall-And-Check 'full-uninstall'

    Invoke-Setup $Installer 'vst3-only' @('/TYPE=compact','/TASKS=')
    Assert-Installed $false
    Uninstall-And-Check 'vst3-only-uninstall'

    # The standard installer replaces a previously copied VST3 in the same folder.
    New-Item -ItemType Directory -Path (Join-Path $bundle 'Contents\x86_64-win') -Force | Out-Null
    [IO.File]::WriteAllText((Join-Path $bundle 'Contents\x86_64-win\RaSampler.vst3'), 'Previous portable VST3 fixture')
    Invoke-Setup $Installer 'portable-upgrade' @('/TYPE=compact','/TASKS=')
    Assert-Installed $false
    Uninstall-And-Check 'portable-upgrade-uninstall'

    [ordered]@{
        version = $version; platform = 'Windows x64'; publisher = 'Independent Audio'
        checks = [ordered]@{
            installerChecksum = $true; freshInstall = $true; installedBundleChecksums = $true
            installedAppsRegistration = $true; installedVst3Pluginval = $true; standaloneStartup = $true
            startMenuAndDesktopShortcuts = $true; upgradeAndRepair = $true; vst3OnlyInstall = $true
            portableUpgrade = $true; uninstall = $true; userPresetsPreserved = $true
        }
    } | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $ReportPath -Encoding UTF8
    Write-Host 'PASS: installer fresh install, installed VST3 validation, standalone startup, upgrade/repair, VST3-only mode, portable upgrade, uninstall and user preset preservation'
} finally {
    if (Test-Path -LiteralPath $preset) { Remove-Item -LiteralPath $preset -Force }
}
