; SpaceX - Windows-Installer (Inno Setup 6)
;
; Wird vom GitHub-Workflow .github/workflows/build-windows.yml gebaut:
;   ISCC.exe /DMyVersion=1.0.1 installer\windows\SpaceX.iss
; Erwartet das fertige Plugin unter out\SpaceX.vst3 (Schritt "Collect").
;
; Installiert:
;   C:\Program Files\Common Files\VST3\SpaceX.vst3      (Standardordner fuer VST3)
;   C:\Program Files\SpaceX\SpaceX Manual (EN).pdf      (dort sucht openManual() zuerst)
; Presets braucht der Installer nicht: sie stecken im Plugin und werden beim
; ersten Start nach Dokumente\SpaceX\Presets geschrieben.

#ifndef MyVersion
  #define MyVersion "0.0.0"
#endif

[Setup]
AppId={{E000C524-77AE-401F-A595-FCEB2243129C}
AppName=SpaceX
AppVersion={#MyVersion}
AppVerName=SpaceX {#MyVersion}
AppPublisher=Mistycat Studios
AppPublisherURL=https://paulmisty.com
DefaultDirName={commonpf64}\SpaceX
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=no
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=..\..\dist
OutputBaseFilename=SpaceX-{#MyVersion}-Windows-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=SpaceX {#MyVersion}

[InstallDelete]
; alte Fassung komplett weg, sonst bleiben geloeschte Dateien im Bundle liegen
Type: filesandordirs; Name: "{commoncf64}\VST3\SpaceX.vst3"

[Files]
Source: "..\..\out\SpaceX.vst3\*"; DestDir: "{commoncf64}\VST3\SpaceX.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\docs\SpaceXManual_EN.pdf"; DestDir: "{app}"; DestName: "SpaceX Manual (EN).pdf"; Flags: ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\SpaceX.vst3"
