#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
#ifndef PayloadDir
  #error PayloadDir must point to the staged release files
#endif
#ifndef OutputDir
  #error OutputDir must point to the release output directory
#endif

[Setup]
AppId={{58E8ED9F-5FEA-4725-8FAC-9DFFAC0DB303}
AppName=Splatterhouse 3 Co-op
AppVersion={#AppVersion}
AppPublisher=Splatterhouse 3 Co-op contributors
AppPublisherURL=https://github.com/Linn1024/splatterhouse3-coop
AppSupportURL=https://github.com/Linn1024/splatterhouse3-coop/issues
DefaultDirName={localappdata}\Programs\Splatterhouse3Coop
DefaultGroupName=Splatterhouse 3 Co-op
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
WizardStyle=modern
DisableProgramGroupPage=yes
LicenseFile={#PayloadDir}\engine\LICENSE.txt
OutputDir={#OutputDir}
OutputBaseFilename=Splatterhouse3-Coop-{#AppVersion}-windows-x64-setup
Compression=lzma2
SolidCompression=yes
UninstallDisplayIcon={app}\Splatterhouse3-Coop.exe
CloseApplications=yes
RestartApplications=no

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: unchecked

[Files]
Source: "{#PayloadDir}\Splatterhouse3-Coop.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\engine\*"; DestDir: "{app}\engine"; Flags: ignoreversion
Source: "{#PayloadDir}\bizhawk\*"; DestDir: "{app}\bizhawk"; Flags: ignoreversion
Source: "{#PayloadDir}\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\DEVELOPING.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\RESEARCH.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\THIRD_PARTY_NOTICES.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\SOURCE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PayloadDir}\source.zip"; DestDir: "{app}"; Flags: ignoreversion
Source: "{code:GetRomFile}"; DestDir: "{app}"; DestName: "Splatterhouse 3 (USA).md"; Flags: external ignoreversion uninsneveruninstall; Check: HasRomFile

[Icons]
Name: "{group}\Splatterhouse 3 Co-op"; Filename: "{app}\Splatterhouse3-Coop.exe"; WorkingDir: "{app}"
Name: "{group}\Game folder"; Filename: "{app}"
Name: "{group}\Help and controls"; Filename: "https://github.com/Linn1024/splatterhouse3-coop#readme"
Name: "{autodesktop}\Splatterhouse 3 Co-op"; Filename: "{app}\Splatterhouse3-Coop.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\Splatterhouse3-Coop.exe"; Description: "Play Splatterhouse 3 Co-op"; Flags: nowait postinstall skipifsilent unchecked; Check: InstalledRomReady

[Code]
var
  RomPage: TInputFileWizardPage;

function RomIsValid(const FileName: String): Boolean;
begin
  Result := False;
  if not FileExists(FileName) then Exit;
  try
    Result := CompareText(GetSHA256OfFile(FileName),
      '8c7737912cf948a606a683e32f4b6a0a4303215cdc99b39fbc5976c196c45710') = 0;
  except
    Result := False;
  end;
end;

procedure InitializeWizard;
begin
  RomPage := CreateInputFilePage(wpSelectDir, 'Select your game ROM',
    'A game ROM is required to play. It is not included.',
    'Choose your unmodified Splatterhouse 3 (USA) ROM. Setup checks the file and copies it into the game folder. Leave this blank to add it later.');
  RomPage.Add('Game ROM:', 'ROM files (*.md;*.bin;*.gen)|*.md;*.bin;*.gen|All files|*.*', '.md');
  RomPage.Values[0] := ExpandConstant('{param:ROMFILE|}');
end;

function GetRomFile(Param: String): String;
begin
  Result := RomPage.Values[0];
end;

function HasRomFile: Boolean;
begin
  Result := (RomPage.Values[0] <> '') and
    (CompareText(ExpandFileName(RomPage.Values[0]),
      ExpandConstant('{app}\Splatterhouse 3 (USA).md')) <> 0);
end;

function ValidateSelectedRom: String;
begin
  Result := '';
  if (RomPage.Values[0] <> '') and not RomIsValid(RomPage.Values[0]) then
    Result := 'This file is not the supported unmodified Splatterhouse 3 (USA) ROM. Select the correct extracted ROM, or clear the field to add it later.';
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
  ErrorMessage: String;
begin
  Result := True;
  if CurPageID = RomPage.ID then begin
    ErrorMessage := ValidateSelectedRom;
    Result := ErrorMessage = '';
    if not Result then MsgBox(ErrorMessage, mbError, MB_OK);
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := ValidateSelectedRom;
end;

function InstalledRomReady: Boolean;
begin
  Result := RomIsValid(ExpandConstant('{app}\Splatterhouse 3 (USA).md'));
end;
