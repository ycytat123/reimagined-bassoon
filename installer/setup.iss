; LTC Reader VST3 Plugin Installer
; VST3 plugin with UPX anti-reverse engineering protection

#define MyAppName "LTC Reader"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Hahahah"
#define MyAppURL ""

[Setup]
AppId={{E8F7A3B2-5C1D-4F8E-9A2B-3D7F1E5C8A9B}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={commoncf}\VST3
DefaultGroupName={#MyAppName}
OutputDir=.
OutputBaseFilename=LTC Reader Setup 1.0.0
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
DisableProgramGroupPage=yes
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; The UPX-packed VST3 plugin
Source: "..\release\LTC Reader.vst3\Contents\x86_64-win\LTC Reader.vst3"; \
    DestDir: "{app}\LTC Reader.vst3\Contents\x86_64-win"; \
    Flags: ignoreversion

[Code]
// Check if VST3 directory exists
function InitializeSetup(): Boolean;
var
  Vst3Dir: String;
begin
  Vst3Dir := ExpandConstant('{commoncf}\VST3');
  if not DirExists(Vst3Dir) then
    CreateDir(Vst3Dir);
  Result := True;
end;
