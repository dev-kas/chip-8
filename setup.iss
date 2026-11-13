[Setup]
AppId={{dd2e1a04-4993-4c78-9eb8-59a2d71b9238}}
AppName=KAS CHIP-8
AppVersion=1.0.0
AppPublisher=KAS Software
DefaultDirName={autopf}\KAS Software\chip8
UninstallFilesDir={app}
PrivilegesRequired=admin
OutputBaseFilename=chip8-windows-x86_64-setup
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64

[Files]
Source: "chip8.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "SDL3.dll"; DestDir: "{app}"; Flags: ignoreversion

[Registry]
; .ch8 file association
Root: HKCR; Subkey: ".ch8"; ValueType: string; ValueName: ""; ValueData: "KAS.CHIP8"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "KAS.CHIP8"; ValueType: string; ValueName: ""; ValueData: "CHIP-8 ROM"; Flags: uninsdeletekey
Root: HKCR; Subkey: "KAS.CHIP8\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\chip8.exe,0"
Root: HKCR; Subkey: "KAS.CHIP8\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\chip8.exe"" ""%1"""

[Tasks]
Name: "addtopath"; Description: "Add CHIP-8 to PATH"; GroupDescription: "Additional options:"; Flags: unchecked

[Registry]
; PATH (optional, machine-wide)
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}"; Tasks: addtopath; Flags: preservestringtype

[Code]
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  Path: string;
begin
  if CurUninstallStep = usUninstall then
  begin
    if RegQueryStringValue(
      HKLM,
      'SYSTEM\CurrentControlSet\Control\Session Manager\Environment',
      'Path',
      Path
    ) then
    begin
      StringChangeEx(Path, ExpandConstant('{app}'), '', True);
      RegWriteExpandStringValue(
        HKLM,
        'SYSTEM\CurrentControlSet\Control\Session Manager\Environment',
        'Path',
        Path
      );
    end;
  end;
end;
