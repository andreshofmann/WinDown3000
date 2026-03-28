[Setup]
AppName=WinDown 3000
AppVersion=3000.0.4
AppPublisher=WinDown3000
AppPublisherURL=https://github.com/andrewhofmann/WinDown3000
DefaultDirName={autopf}\WinDown 3000
DefaultGroupName=WinDown 3000
OutputBaseFilename=WinDown3000-Setup
SetupIconFile=..\resources\icons\windown3000.ico
UninstallDisplayIcon={app}\WinDown3000.exe
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
LicenseFile=..\LICENSE\MIT

; Allow non-admin install
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "associatemd"; Description: "Associate .md files with WinDown 3000"; GroupDescription: "File Associations:"

[Files]
Source: "..\deploy\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\WinDown 3000"; Filename: "{app}\WinDown3000.exe"
Name: "{group}\Uninstall WinDown 3000"; Filename: "{uninstallexe}"
Name: "{autodesktop}\WinDown 3000"; Filename: "{app}\WinDown3000.exe"; Tasks: desktopicon

[Registry]
Root: HKA; Subkey: "Software\Classes\.md\OpenWithProgids"; ValueType: string; ValueName: "WinDown3000.md"; ValueData: ""; Flags: uninsdeletevalue; Tasks: associatemd
Root: HKA; Subkey: "Software\Classes\WinDown3000.md"; ValueType: string; ValueName: ""; ValueData: "Markdown File"; Flags: uninsdeletekey; Tasks: associatemd
Root: HKA; Subkey: "Software\Classes\WinDown3000.md\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\WinDown3000.exe,0"; Tasks: associatemd
Root: HKA; Subkey: "Software\Classes\WinDown3000.md\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\WinDown3000.exe"" ""%1"""; Tasks: associatemd

[Run]
Filename: "{app}\WinDown3000.exe"; Description: "{cm:LaunchProgram,WinDown 3000}"; Flags: nowait postinstall skipifsilent
