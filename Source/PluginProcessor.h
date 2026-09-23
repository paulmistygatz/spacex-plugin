#pragma once
#include <JuceHeader.h>
#include "DSP/ChannelDelayLine.h"
#include "DSP/LCRExtractor.h"
#include "DSP/SimplePitchShifter.h"
#include "Licence.h"
#include <array>
#include <atomic>

// Ringpuffer fuer das Goniometer (lock-free, single-producer/single-consumer)
struct GonioRingBuffer
{
    static constexpr int size = 4096;
    std::array<float, size> left {};
    std::array<float, size> right {};
    std::atomic<int> writeIndex { 0 };

    void push (float l, float r)
    {
        int idx = writeIndex.load (std::memory_order_relaxed);
        left[(size_t) idx] = l;
        right[(size_t) idx] = r;
        writeIndex.store ((idx + 1) % size, std::memory_order_release);
    }
};

#ifndef SPACEX_RAYE_UI
 #define SPACEX_RAYE_UI 0     // 1 = SpaceXraye: Amount + Charakter
#endif
#ifndef SPACEX_PARALLAX_UI
 #define SPACEX_PARALLAX_UI 0   // 1 = drei Regler (SpaceXparaCPU), sonst Modi + Amount
#endif
#ifndef SPACEX_CPU_OPT
 #define SPACEX_CPU_OPT 1     // 1 = SpaceXparaCPU: optimierte DSP
#endif

class LCRMSAudioProcessor : public juce::AudioProcessor
{
public:
    LCRMSAudioProcessor();
    ~LCRMSAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;

    // Undo/Redo-Historie lebt im Processor, nicht im Editor (User: "undo redo
    // nach Schliessen vom Plugin nicht mehr moeglich, muss aber gehen") -
    // Snapshots des APVTS-State-Baums, siehe PluginEditor::pushUndoSnapshotNow().
    juce::Array<juce::MemoryBlock> undoHistory;
    int undoIndex = -1;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SpaceX"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    GonioRingBuffer gonioBuffer;

    // Aktuelle Auto-Pan-Position (-1..1), fuer die Live-Anzeige im Movement-
    // Regler (Ring + leuchtender Punkt). Wird am Ende jedes Blocks aktualisiert.
    std::atomic<float> currentPanPos { 0.0f };

    // Manueller GUI-Bypass (Klick auf das Logo) - komplett unabhaengig von
    // Host-Automation/Parametern, rein fuer schnelles A/B im Studio. Haelt
    // die Host-Latenz-Kompensation korrekt aufrecht (siehe processBlock/
    // passthroughWithLatencyCompensation). Die GUI liest denselben Wert, um
    // saemtliche Animationen (Sterne, Goniometer-Trace) einzufrieren.
    std::atomic<bool> uiBypassed { false };

    // ===== DEMO-MODUS =====
    // Ohne gueltige Seriennummer laeuft das Plugin vollstaendig - es wird nur
    // alle 50 Sekunden fuer gut drei Sekunden leise. Bewusst KEIN Zeitlimit:
    // ein Demo, das nach 14 Tagen tot ist, zwingt zur Kaufentscheidung, bevor
    // jemand das Plugin in einem echten Mix gehoert hat. Bewusst auch kein
    // Rauschen - das verfaelscht den Eindruck vom Klang.
    std::atomic<bool>  licensed { false };
    std::atomic<float> demoDuck { 1.0f };   // 1 = volle Lautstaerke, 0 = stumm; die GUI dimmt entsprechend mit

    // ===== AUTO GAIN =====
    // Gerade angewandter Ausgleich in dB, nur zur Anzeige im Footer. Diese
    // eine Zahl sagt mehr ueber die eigenen Sektionen aus als jede Anleitung:
    // man sieht schwarz auf weiss, welche Einstellung Pegel macht und welche
    // wirklich etwas am Bild aendert.
    std::atomic<float> autoGainDb { 0.0f };

    // Uebernimmt eine Seriennummer (bereits geprueft) und merkt sie sich.
    void storeLicence (const juce::String& serial)
    {
        juce::PropertiesFile props (appPropertiesOptions());
        props.setValue ("licence", spacex::normaliseSerial (serial));
        props.saveIfNeeded();
        licensed.store (true, std::memory_order_relaxed);
    }

    // Host-Bypass als echter Parameter: dann ruft der VST3-Wrapper bei
    // Host-Bypass weiter processBlock() auf (statt processBlockBypassed)
    // und wir koennen Wet und Original weich ueberblenden - der harte
    // Schnitt war das Knacken (User: "bypass plugin ruckelt/knackt").
    juce::AudioProcessorParameter* getBypassParameter() const override { return hostBypassParam; }
    bool isBypassedNow() const
    {
        return uiBypassed.load (std::memory_order_relaxed)
            || (hostBypassParam != nullptr && hostBypassParam->get());
    }

    // "Chaos"-Button (GUI, siehe PluginEditor): Trigger-Flag von der Message-
    // Thread zur Audio-Thread, um kurz vor dem Randomisieren aller Regler das
    // Anti-Vol-Jump-Ducking auszuloesen (siehe chaosDuckGain/processBlock).
    // Muss public sein, da der Editor sie direkt setzt.
    std::atomic<bool> chaosTriggerRequested { false };

    // Bug-Fix ("Galaxy on startup geht nicht"): manche Hosts rufen auch bei
    // einer BRANDNEUEN Instanz kurz nach dem Konstruktor noch einmal
    // setStateInformation() mit ihrem eigenen, gecachten Default-Snapshot
    // auf (der die eingebauten createParameterLayout()-Defaults enthaelt,
    // also Galaxy=aus) - das ueberschrieb unseren im Konstruktor gesetzten
    // "Activate Galaxy als Standard"-Wert sofort wieder. Wird von
    // setStateInformation() auf true gesetzt; der Default wird jetzt per
    // kurz verzoegertem Callback (siehe Konstruktor) angewendet, NACHDEM
    // ein echter Host-Zustand (falls vorhanden) bereits eingetroffen ist.
    bool hasReceivedExternalState = false;

    // Stereo-Korrelation des finalen Ausgangssignals (-1..+1), einmal pro
    // Block berechnet - fuer den Korrelationsmesser in der GUI (nur Anzeige,
    // kein Einfluss auf die Verarbeitung).
    std::atomic<float> currentCorrelation { 0.0f };

    // Input/Output-Pegelanzeige (User-Wunsch: "Links und rechts von Volume
    // ein kleines Input und Output Meter Pegelanzeige") - roher Block-Peak,
    // reine Anzeige ohne Rueckwirkung auf die Verarbeitung. Input = vor
    // jeder Bearbeitung, Output = nach der kompletten Kette inkl. finaler
    // Gain-Stufe. Das GUI-Meter macht sein eigenes Attack/Decay.
    std::atomic<float> currentInputLevel { 0.0f };
    std::atomic<float> currentOutputLevel { 0.0f };

    // Seiten-Betonung des aktuellen Signals (0 = reines Center/Mono, 1 =
    // stark seitenbetont), aus dem Verhaeltnis Side- zu Mid-Energie
    // berechnet und geglaettet - treibt im Goniometer-Hintergrund die
    // Helligkeit der Sterne an den Raendern (siehe currentSideEmphasisSmooth
    // in PluginProcessor.cpp fuer die Glaettung).
    std::atomic<float> currentSideEmphasis { 0.0f };

    // Live-modulierte Werte (in denselben Einheiten wie die jeweiligen
    // GUI-Regler), damit die Regler-Knoepfe einen live bewegten Punkt zeigen
    // koennen (wie beim Flow-Regler) - siehe PluginEditor::timerCallback,
    // das diese Werte per slider.getNormalisableRange().convertTo0to1()
    // in die Regler-Normalposition umrechnet. Reine Anzeige, kein
    // Rueckfluss in die Verarbeitung.
    std::atomic<float> currentDriftLivePercent { 0.0f };
    std::atomic<float> currentBendLiveCt { 0.0f };
    std::atomic<float> currentParallaxAmountLive { 0.0f };   // Mod-Punkt auf Amount
    std::atomic<float> currentExpandLivePercent { 100.0f };
    std::atomic<float> currentBoostLiveDb { 0.0f };
    std::atomic<float> currentMovementLivePercent { 0.0f };
    std::atomic<float> currentSpeedLiveHz { 0.25f };
    // Galaxy (Gravity/Orbit) + Position (Offset/Width/Distance/Elevate),
    // gleiches Live-Anzeige-Prinzip wie oben.
    std::atomic<float> currentGravityLivePercent { 50.0f };
    std::atomic<float> currentOrbitLivePercent { 0.0f };
    std::atomic<float> currentOffsetLivePercent { 0.0f };
    std::atomic<float> currentPosWidthLivePercent { 100.0f };
    std::atomic<float> currentDistanceLivePercent { 0.0f };
    // DEPTH ist EIN Regler (-100..100), der intern auf Distance und
    // Elevate aufgeteilt wird - fuer den Mod-Punkt am Regler braucht es
    // deshalb den zusammengesetzten Live-Wert (Runde 51).
    std::atomic<float> currentDepthLivePercent { 0.0f };
    std::atomic<float> currentElevateLivePercent { 0.0f };

    // Zeitstempel (Millisekunden-Counter) des letzten processBlock()-Aufrufs.
    // Hintergrund (Bug: "Visuals gehen erst an wenn DAW laeuft/Audio eingeht"):
    // ALLE current...Live...-Atomics oben werden ausschliesslich in
    // processBlock() geschrieben. Ruft der Host gerade kein processBlock()
    // auf - Transport gestoppt, Spur inaktiv, oder das Plugin-Fenster wurde
    // gerade erst geoeffnet und es lief noch nie Audio - bleiben sie auf
    // ihrem letzten (bzw. dem Initial-) Wert stehen. Reglerbewegungen hatten
    // dann sichtbar KEINE Wirkung auf das Starfield, obwohl dessen 30Hz-Timer
    // im Editor ganz normal weiterlief.
    // Das Starfield liest diesen Zeitstempel und faellt auf die rohen
    // APVTS-Werte zurueck, sobald hier laenger nichts mehr passiert ist
    // (siehe GoniometerComponent::timerCallback / liveOrRaw). Das ist auch
    // inhaltlich korrekt: die Live-Werte unterscheiden sich von den rohen
    // Reglerwerten nur durch die laufende Modulation - und ohne
    // processBlock() laeuft auch keine Modulation.
    std::atomic<juce::uint32> lastProcessBlockMs { 0 };

    // Transport-Zustand des Hosts (laeuft die DAW gerade?). Wird am Ende von
    // processBlock() aus dem Playhead gelesen und vom Starfield benutzt, um
    // alle Bewegungen bei Stop sanft auszublenden und bei Play wieder
    // hochzufahren (User-Wunsch: "alles langsam anhalten und langsam starten
    // (fade in/out) wenn DAW Stop/start").
    // Ohne Playhead-Information gilt bewusst "spielt" - siehe Kommentar an
    // der Schreibstelle in processBlock().
    std::atomic<bool> currentTransportPlaying { true };

    // Galaxy-Engine "scharfschalten": im Gegensatz zum normalen Power-Icon
    // der Sektion (ID_LCR_ENABLED, jetzt reiner Bypass, siehe dort) steuert
    // NUR dieser globale Schalter die tatsaechlich an den Host gemeldete
    // Latenz (STFT-Engine an/aus) - er soll bewusst SELTEN umgeschaltet
    // werden. Grund (User-Feedback): "Wenn ich Galaxy mal kurz bypaessen
    // will, dann kommt wegen Latenz immer ein Interrupt. Activate Galaxy
    // oben global, und dann der on/off Button ganz normal Bypass." Default
    // AUS, damit das Plugin ohne bewusstes Aktivieren zero-latency bleibt
    // (urspruengliche Anforderung).
    static constexpr auto ID_GALAXY_ACTIVATE = "galaxyActivate";
    static constexpr auto ID_LCR_ENABLED = "lcrEnabled";
    static constexpr auto ID_LCR_SENS    = "lcrSensitivity";
    static constexpr auto ID_LCR_BLEND   = "lcrBlend";
    static constexpr auto ID_LCR_HORIZON = "lcrAir";   // sichtbarer Name: AIR
    static constexpr auto ID_POL_L       = "polarityL";
    static constexpr auto ID_POL_R       = "polarityR";
    // Position des Polarity-Flips im Signalfluss, waehlbar ueber 4 Buttons
    // (1..4): 0=nach LCR, 1=nach Haas/Drift, 2=nach Mid/Side (Default),
    // 3=nach Auto-Pan (ganz am Ende, kurz vor dem Goniometer).
    static constexpr auto ID_POL_POS     = "polarityPos";
    // ===== POLARITY-POSITIONEN, neu belegt =====
    // Ein Flip nur EINES Kanals kommutiert mit allem, was pro Kanal linear
    // arbeitet (Delay, Auto-Pan-Gain, RAYE-Allpaesse). Hoerbar unterscheidet
    // er sich nur an Stellen, wo L und R MISCHEN: Galaxy (LCR), Dimension
    // (M/S) und Vision (M/S-Width). Die alten Slots "nach Haas" und "nach
    // Auto-Pan" klangen deshalb identisch zu ihren Nachbarn, und "vor
    // Galaxy" fehlte ganz. Neue Belegung - vier wirklich verschiedene
    // Klaenge. Indizes 0..3 bleiben, alte Presets laden also ohne Absturz,
    // klingen aber an Position 1, 2 und 4 anders als vorher.
    static constexpr int POL_POS_BEFORE_GALAXY   = 0;   // 1: vor LCR
    static constexpr int POL_POS_AFTER_GALAXY    = 1;   // 2: nach LCR, vor Dimension
    static constexpr int POL_POS_AFTER_DIMENSION = 2;   // 3: nach M/S, vor Vision
    static constexpr int POL_POS_AFTER_VISION    = 3;   // 4: nach Vision, vor RAYE
    // "Balance": automatische Gain-Kompensation fuer den durch Drift/Haas
    // verschobenen Praezedenzeffekt (User-Feedback, mit Verweis auf Fiedler
    // Audio Splat: "Signal kuenstlich auf die Seite schieben, aber
    // wieder tarieren" - z.B. Mono-BVs erst mit Haas breit machen, dann
    // wieder mittiger). Reine An/Aus-Option (kein eigener Staerke-Regler,
    // User-Entscheidung: "Automatische Gain-Kompensation") - hebt die
    // verzoegerte (durch die Vorrangwirkung leiser wahrgenommene) Seite
    // proportional zum Drift-Betrag an, um das Wandern zur Mitte zurueckzuholen.
    static constexpr auto ID_TIMEWARP_BALANCE = "timewarpBalance";
    static constexpr auto ID_DRIFT       = "drift";
    static constexpr auto ID_BEND        = "bend";         // Micro-Pitch (0-8 Cent, L runter/R rauf)
    static constexpr auto ID_SIDE_WIDTH  = "sideWidth";
    static constexpr auto ID_SIDE_BOOST  = "sideBoost";
    static constexpr auto ID_MOVEMENT    = "movement";
    static constexpr auto ID_SPEED       = "speed";       // Bar-Sync-Raste (nur bei Sync aktiv)
    static constexpr auto ID_SPEED_RATE  = "speedRate";    // freie Hz-Rate (ohne Sync)
    static constexpr auto ID_SPEED_SYNC  = "speedSync";    // Sync an/aus
    static constexpr auto ID_PULSE       = "pulse";        // Sinus -> geglaetteter Puls

    // Section-Bypass-Schalter (jeweils oben links in der zugehoerigen
    // GUI-Gruppe, unabhaengig von den Reglerwerten - lassen die Werte
    // erhalten, schalten die Verarbeitung aber komplett aus/an).
    static constexpr auto ID_DRIFT_ON       = "driftOn";
    static constexpr auto ID_POL_ON         = "polOn";
    static constexpr auto ID_WIDTHBOOST_ON  = "widthBoostOn";
    static constexpr auto ID_FLOW_ON        = "flowOn";

    // Mod-Icons: einfaches An/Aus (keine eigene Tiefe/Depth-Regler, siehe
    // Chat - "Ich will ja eigentlich ein einfaches Plugin. Ein Icon reicht
    // aus."). Jeweils ein sanftes, langsames LFO (~8s Zykluslaenge) auf die
    // Kern-Parameter der jeweiligen Sektion:
    // - Timewarp: Drift wird um +-0.2ms UM DEN AKTUELLEN REGLERWERT moduliert
    //   (0.2ms ist nur die Modulationstiefe/Amplitude, kein fester Zielbereich
    //   wie 1.8-2.2ms - der Regler bleibt die Basis, die Modulation schwingt
    //   relativ dazu) + Shift/Bend (Cent, Tiefe je nach Bend-Reglerstand
    //   zwischen +-2 und +-6 Cent, ebenfalls relativ um den Reglerwert)
    // - Dimension ("Breathe"): Expand + Boost pulsieren GEGENPHASIG
    // - Hyperdrive: moduliert sowohl Flow (Movement) als auch Speed, aber NUR
    //   wenn Sync AUS ist (bei Sync AN ist das Icon grau/deaktiviert - macht
    //   sonst keinen Sinn im taktsynchronen Modus).
    static constexpr auto ID_TIMEWARP_MOD   = "timewarpMod";
    static constexpr auto ID_DIMENSION_MOD  = "dimensionMod";
    static constexpr auto ID_HYPERDRIVE_MOD = "hyperdriveMod";
    // Galaxy moduliert Gravity (um dessen Neutralwert 50%) + Orbit (um 0%);
    // Position moduliert alle 4 Regler (Offset/Width/Distance/Elevate)
    // gleichzeitig, jeweils um ihren eigenen Neutralwert - identisches
    // Prinzip wie Timewarp/Dimension/Hyperdrive (User-Feedback: "genau so
    // wie bei den anderen").
    static constexpr auto ID_GALAXY_MOD     = "galaxyMod";
    static constexpr auto ID_POSITION_MOD   = "positionMod";

    // Tiefe-Regler (0-100%) je Mod-Sektion, EIN Regler moduliert beide
    // zugehoerigen Parameter gleichzeitig (User-Entscheidung: "Ein Regler
    // fuer beides. Wenn man eines nicht will, dann Regler auf 0/Neutral").
    // Kurve (siehe modDepthCurve(), Stand nach User-Korrektur): Ausschlag ist
    // relativ zum eingestellten Wert selbst (bzw. dessen Abstand vom
    // Neutralwert), NICHT relativ zur vollen Parameter-Range - sonst koennte
    // ein muehsam eingestellter Reglerwert durch die Modulation ueberschrieben
    // werden. Linear von 0% (Tiefe-Regler 0%) bis max. +-20% des eingestellten
    // Werts (Tiefe-Regler 100%). Default 50% = +-10%.
    static constexpr auto ID_TIMEWARP_DEPTH   = "timewarpDepth";
    static constexpr auto ID_DIMENSION_DEPTH  = "dimensionDepth";
    static constexpr auto ID_HYPERDRIVE_DEPTH = "hyperdriveDepth";
    static constexpr auto ID_GALAXY_DEPTH     = "galaxyDepth";
    static constexpr auto ID_POSITION_DEPTH   = "positionDepth";
    static float modDepthCurve (float knob01) noexcept;

    // App-weite (nicht Projekt-/Preset-gebundene) Ablage fuer persistente
    // Einstellungen - Fenstergroesse (SAVE-Button), Standard-Zustand und
    // Sichtbarkeits-Defaults aus dem neuen Preset-/Hamburger-Menue (siehe
    // PluginEditor::showPresetMenu()). EINE gemeinsame Stelle fuer Processor
    // UND Editor, damit beide garantiert dieselbe Datei referenzieren.
    static juce::PropertiesFile::Options appPropertiesOptions();

    // --- Position (neue Sektion, ganz am Ende der Kette) --------------------
    // Offset = finales Pan, Width = nochmalige, globale Verschmaelerung/
    // Verbreiterung (unabhaengig vom Side Width in DIMENSION), Distance =
    // Naehe/Ferne-Illusion (HF-Daempfung + leichte Pegelabsenkung), Elevate =
    // rein lineare EQ-Anhebung/-Absenkung um eine Elevation zu suggerieren
    // (kein Exciter/Saettigung - siehe Chat).
    static constexpr auto ID_POS_ON       = "posOn";
    static constexpr auto ID_POS_OFFSET   = "posOffset";
    static constexpr auto ID_POS_WIDTH    = "posWidth";
    static constexpr auto ID_POS_DISTANCE = "posDistance";
    static constexpr auto ID_POS_ELEVATE  = "posElevate";
    // DEPTH loest Distance und Elevate als EIN bipolarer Regler ab:
    // links fern und dunkel, rechts nah und offen. Die beiden alten
    // Parameter bleiben als Rechenweg bestehen, sind aber nicht mehr
    // bedienbar.
    static constexpr auto ID_DEPTH        = "depth";

    // ===== PRISM: frequenzselektive Verbreiterung =====
    // Begrenzt, IN WELCHEM Frequenzbereich die Seitenbearbeitung ueberhaupt
    // wirkt. Ausdruecklich KEIN "Bass Protect"/Mono-Safe-Schalter (User-
    // Korrektur: bei Backing Vocals liegt unter 200 Hz ohnehin nichts) -
    // sondern eine Zielauswahl: "nur zwischen 200 und 800 Hz breit machen"
    // oder "nur ab 3 kHz aufwaerts". Mono-Sicherheit faellt als Nebeneffekt
    // ab, ist aber nicht der Zweck.
    // Aus = voller Bereich = exakt das bisherige Verhalten (wichtig fuer
    // bestehende Presets).
    static constexpr auto ID_PRISM_ON = "prismOn";
    // Filter-Bypass pro Sektion (User, Runde 26). Der Filter liegt jetzt
    // standardmaessig VOR Galaxy und Dimension; diese beiden Schalter nehmen
    // die jeweilige Sektion wieder heraus. Bei voller Bandbreite ist der
    // Filterpfad ohnehin uebersprungen, hoerbar wird der Unterschied also
    // erst, sobald man das Band enger zieht.
    static constexpr auto ID_PRISM_GALAXY = "prismGalaxy";     // true = Galaxy am Filter vorbei
    static constexpr auto ID_PRISM_DIM    = "prismDimBypass";  // true = Dimension am Filter vorbei
    static constexpr auto ID_PRISM_VIS    = "prismVisBypass";  // true = Vision am Filter vorbei
    // WING: sanfte Neigung des Seitensignals. 0 = flach, 1 = oben mehr,
    // 2 = unten mehr. Bewusst drei feste Stufen statt eines Reglers - der
    // nutzbare Bereich ist schmal (User).
    // Auto Gain: gleicht den Pegelunterschied aus, den die eigene Bearbeitung
    // verursacht - damit ein Bypass-Vergleich ehrlich wird (User: "oft schwer
    // zu beurteilen ob das Signal jetzt besser oder nur lauter ist").
    static constexpr auto ID_AUTO_GAIN    = "autoGain";
    static constexpr auto ID_BASS_GUARD   = "bassGuard";
    static constexpr auto ID_PRISM_LO = "prismLo";
    static constexpr auto ID_PRISM_HI = "prismHi";

    // Mono-Check: reines Monitoring-Utility (kein eigener "Section"-Charakter),
    // Icon sitzt trotzdem in der Position-Sektion.
    static constexpr auto ID_MONO_CHECK   = "monoCheck";
    // A/B-Vergleich fuer den Mono-Check: hoert statt des bearbeiteten
    // Signals das ROHE Eingangssignal (vor jeglicher Verarbeitung) in Mono -
    // "wie klaenge Mono ohne dieses Plugin". Nur sinnvoll (und in der GUI
    // nur bedienbar), waehrend Mono-Check selbst aktiv ist.
    static constexpr auto ID_MONO_DRY     = "monoDry";

    // Ganz simpler Ausgangs-Trim (+-6dB, kein Wertetext) - allerletzte
    // Gain-Stufe der gesamten Kette, sitzt in der GUI ueber dem Mono-Icon.
    static constexpr auto ID_VOL_TRIM     = "volTrim";
    // Runde 74 (User): Balance ganz am Ende der Kette. Gedacht zum Pruefen,
    // ob 3D und DRIFT auch dann noch eigenstaendig klingen, wenn man ihren
    // Versatz wieder in die Mitte zieht - deshalb bewusst eine BALANCE
    // (die gehaltene Seite bleibt unveraendert laut) und kein Pan.
    static constexpr auto ID_OUT_PAN      = "outPan";
    // Globaler MIX (0..100 %): Menge von Dimension, Hyperdrive, Vision und
    // RAYE gegen den Abgriff nach Galaxy+Timewarp (siehe processBlock,
    // "MIX-Abgriff"). Bewusst KEIN Dry/Wet ueber die ganze Kette: das
    // erzeugte mit Timewarp einen Kammfilter.
    static constexpr auto ID_MIX          = "mix";

    // Sektions-Solo (exklusiv wie Radio-Buttons): schaltet alle ANDEREN
    // Sektionen kurzzeitig auf trocken/dry, unabhaengig von deren eigenem
    // Power-Icon-Status - nutzt dieselben weichen On/Off-Gains, daher immer
    // klickfrei. "Galaxy" (LCR) darf mitmachen; das kann beim Rein-/
    // Rausschalten die gemeldete Latenz aendern (PDC-Sprung), genau wie schon
    // beim manuellen Umschalten des LCR-Power-Icons selbst - kein neues
    // Verhalten, nur automatisiert durch Solo.
    // Globaler Mod-Bypass (User-Idee "Globale Buttons"): deaktiviert ALLE
    // drei LFO-Modulationen (Timewarp/Dimension/Hyperdrive) auf einen Schlag,
    // OHNE die einzelnen Mod-Icons selbst zu veraendern - beim Ausschalten
    // springen alle 3 wieder genau dorthin zurueck, wo sie vorher standen.
    static constexpr auto ID_GLOBAL_MOD_BYPASS = "globalModBypass";
    // LIFE (Runde 37): globaler Regler, skaliert ALLE Modulationstiefen
    // gemeinsam (0..100 %, Default 100 = wie bisher).
    static constexpr auto ID_LIFE = "life";

    static constexpr auto ID_SOLO_SECTION = "soloSection";
    static constexpr int SOLO_NONE       = 0;
    static constexpr int SOLO_GALAXY     = 1;
    static constexpr int SOLO_TIMEWARP   = 2;
    static constexpr int SOLO_POLARITY   = 3;
    static constexpr int SOLO_DIMENSION  = 4;
    static constexpr int SOLO_HYPERDRIVE = 5;
    static constexpr int SOLO_POSITION   = 6;
    static constexpr int SOLO_RAY        = 7;
    // Hoechster Solo-Index - Editor rechnet damit die 0..1-Normalisierung
    // (war vorher als 6 hart codiert, siehe Bug "Ray solo schaltet Position").
    static constexpr int SOLO_MAX        = SOLO_RAY;

    // ===== RAY: Stereo-Phaser =====
    // Neue, bewusst sehr einfache Sektion (User: "Phaser - 3 Click Icon...
    // einfacher als die anderen Effekte"). Vier Allpass-Stufen je Kanal, die
    // eine LFO-gesteuerte Kerbenfolge durchs Spektrum schieben - klassischer
    // Phaser, aber mit einer raeumlichen Besonderheit: der rechte Kanal
    // laeuft dem linken um eine Viertelperiode voraus. Die Kerben wandern
    // dadurch nicht gleichzeitig durch beide Seiten, sondern kreisen -
    // das Bild bekommt eine Drehbewegung statt eines Wobbelns. Genau das
    // ist der Grund, warum ein Phaser in ein Imaging-Plugin gehoert.
    //
    //  - ID_RAY_STRENGTH: 3 Stufen (0..2 = leicht/mittel/stark). Skaliert
    //    NICHT nur den Wet-Anteil (das machte den Effekt nur leiser, nicht
    //    staerker), sondern gemeinsam Sweep-Breite, Rueckkopplung und Anteil.
    //  - ID_RAY_RATE: eigene Geschwindigkeit in Hz.
    //  - ID_RAY_PAIR: koppelt den LFO an den Auto-Pan-LFO von Hyperdrive
    //    (User-Idee "Pair"). Dann bestimmt Hyperdrive inkl. Sync die Rate,
    //    der eigene Speed-Regler ist inaktiv. Keine eigene Sync-Logik noetig.
    // Kein Mod-Icon: das Staerke-Icon IST bereits die Tiefe.
    static constexpr auto ID_RAY_ON       = "rayOn";
    static constexpr auto ID_RAY_STRENGTH = "rayStrength";
    static constexpr auto ID_RAY_RATE     = "rayRate";
    static constexpr auto ID_RAY_PAIR     = "rayPair";
    // Runde 49: Speed-Regler ist in SpaceXraye raus - FAST legt pauschal
    // 30 % auf das Charakter-Tempo drauf.
    static constexpr auto ID_RAY_FAST     = "rayFast";
    static constexpr auto ID_RAY_AMOUNT   = "rayAmount";   // SpaceXraye
    static constexpr auto ID_RAY_CHAR     = "rayCharacter"; // SpaceXraye
    struct RayCharacter { float centreHz, sweepMul, fbMul, mixMul, stereoOffset, rateMul; };
    static const RayCharacter& rayCharacterFor (int index) noexcept;

    // PARALLAX neu (Runde 34, User: "1 Regler + 4 Buttons"): Modus waehlt
    // eine feste Einstellung, Amount skaliert sie. Vorlaeufig schreibt die
    // GUI daraus Drift/Shift - die echten Werte der vier Modi liefert der
    // User noch (MicroPitch / altes Parallax).
    static constexpr auto ID_PARALLAX_MODE   = "parallaxMode";
    static constexpr auto ID_PARALLAX_AMOUNT = "parallaxAmount";
    // Runde 44: Parallax-Modi als WEGPUNKTE. Amount faehrt der Reihe nach
    // von Punkt zu Punkt (gleichmaessig verteilt). amountIsMix: nur ein
    // Punkt, Amount ist dann der Parallax-Mix (0..100 %).
    struct ParallaxPoint { float driftPct, bendCt, tiltPct, mixPct, gainDb; };
    // Runde 45: Amount 0 % ist IMMER "alles auf 0" (User). amountIsMix: die
    // Einstellung steht fest, Amount dreht nur den Mix von 0 bis zum Wert des
    // Presets (nie darueber). Sonst: Amount faehrt von Null ueber die Punkte.
    static constexpr int kParallaxModes = 6;
    struct ParallaxModeDef { int numPoints; bool amountIsMix; bool balance; float amountMax; ParallaxPoint pts[4]; };
    static const ParallaxModeDef& parallaxModeDef (int mode) noexcept;
    static ParallaxPoint evalParallaxMode (int mode, float amount01) noexcept;
    static bool parallaxModeBalance (int mode) noexcept { return parallaxModeDef (mode).balance; }


    // Aktueller LFO-Wert des Phasers (-1..1, 0 wenn aus) fuer das Starfield
    // (goldene Linien, die mit dem Phaser mitschwingen).
    std::atomic<float> currentRayLfo { 0.0f };

    // Werks-Zustand: Schnappschuss des APVTS direkt nach dem Bau, BEVOR ein
    // gespeicherter Standard (defaultPluginState) angewendet wird. Der Editor
    // baut daraus das eingebaute Preset "Default" (siehe defaultPresetTree()).
    juce::ValueTree factoryState;

    // Section-Lock (User-Wunsch: "Sections ausschliessen" von Chaos/Mutate
    // und Breathe, Variante "Lock Icon pro Section") - eine gesperrte
    // Sektion wird von beiden komplett ignoriert. Kein eigener APVTS-
    // Parameter (kein automatisierbarer Zustand, keine Verwirrung als
    // moegliches Chaos/Breathe-Randomisierungsziel), sondern eine einfache
    // Property direkt auf dem APVTS-State-Baum - wird dadurch automatisch
    // mit dem Plugin-/Projekt-Zustand gespeichert/geladen (siehe get-/
    // setStateInformation(), die denselben Baum serialisieren).
    bool isSectionLocked (int soloSectionId) const;
    void setSectionLocked (int soloSectionId, bool locked);

    // 0..100% -> ms, mit 80% der Reglerbewegung auf 0-2ms und den
    // restlichen 20% auf 2-20ms (der Haas-Effekt ist zwischen 0-2ms am
    // staerksten spuerbar, siehe Aenderungsliste Revision 2).
    static float driftPercentToMs (float absPercent) noexcept;
    // Exakte Umkehrfunktion von driftPercentToMs() - fuer die Live-Anzeige
    // auf dem Drift-Regler (der in Prozent skaliert ist, waehrend intern in
    // ms moduliert wird).
    static float driftMsToPercent (float ms) noexcept;
    static float rayStrengthToDepth (float strengthIndex) noexcept;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Reicht das Signal um lastReportedLatency Samples verzoegert durch
    // (eigener Ringpuffer), damit die PDC-Kompensation des Hosts beim
    // Bypass (Host-Bypass ODER Logo-Klick-Bypass) korrekt bleibt.
    void passthroughWithLatencyCompensation (juce::AudioBuffer<float>& buffer);

    // Bug-Fix (User-Feedback: "Mono Check -> Goniometer und Corr Meter
    // sollen davon nicht beeinflusst sein. Auch nicht Mono Check + Bypass.")
    // Gemeinsame Stelle, die Goniometer-Ringpuffer + Korrelations-/Side-
    // Emphasis-Anzeige aus einem fertigen Stereo-Block fuellt - wird jetzt
    // sowohl vom normalen Verarbeitungspfad (mit dem Signal VOR Mono-Check,
    // aber NACH Vol-Trim/Chaos-Duck) als auch vom Bypass-Pfad
    // (passthroughWithLatencyCompensation) aufgerufen, damit die visuellen
    // Anzeigen bei Bypass nicht einfrieren.
    void updateVisualMeters (const float* leftBuf, const float* rightBuf, int numSamples);

    ChannelDelayLine delayL, delayR;

    SimplePitchShifter bendL, bendR;
    float lastBendForRatio = -1.0e9f;   // Cache fuer setRatio (Runde 38)
    // Parallax-Modi (Runde 44): eigener Mix, Pegelausgleich und Tilt INNERHALB
    // der Parallax-Stufe (entspricht dem globalen Mix, mit dem die Punkte
    // eingestellt wurden).
    juce::SmoothedValue<float> pxMixSmoothed, pxGainSmoothed, pxTiltLSmoothed, pxTiltRSmoothed;
    std::atomic<float>* pParallaxMode = nullptr;
    std::atomic<float>* pParallaxAmount = nullptr;
    StereoSTFTExtractor lcrExtractor;

    std::atomic<float>* pGalaxyActivate = nullptr;
    std::atomic<float>* pLcrEnabled = nullptr;
    std::atomic<float>* pLcrSens = nullptr;
    std::atomic<float>* pLcrBlend = nullptr;
    std::atomic<float>* pPolL = nullptr;
    std::atomic<float>* pPolR = nullptr;
    std::atomic<float>* pPolPos = nullptr;
    std::atomic<float>* pTimewarpBalance = nullptr;
    std::atomic<float>* pDrift = nullptr;
    std::atomic<float>* pBend = nullptr;
    std::atomic<float>* pSideWidth = nullptr;
    std::atomic<float>* pSideBoost = nullptr;
    std::atomic<float>* pMovement = nullptr;
    std::atomic<float>* pSpeed = nullptr;
    std::atomic<float>* pSpeedRate = nullptr;
    std::atomic<float>* pSpeedSync = nullptr;
    std::atomic<float>* pPulse = nullptr;

    std::atomic<float>* pDriftOn = nullptr;
    std::atomic<float>* pPolOn = nullptr;
    std::atomic<float>* pWidthBoostOn = nullptr;
    std::atomic<float>* pFlowOn = nullptr;

    std::atomic<float>* pTimewarpMod = nullptr;
    std::atomic<float>* pDimensionMod = nullptr;
    std::atomic<float>* pHyperdriveMod = nullptr;
    std::atomic<float>* pTimewarpDepth = nullptr;
    std::atomic<float>* pDimensionDepth = nullptr;
    std::atomic<float>* pHyperdriveDepth = nullptr;
    std::atomic<float>* pGalaxyMod = nullptr;
    std::atomic<float>* pGalaxyDepth = nullptr;
    std::atomic<float>* pPositionMod = nullptr;
    std::atomic<float>* pPositionDepth = nullptr;

    std::atomic<float>* pRayOn = nullptr;
    std::atomic<float>* pRayStrength = nullptr;
    std::atomic<float>* pRayAmount = nullptr;
    std::atomic<float>* pRayChar = nullptr;
    std::atomic<float>* pRayRate = nullptr;
    std::atomic<float>* pRayPair = nullptr;
    std::atomic<float>* pRayFast = nullptr;


    // RAY (Phaser) - Zustand. Vier Allpass-Stufen erster Ordnung je Kanal
    // (ein Speicherwert pro Stufe), dazu Rueckkopplung und eigene LFO-Phase.
    // rayDepthSmoothed glaettet den Stufenwechsel des 3-Klick-Icons, damit
    // ein Klick keinen Sprung erzeugt; rayOnGain ist das Sektions-Gain mit
    // Solo-Beruecksichtigung wie bei allen anderen Sektionen.
    static constexpr int kRayStages = 4;
    float rayApL[kRayStages] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float rayApR[kRayStages] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float rayFbL = 0.0f, rayFbR = 0.0f;
    double rayPhase = 0.0;
    juce::SmoothedValue<float> rayOnGain, rayDepthSmoothed;
    // CPU-Build (Runde 41): RAYE-Koeffizienten im 8er-Raster, Tilt-Pan-Cache.
    float rayAL = 0.0f, rayAR = 0.0f, rayAStepL = 0.0f, rayAStepR = 0.0f;
    int   rayCoefCountdown = 0;
    bool  rayCoefValid = false;
    float offsetPanCachePos = -9.0f, offsetPanCacheL = 1.0f, offsetPanCacheR = 1.0f;
    juce::SmoothedValue<float> rayLifeSmoothed;   // LIFE skaliert auch RAYE (Runde 39)

    std::atomic<float>* pPosOn = nullptr;
    std::atomic<float>* pPosOffset = nullptr;
    std::atomic<float>* pPosWidth = nullptr;
    std::atomic<float>* pPrismOn = nullptr;
    std::atomic<float>* pPrismGalaxy = nullptr;
    std::atomic<float>* pPrismDim = nullptr;
    std::atomic<float>* pPrismVis = nullptr;
    std::atomic<float>* pAutoGain = nullptr;
    std::atomic<float>* pBassGuard = nullptr;
    std::atomic<float>* pHorizon = nullptr;
    std::atomic<float>* pDepth = nullptr;
    std::atomic<float>* pPrismLo = nullptr;
    std::atomic<float>* pPrismHi = nullptr;
    std::atomic<float>* pPosDistance = nullptr;
    std::atomic<float>* pPosElevate = nullptr;
    std::atomic<float>* pMonoCheck = nullptr;
    std::atomic<float>* pMonoDry = nullptr;
    std::atomic<float>* pSoloSection = nullptr;
    std::atomic<float>* pVolTrim = nullptr;
    std::atomic<float>* pOutPan  = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pGlobalModBypass = nullptr;
    std::atomic<float>* pLife = nullptr;

    // Per-Sample-Glaettung fuer alle Regler, die direkt als Gain/Multiplikator
    // in die Signalkette eingehen - verhindert Zipper-/Knisterrauschen, wenn
    // schnell am Regler gedreht wird (der Rohwert aendert sich sonst nur
    // einmal pro Block, was bei schnellen Bewegungen wie eine Treppe klingt).
    juce::SmoothedValue<float> sensSmoothed, blendSmoothed, widthSmoothed, boostSmoothed, movementSmoothed, bendSmoothed;
    juce::SmoothedValue<float> offsetSmoothed, posWidthSmoothed, distanceSmoothed, elevateSmoothed;
    juce::SmoothedValue<float> volTrimSmoothed;
    juce::SmoothedValue<float> outPanLGain, outPanRGain;
    juce::SmoothedValue<float> mixSmoothed;

    // Section-On/Off als weich ein-/ausgeblendete Gains (0..1) statt harter
    // Verzweigung - sonst gibt es beim Umschalten ein Knacksen, weil das
    // Signal zwischen "trocken" und "bearbeitet" einen Sprung macht.
    juce::SmoothedValue<float> driftOnGain, polOnGain, widthBoostOnGain, flowOnGain, posOnGain;
    // Polarity klickfrei: L/R-Flip und die vier Slots (1-4) als geglaettete
    // Gains statt harter Schalter (siehe processBlock).
    juce::SmoothedValue<float> polLFlip, polRFlip, polSlotGain[4];
    // Weicher Crossfade zwischen dem latenzgleich verzoegerten Original
    // ("dry", 0) und dem verarbeiteten Galaxy/LCR-Ergebnis ("wet", 1) -
    // ermoeglicht den neuen, klickfreien Sektions-Bypass OHNE dass sich
    // dabei die gemeldete Latenz aendert (siehe ID_GALAXY_ACTIVATE-
    // Kommentar im Header sowie processBlock()).
    juce::SmoothedValue<float> lcrWetGain;
    // Runde 37 (CPU): ist die Galaxy-Sektion aus (und ausgeblendet), pausiert
    // die FFT; nur die Dry-Verzoegerung laeuft weiter -> Latenz bleibt gleich.
    // Beim Einschalten laeuft die Engine erst unhoerbar an (Warm-up), dann
    // wird weich eingeblendet.
    bool galaxyEnginePaused = false;
    int  galaxyWarmupRemaining = 0;
    // 0..1 - wie stark die "Balance"-Gain-Kompensation (siehe
    // ID_TIMEWARP_BALANCE) gerade eingeblendet ist, weich statt hart
    // schaltend, damit das Icon klickfrei an/aus geht.
    juce::SmoothedValue<float> balanceOnGain;
    // Eigene, latenzgleiche Verzoegerungsleitung fuer das "dry"-Signal der
    // Galaxy-Sektion (analog zu bypassDelayL/R weiter unten, aber nur fuer
    // die LCR-Stufe statt fuer das gesamte Plugin).
    std::vector<float> lcrDryDelayL, lcrDryDelayR;
    int lcrDryWritePos = 0;
    // Mono-Check ist kein "Section"-Gain (nicht Solo-betroffen), aber
    // ebenfalls weich, damit das Umschalten klickfrei ist.
    juce::SmoothedValue<float> monoCheckGain;
    // Weicher Crossfade zwischen "bearbeitetes Signal in Mono" (0) und
    // "rohes Dry-Signal in Mono" (1) fuer den A/B-Vergleich, siehe ID_MONO_DRY.
    juce::SmoothedValue<float> monoDryBlend;

    // "Chaos"-Button (GUI-Aktion, kein APVTS-Parameter - analog zu "Breathe"):
    // wuerfelt praktisch alle Regler/Mods neu. User-Sorge: "Angst wegen Vol
    // Jumps" bei so einer Rundum-Randomisierung - Loesung: der Gesamt-Output
    // wird kurz geduckt (ca. 60ms bei ca. -9dB), waehrend die neuen Werte
    // einlaufen, und blendet danach weich (ca. 350ms) wieder auf 0dB zurueck,
    // statt dass die neue Zufallskombination als harter Lautstaerke-Sprung
    // hoerbar wird. Der Trigger selbst (chaosTriggerRequested) ist weiter
    // oben im PUBLIC Bereich deklariert, da der Editor ihn direkt setzt.
    juce::SmoothedValue<float> chaosDuckGain;

    // Demo-Modus (siehe licensed/demoDuck oben): Zaehler und aktuelle
    // Absenkung. Nur im Audio-Thread angefasst.
    double demoPhaseSec = 0.0;
    float  demoGain     = 1.0f;

    int chaosDuckHoldSamplesRemaining = 0;

    // Einfaches Direct-Form-I-Biquad fuer den "Elevate"-EQ-Trick (rein
    // linear, kein Waveshaping/Saettigung - siehe Chat-Erklaerung). Die
    // Koeffizienten werden nur einmal pro Block neu berechnet (kein
    // Audio-Rate-Modulationsziel), der Zustand (z1/z2) bleibt pro Kanal.
    struct BiquadCoeffs { float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f; };
    struct BiquadState
    {
        float z1 = 0.0f, z2 = 0.0f;
        float process (float x, const BiquadCoeffs& c) noexcept
        {
            float y = c.b0 * x + z1;
            z1 = c.b1 * x + z2 - c.a1 * y;
            z2 = c.b2 * x - c.a2 * y;
            return y;
        }
    };
    BiquadCoeffs elevateCoeffs;
    BiquadState elevateStateL, elevateStateR;
    static void updatePeakingCoeffs (BiquadCoeffs& c, double sampleRate, float freqHz, float gainDb, float q) noexcept;

    // ===== PRISM-Filter =====
    // Butterworth 2. Ordnung (12 dB/Okt), Hoch- und Tiefpass, bilden zusammen
    // das Band, in dem die Verbreiterung wirkt.
    static void updateHighpassCoeffs (BiquadCoeffs& c, double sampleRate, float freqHz) noexcept;
    static void updateHighShelfCoeffs (BiquadCoeffs& c, double sampleRate, float freqHz, float gainDb) noexcept;
    static void updateLowpassCoeffs  (BiquadCoeffs& c, double sampleRate, float freqHz) noexcept;

    BiquadCoeffs prismHpCoeffs, prismLpCoeffs;

    // ===== BASS-GUARD =====
    // Unterhalb von 120 Hz bleibt das Stereobild unangetastet - fest
    // verdrahtet, ohne Bedienelement. Begruendung: Verbreitern und LCR im
    // Bass ruinieren die Monokompatibilitaet und machen den Bass schwammig.
    // Das ist keine Geschmacksfrage, sondern Korrektheit, und gute Werkzeuge
    // erzwingen Korrektheit still. Niemand will einen "Zerstoer meinen Bass
    // nicht"-Schalter, alle wollen ein Plugin, das ihren Bass nicht zerstoert.
    //
    // Technisch immer als "Original + Hochpass(Aenderung)": unterhalb der
    // Grenze steht damit exakt das Eingangssignal, es entstehen keine
    // Kammfilter, und bei neutralen Einstellungen bleibt alles bitgenau.
    BiquadCoeffs bassGuardCoeffs;
    BiquadState  bassGuardDim;   // nur noch Mid/Side - Galaxy filtert in der FFT
    // ===== AUTO GAIN =====
    // Gemessen wird K-gewichtet (vereinfachtes ITU-R BS.1770: Hochpass gegen
    // den Bassueberschuss, Hoehenschelf fuer die Ohrkurve) auf der Monosumme.
    // Grund fuer die Gewichtung: das Plugin veraendert vor allem die
    // Seitenanteile, und Seitenenergie hebt den nackten RMS staerker als die
    // empfundene Lautheit - ohne Gewichtung wuerde Auto Gain beim
    // Breitmachen zu viel wegnehmen und es klaenge kraftlos.
    //
    // KEINE Latenz: gemessen wird rueckwaerts, kein Lookahead. Der Preis ist
    // Reaktionszeit, und die ist ausdruecklich gewollt - eine schnelle
    // Regelung waere ein Kompressor und wuerde die Dynamik veraendern.
    BiquadCoeffs kwHpCoeffs, kwShelfCoeffs;
    // JE KANAL, nicht auf der Monosumme. Der erste Anlauf hat 0,5*(L+R)
    // gemessen - bei einem Polarity-Flip oder stark seitigem Material loescht
    // sich diese Summe weitgehend aus, die Regelung hielt den Ausgang fuer
    // fast still und drehte bis zum Anschlag auf (User: "der reale Pegel auf
    // der Spur wird bis zu 12 dB hoeher"). Richtig ist, was der
    // Loudness-Standard macht: jeden Kanal einzeln gewichten und die
    // Energien addieren. Ein Polarity-Flip aendert die Messung dann gar
    // nicht - und genau das ist richtig, er macht nichts lauter.
    BiquadState  kwInHpL, kwInShelfL, kwInHpR, kwInShelfR;
    BiquadState  kwOutHpL, kwOutShelfL, kwOutHpR, kwOutShelfR;

    // Die Eingangsmessung wird um die gemeldete Latenz verzoegert, damit
    // wirklich derselbe Moment verglichen wird. Ohne das schwankte der Wert
    // bei aktivem Galaxy um bis zu 3 dB, obwohl gar nichts eingestellt war.
    std::vector<float> autoGainInDelay;
    int autoGainInPos = 0;

    double autoGainInSq = 0.0, autoGainOutSq = 0.0;
    float  autoGainTarget  = 1.0f;   // gehaltener Wert
    float  autoGainApplied = 1.0f;   // zuletzt tatsaechlich angewandt

    // ===== Messfenster statt Dauerbetrieb =====
    // Das Verhaeltnis Ein/Aus haengt nicht nur an den Einstellungen, sondern
    // auch am Material - in der Strophe anders als im Refrain. Wer das
    // dauernd nachregelt, baut einen Level Rider und veraendert die Dynamik
    // des Songs (User: "auto gain rided das signal wie ein gain rider.
    // Autsch!"). Deshalb: gemessen wird nur, solange sich Parameter bewegen,
    // plus eine kurze Nachlaufzeit. Danach steht der Wert fest.
    int   autoGainMeasureSamples = 0;
    float autoGainParamSum = -1.0e9f;   // Pruefsumme ueber alle Parameter
    // ZWEI getrennte Filterzustaende, weil die Verbreiterung an zwei
    // verschiedenen Stellen der Kette passiert (Dimension mit Size/Boost und
    // danach Position mit Width) und dort jeweils ein ANDERES Side-Signal
    // anliegt. Ein gemeinsamer Zustand wuerde die beiden Signale vermischen.
    BiquadState prismDimHp, prismDimLp;
    BiquadState prismPosHp, prismPosLp;
    // Dritte Stelle: die Galaxy-Differenz (wet minus dry), pro Kanal.
    BiquadState prismGalHpL, prismGalLpL, prismGalHpR, prismGalLpR;
    // Pro Block gesetzt: ist PRISM aus oder umfasst das Band ohnehin fast
    // alles, wird der Filterpfad komplett uebersprungen (kein CPU, und
    // bitgenau dasselbe Ergebnis wie vorher).
    bool prismActive = false;
    bool prismGalaxyActive = false;
    bool prismDimActive = false;
    bool prismVisActive = false;
    // WING-Neigung: ein Ein-Pol-Tiefpass teilt das Seitensignal bei ~700 Hz,
    // beide Haelften bekommen gegenlaeufige Gains. Bei gleichen Gains ergibt
    // die Summe wieder exakt das Original - deshalb ist "flach" bitgenau aus.

    // ===== BYPASS =====
    // Im vollen Bypass kehrt processBlock() frueh um und die gesamte
    // Verarbeitung steht still. Filter, Verzoegerungsleitungen und die
    // STFT-FIFO behalten dabei ihren letzten Inhalt - beim Einschalten
    // entlaedt sich das als Knacken (User: "klingt als sei was im Cache was
    // sich entladen wuerde"). Deshalb wird beim Verlassen des Bypass alles
    // Zustandsbehaftete geleert; die 25-ms-Ueberblendung deckt den Neustart.
    bool wasFullyBypassed = false;
    void clearProcessingState() noexcept;
    // Nur die DSP-Zustaende (Delays, Filter, Phaser, STFT) - OHNE Auto Gain.
    // Wird nach laengerer Stille aufgerufen; Auto Gain soll dabei seinen
    // Wert behalten, sonst pumpt der Pegel beim Weiterspielen.
    void clearDspTails() noexcept;
    double silentSeconds = 0.0;
    bool   silenceCleared = false;

    // One-Pole-Tiefpass fuer "Distance" (Naehe/Ferne) - Koeffizient ebenfalls
    // nur einmal pro Block neu berechnet.
    float distanceLpfL = 0.0f, distanceLpfR = 0.0f;

    // Stark geglaetteter Korrelationswert (exponentieller Moving Average
    // ueber die Bloecke hinweg) - der rohe Pro-Block-Wert zappelt deutlich
    // zu stark fuer eine ruhige Anzeige (siehe User-Feedback). Der Attack-
    // Koeffizient wird in prepareToPlay() aus der Samplerate/Blockgroesse
    // abgeleitet, damit die Zeitkonstante unabhaengig vom Host-Blocksize ist.
    float correlationSmooth = 0.0f;

    // Analog geglaetteter Seiten-Betonungswert (EMA, gleicher Ansatz wie
    // correlationSmooth) - reines Anzeige-/Deko-Signal fuer die Sterne, hat
    // keinerlei Einfluss auf die eigentliche Audioverarbeitung.
    float sideEmphasisSmooth = 0.0f;

    double currentSampleRate = 44100.0;
    int lastReportedLatency = -1;

    // Auto-Pan LFO Phase
    double autoPanPhase = 0.0;

    // Mod-LFO-Phasen (jeweils eigene, unabhaengige Phase, ~8s Zykluslaenge,
    // damit sich Timewarp/Dimension/Hyperdrive nicht synchron/mechanisch
    // anfuehlen, falls mehrere gleichzeitig aktiv sind).
    double timewarpModPhase = 0.0;
    double dimensionModPhase = 0.0;
    double hyperdriveModPhase = 0.0;
    double galaxyModPhase = 0.0;
    double positionModPhase = 0.0;
    static constexpr double kModLfoPeriodSeconds = 8.0;

    // Puls-Glaettung (Pulse-Modus): One-Pole-Nachfuehrung Richtung
    // Rechteck-Zielwert, damit der Wechsel hart-aber-nicht-100%-hart wirkt.
    float pulseSmoothState = 0.0f;
    float pulseSmoothCoeff = 0.0f;

    // Eigener, von den Drift-Delaylines unabhaengiger Ringpuffer, nur fuer
    // processBlockBypassed (Latenzkompensation beim Host-Bypass). Groesse
    // entspricht exakt lcrExtractor.getLatencySamples().
    std::vector<float> bypassDelayL, bypassDelayR;
    int bypassWritePos = 0;

    // Weicher Bypass (siehe getBypassParameter): Blend 0 = Wet, 1 = Original;
    // bypassDryL/R = latenzgleiches Original des aktuellen Blocks.
    juce::AudioParameterBool* hostBypassParam = nullptr;
    juce::SmoothedValue<float> bypassBlend;
    std::vector<float> bypassDryL, bypassDryR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LCRMSAudioProcessor)
};
