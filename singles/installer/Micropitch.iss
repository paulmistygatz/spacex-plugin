; Micropitch - Windows-Installer (Inno Setup 6)
; Gebaut vom Workflow .github/workflows/build-singles-windows.yml:
;   ISCC.exe /DMyVersion=1.0.0 singles\installer\Micropitch.iss
; Erwartet das Plugin unter singles\out\Micropitch.vst3.

#ifndef MyVersion
  #define MyVersion "0.0.0"
#endif

[Setup]
AppId={{6B0D6E1A-3C1F-4A8E-9E55-2B7F1D9C4A31}
AppName=Micropitch
AppVersion={#MyVersion}
AppVerName=Micropitch {#MyVersion}
AppPublisher=Mistycat Studios
AppPublisherURL=https://paulmisty.com
DefaultDirName={commonpf64}\Micropitch
DisableDirPage=yes
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=..\dist
OutputBaseFilename=Micropitch-{#MyVersion}-Windows-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=Micropitch {#MyVersion}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
WelcomeLabel2=This will install [name/ver] on your computer.%n%nThe VST3 plug-in goes to Common Files\VST3.
FinishedLabel=Setup has finished installing [name] on your computer. Restart your DAW or let it rescan its plug-ins.

[InstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Micropitch.vst3"

[Files]
Source: "..\out\Micropitch.vst3\*"; DestDir: "{commoncf64}\VST3\Micropitch.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Micropitch.vst3"
