#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"

// Vektorskop / Goniometer mit Nachleucht-Effekt.
// Darstellung 45 Grad gedreht: Mono-Signal erscheint senkrecht,
// volle Stereobreite erscheint horizontal (wie bei Goodhertz Mid/Side).
// Skaliert sich automatisch an den aktuellen Pegel (nutzt die volle
// Flaeche auch bei leiseren Signalen, zieht bei lauten Signalen wieder
// ein) - keine mathematisch exakte Kalibrierung, nur eine plausible
// Naeherung fuer eine gute visuelle Nutzung der Flaeche.
class GoniometerComponent : public juce::Component, private juce::Timer
{
public:
    explicit GoniometerComponent (LCRMSAudioProcessor& processorToRead);
    ~GoniometerComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Preset-Menue (User-Idee "Deactivate Goniometer"): stoppt den eigenen
    // 30Hz-Timer komplett und blendet die Komponente aus - spart CPU, statt
    // nur unsichtbar zu sein (reine Component-Sichtbarkeit haette den Timer
    // weiterlaufen lassen). Getrennt von setSpaceVisualsEnabled(), das nur
    // die dekorative Sternen-/Glow-Ebene betrifft und Goniometer + Timer
    // weiterlaufen laesst.
    void setGoniometerActive (bool active);
    // Preset-Menue (User-Idee "Deactivate Space Visuals"): blendet nur die
    // "Hyperspace"-Sterne und den Mond aus, das eigentliche Vektorskop
    // (Scope-Trace) bleibt unveraendert sichtbar/aktiv.
    void setSpaceVisualsEnabled (bool enabled);

    // "Starfield"-Redesign (User-Wunsch): der Mond soll ungefaehr so gross
    // gezeichnet werden wie der Gravity-Regler tatsaechlich auf dem Bildschirm
    // ist - PluginEditor::layoutContent() kennt die aktuelle, dynamisch an
    // die Fenstergroesse angepasste Gravity-Knob-Groesse (70-190px, siehe
    // dortiger gravSize) und reicht sie bei jedem Resize hier rein, statt
    // dass der Mond einen fest verdrahteten, unabhaengigen Wert benutzt.
    void setGravityKnobDiameter (float diameterPx);

private:
    void timerCallback() override;
    void generateBackground();

    LCRMSAudioProcessor& processor;
    GonioRingBuffer& ring;
    int lastReadIndex = 0;

    juce::Image fadeImage;

    // Auto-Skalierung: langsam nachlaufender Pegel-Schaetzwert.
    float peakEnvelope = 0.2f;

    // "Hyperspace"-Sternenflug-Hintergrund (Star-Wars-artiger Warp-Effekt):
    // Sterne fliegen radial vom Zentrum nach aussen, werden dabei schneller
    // und ziehen einen Streak-Schweif - beim Erreichen des Randes spawnen
    // sie nahe der Mitte neu. Rein prozedural, kein Bild-Asset noetig, und
    // in den Nachleucht-Puffer mit eingerechnet, damit die Streaks ohne
    // zusaetzlichen Zeichenaufwand weich ausfaden - bleibt CPU-guenstig.
    struct FlyingStar { float angle; float radius; float speed; float alpha; };
    std::vector<FlyingStar> bgStars;
    juce::Random bgRandom { 90210 };
    bool spaceVisualsEnabled = true;

    void respawnStar (FlyingStar& s, bool randomiseRadius);

    // "Starfield"-Redesign (User-Wunsch: "loesche alle Planeten die bisher
    // eingebaut sind"): die alten Flow-/Width-Boost-"Planeten"
    // (ReactiveGlow-Objekte flowPlanet/widthMoon, ein-/ausblendend je nach
    // Reglerstaerke) sind komplett entfernt. Ersetzt durch EINEN einzigen,
    // immer sichtbaren "Mond" (siehe Chat-Spezifikation, Koordinatensystem
    // X/Y je -1..+1, Mitte = 0/0):
    //  - Groesse: ungefaehr so gross wie der Gravity-Regler, siehe
    //    setGravityKnobDiameter() - NICHT an dessen Wert gekoppelt, rein
    //    optisch/fix pro Fenstergroesse.
    //  - Start-/Ruheposition: X=0, Y=0,5 (obere Haelfte, siehe timerCallback()).
    //  - Bewegung: Tilt (Position-Sektion) UND Drift (Timewarp-Sektion)
    //    verschieben beide die X-Position, invertiert (Regler nach links =
    //    Mond nach rechts), je Regler max +-0,25, addiert zusammen max
    //    +-0,5 auf der X-Achse (User-Wunsch, inkl. Korrektur "beide je
    //    maximal +-0,25 - zusammen dann max +-0,5").
    //  - Y bleibt vorerst fix bei 0,5 (User: "erstmal ja, aber ich moechte
    //    noch mehr Einfluss erzeugen im naechsten Build" - absichtlich noch
    //    NICHT weiter ausgebaut, siehe Kommentar in timerCallback()).
    juce::Colour moonColour { 0xffe8ecf5 }; // dezentes, warmes Grau-Weiss (kein Sektions-Farbton, da nicht an EINE Sektion gebunden)
    float moonRadiusPx = 20.0f; // Fallback bis der erste setGravityKnobDiameter()-Aufruf kommt

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GoniometerComponent)
};
