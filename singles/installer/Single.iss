; Ein Einzel-Plugin - Windows-Installer (Inno Setup 6)
; Gebaut vom Workflow .github/workflows/build-singles-windows.yml, einmal pro Plugin:
;   ISCC.exe /DMyName=Micropitch /DMyAppId={...} /DMyVersion=1.0.0 singles\installer\Single.iss
; Erwartet das Plugin unter singles\out\<Name>.vst3.

#ifndef MyVersion
  #define MyVersion "0.0.0"
#endif
#ifndef MyName
  #error MyName fehlt
#endif
#ifndef MyAppId
  #error MyAppId fehlt
#endif

[Setup]
AppId={{{#MyAppId}}
AppName={#MyName}
AppVersion={#MyVersion}
AppVerName={#MyName} {#MyVersion}
AppPublisher=Mistycat Studios
AppPublisherURL=https://paulmisty.com
DefaultDirName={commonpf64}\{#MyName}
DisableDirPage=yes
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=..\dist
OutputBaseFilename={#MyName}-{#MyVersion}-Windows-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#MyName} {#MyVersion}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
WelcomeLabel2=This will install [name/ver] on your computer.%n%nThe VST3 plug-in goes to Common Files\VST3.
FinishedLabel=Setup has finished installing [name] on your computer. Restart your DAW or let it rescan its plug-ins.

[InstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\{#MyName}.vst3"

[Files]
Source: "..\out\{#MyName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#MyName}.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\{#MyName}.vst3"
