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
class GoniometerComponent : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    explicit GoniometerComponent (LCRMSAudioProcessor& processorToRead);
    ~GoniometerComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Die Sonne/der Mond ist ein eigenes Bedienelement im Feld und braucht
    // deshalb einen eigenen Hinweis (User: "aufteilen mit info: starfield
    // hover + sun hover"). Liegt die Maus darueber, gewinnt dieser Text.
    juce::String getTooltip() override
    {
        if (sunHitR > 0.0f && isMouseOverOrDragging())
        {
            const auto m = getMouseXYRelative().toFloat();
            if (m.getDistanceFrom (juce::Point<float> (sunHitX, sunHitY)) <= sunHitR)
                return "Sun / Moon: click to switch between the full scene and the clean meter view. Cmd-click to dim the scene";
        }
        return juce::SettableTooltipClient::getTooltip();
    }

    // Preset-Menue "Deactivate Goniometer": blendet NUR die gruene
    // technische Scope-Spur (die eigentlichen Vektorskop-Punkte) aus -
    // Starfield (Sterne/Mond/Planeten/Glows) laeuft unveraendert weiter
    // (User-Korrektur: "soll lediglich das gruene technische Goniometer
    // unsichtbar machen, Starfield soll weiter laufen"). Der 30Hz-Timer
    // wird nur dann komplett gestoppt (CPU sparen), wenn WEDER die
    // Scope-Spur NOCH Starfield etwas anzuzeigen haben, siehe
    // updateTimerRunning().
    void setGoniometerActive (bool active);
    // Preset-Menue (User-Idee "Deactivate Space Visuals"): blendet nur die
    // "Hyperspace"-Sterne und den Mond aus, das eigentliche Vektorskop
    // (Scope-Trace) bleibt unveraendert sichtbar/aktiv.
    void setSpaceVisualsEnabled (bool enabled);
    // View-Panel "Show": 0 = Off, 1 = Stars (nur Foto + funkelnde Sterne),
    // 2 = Full (alles). Stars ist der dezente Modus fuer Hintergruende.
    void setStarfieldMode (int mode);
    // Funkelnde Sterne direkt aus-/einblenden (View-Panel, User).
    void setTwinkleVisible (bool v) { if (v != twinkleVisible) { twinkleVisible = v; repaint(); } }
    // Die radialen Sternlinien (Warp-Streaks) getrennt von den funkelnden
    // Sternen ein-/ausblenden (User-Wunsch aus der ersten Runde).
    void setStarLinesVisible (bool on) { if (starLinesVisible != on) { starLinesVisible = on; repaint(); } }
    bool getStarLinesVisible() const   { return starLinesVisible; }
    // Hintergrundfoto (0 = keins, 1..kPhotoCount). Wird ganz hinten unter
    // allen Ebenen gezeichnet, mit eigener Shine-Kurve. Klick ins Feld
    // schaltet weiter (siehe Editor).
    static constexpr int kPhotoCount = 3;
    static constexpr int kPhotoNebula = 2;
    static const char* photoName (int idx);
    void setPhoto (int idx);
    int  getPhoto() const { return photoChoice; }
    // Theme-Index wie in CustomLookAndFeel.h:
    //   0 Moon (frueher "Modern")   1 Dark Night   2 Pop   3 Sci-Fi
    //   4 Day & Night               5 Flat (frueher "Moon")
    // Die beiden Sci-Fi-Varianten (6, 7) unterscheiden sich NUR in Rahmen-
    // und Titelfarbe der GUI - die Szene ist identisch, deshalb werden sie
    // hier auf 3 abgebildet und alle Tabellen bleiben sechsstellig.
    static constexpr int kThemeCount = 6;
    void setUiTheme (int t) { uiTheme = t >= kThemeCount ? 3 : juce::jlimit (0, kThemeCount - 1, t); setGravityMode (gravityChoice); loadPhoto (effectivePhoto()); repaint(); }
    // Theme-Familien (siehe CustomLookAndFeel.h): Sci-Fi = Neon + Purple Sky, gezeichneter Himmel = flach, kein Foto.
    bool themeSciFi()  const { return uiTheme == 3; }
    bool themeMoon()   const { return uiTheme == 0; }                                   // Menue "Moon"
    bool themeFlat()   const { return uiTheme == 5; }                                   // Menue "Flat"
    bool themeModern() const { return uiTheme == 0 || uiTheme == 4 || uiTheme == 5; }   // Moon, Day & Night, Flat: kein Foto
    bool themeWater()  const { return uiTheme == 1; }                                   // Dark Night: Nebula
    // Day & Night (User): Space-Ansicht = Erde bei Tag, Stars-Ansicht = Erde bei
    // Nacht - Gravity bleibt in der Stars-Ansicht sichtbar.
    bool themeDayNight() const { return uiTheme == 4; }
    // Gravity-Planet: 0 = Sci-Fi (gezeichnet), 1 = Mond, 2 = Erde,
    // 3 = Erde bei Nacht, 4 = roter Planet (alle Foto), 5 = keiner.
    static constexpr int kGravityModes = 6;
    void setGravityMode (int mode);


    // "Starfield"-Redesign (User-Wunsch): der Mond soll ungefaehr so gross
    // gezeichnet werden wie der Gravity-Regler tatsaechlich auf dem Bildschirm
    // ist - PluginEditor::layoutContent() kennt die aktuelle, dynamisch an
    // die Fenstergroesse angepasste Gravity-Knob-Groesse (70-190px, siehe
    // dortiger gravSize) und reicht sie bei jedem Resize hier rein, statt
    // dass der Mond einen fest verdrahteten, unabhaengigen Wert benutzt.
    void setGravityKnobDiameter (float diameterPx);

    // Menue-Eintrag "Mod Movement in Starfield": ist er aus, ignoriert das
    // Sternenfeld die laufende Modulation und folgt nur noch den
    // Reglerstellungen selbst (siehe liveOrRaw() in timerCallback()).
    void setModMovementEnabled (bool enabled) { modMovementEnabled = enabled; }
    // Menue-Eintrag "Reduce Animations": halbiert die Menge bewegter Objekte
    // (Sternlinien, Orbit-Sterne, gleichzeitige Sternschnuppen). Spart
    // nebenbei CPU, weil wirklich weniger gezeichnet wird - nicht nur
    // langsamer.
    void setReducedAnimations (bool reduced) { reducedAnimations = reduced; }

    // ===== VIEW-PANEL (Zahnrad im Sternenfeld) =====
    // Vier reine Darstellungsregler, keine Audio-Wirkung:
    //  - Speed: skaliert die Szenenzeit und die Sternlinien-Geschwindigkeit
    //    fuer alles, was NICHT von Reglern/Modulation gesteuert wird.
    //  - Density: Anteil der gezeichneten Sternlinien (0.3..1.5, ueber 1
    //    werden zusaetzliche Sterne aus dem Pool aktiv).
    //  - Brightness: Deckkraft der gesamten Szene (Sterne, Objekte, Spur).
    void setViewSpeed (float v)      { viewSpeed = juce::jlimit (0.02f, 3.0f, v); }   // min 2 % (User: "ganz anhalten macht keinen Sinn")
    void setViewDensity (float v)    { viewDensity = juce::jlimit (0.2f, 1.5f, v); }
    //  - Dim: dunkelt das Sternenfeld ab (nicht die Scope-Spur).
    //  - Farben: drei GUI-Farben (0 = Blau, 1 = Gruen/Tuerkis, 2 = Violett)
    //    plus Saettigung - fuer Goniometer und Sternenfeld getrennt.
    //    Goniometer-Standard ist Gruen/Tuerkis (die bisherige Spurfarbe),
    //    Sternenfeld-Standard ist Blau.
    //  - Shine (ersetzt Dim): 0 = alles 90 % abgedunkelt (Standard-Basis),
    //    1 = jede Objektart auf ihrem eigenen Maximum (siehe paint()).
    // Shine-Regler 0..1 (Standard 0 = ganz links) -> interne Helligkeit
    // 0,06..1: der Standard liegt bewusst dunkel (User: "da wo bisher ca.
    // -90 % war"), das Maximum bleibt wie gehabt.
    // Regler 0..1 -> intern 0,298..0,70 (Min = alte 40 %, Max = alte 70 %, User).
    void setViewShine (float v)      { viewShine = 0.298f + 0.507f * juce::jlimit (0.0f, 1.0f, v); }   // min = alte 40 %, max = alte 85 % -5 % (User)
    static constexpr float kShineMax = 0.298f + 0.507f;
    //  - Goniometer: Farbe 0..3 (Blau, Gruen, Violett, Gold), sat = Intensitaet
    //    (Deckkraft der Spur), Glow = Schein um die Punkte.
    // Gonio-Farbe (User, Runde 19): nur noch zwei Moeglichkeiten -
    // 0 = Theme (der Grundton des aktuellen Themes), 1 = White.
    void setGonioColour (int idx, float sat) { gonioColourIdx = juce::jlimit (0, 1, idx); gonioSat = juce::jlimit (0.0f, 1.0f, sat); }
    static constexpr int kGonioColours = 2;
    void setGonioGlow (float v)     { gonioGlow = juce::jlimit (0.0f, 1.0f, v); }
    //  - Look: 0 = Solid (normale Deckkraft), 1 = Soft (sehr niedrige
    //    Deckkraft je Punkt, Spur wird zur weichen Dichtewolke - der "edle"
    //    Look, den der User bei Intensity 0 % gefunden hat - hier heller
    //    gemacht, indem die Spur-Ebene mehrfach uebereinandergelegt wird).
    void setGonioLook (int look)    { gonioLook = juce::jlimit (0, 1, look); }
    // Stars-Modus = "kluges System" (User): Spur automatisch an, Shine
    // automatisch ganz unten, Spur automatisch gross.
    bool  traceOn() const       { return scopeTraceVisible || starfieldMode == 1; }
    float effShine() const      { return starfieldMode == 1 ? 0.03f : viewShine; }
    float traceSizeMul() const  { return starfieldMode == 1 ? 0.50f : 0.42f; }
    int  getGonioColourIndex() const { return gonioColourIdx; }
    //  - Style der Scope-Spur: 0 = Punkte (bisher), 1 = Linie (klassisches
    //    Vektorskop), 2 = weiche Punkte mit Schein. Size: 0 = normal, 1 = klein.
    void setGonioStyle (int style) { gonioStyle = juce::jlimit (0, 2, style); }
    //  - Size: 0 = Small (50 %), 1 = Medium (70 %, Standard).
    void setGonioSize (int idx)     { gonioSize = juce::jlimit (0, 1, idx); }
    //  - Speed der Spur: 0 = Slow (langes Nachleuchten), 1 = Mid, 2 = Fast (bisher).
    void setGonioSpeed (int v)      { gonioSpeed = juce::jlimit (0, 2, v); }
    void setStarColour (int idx, float sat)  { starColourIdx  = juce::jlimit (0, 2, idx); starSat  = juce::jlimit (0.25f, 1.0f, sat); }
    static float paletteHue (int idx)
    {
        // Farbtoene der GUI-Palette: Blau 4fa8ff, Gruen 5be3c7, Violett b968ff, Gold ffc247, Pink ff6fb1
        static const float hues[5] = { 0.583f, 0.467f, 0.756f, 0.114f, 0.924f };
        return hues[juce::jlimit (0, 4, idx)];
    }
    // Grundton der Spur je Theme. Moon ist bewusst gold-blau-weiss und
    // eher warm als kuehl (User) - das Theme selbst wirkt warm.
    static juce::Colour themeTraceColour (int theme)
    {
        static const juce::uint32 byTheme[kThemeCount] = {
            0xffe9dcc0,   // 0 Moon: warmes Gold-Weiss mit blauem Hauch
            0xffeed9a6,   // 1 Dark Night: Gold
            0xffc9a4f0,   // 2 Pop: Violett
            0xffa8dcf5,   // 3 Sci-Fi: Cyan
            0xffeed9a6,   // 4 Day & Night: Gold
            0xffc2dcf2    // 5 Flat: kuehles Silberblau
        };
        return juce::Colour (byTheme[juce::jlimit (0, kThemeCount - 1, theme)]);
    }
    juce::Colour traceColour() const
    {
        return gonioColourIdx == 1 ? juce::Colour (0xfff0f4fa) : themeTraceColour (uiTheme);
    }
    juce::Colour tintedLineColour (juce::Colour base) const
    {
        // Bei Blau + Mindestsaettigung bleibt das bisherige Bild (weiss,
        // blau bei Galaxy) praktisch unveraendert; mehr Saettigung faerbt.
        const juce::Colour tint = juce::Colour::fromHSV (paletteHue (starColourIdx), starSat, 1.0f, 1.0f);
        return base.interpolatedWith (tint, starSat);
    }

    // Hidden Egg (User-Idee): ein kleines Raumschiff fliegt einmal durchs
    // Bild. Wird vom Editor selten ausgeloest (siehe dort) - nie von selbst
    // im Dauerbetrieb, damit es ein Ereignis bleibt.
    void triggerEasterEgg() { if (eggT < 0.0f) eggT = 0.0f; }

    // Klick ins Feld: der Editor entscheidet, was passiert (aktuell: die
    // Goniometer-Farbe weiterschalten, 5. Klick = Spur aus).
    // Klick ins Feld: 0 = naechstes Foto, 1 = vorheriges, 2 = Goniometer
    // an/aus (Feldmitte), 3 = Sonne/Mond (Space <-> Gonio-Ansicht),
    // 4 = Cmd+Mitte: Farbe weiter, 5 = Shift+Mitte: Look wechseln.
    std::function<void (int)> onFieldClick;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    juce::ModifierKeys pressMods;

private:

    void timerCallback() override;
    void generateBackground();
    // Startet/stoppt den gemeinsamen 30Hz-Timer je nachdem, ob Scope-Spur
    // ODER Starfield gerade etwas zu zeichnen haben (siehe setGoniometerActive()/
    // setSpaceVisualsEnabled()).
    void updateTimerRunning();
    bool scopeTraceVisible = true;

    LCRMSAudioProcessor& processor;
    GonioRingBuffer& ring;
    int lastReadIndex = 0;

    // ===== EBENEN =====
    // Jede Objektart liegt in einer eigenen, transparenten Ebene und wird in
    // paint() mit ihrer eigenen Helligkeit gezeichnet (Shine-Regler, siehe
    // dort). Nur so lassen sich Planeten, Linien, Sternschnuppen usw.
    // UNTERSCHIEDLICH stark abdunkeln - und nur so bleibt eine Abdunklung
    // auch bei Nachleuchten erhalten (frueherer Bug: ein Objekt, das jeden
    // Frame halbtransparent in einen Nachleucht-Puffer gezeichnet wird,
    // summiert sich dort wieder auf volle Helligkeit auf).
    juce::Image fadeImage;      // Sternlinien (+Orbit, +RAYE-Gold), Nachleuchten
    juce::Image twinkleImage;   // funkelnde Sterne (jeden Frame neu)
    juce::Image objectImage;    // Planeten, Galaxien, Asteroiden, Sternenzerstoerer (jeden Frame neu)
    juce::Image glowImage;      // Selbstleuchter: Sonne, Kometen (jeden Frame neu, eigene Shine-Kurve)
    juce::Image compositeImage; // fertige Szene (alle Ebenen + Vignette), im Timer zusammengesetzt
    void composeScene();
    float sunHitX = -1.0f, sunHitY = -1.0f, sunHitR = 0.0f;   // Klickflaeche der Comic-Sonne
    float sunDrawR = 0.0f;                                    // sichtbarer Radius (Hover-Ring)
    bool  starLinesVisible = true;                            // Sternlinien an/aus (View-Panel)
    // Hintergrundfoto: Original (dekodiert) + auf Feldgroesse skalierte Kopie
    // (Cover-Zuschnitt), damit paint() pro Frame nur ein drawImageAt braucht.
    int photoIndex = 0;             // tatsaechlich geladenes Foto
    int photoChoice = 1;
    juce::Image photoScaledGrey;   // entsaettigte Kopie fuer die Stars-Ansicht
    int uiTheme = 0;
    int gravityChoice = 0;   // Wahl im Panel; gravityMode = ggf. vom Theme erzwungen
    float vigSm = 0.0f;      // Vignette weich ein/aus
    float idleSm = 0.0f;     // Ruhezustand des Gonios weich ein/aus            // Wahl im Panel (Space); Gonio-Ansicht erzwingt Nebula
    juce::Image photoSrc, photoScaled, photoScaledPrev;
    float photoXfade = 0.0f;        // 1 -> 0: altes Foto blendet aus
    int  effectivePhoto() const;
    void loadPhoto (int idx);
    void rescalePhoto();
    // Weiche Uebergaenge
    float spaceBlend = 1.0f;        // 1 = Space-Ebenen sichtbar, 0 = Gonio-Ansicht
    float shineSm = 0.30f, sizeSm = 0.42f;
    // Zweiter, geglaetteter Shine-Wert, der IMMER dem Regler folgt - auch in
    // der Stars-Ansicht, wo effShine() hart auf das Minimum springt. Nur
    // damit wird die Abdunklung der Objekte beim Ansichtswechsel stetig;
    // vorher blitzten die Planeten beim Umschalten kurz auf (User, Pop).
    float shineSpaceSm = 0.30f;
    // Sehr langsam nachgezogener Gravity-Wert. In der Stars-Ansicht wird die
    // Modulationsbewegung des Gravity-Planeten auf 10 % zurueckgenommen
    // (User: "soll ja alles ruhiger werden, und Gravity alleine als Mod zu
    // sehen macht auch keinen Sinn") - dafuer braucht es einen Bezugswert,
    // um den herum die Bewegung gedaempft wird.
    float gravVisSlow = 0.0f;
    int starfieldMode = 2;          // 1 Stars (Foto + Funkeln), 2 Space (alles); 0 wird wie 1 behandelt
    bool twinkleVisible = true;     // View-Panel: "Stars" Show/Hide
    float photoClock = 0.0f;        // Echtzeit-Uhr fuer die sanfte Foto-Bewegung
    int gravityMode = 0;
    juce::Image gravityPhoto[4];    // lazy geladen: Mond, Erde, Erde-Nacht, roter Planet
    juce::Image gravScaled; int gravScaledMode = -1, gravScaledW = 0;   // skalierte Kopie (Cache)
    juce::Image gravScaledN; int gravScaledNMode = -1, gravScaledNW = 0; // zweite Kopie: Erde bei Nacht (Day & Night)
    const juce::Image& gravityPhotoFor (int mode);
    // Szenen-Sprites (Planeten-Fotos): RGB-JPEG + Alpha-PNG, lazy dekodiert,
    // pro Sprite eine auf die aktuelle Zielbreite skalierte Kopie.
    static constexpr int kSpriteCount = 15;
    juce::Image spriteSrc[kSpriteCount], spriteScaled[kSpriteCount];
    int spriteScaledW[kSpriteCount] = {};
    const juce::Image& spriteFor (int idx, int targetW);
    juce::Image shootImage;     // Sternschnuppen, Nachleuchten
    juce::Image gravityImage;   // grosser Gravity-Planet + Hidden Egg (jeden Frame neu)
    juce::Image traceImage;     // Scope-Spur, eigenes Nachleuchten

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
    bool modMovementEnabled = true;
    bool reducedAnimations = false;
    float viewSpeed = 1.0f, viewDensity = 1.0f, viewShine = 0.298f;   // viewShine intern (siehe setViewShine)
    int gonioColourIdx = 0, starColourIdx = 0;   // beide Blau (Standard)
    int gonioStyle = 2;        // Glow (Standard, User)
    int gonioLook = 0;         // Solid
    int  gonioSize = 1;        // Normal (Standard, User)
    float gonioGlow = 0.2f;
    int gonioSpeed = 1;        // Mid (Standard, User)
    float gonioSat = 0.85f, starSat = 0.25f;   // gonioSat = Intensitaet (Deckkraft) der Spur, 0..1

    // Funkelnde Sterne (Visual-Pool): feste, ferne Punkte, die nur in der
    // Helligkeit atmen. Bewusst OHNE Parallaxe - was unendlich weit weg ist,
    // bewegt sich nicht, und genau dieser Kontrast zu den fliegenden Linien
    // macht die Tiefe.
    struct TwinkleStar { float x, y, size, phase, period; };
    std::vector<TwinkleStar> twinkles;

    // Fortschritt des Raumschiff-Flugs (0..1), -1 = inaktiv.
    float eggT = -1.0f;
    // Sternenzerstoerer (zweites Easter Egg): zieht alle paar Minuten sehr
    // langsam und dunkel durchs Bild. destroyerT = Fortschritt 0..1, -1 = ruht.
    float destroyerT = -1.0f;
    float destroyerWait = 60.0f;   // Sekunden bis zum naechsten Auftritt
    float destroyerY = 0.35f;      // Hoehe im Bild (0..1), pro Auftritt neu
    bool  destroyerLeftToRight = true;

    // Kometen/Asteroiden-Pool (max. 1 Komet + 2 Asteroiden, 3 gesamt):
    // Position/Geschwindigkeit als Anteile von Feldbreite/-hoehe, Groesse
    // als Anteil von bgRadius. Ruhen die meiste Zeit (transientWait).
    struct Transient
    {
        bool  active = false, comet = false;
        float x = 0.0f, y = 0.0f, vx = 0.0f, vy = 0.0f;
        float size = 0.01f, spin = 0.0f, spinRate = 0.0f, bright = 1.0f;
        int   seed = 0;
    };
    Transient transients[3];
    float transientWait = 20.0f;

    // Geglaettete Reglerwerte fuer die Himmelskoerper (siehe timerCallback):
    // Preset-Wechsel/Reglerspruenge gleiten in ~0,45 s statt zu springen.
    float lastFrameDt = 1.0f / 30.0f;
    float twinkleClock = 0.0f;      // eigene, langsamere Uhr der funkelnden Sterne
    // Speed fuer alles ausser Sternlinien: gedeckelt und mit Untergrenze
    // (User: "nie absurd schnell", "andere Objekte bewegen sich trotzdem noch").
    float sceneSpeedMul() const { return juce::jlimit (0.20f, 1.25f, viewSpeed); }
    float smDrift = 0.0f, smTilt = 0.0f, smElevate = 0.0f, smWidth = 0.0f, smDistance = 0.0f;

    void respawnStar (FlyingStar& s, bool randomiseRadius);

    // Himmelskoerper (Redesign): fuenf feste, prozedural gezeichnete
    // Objekte (Ringplanet, Baenderplanet, Erde, kleine Sonne, Spiralgalaxie)
    // plus Kometen/Asteroiden-Pool, siehe timerCallback(). Alle Groessen
    // leiten sich von moonRadiusPx ab - dem Referenzradius aus dem
    // Gravity-Knopf (setGravityKnobDiameter()), rein optisch, nicht an
    // dessen Wert gekoppelt.
    float moonRadiusPx = 20.0f; // Fallback bis der erste setGravityKnobDiameter()-Aufruf kommt

    // Geglaetteter Gravity-Wert NUR fuer die Darstellung des grossen
    // Planeten am unteren Bildrand.
    // Bug (User: "wenn section aktiviert wird springt der untere Planet nach
    // oben, weil er davor dem gravity minimum wert entspricht"): gravityNorm
    // wird durch das Sektions-Gating hart auf 0 gesetzt, sobald die
    // Galaxy-Sektion aus ist. Beim Einschalten sprang der Wert dadurch in
    // EINEM Frame von 0 auf den echten Reglerwert - und der Planet machte
    // einen sichtbaren Satz nach oben.
    // Ein gegateter Wert, der eine POSITION steuert, muss immer geglaettet
    // werden - sonst ist jeder Schaltvorgang ein Sprung. Der Zielwert wird
    // pro Frame nur anteilig angenaehert (siehe timerCallback()).
    float gravityVisSmoothed = 0.0f;

    // ===== FLOW: tempo-synchrone Sternschnuppen =====
    // Die Hyperdrive-Sektion (Flow/Pulse/Speed/Sync) hatte bisher keine
    // visuelle Entsprechung. User-Vorgabe: "Letztlich ist nur der finale
    // 'Flow' Output Wert relevant" - deshalb haengt die Ausloesung NICHT an
    // den einzelnen Reglern, sondern am tatsaechlichen Pan-Ausgabewert des
    // Auto-Pan-LFO (processor.currentPanPos). Bei jedem Nulldurchgang nach
    // oben faellt genau eine Sternschnuppe. Das ist automatisch tempo-
    // synchron, sobald Sync aktiv ist, ohne dass hier irgendetwas ueber
    // Takte oder BPM wissen muesste.
    // ===== TRANSPORT-FADE: langsam anhalten, langsam anfahren =====
    // User-Wunsch: "alles langsam anhalten und langsam starten (fade in/out)
    // wenn DAW Stop/start".
    //
    // Umgesetzt NICHT als Deckkraft-Blende, sondern als Zeitlupe: die ganze
    // Szene benutzt keine absolute Uhrzeit mehr, sondern eine eigene,
    // akkumulierte Szenenzeit (sceneClock), die pro Frame nur um
    // dt * motionScale weiterlaeuft. motionScale faehrt bei Stop weich auf 0
    // und bei Play weich zurueck auf 1.
    //
    // Der Unterschied ist entscheidend: bei einer Deckkraft-Blende wuerden
    // Mond und Planeten waehrend des Ausblendens weiter fliegen und beim
    // naechsten Play an einer voellig anderen Stelle wieder auftauchen. Mit
    // der Zeitlupe kommt die Szene dort zum Stehen, wo sie war, und faehrt
    // genau von dort wieder an - so, wie ein Schwungrad auslaeuft.
    // Derselbe Faktor bremst auch die Sternlinien und die Sternschnuppen,
    // damit wirklich ALLES gemeinsam anhaelt und nicht nur die Objekte.
    float motionScale = 1.0f;       // 0 = eingefroren, 1 = volle Geschwindigkeit
    float sceneClock  = 0.0f;       // akkumulierte, verlangsambare Szenenzeit (Sekunden)
    juce::uint32 lastTickMs = 0;    // fuer die echte Frame-Dauer (dt)

    struct ShootingStar { float x, y, vx, vy, life; };
    std::vector<ShootingStar> shootingStars;
    // Letzter Pan-Wert, um den Nulldurchgang zu erkennen.
    float lastFlowPan = 0.0f;
    // Letzte Aenderungsrichtung des Pan-Werts - daran wird der Umkehrpunkt
    // des LFO erkannt (Vorzeichenwechsel der Steigung).
    float lastFlowDelta = 0.0f;

    // Grosser Starfield/Visuals-Umbau (siehe timerCallback() fuer die
    // vollstaendige Umsetzung, Spezifikation im Aenderungs-Changelog):
    //  - Orbit-Sterne: fester Pool, siehe generateBackground()/timerCallback().
    std::vector<FlyingStar> orbitStars;
    //  - Wellen-Phase fuer den Shift-Effekt (wellenfoermige Sternlinien),
    //    laeuft kontinuierlich weiter, unabhaengig vom Reglerwert selbst.
    float wavePhase = 0.0f;
    // Die 3 kleinen, immer sichtbaren Planeten (User-Wunsch: "Polarity
    // Planeten Einfluss wieder loeschen - 3 kleine Planeten einfach so ins
    // Bild rein machen, wie der Mond") brauchen KEINEN eigenen State mehr -
    // Basis-Positionen/Farben sind feste Konstanten direkt in
    // timerCallback(), Bewegung kommt live aus denselben Werten wie beim
    // Mond (kein Ein-/Ausfaden, keine Buttons mehr involviert).

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GoniometerComponent)
};
