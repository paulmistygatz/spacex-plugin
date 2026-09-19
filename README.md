# LCR / MS Matrix — VST3 Plugin

Ein Stereo-Werkzeug-Plugin, das zwei Betriebsmodi kombiniert:

- **Mid/Side-Modus** (Standard, **zero latency**): Side Gain, Side Width,
  Polarity Flip (L/R), Delay L/R (ms), Goniometer.
- **LCR-Modus** (STFT-basierte Phantom-Center-Extraktion, **mit Latenz**):
  Left/Center/Right werden getrennt und einzeln in der Lautstärke geregelt.

## Funktionsumfang (wie besprochen)

| Feature | Status |
|---|---|
| LCR Extraktion (Center/Left/Right) | ✅ STFT-basiert, Sensitivity-Regler |
| Left/Right Phase Polarity Flip | ✅ |
| Side Gain | ✅ (dB) |
| Side Width | ✅ (%) |
| Left/Right Delay in ms | ✅ (0–20 ms, fraktional interpoliert) |
| Goniometer | ✅ eigenes Vektorskop mit Nachleuchten |
| Zero Latency im Mid/Side-Modus | ✅ nur im LCR-Modus wird Latenz an den Host gemeldet |
| GUI | ✅ dunkles, schlankes, modernes Layout |

## Warum ist das noch nicht kompiliert?

Ich habe in dieser Umgebung **keinen Internetzugang und keine C++/JUCE-Toolchain**,
kann das Plugin also nicht selbst bauen oder testen. Der Code ist vollständig
und sollte kompilieren, ist aber **ungetestet** — insbesondere die STFT-basierte
LCR-Extraktion (Overlap-Add-Logik) solltest du beim ersten Bauen genau abhören
und ggf. mit mir zusammen nachjustieren (Sensitivity-Kurve, Fenstergrößen etc.).

## Bauen (Windows/macOS/Linux)

Voraussetzungen: CMake ≥ 3.22, ein C++20-fähiger Compiler, Internetzugang
(JUCE wird beim ersten Konfigurieren automatisch via `FetchContent` geladen).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Das fertige VST3 landet danach unter `build/LCRMSPlugin_artefacts/Release/VST3/`.

Kopiere es in deinen VST3-Ordner:
- **Windows:** `C:\Program Files\Common Files\VST3\`
- **macOS:** `~/Library/Audio/Plug-Ins/VST3/`
- **Linux:** `~/.vst3/`

## Projektstruktur

```
Source/
  PluginProcessor.h/.cpp   Parameter, Signalkette, State-Handling
  PluginEditor.h/.cpp      GUI-Layout
  DSP/
    ChannelDelayLine.h     Fraktionale Delay-Line (L/R Delay)
    LCRExtractor.h         STFT-basierte LCR-Extraktion
  GUI/
    GoniometerComponent.*  Vektorskop
    CustomLookAndFeel.h    Modernes, schlankes Design
```

## Nächste sinnvolle Schritte

1. Lokal bauen und in einer DAW laden.
2. LCR-Modus abhören — Sensitivity-Kurve und ggf. `fftOrder`/`hopSize` in
   `LCRExtractor.h` feinjustieren (kleinere FFT = weniger Latenz, aber
   ungenauere Trennung).
3. Bei Bedarf: Stereo-Pegel-Meter, Preset-Verwaltung oder Mono-Kompatibilitäts-
   Anzeige ergänzen — sag einfach Bescheid.
