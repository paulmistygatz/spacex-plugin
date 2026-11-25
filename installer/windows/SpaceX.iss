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
AppName=Orbit
AppVersion={#MyVersion}
AppVerName=Orbit {#MyVersion}
AppPublisher=Mistycat Studios
AppPublisherURL=https://paulmisty.com
DefaultDirName={commonpf64}\Orbit
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=no
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=..\..\dist
OutputBaseFilename=Orbit-{#MyVersion}-Windows-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=Orbit {#MyVersion}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
WelcomeLabel2=This will install [name/ver] on your computer.%n%nThe VST3 plug-in goes to Common Files\VST3, the manual to Program Files\Orbit. The single modules Micropitch, LCR, Polarity, Mid-Side, Autopan and Phaser are installed as their own plug-ins too. Presets are created by the plug-in on first launch.%n%nWithout a serial, Orbit runs as a demo: everything works, but about every 50 seconds the sound goes quiet for a moment. To activate, click the logo in the plug-in and enter your name and serial.
FinishedLabel=Setup has finished installing [name] on your computer. Restart your DAW or let it rescan its plug-ins.

[InstallDelete]
; alte Fassung komplett weg, sonst bleiben geloeschte Dateien im Bundle liegen.
; Runde 190: SpaceX heisst jetzt Orbit (gleiche AppId, gleiche Plugin-ID) -
; das alte SpaceX.vst3 und der alte Handbuch-Ordner gehen mit weg.
Type: filesandordirs; Name: "{commoncf64}\VST3\SpaceX.vst3"
Type: filesandordirs; Name: "{commonpf64}\SpaceX"
Type: filesandordirs; Name: "{commoncf64}\VST3\Orbit.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\Micropitch.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\LCR.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\Polarity.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\Mid-Side.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\Autopan.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\Phaser.vst3"

[Files]
Source: "..\..\out\Orbit.vst3\*"; DestDir: "{commoncf64}\VST3\Orbit.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
; 20.11.: die sechs Einzel-Plugins (Micropitch, LCR, Polarity, Mid-Side, Autopan, Phaser)
Source: "..\..\out\singles\*"; DestDir: "{commoncf64}\VST3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\docs\SpaceXManual_EN.pdf"; DestDir: "{app}"; DestName: "Orbit Manual (EN).pdf"; Flags: ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Orbit.vst3"
