#ifndef AppVersion
  #error AppVersion is required
#endif
#ifndef StageDir
  #error StageDir is required
#endif
#ifndef OutputDir
  #error OutputDir is required
#endif
#if !FileExists(StageDir + "\RaSampler.vst3\Contents\x86_64-win\RaSampler.vst3")
  #error Missing Windows x64 VST3 module
#endif
#if !FileExists(StageDir + "\RaSampler.exe")
  #error Missing Windows x64 standalone application
#endif

[Setup]
; Stable installer identity: retain this GUID for future in-place upgrades.
AppId={{CED1643B-8D20-43C2-97D0-01746B7C50C0}
AppName=RaSampler
AppVersion={#AppVersion}
AppVerName=RaSampler {#AppVersion}
AppPublisher=Independent Audio
AppPublisherURL=https://github.com/voyagersquare/RaSampler
AppSupportURL=https://github.com/voyagersquare/RaSampler/issues
VersionInfoVersion={#AppVersion}.0
VersionInfoCompany=Independent Audio
VersionInfoDescription=RaSampler VST3 Instrument Installer
DefaultDirName={autopf}\Independent Audio\RaSampler
DefaultGroupName=Independent Audio\RaSampler
DisableProgramGroupPage=yes
DisableWelcomePage=no
UsePreviousAppDir=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
PrivilegesRequired=admin
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
OutputDir={#OutputDir}
OutputBaseFilename=RaSampler-{#AppVersion}-Windows-x64-Setup
InfoBeforeFile=Install-Info.txt
SetupLogging=yes
CloseApplications=yes
CloseApplicationsFilter=*.exe,*.dll,*.vst3
RestartApplications=no
UninstallDisplayName=RaSampler
UninstallFilesDir={app}\Uninstall
Uninstallable=yes
ChangesAssociations=no
ChangesEnvironment=no

[Languages]
Name: english; MessagesFile: compiler:Default.isl

[Types]
Name: full; Description: "VST3 and standalone application"
Name: compact; Description: "VST3 instrument only"
Name: custom; Description: "Custom installation"; Flags: iscustom

[Components]
Name: vst3; Description: "RaSampler VST3 instrument (64-bit)"; Types: full compact custom; Flags: fixed
Name: standalone; Description: "RaSampler standalone application"; Types: full

[Tasks]
Name: desktopicon; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"; Components: standalone; Flags: unchecked

[Files]
Source: "{#StageDir}\RaSampler.vst3\*"; DestDir: "{commoncf64}\VST3\RaSampler.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#StageDir}\RaSampler.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "{#StageDir}\README.md"; DestDir: "{app}\Documentation"; Flags: ignoreversion
Source: "Install-Info.txt"; DestDir: "{app}\Documentation"; Flags: ignoreversion
Source: "{#StageDir}\THIRD_PARTY.md"; DestDir: "{app}\Documentation"; Flags: ignoreversion
Source: "{#StageDir}\Licenses\*"; DestDir: "{app}\Licenses"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\RaSampler"; Filename: "{app}\RaSampler.exe"; Components: standalone
Name: "{group}\Uninstall RaSampler"; Filename: "{uninstallexe}"
Name: "{autodesktop}\RaSampler"; Filename: "{app}\RaSampler.exe"; Components: standalone; Tasks: desktopicon

[Run]
Filename: "{app}\RaSampler.exe"; Description: "Launch RaSampler"; Components: standalone; Flags: nowait postinstall skipifsilent unchecked

; Only files installed by this package are removed. User presets, samples and
; DAW projects are deliberately outside the installer/uninstaller file lists.
