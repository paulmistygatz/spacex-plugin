#ifndef SPACEX_TUNE
 #define SPACEX_TUNE 0
#endif
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SpaceXManualData.h"   // eingebettetes Handbuch, siehe openManual()
#include "SpaceXPresetData.h"   // Runde 133: Werks-Presets, siehe presetFolder()
#include "GUI/SpaceAssets.h"

namespace
{
    void styleRotary (juce::Slider& s, bool withTextBox = false)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        if (withTextBox)
        {
            s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
            s.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        }
        else
        {
            s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        }
    }

    // Kleine, moderne "ENHANCER"-Style-Typografie: kompakt, fett, mit
    // leichtem zusaetzlichen Zeichenabstand statt reiner Systemschrift.
    juce::Font paramFont()  { return juce::Font (juce::FontOptions (12.0f, juce::Font::bold)).withExtraKerningFactor (0.06f); }
    juce::Font titleFont()  { return juce::Font (juce::FontOptions (13.0f, juce::Font::bold)).withExtraKerningFactor (0.12f); }

    // ===== KONTAKTDATEN FUER DAS BACK PANEL =====
    // Alles an EINER Stelle. Der QR-Code zeigt auf die lnk.bio-Seite, nicht
    // auf eine einzelne Adresse - eine Seite, die Paul selbst pflegen kann,
    // ohne dass ein neues Plugin gebaut werden muss. Genau deshalb steht sie
    // hier und nicht die Einzellinks.
    namespace spacexContact
    {
        inline constexpr const char* email       = "info@paulmisty.com";
        inline constexpr const char* website     = "paulmisty.com";
        inline constexpr const char* websiteUrl  = "https://www.paulmisty.com/";
        inline constexpr const char* instagram   = "@paulmisty.studio";
        inline constexpr const char* instagramUrl= "https://www.instagram.com/paulmisty.studio/";
        inline constexpr const char* linksUrl    = "https://lnk.bio/paulmisty";
        // Hier wird SpaceX verkauft - der DEMO-Aufkleber fuehrt direkt dorthin.
        inline constexpr const char* shopUrl     = "https://ko-fi.com/paulmisty";
        inline constexpr const char* designer    = "Paul Misty";
        inline constexpr const char* thanks      = "Everyone who tested SpaceX and sent feedback";
    }

    void styleLabel (juce::Label& l, const juce::String& text)
    {
        // Runde 53 (User: "alles in Grossbuchstaben oder Normal?"): normal.
        // Versalien lesen sich in kleinen Groessen schlechter - die Hierarchie
        // macht jetzt allein der Sektionstitel (der bleibt in Versalien).
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (paramFont());
        // Bug-Fix (User: "MONO Icon + MONO Schrift sieht gestaucht aus"):
        // JUCE-Labels stauchen Text horizontal, wenn er nicht in die Breite
        // passt (minimumHorizontalScale < 1). Das war die "gequetschte"
        // Schrift bei den schmalen Footer-Labels. Nie mehr stauchen - lieber
        // die Labelflaeche breiter machen (siehe Footer-Layout).
        // Bug-Fix (User: "unten in den beiden Sections sind die Schriften
        // abgeschnitten" - DIST..., ELEV..., SPE...): 1.0 fuer ALLE Labels
        // war zu hart - die Regler-Labels in den schmalen Sektionen brauchen
        // etwas Spielraum. Regler-Labels duerfen minimal stauchen (0.88, kaum
        // sichtbar), die Footer-Labels bleiben auf 1.0 (siehe dort). Dazu
        // faellt der 5px-Standard-Innenrand links/rechts weg, der bei
        // zentriertem Text nur Platz frisst.
        l.setMinimumHorizontalScale (0.88f);
        l.setBorderSize (juce::BorderSize<int> (1, 0, 1, 0));
    }

    // Sektions-Titel oben links in jedem Rahmen, in der Rahmenfarbe gehalten.
    void styleTitle (juce::Label& l, const juce::String& text, juce::Colour colour)
    {
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centredLeft);
        l.setColour (juce::Label::textColourId, colour);
        l.setFont (titleFont());
        // DAS war das Abschneiden (Runde 54): ein juce::Label hat von Haus aus
        // 5 px Innenrand links UND rechts. fitTitle() rechnet aber mit der
        // reinen Textbreite - die Beschriftung bekam also 10 px weniger Platz,
        // als sie braucht, und JUCE kuerzte mit "...". Kein Innenrand mehr,
        // und gestaucht wird auch nicht (siehe fitTitle).
        l.setBorderSize (juce::BorderSize<int> (0));
        l.setMinimumHorizontalScale (1.0f);
    }
}

void LCRMSAudioProcessorEditor::setupPowerButton (juce::TextButton& button, const juce::String& paramId,
                                                   std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& attachment,
                                                   int soloValue)
{
    button.setClickingTogglesState (true);
    button.getProperties().set ("powerIcon", true);
    // Verhindert einen vom Betriebssystem/JUCE gezeichneten Fokus-Rahmen um
    // das Icon (sah wie ein zusaetzliches "Kaestchen" um den Button aus) -
    // nur der reine Power-Icon-Kreis soll zu sehen sein.
    button.setWantsKeyboardFocus (false);
    content.addAndMakeVisible (button);
    attachment = std::make_unique<ButtonAttachment> (processor.apvts, paramId, button);

    // Symmetrisch zur "Solo schaltet dauerhaft an"-Regel: schaltet man eine
    // solote Sektion manuell per Power-Icon aus, geht Solo automatisch mit
    // auf "None" zurueck - eine Sektion kann nie solo+off gleichzeitig sein
    // (User-Bestaetigung).
    if (soloValue != LCRMSAudioProcessor::SOLO_NONE || paramId == LCRMSAudioProcessor::ID_LCR_ENABLED)
    {
        button.onClick = [this, &button, soloValue, paramId]
        {
            // onClick feuert NACH dem Umschalten des Toggle-Status - nur bei
            // AUSschalten (nicht beim Einschalten) soll Solo mit zurueckgesetzt
            // werden.
            // Solo bleibt beim Ausschalten stehen (User: "Sektion weiterhin
            // solo lassen, aber bypassed") - hoerbar ist dann das trockene
            // Signal, und beim Wiedereinschalten ist die Sektion sofort
            // wieder allein zu hoeren.
            // Bug-Fix (User-Feedback): Galaxy-Sektion per Power-Icon
            // einzuschalten, waehrend Activate Galaxy global aus ist, hatte
            // keine hoerbare Wirkung - jetzt automatisch mit aktiviert (siehe
            // activateGalaxyIfNeeded()-Kommentar im Header).
            if (button.getToggleState() && paramId == LCRMSAudioProcessor::ID_LCR_ENABLED)
                activateGalaxyIfNeeded();
            juce::ignoreUnused (soloValue);
        };
    }
}

// Solo-Icons sind KEIN normaler Bool-Parameter, sondern schreiben den
// gemeinsamen Choice-Parameter ID_SOLO_SECTION - exklusiv wie ein
// Radio-Button, aber erneutes Klicken auf das bereits aktive Solo schaltet
// wieder auf "None" zurueck (kein Solo mehr aktiv).
void LCRMSAudioProcessorEditor::setupSoloButton (juce::TextButton& button, int soloValue)
{
    button.setClickingTogglesState (false); // Toggle-Status kommt aus dem geteilten Parameter, nicht lokal
    button.getProperties().set ("soloIcon", true);
    button.setWantsKeyboardFocus (false);
    // Runde 30: unsichtbar (siehe soloSection im Prozessor). Knopf und Logik
    // bleiben unveraendert bestehen, damit es keine zweite Buchfuehrung fuer
    // denselben Zustand gibt - er nimmt nur keinen Platz mehr ein.
    content.addChildComponent (button);
    button.setVisible (false);
    button.setEnabled (false);
    button.onClick = [this, soloValue]
    {
        if (auto* param = processor.apvts.getParameter (LCRMSAudioProcessor::ID_SOLO_SECTION))
        {
            const int current = juce::jlimit (0, LCRMSAudioProcessor::SOLO_MAX, (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_SOLO_SECTION)->load()));
            const int next = (current == soloValue) ? LCRMSAudioProcessor::SOLO_NONE : soloValue;
            // Bug-Fix (User: "ray solo schaltet Position solo ein"): der Teiler
            // war mit 6 hart codiert - bei jetzt 7 Sektionen landete der Wert von
            // RAYE (7/6 > 1) geklemmt auf dem letzten Eintrag davor: Position.
            param->setValueNotifyingHost ((float) next / (float) LCRMSAudioProcessor::SOLO_MAX);

            // Solo schaltet die zugehoerige Sektion dauerhaft an (schreibt den
            // echten Power-Parameter mit, kein unsichtbarer Zwischenzustand) -
            // eine Sektion kann also nicht gleichzeitig solo und off sein.
            // Bleibt so auch nach dem Aus-Solon bestehen (kein Zuruecksetzen).
            if (next != LCRMSAudioProcessor::SOLO_NONE)
            {
                const char* onParamId = nullptr;
                switch (next)
                {
                    case LCRMSAudioProcessor::SOLO_GALAXY:     onParamId = LCRMSAudioProcessor::ID_LCR_ENABLED;    break;
                    case LCRMSAudioProcessor::SOLO_TIMEWARP:   onParamId = LCRMSAudioProcessor::ID_DRIFT_ON;       break;
                    case LCRMSAudioProcessor::SOLO_POLARITY:   onParamId = LCRMSAudioProcessor::ID_POL_ON;         break;
                    case LCRMSAudioProcessor::SOLO_DIMENSION:  onParamId = LCRMSAudioProcessor::ID_WIDTHBOOST_ON;  break;
                    case LCRMSAudioProcessor::SOLO_HYPERDRIVE: onParamId = LCRMSAudioProcessor::ID_FLOW_ON;        break;
                    case LCRMSAudioProcessor::SOLO_POSITION:   onParamId = LCRMSAudioProcessor::ID_POS_ON;         break;
                    case LCRMSAudioProcessor::SOLO_RAY:        onParamId = LCRMSAudioProcessor::ID_RAY_ON;         break;
                    default: break;
                }
                if (onParamId != nullptr)
                {
                    if (auto* onParam = processor.apvts.getParameter (onParamId))
                        onParam->setValueNotifyingHost (1.0f);
                }
            }
        }
    };
}

// Setzt Solo zurueck auf "None", falls die aktuell solote Sektion genau die
// ist, deren Power gerade (manuell, per Icon ODER per Klick auf den Titel)
// ausgeschaltet wurde - eine Sektion kann nie solo+off gleichzeitig sein.
void LCRMSAudioProcessorEditor::resetSoloIfMatches (int soloValue)
{
    if (auto* soloParam = processor.apvts.getParameter (LCRMSAudioProcessor::ID_SOLO_SECTION))
    {
        const int current = juce::jlimit (0, LCRMSAudioProcessor::SOLO_MAX, (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_SOLO_SECTION)->load()));
        if (current == soloValue)
            soloParam->setValueNotifyingHost ((float) LCRMSAudioProcessor::SOLO_NONE / (float) LCRMSAudioProcessor::SOLO_MAX);
    }
}

// Siehe Kommentar an der Deklaration (PluginEditor.h).
void LCRMSAudioProcessorEditor::toggleUiBypass()
{
    // Runde 174 (User): nur noch EIN Bypass. Power-Knopf und Logo schalten
    // den Bypass-Parameter des Hosts - die DAW zeigt ihn dann auch an, und
    // der DAW-Bypass sieht im Plugin genauso aus wie der eigene.
    if (auto* bp = processor.getBypassParameter())
    {
        const bool nowBypassed = bp->getValue() > 0.5f;
        bp->beginChangeGesture();
        bp->setValueNotifyingHost (nowBypassed ? 0.0f : 1.0f);
        bp->endChangeGesture();
    }
    content.repaint();
}

// Section-Lock-Icon (User-Wunsch: "Sections ausschliessen" von Chaos/Mutate
// und Breathe, Variante "Lock Icon pro Section") - reiner GUI-Toggle, der
// processor.setSectionLocked() schreibt (persistiert automatisch mit dem
// Plugin-Zustand, siehe Processor-Kommentar). Kein APVTS-Attachment, daher
// muss der sichtbare Status beim Laden eines Zustands einmalig aus dem
// Processor uebernommen werden (siehe Konstruktor-Ende).
void LCRMSAudioProcessorEditor::setupLockButton (juce::TextButton& button, int soloValue)
{
    button.setClickingTogglesState (true);
    button.getProperties().set ("lockIcon", true);
    button.setWantsKeyboardFocus (false);
    content.addAndMakeVisible (button);
    button.setToggleState (processor.isSectionLocked (soloValue), juce::dontSendNotification);
    button.onClick = [this, &button, soloValue]
    {
        processor.setSectionLocked (soloValue, button.getToggleState());
    };
}

// Siehe Kommentar an der Deklaration (PluginEditor.h). ID_GALAXY_ACTIVATE
// (globaler Engine-An/Aus-Schalter, steuert AUSSCHLIESSLICH die Latenz)
// gehoert bewusst NICHT zur LCR/Galaxy-Liste - Mutate fasst ihn NIE an
// (siehe globalChaosButton.onClick), ein anderes Konzept als die Sektion im
// Signalpfad. Die Sektion selbst (ID_LCR_ENABLED) gehoert dagegen weiterhin
// zu dieser Liste - Mutate darf sie umwuerfeln, aber nur solange Galaxy
// gerade manuell aktiv ist (siehe globalChaosButton.onClick); Section-Lock
// schliesst sie zusaetzlich IMMER aus, unabhaengig davon.
// ===== MUTATE =====
// Gemeinsame Umsetzung beider Mutate-Tasten.
//
// Die urspruengliche Fassung hat schlicht JEDEN nicht ausgeschlossenen
// Parameter auf rng.nextFloat() gesetzt, also gleichverteilt zwischen 0 und 1.
// Genau das war das eigentliche Problem hinter "Mutate soll auch geile
// Ergebnisse liefern": bei rund zwanzig Parametern ist die Wahrscheinlichkeit,
// dass alle gleichzeitig musikalisch sinnvoll landen, verschwindend klein.
// Schutzregeln reparieren dann nur die schlimmsten Kollisionen, machen das
// DURCHSCHNITTLICHE Ergebnis aber nicht besser.
//
// Deshalb jetzt dreistufig:
//  1. GLOCKENVERTEILUNG um den jeweiligen Default statt Gleichverteilung.
//     Moderate Werte werden haeufig, Extreme selten - so, wie ein Mensch
//     einstellen wuerde.
//  2. WILDCARDS: 1 bis 3 zufaellig gewaehlte Parameter werden davon
//     ausgenommen und voll gleichverteilt gewuerfelt. Ohne sie waere Mutate
//     brav und langweilig - jeder Wurf klaenge nach demselben Mittelmass.
//     Mit ihnen ist das meiste sinnvoll und EINE Sache ueberrascht. Bewusst
//     nicht mehr als 3: ist alles auffaellig, ist nichts mehr auffaellig,
//     und wir waeren wieder bei der Gleichverteilung.
//  3. SICHERUNGEN ganz zum Schluss, NACH den Wildcards - damit auch eine
//     Wildcard die Regeln nicht aushebeln kann.
void LCRMSAudioProcessorEditor::runMutate (bool mayDisableSections)
{
    // Runde 124 (User): Wuerfeln bei aktivem Solo ergibt keinen Sinn - man
    // hoert nur eine Sektion, der Wuerfel veraendert aber alle.
    if ((int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_SOLO_SECTION)->load())
            != LCRMSAudioProcessor::SOLO_NONE)
        return;
    juce::Random& rng = juce::Random::getSystemRandom();
    // Sektions-Schalter, die Mutate setzt, duerfen Galaxy nicht scharfschalten.
    suppressGalaxyAutoArm = true;
    struct Unsuppress { bool& f; ~Unsuppress() { f = false; } } unsuppress { suppressGalaxyAutoArm };
    globalChaosButton.getProperties().set ("mutateColorState", rng.nextInt (16));
    // Runde 164 (User): der Wuerfel "rollt" kurz sichtbar (auch wenn am Ende
    // dieselbe Zahl steht) und landet auf der Zahl der aktiven Sektionen.
    dieRollUntilMs = juce::Time::getMillisecondCounterHiRes() + 420.0;
    globalChaosSectionsButton.getProperties().set ("mutateColorState", rng.nextInt (16));

    processor.chaosTriggerRequested.store (true);

    // Stand der Polarity-Position VOR dem Wuerfeln merken. Wenn dieser Wurf
    // weder L noch R aktiviert, wird sie unten wieder zurueckgesetzt - eine
    // wandernde 1-4-Auswahl ohne Flip ist reine Augenwischerei (User).
    const float polPosBefore = [this]
    {
        auto* p = processor.apvts.getParameter (LCRMSAudioProcessor::ID_POL_POS);
        return p != nullptr ? p->getValue() : 0.0f;
    }();

    // Hidden Egg: rund jeder zwanzigste Mutate-Wurf schickt das Raumschiff
    // los. Selten genug, dass man es nicht erwartet - und Mutate ist der
    // Moment, in dem man ohnehin aufs Feld schaut.
    if (rng.nextInt (20) == 0)
        goniometer.triggerEasterEgg();

    // Der globale Engine-Schalter (ID_GALAXY_ACTIVATE, steuert AUSSCHLIESSLICH
    // die Latenz) wird von Mutate NIE angefasst. Der Section-On/Off-Schalter
    // der Galaxy-Sektion darf nur umgeworfen werden, wenn Galaxy gerade
    // manuell aktiv ist - sonst haette es keine hoerbare Wirkung.
    auto* galaxyActivateParam = processor.apvts.getParameter (LCRMSAudioProcessor::ID_GALAXY_ACTIVATE);
    auto* galaxySectionParam  = processor.apvts.getParameter (LCRMSAudioProcessor::ID_LCR_ENABLED);
    const bool galaxyCurrentlyActive = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_GALAXY_ACTIVATE)->load() > 0.5f;

    juce::Array<juce::RangedAudioParameter*> excluded {
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_VOL_TRIM),
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_OUT_PAN),   // Runde 174: Fusszeile wird nie gewuerfelt
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_MONO_CHECK),
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_MONO_DRY),
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_GLOBAL_MOD_BYPASS),
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_SOLO_SECTION),
        // Runde 38: Amount/Modus sind nur Bedienhilfen fuer Drift/Shift -
        // gewuerfelt wird Drift/Shift selbst.
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_PARALLAX_MODE),
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_PARALLAX_AMOUNT),
        galaxyActivateParam,
        // ===== PRISM WIRD NICHT MITGEWUERFELT (jedenfalls nicht so) =====
        // User-Frage: "Soll Prism auch randomisiert werden? Oder eher nicht?"
        //
        // Im generischen Pool auf keinen Fall - und das war bis eben ein
        // echter Fehler: die drei PRISM-Parameter waren schlicht nicht
        // ausgeschlossen und wurden wie jeder andere Regler gewuerfelt. Lo
        // und Hi sind aber nicht unabhaengig voneinander; frei gewuerfelt
        // landet man regelmaessig bei einem 40Hz-Band irgendwo oben, das
        // alles andere praktisch stummschaltet. Man haette Mutate dann nicht
        // als "anderer Klang" erlebt, sondern als "kaputt".
        //
        // Stattdessen wird PRISM weiter unten GEZIELT und selten gesetzt,
        // mit musikalisch sinnvollen Baendern.
        // Die beiden Focus-Bypass-Schalter gehoeren ebenfalls NICHT in den Pool
        // (User: "Smart sollte diese Filter nicht deaktivieren koennen") - sie
        // entscheiden ueber die Struktur des Signalwegs, nicht ueber den Klang
        // einer Sektion, und ein zufaellig umgelegter Bypass sieht aus wie ein
        // Fehler.
        // Auto Gain ist ein Messwerkzeug, kein Klangparameter - wer es zufaellig
        // umlegt, zerstoert genau die Vergleichbarkeit, fuer die es da ist.
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_AUTO_GAIN),
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_PRISM_GALAXY),
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_PRISM_DIM),
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_PRISM_VIS),
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_PRISM_ON),
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_PRISM_LO),
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_PRISM_HI),
        // RAY ebenfalls gezielt statt generisch (siehe eigener Block unten):
        // ein Phaser ist ein Charakter-Eingriff, der nicht in jedem Wurf
        // vorkommen soll - und sein Ein/Aus darf NICHT wie die sechs
        // Sektionsschalter behandelt werden ("alle an").
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_RAY_ON),
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_RAY_STRENGTH),
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_RAY_RATE),
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_RAY_PAIR),
        // MIX: nie im generischen Pool, optional gezielt (siehe unten).
        processor.apvts.getParameter (LCRMSAudioProcessor::ID_MIX)
    };
    // Show Advanced Modulation an: die Sektions-Tiefen werden sichtbar
    // gewuerfelt - Life zusaetzlich zu wuerfeln waere doppelt (User, Runde 38).
    if (advancedModVisible)
        excluded.add (processor.apvts.getParameter (LCRMSAudioProcessor::ID_LIFE));
    // Regel (User, Runde 26): Smart schaltet NIE die Galaxy-Engine scharf -
    // eine Latenzaenderung darf nie aus einem Wuerfelwurf kommen. Solange die
    // Engine aus ist, bleibt die ganze Sektion unberuehrt (frueher wurde nur
    // der Section-Schalter geschont, die Regler liefen trotzdem mit - genau
    // die Inkonsistenz, die bei den Kategorien aufgefallen ist).
    if (! galaxyCurrentlyActive)
    {
        excluded.add (galaxySectionParam);
        for (auto& paramId : sectionParamIds (LCRMSAudioProcessor::SOLO_GALAXY))
            excluded.add (processor.apvts.getParameter (paramId));
    }

    // Section-Lock: jede gesperrte Sektion bleibt komplett unangetastet.
    const int lockableSections[] = {
        LCRMSAudioProcessor::SOLO_GALAXY, LCRMSAudioProcessor::SOLO_TIMEWARP,
        LCRMSAudioProcessor::SOLO_POLARITY, LCRMSAudioProcessor::SOLO_DIMENSION,
        LCRMSAudioProcessor::SOLO_HYPERDRIVE, LCRMSAudioProcessor::SOLO_POSITION,
        LCRMSAudioProcessor::SOLO_RAY
    };
    for (int section : lockableSections)
    {
        if (! processor.isSectionLocked (section))
            continue;
        for (auto& paramId : sectionParamIds (section))
            excluded.add (processor.apvts.getParameter (paramId));
    }

    // Die sechs Sektions-Schalter werden NICHT mitgewuerfelt, sondern hier
    // gezielt gesetzt - sie entscheiden ueber Struktur, nicht ueber Klang.
    //
    // NEU (User, Runde 26): Das passiert JETZT VOR dem Wuerfeln der Klang-
    // parameter. Eine Sektion, die aus bleibt, wird anschliessend komplett
    // ausgeklammert und behaelt ihre Werte. Vorher wurde auch in
    // ausgeschalteten Sektionen alles neu gewuerfelt - schaltete man so eine
    // Sektion spaeter von Hand an, bekam man eine Einstellung, die nie jemand
    // gehoert oder geprueft hatte. Genau daran fuehlt sich "random" an statt
    // "smart"; dieselbe Ueberlegung steckt schon hinter der Polarity- und
    // RAYE-Kopplung.
    struct SectionSwitch { const char* onId; int solo; };
    static const SectionSwitch sectionSwitches[6] = {
        { LCRMSAudioProcessor::ID_LCR_ENABLED,   LCRMSAudioProcessor::SOLO_GALAXY },
        { LCRMSAudioProcessor::ID_DRIFT_ON,      LCRMSAudioProcessor::SOLO_TIMEWARP },
        { LCRMSAudioProcessor::ID_POL_ON,        LCRMSAudioProcessor::SOLO_POLARITY },
        { LCRMSAudioProcessor::ID_WIDTHBOOST_ON, LCRMSAudioProcessor::SOLO_DIMENSION },
        { LCRMSAudioProcessor::ID_FLOW_ON,       LCRMSAudioProcessor::SOLO_HYPERDRIVE },
        { LCRMSAudioProcessor::ID_POS_ON,        LCRMSAudioProcessor::SOLO_POSITION }
    };
    juce::Array<juce::RangedAudioParameter*> sectionOnParams;
    for (auto& sw : sectionSwitches)
        if (auto* p = processor.apvts.getParameter (sw.onId))
            sectionOnParams.add (p);

    // Welche Sektionen darf der Wuerfel ueberhaupt schalten? (Schloss, und
    // Galaxy nur bei scharfer Engine - siehe oben.)
    juce::Array<int> switchable;
    for (int i = 0; i < 6; ++i)
    {
        auto* p = processor.apvts.getParameter (sectionSwitches[i].onId);
        if (p != nullptr && ! excluded.contains (p))
            switchable.add (i);
    }
    for (int i : switchable)
        processor.apvts.getParameter (sectionSwitches[i].onId)->setValueNotifyingHost (1.0f);

    juce::Array<int> turnedOff;
    if (mayDisableSections && switchable.size() >= 4)
    {
        const int offCount = juce::jmin (switchable.size() - 3, 2 + rng.nextInt (2));   // 2 oder 3
        for (int i = 0; i < offCount; ++i)
        {
            const int idx = switchable[rng.nextInt (switchable.size())];
            if (! turnedOff.contains (idx))
                turnedOff.add (idx);
        }
        for (int idx : turnedOff)
            processor.apvts.getParameter (sectionSwitches[idx].onId)->setValueNotifyingHost (0.0f);
    }
    // Was aus bleibt, bleibt auch unveraendert.
    for (int idx : turnedOff)
        for (auto& paramId : sectionParamIds (sectionSwitches[idx].solo))
            excluded.add (processor.apvts.getParameter (paramId));
    const bool polarityTurnedOff = turnedOff.contains (2);

    // ---- Pool der zu wuerfelnden Klangparameter aufbauen ----
    juce::Array<juce::RangedAudioParameter*> pool;
    for (auto* param : processor.getParameters())
    {
        auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param);
        if (rp == nullptr || excluded.contains (rp) || sectionOnParams.contains (rp)
            || param == processor.getBypassParameter()
            || rp->getParameterID() == LCRMSAudioProcessor::ID_MS_EQ_ON)   // Runde 159: ohne Schalter nie wuerfeln
            continue;
        pool.add (rp);
    }

    // ---- Wildcards auswaehlen ----
    // Gewichtung 45/35/20 auf 1/2/3: eine einzelne Ueberraschung ist am
    // besten lesbar, drei sind die sinnvolle Obergrenze.
    const int roll = rng.nextInt (100);
    const int wildcardCount = (roll < 45) ? 1 : (roll < 80) ? 2 : 3;

    juce::Array<int> wildcards;
    for (int i = 0; i < wildcardCount && pool.size() > 0; ++i)
    {
        const int idx = rng.nextInt (pool.size());
        if (! wildcards.contains (idx))
            wildcards.add (idx);
    }

    // ---- Wuerfeln ----
    // Glocke: der Mittelwert dreier Gleichverteilungen ist glockenfoermig um
    // 0,5 verteilt. Auf den DEFAULT des jeweiligen Parameters zentriert (nicht
    // auf 0,5) und mit kSpread skaliert ergibt das "meistens moderat, selten
    // extrem". Der Default als Zentrum ist wichtig: bei einem einseitigen
    // Regler wie Boost (0..6 dB, Default 0) entsteht dadurch automatisch
    // "meistens wenig, manchmal viel" statt "meistens die Haelfte".
    constexpr float kSpread = 0.55f;

    // ---- Abweichende Glocken-Zentren fuer einzelne Regler ----
    // Der Default ist NICHT immer ein gutes Zentrum. Bei Reglern, deren
    // Default am unteren Anschlag liegt, landet die Glocke sonst fast
    // immer bei null - der Regler wird dadurch praktisch nie gewuerfelt.
    //
    // User-Beobachtung: "Hier ist mir aufgefallen, dass Orbit oft fast auf 0
    // ist. Hier darf also die Glockenkurve deutlich hoeher sein."
    // Genau dieser Fall: Orbit laeuft 0..100 mit Default 0.
    //
    // Entscheidend ist dabei die Unterscheidung, WARUM ein Default am
    // Anschlag liegt:
    //  - Boost (0..6 dB, Default 0) steht dort, weil "nichts" der neutrale
    //    und leiseste Zustand ist. Hier ist "meistens wenig" richtig - das
    //    Zentrum bleibt der Default.
    //  - Orbit und Flow stehen dort nur, weil das Plugin neutral starten
    //    soll. Beide sind aber genau das Interessante an ihrer Sektion und
    //    voellig ungefaehrlich. Ihr Zentrum wird deshalb bewusst angehoben.
    auto* orbitParam = processor.apvts.getParameter (LCRMSAudioProcessor::ID_LCR_BLEND);
    auto* flowParam  = processor.apvts.getParameter (LCRMSAudioProcessor::ID_MOVEMENT);

    for (int i = 0; i < pool.size(); ++i)
    {
        auto* p = pool.getReference (i);
        if (wildcards.contains (i))
        {
            p->setValueNotifyingHost (rng.nextFloat());
        }
        else
        {
            float centre = p->getDefaultValue();
            if (p == orbitParam)      centre = 0.55f;
            else if (p == flowParam)  centre = 0.35f;

            const float t = (rng.nextFloat() + rng.nextFloat() + rng.nextFloat()) / 3.0f;
            const float dev = (t - 0.5f) * 2.0f * kSpread;
            p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, centre + dev));
        }
    }

    // ---- Parallax-Modus: gleichverteilt statt Glocke ----
    // In den Modus-Builds entscheidet der Modus, was Drift/Shift/Tilt
    // ueberhaupt tun - eine Glocke um den Default haette fast immer DOUBLE
    // gezogen (User: "Smart wuerfelt Parallax in den Modus-Builds nicht mit").
    if (SPACEX_PARALLAX_UI != 1)
    {
        if (auto* pxMode = processor.apvts.getParameter (LCRMSAudioProcessor::ID_PARALLAX_MODE))
            if (! excluded.contains (pxMode))
                pxMode->setValueNotifyingHost (pxMode->convertTo0to1 ((float) rng.nextInt (LCRMSAudioProcessor::kParallaxModes)));
    }

    // ---- Polarity: Sektion und Positionen an den Flip koppeln ----
    // User: "es macht keinen Sinn, wenn 1-4 sich aendern, die Section aber
    // off bleibt und sich sowieso nicht L oder R aktiviert hat - oder on ist,
    // aber L oder R nicht aktiviert wurden. Da Polarity ein groesserer
    // Eingriff ist als RAYE."
    // Regel: ein Flip (L, R oder beide) ist die Bedingung fuer alles andere.
    //   * Kein Flip  -> Positionen 1-4 bleiben unveraendert stehen.
    //                   Smart 2 schaltet die Sektion zusaetzlich aus.
    //   * Mit Flip   -> Sektion an, Positionen duerfen gewandert sein.
    // Smart 1 laesst die Sektion wie alle anderen immer an; nur die
    // Positionen bleiben ohne Flip unangetastet.
    if (! polarityTurnedOff && ! processor.isSectionLocked (LCRMSAudioProcessor::SOLO_POLARITY))
    {
        auto* polL   = processor.apvts.getParameter (LCRMSAudioProcessor::ID_POL_L);
        auto* polR   = processor.apvts.getParameter (LCRMSAudioProcessor::ID_POL_R);
        auto* polPos = processor.apvts.getParameter (LCRMSAudioProcessor::ID_POL_POS);
        auto* polOn  = processor.apvts.getParameter (LCRMSAudioProcessor::ID_POL_ON);
        const bool flip = (polL != nullptr && polL->getValue() > 0.5f)
                       || (polR != nullptr && polR->getValue() > 0.5f);
        if (! flip && polPos != nullptr)
            polPos->setValueNotifyingHost (polPosBefore);
        if (polOn != nullptr && ! excluded.contains (polOn))
            polOn->setValueNotifyingHost ((flip || ! mayDisableSections) ? 1.0f : 0.0f);
    }

    // ---- PRISM: selten, aber dann musikalisch ----
    // Antwort auf die User-Frage "Soll Prism auch randomisiert werden?":
    // ja - aber nicht als Regler, sondern als Auswahl aus einer Handvoll
    // brauchbarer Baender.
    //
    // Der Gedanke dahinter: PRISM ist kein Effektregler, den man "ein
    // bisschen" aufdreht, sondern eine Entscheidung darueber, WO die
    // Verbreiterung ueberhaupt passiert. Solche Entscheidungen wuerfelt man
    // sinnvollerweise aus einer Liste guter Antworten, nicht aus einem
    // stufenlosen Bereich - genau so, wie man auch keine zufaellige Tonart
    // auswuerfelt, indem man eine Frequenz zieht.
    //
    // Deshalb: in zwei von drei Wuerfen bleibt PRISM auf dem Standard
    // (200 Hz aufwaerts, also nur der Bass geschuetzt). Nur im letzten
    // Drittel wird eines von vier Baendern gezogen, die bei Backing Vocals
    // und Adlibs tatsaechlich Sinn ergeben. PRISM wird dabei NIE
    // ausgeschaltet - der 200-Hz-Rolloff ist immer die richtige Grundlage.
    {
        auto* prismOnP = processor.apvts.getParameter (LCRMSAudioProcessor::ID_PRISM_ON);
        auto* prismLoP = processor.apvts.getParameter (LCRMSAudioProcessor::ID_PRISM_LO);
        auto* prismHiP = processor.apvts.getParameter (LCRMSAudioProcessor::ID_PRISM_HI);

        // Neu (User): nur, wenn im Menue "Mutate Changes Frequency Band"
        // an ist - sonst bleibt PRISM komplett unangetastet. Wenn an:
        // Glocke um 3 kHz fuer die UNTERE Kante (im Oktavraum, +-1,5 Okt.),
        // die obere Kante bleibt meist offen (20 kHz, wie ein High-Shelf),
        // in etwa jedem dritten Wurf liegt sie 1,5-3 Oktaven ueber der
        // unteren. PRISM wird dabei eingeschaltet.
        // Bug-Fix (User: "mutate -> frequency geht nicht"): die frueheren
        // Pruefung "! excluded.contains (prismOnP)" war IMMER falsch, weil
        // die drei PRISM-Parameter genau deshalb in `excluded` stehen, damit
        // der generische Pool sie nicht anfasst. Der gezielte Block hier lief
        // also nie. PRISM hat keinen Section-Lock, die Pruefung entfaellt.
        if (prismOnP != nullptr && prismLoP != nullptr && prismHiP != nullptr
            && juce::PropertiesFile (LCRMSAudioProcessor::appPropertiesOptions()).getBoolValue ("mutateChangesPrism", true))
        {
            // Glocke tiefer und breiter (User: "geht nie unter 1k, muss
            // oefter auch mal bis 220 Hz gehen"): Mitte 1,2 kHz, +-2,6
            // Oktaven -> ~200 Hz bis ~7 kHz, Schwerpunkt 500 Hz - 3 kHz.
            const float bell = (rng.nextFloat() + rng.nextFloat() + rng.nextFloat()) / 3.0f - 0.5f;   // -0.5..0.5, Glocke
            const float loHz = juce::jlimit (200.0f, 8000.0f, 1200.0f * std::exp2 (bell * 5.2f));
            float hiHz = 20000.0f;
            if (rng.nextInt (3) == 0)
                hiHz = juce::jlimit (loHz * 2.0f, 20000.0f, loHz * std::exp2 (1.5f + rng.nextFloat() * 1.5f));

            prismOnP->setValueNotifyingHost (1.0f);
            prismLoP->setValueNotifyingHost (prismLoP->convertTo0to1 (loHz));
            prismHiP->setValueNotifyingHost (prismHiP->convertTo0to1 (hiHz));
        }
    }

    // ---- MIX: wird nie gewuerfelt ----
    // Runde 174 (User: "Random soll Mix Regler nie aendern"). Die alte Option
    // "Mutate Changes Mix" hatte keinen Menuepunkt mehr, ein alter Eintrag in
    // den Einstellungen konnte sie aber noch einschalten.

    // ---- RAY: selten dabei, und wenn, dann meist leicht ----
    // User-Vorgabe: "Phaser ... auch randomisiert werden. Glockenkurve
    // default sagen wir mal 10%." Umgesetzt als zwei Wuerfe: ob der Phaser
    // ueberhaupt mitspielt (rund jeder dritte Wurf), und wenn ja, wie stark
    // (leicht 60 %, mittel 30 %, stark 10 % - das ist die "10 %"). Pair
    // gelegentlich, damit auch der gekoppelte Fall vorkommt. Speed bleibt
    // als einziger stufenloser Regler in der Glocke um seinen Default.
    {
        auto* rayOnP  = processor.apvts.getParameter (LCRMSAudioProcessor::ID_RAY_ON);
        auto* raySt   = processor.apvts.getParameter (LCRMSAudioProcessor::ID_RAY_STRENGTH);
        auto* rayRate = processor.apvts.getParameter (LCRMSAudioProcessor::ID_RAY_RATE);
        auto* rayPair = processor.apvts.getParameter (LCRMSAudioProcessor::ID_RAY_PAIR);
        // Speed liegt auch im generischen Pool und wurde deshalb selbst dann
        // verstellt, wenn RAYE gar nicht spielt (User: "Speed on, Pair off,
        // Tiefe 0 - also ist der Effekt aus, genau wie vorhin bei Polarity").
        // Der Stand vor dem Wurf wird gemerkt und ohne Stufe zurueckgesetzt.
        const float rayRateBefore = rayRate != nullptr ? rayRate->getValue() : 0.0f;

        if (rayOnP != nullptr && raySt != nullptr && rayRate != nullptr && rayPair != nullptr
            && ! processor.isSectionLocked (LCRMSAudioProcessor::SOLO_RAY))
        {
            // Die Sektion bleibt AN wie alle anderen (User: "Mutate schaltet
            // RAYE aus, das irritiert") - "spielt nicht" heisst Stufe Off.
            const bool rayPlays = rng.nextInt (100) < 33;
            int level = 0;
            if (rayPlays)
            {
                const int r = rng.nextInt (100);
                level = (r < 60) ? 1 : (r < 90) ? 2 : 3;   // Light/Medium/Strong (0 = Off)
            }
            // Bug (User): Pair wurde frueher nur im rayPlays-Zweig gesetzt und
            // blieb sonst auf seinem alten Wert stehen - dann stand Pair auf
            // "an", waehrend die Stufe Off war. Pair wird jetzt IMMER gewuerfelt;
            // faellt es an, waehrend keine Stufe gezogen wurde, gibt es Stufe 1
            // dazu ("dann schalte Intensity auf Stufe 1"). Hat der Wurf bereits
            // Stufe 1-3 ergeben, bleibt Pair einfach zufaellig.
            const bool pairOn = rng.nextInt (100) < 25;
            if (pairOn && level == 0)
                level = 1;

            rayOnP->setValueNotifyingHost (1.0f);
            raySt->setValueNotifyingHost (raySt->convertTo0to1 ((float) level));
            rayPair->setValueNotifyingHost (pairOn ? 1.0f : 0.0f);
            if (level > 0)
            {
                const float t = (rng.nextFloat() + rng.nextFloat() + rng.nextFloat()) / 3.0f;
                rayRate->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, rayRate->getDefaultValue() + (t - 0.5f) * 2.0f * kSpread));
            }
            else
            {
                // Keine Stufe -> RAYE ist stumm, also bleibt auch Speed stehen.
                rayRate->setValueNotifyingHost (rayRateBefore);
            }
        }
    }

    // ---- Freischalter: Regler, die andere Regler erst wirksam machen ----
    // User-Feedback aus der Praxis, drei Beobachtungen mit derselben Ursache:
    //   "Random Aenderung von den 1-4 Buttons ohne dass L oder R an geht
    //    bringt nichts."
    //   "Gravity hat keinen Einfluss wenn Orbit off ist."
    //   "Speed / Sync hat keinen Einfluss wenn Flow off ist."
    // In allen drei Faellen schaltet ein Parameter die Wirkung eines anderen
    // ueberhaupt erst frei. Wuerfelt man den abhaengigen Regler, waehrend der
    // Freischalter auf null steht, ist dieser Wurf schlicht verschwendet -
    // der Nutzer sieht Regler wandern und hoert nichts.
    // Deshalb wird nach dem Wuerfeln sichergestellt, dass jeder Freischalter
    // tatsaechlich etwas durchlaesst.
    {
        // a) Polarity BEWUSST OHNE Freischalter.
        //    Erster Anlauf war hier falsch: wenn weder L noch R aktiv ist,
        //    wurde einer davon eingeschaltet, damit die Positionen 1-4 eine
        //    Wirkung haben. Das war ein Denkfehler - dadurch hatte JEDER
        //    Mutate-Wurf einen Phasen-Flip (User: "Aber das ist oft schon
        //    auch ein krasser Eingriff. Es sollte eher seltener vorkommen").
        //    Ein Flip ist der drastischste Eingriff im ganzen Plugin; dass
        //    die Positions-Buttons ohne ihn bedeutungslos sind, ist dagegen
        //    voellig harmlos - man sieht und hoert schlicht nichts davon.
        //    Es gibt also nichts zu reparieren.
        //    Wie oft ein Flip vorkommt, regeln jetzt allein die Wildcards:
        //    L und R werden nur dann eingeschaltet, wenn sie als Wildcard
        //    gezogen werden - bei rund 30 Parametern und 1-3 Wildcards also
        //    ungefaehr jeder fuenfzehnte Wurf. Genau die gewollte Seltenheit,
        //    ohne eine weitere Stellschraube.

        // b) Orbit schaltet Gravity frei. Gravity bekommt immer einen Wert,
        //    also muss Orbit etwas durchlassen, sonst ist er wirkungslos.
        if (orbitParam != nullptr && ! excluded.contains (orbitParam)
            && orbitParam->getValue() < 0.12f)
            orbitParam->setValueNotifyingHost (0.20f + rng.nextFloat() * 0.45f);

        // c) Flow schaltet Speed, Sync und Pulse frei. Steht Flow auf null,
        //    laeuft der Auto-Pan-LFO gar nicht und alle drei sind wirkungslos.
        if (flowParam != nullptr && ! excluded.contains (flowParam)
            && flowParam->getValue() < 0.12f)
            flowParam->setValueNotifyingHost (0.18f + rng.nextFloat() * 0.35f);
    }

    // ---- Sicherung 1: Flow abhaengig von der Geschwindigkeit ----
    // Schnelles Auto-Pan bei hoher Tiefe ist unangenehm, langsames bei hoher
    // Tiefe ist schoen - eine echte musikalische Abhaengigkeit, keine Vorliebe.
    // ACHTUNG bei den Parameternamen: ID_SPEED ist die SYNC-AUSWAHL
    // ("1/16".."8 Bars", Index 0 = am schnellsten), ID_SPEED_RATE ist die
    // freie Hz-Rate. Die beiden heissen genau andersherum, als man vermutet -
    // das wurde vor dem Einbau im Processor gegengeprueft.
    {
        auto* flowP = processor.apvts.getParameter (LCRMSAudioProcessor::ID_MOVEMENT);
        if (flowP != nullptr && ! excluded.contains (flowP))
        {
            const bool syncOn = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_SPEED_SYNC)->load() > 0.5f;

            float fastness = 0.0f; // 0 = langsam, 1 = so schnell wie moeglich
            if (syncOn)
            {
                const int rateIdx = juce::jlimit (0, 7,
                    (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_SPEED)->load()));
                fastness = 1.0f - (float) rateIdx / 7.0f; // Index 0 = 1/16 = am schnellsten
            }
            else if (auto* rateP = processor.apvts.getParameter (LCRMSAudioProcessor::ID_SPEED_RATE))
            {
                fastness = rateP->getValue(); // normalisiert, monoton mit der Frequenz
            }

            // 100% erlaubt bei langsam, herunter auf 30% bei maximalem Tempo.
            const float maxFlow = 1.0f - fastness * 0.7f;
            if (flowP->getValue() > maxFlow)
                flowP->setValueNotifyingHost (maxFlow * (0.6f + rng.nextFloat() * 0.4f));
        }
    }

    // ---- Sicherung 2: Lautheit (Drift / Size / Boost) ----
    // Diese drei erhoehen alle die Seitenenergie. Einzeln brauchbar, zu zweit
    // vertretbar, alle drei am Anschlag ergibt zuverlaessig ein zu lautes,
    // zerfallenes Stereobild. Laeuft bewusst ZULETZT, damit auch eine
    // Wildcard sie nicht umgehen kann.
    // Im normalisierten Raum als Abstand vom Default gerechnet, weil die drei
    // voellig unterschiedliche Einheiten haben (Drift % um 0, Size 50..200%
    // um 100, Boost 0..6 dB um 0).
    {
        const char* guardIds[3] = { LCRMSAudioProcessor::ID_DRIFT,
                                    LCRMSAudioProcessor::ID_SIDE_WIDTH,
                                    LCRMSAudioProcessor::ID_SIDE_BOOST };

        juce::RangedAudioParameter* gp[3] = { nullptr, nullptr, nullptr };
        float gdev[3] = { 0.0f, 0.0f, 0.0f };
        bool allPresent = true;

        for (int i = 0; i < 3; ++i)
        {
            auto* p = processor.apvts.getParameter (guardIds[i]);
            if (p == nullptr || excluded.contains (p))
            {
                allPresent = false;
                break;
            }
            gp[i]   = p;
            gdev[i] = std::abs (p->getValue() - p->getDefaultValue());
        }

        if (allPresent)
        {
            int lo = 0;
            for (int i = 1; i < 3; ++i)
                if (gdev[i] < gdev[lo])
                    lo = i;

            bool othersExtreme = true;
            for (int i = 0; i < 3; ++i)
                if (i != lo && gdev[i] <= 0.5f)
                    othersExtreme = false;

            if (othersExtreme && gdev[lo] > 0.1f)
            {
                auto* p = gp[lo];
                const float def = p->getDefaultValue();
                const float sign = (p->getValue() >= def) ? 1.0f : -1.0f;
                p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, def + sign * rng.nextFloat() * 0.10f));
            }
        }
    }

    // ---- Kategorie-Profil (Chips ueber dem Sternenfeld) - hat das letzte Wort ----
    applyMutateProfile (rng, mutateCategory());
}

// ===== KATEGORIE-PROFILE fuer Mutate =====
// Eine Kategorie ist keine Klangfarbe, sondern eine Regel, was Mutate DARF.
// Sie laeuft NACH dem generischen Wurf und ueberschreibt gezielt: welche
// Sektionen mitspielen, in welchem Bereich die wichtigen Regler landen,
// wo PRISM beginnt, wie stark RAYE. Alles andere bleibt vom generischen
// Wurf. Gesperrte Sektionen (Lock) werden auch hier nicht angefasst.
// Was tut das gewaehlte Profil? Steht ueber dem Sternenfeld - die Chips
// haben das nie verraten, der Name allein beantwortet die Frage nicht.
juce::String LCRMSAudioProcessorEditor::smartInfoTextFor (int cat)
{
    switch (cat)
    {
        case 1: return "Lead vocal - wide, centre stays put.";
        case 2: return "Stacks, busses and mono doubles - wide, but tidy.";
        case 3: return "Adlibs - space, movement, clear sides.";
        // Runde 84 (User): "anything goes" stimmte nicht - eine Parallel-
        // kompression ist auch ein Send, und dort will man SpaceX gerade nicht.
        // Gemeint sind Spuren, die nur aus Effekt bestehen.
        case 4: return "Reverb and delay returns - pure effect, nothing dry.";
        default: return {};   // kein Profil, keine Zeile (User)
    }
}

int LCRMSAudioProcessorEditor::mutateCategory() const
{
    return juce::jlimit (0, kNumCategories, mutateCategoryValue);
}

void LCRMSAudioProcessorEditor::setMutateCategory (int cat)
{
    mutateCategoryValue = (cat < 0 || cat > kNumCategories) ? 0 : cat;   // alte Sessions (5/6) -> aus
    processor.apvts.state.setProperty ("mutateCategory", mutateCategoryValue, nullptr);   // Session-Recall
    for (int i = 0; i < kNumCategories; ++i)
        categoryBtn[i].setToggleState (mutateCategoryValue == i + 1, juce::dontSendNotification);

    static const char* const pillNames[kNumCategories + 1] = { "NO PROFILE", "LEAD VOCAL", "BACKINGS", "ADLIBS", "SEND FX" };
    categoryButton.setButtonText (pillNames[juce::jlimit (0, kNumCategories, mutateCategoryValue)]);
    const bool profileArmed = mutateCategoryValue > 0;
    categoryButton.getProperties().set ("pillStrong", true);
    categoryButton.getProperties().set ("pillArmed", profileArmed);
    // Runde 108 (User): Icon links vom Namen, kein Rahmen - das Icon zeigt,
    // wo die Quelle im Stereobild sitzt.
    categoryButton.getProperties().set ("profileDiagram", juce::jlimit (0, kNumCategories, mutateCategoryValue));
    categoryButton.getProperties().set ("iconAbove", true);   // Runde 110: Icon ueber dem Namen
    content.repaint (categoryButton.getBounds().expanded (180, 30));   // Runde 126: Glow folgt dem Profil
    categoryButton.getProperties().set ("pillColour",
        (int) (profileArmed ? themePalette().knob : juce::Colour (0xff7b808b)).getARGB());
    categoryButton.repaint();
    catDots.index = mutateCategoryValue;
    catDots.repaint();
    smartInfoLabel.setText (smartInfoTextFor (mutateCategoryValue), juce::dontSendNotification);
    // Runde 114: ohne Profil gibt es nichts zu erklaeren - (i) und Zeile weg.
    smartInfoToggle.setVisible (showMutateCategories && mutateCategoryValue > 0);
    smartInfoLabel.setVisible (showMutateCategories && smartInfoVisible && mutateCategoryValue > 0);

    // Widerspruch aufgeloest (User): mit gewaehlter Kategorie schaltet auch
    // Smart 1 Sektionen aus, obwohl Smart 1 eigentlich "alle bleiben an"
    // bedeutet - die Kategorie entscheidet ja selbst, welche Sektion mitspielt.
    // Deshalb ist Smart 1 gesperrt, solange eine Kategorie aktiv ist; es
    // bleibt nur der zweite Wuerfel, der ohnehin Sektionen schalten darf.
    const bool catOn = mutateCategoryValue > 0;
    globalChaosButton.setEnabled (true);
    globalChaosButton.setAlpha (1.0f);
    globalChaosButton.setTooltip (catOn ? "Smart: rolls a new setting inside the selected profile, and decides which sections belong in it"
                                        : "Smart: randomize the sound and leave every section switched on");
    // Der zweite Wuerfel bekommt so lange einen dezenten Hof, damit man
    // sofort sieht, wohin die Kategorie wirkt (User: "Smart-Icon highlighten").
    // Der Wuerfel selbst zeigt jetzt, dass ein Profil laeuft (User Runde 56:
    // "leicht leuchtend ... vgl. galaxy global button").
    globalChaosButton.getProperties().set ("categoryArmed", catOn);
    // Der Wuerfel wechselt zwischen schmal und breit - das ist Layout.
    // WICHTIG: content.resized(), nicht resized(). resized() setzt nur die
    // Bounds von content neu; sind die unveraendert, ruft JUCE dessen
    // resized() gar nicht auf - das Layout lief dadurch erst beim naechsten
    // Anlass und hing genau einen Schritt hinterher (User Runde 53: "grosser
    // Wuerfel ist nicht aktiv wenn Kategorie selektiert ist - aber dafuer
    // wenn keine an ist").
    if (getWidth() > 0)
        content.resized();
    globalChaosButton.repaint();
    globalChaosSectionsButton.repaint();
}

void LCRMSAudioProcessorEditor::applyMutateProfile (juce::Random& rng, int category)
{
    if (category <= 0)
        return;
    using P = LCRMSAudioProcessor;

    auto bell = [&rng]() { return (rng.nextFloat() + rng.nextFloat() + rng.nextFloat()) / 1.5f - 1.0f; };   // -1..1, Glocke
    auto param = [this] (const char* id) { return processor.apvts.getParameter (id); };
    auto locked = [this] (int solo) { return processor.isSectionLocked (solo); };
    // Wert in ECHTEN Einheiten setzen: centre +- spread (Glocke), begrenzt auf lo..hi.
    auto roll = [&] (const char* id, float centre, float spread, float lo, float hi)
    {
        if (auto* p = param (id))
            p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (lo, hi, centre + bell() * spread)));
    };
    auto set = [&] (const char* id, float value)
    {
        if (auto* p = param (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
    };
    auto chance = [&rng] (int percent) { return rng.nextInt (100) < percent; };
    // Modus-Builds (User Runde 50): die Kategorie waehlt einen passenden
    // Parallax-STYLE und einen Amount-Bereich - Drift/Shift/Tilt direkt zu
    // setzen bringt dort nichts, die ueberschreibt der Modus ohnehin.
    // 0=Double 1=Wide 2=Illusion 3=3D 4=Drift 5=Flux
    constexpr bool pxModes = (SPACEX_PARALLAX_UI != 1);
    auto parallax = [&] (std::initializer_list<int> modes, float amtCentre, float amtSpread, float lo, float hi)
    {
        if (auto* mp = param (P::ID_PARALLAX_MODE))
        {
            const int pick = *(modes.begin() + rng.nextInt (juce::jmax (1, (int) modes.size())));
            mp->setValueNotifyingHost (mp->convertTo0to1 ((float) pick));
        }
        roll (P::ID_PARALLAX_AMOUNT, amtCentre, amtSpread, lo, hi);
    };
    juce::ignoreUnused (parallax);
    const bool galaxyArmed = processor.apvts.getRawParameterValue (P::ID_GALAXY_ACTIVATE)->load() > 0.5f;
    auto section = [&] (const char* onId, int solo, int onPercent) -> bool
    {
        if (locked (solo)) return false;
        // Dieselbe Regel wie beim freien Wuerfeln (User-Bug: "wenn categories
        // selected sind aber global galaxy off, dann aktiviert der Wuerfel
        // dennoch die Galaxy-Section"). Ohne scharfe Engine bleibt sie in Ruhe.
        if (solo == P::SOLO_GALAXY && ! galaxyArmed) return false;
        const bool on = chance (onPercent);
        set (onId, on ? 1.0f : 0.0f);
        return on;
    };
    // Drift in Millisekunden (Vorzeichen zufaellig), Shift in Cent.
    auto driftMs = [&] (float centreMs, float spreadMs, float maxMs)
    {
        const float ms = juce::jlimit (0.0f, maxMs, centreMs + bell() * spreadMs);
        const float pct = P::driftMsToPercent (ms) * (chance (50) ? 1.0f : -1.0f);
        set (P::ID_DRIFT, pct);
    };
    // Hyperdrive langsam per Sync: Notenindex 3..7 = 1/2 .. 8 Takte.
    auto slowSync = [&] (int minIdx, int maxIdx)
    {
        set (P::ID_SPEED_SYNC, 1.0f);
        if (auto* p = param (P::ID_SPEED))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) (minIdx + rng.nextInt (maxIdx - minIdx + 1))));
    };
    // PRISM: untere Kante log-verteilt zwischen loMin..loMax, oben offen.
    auto prism = [&] (float loMin, float loMax)
    {
        // Das Menue gilt auch fuer die Kategorien (User-Bug: "wenn smart
        // aendert frequency band = off, dann aendert smart es trotzdem, wenn
        // eine Kategorie ausgewaehlt ist").
        if (! juce::PropertiesFile (LCRMSAudioProcessor::appPropertiesOptions())
                 .getBoolValue ("mutateChangesPrism", true))
            return;
        const float t = 0.5f + 0.5f * bell();   // 0..1 Glocke
        const float lo = loMin * std::exp2 (t * std::log2 (loMax / loMin));
        set (P::ID_PRISM_ON, 1.0f);
        set (P::ID_PRISM_LO, lo);
        set (P::ID_PRISM_HI, 20000.0f);
    };
    // RAYE: Sektion an, Stufe per Wahrscheinlichkeit (0 = Off).
    auto raye = [&] (int playPercent, int lightPct, int medPct, int pairPct)
    {
        if (locked (P::SOLO_RAY)) return;
        int level = 0;
        if (chance (playPercent))
        {
            const int r = rng.nextInt (100);
            level = (r < lightPct) ? 1 : (r < lightPct + medPct) ? 2 : 3;
        }
        // Mit Kategorie: Stufe Off = Sektion aus (User: "Drums -> RAYE ist
        // immer on" sah falsch aus). Ohne Kategorie bleibt alles an.
        set (P::ID_RAY_ON, level > 0 ? 1.0f : 0.0f);
        set (P::ID_RAY_STRENGTH, (float) level);
        set (P::ID_RAY_PAIR, chance (pairPct) ? 1.0f : 0.0f);
    };
    auto polarityOff = [&]
    {
        if (locked (P::SOLO_POLARITY)) return;
        set (P::ID_POL_L, 0.0f);
        set (P::ID_POL_R, 0.0f);
    };
    auto polarityRare = [&] (int percent)
    {
        if (locked (P::SOLO_POLARITY)) return;
        const bool flip = chance (percent);
        set (P::ID_POL_L, (flip && chance (50)) ? 1.0f : 0.0f);
        set (P::ID_POL_R, (flip && param (P::ID_POL_L)->getValue() < 0.5f) ? 1.0f : 0.0f);
        // Ohne Flip ist die Sektion aus - sonst wechseln die Positionen 1-4
        // sichtbar, obwohl nichts passiert (User: "sieht komisch aus").
        set (P::ID_POL_ON, flip ? 1.0f : 0.0f);
    };

    switch (category)
    {
        // Runde 37 (User): vier einfache Kategorien, geordnet danach, wie viel
        // veraendert werden darf - VOCAL am wenigsten, FX am meisten.
        // Tilt = ID_POS_OFFSET, Depth = ID_DEPTH (- = Ferne, + = Hoehe).
        case 1: // VOCAL - Lead/Mono-Vocal breit machen, Mitte bleibt stehen
        {
            if (section (P::ID_LCR_ENABLED, P::SOLO_GALAXY, 30))
            {
                roll (P::ID_LCR_SENS, 45.0f, 15.0f, 20.0f, 70.0f);
                roll (P::ID_LCR_BLEND, 10.0f, 10.0f, 0.0f, 30.0f);
            }
            // Mono wird nur durch PARALLAX stereo - deshalb fast immer an.
            if (section (P::ID_DRIFT_ON, P::SOLO_TIMEWARP, 90))
            {
                if (pxModes) parallax ({ 0, 1 }, 40.0f, 12.0f, 20.0f, 60.0f);
                else { driftMs (1.5f, 0.8f, 3.0f); roll (P::ID_BEND, 2.0f, 1.5f, 0.0f, 4.0f); }
                set (P::ID_TIMEWARP_BALANCE, 1.0f);
            }
            if (! locked (P::SOLO_POLARITY)) { set (P::ID_POL_ON, 0.0f); polarityOff(); }
            if (section (P::ID_WIDTHBOOST_ON, P::SOLO_DIMENSION, 100))
            {
                roll (P::ID_SIDE_WIDTH, 115.0f, 10.0f, 100.0f, 130.0f);
                roll (P::ID_SIDE_BOOST, 0.5f, 1.0f, 0.0f, 2.0f);
                roll (P::ID_DEPTH, 0.0f, 8.0f, -10.0f, 15.0f);
            }
            if (! locked (P::SOLO_HYPERDRIVE)) set (P::ID_FLOW_ON, 0.0f);
            if (section (P::ID_POS_ON, P::SOLO_POSITION, 100))
                set (P::ID_POS_OFFSET, 0.0f);
            raye (10, 100, 0, 0);
            prism (200.0f, 350.0f);
            break;
        }
        case 2: // BACKING - Backing-Stacks/Bus: breit, aber noch geordnet
        {
            if (section (P::ID_LCR_ENABLED, P::SOLO_GALAXY, 60))
            {
                roll (P::ID_LCR_SENS, 50.0f, 20.0f, 20.0f, 80.0f);
                roll (P::ID_LCR_BLEND, 40.0f, 20.0f, 15.0f, 70.0f);
            }
            if (section (P::ID_DRIFT_ON, P::SOLO_TIMEWARP, 80))
            {
                if (pxModes) parallax ({ 0, 1, 3 }, 55.0f, 15.0f, 30.0f, 80.0f);
                else { driftMs (3.5f, 1.8f, 6.0f); roll (P::ID_BEND, 2.5f, 1.5f, 0.0f, 5.0f); }
                set (P::ID_TIMEWARP_BALANCE, 1.0f);
            }
            polarityRare (10);
            if (section (P::ID_WIDTHBOOST_ON, P::SOLO_DIMENSION, 100))
            {
                roll (P::ID_SIDE_WIDTH, 135.0f, 15.0f, 115.0f, 160.0f);
                roll (P::ID_SIDE_BOOST, 1.0f, 1.2f, 0.0f, 3.0f);
                roll (P::ID_DEPTH, -15.0f, 15.0f, -40.0f, 15.0f);
            }
            if (section (P::ID_FLOW_ON, P::SOLO_HYPERDRIVE, 25))
            {
                roll (P::ID_MOVEMENT, 20.0f, 10.0f, 8.0f, 35.0f);
                slowSync (5, 7);   // 2 .. 8 Takte
            }
            if (section (P::ID_POS_ON, P::SOLO_POSITION, 100))
                roll (P::ID_POS_OFFSET, 0.0f, 4.0f, -6.0f, 6.0f);
            raye (45, 60, 40, 20);
            prism (200.0f, 260.0f);
            break;
        }
        case 3: // ADLIB - darf viel: Raum, Bewegung, deutliche Seiten
        {
            if (section (P::ID_LCR_ENABLED, P::SOLO_GALAXY, 50))
            {
                roll (P::ID_LCR_SENS, 50.0f, 20.0f, 20.0f, 80.0f);
                roll (P::ID_LCR_BLEND, 50.0f, 25.0f, 20.0f, 85.0f);
            }
            if (section (P::ID_DRIFT_ON, P::SOLO_TIMEWARP, 90))
            {
                if (pxModes) parallax ({ 1, 2, 3, 4 }, 70.0f, 18.0f, 40.0f, 95.0f);
                else { driftMs (6.0f, 3.0f, 10.0f); roll (P::ID_BEND, 4.0f, 2.5f, 0.0f, 8.0f); }
                set (P::ID_TIMEWARP_BALANCE, 1.0f);
            }
            polarityRare (20);
            if (section (P::ID_WIDTHBOOST_ON, P::SOLO_DIMENSION, 100))
            {
                roll (P::ID_SIDE_WIDTH, 150.0f, 20.0f, 120.0f, 180.0f);
                roll (P::ID_SIDE_BOOST, 1.5f, 1.5f, 0.0f, 4.0f);
                roll (P::ID_DEPTH, -25.0f, 20.0f, -60.0f, 20.0f);
            }
            if (section (P::ID_FLOW_ON, P::SOLO_HYPERDRIVE, 60))
            {
                roll (P::ID_MOVEMENT, 35.0f, 15.0f, 15.0f, 55.0f);
                slowSync (3, 6);   // 1/2 .. 4 Takte
            }
            if (section (P::ID_POS_ON, P::SOLO_POSITION, 100))
                roll (P::ID_POS_OFFSET, 0.0f, 8.0f, -15.0f, 15.0f);
            raye (60, 40, 40, 30);
            prism (150.0f, 300.0f);
            break;
        }
        case 4: // FX - Throws, Wet-Spuren, FX-Returns: alles erlaubt
        {
            if (section (P::ID_LCR_ENABLED, P::SOLO_GALAXY, 60))
            {
                roll (P::ID_LCR_SENS, 55.0f, 20.0f, 20.0f, 85.0f);
                roll (P::ID_LCR_BLEND, 60.0f, 25.0f, 25.0f, 90.0f);
            }
            if (section (P::ID_DRIFT_ON, P::SOLO_TIMEWARP, 85))
            {
                if (pxModes) parallax ({ 2, 3, 4, 5 }, 80.0f, 20.0f, 45.0f, 100.0f);
                else { driftMs (8.0f, 4.0f, 14.0f); roll (P::ID_BEND, 5.5f, 2.5f, 2.0f, 8.0f); }
                set (P::ID_TIMEWARP_BALANCE, 1.0f);
            }
            polarityRare (25);
            if (section (P::ID_WIDTHBOOST_ON, P::SOLO_DIMENSION, 100))
            {
                roll (P::ID_SIDE_WIDTH, 165.0f, 25.0f, 130.0f, 190.0f);
                roll (P::ID_SIDE_BOOST, 2.0f, 2.0f, 0.0f, 4.5f);
                roll (P::ID_DEPTH, -40.0f, 25.0f, -80.0f, 20.0f);
            }
            if (section (P::ID_FLOW_ON, P::SOLO_HYPERDRIVE, 75))
            {
                roll (P::ID_MOVEMENT, 40.0f, 15.0f, 20.0f, 60.0f);
                slowSync (4, 7);   // 1 .. 8 Takte
            }
            if (section (P::ID_POS_ON, P::SOLO_POSITION, 100))
                roll (P::ID_POS_OFFSET, 0.0f, 10.0f, -20.0f, 20.0f);
            raye (70, 30, 40, 50);
            prism (100.0f, 200.0f);
            break;
        }
        default: break;
    }
}

juce::StringArray LCRMSAudioProcessorEditor::sectionParamIds (int soloSectionId)
{
    using P = LCRMSAudioProcessor;
    switch (soloSectionId)
    {
        case P::SOLO_GALAXY:
            return { P::ID_LCR_ENABLED, P::ID_LCR_SENS, P::ID_LCR_BLEND, P::ID_LCR_HORIZON,
                     P::ID_GALAXY_MOD, P::ID_GALAXY_DEPTH };
        case P::SOLO_TIMEWARP:
            return { P::ID_DRIFT_ON, P::ID_DRIFT, P::ID_BEND, P::ID_TIMEWARP_BALANCE, P::ID_TIMEWARP_MOD, P::ID_TIMEWARP_DEPTH,
                     P::ID_PARALLAX_MODE, P::ID_PARALLAX_AMOUNT };
        case P::SOLO_POLARITY:
            return { P::ID_POL_ON, P::ID_POL_L, P::ID_POL_R, P::ID_POL_POS };
        case P::SOLO_DIMENSION:
            return { P::ID_WIDTHBOOST_ON, P::ID_SIDE_WIDTH, P::ID_SIDE_BOOST, P::ID_DIMENSION_MOD, P::ID_DIMENSION_DEPTH };
        case P::SOLO_HYPERDRIVE:
            return { P::ID_FLOW_ON, P::ID_MOVEMENT, P::ID_SPEED, P::ID_SPEED_RATE, P::ID_SPEED_SYNC, P::ID_PULSE, P::ID_HYPERDRIVE_MOD, P::ID_HYPERDRIVE_DEPTH };
        case P::SOLO_POSITION:
            return { P::ID_POS_ON, P::ID_POS_OFFSET, P::ID_POS_WIDTH, P::ID_POS_DISTANCE, P::ID_POS_ELEVATE, P::ID_POSITION_MOD, P::ID_POSITION_DEPTH };
        case P::SOLO_RAY:
            return { P::ID_RAY_ON, P::ID_RAY_STRENGTH, P::ID_RAY_RATE, P::ID_RAY_PAIR };
        default:
            return {};
    }
}

// ===== Settings "Technical Labels" =====
// Zwei Namensschichten fuer dieselben Regler: der Vibe-Name (GALAXY, PARALLAX,
// Orbit, Drift) verkauft, der technische sagt, was passiert. Beides in einer
// Zeile unterzubringen war nie moeglich - also ein Schalter statt eines
// Kompromisses. Die Parameter-IDs bleiben unveraendert, es ist reine Optik.
void LCRMSAudioProcessorEditor::applyLabelStyle()
{
    const bool t = technicalLabels;
    auto put = [t] (juce::Label& l, const char* vibe, const char* tech)
    {
        l.setText (t ? tech : vibe, juce::dontSendNotification);
    };

    put (lcrTitleLabel,        "GALAXY",     "LCR MATRIX");
    put (driftTitleLabel,      "PARALLAX",   "MICROPITCH");
    put (polTitleLabel,        "ECLIPSE",    "POLARITY");
    put (widthBoostTitleLabel, "DIMENSION",  "MID-SIDE");
    put (flowTitleLabel,       "HYPERDRIVE", "AUTOPAN");
    put (rayTitleLabel,        "RAYE",       "PHASER");

    put (gravityLabel,   "Gravity", "C-Weight");
    put (orbitLabel,     "Orbit",   "L/R");
    put (driftLabel,     "Drift",   "Haas");
    put (bendLabel,      "Shift",   "Detune");
    put (offsetLabel,    "Tilt",    "Pan");
    put (sideWidthLabel, "Size",    "Width");
    put (sideBoostLabel, "Boost",   "Sides");
    put (movementLabel,  "Flow",    "Amount");
    put (horizonLabel,   "Regain",  "HF Regain");
    put (distanceLabel,  "Depth",   "Distance");
    // Der globale Knopf schaltet dieselbe Engine (User Runde 56), und der
    // Settings-Eintrag meint denselben Schalter (User Runde 57).
    globalGalaxyActivateButton.setButtonText (t ? "LCR" : "GALAXY");
    settingsPanel.behavBtn[0].setButtonText (t ? "LCR On Startup (Latency)" : "Galaxy On Startup (Latency)");
    globalGalaxyActivateButton.setTooltip (t ? "LCR engine: needed for the L/C/R split. Switching it on adds latency"
                                             : "Galaxy engine: needed for L/C/R extraction. Switching it on adds latency");
    // Amount und Speed heissen in beiden Welten gleich.
    // EARLY/LATE heisst technisch PRE/POST (User Runde 100) - der Text steht
    // live in timerCallback(), deshalb hier nur der Anstoss.

    // ===== HINWEISZEILE (Runde 60, User: "komplett ueberpruefen, stimmt
    // alles noch? und auch die technical Terms darin aktualisieren") =====
    // Alles, was einen Sektions- oder Reglernamen nennt, steht hier - und nur
    // hier. Sonst laufen Beschriftung und Hinweis irgendwann auseinander,
    // genau das war bei TIMEWARP passiert (die Sektion heisst seit Langem
    // PARALLAX).
    auto tipFor = [] (juce::SettableTooltipClient& c, const juce::String& text) { c.setTooltip (text); };
    tipFor (lcrTitleLabel,        t ? "LCR MATRIX: pulls the centre out of the stereo image and treats L, C and R apart"
                                    : "GALAXY: pulls the centre out of the stereo image and treats L, C and R apart");
    tipFor (polTitleLabel,        t ? "POLARITY: flips the phase of one channel at a chosen point in the chain"
                                    : "ECLIPSE: flips the phase of one channel at a chosen point in the chain");
    tipFor (driftTitleLabel,      t ? "MICROPITCH: opens a mono-ish sound into a wide one - a little delay, a little detune"
                                    : "PARALLAX: opens a mono-ish sound into a wide one - a little delay, a little detune");
    tipFor (widthBoostTitleLabel, t ? "MID-SIDE: the width stage - how far the sides reach and how much weight they carry"
                                    : "DIMENSION: the width stage - how far the sides reach and how much weight they carry");
    tipFor (flowTitleLabel,       t ? "AUTOPAN: slow automatic movement through the stereo field"
                                    : "HYPERDRIVE: slow automatic movement through the stereo field");
    tipFor (rayTitleLabel,        t ? "PHASER: a specially tuned phaser that moves the image instead of the tone"
                                    : "RAYE: a specially tuned phaser that moves the image instead of the tone");

    tipFor (gravitySlider,   t ? "C-Weight: how strongly the centre is separated from the sides"
                               : "Gravity: how strongly the centre is separated from the sides");
    tipFor (orbitSlider,     t ? "L/R: how much of the sides comes back in. All the way down is centre only"
                               : "Orbit: how much of the sides comes back in. All the way down is centre only");
    tipFor (horizonSlider,   t ? "HF Regain: how much of the level the split takes away comes back"
                               : "Regain: how much of the level the split takes away comes back");
    tipFor (sideWidthSlider, t ? "Width: how far the image reaches. Below 100 % pulls it in, above pushes it out"
                               : "Size: how far the image reaches. Below 100 % pulls it in, above pushes it out");
    tipFor (sideBoostSlider, t ? "Sides: gives the sides weight without touching what sits in the centre"
                               : "Boost: gives the sides weight without touching what sits in the centre");
    tipFor (distanceSlider,  t ? "Distance: left moves the sound back into the room, right pulls it close"
                               : "Depth: left moves the sound back into the room, right pulls it close");
    tipFor (movementSlider,  t ? "Amount: how far the sound travels left and right"
                               : "Flow: how far the sound travels left and right");
    tipFor (rayPairButton,   t ? "Link: follow Autopan at half its speed"
                               : "Link: follow Hyperdrive at half its speed");
    tipFor (parallaxAmountSlider, "Amount: the single dial for this style - from off to the full effect");
    tipFor (rayAmountSlider,      "Amount: how strong the movement is");
    tipFor (categoryButton,  "Smart profile: click for the next one, Cmd-click to go back. The dice then stays inside what fits that source");
    tipFor (globalChaosButton, "Smart: rolls a new setting and decides which sections belong in it");
    applyHintTexts();   // Runde 133

    if (getWidth() > 0)
    {
        resized();
        // Bug (User Runde 65): schaltet man die Beschriftung bei GEOEFFNETEM
        // Fenster um, standen die neuen Namen abgeschnitten da - erst nach
        // Schliessen und Neuoeffnen sassen sie richtig. Grund ist der alte
        // JUCE-Stolperstein: resized() auf dem Editor ruft das resized() des
        // Kindes NICHT auf, solange dessen Bounds gleich bleiben - und genau
        // dort rechnet fitTitle die Schriftgroesse neu.
        content.resized();
    }
    repaint();
}

// Macht einen Sektions-Titel zusaetzlich zum Power-Icon klickbar, um die
// Sektion an/aus zu schalten (User-Feedback). Speichert Ziel-Parameter und
// Solo-Wert als Component-Properties, damit mouseUp() generisch (ohne feste
// Label-Liste) herausfinden kann, was zu tun ist.
void LCRMSAudioProcessorEditor::setupClickableTitle (juce::Label& label, const juce::String& powerParamId, int soloValue)
{
    label.getProperties().set ("titlePowerParam", powerParamId);
    label.getProperties().set ("titleSoloValue", soloValue);
    label.setInterceptsMouseClicks (true, false);
    // (Kein eigener addMouseListener mehr - der Editor lauscht seit dem
    //  View-Panel-Schliessen auf ALLE Kinder von content; doppelt registriert
    //  kaeme jeder Klick zweimal an.)
    label.setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void LCRMSAudioProcessorEditor::closeViewPanel()
{
    if (! viewPanel.isVisible())
        return;
    viewGearButton.setToggleState (false, juce::dontSendNotification);
    // Kurzer Fade-out (User); die Abdunklung der Spalte folgt in paintOverContent.
    juce::Desktop::getInstance().getAnimator().fadeOut (&viewPanel, 140);
    content.repaint();
}

void LCRMSAudioProcessorEditor::mouseUp (const juce::MouseEvent& e)
{
    auto* comp = e.eventComponent;
    if (comp == nullptr)
        return;

    // Runde 58 (User: "demo klicken geht immer noch nicht"): der DEMO-
    // Aufkleber ist jetzt wirklich anklickbar und fuehrt dorthin, wo es
    // SpaceX zu kaufen gibt. Die Flaeche kommt aus paintContent().
    if (! demoChipArea.isEmpty() && ! processor.licensed.load (std::memory_order_relaxed))
    {
        const auto pos = e.getEventRelativeTo (&content).position;
        if (demoChipArea.expanded (4.0f).contains (pos))
        {
            // Runde 60 (User): erst fragen, was der Nutzer eigentlich will -
            // direkt in den Browser zu springen waere uebergriffig. Eigene
            // Knopfbeschriftungen, damit man vor dem Klick weiss, was kommt.
            demoDialog = std::make_unique<juce::AlertWindow> ("SpaceX Demo",
                "This copy runs in demo mode and goes quiet for a moment every 50 seconds.\n\n"
                "Already bought it? Enter your serial. Otherwise have a look at the shop.",
                juce::MessageBoxIconType::NoIcon);
            demoDialog->addButton ("Enter Serial",  1);
            demoDialog->addButton ("Buy SpaceX",    2);
            demoDialog->addButton ("Continue Demo", 0, juce::KeyPress (juce::KeyPress::escapeKey));
            styleNameDialog (*demoDialog);
            // Review 1.0.1: JUCE ruft den Rueckruf auch dann noch (asynchron) auf,
            // wenn der Dialog beim Schliessen des Plugin-Fensters mit dem Editor
            // geloescht wird - dann darf 'this' nicht mehr benutzt werden.
            demoDialog->enterModalState (true, juce::ModalCallbackFunction::create ([this, safe = juce::Component::SafePointer<LCRMSAudioProcessorEditor> (this)] (int r)
            {
                if (safe == nullptr)
                    return;
                demoDialog.reset();
                if (r == 1)      promptActivate();
                else if (r == 2) juce::URL (spacexContact::shopUrl).launchInDefaultBrowser();
            }), false);
            return;
        }
    }

    // Footer: Klick auf die Beschriftung MONO/DRY schaltet wie das Icon
    // (User: "Klick Bereich erweitern -> auch auf SCHRIFT soll on off machen").
    if (comp == &monoCheckLabel) { monoCheckButton.triggerClick(); return; }
    if (comp == &monoDryLabel)   { if (monoDryButton.isEnabled()) monoDryButton.triggerClick(); return; }
    if (comp == &autoGainLabel)  { autoGainButton.triggerClick(); return; }   // Runde 150 (User)

    if (! comp->getProperties().contains ("titlePowerParam"))
        return;

    const juce::String paramId = comp->getProperties()["titlePowerParam"].toString();
    const int soloValue = (int) comp->getProperties()["titleSoloValue"];

    // Runde 128 (User): Cmd + Shift + Klick auf den Sektionsnamen setzt die
    // ganze Sektion auf ihre Standardwerte zurueck. An/Aus bleibt, wie es
    // ist - Reset heisst "Einstellungen zurueck", nicht "ausschalten".
    if (e.mods.isCommandDown() && e.mods.isShiftDown())
    {
        using P = LCRMSAudioProcessor;
        juce::StringArray ids = sectionParamIds (soloValue);
        switch (soloValue)
        {
            case P::SOLO_GALAXY:    ids.add (P::ID_MS_EQ_LCR); break;
            case P::SOLO_DIMENSION: ids.addArray ({ P::ID_MS_EQ, P::ID_MS_EQ_ON, P::ID_MS_EQ_AMT }); break;
            case P::SOLO_RAY:       ids.addArray ({ P::ID_RAY_AMOUNT, P::ID_RAY_CHAR, P::ID_RAY_FAST }); break;
            default: break;
        }
        ids.removeString (paramId);   // der An/Aus-Schalter der Sektion
        for (const auto& id : ids)
            if (auto* prm = processor.apvts.getParameter (id))
                if (std::abs (prm->getValue() - prm->getDefaultValue()) > 1.0e-6f)
                    prm->setValueNotifyingHost (prm->getDefaultValue());
        return;
    }

    // Runde 48 (User): Cmd + Klick auf den Sektionsnamen schaltet Solo statt
    // an/aus - dieselbe Wirkung wie das (ausgeblendete) Solo-Icon.
    if (e.mods.isCommandDown())
    {
        if (auto* sp = processor.apvts.getParameter (LCRMSAudioProcessor::ID_SOLO_SECTION))
        {
            const int current = juce::jlimit (0, LCRMSAudioProcessor::SOLO_MAX,
                                              (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_SOLO_SECTION)->load()));
            const int next = (current == soloValue) ? LCRMSAudioProcessor::SOLO_NONE : soloValue;
            sp->setValueNotifyingHost ((float) next / (float) LCRMSAudioProcessor::SOLO_MAX);
            if (next != LCRMSAudioProcessor::SOLO_NONE)
                if (auto* onP = processor.apvts.getParameter (paramId))
                    if (onP->getValue() < 0.5f)
                        onP->setValueNotifyingHost (1.0f);
        }
        return;
    }

    // Runde 118 (User): ist genau diese Sektion solo, beendet ein normaler
    // Klick auf den Namen das Solo - wie Cmd-Klick, leichter zu merken. Die
    // Sektion bleibt dabei an (kein An/Aus).
    {
        const int current = juce::jlimit (0, LCRMSAudioProcessor::SOLO_MAX,
                                          (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_SOLO_SECTION)->load()));
        if (current != LCRMSAudioProcessor::SOLO_NONE && current == soloValue)
        {
            if (auto* sp = processor.apvts.getParameter (LCRMSAudioProcessor::ID_SOLO_SECTION))
                sp->setValueNotifyingHost (sp->convertTo0to1 ((float) LCRMSAudioProcessor::SOLO_NONE));
            return;
        }
        // Runde 123 (User): waehrend eine ANDERE Sektion solo ist, schaltet
        // der Klick auf den Namen nichts um - man saehe es nicht, und nach
        // dem Solo waeren ploetzlich Sektionen an. Cmd-Klick verlegt das Solo.
        if (current != LCRMSAudioProcessor::SOLO_NONE)
            return;
    }

    if (auto* param = processor.apvts.getParameter (paramId))
    {
        const bool wasOn = param->getValue() > 0.5f;
        param->setValueNotifyingHost (wasOn ? 0.0f : 1.0f);
        // Solo bleibt stehen (User) - siehe setupPowerButton().
        juce::ignoreUnused (soloValue);
        if (! wasOn && paramId == LCRMSAudioProcessor::ID_LCR_ENABLED) // wurde gerade eingeschaltet
            activateGalaxyIfNeeded();
    }
}

// "Show Advanced Modulation" (Runde 37): Mod-Icon + Tiefe-Regler in den
// Sektionskoepfen nur bei Bedarf. Ausgeblendet laeuft die Modulation weiter
// wie eingestellt; LIFE skaliert sie gemeinsam, die Mod-Punkte auf den
// Reglern bleiben sichtbar.
void LCRMSAudioProcessorEditor::applyAdvancedModVisibility()
{
    // Runde 101 (User): es gibt sie nicht mehr - weder Icon noch Regler.
    // Im Tune-Build bleiben die Tiefe-Regler stehen (layoutHeader setzt sie).
    for (auto* c : std::initializer_list<juce::Component*> {
             &galaxyModButton, &driftModButton, &dimensionModButton, &hyperdriveModButton
           #if ! SPACEX_TUNE
             , &galaxyModDepthSlider, &driftModDepthSlider, &dimensionModDepthSlider, &hyperdriveModDepthSlider
           #endif
         })
    {
        c->setVisible (false);
        c->setBounds ({});
    }
}

void LCRMSAudioProcessorEditor::activateGalaxyIfNeeded()
{
    if (suppressGalaxyAutoArm)
        return;
    if (auto* galaxyActivateParam = processor.apvts.getParameter (LCRMSAudioProcessor::ID_GALAXY_ACTIVATE))
        if (galaxyActivateParam->getValue() < 0.5f)
            galaxyActivateParam->setValueNotifyingHost (1.0f);
}

// Workflow-Beschleunigung (User-Idee): "Wenn Mod Icon off ist, dann ist ja
// der Mod Regler grau. Wenn ich jetzt am Mod Regler drehe springt Mod Icon
// automatisch an." Reagiert bewusst auf onValueChange (echte
// Wertaenderung), nicht auf Klick/Fokus - ein versehentliches Antippen
// ohne tatsaechliche Drehung loest also nichts aus.
void LCRMSAudioProcessorEditor::wireModAutoEnable (juce::Slider& depthSlider, juce::TextButton& modButton, const juce::String& modParamId)
{
    depthSlider.onValueChange = [this, &modButton, modParamId]
    {
        if (! modButton.getToggleState())
            if (auto* modParam = processor.apvts.getParameter (modParamId))
                modParam->setValueNotifyingHost (1.0f);
    };
}

// Liest die beiden Start-Standardwerte aus dem Hamburger-Menue und wendet
// sie als Startzustand fuer dieses Fenster an. 2. Anlauf (User-Wunsch:
// "Gonio und Stars entfernen in der global Leiste. Dafuer im Menu einfach
// je einen Eintrag hinzufuegen.") - die beiden eigenen Buttons in der
// globalen Zeile sind komplett entfallen, das Hamburger-Menue ist jetzt
// die alleinige und SOFORT wirksame Kontrolle (siehe showPresetMenu()),
// nicht mehr nur ein Start-Standard fuers naechste Oeffnen.
void LCRMSAudioProcessorEditor::applyVisualsVisibility()
{
    juce::PropertiesFile props (LCRMSAudioProcessor::appPropertiesOptions());
    // Neuer Schluessel "uiTheme3" (7 Themes, andere Indizes): Standard Sci-Fi (User).
    uiLayoutRef() = juce::jlimit (0, 2, props.getIntValue ("uiLayout", 0));
    setUiTheme (props.getIntValue ("uiTheme4", 3), false);
    uiBrightnessRef() = juce::jlimit (0.0f, 1.0f, (float) props.getDoubleValue ("uiBrightness", 0.0));   // Runde 174
    applyLayoutMode();
    goniometerVisualsOn = ! props.getBoolValue ("goniometerDisabledDefault", false);
    starVisualsOn       = ! props.getBoolValue ("spaceVisualsDisabledDefault", false);
    starfieldModMovementOn     = ! props.getBoolValue ("starfieldModMovementDisabled", false);
    starfieldReducedAnimations = false;   // Reduce Anim. entfernt (Density uebernimmt)

    // Show dreistufig (Off/Stars/Full); alter Bool-Schalter wird uebernommen.
    const int starMode = props.containsKey ("viewStarMode") ? props.getIntValue ("viewStarMode", 2) : (starVisualsOn ? 2 : 0);
    starVisualsOn = starMode > 0;

    goniometer.setGoniometerActive (goniometerVisualsOn);
    goniometer.setStarfieldMode (starMode);
    goniometer.setModMovementEnabled (starfieldModMovementOn);
    goniometer.setReducedAnimations (starfieldReducedAnimations);

    // View-Panel mit denselben Werten fuellen (ohne Callback-Sturm), plus
    // die vier Darstellungsregler.
    viewPanel.gonioBtn.setToggleState (goniometerVisualsOn, juce::dontSendNotification);
    viewPanel.setStarModeIndex (starMode);
    viewPanel.setPhotoIndex (props.getIntValue ("viewPhoto", 1));
    viewPanel.setGravityIndex (props.getIntValue ("viewGravity", 0));
    viewPanel.modMoveBtn.setToggleState (starfieldModMovementOn, juce::dontSendNotification);
    // Dim ist reiner Sitzungszustand: beim Oeffnen IMMER aus (User: "default:
    // dim off") - gespeicherte alte Werte werden absichtlich ignoriert.
    viewPanel.dimSlider.setValue      (1.0, juce::dontSendNotification);
    viewPanel.setGonioStyleIndex (props.getIntValue ("viewGonioStyle", 2));      // Glow
    viewPanel.speedSlider.setValue    (props.getDoubleValue ("viewSpeed", 1.0),     juce::dontSendNotification);
    viewPanel.densitySlider.setValue  (props.getDoubleValue ("viewDensity", 1.0),   juce::dontSendNotification);
    // Gonio-Farbe beim Oeffnen = Standardfarbe des Themes (User), nicht der
    // zuletzt gewaehlte Wert; die Wahl gilt fuer die Sitzung.
    viewPanel.setGonioColourIndex (gonioColourForTheme (uiThemeIndex));
    viewPanel.starsBtn.setToggleState (props.getBoolValue ("viewStars", true), juce::dontSendNotification);
    viewPanel.linesBtn.setToggleState (props.getBoolValue ("viewStarLines", true), juce::dontSendNotification);
    applyViewSettings (false);
}

// Gonio-Standardfarbe je Theme: seit Runde 19 gibt es nur noch Theme und
// White - der Standard ist immer "Theme", der Grundton kommt aus
// GoniometerComponent::themeTraceColour().
int LCRMSAudioProcessorEditor::gonioColourForTheme (int)
{
    return 0;
}

// ===== VIEW-PANEL anwenden (und optional speichern) =====
// Wird bei jeder Aenderung im Panel gerufen. Die Schalter schreiben dieselben
// Properties wie frueher die Menue-Eintraege, damit ein gespeicherter
// Standard weiterhin gilt.
void LCRMSAudioProcessorEditor::applyViewSettings (bool persist)
{
    goniometerVisualsOn        = viewPanel.gonioBtn.getToggleState();
    starVisualsOn              = viewPanel.starModeIndex() > 0;
    starfieldModMovementOn     = viewPanel.modMoveBtn.getToggleState();
    starfieldReducedAnimations = false;

    goniometer.setGoniometerActive (goniometerVisualsOn);
    goniometer.setStarfieldMode (viewPanel.starModeIndex());
    goniometer.setPhoto (viewPanel.photoIndex());
    goniometer.setGravityMode (viewPanel.gravityIndex());
    viewPanel.updateModMoveEnabled();
    goniometer.setModMovementEnabled (starfieldModMovementOn);
    goniometer.setReducedAnimations (starfieldReducedAnimations);
    goniometer.setViewSpeed ((float) viewPanel.speedSlider.getValue());
    goniometer.setViewDensity ((float) viewPanel.densitySlider.getValue());
    goniometer.setViewShine ((float) viewPanel.dimSlider.getValue());
    viewPanel.syncDimBtn();
    goniometer.setGonioColour (viewPanel.gonioColourIndex(), 0.85f);   // Intensity fest (Standard-Look)
    goniometer.setTwinkleVisible (viewPanel.starsBtn.getToggleState());
    goniometer.setStarLinesVisible (viewPanel.linesBtn.getToggleState());
    goniometer.setStarColour  (0, 0.25f);   // Sternlinien: fest Blau/pastell (Color+Intensity entfernt, User)
    goniometer.setGonioStyle (viewPanel.gonioStyleIndex());
    // Blur fest je Stil (User): Lines weich (0,76), Glow knapp (0,14).
    goniometer.setGonioGlow (viewPanel.gonioStyleIndex() == 1 ? 0.76f : 0.14f);
    goniometer.setGonioSpeed (1);   // fest Mid (User: "dann einfach Medium")
    goniometer.repaint();
    storeViewSettingsInState();

    if (persist)
    {
        juce::PropertiesFile props (LCRMSAudioProcessor::appPropertiesOptions());
        props.setValue ("goniometerDisabledDefault", ! goniometerVisualsOn);
        props.setValue ("spaceVisualsDisabledDefault", ! starVisualsOn);
        props.setValue ("viewStarMode", viewPanel.starModeIndex());
        props.setValue ("viewPhoto", viewPanel.photoIndex());
        props.setValue ("viewGravity", viewPanel.gravityIndex());
        props.setValue ("starfieldModMovementDisabled", ! starfieldModMovementOn);
        props.setValue ("viewShine2", viewPanel.dimSlider.getValue());
        props.setValue ("viewSpeed", viewPanel.speedSlider.getValue());
        props.setValue ("viewDensity", viewPanel.densitySlider.getValue());
        props.setValue ("viewGonioColour", viewPanel.gonioColourIndex());
        props.setValue ("viewStars", viewPanel.starsBtn.getToggleState());
        props.setValue ("viewStarLines", viewPanel.linesBtn.getToggleState());
        props.setValue ("viewGonioStyle", viewPanel.gonioStyleIndex());
        props.saveIfNeeded();
    }
}

// Klick ins Sternenfeld: Farbe weiter, nach Gold aus, danach wieder Blau.
// Klick ins Sternenfeld: naechstes Hintergrundfoto, nach dem letzten "None"
// (User: "Klick auf Starfield aendert Hintergrundbild"). Die Gonio-Farbe
// wird nur noch im View-Panel gewaehlt.
void LCRMSAudioProcessorEditor::cycleGonioColourFromField (int action)
{
    switch (action)
    {
        case 2:   // Feldmitte: Goniometer an/aus (nur in Space sinnvoll)
            if (viewPanel.starModeIndex() == 2)
                viewPanel.gonioBtn.setToggleState (! viewPanel.gonioBtn.getToggleState(), juce::dontSendNotification);
            break;
        case 3:   // Sonne/Mond: Space <-> Gonio-Ansicht ("Licht an / aus")
            viewPanel.setStarModeIndex (viewPanel.starModeIndex() == 2 ? 1 : 2);
            break;
        case 4:   // Cmd+Mitte: naechste Spurfarbe
            viewPanel.setGonioColourIndex ((viewPanel.gonioColourIndex() + 1) % ViewPanelComponent::kGonioColours);
            break;
        case 5:   // Shift+Mitte: Look (Glow <-> Lines)
            viewPanel.setGonioStyleIndex (viewPanel.gonioStyleIndex() == 1 ? 2 : 1);
            break;
        case 6:   // Sonne (ohne Cmd): Shine min <-> max (User: "Sonne hatte min/max Shine gemacht")
            viewPanel.dimSlider.setValue (viewPanel.dimSlider.getValue() < 0.5 ? 1.0 : 0.0, juce::sendNotificationSync);
            break;
        default:
            break;   // Fotos/Gravity sind fest je Theme (User)
    }
    applyViewSettings (false);
}

bool LCRMSAudioProcessorEditor::isVolLocked()
{
    return juce::PropertiesFile (LCRMSAudioProcessor::appPropertiesOptions()).getBoolValue ("lockVol", false);
}

// Rechtsklick auf Mix oder Vol sperrt den Regler: Presets, A/B, Reset und
// Smart lassen ihn dann stehen. Ein kleines Schloss ueber dem Regler zeigt es
// an (siehe drawRotarySlider, Property "knobLocked"). Ersetzt den frueheren
// Menuepunkt "Lock Mix Knob" - die Einstellung gehoert an den Regler, nicht
// in eine Liste (User).
void LCRMSAudioProcessorEditor::toggleKnobLock (const char* propName, juce::Slider& knob)
{
    juce::PropertiesFile p (LCRMSAudioProcessor::appPropertiesOptions());
    const bool locked = ! p.getBoolValue (propName, false);
    p.setValue (propName, locked);
    p.saveIfNeeded();
    knob.getProperties().set ("knobLocked", locked);
    knob.repaint();
}

bool LCRMSAudioProcessorEditor::isMixLocked()
{
    return juce::PropertiesFile (LCRMSAudioProcessor::appPropertiesOptions()).getBoolValue ("lockMix", false);
}

// ===== HOVER HINTS =====
// Kurze, klare englische Hinweise fuer jedes Bedienelement (Menue "Show
// Hover Hints"). Die Texte sind immer gesetzt; ob sie erscheinen, entscheidet
// allein, ob das Tooltip-Fenster existiert. Dadurch aendert sich am Layout
// (Header-Breite!) nichts.
// ===== LAYOUT-MODI =====
// "Flat" nimmt die Sektionskaesten weg, "Outline" laesst nur den Rahmen
// stehen (siehe drawGroup). Der frueher hier eingebaute "Simple"-Modus, der
// Solo/Lock/On-Off/Titel ausgeblendet hat, ist wieder raus (User: "macht
// optisch gar keinen Sinn") - diese Funktion stellt jetzt nur noch sicher,
// dass nach einem Update aus jener Zeit alles wieder sichtbar ist.
void LCRMSAudioProcessorEditor::applyLayoutMode()
{
    // ACHTUNG (Runde 30): hier standen frueher auch die Solo- und
    // Power-Knoepfe drin. Beide sind dauerhaft unsichtbar, diese Schleife hat
    // sie aber bei jedem Layout-Wechsel wieder eingeblendet - bei Vision und
    // RAYE blieb das "S" dadurch sichtbar, weil deren Kopf nicht durch
    // layoutHeader() laeuft (das den Knopf jedes Mal wieder versteckt hat).
    // Wer hier etwas ergaenzt: nur Dinge, die wirklich sichtbar sein sollen.
    juce::Component* headerBits[] = {
        &lcrLockButton, &polLockButton, &driftLockButton, &widthBoostLockButton, &flowLockButton, &posLockButton, &rayLockButton,
        &lcrTitleLabel, &polTitleLabel, &driftTitleLabel, &widthBoostTitleLabel, &flowTitleLabel, &posTitleLabel, &rayTitleLabel
    };
    for (auto* c : headerBits)
        c->setVisible (true);
}

// ===== INFOZEILE (Runde 133, User: "neu machen - nur aktuelle Labels,
// einfache Erklaerung, Key Commands erwaehnen") =====
// Ein Satz, was es tut - danach, getrennt mit " · ", die Tastenkuerzel.
// Laeuft als LETZTES nach applyHoverHints() und applyLabelStyle(), damit
// keine alten Texte (Galaxy, Timewarp, ...) mehr durchkommen.
void LCRMSAudioProcessorEditor::applyHintTexts()
{
    auto tip = [] (juce::SettableTooltipClient& c, const juce::String& text) { c.setTooltip (text); };
    // Runde 153 (User: "nicht mehr jeden Command, das stoert - Doppelklick =
    // Default soll ueberall weg, das ist Basics"): Grundbedienung (Default per
    // Doppelklick, Klick = an/aus, Klick = weiter) steht nicht mehr in der
    // Zeile. Uebrig bleiben nur Befehle, die man nicht erraten kann.
    const juce::String knob;
    const juce::String section = juce::String::fromUTF8 (" · Cmd-click: solo · Cmd+Shift-click: reset section");
    const juce::String cycle   = juce::String::fromUTF8 (" · Cmd-click: previous");   // User: bleibt, kann man nicht erraten

    // Kopf
    tip (logoButton,                 "Bypass: switch the whole plugin off and on");
    tip (globalBypassButton,         "Bypass: switch the whole plugin off and on");
    tip (globalChaosButton,          "Smart: rolls a new setting. With a Smart profile it stays inside what suits that source. Paused during solo");
    tip (lifeSlider,                 "Life: how much everything moves - modulation depth for all sections. 0 = still" + knob);
    tip (globalModBypassButton,      "Mod: all modulation on or off");
    tip (globalGalaxyActivateButton, "LCR: starts the LCR Matrix engine. Adds latency while it runs");
    tip (undoButton,                 "Undo");
    tip (redoButton,                 "Redo");
    tip (presetMenuButton,           "Settings: themes, behaviour, preset folder");
    tip (presetPrevButton,           "Previous preset");
    tip (presetNextButton,           "Next preset");
    tip (presetNameButton,           "Presets: open the list");
    tip (globalSaveSizeButton,       "Save: store the current setting as a preset - pick the folder in the dialog");
    tip (presetDeleteButton,         "Delete the current preset");
    tip (globalABButton,             "A/B: switch between two settings to compare");
    tip (abCopyButton,               "Copy: copy the active side to the other one");
    tip (globalResetButton,          "Reset: back to the default setting");
    tip (categoryButton,             "Smart profile: Lead Vocal, Backings, Adlibs, Send FX - guides the Smart dice" + cycle);
    tip (catDots,                    "Smart profile: pick one directly");

    // Sektionen
    tip (lcrTitleLabel,        "LCR MATRIX: splits the image into left, centre and right" + section);
    tip (polTitleLabel,        "POLARITY: flips the phase of the left and/or right channel" + section);
    tip (driftTitleLabel,      "MICROPITCH: makes a mono sound wide with tiny time and pitch offsets" + section);
    tip (widthBoostTitleLabel, "MID-SIDE: width, side level and the Sides EQ" + section);
    tip (flowTitleLabel,       "AUTOPAN: moves the sound between left and right" + section);
    tip (rayTitleLabel,        "PHASER: a stereo phaser that moves the image" + section);
    for (auto* b : { &lcrLockButton, &polLockButton, &driftLockButton, &widthBoostLockButton, &flowLockButton, &posLockButton, &rayLockButton })
        tip (*b, "Lock: the Smart dice leaves this section alone");

    // LCR MATRIX
    tip (orbitSlider,   "L/R: level of the left and right parts. All the way down = centre only" + knob);
    tip (gravitySlider, "C-Weight: how strongly the centre is separated from the sides" + knob);
    tip (horizonSlider, "HF Regain: brings back the highs the split takes away. 0 = off" + knob);
    tip (lcrEqButton,   juce::String::fromUTF8 ("EQ \xe2\x86\x92 LCR: the Sides EQ works on centre and sides of the LCR Matrix instead of mid and side"));

    // POLARITY
    tip (polLButton,     juce::String::fromUTF8 ("L: flip the phase of the left channel · Cmd-click: only left"));
    tip (polRButton,     juce::String::fromUTF8 ("R: flip the phase of the right channel · Cmd-click: only right"));
    tip (polLinkButton,  "Link: switch L and R together");
    tip (polPos2Button,  "PRE/POST: flip before or after Micropitch and Mid-Side");

    // MICROPITCH
    tip (parallaxAmountSlider,     "Amount: how much of the style - from off to full" + knob);
    tip (parallaxModeButtons[0],   "Style: Velvet, Halo, Illusion, Double" + cycle);
    tip (pxModeDots,               "Style: pick one directly");

    // MID-SIDE
    tip (sideWidthSlider, "Width: how far the image reaches. Below 100 % narrower, above wider" + knob);
    tip (sideBoostSlider, "Sides: level of the sides, the centre stays as it is" + knob);
    tip (msEqButton,      "Sides EQ: Flat, Tight (cleans the lows), Clear (clean + air), Focus (calmer, brighter centre)" + cycle);
    tip (msEqDots,        "Sides EQ: pick one directly");
    tip (msEqPowerButton, "EQ on/off - compare with and without, the setting stays");
    tip (msEqAmtSlider,   "EQ amount: left = gentle (default), right = strongest");

    // AUTOPAN
    tip (movementSlider,  "Amount: how far the sound travels left and right" + knob);
    tip (pulseButton,     "Pulse: a smoother, pulse-like movement instead of a sine");
    tip (speedRateSlider, "Speed: how fast it moves. Locked while synced to bars" + knob);
    tip (syncButton,      "Sync: lock the speed to the song tempo");
    tip (speedBox,        "Rate: note length of one movement while Sync is on");

    // PHASER
    tip (rayAmountSlider, "Amount: how strong the phaser is" + knob);
    tip (rayCharButton,   "Character: Sweep, Shimmer, Spin, Swirl" + cycle);
    tip (rayModeDots,     "Character: pick one directly");
    tip (rayFastButton,   "Fast: runs the character a bit quicker");
    tip (rayPairButton,   "Link: follow Autopan at half its speed");

    // Fuss
    tip (monoCheckButton, "Mono: listen to the result in mono");
    tip (monoDryButton,   "Dry: while in mono, compare with the unprocessed input");
    tip (mixSlider,       juce::String::fromUTF8 ("Mix: blend between original and processed · Right-click: lock against presets, A/B, Reset and Smart") + knob);
    tip (panSlider,       "Pan: balance at the very end" + knob);
    tip (volSlider,       juce::String::fromUTF8 ("Vol: output level, plus or minus 6 dB · Right-click: lock against presets and Reset") + knob);
    tip (autoGainButton,  "AG: auto gain - matches output to input level for a fair bypass comparison");
    tip (goniometer,      juce::String::fromUTF8 ("Starfield: click to switch the scope on or off · Cmd-click centre: trace colour · Shift-click: look"));
    tip (viewGearButton,  "View: display settings for the starfield");
    tip (correlationMeter, "Correlation: right of centre is mono-safe, left of it cancels in mono");
    tip (helpButton,      "Help: show a short explanation for whatever the mouse is over");
}

void LCRMSAudioProcessorEditor::applyHoverHints()
{
    auto tip = [] (juce::SettableTooltipClient& c, const char* text) { c.setTooltip (text); };

    // Durchgaengiges Muster: "Name: was es tut." Die Bezeichnung bis zum
    // Doppelpunkt wird fett gesetzt (siehe CustomLookAndFeel::drawTooltip).

    // Header
    tip (globalBypassButton,         "Bypass: mute the whole plugin and pass the input through");
    tip (globalChaosButton,          "Smart: rolls a new setting and decides which sections belong in it");
    tip (globalChaosSectionsButton,  "Smart+: rolls a new setting and decides which sections belong in it");
    tip (globalBreatheButton,        "Breathe: injects life into every section by bringing modulation in at fresh depths");
    tip (globalModBypassButton,      "Mod: switch every modulation on or off at once");
    tip (lifeSlider,                 "Life: how much all modulation moves, scaled together");
    tip (globalGalaxyActivateButton, "Galaxy engine: needed for L/C/R extraction. Switching it on adds latency");
    tip (undoButton,                 "Undo: step back through your changes");
    tip (redoButton,                 "Redo: step forward again");
    tip (presetMenuButton,           "Settings: themes, layout, Smart options and everything else. The panel stays open");
    tip (presetNameButton,           "Preset: the one loaded right now. Click to pick another");
    tip (presetPrevButton,           "Previous preset");
    tip (presetNextButton,           "Next preset");
    tip (globalSaveSizeButton,       "Save: store the current settings as a preset. Name it Default to overwrite the default");
    tip (presetDeleteButton,         "Delete: remove the loaded preset");
    tip (globalABButton,             "A / B: switch between two versions of your settings to compare them");
    tip (abCopyButton,               "Copy: send the current settings to the other A/B slot");
    tip (globalResetButton,          "Reset: load the Default preset again");
    tip (logoButton,                 "SpaceX: click the logo for the back panel");

    // Galaxy
    tip (lcrTitleLabel,   "GALAXY: pulls the centre out of the stereo image and treats L, C and R apart");
    tip (gravitySlider,   "Gravity: how strongly the centre is separated from the sides");
    tip (orbitSlider,     "Orbit: down keeps the centre only, up keeps the sides only, middle is the original");
    tip (galaxyFilterButton, "Focus: Galaxy only separates inside the focus range. Click to let it work across the whole spectrum");
    tip (parallaxHpButton,   "Bass Protect for Parallax: keeps everything below 120 Hz out of the widening. The low end stays exactly as it came in");
    tip (galaxyModButton, "Mod: switch modulation on or off for this section");
    tip (galaxyModDepthSlider, "Depth: how far the modulation moves this section");

    // Polarity
    tip (polTitleLabel,  "POLARITY: flips the phase of one channel at a chosen point in the chain");
    tip (polLButton,     "L: flip the left channel. Cmd-click: only left");
    tip (polRButton,     "R: flip the right channel. Cmd-click: only right");
    tip (polLinkButton,  "Link: switch L and R together");
    tip (polPos2Button,  technicalLabels ? "Pre / Post: click to switch. Pre flips right after the LCR stage, Post at the end after the width"
                                        : "Early / Late: click to switch. Early flips right after Galaxy, Late at the end after the width");

    // Timewarp
    tip (driftTitleLabel,   "TIMEWARP: opens a mono-ish sound into a wide one by pulling left and right apart in time and in pitch");
    tip (driftSlider,       "Drift: sends one side a few milliseconds late, so the image leans and widens - the Haas trick, done carefully");
    tip (bendSlider,        "Shift: detunes the two sides against each other by a few cents. Width without any delay");
    tip (driftBalanceButton, "Balance: evens out the loudness shift that Drift causes");
    tip (driftModButton,    "Mod: switch modulation on or off for this section");
    tip (driftModDepthSlider, "Depth: how far the modulation moves this section");

    // Dimension
    tip (widthBoostTitleLabel, "DIMENSION: the width stage - how far the sides reach and how much weight they carry");
    tip (sideWidthSlider,   "Size: how far the image reaches. Below 100 % pulls it in, above 100 % pushes it out");
    tip (sideBoostSlider,   "Boost: gives the sides weight without touching what sits in the centre");
    tip (dimFilterButton, "Focus: Dimension only widens inside the focus range. Click to let it work across the whole spectrum");
    tip (dimensionModButton, "Mod: switch modulation on or off for this section");
    tip (dimensionModDepthSlider, "Depth: how far the modulation moves this section");

    // Hyperdrive
    tip (flowTitleLabel,   "HYPERDRIVE: slow automatic movement through the stereo field");
    tip (movementSlider,   "Flow: how far the sound travels left and right");
    tip (pulseButton,      "Pulse: a smoother, pulse-like movement instead of a sine");
    tip (speedRateSlider,  "Speed: movement rate in Hz");
    tip (syncButton,       "Sync: lock the speed to the host tempo");
    tip (speedBox,         "Rate: the note length used while Sync is on");
    tip (hyperdriveModButton, "Mod: switch modulation on or off for this section");
    tip (hyperdriveModDepthSlider, "Depth: how far the modulation moves this section");

    // Vision
    tip (posTitleLabel,   "VISION: where the sound sits in the picture");
    tip (offsetSlider,    "Tilt: shifts the whole image left or right");
    tip (posWidthSlider,  "Width: final width of the image");
    tip (distanceSlider,  "Depth: left moves it back into the room and darkens it, right pulls it close and opens it up");
    tip (elevateSlider,   "Elevate: lifts the sound up and forward, out from behind the rest of the mix");
    tip (posFilterButton, "Focus: Vision only widens inside the focus range. Click to let it work across the whole spectrum");
    tip (positionModButton, "Mod: switch modulation on or off for this section");
    tip (positionModDepthSlider, "Depth: how far the modulation moves this section");

    // Raye
    tip (rayTitleLabel,     "RAYE: a specially tuned phaser that moves the image instead of the tone");
    tip (rayStrengthButton, "Strength: click to step through Light, Medium, Strong and Off");
    tip (rayRateSlider,     "Speed: how fast the movement cycles");
    tip (rayPairButton,     "Link: follow Hyperdrive at half its speed");

    // Section headers (shared)
    for (auto* b : { &lcrSoloButton, &polSoloButton, &driftSoloButton, &widthBoostSoloButton, &flowSoloButton, &posSoloButton, &raySoloButton })
        tip (*b, "Solo: hear this section on its own");
    for (auto* b : { &lcrLockButton, &polLockButton, &driftLockButton, &widthBoostLockButton, &flowLockButton, &posLockButton, &rayLockButton })
        tip (*b, "Lock: Smart and Breathe leave this section alone");

    // Footer
    tip (monoCheckButton, "Mono: listen to the result in mono");
    tip (monoDryButton,   "Dry: while in mono, compare with the unprocessed input");
    tip (mixSlider,       "Mix: blend the processed sound with the original. Right-click to lock it - presets, A/B, Reset and Smart then leave it alone");
    tip (volSlider,       "Vol: output trim, plus or minus 6 dB. Right-click to lock it against presets and Reset");
    tip (panSlider,       "Pan: balance at the very end of the chain. Left or right trims the other side - use it to pull a mode that leans to one side back to the centre");
    tip (prismOnButton,   "Focus: switch the focus range on or off. Wide open it does nothing at all");
    tip (prismBand,       "Focus: the range SpaceX works in. Drag an edge to resize, the middle to move, up and down or scroll to widen. Outside it the sound passes through untouched");
    tip (correlationMeter, "Correlation: to the right of centre is mono-safe, to the left it cancels in mono");
    tip (viewGearButton,  "View: display settings for the goniometer and the starfield");
    tip (goniometer,      "Starfield: click the field to switch the scope on or off. Cmd-click the centre for the trace colour, shift-click for its look");

    // Kein TooltipWindow mehr (User: "die Hover-Infos sollen in der Zeile
    // unter dem Footer stehen statt direkt an der Maus") - die Texte werden
    // weiterhin als Tooltips gesetzt, gelesen werden sie jetzt aber von
    // updateHintBar() und unten angezeigt.
    tooltipWindow.reset();
    helpButton.setToggleState (juce::PropertiesFile (LCRMSAudioProcessor::appPropertiesOptions()).getBoolValue ("hoverHints", false),
                               juce::dontSendNotification);
    helpButton.repaint();
    applyHintTexts();   // Runde 133: die neuen Texte gewinnen
}

// Liest den Tooltip des Elements unter der Maus und legt ihn in die
// Hinweiszeile unten. Laeuft im Timer mit; nur bei echter Aenderung wird
// neu gezeichnet, damit es nichts kostet.
void LCRMSAudioProcessorEditor::updateHintBar()
{
    // Runde 38 (User): solange eine Maustaste gedrueckt ist (Regler ziehen),
    // wechselt der Hinweis NICHT - sonst springt er auf alles, woran die Maus
    // beim Ziehen vorbeifaehrt. Runde 99 (User): sein Text wird trotzdem neu
    // gelesen, damit Werte wie die Hz-Zahl von Regain live mitlaufen.
    const bool draggingNow = juce::ModifierKeys::currentModifiers.isAnyMouseButtonDown();
    juce::String want;
    if (draggingNow)
    {
        if (! helpButton.getToggleState())
            return;
        if (auto* src = hintSource.getComponent())
            if (auto* ttc = dynamic_cast<juce::TooltipClient*> (src))
                want = ttc->getTooltip();
        if (want.isEmpty())
            return;
    }
    else if (helpButton.getToggleState())
    {
        // Frueher ueber getComponentUnderMouse() - das haengt an den zuletzt
        // zugestellten Mausereignissen und lieferte erst nach einem Klick
        // etwas (User-Bug: "geht erst bei click - nicht bei mouse hover").
        // Jetzt wird die Position selbst abgefragt und der Baum von Hand
        // durchsucht: unabhaengig von Events, deshalb sofort beim Ueberfahren.
        const auto screenPos = juce::Desktop::getInstance().getMainMouseSource().getScreenPosition();
        const auto local = getLocalPoint (nullptr, screenPos).roundToInt();
        juce::Component* c = getLocalBounds().contains (local) ? getComponentAt (local) : nullptr;
        hintSource = nullptr;
        for (int guard = 0; c != nullptr && guard < 6 && want.isEmpty(); ++guard, c = c->getParentComponent())
            if (auto* ttc = dynamic_cast<juce::TooltipClient*> (c))
            {
                want = ttc->getTooltip();
                if (want.isNotEmpty()) hintSource = c;
            }
    }
    if (want != currentHint)
    {
        currentHint = want;
        // WICHTIG: hintBarArea liegt im Design-Koordinatensystem der
        // content-Komponente, nicht im Editor. Ein repaint() auf dem Editor
        // traf deshalb den falschen Bereich - sichtbar wurde die Zeile nur,
        // wenn ohnehin gerade alles neu gezeichnet wurde (z.B. waehrend Solo
        // laeuft). Das war der eigentliche Grund fuer "geht nur bei Klick".
        content.repaint (hintBarArea.expanded (6));
    }
}

juce::ValueTree LCRMSAudioProcessorEditor::viewSettingsTree() const
{
    juce::ValueTree t ("ViewSettings");
    t.setProperty ("gonio",     viewPanel.gonioBtn.getToggleState(),     nullptr);
    t.setProperty ("starMode",  viewPanel.starModeIndex(),               nullptr);
    t.setProperty ("photo",     viewPanel.photoIndex(),                  nullptr);
    t.setProperty ("gravity",   viewPanel.gravityIndex(),                nullptr);
    t.setProperty ("modMove",   viewPanel.modMoveBtn.getToggleState(),   nullptr);
    t.setProperty ("shine2",    viewPanel.dimSlider.getValue(),          nullptr);
    t.setProperty ("speed",     viewPanel.speedSlider.getValue(),        nullptr);
    t.setProperty ("density",   viewPanel.densitySlider.getValue(),      nullptr);
    t.setProperty ("gonioCol",  viewPanel.gonioColourIndex(),            nullptr);
    t.setProperty ("gonioStyle", viewPanel.gonioStyleIndex(),            nullptr);
    t.setProperty ("stars",     viewPanel.starsBtn.getToggleState(),     nullptr);
    t.setProperty ("starLines", viewPanel.linesBtn.getToggleState(),     nullptr);
    return t;
}

void LCRMSAudioProcessorEditor::storeViewSettingsInState()
{
    auto& state = processor.apvts.state;
    auto existing = state.getChildWithName ("ViewSettings");
    if (existing.isValid())
        state.removeChild (existing, nullptr);
    // Immer als LETZTES Kind anhaengen: die Preset-Pruefsumme zaehlt die
    // Kinder der Reihe nach, ein Kind am Ende aendert sie nicht.
    state.appendChild (viewSettingsTree(), nullptr);
    // Die Mutate-Kategorie gehoert zur Instanz (Spur), nicht zum Preset -
    // nach jedem replaceState() wieder eintragen.
    state.setProperty ("mutateCategory", mutateCategoryValue, nullptr);
}

bool LCRMSAudioProcessorEditor::applyViewSettingsFromTree (const juce::ValueTree& parent)
{
    auto t = parent.getChildWithName ("ViewSettings");
    if (! t.isValid())
        return false;
    viewPanel.gonioBtn.setToggleState     ((bool) t.getProperty ("gonio",     true),  juce::dontSendNotification);
    viewPanel.setStarModeIndex ((int) t.getProperty ("starMode", 2));
    viewPanel.setPhotoIndex    ((int) t.getProperty ("photo",    1));
    viewPanel.setGravityIndex  ((int) t.getProperty ("gravity",  0));
    viewPanel.modMoveBtn.setToggleState   ((bool) t.getProperty ("modMove",   true),  juce::dontSendNotification);
    // Dim/Shine bewusst NICHT aus Presets/Slots uebernommen (Sitzungszustand, User).
    viewPanel.speedSlider.setValue    ((double) t.getProperty ("speed",    1.0),  juce::dontSendNotification);
    viewPanel.densitySlider.setValue  ((double) t.getProperty ("density",  1.0),  juce::dontSendNotification);
    viewPanel.setGonioColourIndex ((int) t.getProperty ("gonioCol", 0));
    viewPanel.starsBtn.setToggleState ((bool) t.getProperty ("stars", true), juce::dontSendNotification);
    viewPanel.linesBtn.setToggleState ((bool) t.getProperty ("starLines", true), juce::dontSendNotification);
    viewPanel.setGonioStyleIndex  ((int) t.getProperty ("gonioStyle", 2));
    applyViewSettings (false);
    return true;
}

// "Save" im View-Panel: schreibt die View-Einstellungen in die Datei des
// geladenen Presets (nur dort - der Klang bleibt unangetastet). Ein Preset
// traegt View-Einstellungen also nur, wenn man das ausdruecklich will.
void LCRMSAudioProcessorEditor::saveViewSettingsToPreset()
{
    if (currentPresetName.isEmpty() || isDefaultPresetName (currentPresetName))
        return;
    const auto f = presetFile (currentPresetName);
    if (! f.existsAsFile())
        return;
    auto xml = juce::XmlDocument::parse (f);
    if (xml == nullptr)
        return;
    auto tree = juce::ValueTree::fromXml (*xml);
    if (! tree.isValid())
        return;
    auto old = tree.getChildWithName ("ViewSettings");
    if (old.isValid())
        tree.removeChild (old, nullptr);
    tree.appendChild (viewSettingsTree(), nullptr);
    if (auto out = tree.createXml())
        f.replaceWithText (out->toString());
}

// Das frühere PopupMenu ist einem eigenen Panel gewichen (User: "Menu blinkt
// bei jedem Klick" - JUCE schliesst ein PopupMenu bei jeder Auswahl, und das
// Wiederoeffnen blitzt sichtbar). Das Panel bleibt offen, zeigt alle Schalter
// gleichzeitig und ist damit auch schneller zu ueberblicken.
void LCRMSAudioProcessorEditor::showPresetMenu()
{
    if (settingsPanel.isVisible())
    {
        closeSettingsPanel();
        return;
    }
    closeViewPanel();
    // Momentaufnahme der Oberflaeche, klein gerechnet, weichgezeichnet und
    // gemerkt. Das kostet einmal ein paar Millisekunden statt jedes Bild neu.
    {
        constexpr int kDiv = 4;
        auto shot = content.createComponentSnapshot (content.getLocalBounds(), false, 1.0f / (float) kDiv);
        if (shot.isValid())
        {
            juce::ImageConvolutionKernel blur (7);
            blur.createGaussianBlur (2.6f);
            blur.applyToImage (shot, shot, shot.getBounds());
            settingsBlur = shot;
        }
    }
    captureSettingsSnapshot();
    refreshSettingsPanel();
    settingsBackdrop.setVisible (true);
    settingsBackdrop.toFront (false);
    settingsPanel.setAlpha (1.0f);
    settingsPanel.setVisible (true);
    settingsPanel.toFront (false);
    juce::Desktop::getInstance().getAnimator().fadeIn (&settingsPanel, 120);
    content.repaint();
}

void LCRMSAudioProcessorEditor::captureSettingsSnapshot()
{
    juce::PropertiesFile p (LCRMSAudioProcessor::appPropertiesOptions());
    settingsSnap.theme       = uiThemeIndex;
    settingsSnap.layout      = uiLayoutRef();
    settingsSnap.prism       = p.getBoolValue ("mutateChangesPrism", true);
    settingsSnap.mix         = p.getBoolValue ("mutateChangesMix", false);
    settingsSnap.cats        = p.getBoolValue ("showMutateCategories", true);
    settingsSnap.clickEdge   = p.getBoolValue ("prismClickJumps", false);
    settingsSnap.galaxyStart = p.getBoolValue ("galaxyActivateDefault", false);
    settingsSnap.showHz      = p.getBoolValue ("showFocusHz", false);
    settingsSnap.keepSolo    = keepSoloWhenSectionOff;
    settingsSnap.modVis      = modulationVisualsEnabled;
    settingsSnap.advMod      = advancedModVisible;
    settingsSnap.techLabels  = technicalLabels;
    settingsSnap.bright      = uiBrightnessRef();
    settingsSnap.autoGain    = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_AUTO_GAIN)->load() > 0.5f;
    settingsSnap.bassGuard   = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_BASS_GUARD)->load() > 0.5f;
}

// "Cancel": alles zurueck auf den Stand beim Oeffnen. Laeuft ueber dieselben
// Aktionen wie die Knoepfe selbst - es gibt also keinen zweiten Weg, auf dem
// eine Einstellung gesetzt werden koennte.
void LCRMSAudioProcessorEditor::restoreSettingsSnapshot()
{
    juce::PropertiesFile p (LCRMSAudioProcessor::appPropertiesOptions());
    auto flipIf = [&] (bool current, bool wanted, int id) { if (current != wanted) handleSettingsAction (id); };
    flipIf (p.getBoolValue ("mutateChangesPrism", true),    settingsSnap.prism,       idMutatePrism);
    flipIf (p.getBoolValue ("prismClickJumps", false),       settingsSnap.clickEdge,   idPrismClickJumps);
    flipIf (p.getBoolValue ("showFocusHz", false),           settingsSnap.showHz,      idShowHz);
    flipIf (p.getBoolValue ("galaxyActivateDefault", false),settingsSnap.galaxyStart, idGalaxyDefault);
    flipIf (keepSoloWhenSectionOff,                         settingsSnap.keepSolo,    idKeepSolo);
    flipIf (modulationVisualsEnabled,                       settingsSnap.modVis,      idShowModulation);
    flipIf (advancedModVisible,                             settingsSnap.advMod,      idShowAdvancedMod);
    flipIf (technicalLabels,                                settingsSnap.techLabels,  idTechnicalLabels);
    flipIf (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_AUTO_GAIN)->load() > 0.5f,
                                                            settingsSnap.autoGain,    idAutoGain);
    flipIf (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_BASS_GUARD)->load() > 0.5f,
                                                            settingsSnap.bassGuard,   idBassGuard);

    if (std::abs (uiBrightnessRef() - settingsSnap.bright) > 0.0001f)
        applyBrightness (settingsSnap.bright, true);
    if (uiLayoutRef() != settingsSnap.layout)
        handleSettingsAction (settingsSnap.layout == 0 ? idLayoutFrames
                            : settingsSnap.layout == 1 ? idLayoutFrameless : idLayoutEasy);
    if (uiThemeIndex != settingsSnap.theme)
    {
        static const int themeForId[SettingsPanelComponent::kThemes] = { 4, 1, 3 };   // Runde 126: Day & Night, Fairy Tale, Sci-Fi (Pop raus)
        for (int i = 0; i < SettingsPanelComponent::kThemes; ++i)
            if (themeForId[i] == settingsSnap.theme)
            {
                handleSettingsAction (SettingsPanelComponent::themeIds()[i]);
                break;
            }
    }
}

// ===== TAKE THE TOUR =====
// Die Schritte werden bei jedem Start neu aus den AKTUELLEN Rahmen gebaut -
// so stimmt die Markierung auch nach Layout-/Variantenwechsel, ohne dass
// irgendwo Koordinaten doppelt gepflegt werden muessen.
void LCRMSAudioProcessorEditor::startTour (bool firstRun)
{
    closeSettingsPanel();
    closeBackPanel();
    closeViewPanel();

    // Runde 172 (User: "Tour komplett neu - alle neuen Labels, Features,
    // Icons. Nicht jeden Knopf, aber das Wichtigste, damit jeder das Plugin
    // sofort versteht"). Reihenfolge wie beim ersten Arbeiten: Profil, Wuerfel,
    // Kopf, dann die Sektionen im Signalweg, dann Kontrolle, Presets, Hilfe.
    const bool t = technicalLabels;
    auto L = [t] (const char* tech, const char* story) { return juce::String (t ? tech : story); };
    auto area = [this] (std::initializer_list<juce::Component*> comps)
    {
        juce::Rectangle<int> r;
        for (auto* c : comps)
            if (c != nullptr && c->isVisible() && ! c->getBounds().isEmpty())
            {
                const auto b = content.getLocalArea (c, c->getLocalBounds());
                r = r.isEmpty() ? b : r.getUnion (b);
            }
        return r;
    };

    tourOverlay.steps.clear();
    auto add = [this] (juce::Rectangle<int> target, const juce::String& eyebrow,
                       const juce::String& head, const juce::String& text, bool allowEmpty = false)
    {
        if (allowEmpty || ! target.isEmpty())
            tourOverlay.steps.push_back ({ target, eyebrow, head, text });
    };

    // Runde 173 (User: "kurz und knapp, keine Doktorarbeit - aber ein bisschen
    // mehr darf es sein; ab und zu erwaehnen, dass die Modi sorgfaeltig
    // ausgesucht sind"). Wuerfel heisst ueberall "Smart dice".
    add ({}, "Welcome", "Welcome to SpaceX",
         "Six sections, top-left to bottom-right, in the order the audio flows. "
         "Simple on the surface - a lot of fine-tuning underneath. This takes a minute.", true);

    add (area ({ &categoryButton, &catDots }), "Smart", "Pick your source",
         "Lead Vocal, Backings, Adlibs or Send FX. Each profile knows what suits that source "
         "and steers the Smart dice. Click for the next one, or hit a dot.");

    add (area ({ &globalChaosButton }), "Smart", "The Smart dice",
         "Rolls a complete setting for your profile and switches on only the sections that belong in it. "
         "Every range it can reach was tuned by ear - roll until something surprises you. Undo takes it back.");

    add (area ({ &globalBypassButton, &lifeSlider, &globalModBypassButton }), "Header", "Power, Life, Mod",
         "Power bypasses the plugin. Life sets how much everything moves - one knob for all modulation. "
         "Mod switches the modulation off without losing it.");
    // Runde 174 (User): der Rahmen umschliesst auch den Wuerfel - der ist hier
    // nicht gemeint und wird im Loch mit abgedunkelt.
    if (! tourOverlay.steps.empty() && tourOverlay.steps.back().head == "Power, Life, Mod")
        tourOverlay.steps.back().dims.push_back (area ({ &globalChaosButton }));

    add (area ({ &globalGalaxyActivateButton }), "Engine", L ("LCR", "Galaxy"),
         "Switches on the real L/C/R split - a true centre, not just mid-side. "
         "It adds latency while it runs. Off, SpaceX has zero latency.");

    add (area ({ &lcrTitleLabel, &lcrLockButton }), "Every section", "Name and lock",
         "Click a section name to switch it on or off. Cmd-click solos it. "
         "The lock keeps the Smart dice away from that section.");

    add (groupLcrArea, "Section 1", L ("LCR Matrix", "Galaxy"),
         L ("L/R", "Orbit") + " sets how much of the sides you keep, " + L ("C-Weight", "Gravity")
         + " how firmly the centre is held, " + L ("HF Regain", "Regain") + " brings back the highs the split takes away.");

    add (groupPolArea, "Section 2", L ("Polarity", "Eclipse"),
         "Flips the phase of L, R or both - PRE or POST decides where in the chain. "
         "The biggest single change in the plugin, so check Mono.");

    add (groupDriftArea, "Section 3", L ("Micropitch", "Parallax"),
         "Makes a mono sound wide. The four styles look simple, but each is its own hand-tuned mix "
         "of time and pitch tricks. Pick one, turn Amount.");

    add (groupWidthBoostArea, "Section 4", L ("Mid-Side", "Dimension"),
         L ("Width", "Size") + " and " + L ("Sides", "Boost") + " open the image. The Sides EQ curves look plain, "
         "but each was picked by ear to solve a real mix problem. The small fader sets how strong.");

    add (groupFlowArea.getUnion (groupRayArea), "Sections 5 + 6", L ("Autopan and Phaser", "Hyperdrive and Raye"),
         L ("Autopan", "Hyperdrive") + " moves the sound left and right - the note syncs it to the song. "
         "The phaser moves the image, not the tone. LINK ties it to the " + L ("Autopan", "Hyperdrive") + ".");

    add (area ({ &goniometer }), "Look", "The starfield",
         "Your stereo image, live - it reacts to what SpaceX does. Click it for the goniometer. "
         "The bar at the bottom shows how mono-safe you are.");

    add (area ({ &volInputMeter, &volOutputMeter, &monoCheckButton, &monoDryButton, &mixSlider,
                 &panSlider, &volSlider, &autoGainButton }), "Listen", "Output",
         "Mono to check, Mix to blend, Vol at the very end. "
         "AG matches the level, so bypass is a fair comparison.");

    add (area ({ &presetPrevButton, &presetNameButton, &presetNextButton, &globalSaveSizeButton }), "Presets", "Presets",
         "35 presets, set by ear on real sessions, sorted in folders. "
         "The arrows step through the current folder, Save lets you pick one.");

    add (area ({ &undoButton, &redoButton, &globalABButton, &abCopyButton, &presetMenuButton, &globalResetButton }),
         "Compare", "Undo, A / B, Settings",
         "Undo covers everything, the Smart dice included. A / B compares two versions. "
         "Settings has themes and this tour.");

    add (area ({ &helpButton }).getUnion (hintBarArea), "Help", "Need help?",
         "Click the ? and hover anything - a short hint appears down here. Have fun.");

    tourOverlay.index = 0;
    tourOverlay.dontShowBtn.setVisible (firstRun);
    tourOverlay.dontShowBtn.setToggleState (false, juce::dontSendNotification);
    tourOverlay.setBounds (content.getLocalBounds());
    tourOverlay.setVisible (true);
    tourOverlay.toFront (true);
    tourOverlay.refresh();
    tourOverlay.grabKeyboardFocus();
    content.repaint();
}

// ===== BACK PANEL =====
void LCRMSAudioProcessorEditor::showBackPanel (bool welcomeMode)
{
    if (backPanel.isVisible())
    {
        closeBackPanel();
        return;
    }
    closeSettingsPanel();
    closeViewPanel();

    juce::PropertiesFile props (LCRMSAudioProcessor::appPropertiesOptions());
    const bool lic = processor.licensed.load (std::memory_order_relaxed);
    const auto owner = props.getValue ("licenceName", juce::String()).trim();
    backPanel.activateBtn.setButtonText (lic ? "Activated" : "Enter Serial");
    backPanel.activateBtn.setToggleState (lic, juce::dontSendNotification);
    backPanel.regName.setText (! lic ? "Demo - not activated"
                                     : owner.isNotEmpty() ? owner : "This copy is activated",
                               juce::dontSendNotification);
    backPanel.byName.setText (spacexContact::designer, juce::dontSendNotification);
    backPanel.thanksText.setText (spacexContact::thanks, juce::dontSendNotification);
    backPanel.mailBtn.setButtonText (spacexContact::email);
    backPanel.webBtn.setButtonText (spacexContact::website);
    backPanel.instaBtn.setButtonText (spacexContact::instagram);
    backPanel.linksBtn.setButtonText ("lnk.bio/paulmisty");

    settingsBackdrop.setVisible (true);
    settingsBackdrop.toFront (false);
    backPanel.dontShowBtn.setVisible (welcomeMode);
    backPanel.resized();
    backPanel.setAlpha (1.0f);
    backPanel.setVisible (true);
    backPanel.toFront (false);
    juce::Desktop::getInstance().getAnimator().fadeIn (&backPanel, 120);
    content.repaint();
}

void LCRMSAudioProcessorEditor::closeBackPanel()
{
    if (! backPanel.isVisible())
        return;
    settingsBackdrop.setVisible (false);
    juce::Desktop::getInstance().getAnimator().fadeOut (&backPanel, 120);
    content.repaint();
}

void LCRMSAudioProcessorEditor::closeSettingsPanel()
{
    if (! settingsPanel.isVisible())
        return;
    settingsBackdrop.setVisible (false);
    settingsBlur = {};
    juce::Desktop::getInstance().getAnimator().fadeOut (&settingsPanel, 120);
    content.repaint();
}

// Haken und Auswahl im Panel auf den tatsaechlichen Stand bringen. Wird nach
// jeder Aktion aufgerufen - das Panel bleibt dabei offen.
void LCRMSAudioProcessorEditor::refreshSettingsPanel()
{
    juce::PropertiesFile p (LCRMSAudioProcessor::appPropertiesOptions());
    static const int themeForId[SettingsPanelComponent::kThemes] = { 4, 1, 3 };   // Runde 126: Day & Night, Fairy Tale, Sci-Fi (Pop raus)
    for (int i = 0; i < SettingsPanelComponent::kThemes; ++i)
        settingsPanel.themeBtn[i].setToggleState (uiThemeIndex == themeForId[i], juce::dontSendNotification);

    // Runde 71: die Layout-Knoepfe gibt es nicht mehr - jedes Theme hat sein
    // festes Layout. "Technical Labels" steht jetzt bei Behaviour.
    settingsPanel.behavBtn[1].setToggleState (modulationVisualsEnabled,  juce::dontSendNotification);
    settingsPanel.brightSlider.setValue (uiBrightnessRef(), juce::dontSendNotification);
    // "SpaceX Labels" ist die Umkehrung: angehakt = NICHT technisch.
    settingsPanel.labelBtn.setToggleState (! technicalLabels, juce::dontSendNotification);
    settingsPanel.behavBtn[0].setToggleState (p.getBoolValue ("galaxyActivateDefault", false),   juce::dontSendNotification);
    const bool lic = processor.licensed.load (std::memory_order_relaxed);
    backPanel.activateBtn.setButtonText (lic ? "Activated" : "Enter Serial");
    backPanel.activateBtn.setToggleState (lic, juce::dontSendNotification);
    backPanel.activateBtn.setTooltip (lic ? "Add or change the name shown above"
                                          : "Enter your name and serial number to remove the demo mute");
    settingsPanel.repaint();
}

// Fuehrt genau eine Einstellung aus. Frueher der Rumpf des Menue-Callbacks -
// das Panel ruft dieselbe Stelle auf, damit es nur EINE Wahrheit gibt.
void LCRMSAudioProcessorEditor::handleSettingsAction (int result)
{
    juce::PropertiesFile writeProps (LCRMSAudioProcessor::appPropertiesOptions());

    // Das Band-begrenzt-Galaxy ist ein echter Parameter (steht im Preset),
    // deshalb laeuft er nicht ueber die Properties wie alles andere hier.
    if (result == idBandGalaxy)
    {
        if (auto* prm = processor.apvts.getParameter (LCRMSAudioProcessor::ID_PRISM_GALAXY))
            prm->setValueNotifyingHost (prm->getValue() > 0.5f ? 0.0f : 1.0f);
        refreshSettingsPanel();
        return;
    }

    switch (result)
            {
                case idGalaxyDefault:
                    // Schreibt in das eingebaute Preset "Default" - der Stern
                    // am Namen folgt sofort, falls der Live-Zustand dadurch
                    // vom Default abweicht.
                    writeProps.setValue ("galaxyActivateDefault", ! writeProps.getBoolValue ("galaxyActivateDefault", false));
                    writeProps.saveIfNeeded();
                    if (isDefaultPresetName (currentPresetName))
                    {
                        presetSignature = signatureOfTree (defaultPresetTree());
                        presetDirty = std::abs (computePresetSignature() - presetSignature) > 1.0e-5f;
                        refreshPresetNameDisplay();
                    }
                    break;
                case idOpenPresetFolder:
                    presetFolder().revealToUser();
                    break;
                case idSetPresetFolder:
                    presetFolderChooser = std::make_unique<juce::FileChooser> ("Choose Preset Folder", presetFolder());
                    presetFolderChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                        [this] (const juce::FileChooser& fc)
                        {
                            const auto dir = fc.getResult();
                            if (dir.isDirectory())
                            {
                                juce::PropertiesFile p2 (LCRMSAudioProcessor::appPropertiesOptions());
                                p2.setValue ("presetFolder", dir.getFullPathName());
                                p2.saveIfNeeded();
                            }
                            presetFolderChooser.reset();
                        });
                    break;
                case idMutatePrism:
                    writeProps.setValue ("mutateChangesPrism", ! writeProps.getBoolValue ("mutateChangesPrism", true));
                    writeProps.saveIfNeeded();
                    break;
                case idLockMix:
                    writeProps.setValue ("lockMix", ! writeProps.getBoolValue ("lockMix", false));
                    writeProps.saveIfNeeded();
                    break;
                case idMutateMix:
                    writeProps.setValue ("mutateChangesMix", ! writeProps.getBoolValue ("mutateChangesMix", false));
                    writeProps.saveIfNeeded();
                    break;
                case idShowCategories:
                    writeProps.setValue ("showMutateCategories", ! writeProps.getBoolValue ("showMutateCategories", true));
                    writeProps.saveIfNeeded();
                    showMutateCategories = writeProps.getBoolValue ("showMutateCategories", true);
                    categoryButton.setVisible (showMutateCategories);
                    catDots.setVisible (showMutateCategories);
                    smartInfoToggle.setVisible (showMutateCategories && mutateCategoryValue > 0);   // Runde 114: ohne Profil kein (i)
                    smartInfoLabel.setVisible (showMutateCategories && smartInfoVisible && mutateCategoryValue > 0);
                    break;
                case idTechnicalLabels:
                    technicalLabels = ! technicalLabels;
                    writeProps.setValue ("technicalLabels", technicalLabels);
                    writeProps.saveIfNeeded();
                    applyLabelStyle();
                    break;
                case idBackPanel:
                    closeSettingsPanel();
                    showBackPanel();
                    break;
                case idOpenManual:
                    openManual();
                    break;

                case idTakeTour:
                    closeSettingsPanel();
                    startTour();
                    break;

                case idLayoutFrames: case idLayoutFrameless: case idLayoutEasy:
                    uiLayoutRef() = (result == idLayoutFrames) ? 0 : (result == idLayoutFrameless) ? 1 : 2;
                    writeProps.setValue ("uiLayout", uiLayoutRef());
                    writeProps.saveIfNeeded();
                    applyLayoutMode();
                    themePlateFor = -1;   // Platte neu backen
                    resized();
                    repaint();
                    break;

                case idSaveSettings:
                    closeSettingsPanel();
                    break;

                case idCancelSettings:
                    restoreSettingsSnapshot();
                    closeSettingsPanel();
                    break;

                case idResetSettings:
                {
                    // Alles zurueck auf Werkseinstellung (gleicher Gedanke wie
                    // "Reset" im View-Panel, deshalb auch derselbe Name).
                    writeProps.setValue ("mutateChangesPrism", true);
                    writeProps.setValue ("mutateChangesMix", false);
                    writeProps.setValue ("showMutateCategories", true);
                    writeProps.setValue ("prismClickJumps", false);
                    writeProps.setValue ("showFocusHz", false);
                    prismBand.setShowHz (false);
                    writeProps.setValue ("galaxyActivateDefault", false);
                    writeProps.setValue ("keepSoloWhenSectionOff", false);
                    keepSoloWhenSectionOff = false;
                    modulationVisualsEnabled = true;
                    showMutateCategories = true;
                    writeProps.setValue ("technicalLabels", true);
                    technicalLabels = true;
                    writeProps.saveIfNeeded();
                    applyBrightness (0.0f, true);   // Runde 174
                    applyLabelStyle();
                    applyLayoutMode();
                    resized();
                    repaint();
                    break;
                }

                case idShowHz:
                    writeProps.setValue ("showFocusHz", ! writeProps.getBoolValue ("showFocusHz", false));
                    writeProps.saveIfNeeded();
                    prismBand.setShowHz (writeProps.getBoolValue ("showFocusHz", false));
                    break;

                case idKeepSolo:
                    keepSoloWhenSectionOff = ! keepSoloWhenSectionOff;
                    writeProps.setValue ("keepSoloWhenSectionOff", keepSoloWhenSectionOff);
                    writeProps.saveIfNeeded();
                    break;

                case idHoverHints:
                    writeProps.setValue ("hoverHints", ! writeProps.getBoolValue ("hoverHints", false));
                    writeProps.saveIfNeeded();
                    applyHoverHints();
                    break;
                // Theme-Wahl: Menue danach gleich wieder oeffnen (User: "Menue
                // soll offen bleiben") - JUCE schliesst PopupMenus bei Auswahl.
                case idThemeModern: case idThemePurple: case idThemeDay: case idThemeDark: case idThemeMoon: case idThemeComic:
                case idThemeSciFiDark:
                {
                    static const std::pair<int, int> map[] = { { idThemeModern, 0 }, { idThemeDark, 1 }, { idThemeComic, 2 },
                                                               { idThemePurple, 3 }, { idThemeDay, 4 }, { idThemeMoon, 5 },
                                                               { idThemeSciFiDark, 3 } };   // Sci-Fi Dark ist in Sci-Fi aufgegangen
                    for (const auto& m : map)
                        if (m.first == result) setUiTheme (m.second, true);
                    break;   // Wiederoeffnen uebernimmt reopenGuard (siehe oben)
                }
                case idActivate:
                    promptActivate();
                    break;
                case idAutoGain:
                    if (auto* prm = processor.apvts.getParameter (LCRMSAudioProcessor::ID_AUTO_GAIN))
                    {
                        const bool on = prm->getValue() > 0.5f;
                        prm->setValueNotifyingHost (on ? 0.0f : 1.0f);
                    }
                    break;
                case idBassGuard:
                    if (auto* prm = processor.apvts.getParameter (LCRMSAudioProcessor::ID_BASS_GUARD))
                    {
                        const bool on = prm->getValue() > 0.5f;
                        prm->setValueNotifyingHost (on ? 0.0f : 1.0f);
                    }
                    break;
                case idPresetSetsGalaxy:
                    writeProps.setValue ("presetSetsGalaxy", ! writeProps.getBoolValue ("presetSetsGalaxy", false));
                    writeProps.saveIfNeeded();
                    break;
                case idPrismClickJumps:
                    writeProps.setValue ("prismClickJumps", ! writeProps.getBoolValue ("prismClickJumps", false));
                    writeProps.saveIfNeeded();
                    prismBand.setClickJumps (writeProps.getBoolValue ("prismClickJumps", false));
                    break;
                case idHideGonioDefault:
                    // Jetzt sofort live UND als neuer Start-Standard fuers
                    // naechste Oeffnen (User-Wunsch: einfacher Menu-Eintrag
                    // zum Aktivieren/Deaktivieren, kein separater Live-
                    // Button mehr).
                    goniometerVisualsOn = ! goniometerVisualsOn;
                    goniometer.setGoniometerActive (goniometerVisualsOn);
                    writeProps.setValue ("goniometerDisabledDefault", ! goniometerVisualsOn);
                    break;
                case idHideSpaceVisualsDefault:
                    starVisualsOn = ! starVisualsOn;
                    goniometer.setSpaceVisualsEnabled (starVisualsOn);
                    writeProps.setValue ("spaceVisualsDisabledDefault", ! starVisualsOn);
                    break;
                case idShowModulation:
                    modulationVisualsEnabled = ! modulationVisualsEnabled;
                    writeProps.setValue ("modulationVisualsDisabled", ! modulationVisualsEnabled);
                    break;
                case idShowAdvancedMod:
                    advancedModVisible = ! advancedModVisible;
                    writeProps.setValue ("showAdvancedModulation", advancedModVisible);
                    applyAdvancedModVisibility();
                    break;
                case idDisableModMovement:
                    starfieldModMovementOn = ! starfieldModMovementOn;
                    goniometer.setModMovementEnabled (starfieldModMovementOn);
                    writeProps.setValue ("starfieldModMovementDisabled", ! starfieldModMovementOn);
                    break;
                case idReduceAnimations:
                    starfieldReducedAnimations = ! starfieldReducedAnimations;
                    goniometer.setReducedAnimations (starfieldReducedAnimations);
                    writeProps.setValue ("starfieldReducedAnimations", starfieldReducedAnimations);
                    break;
                case idSaveSizeDefault:
                    if (SPACEX_ROW2_VARIANT == 0)   // Varianten: anderes Seitenverhaeltnis
                    {
                        writeProps.setValue ("windowWidth", getWidth());
                        writeProps.setValue ("windowHeight", getHeight());
                    }
                    break;
                case idSaveStateDefault:
                    saveCurrentStateAsDefault();
                    break;
                default:
                    break;
            }

    writeProps.saveIfNeeded();
    refreshSettingsPanel();
}


// ===== PRESET-SYSTEM (dateibasiert) =====
// Presets liegen jetzt als einzelne XML-Dateien in einem Ordner (User-Wunsch:
// "Open Preset Folder", "Set Preset Folder") - eine Datei pro Preset, Name =
// Dateiname. Das ist der Standard, den man von anderen Herstellern kennt:
// man kann Presets kopieren, teilen, sichern und den Ordner selbst waehlen.
// Vorher lagen alle Presets in der App-Properties-Datei; die werden beim
// ersten Start EINMALIG in den Ordner uebernommen (siehe migrate...).
//
// Zusaetzlich gibt es das eingebaute Preset "Default": immer ganz oben,
// nicht loeschbar, nicht umbenennbar. Es ist der Zustand, mit dem das Plugin
// oeffnet. "Save current state as default" im Menue ueberschreibt es;
// "Activate Galaxy on startup" setzt darin nur den Galaxy-Schalter.
static const char* const kDefaultPresetName = "Default";
static const char* const kPresetExt = ".spacex";

juce::File LCRMSAudioProcessorEditor::presetFolder() const
{
    juce::PropertiesFile props (LCRMSAudioProcessor::appPropertiesOptions());
    const auto custom = props.getValue ("presetFolder", {});
    juce::File folder = custom.isNotEmpty() ? juce::File (custom)
                      : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                            .getChildFile ("SpaceX").getChildFile ("Presets");
    if (! folder.exists())
    {
        // Umbenennung "Space X" -> "SpaceX" (User): was im alten Ordner liegt,
        // wird einmalig mitgenommen, damit keine Presets verloren gehen.
        auto legacy = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                          .getChildFile ("Space X").getChildFile ("Presets");
        folder.createDirectory();
        if (legacy.isDirectory())
            for (const auto& f : legacy.findChildFiles (juce::File::findFiles, false, "*" + juce::String (kPresetExt)))
                f.copyFileTo (folder.getChildFile (f.getFileName()));
    }
    // Runde 109 (User): die vier Smart-Kategorien als echte Ordner - einmalig.
    // Loescht der User einen davon im Finder, kommt er nicht wieder. Dabei
    // wandern vorhandene Presets, die mit einem Smart-Profil gespeichert
    // wurden, in dessen Ordner; alle anderen bleiben, wo sie sind.
    if (! props.getBoolValue ("presetCategoryFolders", false))
    {
        static const char* const cats[4] = { "Lead Vocal", "Backings", "Adlibs", "Send FX" };
        for (auto* c : cats)
            folder.getChildFile (c).createDirectory();
        for (const auto& f : folder.findChildFiles (juce::File::findFiles, false, "*" + juce::String (kPresetExt)))
            if (auto xml = juce::XmlDocument::parse (f))
            {
                const int cat = xml->getIntAttribute ("mutateCategory", 0);
                if (cat >= 1 && cat <= 4)
                {
                    const auto target = folder.getChildFile (cats[cat - 1]).getChildFile (f.getFileName());
                    if (! target.exists())
                        f.moveFileTo (target);
                }
            }
        props.setValue ("presetCategoryFolders", true);
        props.saveIfNeeded();
    }
    // Runde 133 (User: "Presets in den Installer"): die Werks-Presets stecken
    // im Plugin selbst - das funktioniert mit dem .pkg genauso wie mit einem
    // von Hand kopierten VST3. Einmal pro Werks-Version; vorhandene Dateien
    // bleiben unangetastet, geloeschte kommen erst mit einer neuen Version.
    // Runde 158 (User: "Adlibs zusammen schreiben"): einmalig den alten Ordner
    // "Ad-Libs" nach "Adlibs" umziehen, samt der Werks-Presets "Ad-Lib ...".
    // Nichts wird ueberschrieben - existiert das Ziel schon, bleibt die
    // Quelle liegen.
    if (! props.getBoolValue ("adlibsRenamed", false))
    {
        const auto oldDir = folder.getChildFile ("Ad-Libs");
        const auto newDir = folder.getChildFile ("Adlibs");
        if (oldDir.isDirectory())
        {
            if (! newDir.exists())
                oldDir.moveFileTo (newDir);
            else
                for (const auto& f : oldDir.findChildFiles (juce::File::findFiles, false))
                {
                    const auto t = newDir.getChildFile (f.getFileName());
                    if (! t.exists()) f.moveFileTo (t);
                }
        }
        if (newDir.isDirectory())
            for (const auto& f : newDir.findChildFiles (juce::File::findFiles, false))
                if (f.getFileName().contains ("Ad-Lib"))
                {
                    const auto t = newDir.getChildFile (f.getFileName().replace ("Ad-Lib", "Adlib"));
                    if (! t.exists()) f.moveFileTo (t);
                }
        props.setValue ("adlibsRenamed", true);
        props.saveIfNeeded();
    }
    // Runde 171: Version 2 bringt die neuen Werks-Ordner Vocals, Drums,
    // Bass, Music und die Send-FX-Presets - vorhandene Dateien bleiben.
    constexpr int kFactoryPresetsVersion = 2;
    if (props.getIntValue ("factoryPresetsVersion", 0) < kFactoryPresetsVersion)
    {
        for (int i = 0; i < SpaceXPresetData::namedResourceListSize; ++i)
        {
            const juce::String file = juce::String::fromUTF8 (SpaceXPresetData::originalFilenames[i]);
            if (! file.endsWithIgnoreCase (kPresetExt) || ! file.contains ("__"))
                continue;
            const auto sub  = file.upToFirstOccurrenceOf ("__", false, false).trim();
            const auto name = file.fromFirstOccurrenceOf ("__", false, false).trim();
            int size = 0;
            if (const char* data = SpaceXPresetData::getNamedResource (SpaceXPresetData::namedResourceList[i], size))
            {
                const auto dir = folder.getChildFile (sub);
                dir.createDirectory();
                const auto target = dir.getChildFile (name);
                if (! target.exists())
                    target.replaceWithData (data, (size_t) size);
            }
        }
        props.setValue ("factoryPresetsVersion", kFactoryPresetsVersion);
        props.saveIfNeeded();
    }
    return folder;
}

// Ordner in Menue-Reihenfolge: erst die Smart-Kategorien (in deren
// Reihenfolge), dann eigene Ordner alphabetisch.
juce::StringArray LCRMSAudioProcessorEditor::presetFolderOrder() const
{
    static const char* const cats[4] = { "Lead Vocal", "Backings", "Adlibs", "Send FX" };
    const auto root = presetFolder();
    juce::StringArray order;
    for (auto* c : cats)
        if (root.getChildFile (c).isDirectory())
            order.add (c);
    // Runde 171: danach die Werks-Ordner in sinnvoller Reihenfolge,
    // erst dann eigene Ordner alphabetisch.
    static const char* const factoryFolders[4] = { "Vocals", "Drums", "Bass", "Music" };
    for (auto* c : factoryFolders)
        if (root.getChildFile (c).isDirectory())
            order.add (c);
    juce::StringArray others;
    for (const auto& d : root.findChildFiles (juce::File::findDirectories, false))
        if (! order.contains (d.getFileName()) && ! d.getFileName().startsWithChar ('.'))
            others.add (d.getFileName());
    others.sortNatural();
    order.addArray (others);
    return order;
}

juce::String LCRMSAudioProcessorEditor::presetDisplayName (const juce::String& key)
{
    return key.containsChar ('/') ? key.fromLastOccurrenceOf ("/", false, false) : key;
}

juce::String LCRMSAudioProcessorEditor::presetFolderOf (const juce::String& key)
{
    return key.containsChar ('/') ? key.upToLastOccurrenceOf ("/", false, false) : juce::String();
}

// Jeder Teil des Pfads einzeln dateinamen-tauglich machen - der
// Schraegstrich selbst muss stehen bleiben.
static juce::String legalPresetPath (const juce::String& key)
{
    auto parts = juce::StringArray::fromTokens (key, "/", {});
    parts.removeEmptyStrings();
    for (auto& p : parts)
        p = juce::File::createLegalFileName (p.trim());
    return parts.joinIntoString ("/");
}

// Alte Namen ohne Ordner (A/B, Sessions, eben verschobene Presets) auf den
// echten Ort umschreiben.
juce::String LCRMSAudioProcessorEditor::resolvePresetKey (const juce::String& keyIn) const
{
    // Runde 158 (User): "Ad-Libs" heisst jetzt "Adlibs" - alte Sessions
    // merken sich noch den alten Pfad.
    juce::String key = keyIn;
    if (key.startsWith ("Ad-Libs/"))
        key = "Adlibs/" + key.fromFirstOccurrenceOf ("/", false, false).replace ("Ad-Lib", "Adlib");
    if (key.isEmpty() || isDefaultPresetName (key) || key.containsChar ('/'))
        return key;
    const auto f = presetFile (key);
    if (! f.existsAsFile())
        return key;
    return f.getRelativePathFrom (presetFolder()).replaceCharacter ('\\', '/')
            .upToLastOccurrenceOf (kPresetExt, false, true);
}

// Wohin ein neu gespeichertes Preset kommt:
//  - "Ordner/Name" getippt -> genau dorthin
//  - derselbe Name wie das geladene Preset -> dieses Preset (ueberschreiben)
//  - sonst in den Ordner des aktiven Smart-Profils,
//  - ohne Profil in den Ordner des geladenen Presets, sonst nach oben.
juce::String LCRMSAudioProcessorEditor::keyForTypedName (const juce::String& typed) const
{
    if (typed.containsChar ('/') || isDefaultPresetName (typed))
        return typed;
    const auto cur = resolvePresetKey (currentPresetName);
    if (cur.isNotEmpty() && ! isDefaultPresetName (cur) && typed.equalsIgnoreCase (presetDisplayName (cur)))
        return cur;
    static const char* const cats[4] = { "Lead Vocal", "Backings", "Adlibs", "Send FX" };
    // Runde 172: wie im Speichern-Dialog - erst der aktuelle Ordner, dann das Profil.
    juce::String folder = isDefaultPresetName (cur) ? juce::String() : presetFolderOf (cur);
    if (folder.isEmpty() && mutateCategoryValue >= 1 && mutateCategoryValue <= 4)
        folder = cats[mutateCategoryValue - 1];
    return folder.isEmpty() ? typed : folder + "/" + typed;
}

juce::File LCRMSAudioProcessorEditor::presetFile (const juce::String& name) const
{
    const auto root = presetFolder();
    auto f = root.getChildFile (legalPresetPath (name) + kPresetExt);
    if (! f.existsAsFile() && ! name.containsChar ('/'))
    {
        // Name ohne Ordner: das Preset kann inzwischen in einem Ordner liegen.
        juce::Array<juce::File> hits;
        root.findChildFiles (hits, juce::File::findFiles, true,
                             juce::File::createLegalFileName (name) + kPresetExt);
        if (! hits.isEmpty())
            return hits.getFirst();
    }
    return f;
}

// Einmalige Uebernahme der alten Properties-Presets in den Ordner.
void LCRMSAudioProcessorEditor::migrateLegacyPresets()
{
    juce::PropertiesFile props (LCRMSAudioProcessor::appPropertiesOptions());
    if (props.getBoolValue ("presetsMigratedToFolder", false))
        return;
    auto names = juce::StringArray::fromLines (props.getValue ("presetNames", {}));
    names.removeEmptyStrings();
    for (const auto& n : names)
    {
        const auto xml = props.getValue ("presetXml_" + n, {});
        auto f = presetFile (n);
        if (xml.isNotEmpty() && ! f.existsAsFile())
            f.replaceWithText (xml);
    }
    props.setValue ("presetsMigratedToFolder", true);
    props.saveIfNeeded();
}

juce::StringArray LCRMSAudioProcessorEditor::getPresetNames() const
{
    // Runde 109: Reihenfolge = Menue-Reihenfolge (auch fuer die Pfeile):
    // Default, dann die Ordner, dann was oben ohne Ordner liegt.
    juce::StringArray names;
    names.add (kDefaultPresetName);
    const auto root = presetFolder();
    for (const auto& folder : presetFolderOrder())
    {
        juce::StringArray inFolder;
        for (const auto& f : root.getChildFile (folder).findChildFiles (juce::File::findFiles, true, juce::String ("*") + kPresetExt))
            inFolder.add (f.getRelativePathFrom (root).replaceCharacter ('\\', '/')
                              .upToLastOccurrenceOf (kPresetExt, false, true));
        inFolder.sortNatural();
        names.addArray (inFolder);
    }
    juce::StringArray top;
    for (const auto& f : root.findChildFiles (juce::File::findFiles, false, juce::String ("*") + kPresetExt))
        top.add (f.getFileNameWithoutExtension());
    top.sortNatural();
    names.addArray (top);
    return names;
}

bool LCRMSAudioProcessorEditor::isDefaultPresetName (const juce::String& name)
{
    return name.equalsIgnoreCase (kDefaultPresetName);
}

// Das eingebaute "Default": gespeicherter Standard-Schnappschuss, falls
// vorhanden, sonst der Werkszustand - und obendrauf der Galaxy-Startschalter
// aus dem Menue.
juce::ValueTree LCRMSAudioProcessorEditor::defaultPresetTree() const
{
    juce::PropertiesFile props (LCRMSAudioProcessor::appPropertiesOptions());
    juce::ValueTree tree;
    const auto xmlStr = props.getValue ("defaultPluginState", {});
    if (xmlStr.isNotEmpty())
        if (auto xml = juce::XmlDocument::parse (xmlStr))
            tree = juce::ValueTree::fromXml (*xml);
    if (! tree.isValid() || ! tree.hasType (processor.apvts.state.getType()))
        tree = processor.factoryState.createCopy();

    const bool galaxyOnStart = props.getBoolValue ("galaxyActivateDefault", false);
    for (int i = 0; i < tree.getNumChildren(); ++i)
    {
        auto c = tree.getChild (i);
        if (c.hasType ("PARAM") && c.getProperty ("id").toString() == LCRMSAudioProcessor::ID_GALAXY_ACTIVATE)
            c.setProperty ("value", galaxyOnStart ? 1.0f : 0.0f, nullptr);
    }
    return tree;
}

juce::ValueTree LCRMSAudioProcessorEditor::presetTree (const juce::String& name) const
{
    if (isDefaultPresetName (name))
        return defaultPresetTree();
    const auto f = presetFile (name);
    if (! f.existsAsFile())
        return {};
    if (auto xml = juce::XmlDocument::parse (f))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        if (tree.isValid() && tree.hasType (processor.apvts.state.getType()))
            return tree;
    }
    return {};
}

// Klick auf das Namensfeld: Liste zum Laden (deleteMode=false) bzw. zum
// Loeschen (Rechtsklick, deleteMode=true). Unten "Rename..." statt des
// frueheren "Save as..." (User-Wunsch) - benennt NUR um, speichert keine
// veraenderten Einstellungen mit.
// Preset "benutzt Galaxy" = Galaxy-Sektion ist darin eingeschaltet.
static bool presetTreeUsesGalaxy (const juce::ValueTree& tree)
{
    for (int i = 0; i < tree.getNumChildren(); ++i)
    {
        auto c = tree.getChild (i);
        if (c.hasType ("PARAM") && c.getProperty ("id").toString() == LCRMSAudioProcessor::ID_LCR_ENABLED)
            return (float) c.getProperty ("value", 0.0f) > 0.5f;
    }
    return false;
}

void LCRMSAudioProcessorEditor::showLoadPresetPopup (bool deleteMode)
{
    const auto presetNames = getPresetNames();
    juce::PopupMenu menu;
    // Ohne das hier zeichnet JUCE die Liste im eigenen Standard-Look (grauer
    // Kasten, blauer Balken) - ein PopupMenu erbt das LookAndFeel NICHT vom
    // Zielknopf (User: "immer noch genau gleich grau").
    menu.setLookAndFeel (&lookAndFeel);
    const bool galaxyArmedNow =
        processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_GALAXY_ACTIVATE)->load() > 0.5f;

    // Runde 109 (User): Ordner. Der Ordner des geladenen Presets steht
    // aufgeklappt da (Ueberschrift + Presets), das Preset selbst ist
    // abgehakt; alle anderen Ordner sind Untermenues.
    const auto currentKey = resolvePresetKey (currentPresetName);
    const auto currentTop = currentKey.containsChar ('/') ? currentKey.upToFirstOccurrenceOf ("/", false, false)
                                                          : juce::String();
    int currentId = 0;
    auto makeItem = [&] (int i, const juce::String& label)
    {
        const auto& key = presetNames[i];
        const bool isDef = isDefaultPresetName (key);
        // Galaxy global aus: Presets mit Galaxy grau (User, Runde 35).
        const bool galaxyBlocked = ! deleteMode && ! isDef && ! galaxyArmedNow
                                   && presetTreeUsesGalaxy (presetTree (key));
        juce::PopupMenu::Item it (label);
        it.itemID    = i + 1;
        it.isEnabled = ! (deleteMode && isDef) && ! galaxyBlocked;
        it.isTicked  = key.equalsIgnoreCase (currentKey);
        if (it.isTicked)
            currentId = i + 1;
        return it;
    };
    constexpr int kEmptyId = 99999;   // nie waehlbar
    juce::ignoreUnused (currentId);

    // Runde 156 (User): ALLE Ordner sehen gleich aus (Untermenue mit Pfeil) -
    // der Ordner des geladenen Presets wird nur beim Oeffnen direkt
    // aufgeklappt (siehe unten). "Default" steht nicht mehr oben, sondern
    // unter den Ordnern.
    constexpr int kOpenFolderId = 100010;   // Ordner-Eintrag, nie ein Ergebnis
    int openFolderPos = -1;                 // Position des aktuellen Presets im Untermenue (nur waehlbare)
    const auto folders = presetFolderOrder();
    static const char* const catFolders[4] = { "Lead Vocal", "Backings", "Adlibs", "Send FX" };
    bool ownSeparatorDone = false;
    for (const auto& folder : folders)
    {
        // Runde 112 (User): eigene Ordner stehen nach einer dezenten Trennung
        // unter den Smart-Kategorien.
        bool isCat = false;
        for (auto* cf : catFolders) isCat = isCat || folder == cf;
        if (! isCat && ! ownSeparatorDone)
        {
            menu.addSeparator();
            ownSeparatorDone = true;
        }
        juce::Array<int> idx;
        for (int i = 1; i < presetNames.size(); ++i)
            if (presetNames[i].containsChar ('/') && presetNames[i].upToFirstOccurrenceOf ("/", false, false) == folder)
                idx.add (i);
        juce::PopupMenu sub;
        sub.setLookAndFeel (&lookAndFeel);
        int selectable = 0;
        for (int i : idx)
        {
            auto it = makeItem (i, presetNames[i].fromFirstOccurrenceOf ("/", false, false));
            if (folder == currentTop && it.isTicked && it.isEnabled)
                openFolderPos = selectable;
            if (it.isEnabled) ++selectable;
            sub.addItem (it);
        }
        if (idx.isEmpty())
            sub.addItem (kEmptyId, "(empty)", false);
        if (folder == currentTop)
            menu.addSubMenu (folder, sub, true, std::unique_ptr<juce::Drawable>(), false, kOpenFolderId);
        else
            menu.addSubMenu (folder, sub);
    }
    bool firstTop = true;
    for (int i = 1; i < presetNames.size(); ++i)
        if (! presetNames[i].containsChar ('/'))
        {
            if (firstTop) { menu.addSeparator(); firstTop = false; }
            menu.addItem (makeItem (i, presetNames[i]));
        }
    menu.addSeparator();
    menu.addItem (makeItem (0, presetNames[0]));   // Default - unter den Ordnern (User)

    constexpr int kRenameId = 100000;
    if (! deleteMode)
    {
        const bool ownPreset = currentPresetName.isNotEmpty() && ! isDefaultPresetName (currentPresetName);
        menu.addSeparator();
        // Runde 161 (User): die drei Befehle dezenter als die Presets - sie
        // bekommen eine eigene (graue) Farbe, das LookAndFeel setzt sie
        // daran erkennbar kleiner und leiser.
        auto cmd = [&menu] (int id, const char* txt, bool enabled)
        {
            juce::PopupMenu::Item it (txt);
            it.itemID = id;
            it.isEnabled = enabled;
            it.colour = juce::Colour (0xff8f96a4);
            menu.addItem (it);
        };
        cmd (kRenameId,     "Rename...",        ownPreset);
        cmd (kRenameId + 2, "Delete...",        ownPreset);      // Runde 156
        cmd (kRenameId + 1, "Preset Folder..."  , true);
    }

    // Runde 157 (User: "klappt nach oben auf, geht an den Bildschirmrand"):
    // Ursache war withItemThatMustBeVisible - JUCE schob das Menue so weit
    // hoch, dass das geladene Preset auf Hoehe des Namens lag. Das ist raus;
    // das Menue oeffnet nach unten.
    auto menuOptions = juce::PopupMenu::Options().withTargetComponent (presetNameButton)
                           .withPreferredPopupDirection (juce::PopupMenu::Options::PopupDirection::downwards);
    const bool autoOpen = currentTop.isNotEmpty() && folders.contains (currentTop);
    if (autoOpen)
        menuOptions = menuOptions.withInitiallySelectedItem (kOpenFolderId);
    menu.showMenuAsync (menuOptions,
        [this, presetNames, deleteMode] (int result)
        {
            if (result == 100000)
            {
                promptRenamePreset();
                return;
            }
            if (result == 100001)
            {
                presetFolder().revealToUser();
                return;
            }
            if (result == 100002)
            {
                if (presetDeleteButton.onClick)
                    presetDeleteButton.onClick();
                return;
            }
            if (result <= 0 || result > presetNames.size())
                return;
            const auto name = presetNames[result - 1];
            if (! deleteMode)
            {
                loadPreset (name);
                return;
            }
            juce::NativeMessageBox::showOkCancelBox (juce::MessageBoxIconType::WarningIcon,
                "Delete Preset", "Delete preset \"" + presetDisplayName (name) + "\"?",
                nullptr,
                juce::ModalCallbackFunction::create ([this, name] (int okResult)
                {
                    if (okResult != 0)
                        deletePreset (name);
                }));
        });

    // Runde 156 (User: "den Ordner direkt offen haben, aber nicht anders
    // aussehen als sonst"): JUCE kann ein Untermenue nicht per Befehl
    // oeffnen - wohl aber per Tastatur. Der Ordner ist vorausgewaehlt
    // (withInitiallySelectedItem), ein "Pfeil rechts" klappt ihn auf, und
    // "Pfeil runter" fuehrt die Markierung auf das geladene Preset.
    if (autoOpen)
    {
        // Runde 161: mit kleiner Verzoegerung - das Menuefenster ist dann
        // sicher sichtbar und im Vordergrund.
        juce::Timer::callAfterDelay (60, [openFolderPos]
        {
            auto* m = juce::Component::getCurrentlyModalComponent();
            if (m == nullptr || m->getName() != "menu")
                return;
            m->keyPressed (juce::KeyPress (juce::KeyPress::rightKey));
            if (auto* sub = juce::Component::getCurrentlyModalComponent(); sub != nullptr && sub != m && sub->getName() == "menu")
                for (int k = 0; k < openFolderPos; ++k)
                    sub->keyPressed (juce::KeyPress (juce::KeyPress::downKey));
        });
    }
}

// Schreibt den aktuellen Zustand als eingebautes "Default". Erreichbar ueber
// das Menue UND ueber Save mit dem Namen "Default" (User: "so dass beide Wege
// gehen").
// "Open Manual (PDF)": Das Handbuch steckt im Plugin selbst (siehe
// juce_add_binary_data in CMakeLists.txt). Liegt es bereits am
// Standard-Installationsort, wird dieses genommen - sonst wird die eingebaute
// Fassung einmalig nach "Application Support/Space X" geschrieben und von dort
// geoeffnet. So funktioniert der Eintrag auch ohne Installer.
void LCRMSAudioProcessorEditor::openManual()
{
    const juce::String fileName ("SpaceX Manual (EN).pdf");

    const juce::File installed =
       #if JUCE_MAC
        juce::File ("/Library/Audio/Plug-Ins/Documentation/SpaceX").getChildFile (fileName);
       #else
        juce::File::getSpecialLocation (juce::File::globalApplicationsDirectory)
            .getChildFile ("SpaceX").getChildFile (fileName);
       #endif
    if (installed.existsAsFile())
    {
        installed.startAsProcess();
        return;
    }

    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("SpaceX");
    dir.createDirectory();
    auto pdf = dir.getChildFile (fileName);
    // Nur schreiben, wenn die Datei fehlt oder aus einer aelteren Version
    // stammt - sonst kostet jeder Klick unnoetig 4 MB Schreibarbeit.
    if (! pdf.existsAsFile() || pdf.getSize() != (juce::int64) SpaceXManualData::SpaceXManual_EN_pdfSize)
        pdf.replaceWithData (SpaceXManualData::SpaceXManual_EN_pdf,
                             (size_t) SpaceXManualData::SpaceXManual_EN_pdfSize);
    if (pdf.existsAsFile())
        pdf.startAsProcess();
}

void LCRMSAudioProcessorEditor::saveCurrentStateAsDefault()
{
    juce::PropertiesFile props (LCRMSAudioProcessor::appPropertiesOptions());
    {
        // Ohne View-Einstellungen: dafuer ist "Save as Default" im View-Panel da.
        auto tree = processor.apvts.copyState();
        auto vs = tree.getChildWithName ("ViewSettings");
        if (vs.isValid())
            tree.removeChild (vs, nullptr);
        if (auto xml = tree.createXml())
            props.setValue ("defaultPluginState", xml->toString());
    }
    props.saveIfNeeded();
    currentPresetName = "Default";
    // Der aktuelle Zustand IST ab jetzt der Default - Signatur direkt von ihm
    // nehmen, sonst bleibt der Stern stehen (User-Bug).
    presetSignature = computePresetSignature();
    presetDirty = false;
    refreshPresetNameDisplay();
}

void LCRMSAudioProcessorEditor::writePreset (const juce::String& name)
{
    if (name.isEmpty())
        return;
    auto tree = processor.apvts.copyState();
    // Der Klang-Save nimmt KEINE View-Einstellungen mit - aber wenn die
    // Datei schon welche hat (View-Panel "Save"), bleiben sie erhalten.
    {
        auto vs = tree.getChildWithName ("ViewSettings");
        if (vs.isValid())
            tree.removeChild (vs, nullptr);
        const auto f = presetFile (name);
        if (f.existsAsFile())
            if (auto oldXml = juce::XmlDocument::parse (f))
            {
                auto oldTree = juce::ValueTree::fromXml (*oldXml);
                auto oldVs = oldTree.getChildWithName ("ViewSettings");
                if (oldVs.isValid())
                    tree.appendChild (oldVs.createCopy(), nullptr);
            }
    }
    if (auto xml = tree.createXml())
    {
        const auto target = presetFile (name);
        target.getParentDirectory().createDirectory();
        target.replaceWithText (xml->toString());
    }

    currentPresetName = resolvePresetKey (name);
    presetSignature = computePresetSignature();
    presetDirty = false;
    refreshPresetNameDisplay();
}

void LCRMSAudioProcessorEditor::promptAndSaveNewPreset (bool prefillCurrent)
{
    // "Default" darf jetzt auch hier ueberschrieben werden (User: "so dass
    // beide Wege gehen") - im Menue gibt es denselben Befehl weiterhin.
    const juce::String prefill = prefillCurrent ? presetDisplayName (currentPresetName) : juce::String();

    // Runde 127 (User: "beim Speichern sagen, in welche Kategorie"): der
    // Dialog hat eine Ordner-Auswahl. Oben die vier Smart-Kategorien, dann
    // eigene Ordner, am Ende "New Folder...". Vorausgewaehlt ist, was am
    // wahrscheinlichsten stimmt: beim Ueberschreiben der Ordner des Presets,
    // sonst das aktive Smart-Profil, sonst der Ordner des geladenen Presets.
    // "Ordner/Name" ins Namensfeld getippt geht weiterhin und hat Vorrang.
    static const char* const cats[4] = { "Lead Vocal", "Backings", "Adlibs", "Send FX" };
    juce::StringArray folders (cats, 4);
    {
        juce::StringArray own;
        for (const auto& d : presetFolder().findChildFiles (juce::File::findDirectories, false))
        {
            const auto n = d.getFileName();
            if (! n.startsWithChar ('.') && ! folders.contains (n, true))
                own.add (n);
        }
        own.sortNatural();
        folders.addArray (own);
    }
    const auto cur = resolvePresetKey (currentPresetName);
    juce::String def;
    // Runde 172 (User: "beim Speichern soll nicht immer Lead Vocal an sein,
    // sondern der aktuelle Ordner"): zuerst der Ordner des geladenen Presets,
    // erst wenn es keinen gibt (Default) das Smart-Profil.
    if (! isDefaultPresetName (cur))
        def = presetFolderOf (cur);
    if (def.isEmpty() && mutateCategoryValue >= 1 && mutateCategoryValue <= 4)
        def = cats[mutateCategoryValue - 1];
    // Runde 149 (User: "sieht schlecht aus ... hier auch noch open preset
    // folder"): statt AlertWindow die Karte im Plugin (SavePresetPanel).
    juce::StringArray ownFolders;
    for (int i = 4; i < folders.size(); ++i)
        ownFolders.add (folders[i]);

    closeSettingsPanel();
    closeBackPanel();
    closeViewPanel();
    {
        constexpr int kDiv = 4;
        auto shot = content.createComponentSnapshot (content.getLocalBounds(), false, 1.0f / (float) kDiv);
        if (shot.isValid())
        {
            juce::ImageConvolutionKernel blur (7);
            blur.createGaussianBlur (2.6f);
            blur.applyToImage (shot, shot, shot.getBounds());
            settingsBlur = shot;
        }
    }
    savePanel.open (juce::StringArray (cats, 4), ownFolders, def, prefill);
    settingsBackdrop.setVisible (true);
    settingsBackdrop.toFront (false);
    savePanel.setAlpha (1.0f);
    savePanel.setVisible (true);
    savePanel.toFront (true);
    juce::Desktop::getInstance().getAnimator().fadeIn (&savePanel, 120);
    content.repaint();
    // Fokus erst nach dem Sichtbarwerden (im Host sonst manchmal ohne Wirkung).
    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<SavePresetPanel> (&savePanel)]
    {
        if (safe != nullptr && safe->isShowing())
            safe->focusName();
    });
}

void LCRMSAudioProcessorEditor::closeSavePanel()
{
    if (! savePanel.isVisible())
        return;
    settingsBackdrop.setVisible (false);
    settingsBlur = {};
    juce::Desktop::getInstance().getAnimator().fadeOut (&savePanel, 120);
    content.repaint();
}

void LCRMSAudioProcessorEditor::saveUnderKey (const juce::String& key)
{
    const juce::String name = key;
    if (isDefaultPresetName (name))
    {
        // Beide Wege fuehren zum selben Ziel (User).
        juce::NativeMessageBox::showOkCancelBox (juce::MessageBoxIconType::WarningIcon,
            "Overwrite Default",
            "Overwrite the built-in Default with the current settings?",
            nullptr,
            juce::ModalCallbackFunction::create ([this] (int okResult)
            {
                if (okResult != 0)
                    saveCurrentStateAsDefault();
            }));
        return;
    }

    if (presetFile (name).existsAsFile())
    {
        juce::NativeMessageBox::showOkCancelBox (juce::MessageBoxIconType::WarningIcon,
            "Overwrite Preset",
            "Preset \"" + presetDisplayName (name) + "\" already exists. Overwrite it?",
            nullptr,
            juce::ModalCallbackFunction::create ([this, name] (int okResult)
            {
                if (okResult != 0)
                    writePreset (name);
            }));
        return;
    }
    writePreset (name);
}

// Umbenennen: nur der Name, nie die Einstellungen (User-Vorgabe: "wenn
// Settings veraendert wurden sollen diese hierbei NICHT mit gespeichert
// werden!"). Die Datei wird umbenannt, der Inhalt bleibt exakt gleich - ein
// eventueller Stern am Namen bleibt deshalb auch stehen.
// Ein AlertWindow benutzt sonst das Standard-LookAndFeel und sieht damit aus
// wie aus einem anderen Programm. Farben kommen live aus der Theme-Palette,
// deshalb hier und nicht einmalig im LookAndFeel-Konstruktor.
void LCRMSAudioProcessorEditor::styleNameDialog (juce::AlertWindow& w)
{
    const auto pal = themePalette();
    w.setLookAndFeel (&lookAndFeel);
    w.setColour (juce::AlertWindow::backgroundColourId, pal.plate.interpolatedWith (juce::Colours::white, 0.05f));
    w.setColour (juce::AlertWindow::outlineColourId,    pal.frameMain.withAlpha (0.38f));
    w.setColour (juce::AlertWindow::textColourId,       juce::Colour (0xffdfe3ea));
    if (auto* te = w.getTextEditor ("name"))
    {
        te->setColour (juce::TextEditor::backgroundColourId,     pal.plate.darker (0.35f));
        te->setColour (juce::TextEditor::textColourId,           juce::Colour (0xffdfe3ea));
        te->setColour (juce::TextEditor::outlineColourId,        pal.frameMain.withAlpha (0.30f));
        te->setColour (juce::TextEditor::focusedOutlineColourId, pal.knob.withAlpha (0.55f));
        // Runde 42 (User): dunkle Schrift auf halbtransparentem Akzent war in
        // Sci-Fi kaum lesbar - jetzt helle Schrift auf der Akzentfarbe.
        te->setColour (juce::TextEditor::highlightColourId,      pal.knob.withAlpha (0.42f));
        te->setColour (juce::TextEditor::highlightedTextColourId, juce::Colours::white);
        te->setColour (juce::CaretComponent::caretColourId,      pal.knob);
    }

    // Runde 42 (User-Bug): Name war markiert, Tippen kam aber nicht an - das
    // Dialogfenster bekam im Host keinen Tastaturfokus. Nach dem Oeffnen
    // einmal nach vorne holen und dem Textfeld den Fokus geben.
    juce::Component::SafePointer<juce::AlertWindow> safe (&w);
    juce::MessageManager::callAsync ([safe]
    {
        if (safe == nullptr) return;
        safe->toFront (true);
        if (auto* te = safe->getTextEditor ("name"))
        {
            te->grabKeyboardFocus();
            te->selectAll();
        }
    });
}

// ===== AKTIVIERUNG =====
// Offline: die Nummer traegt ihre eigene Pruefsumme (Source/Licence.h), es
// wird also nichts verschickt und nichts nachgeschlagen.
void LCRMSAudioProcessorEditor::promptActivate()
{
    // Runde 54: eine bereits aktivierte Kopie wird NICHT mehr abgewiesen.
    // Sonst kaeme man nach der Aktivierung nie mehr an das Namensfeld heran -
    // und genau das braucht man, um "Registered to" nachzutragen.
    const bool alreadyLicensed = processor.licensed.load (std::memory_order_relaxed);

    presetNameDialog = std::make_unique<juce::AlertWindow> ("Activate SpaceX",
                                                              alreadyLicensed
                                                                ? juce::String ("This copy is activated.\n\n"
                                                                                "Enter your name and serial number to put\n"
                                                                                "your name on the back panel.")
                                                                : juce::String ("Enter your name and serial number,\n"
                                                                                "exactly as they appear in your order."),
                                                              juce::MessageBoxIconType::NoIcon);
    // Der Name gehoert zur Nummer: tools/make_serials.py leitet die Nutzlast
    // aus ihm ab, das Plugin prueft sie gegen den eingetippten Namen (siehe
    // Licence.h). Dadurch steht auf dem Back Panel nie ein erfundener Name.
    presetNameDialog->addTextEditor ("owner", juce::String(), "Your name");
    presetNameDialog->addTextEditor ("name", juce::String(), "SPX1-XXXX-XXXX-XXXX");
    if (auto* te = presetNameDialog->getTextEditor ("owner"))
    {
        te->setSelectAllWhenFocused (true);
        te->selectAll();
        // JUCE setzt den Fokus sonst auf das zuletzt angelegte Feld - der
        // Cursor stand dadurch in der Seriennummer statt im Namen (User).
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<juce::TextEditor> (te)]
        {
            if (safe != nullptr)
                safe->grabKeyboardFocus();
        });
    }
    presetNameDialog->addButton (alreadyLicensed ? "Save" : "Activate", 1, juce::KeyPress (juce::KeyPress::returnKey));
    presetNameDialog->addButton ("Cancel",   0, juce::KeyPress (juce::KeyPress::escapeKey));
    styleNameDialog (*presetNameDialog);

    presetNameDialog->enterModalState (true, juce::ModalCallbackFunction::create ([this, safe = juce::Component::SafePointer<LCRMSAudioProcessorEditor> (this)] (int result)
    {
        if (safe == nullptr)   // Review 1.0.1: Fenster schon geschlossen (siehe Demo-Dialog)
            return;
        juce::String entered, owner;
        if (result == 1 && presetNameDialog != nullptr)
        {
            entered = presetNameDialog->getTextEditorContents ("name");
            owner   = presetNameDialog->getTextEditorContents ("owner").trim();
        }
        presetNameDialog.reset();
        if (result != 1)
            return;

        if (! spacex::isValidSerial (entered))
        {
            juce::NativeMessageBox::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                "SpaceX", "That serial number was not recognised.\n\nIt looks like: SPX1-XXXX-XXXX-XXXX");
            return;
        }
        // Ein Name wurde eingetippt, passt aber nicht zur Nummer: nicht
        // aktivieren. Sonst koennte jeder einen beliebigen Namen eintragen und
        // "Registered to" waere wertlos. Ohne Namen (leeres Feld) gilt wie
        // bisher allein die Pruefsumme - dafuer gibt es die Zufallsnummern
        // fuer Tester.
        if (owner.isNotEmpty() && ! spacex::serialMatchesName (entered, owner))
        {
            juce::NativeMessageBox::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                "SpaceX", "Name and serial number do not match.\n\n"
                          "Please type your name exactly as it appears in your order, "
                          "or leave the name empty.");
            return;
        }

        processor.storeLicence (entered);
        {
            juce::PropertiesFile props (LCRMSAudioProcessor::appPropertiesOptions());
            props.setValue ("licenceName", owner);
            props.saveIfNeeded();
        }
        refreshSettingsPanel();
        content.repaint();
        juce::NativeMessageBox::showMessageBoxAsync (juce::MessageBoxIconType::NoIcon,
            "SpaceX", owner.isNotEmpty() ? "Activated. Thank you, " + owner + "."
                                         : juce::String ("Activated. Thank you for supporting independent plugins."));
    }), false);
}

void LCRMSAudioProcessorEditor::promptRenamePreset()
{
    if (currentPresetName.isEmpty() || isDefaultPresetName (currentPresetName))
        return;
    const juce::String oldName = resolvePresetKey (currentPresetName);

    presetNameDialog = std::make_unique<juce::AlertWindow> ("Rename Preset",
                                                              "New name:",
                                                              juce::MessageBoxIconType::NoIcon);
    presetNameDialog->addTextEditor ("name", presetDisplayName (oldName), "Preset name");
    if (auto* te = presetNameDialog->getTextEditor ("name")) { te->setSelectAllWhenFocused (true); te->selectAll(); }
    presetNameDialog->addButton ("Rename", 1, juce::KeyPress (juce::KeyPress::returnKey));
    presetNameDialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    styleNameDialog (*presetNameDialog);

    presetNameDialog->enterModalState (true, juce::ModalCallbackFunction::create ([this, oldName, safe = juce::Component::SafePointer<LCRMSAudioProcessorEditor> (this)] (int result)
    {
        if (safe == nullptr)   // Review 1.0.1: Fenster schon geschlossen (siehe Demo-Dialog)
            return;
        juce::String name;
        if (result == 1 && presetNameDialog != nullptr)
            name = presetNameDialog->getTextEditorContents ("name").removeCharacters ("\r\n").trim();
        presetNameDialog.reset();

        if (name.isEmpty() || name == presetDisplayName (oldName) || isDefaultPresetName (name))
            return;
        // Runde 109: umbenannt wird im selben Ordner (ausser man tippt selbst
        // "Ordner/Name").
        if (! name.containsChar ('/') && presetFolderOf (oldName).isNotEmpty())
            name = presetFolderOf (oldName) + "/" + name;
        if (presetFile (name).existsAsFile())
        {
            juce::NativeMessageBox::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                "Rename Preset", "A preset named \"" + name + "\" already exists.");
            return;
        }
        presetFile (name).getParentDirectory().createDirectory();
        if (presetFile (oldName).moveFileTo (presetFile (name)))
        {
            currentPresetName = resolvePresetKey (name);
            refreshPresetNameDisplay();
        }
    }), false);
}

void LCRMSAudioProcessorEditor::loadPreset (const juce::String& name)
{
    auto tree = presetTree (name);
    if (! tree.isValid())
        return;
    suppressGalaxyAutoArm = true;
    struct Unsuppress { bool& f; ~Unsuppress() { f = false; } } unsuppress { suppressGalaxyAutoArm };

    // Galaxy bleibt scharf, wenn es vorher scharf war (nur in EINE Richtung;
    // Ausschalten bleibt manuell) - vermeidet den Latenz-Interrupt.
    const bool galaxyWasArmed =
        processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_GALAXY_ACTIVATE)->load() > 0.5f;

    // Gesperrte Regler ueberleben den Preset-Wechsel (Rechtsklick auf Mix/Vol).
    auto* mixParam = processor.apvts.getParameter (LCRMSAudioProcessor::ID_MIX);
    const float mixBefore = mixParam != nullptr ? mixParam->getValue() : 1.0f;
    const bool  keepMix   = isMixLocked();
    auto* volParam = processor.apvts.getParameter (LCRMSAudioProcessor::ID_VOL_TRIM);
    const float volBefore = volParam != nullptr ? volParam->getValue() : 0.5f;
    const bool  keepVol   = isVolLocked();

    {
        auto migrated = tree.createCopy();
        LCRMSAudioProcessor::migrateMsEqOn (migrated);   // Runde 159
        processor.apvts.replaceState (migrated);
    }
    if (keepMix && mixParam != nullptr)
        mixParam->setValueNotifyingHost (mixBefore);
    if (keepVol && volParam != nullptr)
        volParam->setValueNotifyingHost (volBefore);

    // View-Einstellungen sind Preset-UNABHAENGIG (User): replaceState hat
    // das Kind verworfen, die aktuellen Werte werden wieder angehaengt.
    storeViewSettingsInState();

    // Ein Preset bringt seinen Galaxy-Zustand IMMER mit (User): ohne ihn
    // klingt das Preset nicht so, wie es gespeichert wurde. Der fruehere
    // Menuepunkt "Presets Switch Galaxy On/Off" ist deshalb entfallen - er
    // konnte nur dafuer sorgen, dass ein Preset anders klingt als beim
    // Speichern. Latenz aendert sich hier also bewusst.
    // Runde 35 (User: "Galaxy globally wird immer aktiviert"): der globale
    // Schalter gehoert NICHT zum Preset. Er bleibt, wie er vor dem Laden
    // war - Presets mit Galaxy sind bei ausgeschaltetem Galaxy ohnehin
    // grau bzw. werden uebersprungen.
    if (auto* act = processor.apvts.getParameter (LCRMSAudioProcessor::ID_GALAXY_ACTIVATE))
        if ((act->getValue() > 0.5f) != galaxyWasArmed)
            act->setValueNotifyingHost (galaxyWasArmed ? 1.0f : 0.0f);

    currentPresetName = name;
    presetSignature = computePresetSignature();
    presetDirty = false;
    refreshPresetNameDisplay();
}

void LCRMSAudioProcessorEditor::stepPreset (int direction)
{
    // Runde 163 (User: "Presets klicken - soll im Ordner bleiben"): die
    // Pfeile blaettern nur im Ordner des geladenen Presets und springen am
    // Ende wieder an den Anfang. Ohne Ordner (Default, lose Presets) nur
    // durch diese.
    const auto all = getPresetNames();
    const auto curKey = resolvePresetKey (currentPresetName);
    const auto folderOf = [] (const juce::String& k)
    {
        return k.containsChar ('/') ? k.upToFirstOccurrenceOf ("/", false, false) : juce::String();
    };
    const auto curFolder = folderOf (curKey);
    juce::StringArray names;
    for (const auto& n : all)
        if (folderOf (n) == curFolder)
            names.add (n);
    if (names.isEmpty())
        names = all;
    if (names.isEmpty())
        return;
    int index = names.indexOf (curKey, true);
    if (index < 0)
        index = (direction > 0) ? -1 : 0;

    // Galaxy global AUS (User): Presets, die Galaxy benutzen, beim
    // Durchblaettern ueberspringen - sonst wuerde das Preset Galaxy
    // scharfschalten (Latenz-Interrupt). Ueber die Liste bleiben sie ladbar.
    const bool galaxyArmed =
        processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_GALAXY_ACTIVATE)->load() > 0.5f;
    for (int tries = 0; tries < names.size(); ++tries)
    {
        index = (index + direction + names.size() * 2) % names.size();
        if (galaxyArmed || isDefaultPresetName (names[index]) || ! presetTreeUsesGalaxy (presetTree (names[index])))
        {
            loadPreset (names[index]);
            return;
        }
    }
}

// PARALLAX (Runde 34), vorlaeufig: Modus -> feste Drift/Shift-Werte,
// Amount skaliert sie linear. PLATZHALTER-Werte - werden durch die
// Einstellungen des Users (MicroPitch / altes Parallax) ersetzt.
void LCRMSAudioProcessorEditor::applyParallaxMode()
{
    // Runde 44: Modus + Amount wertet jetzt der Processor selbst aus
    // (Wegpunkte, siehe evalParallaxMode) - damit wirken auch Automation und
    // Modulation. Hier gibt es nichts mehr zu schreiben.
}

// Pruefsumme des LIVE-Zustands - ueber denselben Baum-Weg wie die A/B-Slots,
// damit beide Vergleiche dieselbe Gewichtung benutzen.
float LCRMSAudioProcessorEditor::computePresetSignature() const
{
    return signatureOfTree (processor.apvts.copyState());
}

float LCRMSAudioProcessorEditor::signatureOfTree (const juce::ValueTree& tree)
{
    float sum = 0.0f;
    int index = 1;
    for (int i = 0; i < tree.getNumChildren(); ++i)
    {
        auto child = tree.getChild (i);
        if (child.hasType ("PARAM"))
            sum += (float) child.getProperty ("value", 0.0f) * (float) index * 0.3719f;
        ++index;
    }
    return sum;
}

void LCRMSAudioProcessorEditor::refreshPresetNameDisplay()
{
    juce::String text = currentPresetName.isEmpty() ? juce::String (kDefaultPresetName)
                                                    : presetDisplayName (currentPresetName);
    if (presetDirty)
        text += " *";
    if (presetNameButton.getButtonText() != text)
        presetNameButton.setButtonText (text);
    presetNameButton.getProperties().set ("presetNameEmpty", false);
    presetDeleteButton.setEnabled (currentPresetName.isNotEmpty() && ! isDefaultPresetName (currentPresetName));
}

void LCRMSAudioProcessorEditor::deletePreset (const juce::String& name)
{
    if (isDefaultPresetName (name))
        return;
    presetFile (name).deleteFile();
    if (currentPresetName.equalsIgnoreCase (name))
    {
        // Nach dem Loeschen steht man auf "Default" - aber ohne den Zustand
        // anzufassen: der Klang bleibt, nur die Herkunft ist weg.
        currentPresetName = kDefaultPresetName;
        presetSignature = signatureOfTree (defaultPresetTree());
        presetDirty = std::abs (computePresetSignature() - presetSignature) > 1.0e-5f;
    }
    refreshPresetNameDisplay();
}

// Mod-Icon (Sinuswelle): einfacher An/Aus-Toggle-Button wie Power-Icon,
// schreibt aber einen der drei ID_*_MOD-Parameter statt eines Section-On.
void LCRMSAudioProcessorEditor::setupModButton (juce::TextButton& button, const juce::String& paramId,
                                                 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& attachment)
{
    button.setClickingTogglesState (true);
    button.getProperties().set ("modIcon", true);
    button.setWantsKeyboardFocus (false);
    content.addAndMakeVisible (button);
    attachment = std::make_unique<ButtonAttachment> (processor.apvts, paramId, button);
}

LCRMSAudioProcessorEditor::LCRMSAudioProcessorEditor (LCRMSAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p), goniometer (p),
      volInputMeter (p.currentInputLevel), volOutputMeter (p.currentOutputLevel),
      undoHistory (p.undoHistory), undoIndex (p.undoIndex),
      abSlotA (p.abSlotA), abSlotB (p.abSlotB), abCurrentIsA (p.abCurrentIsA)
{
    setLookAndFeel (&lookAndFeel);

    addAndMakeVisible (content);

    // Logo als Klickflaeche fuer den manuellen GUI-Bypass (kein Text/Icon,
    // rein zum Klicken - siehe "invisibleHit" in CustomLookAndFeel).
    logoButton.getProperties().set ("invisibleHit", true);
    logoButton.setWantsKeyboardFocus (false);
    content.addAndMakeVisible (logoButton);
    // Runde 56 (User): der Logo-Klick oeffnet das Back Panel. Bypass hat
    // seinen eigenen Knopf in der Global-Zeile - den Umweg ueber das Logo
    // braucht niemand, eine Rueckseite hinter dem Logo erwartet jeder.
    logoButton.onClick = [this] { showBackPanel(); };

    // --- Globale Buttons (oben rechts, User-Idee "Globale Buttons") --------
    // Reset: reine Aktion, kein eigener Zustand - setzt ALLE Parameter auf
    // ihren Default-Wert zurueck (inkl. Solo, Mod-Icons, Section-On/Off).
    globalResetButton.setClickingTogglesState (false);
    globalResetButton.setWantsKeyboardFocus (false);
    // Gleiche feste Schriftgroesse wie die Parameter-Labels (User-Feedback:
    // "alle Schriften in der Global-Zeile sollen so gross sein wie Expand,
    // Boost usw.").
    // Reset jetzt als Icon (User-Idee: "Kreis mit Pfeil Icon ... vielleicht
    // in Rot") - siehe drawResetIcon() im LookAndFeel. Der Text "RESET" ist
    // damit weg; das Icon sitzt in der Preset-Zeile neben dem Speichern-Icon.
    globalResetButton.getProperties().set ("resetIcon", true);
    globalResetButton.setTooltip ("Reset all parameters to default");
    content.addAndMakeVisible (globalResetButton);
    globalResetButton.onClick = [this]
    {
        // ID_GALAXY_ACTIVATE bewusst AUSGENOMMEN (User-Info: "Der Button
        // Reset soll den neuen globalen Galaxy-Button nicht resetten.") -
        // schaltet die Latenz/Engine, ein versehentliches Zuruecksetzen
        // waere ein unerwarteter Host-Interrupt bzw. wuerde die Sektion
        // ungefragt wieder deaktivieren. Per Pointer-Vergleich statt
        // getParameterID() (nicht Teil der Basisklasse AudioProcessorParameter
        // in dieser JUCE-Version).
        // Auto Gain ist ebenfalls ausgenommen (User-Bug: "Reset resettet
        // aktuell auch die Settings"). Es ist eine Arbeitseinstellung wie
        // Galaxy, kein Klangparameter - wer Regler zuruecksetzt, will nicht
        // gleichzeitig sein Messwerkzeug verlieren.
        auto* autoGainParam = processor.apvts.getParameter (LCRMSAudioProcessor::ID_AUTO_GAIN);
        auto* bassGuardParam = processor.apvts.getParameter (LCRMSAudioProcessor::ID_BASS_GUARD);
        auto* galaxyActivateParam = processor.apvts.getParameter (LCRMSAudioProcessor::ID_GALAXY_ACTIVATE);
        auto* mixLockedParam = isMixLocked() ? processor.apvts.getParameter (LCRMSAudioProcessor::ID_MIX) : nullptr;
        auto* volLockedParam = isVolLocked() ? processor.apvts.getParameter (LCRMSAudioProcessor::ID_VOL_TRIM) : nullptr;
        for (auto* param : processor.getParameters())
            if (param != nullptr && param != galaxyActivateParam && param != autoGainParam
                && param != bassGuardParam
                && param != mixLockedParam
                && param != volLockedParam && param != processor.getBypassParameter())
                param->setValueNotifyingHost (param->getDefaultValue());
    };

    // "ACTIVATE GALAXY": globaler, echter APVTS-Parameter - schaltet NUR
    // die STFT-Engine/Latenz (siehe DSP-Kommentar zu ID_GALAXY_ACTIVATE).
    // Bewusst simpler Toggle mit der "globalBtn"-Textoptik (hell = an,
    // gedimmt = aus) statt einer eigenen Zeichenroutine.
    globalGalaxyActivateButton.setClickingTogglesState (true);
    globalGalaxyActivateButton.setWantsKeyboardFocus (false);
    globalGalaxyActivateButton.getProperties().set ("globalBtn", true);
    // "galaxyBtn": eigener, deutlich auffaelligerer Glow-Hintergrund wenn
    // aktiv (User-Wunsch: "Galaxy muss DEUTLICH auffaelliger sein wenn
    // aktiv"), siehe CustomLookAndFeel::drawGalaxyButtonBackground().
    globalGalaxyActivateButton.getProperties().set ("galaxyBtn", true);
    globalGalaxyActivateButton.getProperties().set ("softChip", true);   // Runde 108
    content.addAndMakeVisible (globalGalaxyActivateButton);
    globalGalaxyActivateAttachment = std::make_unique<ButtonAttachment> (processor.apvts, LCRMSAudioProcessor::ID_GALAXY_ACTIVATE, globalGalaxyActivateButton);
    // Neu (User-Wunsch): globales Galaxy AUSschalten schaltet auch die
    // Galaxy/LCR-Sektion aus. Die Gegenrichtung bleibt unveraendert: die
    // Sektion einzuschalten aktiviert Global Galaxy automatisch mit (siehe
    // activateGalaxyIfNeeded()), aber die Sektion auszuschalten laesst
    // Global Galaxy bewusst an (User-Bestaetigung "beim Ausschalten muss es
    // dann an bleiben").
    // Nachtrag (User-Feedback: "wenn Section Galaxy on ist und ich danach
    // global galaxy deaktiviere und wieder aktiviere soll section galaxy auch
    // wieder on sein"). Richtig - der globale Schalter soll die Engine
    // schlafen legen, nicht die Einstellung darunter vergessen. Sonst muss man
    // nach jedem Latenz-Aus/An die Sektion von Hand wieder suchen, obwohl man
    // sie nie ausgeschaltet hat.
    //
    // Deshalb wird beim AUSschalten gemerkt, wie die Sektion stand, und beim
    // Wiedereinschalten genau dieser Zustand hergestellt. Wer die Sektion in
    // der Zwischenzeit bewusst ausgeschaltet gelassen hat, bekommt sie auch
    // nicht zurueck - gemerkt wird der Stand im Moment des Abschaltens.
    globalGalaxyActivateButton.onClick = [this]
    {
        auto* lcrParam = processor.apvts.getParameter (LCRMSAudioProcessor::ID_LCR_ENABLED);
        if (lcrParam == nullptr)
            return;

        // onClick feuert NACH dem Umschalten - getToggleState() ist also
        // bereits der neue Zustand.
        if (! globalGalaxyActivateButton.getToggleState())
        {
            galaxySectionWasOnBeforeDeactivate = lcrParam->getValue() > 0.5f;
            lcrParam->setValueNotifyingHost (0.0f);
        }
        else if (galaxySectionWasOnBeforeDeactivate && lcrParam->getValue() < 0.5f)
        {
            lcrParam->setValueNotifyingHost (1.0f);
        }
    };

    // "BREATHE": reine Aktion - schaltet in allen 5 Mod-Sektionen den
    // Mod-Toggle an und wuerfelt die Tiefe-Regler neu (User-Idee: "Leben
    // in das Plugin einhauchen").
    globalBreatheButton.setClickingTogglesState (false);
    globalBreatheButton.setWantsKeyboardFocus (false);
    // "breatheIcon": vereinheitlichter Icon+Text-Stil wie Mod On/Off/BYP/
    // Mutate (User-Feedback: "Jeder der 3 Buttons sieht anders aus") - ein
    // staendig sanft pulsierender Kreis statt des alten Glow-Pillen-
    // Hintergrunds, siehe CustomLookAndFeel::drawBreatheContent().
    globalBreatheButton.getProperties().set ("breatheIcon", true);
    content.addAndMakeVisible (globalBreatheButton);
    globalBreatheButton.onClick = [this]
    {
        // Kleiner Strich im Icon wechselt bei jedem Klick die Position
        // (User-Wunsch: "Jedes Mal, wenn man auf das Icon klickt veraendert
        // sich die Position des Striches") - 8 feste Positionen im Kreis,
        // wie ein Regler-Zeiger, siehe drawBreatheContent().
        auto& props = globalBreatheButton.getProperties();
        // Zufaellige statt reihum durchlaufender Position (User-Wunsch:
        // "Randomize Regler Icon soll random Positionen erhalten - momentan
        // dreht er sich im Uhrzeigersinn"). Der alte "+1"-Schritt liess den
        // Zeiger bei wiederholtem Klicken sauber im Kreis wandern, was nach
        // einem geordneten Ablauf aussah statt nach Zufall - genau das
        // Gegenteil der Aussage des Buttons. Der jeweils aktuelle Wert wird
        // ausgeschlossen, damit ein Klick NIE wirkungslos aussieht.
        const int currentState = (int) props.getWithDefault ("breatheStrokeState", 0);
        int next = juce::Random::getSystemRandom().nextInt (7); // 0..6
        if (next >= currentState) ++next;                       // aktuellen Wert ueberspringen -> 0..7 ohne current
        props.set ("breatheStrokeState", next);

        juce::Random& rng = juce::Random::getSystemRandom();
        // Section-Lock (User-Wunsch): eine gesperrte Sektion wird von
        // Breathe komplett uebersprungen (weder Mod an, noch Tiefe neu
        // gewuerfelt).
        auto breatheOne = [&] (int soloSectionId, const juce::String& modId, const juce::String& depthId)
        {
            if (processor.isSectionLocked (soloSectionId))
                return;
            if (auto* modParam = processor.apvts.getParameter (modId))
                modParam->setValueNotifyingHost (1.0f);
            if (auto* depthParam = processor.apvts.getParameter (depthId))
                depthParam->setValueNotifyingHost (juce::jmap (rng.nextFloat(), 0.10f, 1.0f));
        };
        breatheOne (LCRMSAudioProcessor::SOLO_TIMEWARP,   LCRMSAudioProcessor::ID_TIMEWARP_MOD,   LCRMSAudioProcessor::ID_TIMEWARP_DEPTH);
        breatheOne (LCRMSAudioProcessor::SOLO_DIMENSION,  LCRMSAudioProcessor::ID_DIMENSION_MOD,  LCRMSAudioProcessor::ID_DIMENSION_DEPTH);
        breatheOne (LCRMSAudioProcessor::SOLO_HYPERDRIVE, LCRMSAudioProcessor::ID_HYPERDRIVE_MOD, LCRMSAudioProcessor::ID_HYPERDRIVE_DEPTH);
        breatheOne (LCRMSAudioProcessor::SOLO_GALAXY,     LCRMSAudioProcessor::ID_GALAXY_MOD,     LCRMSAudioProcessor::ID_GALAXY_DEPTH);
        breatheOne (LCRMSAudioProcessor::SOLO_POSITION,   LCRMSAudioProcessor::ID_POSITION_MOD,   LCRMSAudioProcessor::ID_POSITION_DEPTH);
    };

    // "SAVE": vorher nur Fenstergroesse (User-Feedback: "Save hat keine
    // Funktion. Wie sollen wir das loesen?") - oeffnet jetzt einen Namens-
    // Dialog und speichert den KOMPLETTEN aktuellen Plugin-Zustand als
    // benanntes Preset (siehe promptAndSaveNewPreset()). Fenstergroesse als
    // Standard laesst sich weiterhin ueber das Hamburger-Menue setzen
    // ("Save window size as default").
    globalSaveSizeButton.setClickingTogglesState (false);
    globalSaveSizeButton.setWantsKeyboardFocus (false);
    // Speichern jetzt als Disketten-Icon (User-Idee) - siehe drawSaveIcon().
    globalSaveSizeButton.getProperties().set ("saveIcon", true);
    globalSaveSizeButton.setTooltip ("Save preset");
    content.addAndMakeVisible (globalSaveSizeButton);
    globalSaveSizeButton.onClick = [this] { promptAndSaveNewPreset(); };

    // Der frueher hier aufgebaute "LOAD"-Button ist ENTFALLEN (User: "Wenn
    // der Preset Name da steht kann Load weg - ist ja redundant"). Seine
    // Belegung (Linksklick = laden, Rechtsklick = loeschen) ist unveraendert
    // auf das Preset-Namensfeld gewandert, siehe presetNameButton.
    // Auch das Popup haengt jetzt am Namensfeld statt am LOAD-Button.

    // "CHAOS": wuerfelt wirklich ALLE Regler/Mods neu (User-Wunsch), per
    // Exclusion-Liste (Pointer-Vergleich, wie schon bei RESET) ausgenommen:
    // Vol Trim, Mono Check + dessen Dry-Vergleich, sowie die beiden echten
    // globalen APVTS-Schalter (Activate Galaxy, globaler Mod-Bypass). Solo
    // bleibt ebenfalls unangetastet (siehe Kommentar im Header). Gegen die
    // vom User befuerchteten "Vol Jumps": der Processor duckt den Output
    // kurz, waehrend die neuen Werte einlaufen (siehe chaosTriggerRequested).
    globalChaosButton.setClickingTogglesState (false);
    globalChaosButton.setWantsKeyboardFocus (false);
    // "mutateIcon": vereinheitlichter Icon+Text-Stil (Wuerfel-Icon), siehe
    // CustomLookAndFeel::drawMutateContent() - ersetzt die alte reine
    // "globalBtn"-Textoptik (User-Feedback: "Jeder der 3 Buttons sieht
    // anders aus").
    globalChaosButton.getProperties().set ("mutateIcon", true);
    content.addAndMakeVisible (globalChaosButton);
    // Runde 55 (User): nur noch EIN Wuerfel, und der darf IMMER Sektionen
    // aus- und einschalten - mit Kategorie wie ohne. Die Begruendung des
    // Users ist die richtige: "Wenn eine Sektion sowieso keine Aenderungen
    // randomly bekommt, dann kann sie auch off gestellt werden pro
    // Wuerfelrunde" - sonst sieht man dem Wurf nicht an, was er getan hat.
    globalChaosButton.onClick  = [this] { runMutate (true); };

    // Zweite Mutate-Taste: identische Logik, darf aber zusaetzlich Sektionen
    // ausschalten (User-Idee). Eigenes Icon mit zwei grauen Kaestchen, siehe
    // CustomLookAndFeel::drawMutateContent().
    globalChaosSectionsButton.setVisible (false);   // Runde 55: aufgegangen im einen Wuerfel
    globalChaosSectionsButton.setClickingTogglesState (false);
    globalChaosSectionsButton.setWantsKeyboardFocus (false);
    globalChaosSectionsButton.getProperties().set ("mutateIcon", true);
    globalChaosSectionsButton.getProperties().set ("mutateSectionsIcon", true);
    content.addAndMakeVisible (globalChaosSectionsButton);
    globalChaosSectionsButton.onClick = [this] { runMutate (true); };

    // Globaler Mod-Bypass: echter APVTS-Parameter (automatisierbar, Teil des
    // gespeicherten Zustands), schaltet alle 3 LFO-Modulationen gemeinsam
    // stumm, ohne die einzelnen Mod-Icons zu veraendern (siehe DSP).
    globalModBypassButton.setClickingTogglesState (true);
    globalModBypassButton.setWantsKeyboardFocus (false);
    globalModBypassButton.getProperties().set ("modBypassIcon", true);
    content.addAndMakeVisible (globalModBypassButton);
    globalModBypassAttachment = std::make_unique<ButtonAttachment> (processor.apvts, LCRMSAudioProcessor::ID_GLOBAL_MOD_BYPASS, globalModBypassButton);

    // LIFE (Runde 37): kleiner Regler direkt neben dem Mod-Bypass - skaliert
    // alle Modulationstiefen gemeinsam. Gleicher Stil wie die Tiefe-Regler
    // in den Sektionskoepfen.
    styleRotary (lifeSlider, false);
    content.addAndMakeVisible (lifeSlider);
    lifeAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_LIFE, lifeSlider);
    lifeSlider.setDoubleClickReturnValue (true, 0.0, juce::ModifierKeys::commandModifier);

    // "BYP": neuer globaler Bypass-Button (User-Wunsch: "BYP Button links
    // von Chaos/Mutate") - schaltet denselben rein GUI-seitigen Bypass wie
    // der bestehende Logo-Klick (siehe logoButton.onClick oben,
    // processor.uiBypassed), damit beide Wege synchron bleiben. Kein eigener
    // APVTS-Parameter, daher kein ButtonAttachment - der Toggle-Status wird
    // stattdessen in timerCallback() aus processor.uiBypassed nachgezogen.
    globalBypassButton.setClickingTogglesState (false);
    globalBypassButton.setWantsKeyboardFocus (false);
    globalBypassButton.getProperties().set ("bypassToggleIcon", true);
    content.addAndMakeVisible (globalBypassButton);
    // Bug-Fix (User-Feedback: "BYP Bug. Mache es einfach genau so wie wenn
    // man auf das Space X Logo klickt.") - ruft jetzt DIESELBE Funktion wie
    // der Logo-Klick auf (toggleUiBypass()), statt den Bypass-Zustand separat
    // (ohne das noetige content.repaint() fuer den Abdunkel-Schleier) zu
    // setzen - vorher blieb der visuelle Bypass-Effekt beim Klick auf BYP
    // teils bis zum naechsten ohnehin faelligen Repaint verzoegert/aus.
    globalBypassButton.onClick = [this] { toggleUiBypass(); };

    // A/B: reiner GUI-Snapshot-Vergleich (nicht Teil des gespeicherten
    // Plugin-Zustands) - Klick sichert den aktuellen (noch aktiven) Zustand
    // in seinen Slot und laedt den jeweils anderen Slot. "A/B" steht immer
    // auf dem Button (User-Feedback), nur der aktive Buchstabe leuchtet.
    globalABButton.setClickingTogglesState (false);
    globalABButton.setWantsKeyboardFocus (false);
    globalABButton.getProperties().set ("abIcon", true);
    globalABButton.getProperties().set ("abIsA", true);
    content.addAndMakeVisible (globalABButton);
    // Runde 84: nur befuellen, wenn noch nichts da ist. Kam A/B aus dem
    // geladenen Zustand, wuerde ein Ueberschreiben hier genau den Bug
    // zurueckbringen, den wir gerade beseitigt haben.
    if (! abSlotA.isValid()) abSlotA = processor.apvts.copyState();
    if (! abSlotB.isValid()) abSlotB = processor.apvts.copyState();
    globalABButton.getProperties().set ("abIsA", abCurrentIsA);
    globalABButton.onClick = [this]
    {
        // Runde 84 (User): jeder Slot merkt sich seinen eigenen Preset-Namen.
        // Vorher blieb beim Umschalten der Name des zuletzt geladenen Presets
        // stehen und bekam ein Sternchen - obwohl der andere Slot in Wahrheit
        // ein anderes Preset ist.
        (abCurrentIsA ? abSlotA : abSlotB) = processor.apvts.copyState();
        (abCurrentIsA ? processor.abNameA : processor.abNameB) = currentPresetName;
        const bool dirtyBefore = presetDirty;
        (abCurrentIsA ? abDirtyA : abDirtyB) = dirtyBefore;
        abCurrentIsA = ! abCurrentIsA;
        auto* mixParam = processor.apvts.getParameter (LCRMSAudioProcessor::ID_MIX);
        const float mixBefore = mixParam != nullptr ? mixParam->getValue() : 1.0f;
        processor.apvts.replaceState (abCurrentIsA ? abSlotA : abSlotB);
        if (isMixLocked() && mixParam != nullptr)
            mixParam->setValueNotifyingHost (mixBefore);
        currentPresetName = abCurrentIsA ? processor.abNameA : processor.abNameB;
        presetDirty       = abCurrentIsA ? abDirtyA : abDirtyB;
        refreshPresetNameDisplay();
        storeViewSettingsInState();   // View gehoert nicht zu A/B
        globalABButton.getProperties().set ("abIsA", abCurrentIsA);
        globalABButton.repaint();
        abCopyButton.getProperties().set ("abCopyToRight", abCurrentIsA);
        abCopyButton.repaint();
    };

    // ===== COPY (neben A/B) =====
    // User-Wunsch: "Copy muss leuchten wenn A != B ist und ueberträgt alle
    // Einstellungen des Plugins auf den nicht-ausgewaehlten Buchstaben ...
    // Copy erlischt wenn A = B."
    //
    // Icon: ein Pfeil, der in die KOPIERRICHTUNG zeigt (A aktiv -> nach
    // rechts zu B; B aktiv -> nach links zu A). Bewusst kein Doppelpfeil:
    // ein Doppelpfeil sagt "tauschen", Copy tauscht aber nicht, es
    // ueberschreibt eine Seite mit der anderen. Der einfache Pfeil sagt
    // genau das - und weil er die Richtung wechselt, sieht man ausserdem
    // ohne Nachdenken, WELCHER Slot gleich ueberschrieben wird.
    abCopyButton.setClickingTogglesState (false);
    abCopyButton.setWantsKeyboardFocus (false);
    abCopyButton.getProperties().set ("abCopyIcon", true);
    abCopyButton.setTooltip ("Copy current state to the other A/B slot");
    content.addAndMakeVisible (abCopyButton);
    abCopyButton.onClick = [this]
    {
        // Der aktive Slot ist per Definition der Live-Zustand. Der andere
        // bekommt eine Kopie davon - ab jetzt sind beide gleich, das Icon
        // erlischt beim naechsten Tick von selbst.
        (abCurrentIsA ? abSlotB : abSlotA) = processor.apvts.copyState();
        (abCurrentIsA ? processor.abNameB : processor.abNameA) = currentPresetName;
        (abCurrentIsA ? abDirtyB : abDirtyA) = presetDirty;
    };

    // --- LCR --------------------------------------------------------------
    setupPowerButton (lcrPowerButton, LCRMSAudioProcessor::ID_LCR_ENABLED, lcrAttachment, LCRMSAudioProcessor::SOLO_GALAXY);
    // Eigene, sonst nirgends im Plugin verwendete Akzentfarbe (Blau) statt
    // des gemeinsamen Tuerkis/Lila-Wechsels der anderen 5 Sektionen - Galaxy
    // ist strukturell besonders (verursacht als einzige Sektion Latenz),
    // soll sich daher optisch klar abheben (User-Feedback), waehrend die
    // uebrigen Sektionen ihre bunte Abwechslung behalten.
    styleTitle (lcrTitleLabel, "GALAXY", juce::Colour (0xff4fa8ff));
    setupClickableTitle (lcrTitleLabel, LCRMSAudioProcessor::ID_LCR_ENABLED, LCRMSAudioProcessor::SOLO_GALAXY);
    content.addAndMakeVisible (lcrTitleLabel);
    setupSoloButton (lcrSoloButton, LCRMSAudioProcessor::SOLO_GALAXY);
    setupLockButton (lcrLockButton, LCRMSAudioProcessor::SOLO_GALAXY);

    styleRotary (gravitySlider, false);
    gravitySlider.getProperties().set ("centerOut", true);
    content.addAndMakeVisible (gravitySlider);
    styleLabel (gravityLabel, "Gravity");
    content.addAndMakeVisible (gravityLabel);
    gravityAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_LCR_SENS, gravitySlider);
    // Reset auf Default-Wert per Cmd-Klick statt JUCE-Standard (Alt/Option-
    // Klick) - User-Feedback, gilt einheitlich fuer alle Regler im Plugin.
    // Muss NACH dem Attachment gesetzt werden, da SliderAttachment selbst
    // intern schon setDoubleClickReturnValue() mit dem Alt-Modifier aufruft.
    gravitySlider.setDoubleClickReturnValue (true, 50.0, juce::ModifierKeys::commandModifier);

    orbitSlider.setSliderStyle (juce::Slider::LinearVertical);
    orbitSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    orbitSlider.getProperties().set ("focusStyle", true);
    content.addAndMakeVisible (orbitSlider);
    styleLabel (orbitLabel, "Orbit");
    content.addAndMakeVisible (orbitLabel);
    focusAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_LCR_BLEND, orbitSlider);
    orbitSlider.setDoubleClickReturnValue (true, 0.0, juce::ModifierKeys::commandModifier);

    // HORIZON - obere Bandgrenze. Voller Ausschlag = aus, deshalb ist der
    // Default auch der Rechtsanschlag und kein Mittelwert.
    styleRotary (horizonSlider, false);
    content.addAndMakeVisible (horizonSlider);
    styleLabel (horizonLabel, "Regain");
    content.addAndMakeVisible (horizonLabel);
    horizonAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_LCR_HORIZON, horizonSlider);
    horizonSlider.setDoubleClickReturnValue (true, 0.0, juce::ModifierKeys::commandModifier);
    horizonSlider.textFromValueFunction = [] (double v)
    {
        if (v < 0.5) return juce::String ("Off");
        const double hz = 20000.0 * std::pow (0.025, v * 0.01);
        if (hz >= 1000.0) return juce::String (hz / 1000.0, hz >= 10000.0 ? 1 : 2) + " kHz";
        return juce::String ((int) std::round (hz)) + " Hz";
    };
    horizonSlider.updateText();
    // Die Regler zeigen bewusst keine Zahlen an - bei einer FREQUENZ ist die
    // Zahl aber die halbe Information. Deshalb steht sie im Tooltip und
    // damit in der Hinweiszeile unten, und sie wandert beim Drehen mit.
    horizonSlider.onValueChange = [this]
    {
        horizonSlider.setTooltip ("Regain " + horizonSlider.getTextFromValue (horizonSlider.getValue())
                                  + ": turn up to keep more of the top end in the sides");
    };
    horizonSlider.onValueChange();

    // Galaxy-Mod (Gravity + Orbit) - genau dasselbe Prinzip wie bei Timewarp/
    // Dimension/Hyperdrive (User-Feedback).
    setupModButton (galaxyModButton, LCRMSAudioProcessor::ID_GALAXY_MOD, galaxyModAttachment);
    styleRotary (galaxyModDepthSlider, false);
    content.addAndMakeVisible (galaxyModDepthSlider);
    galaxyDepthAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_GALAXY_DEPTH, galaxyModDepthSlider);
    wireModAutoEnable (galaxyModDepthSlider, galaxyModButton, LCRMSAudioProcessor::ID_GALAXY_MOD);
    galaxyModDepthSlider.setDoubleClickReturnValue (true, 50.0, juce::ModifierKeys::commandModifier);

    // --- Drift (Haas + Bend) ------------------------------------------------
    setupPowerButton (driftPowerButton, LCRMSAudioProcessor::ID_DRIFT_ON, driftOnAttachment, LCRMSAudioProcessor::SOLO_TIMEWARP);
    // "Timewarp" statt "Drift" als Header, damit sich Header und Regler-Label
    // ("Drift") nicht mehr wiederholen.
    // PARALLAX statt TIMEWARP: sobald Tilt neben Drift steht, geht es nicht
    // mehr um Zeit, sondern um die seitliche Verschiebung des Bildes -
    // einmal ueber Zeit (Drift), einmal ueber Pegel (Tilt).
    styleTitle (driftTitleLabel, "PARALLAX", juce::Colour (0xffb968ff));
    setupClickableTitle (driftTitleLabel, LCRMSAudioProcessor::ID_DRIFT_ON, LCRMSAudioProcessor::SOLO_TIMEWARP);
    content.addAndMakeVisible (driftTitleLabel);
    setupSoloButton (driftSoloButton, LCRMSAudioProcessor::SOLO_TIMEWARP);
    setupLockButton (driftLockButton, LCRMSAudioProcessor::SOLO_TIMEWARP);
    setupModButton (driftModButton, LCRMSAudioProcessor::ID_TIMEWARP_MOD, timewarpModAttachment);
    styleRotary (driftModDepthSlider, false);
    content.addAndMakeVisible (driftModDepthSlider);
    timewarpDepthAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_TIMEWARP_DEPTH, driftModDepthSlider);
    wireModAutoEnable (driftModDepthSlider, driftModButton, LCRMSAudioProcessor::ID_TIMEWARP_MOD);
    driftModDepthSlider.setDoubleClickReturnValue (true, 50.0, juce::ModifierKeys::commandModifier);

    styleRotary (driftSlider, false);
    driftSlider.getProperties().set ("centerOut", true);
    content.addAndMakeVisible (driftSlider);
    styleLabel (driftLabel, "Drift");
    content.addAndMakeVisible (driftLabel);
    driftAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_DRIFT, driftSlider);
    driftSlider.setDoubleClickReturnValue (true, 0.0, juce::ModifierKeys::commandModifier);

    // "Balance": kleines Icon direkt neben dem Drift-Label (User-Feedback,
    // Fiedler Audio Splat als Referenz genannt) - automatische Gain-
    // Kompensation fuer den Haas-Praezedenzeffekt, siehe DSP.
    // Filter-Bypass-Icons (User, Runde 26): Galaxy und Dimension arbeiten
    // standardmaessig innerhalb des Filters; ein Klick nimmt die Sektion
    // wieder heraus. Analog zum Balance-Icon in Timewarp aufgebaut.
    for (auto* b : { &galaxyFilterButton, &dimFilterButton, &posFilterButton })
    {
        b->setClickingTogglesState (true);
        b->setWantsKeyboardFocus (false);
        b->getProperties().set ("filterIcon", true);
        content.addChildComponent (*b);
        // Runde 30: unsichtbar. Der Knopf bleibt samt Attachment bestehen
        // (kein zweiter Weg fuer denselben Zustand), nimmt aber keinen Platz
        // mehr ein und ist nicht mehr bedienbar. Der Focus wirkt jetzt
        // einheitlich auf Galaxy, Dimension und Vision.
        b->setVisible (false);
        b->setEnabled (false);
    }
    // --- Parallax-Hochpass (Runde 76, Test) ---
    // Drei Stufen auf EINEM Knopf: aus / 400 / 800. Der Zustand steht im
    // Symbol (Zahl der Kurvenstriche), nicht in einem zweiten Element.
    parallaxHpButton.setClickingTogglesState (true);
    parallaxHpButton.setWantsKeyboardFocus (false);
    parallaxHpButton.getProperties().set ("filterIcon", true);
    parallaxHpButton.getProperties().set ("focusDirect", true);
    content.addAndMakeVisible (parallaxHpButton);
    parallaxHpBtnAttachment = std::make_unique<ButtonAttachment> (processor.apvts, LCRMSAudioProcessor::ID_PX_HP, parallaxHpButton);


    galaxyFilterAttachment = std::make_unique<ButtonAttachment> (processor.apvts, LCRMSAudioProcessor::ID_PRISM_GALAXY, galaxyFilterButton);
    dimFilterAttachment    = std::make_unique<ButtonAttachment> (processor.apvts, LCRMSAudioProcessor::ID_PRISM_DIM,    dimFilterButton);
    posFilterAttachment    = std::make_unique<ButtonAttachment> (processor.apvts, LCRMSAudioProcessor::ID_PRISM_VIS,    posFilterButton);

    driftBalanceButton.setClickingTogglesState (true);
    driftBalanceButton.setWantsKeyboardFocus (false);
    driftBalanceButton.getProperties().set ("balanceIcon", true);
    content.addAndMakeVisible (driftBalanceButton);
    driftBalanceAttachment = std::make_unique<ButtonAttachment> (processor.apvts, LCRMSAudioProcessor::ID_TIMEWARP_BALANCE, driftBalanceButton);

    // Bend: Eventide-MicroPitch-artiger Mini-Detune-Regler (0-10 Cent,
    // L runter / R rauf), gehoert mit ins Drift-Frame.
    styleRotary (bendSlider, false);
    content.addAndMakeVisible (bendSlider);
    styleLabel (bendLabel, "Shift");
    content.addAndMakeVisible (bendLabel);
    bendAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_BEND, bendSlider);
    bendSlider.setDoubleClickReturnValue (true, 0.0, juce::ModifierKeys::commandModifier);

    // PARALLAX neu (Runde 34): ein Regler (Amount) + vier Modus-Knoepfe.
    // Vorlaeufig: der Modus waehlt feste Drift/Shift-Werte, Amount skaliert
    // sie (siehe applyParallaxMode). Die echten Modi folgen mit den Werten
    // vom User.
    styleRotary (parallaxAmountSlider, false);
    content.addAndMakeVisible (parallaxAmountSlider);
    styleLabel (parallaxAmountLabel, "Amount");
    content.addAndMakeVisible (parallaxAmountLabel);
    parallaxAmountAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_PARALLAX_AMOUNT, parallaxAmountSlider);
    parallaxAmountSlider.setDoubleClickReturnValue (true, 0.0, juce::ModifierKeys::commandModifier);
    // Nur echte Bedienung schreibt Drift/Shift - nicht das Nachziehen durch
    // Preset/Automation, sonst wuerden alte Presets (Amount 0) ihr Drift
    // verlieren.
    parallaxAmountSlider.onValueChange = [this]
    {
        if (parallaxAmountSlider.isMouseOverOrDragging())
            applyParallaxMode();
    };
    {
        // Runde 45: fuenf Modi aus den User-Presets (Namen folgen).
        static const char* modeNames[kPxModes] = { "VELVET", "HALO", "ILLUSION", "DOUBLE" };
        static const char* modeTips[kPxModes]  = { "Velvet: the gentlest - soft and close, the centre barely moves",
                                                   "Halo: a soft ring around the sound - stays centred, holds up on a full mix",
                                                   "Illusion: sounds wider than it is, but stays tight",
                                                   "Double: the widest of the four, with the strongest character" };
        for (int i = 0; i < kPxModes; ++i)
        {
            auto& b = parallaxModeButtons[i];
            b.setButtonText (modeNames[i]);
            b.setTooltip (modeTips[i]);
            b.setClickingTogglesState (true);
            b.getProperties().set ("thinOnFrame", true);
            b.setRadioGroupId (4343, juce::dontSendNotification);
            b.setWantsKeyboardFocus (false);
            content.addAndMakeVisible (b);
            b.onClick = [this, i]
            {
                if (auto* prm = processor.apvts.getParameter (LCRMSAudioProcessor::ID_PARALLAX_MODE))
                    prm->setValueNotifyingHost (prm->convertTo0to1 ((float) i));
                applyParallaxMode();
            };
        }
       #if SPACEX_PARALLAX_UI == 2
        // SpaceXclick (Runde 41): EIN Knopf zeigt den Modus, Klick = weiter,
        // Cmd+Klick = zurueck. Die anderen drei Knoepfe sind aus.
        {
            auto& b = parallaxModeButtons[0];
            b.setRadioGroupId (0, juce::dontSendNotification);
            b.setClickingTogglesState (false);
            b.getProperties().set ("modePill", true);
            b.setTooltip ("Parallax mode: click for the next one, Cmd-click to go back");
            b.onClick = [this]
            {
                if (auto* prm = processor.apvts.getParameter (LCRMSAudioProcessor::ID_PARALLAX_MODE))
                {
                    const int cur = juce::jlimit (0, kPxModes - 1, (int) std::round (prm->convertFrom0to1 (prm->getValue())));
                    const bool back = juce::ModifierKeys::currentModifiers.isCommandDown();
                    const int next = (cur + (back ? kPxModes - 1 : 1)) % kPxModes;
                    prm->setValueNotifyingHost (prm->convertTo0to1 ((float) next));
                }
                applyParallaxMode();
            };
            for (int i = 1; i < kPxModes; ++i) { parallaxModeButtons[i].setVisible (false); parallaxModeButtons[i].setEnabled (false); }
            pxModeDots.count = kPxModes;
            // Runde 82 (User): kleine Luecke nach den ersten beiden - links
            // die Modi, die formen und mittig bleiben (Halo, 3D), rechts die,
            // die breit machen (Double, Illusion).
            pxModeDots.groupAfter = 0;   // Runde 89 (User): Luecke wieder weg
           #if SPACEX_PX_DIAG_ONLY
            // Runde 94 (User: "beim Laden kommt kurz eine Box um die Icons"):
            // das Icon-Merkmal wurde erst im Timer gesetzt, der erste Anstrich
            // sah also einen Knopf OHNE Icon - und der traegt einen Rahmen.
            // Jetzt steht es schon vor dem ersten Zeichnen.
            parallaxModeButtons[0].getProperties().set ("pxDiagram",
                juce::jlimit (0, kPxModes - 1,
                    (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_PARALLAX_MODE)->load())));
           #endif
            pxModeDots.setTooltip ("Parallax mode: click a dot to pick it directly");
            pxModeDots.onPick = [this] (int i)
            {
                if (auto* prm = processor.apvts.getParameter (LCRMSAudioProcessor::ID_PARALLAX_MODE))
                    prm->setValueNotifyingHost (prm->convertTo0to1 ((float) i));
            };
            content.addAndMakeVisible (pxModeDots);
        }
       #endif
    }

    // --- Polarity -----------------------------------------------------------
    setupPowerButton (polPowerButton, LCRMSAudioProcessor::ID_POL_ON, polOnAttachment, LCRMSAudioProcessor::SOLO_POLARITY);
    // Zurueck zu "Polarity" (User-Feedback) - "Flip" war zu kurz/unklar.
    // ECLIPSE statt POLARITY: eine Finsternis ist woertlich der Fall, in dem
    // sich zwei Dinge ausloeschen - genau die Physik dahinter. Und der Name
    // verspricht einen Effekt, statt nach Werkzeugkasten zu klingen; genau
    // darum geht es hier (User: "was man nicht sieht, macht man nicht").
    styleTitle (polTitleLabel, "ECLIPSE", juce::Colour (0xffb968ff));
    setupClickableTitle (polTitleLabel, LCRMSAudioProcessor::ID_POL_ON, LCRMSAudioProcessor::SOLO_POLARITY);
    content.addAndMakeVisible (polTitleLabel);
    setupSoloButton (polSoloButton, LCRMSAudioProcessor::SOLO_POLARITY);
    setupLockButton (polLockButton, LCRMSAudioProcessor::SOLO_POLARITY);

    polLButton.setClickingTogglesState (true);
    polRButton.setClickingTogglesState (true);
    // User: "Polarity L/R und 1-4 brauchen nicht so einen visuellen Fokus" -
    // der An-Rahmen ist hier minimal duenner als bei den uebrigen Knoepfen
    // (Pop behaelt seinen Comic-Rahmen, siehe drawButtonBackground).
    polLButton.getProperties().set ("thinOnFrame", true);
    polRButton.getProperties().set ("thinOnFrame", true);
    // Runde 108 (User): Soft-Chips in Gold (Gold = an) und das Zeichen fuer
    // Phasendrehung - ohne Rahmen saehe ein einzelnes "L" sonst wie eine
    // Beschriftung aus statt wie ein Knopf.
    for (auto* b : { &polLButton, &polRButton })
    {
        b->getProperties().set ("softChip", true);
        b->getProperties().set ("altAccent", true);
    }
    // Runde 110: das Ø ist jetzt das Icon, der Name darunter nur noch L/R.
    polLButton.setButtonText ("L");
    polRButton.setButtonText ("R");
    polLButton.getProperties().set ("phaseIcon", true);
    polRButton.getProperties().set ("phaseIcon", true);
    polLinkButton.getProperties().set ("noLinkLines", true);
    content.addAndMakeVisible (polLButton);
    content.addAndMakeVisible (polRButton);
    // Runde 148 (User: "der erste Crash ever", Klick auf Polarity L): der
    // JUCE-ButtonAttachment setzt den Knopf bei jeder Parameteraenderung mit
    // sendNotificationSync - das loest onClick erneut aus. Seit Runde 142
    // schaltet onClick den Parameter selbst um: Klick -> Parameter -> Knopf ->
    // onClick -> Parameter -> ... endlos, bis der Stack ueberlaeuft. Auch
    // Automation und Preset-Laden haetten den Wert sofort wieder umgedreht.
    // Jetzt eine schlanke ParameterAttachment: sie zeigt den Parameter nur an
    // (dontSendNotification), geschaltet wird ausschliesslich in onClick.
    auto polAttach = [this] (const juce::String& id, juce::TextButton& b)
    {
        auto att = std::make_unique<juce::ParameterAttachment> (*processor.apvts.getParameter (id),
            [&b] (float v) { b.setToggleState (v >= 0.5f, juce::dontSendNotification); }, nullptr);
        att->sendInitialUpdate();
        return att;
    };
    polLAttachment = polAttach (LCRMSAudioProcessor::ID_POL_L, polLButton);
    polRAttachment = polAttach (LCRMSAudioProcessor::ID_POL_R, polRButton);
    // Runde 126 (User): Cmd-Klick auf L oder R schaltet exklusiv - nur diese
    // Seite an, die andere aus (wie Solo).
    // Runde 142 (User-Bug: "wechselt hin und her"): vorher schaltete der Klick
    // erst um und danach wurde korrigiert - sicht- und hoerbar ein kurzes Hin
    // und Her. Jetzt schaltet der Knopf nicht mehr selbst um; onClick setzt
    // den Endzustand direkt, die Attachments ziehen die Anzeige nach.
    polLButton.setClickingTogglesState (false);
    polRButton.setClickingTogglesState (false);
    auto clickPol = [this] (bool leftSide)
    {
        auto* pl = processor.apvts.getParameter (LCRMSAudioProcessor::ID_POL_L);
        auto* pr = processor.apvts.getParameter (LCRMSAudioProcessor::ID_POL_R);
        if (pl == nullptr || pr == nullptr || polLAttachment == nullptr || polRAttachment == nullptr) return;
        // Als abgeschlossene Geste, damit der Host den Klick sauber als
        // Automationspunkt / Undo-Schritt sieht.
        if (juce::ModifierKeys::currentModifiers.isCommandDown())
        {
            polLAttachment->setValueAsCompleteGesture (leftSide ? 1.0f : 0.0f);
            polRAttachment->setValueAsCompleteGesture (leftSide ? 0.0f : 1.0f);
            return;
        }
        auto* p = leftSide ? pl : pr;
        auto& a = leftSide ? *polLAttachment : *polRAttachment;
        a.setValueAsCompleteGesture (p->getValue() > 0.5f ? 0.0f : 1.0f);
    };
    polLButton.onClick = [clickPol] { clickPol (true); };
    polRButton.onClick = [clickPol] { clickPol (false); };

    // Link-Button: keine eigene Parameter-Bindung (reine Aktion) - schaltet
    // L UND R gemeinsam um. Logik (User-Feedback): ist aktuell KEINER von
    // beiden an, schaltet Klick beide AN; ist mindestens einer an, schaltet
    // Klick beide AUS.
    polLinkButton.setClickingTogglesState (false);
    polLinkButton.getProperties().set ("linkIcon", true);
    polLinkButton.setWantsKeyboardFocus (false);
    content.addAndMakeVisible (polLinkButton);
    polLinkButton.onClick = [this]
    {
        auto* pl = processor.apvts.getParameter (LCRMSAudioProcessor::ID_POL_L);
        auto* pr = processor.apvts.getParameter (LCRMSAudioProcessor::ID_POL_R);
        if (pl == nullptr || pr == nullptr) return;
        // Neu (User): ist nur EINER an, schaltet Link beide AN. Erst wenn
        // beide an sind, schaltet der naechste Klick beide aus.
        const bool bothOn = pl->getValue() > 0.5f && pr->getValue() > 0.5f;
        const float target = bothOn ? 0.0f : 1.0f;
        pl->setValueNotifyingHost (target);
        pr->setValueNotifyingHost (target);
    };

    // 4 kleine Radio-Buttons waehlen, an welcher Stelle im Signalfluss die
    // Polarity greift (1=nach LCR, 2=nach Haas, 3=nach Mid/Side [Default],
    // 4=nach Auto-Pan/Flow). Kein Choice-ButtonAttachment in JUCE verfuegbar,
    // daher manuell verdrahtet: Klick schreibt den Parameter, timerCallback
    // haelt die Buttons mit dem aktuellen Parameterwert synchron (Presets/Automation).
    // Von vier Positionen bleiben zwei (User-Befund und Messung):
    //  - VOR Galaxy ist kaputt. Ein einseitiger Flip dreht dort die
    //    Korrelation auf -1; die Extraktion sucht Phasengleichheit und
    //    klemmt negative Werte auf 0, also wandert KEIN Bin mehr in die
    //    Mitte - Galaxy ist damit faktisch aus.
    //  - HINTER Vision ist identisch mit "hinter Dimension", sobald Vision
    //    keine eigene Stufe mehr ist: ein Flip ist pro Kanal linear und
    //    vertauscht mit Delay, dazwischen liegt dann nichts mehr.
    // Uebrig bleiben EARLY (hinter Galaxy) und LATE (hinter Dimension).
    juce::TextButton* polPosButtons[4] = { &polPos1Button, &polPos2Button, &polPos3Button, &polPos4Button };
    for (int idx = 0; idx < 4; ++idx)
    {
        auto* btn = polPosButtons[idx];
        btn->setClickingTogglesState (true);
        btn->getProperties().set ("thinOnFrame", true);
        btn->setRadioGroupId (4242, juce::dontSendNotification);
        content.addAndMakeVisible (*btn);
        btn->onClick = [this, idx]
        {
            if (auto* param = processor.apvts.getParameter (LCRMSAudioProcessor::ID_POL_POS))
                param->setValueNotifyingHost ((float) idx / 3.0f);
        };
    }
    // ERST JETZT verstecken. Die Schleife darueber ruft addAndMakeVisible()
    // auf alle vier - ein setVisible(false) DAVOR wird davon sofort wieder
    // aufgehoben (genau dieser Fehler: im Build standen weiterhin "1" und
    // "4" neben den beiden neuen Knoepfen).
    // Runde 39 (User): EARLY/LATE ist EIN Knopf - Klick schaltet um, die
    // Beschriftung zeigt die aktuelle Position. polPos2Button traegt ihn,
    // die anderen drei sind aus.
    polPos2Button.setButtonText (technicalLabels ? "PRE" : "EARLY");
    // Runde 62 (User): im aktiven Zustand zurueckhaltender als L und R - er
    // sagt nur, WO umgepolt wird, nicht DASS umgepolt wird.
    polPos2Button.getProperties().set ("softOn", true);
    // Runde 108 (User): PRE/POST ist ein Icon-Feld - die Kette als Linie, die
    // Breiten-Stufe als Kaestchen, der Punkt sitzt davor (PRE) oder dahinter
    // (POST). Icon links vom Namen, weil die Hoehe fuer Icon ueber Name
    // hier nicht reicht.
    polPos2Button.getProperties().set ("modePill", true);
    polPos2Button.getProperties().set ("pillColour", (int) themePalette().frameRaye.getARGB());
    polPos2Button.getProperties().set ("ppDiagram",
        (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_POL_POS)->load()) == 2 ? 1 : 0);
    polPosDots.count = 2;
    polPosDots.setTooltip ("Pre / Post: click a dot to pick it directly");
    polPosDots.onPick = [this] (int i)
    {
        if (auto* param = processor.apvts.getParameter (LCRMSAudioProcessor::ID_POL_POS))
            param->setValueNotifyingHost ((float) (i == 1 ? 2 : 1) / 3.0f);
    };
    content.addAndMakeVisible (polPosDots);
    polPos2Button.setClickingTogglesState (false);
    polPos2Button.setRadioGroupId (0, juce::dontSendNotification);
    polPos2Button.onClick = [this]
    {
        if (auto* param = processor.apvts.getParameter (LCRMSAudioProcessor::ID_POL_POS))
        {
            const int cur = juce::jlimit (0, 3, (int) std::round (param->convertFrom0to1 (param->getValue())));
            const int next = (cur == 2) ? 1 : 2;   // EARLY (1) <-> LATE (2)
            param->setValueNotifyingHost ((float) next / 3.0f);
        }
    };
    for (auto* dead : { &polPos1Button, &polPos3Button, &polPos4Button })
    {
        dead->setVisible (false);
        dead->setEnabled (false);
    }

    // --- Mid/Side (Width/Boost) ----------------------------------------------
    setupPowerButton (widthBoostPowerButton, LCRMSAudioProcessor::ID_WIDTHBOOST_ON, widthBoostOnAttachment, LCRMSAudioProcessor::SOLO_DIMENSION);
    // "Dimension" statt "Size" - vermeidet die Wiederholung von "Width" und
    // gibt "Size" als Reglername frei (Regler heisst jetzt "Size" statt
    // "Width", da "Width" jetzt vom globalen Width-Regler in der neuen
    // Position-Sektion belegt wird).
    styleTitle (widthBoostTitleLabel, "DIMENSION", juce::Colour (0xff5be3c7));
    setupClickableTitle (widthBoostTitleLabel, LCRMSAudioProcessor::ID_WIDTHBOOST_ON, LCRMSAudioProcessor::SOLO_DIMENSION);
    content.addAndMakeVisible (widthBoostTitleLabel);
    setupSoloButton (widthBoostSoloButton, LCRMSAudioProcessor::SOLO_DIMENSION);
    setupLockButton (widthBoostLockButton, LCRMSAudioProcessor::SOLO_DIMENSION);
    setupModButton (dimensionModButton, LCRMSAudioProcessor::ID_DIMENSION_MOD, dimensionModAttachment);
    styleRotary (dimensionModDepthSlider, false);
    content.addAndMakeVisible (dimensionModDepthSlider);
    dimensionDepthAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_DIMENSION_DEPTH, dimensionModDepthSlider);
    wireModAutoEnable (dimensionModDepthSlider, dimensionModButton, LCRMSAudioProcessor::ID_DIMENSION_MOD);
    dimensionModDepthSlider.setDoubleClickReturnValue (true, 50.0, juce::ModifierKeys::commandModifier);

    styleRotary (sideWidthSlider, false);
    sideWidthSlider.getProperties().set ("centerOut", true);
    styleRotary (sideBoostSlider, false);
    content.addAndMakeVisible (sideWidthSlider);
    content.addAndMakeVisible (sideBoostSlider);
    // Umbenannt von "Expand" zu "Size" (User-Feedback: Regler startet bei
    // 12 Uhr/Default und kann auch verkleinern, "Expand" klingt nach
    // reinem Vergroessern; "Width" ist unten bei Position schon vergeben).
    // Nur der GUI-Text aendert sich, der Regler selbst (Parameter-ID,
    // Verhalten, automatisierbarer Host-Name "Width") bleibt unveraendert.
    styleLabel (sideWidthLabel, "Size");
    styleLabel (sideBoostLabel, "Boost");
    content.addAndMakeVisible (sideWidthLabel);
    content.addAndMakeVisible (sideBoostLabel);
    sideWidthAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_SIDE_WIDTH, sideWidthSlider);
    sideBoostAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_SIDE_BOOST, sideBoostSlider);
    sideWidthSlider.setDoubleClickReturnValue (true, 100.0, juce::ModifierKeys::commandModifier);
    sideBoostSlider.setDoubleClickReturnValue (true, 0.0, juce::ModifierKeys::commandModifier);

    // --- Auto-Pan ("Flow") -----------------------------------------------------
    setupPowerButton (flowPowerButton, LCRMSAudioProcessor::ID_FLOW_ON, flowOnAttachment, LCRMSAudioProcessor::SOLO_HYPERDRIVE);
    styleTitle (flowTitleLabel, "HYPERDRIVE", juce::Colour (0xffb968ff));
    setupClickableTitle (flowTitleLabel, LCRMSAudioProcessor::ID_FLOW_ON, LCRMSAudioProcessor::SOLO_HYPERDRIVE);
    setupSoloButton (flowSoloButton, LCRMSAudioProcessor::SOLO_HYPERDRIVE);
    setupLockButton (flowLockButton, LCRMSAudioProcessor::SOLO_HYPERDRIVE);
    setupModButton (hyperdriveModButton, LCRMSAudioProcessor::ID_HYPERDRIVE_MOD, hyperdriveModAttachment);
    styleRotary (hyperdriveModDepthSlider, false);
    content.addAndMakeVisible (hyperdriveModDepthSlider);
    hyperdriveDepthAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_HYPERDRIVE_DEPTH, hyperdriveModDepthSlider);
    wireModAutoEnable (hyperdriveModDepthSlider, hyperdriveModButton, LCRMSAudioProcessor::ID_HYPERDRIVE_MOD);
    hyperdriveModDepthSlider.setDoubleClickReturnValue (true, 50.0, juce::ModifierKeys::commandModifier);
    content.addAndMakeVisible (flowTitleLabel);

    styleRotary (movementSlider, false);
    // Eigener Ring: waechst symmetrisch von 12 Uhr aus nach links/rechts
    // (zeigt die Breite des Auto-Pan-Schwenks), plus leuchtender Punkt fuer
    // die aktuelle Live-Position (siehe timerCallback).
    movementSlider.getProperties().set ("movementRing", true);
    content.addAndMakeVisible (movementSlider);
    // Header ist jetzt "HYPERDRIVE", der Regler kann daher wieder "Flow"
    // heissen (keine Ueberschneidung mehr mit dem Sektionsnamen).
    styleLabel (movementLabel, "Flow");
    content.addAndMakeVisible (movementLabel);
    movementAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_MOVEMENT, movementSlider);
    movementSlider.setDoubleClickReturnValue (true, 0.0, juce::ModifierKeys::commandModifier);

    // Sinus -> geglaetteter Puls (hart links/rechts, leicht gesmoothed).
    pulseButton.setClickingTogglesState (true);
    // Icon statt Wort (User: "wie bei einem Synth"). Das ist nicht nur
    // Kosmetik - erst dadurch wird HYPERDRIVE schmal genug, dass RAYE neben
    // ihm in dieselbe Reihe passt.
    pulseButton.getProperties().set ("pulseIcon", true);
    pulseButton.getProperties().set ("glowIcon", true);      // Runde 108: ohne Kaestchen
    content.addAndMakeVisible (pulseButton);
    pulseAttachment = std::make_unique<ButtonAttachment> (processor.apvts, LCRMSAudioProcessor::ID_PULSE, pulseButton);

    styleRotary (speedRateSlider, false);
    content.addAndMakeVisible (speedRateSlider);
    styleLabel (speedLabel, "Speed");
    content.addAndMakeVisible (speedLabel);
    speedRateAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_SPEED_RATE, speedRateSlider);
    speedRateSlider.setDoubleClickReturnValue (true, 0.25, juce::ModifierKeys::commandModifier);

    syncButton.setClickingTogglesState (true);
    syncButton.getProperties().set ("syncIcon", true);
    syncButton.getProperties().set ("glowIcon", true);       // Runde 108: ohne Kaestchen
    content.addAndMakeVisible (syncButton);
    syncAttachment = std::make_unique<ButtonAttachment> (processor.apvts, LCRMSAudioProcessor::ID_SPEED_SYNC, syncButton);

    // Runde 68 (User): Versalien wie bei allen anderen Auswahl-Knoepfen -
    // "4 Bars" war die einzige Beschriftung in gemischter Schreibung.
    speedBox.addItemList ({ "1/16", "1/8", "1/4", "1/2", "1 BAR", "2 BARS", "4 BARS", "8 BARS" }, 1);
    speedBox.getProperties().set ("noArrow", true);
    content.addAndMakeVisible (speedBox);
    speedAttachment = std::make_unique<ComboAttachment> (processor.apvts, LCRMSAudioProcessor::ID_SPEED, speedBox);

    // --- Position (neue Sektion, ganz am Ende der Kette) ---------------------
    setupPowerButton (posPowerButton, LCRMSAudioProcessor::ID_POS_ON, posOnAttachment, LCRMSAudioProcessor::SOLO_POSITION);
    setupSoloButton (posSoloButton, LCRMSAudioProcessor::SOLO_POSITION);
    setupLockButton (posLockButton, LCRMSAudioProcessor::SOLO_POSITION);
    // "VISION" statt "POSITION" (User: "Position wirkt etwas technisch im
    // Vergleich zu allen anderen Sections"). Stimmt - Galaxy, Timewarp,
    // Hyperdrive sind Bilder, Position ist ein Fachwort. Tilt/Elevate/
    // Distance sind Blickwinkel-Begriffe, "Vision" trifft das.
    styleTitle (posTitleLabel, "VISION", juce::Colour (0xff5be3c7));
    setupClickableTitle (posTitleLabel, LCRMSAudioProcessor::ID_POS_ON, LCRMSAudioProcessor::SOLO_POSITION);
    content.addAndMakeVisible (posTitleLabel);

    // Position-Mod: EIN Tiefe-Regler wirkt auf alle 4 Regler dieser Sektion
    // (Offset/Width/Distance/Elevate), gleiches Prinzip wie ueberall sonst
    // (User-Feedback: "Einfluss auf alle Regler").
    setupModButton (positionModButton, LCRMSAudioProcessor::ID_POSITION_MOD, positionModAttachment);
    styleRotary (positionModDepthSlider, false);
    content.addAndMakeVisible (positionModDepthSlider);
    positionDepthAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_POSITION_DEPTH, positionModDepthSlider);
    wireModAutoEnable (positionModDepthSlider, positionModButton, LCRMSAudioProcessor::ID_POSITION_MOD);
    positionModDepthSlider.setDoubleClickReturnValue (true, 50.0, juce::ModifierKeys::commandModifier);

    // --- RAY (Stereo-Phaser, unten rechts neben Position) --------------------
    // Die knappste Sektion im Plugin, absichtlich: Staerke-Icon, Speed, Pair.
    // Solo/Lock/klickbarer Titel wie ueberall, damit sie sich wie eine
    // vollwertige Sektion verhaelt (Mutate, Solo, Lock, Rahmen).
    setupPowerButton (rayPowerButton, LCRMSAudioProcessor::ID_RAY_ON, rayOnAttachment, LCRMSAudioProcessor::SOLO_RAY);
    setupSoloButton (raySoloButton, LCRMSAudioProcessor::SOLO_RAY);
    setupLockButton (rayLockButton, LCRMSAudioProcessor::SOLO_RAY);
    // "RAYE" (User: "RAY sieht zu klein aus ... Vielleicht RAYE? Ja lass RAYE
    // machen!") - Parameter-IDs bleiben "ray*", nur der sichtbare Name.
    styleTitle (rayTitleLabel, "RAYE", juce::Colour (0xffffc247));
    setupClickableTitle (rayTitleLabel, LCRMSAudioProcessor::ID_RAY_ON, LCRMSAudioProcessor::SOLO_RAY);
    content.addAndMakeVisible (rayTitleLabel);

    // 3-Klick-Icon fuer die Staerke (User: "ein Icon, das man dreimal klicken
    // kann"). Kein ButtonAttachment - der Klick schaltet den Choice-Parameter
    // selbst weiter (1 -> 2 -> 3 -> 1); die Anzeige wird in timerCallback()
    // aus dem Parameter nachgezogen, damit Automation/Presets stimmen.
    // Ist die Sektion aus, schaltet der erste Klick sie ein statt die Stufe
    // weiterzudrehen - ein Klick auf ein ausgegrautes Icon soll etwas
    // Hoerbares tun.
    rayStrengthButton.setClickingTogglesState (false);
    rayStrengthButton.setWantsKeyboardFocus (false);
    rayStrengthButton.getProperties().set ("rayStrengthIcon", true);
    rayStrengthButton.getProperties().set ("rayLevel", 0);
    content.addAndMakeVisible (rayStrengthButton);
    rayStrengthButton.onClick = [this]
    {
        auto* onP = processor.apvts.getParameter (LCRMSAudioProcessor::ID_RAY_ON);
        auto* stP = processor.apvts.getParameter (LCRMSAudioProcessor::ID_RAY_STRENGTH);
        if (onP == nullptr || stP == nullptr)
            return;

        // Vier Klicks im Kreis (User: "ray 4 mal klicken -> einmal off?"):
        // aus -> leicht -> mittel -> stark -> aus. Der Power-Schalter der
        // Sektion bleibt davon unabhaengig nutzbar.
        // Vier Stufen im Kreis: Off -> Light -> Medium -> Strong -> Off. Die
        // SEKTION bleibt dabei an (User-Korrektur) - "Off" ist eine Stufe des
        // Effekts, kein Ausschalten der Sektion. Ist die Sektion aus, schaltet
        // der Klick sie ein und startet bei Light.
        // Sektion aus: bleibt aus, nur die Stufe wird weitergeschaltet (User:
        // "bei den anderen Sektionen ist es ja genauso").
        const int current = juce::jlimit (0, 3, (int) std::round (stP->convertFrom0to1 (stP->getValue())));
        stP->setValueNotifyingHost (stP->convertTo0to1 ((float) ((current + 1) % 4)));
    };

   #if SPACEX_RAYE_UI == 1
    // SpaceXraye (Runde 41): Amount = wie stark (stufenlos, ersetzt die drei
    // Stufen), Charakter-Knopf = welche Art von Bewegung.
    rayStrengthButton.setVisible (false);
    rayStrengthButton.setEnabled (false);
    styleRotary (rayAmountSlider, false);
    content.addAndMakeVisible (rayAmountSlider);
    styleLabel (rayAmountLabel, "Amount");
    content.addAndMakeVisible (rayAmountLabel);
    rayAmountAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_RAY_AMOUNT, rayAmountSlider);
    rayAmountSlider.setDoubleClickReturnValue (true, 0.0, juce::ModifierKeys::commandModifier);
    rayAmountSlider.setTooltip ("Amount: how strong the movement is");
    rayCharButton.setClickingTogglesState (false);
    rayCharButton.setWantsKeyboardFocus (false);
    rayCharButton.getProperties().set ("thinOnFrame", true);
    rayCharButton.getProperties().set ("modePill", true);
    rayCharButton.setTooltip ("Character: Sweep, Shimmer, Spin, Swirl. Click for the next one, Cmd-click to go back");
    content.addAndMakeVisible (rayCharButton);
   #if SPACEX_PX_DIAG_ONLY
    rayCharButton.getProperties().set ("rayDiagram",
        juce::jlimit (0, 3, (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_RAY_CHAR)->load())));
   #endif
    rayModeDots.count = 4;
    rayModeDots.setTooltip ("Character: click a dot to pick it directly");
    rayModeDots.onPick = [this] (int i)
    {
        if (auto* prm = processor.apvts.getParameter (LCRMSAudioProcessor::ID_RAY_CHAR))
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) i));
    };
    content.addAndMakeVisible (rayModeDots);
    rayCharButton.onClick = [this]
    {
        if (auto* prm = processor.apvts.getParameter (LCRMSAudioProcessor::ID_RAY_CHAR))
        {
            const int cur = juce::jlimit (0, 3, (int) std::round (prm->convertFrom0to1 (prm->getValue())));
            const bool back = juce::ModifierKeys::currentModifiers.isCommandDown();
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) ((cur + (back ? 3 : 1)) % 4)));
        }
    };
   #endif

    styleRotary (rayRateSlider, false);
    content.addAndMakeVisible (rayRateSlider);
    styleLabel (rayRateLabel, "Speed");
    content.addAndMakeVisible (rayRateLabel);
    rayRateAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_RAY_RATE, rayRateSlider);
    rayRateSlider.setDoubleClickReturnValue (true, 0.15, juce::ModifierKeys::commandModifier);

    // "Pair": koppelt den Phaser-LFO an den Hyperdrive-LFO (inkl. dessen
    // Sync). Gestaltet wie der Sync-Button in Hyperdrive - dieselbe Art von
    // Entscheidung ("wer bestimmt das Tempo?"), also dieselbe Form.
    rayPairButton.setClickingTogglesState (true);
    rayPairButton.getProperties().set ("softChip", true);    // Runde 108: blau = gekoppelt
    rayPairButton.getProperties().set ("hdrIcon", 2);        // Runde 110: Kettenglieder
    rayPairButton.getProperties().set ("hdrTextSize", 11.0); // Runde 161
    rayPairButton.getProperties().set ("ctlStyle", 1);       // Runde 169 (User): LED-Punkt wie EQ -> LCR, keine Schalter
    rayPairButton.setWantsKeyboardFocus (false);
    content.addAndMakeVisible (rayPairButton);
    rayPairAttachment = std::make_unique<ButtonAttachment> (processor.apvts, LCRMSAudioProcessor::ID_RAY_PAIR, rayPairButton);

    // Runde 49: FAST statt Speed-Regler - ein Klick, 30 % schneller.
    rayFastButton.setClickingTogglesState (true);
    rayFastButton.setWantsKeyboardFocus (false);
    rayFastButton.getProperties().set ("thinOnFrame", true);
    rayFastButton.getProperties().set ("altAccent", true);   // Gold, PAIR bekommt den Hauptakzent
    rayFastButton.getProperties().set ("softChip", true);    // Runde 108: moderne Form
    rayFastButton.getProperties().set ("hdrIcon", 1);        // Runde 110: Doppelpfeil
    rayFastButton.getProperties().set ("hdrTextSize", 11.0); // Runde 161
    rayFastButton.getProperties().set ("ctlStyle", 1);       // Runde 169 (User): LED-Punkt wie EQ -> LCR, keine Schalter
    rayFastButton.setTooltip ("Fast: runs the current character 30% quicker");
    content.addAndMakeVisible (rayFastButton);
    rayFastAttachment = std::make_unique<ButtonAttachment> (processor.apvts, LCRMSAudioProcessor::ID_RAY_FAST, rayFastButton);

    // Mono-Check ist ein reines Monitoring-Utility (kein eigener Solo-
    // Kandidat) - sitzt als 5. "Regler"-Slot unten in der Reglerzeile, nicht
    // mehr oben im Header (User-Feedback: "Soll unten zu den Reglern rein").
    monoCheckButton.setClickingTogglesState (true);
    monoCheckButton.getProperties().set ("monoIcon", true);
    monoCheckButton.setWantsKeyboardFocus (false);
    content.addAndMakeVisible (monoCheckButton);
    monoCheckAttachment = std::make_unique<ButtonAttachment> (processor.apvts, LCRMSAudioProcessor::ID_MONO_CHECK, monoCheckButton);
    styleLabel (monoCheckLabel, "Mono");
    content.addAndMakeVisible (monoCheckLabel);
    styleLabel (monoDryLabel, "Dry");
    content.addAndMakeVisible (monoDryLabel);
    // Footer-Schriften weniger leuchtend (User: "wirkt viel heller als z.B.
    // Size oder Boost") - dieselbe gedaempfte Farbe wie die Regler-Labels.
    for (auto* l : { &monoCheckLabel, &monoDryLabel, &volLabel, &inputMeterLabel, &outputMeterLabel })
    {
        l->setColour (juce::Label::textColourId, juce::Colour (0xffb5b9c2));
        l->setMinimumHorizontalScale (1.0f);   // Footer: nie stauchen (breit genug ausgelegt)
    }
    // Klickbar, aber ohne Hand-Cursor (User). Der Editor bekommt die Klicks
    // ueber den content-weiten Maus-Listener (siehe View-Panel).

    // ===== PRISM =====
    // On/Off benutzt dasselbe Power-Ring-Icon wie die Sektions-Bypass-Schalter,
    // damit sofort klar ist, dass es ein Ein/Aus ist und kein Regler.
    // Rechtsklick sperrt/entsperrt (User-Idee statt des Menuepunkts).
    {
        juce::PropertiesFile p (LCRMSAudioProcessor::appPropertiesOptions());
        mixSlider.getProperties().set ("knobLocked", p.getBoolValue ("lockMix", false));
        volSlider.getProperties().set ("knobLocked", p.getBoolValue ("lockVol", false));
    }
    mixSlider.onRightClick = [this] { toggleKnobLock ("lockMix", mixSlider); };
    volSlider.onRightClick = [this] { toggleKnobLock ("lockVol", volSlider); };
    panSlider.getProperties().set ("knobLocked",
        juce::PropertiesFile (LCRMSAudioProcessor::appPropertiesOptions()).getBoolValue ("lockPan", false));

    prismOnButton.setClickingTogglesState (true);
    prismOnButton.setWantsKeyboardFocus (false);
    // Eigenes Prisma-Symbol statt des Bypass-Icons (User-Feedback: "Das Icon
    // fuer Prism ist nicht gut, sieht aus wie das Bypass Icon") - siehe
    // drawPrismIcon() im LookAndFeel.
    // Kein Prisma-Symbol und kein "PRISM"-Schriftzug mehr (User: "Der Name
    // PRISM klingt gut, aber macht da keinen Sinn und wirft Fragen auf.
    // Einfach einen On/Off Button ... verschmolzen mit dem Frequenzmeter").
    // Der Knopf ist jetzt das normale Power-Icon und sitzt IN der Leiste
    // (siehe PrismBandComponent::setLeftInset).
    // Gleiches Symbol wie die Focus-Knoepfe in den Sektionen (User) - nur
    // mit direkter Polaritaet: an = Focus wirkt.
    prismOnButton.getProperties().set ("filterIcon", true);
    prismOnButton.getProperties().set ("focusDirect", true);
    // Er sitzt bereits in der Kachel der Focus-Leiste - eine zweite Flaeche
    // darum waere ein Kasten im Kasten (User).
    prismOnButton.getProperties().set ("noPlate", true);
    prismOnButton.getProperties().set ("powerColour", (int) 0xff8a6ab8);   // gedaempftes Lila (User: "dezenter")
    content.addAndMakeVisible (prismBand);
    content.addAndMakeVisible (prismOnButton);   // NACH der Leiste -> liegt darueber
    prismOnAttachment = std::make_unique<ButtonAttachment> (processor.apvts, LCRMSAudioProcessor::ID_PRISM_ON, prismOnButton);
    prismLabel.setVisible (false);
    {
        juce::PropertiesFile pp (LCRMSAudioProcessor::appPropertiesOptions());
        prismBand.setClickJumps (pp.getBoolValue ("prismClickJumps", false));
        prismBand.setShowHz (pp.getBoolValue ("showFocusHz", false));
    }

    // A/B-Dry-Vergleich: eigener Bool-Parameter + Attachment wie ueblich,
    // zusaetzlich per onClick/timerCallback an Mono-Check gekoppelt (kann
    // nicht "alleine" an sein - User-Feedback).
    monoDryButton.setClickingTogglesState (true);
    monoDryButton.getProperties().set ("bypassIcon", true);
    monoDryButton.setWantsKeyboardFocus (false);
    content.addAndMakeVisible (monoDryButton);
    monoDryAttachment = std::make_unique<ButtonAttachment> (processor.apvts, LCRMSAudioProcessor::ID_MONO_DRY, monoDryButton);
    // Schaltet man Mono-Check aus, geht der Dry-Vergleich automatisch mit
    // aus - er soll nie unsichtbar "an" im Hintergrund bleiben.
    monoCheckButton.onClick = [this]
    {
        if (! monoCheckButton.getToggleState())
            if (auto* monoDryParam = processor.apvts.getParameter (LCRMSAudioProcessor::ID_MONO_DRY))
                monoDryParam->setValueNotifyingHost (0.0f);
    };

    // VOL: ganz simpler Trim-Regler, sitzt mittig ueber dem Mono-Icon (in dem
    // Freiraum, den Mono ohnehin frei laesst, weil es - anders als die
    // anderen Sektionen - keinen eigenen Header/Power-Button braucht).
    // Bewusst ohne Textbox/Wertanzeige (User-Feedback: "Ohne Werte"). Runder
    // Regler statt horizontalem Fader (User-Feedback: passt optisch besser
    // zu den restlichen Reglern).
    styleRotary (volSlider, false);
    volSlider.getProperties().set ("centerOut", true);
    content.addAndMakeVisible (volSlider);
    styleLabel (volLabel, "Vol");
    // Runde 53 (User): Mono/Dry/Mix/Vol MINIMAL weniger leuchtend.
    for (auto* l : { &monoCheckLabel, &monoDryLabel, &mixLabel, &volLabel,
                     &inputMeterLabel, &outputMeterLabel })
        l->setColour (juce::Label::textColourId, juce::Colour (0xffa9aeb8));
    content.addAndMakeVisible (volLabel);
    volAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_VOL_TRIM, volSlider);

    // --- Pan (Balance), allerletzte Stufe (Runde 74, User) ---
    styleRotary (panSlider, false);
    panSlider.getProperties().set ("centerOut", true);
    panSlider.getProperties().set ("footerKnob", true);
    content.addAndMakeVisible (panSlider);
    styleLabel (panLabel, "Pan");
    content.addAndMakeVisible (panLabel);
    panAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_OUT_PAN, panSlider);
    // Runde 84 (User): Cmd-Klick setzt zurueck - dieselbe Geste wie ueberall
    // sonst. Vorher war nur Doppelklick belegt, und der ist im Plugin
    // nirgends die Ruecksetz-Geste.
    panSlider.setDoubleClickReturnValue (true, 0.0, juce::ModifierKeys::commandModifier);
    panSlider.onRightClick = [this] { toggleKnobLock ("lockPan", panSlider); };
    volSlider.setDoubleClickReturnValue (true, 0.0, juce::ModifierKeys::commandModifier);
    // MIX (User-Frage "Gesamter Mix Regler?"): bearbeitet gegen Original,
    // latenzgleich, Polarity-Flip wird aufs Original uebernommen (siehe
    // ID_MIX im Processor).
    styleRotary (mixSlider, false);
    content.addAndMakeVisible (mixSlider);
    styleLabel (mixLabel, "Mix");
    mixLabel.setColour (juce::Label::textColourId, juce::Colour (0xffb5b9c2));
    mixLabel.setMinimumHorizontalScale (1.0f);
    content.addAndMakeVisible (mixLabel);
    mixAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_MIX, mixSlider);
    mixSlider.setDoubleClickReturnValue (true, 100.0, juce::ModifierKeys::commandModifier);
    content.addAndMakeVisible (volInputMeter);
    content.addAndMakeVisible (volOutputMeter);
    // IN/OUT benutzen jetzt EXAKT dieselbe Schrift wie alle anderen
    // Parameter-Beschriftungen (User-Feedback: "Die Schrift bei IN/OUT Meter
    // passt nicht zum Rest. Soll gleich sein.") - die vorherige Sonderloesung
    // mit 9pt war noetig, weil die Label-Hoehe an die Balkenhoehe gekoppelt
    // war; das ist im neuen Footer-Layout behoben (Zeilenhoehe = Label-Hoehe).
    // Nur die Ausrichtung weicht bewusst ab: linksbuendig, weil die
    // Beschriftung hier NEBEN dem Balken steht und nicht darunter.
    styleLabel (inputMeterLabel, "In");
    styleLabel (outputMeterLabel, "Out");
    inputMeterLabel.setJustificationType (juce::Justification::centredLeft);
    outputMeterLabel.setJustificationType (juce::Justification::centredLeft);
    content.addAndMakeVisible (inputMeterLabel);
    content.addAndMakeVisible (outputMeterLabel);

    styleRotary (offsetSlider, false);
    offsetSlider.getProperties().set ("centerOut", true);
    content.addAndMakeVisible (offsetSlider);
    styleLabel (offsetLabel, "Tilt"); // vorher "Offset" (User-Wunsch), Parameter-ID unveraendert
    content.addAndMakeVisible (offsetLabel);
    offsetAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_POS_OFFSET, offsetSlider);
    offsetSlider.setDoubleClickReturnValue (true, 0.0, juce::ModifierKeys::commandModifier);

    styleRotary (posWidthSlider, false);
    posWidthSlider.getProperties().set ("centerOut", true);
    content.addAndMakeVisible (posWidthSlider);
    styleLabel (posWidthLabel, "Width");
    content.addAndMakeVisible (posWidthLabel);
    posWidthAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_POS_WIDTH, posWidthSlider);
    posWidthSlider.setDoubleClickReturnValue (true, 100.0, juce::ModifierKeys::commandModifier);

    styleRotary (distanceSlider, false);
    content.addAndMakeVisible (distanceSlider);
    // DEPTH: ein bipolarer Regler statt Distance UND Elevate - beide bedienen
    // dieselbe Wahrnehmungsachse. Links fern und dunkel, rechts nah und offen,
    // Mitte unbearbeitet. Deshalb auch centerOut-Optik wie bei Drift und Size.
    styleLabel (distanceLabel, "Depth");
    content.addAndMakeVisible (distanceLabel);
    distanceSlider.getProperties().set ("centerOut", true);
    distanceAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_DEPTH, distanceSlider);
    distanceSlider.setDoubleClickReturnValue (true, 0.0, juce::ModifierKeys::commandModifier);
    // Runde 105 (User): DEPTH ist raus - der Regler bleibt nur als Objekt
    // bestehen (Attachment), wird aber nicht mehr gezeigt.
    distanceSlider.setVisible (false);
    distanceLabel.setVisible (false);

    // ===== SEITEN-EQ (Runde 105) =====
    // Icon-Feld wie Micropitch und Phaser: Kurve oben, Name darunter, Punkte
    // unter dem Feld. x2 sitzt im Kopf, wie FAST beim Phaser.
    msEqButton.setClickingTogglesState (false);
    msEqButton.setWantsKeyboardFocus (false);
    msEqButton.getProperties().set ("thinOnFrame", true);
    msEqButton.getProperties().set ("modePill", true);
    msEqButton.getProperties().set ("eqDiagram",
        juce::jlimit (0, LCRMSAudioProcessor::kMsEqModes - 1,
                      (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_MS_EQ)->load())));
    msEqButton.setTooltip ("Sides EQ: Flat, Tight, Clear, Focus. Click for the next one, Cmd-click to go back");
    content.addAndMakeVisible (msEqButton);
    msEqButton.onClick = [this]
    {
        if (auto* prm = processor.apvts.getParameter (LCRMSAudioProcessor::ID_MS_EQ))
        {
            constexpr int n = LCRMSAudioProcessor::kMsEqModes;
            // Runde 133: in der Anzeige-Reihenfolge FLAT, TIGHT, CLEAR, FOCUS.
            const int cur  = juce::jlimit (0, n - 1, (int) std::round (prm->convertFrom0to1 (prm->getValue())));
            const bool back = juce::ModifierKeys::currentModifiers.isCommandDown();
            const int d    = (sideeq::displayFromParam (cur) + (back ? n - 1 : 1)) % n;
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) sideeq::paramFromDisplay (d)));
        }
    };
    msEqDots.count = LCRMSAudioProcessor::kMsEqModes;
    msEqDots.setTooltip ("Sides EQ: click a dot to pick it directly");
    msEqDots.onPick = [this] (int i)
    {
        if (auto* prm = processor.apvts.getParameter (LCRMSAudioProcessor::ID_MS_EQ))
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) sideeq::paramFromDisplay (i)));
    };
    content.addAndMakeVisible (msEqDots);

    msEqX2Button.setButtonText ("x2");
    msEqX2Button.setClickingTogglesState (true);
    msEqX2Button.setWantsKeyboardFocus (false);
    msEqX2Button.getProperties().set ("thinOnFrame", true);
    msEqX2Button.getProperties().set ("altAccent", true);   // wie FAST
    msEqX2Button.getProperties().set ("headerPill", true);
    msEqX2Button.getProperties().set ("softChip", true);
    msEqX2Button.getProperties().set ("hdrIcon", 3);         // Runde 110: doppelte Kurve
    msEqX2Button.setTooltip ("x2: doubles the curve of the Sides EQ");
    // Runde 125: x2 ist ersetzt. Der Knopf bleibt unsichtbar als Platzhalter
    // im Kopf (layoutHeader reserviert damit die Breite fuer An/Aus + Fader).
    msEqX2Button.getProperties().set ("headerPillW", 108);
    content.addChildComponent (msEqX2Button);
    msEqX2Button.setVisible (false);

    msEqPowerButton.setClickingTogglesState (true);
    msEqPowerButton.getProperties().set ("powerIcon", true);
    msEqPowerButton.setWantsKeyboardFocus (false);
    msEqPowerButton.setTooltip ("EQ on/off - compare with and without, the setting stays");
    content.addAndMakeVisible (msEqPowerButton);
    msEqOnAttachment = std::make_unique<ButtonAttachment> (processor.apvts, LCRMSAudioProcessor::ID_MS_EQ_ON, msEqPowerButton);

    msEqAmtSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    msEqAmtSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    msEqAmtSlider.getProperties().set ("miniFader", true);
    msEqAmtSlider.setTextValueSuffix (" %");
    msEqAmtSlider.setWantsKeyboardFocus (false);
    msEqAmtSlider.setTooltip ("EQ amount: 50 % is the curve as tuned, 100 % the strongest. Double-click: back to 50 %");
    content.addAndMakeVisible (msEqAmtSlider);
    msEqAmtAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_MS_EQ_AMT, msEqAmtSlider);
    msEqAmtSlider.setDoubleClickReturnValue (true, 0.0, juce::ModifierKeys::commandModifier);   // Runde 160
    eqIconTicker.fn = [this] { tickEqIcon(); };
    eqIconTicker.startTimerHz (60);

    // Runde 115 (User): der EQ aus MID-SIDE kann in die LCR Matrix wandern.
    // Nur Text wie FAST/LINK; an = blau (gekoppelt), und das EQ-Feld unten
    // wird ebenfalls blau, damit man sieht, wo er gerade wirkt.
    lcrEqButton.setButtonText (juce::String::fromUTF8 ("EQ \xe2\x86\x92 LCR"));
    lcrEqButton.setClickingTogglesState (true);
    lcrEqButton.setWantsKeyboardFocus (false);
    lcrEqButton.getProperties().set ("headerPill", true);
    lcrEqButton.getProperties().set ("headerPillW", 86);
    lcrEqButton.getProperties().set ("softChip", true);
    lcrEqButton.getProperties().set ("hdrIcon", 2);           // nur der Name
    lcrEqButton.getProperties().set ("hdrTextSize", 11.0);    // Runde 161 (User): Schrift war zu klein
    lcrEqButton.getProperties().set ("ctlStyle", 1);          // Runde 166: LED-Punkt
    lcrEqButton.setTooltip ("EQ to LCR: the Mid-Side EQ works on Center and Sides of the LCR Matrix instead of Mid and Side");
    content.addAndMakeVisible (lcrEqButton);
    lcrEqAttachment = std::make_unique<ButtonAttachment> (processor.apvts, LCRMSAudioProcessor::ID_MS_EQ_LCR, lcrEqButton);

    styleRotary (elevateSlider, false);
    content.addAndMakeVisible (elevateSlider);
    styleLabel (elevateLabel, "Elevate");
    content.addAndMakeVisible (elevateLabel);
    elevateAttachment = std::make_unique<SliderAttachment> (processor.apvts, LCRMSAudioProcessor::ID_POS_ELEVATE, elevateSlider);
    elevateSlider.setDoubleClickReturnValue (true, 0.0, juce::ModifierKeys::commandModifier);

    content.addAndMakeVisible (goniometer);
    content.addAndMakeVisible (correlationMeter);

    // Preset-/Hamburger-Menue (User-Idee): Text statt eigenem Icon haelt es
    // simpel - drei kurze horizontale Striche als "Hamburger"-Symbol.
    presetMenuButton.setButtonText ({});
    presetMenuButton.setClickingTogglesState (false);
    presetMenuButton.setWantsKeyboardFocus (false);
    presetMenuButton.getProperties().set ("settingsIcon", true);
    // "bigGlobalIcon": deutlich groessere Schrift NUR fuer das Hamburger-
    // Symbol (User-Wunsch: "Hamburger Menu Icon deutlich groesser") - alle
    // anderen globalBtn-Texte bleiben bei globalRowFont().
    presetMenuButton.getProperties().set ("bigGlobalIcon", true);
    content.addAndMakeVisible (presetMenuButton);
    presetMenuButton.onClick = [this] { showPresetMenu(); };

    // ===== PRESET-SCHRITTPFEILE =====
    // User-Wunsch: "kleine Preset Arrows L R". Sie benutzen dasselbe
    // Pfeil-Icon wie Undo/Redo (Eigenschaft "arrowIcon"), aber in der
    // schlichten Chevron-Variante ("chevronArrow") - ein voll ausgebauter
    // Undo-Bogen waere hier die falsche Aussage: die Pfeile machen nichts
    // rueckgaengig, sie blaettern.
    for (auto* b : { &presetPrevButton, &presetNextButton })
    {
        b->setClickingTogglesState (false);
        b->setWantsKeyboardFocus (false);
        b->getProperties().set ("arrowIcon", true);
        b->getProperties().set ("chevronArrow", true);
        content.addAndMakeVisible (*b);
    }
    presetPrevButton.getProperties().set ("arrowForward", false);
    presetNextButton.getProperties().set ("arrowForward", true);
    presetPrevButton.setTooltip ("Previous preset");
    presetNextButton.setTooltip ("Next preset");
    presetPrevButton.onClick = [this] { stepPreset (-1); };
    presetNextButton.onClick = [this] { stepPreset (+1); };

    // ===== PRESET-NAMENSFELD =====
    // Die eigentliche Antwort auf "wo bin ich gerade?". Klick darauf oeffnet
    // dieselbe Liste wie LOAD - ein Namensfeld, das man nicht anklicken kann,
    // waere eine verschenkte Flaeche.
    presetNameButton.setClickingTogglesState (false);
    presetNameButton.setWantsKeyboardFocus (false);
    presetNameButton.getProperties().set ("presetNameField", true);
    content.addAndMakeVisible (presetNameButton);
    // Linksklick = Preset-Liste, Rechtsklick = Loeschen-Liste - exakt die
    // Belegung, die vorher auf dem LOAD-Button lag. Der LOAD-Button selbst
    // ist damit entfallen (User: "Wenn der Preset Name da steht kann Load weg
    // - ist ja redundant"). Richtig: ein Namensfeld, das die Liste oeffnet,
    // erledigt beides, und genau so macht es auch Pro-Q - dort gibt es
    // ebenfalls keinen Load-Knopf, sondern nur den Namen zwischen zwei
    // Pfeilen.
    presetNameButton.onClick = [this]
    {
        showLoadPresetPopup (juce::ModifierKeys::currentModifiers.isPopupMenu());
    };
    // Papierkorb: loescht das aktuell geladene Preset (mit Rueckfrage).
    // Bewusst NICHT die Liste oeffnen - dafuer gibt es den Rechtsklick auf
    // den Namen. Ein Papierkorb-Icon verspricht "das hier weg", nicht
    // "suche dir etwas zum Wegwerfen aus".
    presetDeleteButton.setClickingTogglesState (false);
    presetDeleteButton.setWantsKeyboardFocus (false);
    presetDeleteButton.getProperties().set ("trashIcon", true);
    presetDeleteButton.setTooltip ("Delete current preset");
    content.addAndMakeVisible (presetDeleteButton);
    presetDeleteButton.onClick = [this]
    {
        if (currentPresetName.isEmpty())
            return;
        const juce::String name = currentPresetName;
        juce::NativeMessageBox::showOkCancelBox (juce::MessageBoxIconType::WarningIcon,
            "Delete Preset", "Delete preset \"" + presetDisplayName (name) + "\"?",
            nullptr,
            juce::ModalCallbackFunction::create ([this, name] (int okResult)
            {
                if (okResult != 0)
                {
                    deletePreset (name);
                    refreshPresetNameDisplay();
                }
            }));
    };

    // ===== VIEW-PANEL + ZAHNRAD =====
    // Das Zahnrad sitzt IM Sternenfeld (oben rechts), halbtransparent, und
    // klappt das Panel direkt darunter auf. Die Einstellungen gehoeren zum
    // Feld, also sitzt ihr Schalter auch dort - nicht im globalen Menue.
    viewGearButton.setClickingTogglesState (true);
    viewGearButton.setWantsKeyboardFocus (false);
    viewGearButton.getProperties().set ("gearIcon", true);
    viewGearButton.setTooltip ("Starfield view settings");
    content.addAndMakeVisible (viewGearButton);

    // "?" ganz unten links: schaltet die Hinweiszeile darunter an und aus
    // (User). Ersetzt den Menue-Eintrag als taeglichen Weg dorthin - der
    // Eintrag bleibt trotzdem, damit beides denselben Schalter bedient.
    // Auto Gain ist per Klick auf seine Anzeige schaltbar (User) - die Zahl
    // steht ohnehin dort, ein zweites Bedienelement waere Verschwendung.
    // Runde 110 (User): AG sitzt jetzt im Footer zwischen Mix und Vol - dort,
    // wo es im Signalweg auch wirkt. Soft-Chip mit dem Wert, "AG" darunter.
    autoGainButton.getProperties().set ("plainValue", true);   // Runde 112: ohne Box
    autoGainButton.getProperties().set ("altAccent", true);
    autoGainButton.setClickingTogglesState (false);
    styleLabel (autoGainLabel, "AG");
    autoGainLabel.setColour (juce::Label::textColourId, juce::Colour (0xffa9aeb8));
    content.addAndMakeVisible (autoGainLabel);
    autoGainButton.setWantsKeyboardFocus (false);
    autoGainButton.setTooltip ("Auto Gain: matches the output level to the input so bypass is an honest comparison. Click to switch it off");
    content.addAndMakeVisible (autoGainButton);
    autoGainButton.onClick = [this]
    {
        if (auto* prm = processor.apvts.getParameter (LCRMSAudioProcessor::ID_AUTO_GAIN))
            prm->setValueNotifyingHost (prm->getValue() > 0.5f ? 0.0f : 1.0f);
        refreshSettingsPanel();
        content.repaint (hintBarArea.expanded (8));
    };

    helpButton.getProperties().set ("helpIcon", true);
    helpButton.setClickingTogglesState (true);
    helpButton.setWantsKeyboardFocus (false);
    helpButton.setTooltip ("Help: show a short explanation for whatever the mouse is over");
    {
        juce::PropertiesFile hp (LCRMSAudioProcessor::appPropertiesOptions());
        helpButton.setToggleState (hp.getBoolValue ("hoverHints", false), juce::dontSendNotification);
    }
    helpButton.onClick = [this]
    {
        juce::PropertiesFile wp (LCRMSAudioProcessor::appPropertiesOptions());
        wp.setValue ("hoverHints", helpButton.getToggleState());
        wp.saveIfNeeded();
        currentHint.clear();
        repaint();
    };
    content.addAndMakeVisible (helpButton);
    content.addChildComponent (viewPanel);   // erst sichtbar per Zahnrad
    content.addChildComponent (settingsBackdrop);
    settingsBackdrop.onClick = [this] { closeSettingsPanel(); closeBackPanel(); closeSavePanel(); };
    content.addChildComponent (settingsPanel);
    content.addChildComponent (backPanel);
    content.addChildComponent (savePanel);   // Runde 149
    savePanel.onCancel = [this] { closeSavePanel(); };
    savePanel.onSave = [this] (const juce::String& name, const juce::String& folder)
    {
        closeSavePanel();
        // "Ordner/Name" ins Namensfeld getippt hat weiterhin Vorrang.
        if (name.containsChar ('/') || isDefaultPresetName (name))
            saveUnderKey (name);
        else
            saveUnderKey (folder.isEmpty() ? name : folder + "/" + name);
    };
    savePanel.onOpenFolder = [this] (const juce::String& folder)
    {
        // Oeffnet den gewaehlten Ordner (falls es ihn schon gibt), sonst den
        // Preset-Ordner selbst. Das Panel bleibt offen.
        auto dir = presetFolder();
        if (folder.isNotEmpty() && dir.getChildFile (folder).isDirectory())
            dir = dir.getChildFile (folder);
        dir.startAsProcess();
    };
    backPanel.onClose  = [this] { closeBackPanel(); };
    backPanel.onManual = [this] { openManual(); };
    backPanel.onTour   = [this] { closeBackPanel(); startTour(); };
    backPanel.onActivate = [this] { promptActivate(); };
    backPanel.onBuy      = [this] { juce::URL (spacexContact::shopUrl).launchInDefaultBrowser(); };
    backPanel.onSupport  = [this] { juce::URL (spacexContact::shopUrl).launchInDefaultBrowser(); };
    backPanel.onDontShow = [this] (bool dontShow)
    {
        juce::PropertiesFile wp (LCRMSAudioProcessor::appPropertiesOptions());
        wp.setValue ("seenWelcome", dontShow);
        wp.saveIfNeeded();
    };
    content.addChildComponent (tourOverlay);
    tourOverlay.onFinish = [this]
    {
        tourOverlay.setVisible (false);
        content.repaint();
    };
    tourOverlay.onDontShow = [this] (bool dontShow)
    {
        juce::PropertiesFile wp (LCRMSAudioProcessor::appPropertiesOptions());
        wp.setValue ("seenWelcome", dontShow);
        wp.saveIfNeeded();
    };
    backPanel.mailBtn.onClick   = [this] { juce::URL (juce::String ("mailto:") + spacexContact::email).launchInDefaultBrowser(); };
    backPanel.webBtn.onClick    = [this] { juce::URL (spacexContact::websiteUrl).launchInDefaultBrowser(); };
    backPanel.instaBtn.onClick  = [this] { juce::URL (spacexContact::instagramUrl).launchInDefaultBrowser(); };
    backPanel.linksBtn.onClick  = [this] { juce::URL (spacexContact::linksUrl).launchInDefaultBrowser(); };
    settingsPanel.onAction = [this] (int id) { handleSettingsAction (id); };
    settingsPanel.onClose  = [this] { closeSettingsPanel(); };
    settingsPanel.onBrightness = [this] (float v, bool persist) { applyBrightness (v, persist); };
    content.addMouseListener (this, true);   // Klicks auf Titel-/Footer-Labels (mouseUp)
    // Aenderungen wirken sofort und leben im Plugin-Zustand (DAW-Session);
    // erst "Make Default" schreibt sie als Startwerte, "Reset" holt die
    // Startwerte zurueck.
    viewPanel.onChange      = [this] { applyViewSettings (false); };
    viewPanel.onReset       = [this] { applyVisualsVisibility(); };                    // gespeicherter Standard
    viewPanel.onSave        = [this] { applyViewSettings (true); closeViewPanel(); };   // als Standard speichern
    viewPanel.onSaveOnly    = [this] { applyViewSettings (false); closeViewPanel(); };  // nur uebernehmen und schliessen (User)
    viewPanel.onClose       = [this] { closeViewPanel(); };
    viewPanel.onCancel      = [this]
    {
        // Aenderungen seit dem Oeffnen verwerfen.
        if (viewPanelSnapshot.isValid())
        {
            juce::ValueTree wrap ("tmp");
            wrap.appendChild (viewPanelSnapshot.createCopy(), nullptr);
            applyViewSettingsFromTree (wrap);
        }
        closeViewPanel();
    };

    // Klick ins Sternenfeld: Goniometer-Farbe weiterschalten (Blau, Gruen,
    // Violett, Gold), der fuenfte Klick schaltet die Spur aus, der sechste
    // beginnt wieder bei Blau.
    goniometer.onFieldClick = [this] (int action) { cycleGonioColourFromField (action); };

    // ===== Kategorie-Chips (Mutate-Profile) links ueber dem Sternenfeld =====
    {
        // Runde 37 (User): vier einfache Kategorien, geordnet danach, wie viel
        // veraendert werden darf.
        static const char* const catNames[kNumCategories] = { "Lead Vocal", "Backings", "Adlibs", "Send FX" };
        // Die Hinweise sagen, WAS unter die Kategorie faellt - "Plucked" allein
        // beantwortet die Frage nicht (User).
        static const char* const catHints[kNumCategories] = {
            "Vocal: lead or mono vocals made wide - the centre stays intact",
            "Backing: backing stacks and busses - wide, but still tidy",
            "Adlib: adlibs - space, movement and clear sides",
            "FX: throws, wet tracks and FX returns - anything goes"
        };
        for (int i = 0; i < kNumCategories; ++i)
        {
            categoryBtn[i].setButtonText (catNames[i]);
            categoryBtn[i].getProperties().set ("chipBtn", true);
            categoryBtn[i].setWantsKeyboardFocus (false);
            categoryBtn[i].setTooltip (juce::String ("Smart profile ") + catHints[i] + ". Click again to switch off");
            content.addAndMakeVisible (categoryBtn[i]);
            categoryBtn[i].onClick = [this, i] { setMutateCategory (mutateCategory() == i + 1 ? 0 : i + 1); };
        }
        // Runde 55 (User): die vier Chips ueber dem Sternenfeld werden zu
        // EINER Pille mit Punkten - dieselbe Bildsprache wie die Modi in
        // Parallax und RAYE, und die Chip-Zeile gibt ihre Hoehe an das
        // Sternenfeld zurueck. Punkt 0 = aus.
        for (auto& b : categoryBtn) { b.setVisible (false); b.setBounds ({}); }

        categoryButton.setClickingTogglesState (false);
        categoryButton.setWantsKeyboardFocus (false);
        categoryButton.getProperties().set ("modePill", true);
        categoryButton.setTooltip ("Smart profile: click for the next one, Cmd-click to go back. The dice then stays inside what fits that source");
        content.addAndMakeVisible (categoryButton);
        categoryButton.onClick = [this]
        {
            const bool back = juce::ModifierKeys::currentModifiers.isCommandDown();
            const int n = kNumCategories + 1;
            setMutateCategory ((mutateCategory() + (back ? n - 1 : 1)) % n);
        };
        catDots.count = kNumCategories + 1;
        catDots.setTooltip ("Smart profile: click a dot to pick it directly");
        catDots.onPick = [this] (int i) { setMutateCategory (i); };
        content.addAndMakeVisible (catDots);

        smartInfoLabel.setJustificationType (juce::Justification::centredLeft);
        // Runde 66 (User): rechts stand noch reichlich Platz frei, selbst bei
        // der laengsten Zeile (Backings) - die Schrift darf also groesser.
        smartInfoLabel.setFont (juce::Font (juce::FontOptions (14.5f)));
        smartInfoLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8f96a4));
        smartInfoLabel.setMinimumHorizontalScale (1.0f);
        smartInfoLabel.setBorderSize (juce::BorderSize<int> (0));
        smartInfoLabel.setInterceptsMouseClicks (false, false);
        content.addAndMakeVisible (smartInfoLabel);

        smartInfoToggle.getProperties().set ("helpIcon", true);
        smartInfoToggle.getProperties().set ("helpLetter", "i");
        smartInfoToggle.setClickingTogglesState (true);
        smartInfoToggle.setWantsKeyboardFocus (false);
        smartInfoToggle.setTooltip ("Show what the selected Smart profile does");
        {
            juce::PropertiesFile hp (LCRMSAudioProcessor::appPropertiesOptions());
            smartInfoVisible = hp.getBoolValue ("smartInfoVisible", true);
        }
        smartInfoToggle.setToggleState (smartInfoVisible, juce::dontSendNotification);
        content.addAndMakeVisible (smartInfoToggle);
        smartInfoToggle.onClick = [this]
        {
            smartInfoVisible = smartInfoToggle.getToggleState();
            juce::PropertiesFile wp (LCRMSAudioProcessor::appPropertiesOptions());
            wp.setValue ("smartInfoVisible", smartInfoVisible);
            wp.saveIfNeeded();
            smartInfoLabel.setVisible (showMutateCategories && smartInfoVisible && mutateCategoryValue > 0);
        };

        setMutateCategory ((int) processor.apvts.state.getProperty ("mutateCategory", 0));
        showMutateCategories = juce::PropertiesFile (LCRMSAudioProcessor::appPropertiesOptions()).getBoolValue ("showMutateCategories", true);
        // Runde 100 (User, nach Feedback eines Engineer-Kollegen): die
        // technische Beschriftung ist der Standard, die Space-Namen sind die
        // Option ("SpaceX Labels" unter den Themes).
        technicalLabels = true;   // Runde 133: "SpaceX Labels" entfernt - immer technische Namen
        applyLabelStyle();

        // Runde 58 (User-Korrektur): beim allerersten Oeffnen startet direkt
        // die TOUR - nicht die Rueckseite. Die Checkbox sitzt in der Tour.
        {
            juce::PropertiesFile wp (LCRMSAudioProcessor::appPropertiesOptions());
            if (! wp.getBoolValue ("seenWelcome", false))
                juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<LCRMSAudioProcessorEditor> (this)]
                {
                    if (safe != nullptr)
                        safe->startTour (true);
                });
        }
        // Runde 116 (User-Bug: "Solo, Klick auf den Namen -> alle Sektionen
        // aus"): die Option ist nicht mehr im Menue, stand aber auf "an".
        // Jetzt fest aus - schaltet man die solierte Sektion aus, endet Solo.
        keepSoloWhenSectionOff = false;
    }
    viewGearButton.onClick = [this]
    {
        const bool open = viewGearButton.getToggleState();
        if (open)
        {
            viewPanelSnapshot = viewSettingsTree();   // fuer Cancel
            viewPanel.setAlpha (1.0f);
            viewPanel.toFront (false);
            juce::Desktop::getInstance().getAnimator().fadeIn (&viewPanel, 140);   // kurzer Fade-in (User)
        }
        else
            closeViewPanel();
        content.repaint();
    };

    // Beim Oeffnen steht das Plugin auf "Default" (User-Wunsch). Ob der
    // Host einen abweichenden Zustand mitgebracht hat, zeigt der Stern: die
    // Pruefsumme wird gegen das Default-Preset gerechnet, nicht gegen den
    // Live-Zustand.
    migrateLegacyPresets();
    currentPresetName = "Default";
    presetSignature = signatureOfTree (defaultPresetTree());
    presetDirty = std::abs (computePresetSignature() - presetSignature) > 1.0e-5f;
    refreshPresetNameDisplay();

    // Goniometer/Star-Visuals: keine eigenen Buttons mehr in der globalen
    // Zeile (User-Wunsch, 2. Anlauf) - Kontrolle laeuft jetzt komplett
    // ueber die beiden neuen Live-Eintraege im Hamburger-Menue (siehe
    // showPresetMenu()).
    // Hat der Host einen Zustand mit View-Einstellungen mitgebracht (DAW-
    // Session), gelten diese statt der Startwerte. Vorher sichern, weil
    // applyVisualsVisibility() selbst schon ein (eigenes) Kind anhaengt.
    const auto hostView = processor.apvts.state.getChildWithName ("ViewSettings").createCopy();
    applyVisualsVisibility();
    applyHoverHints();
    if (hostView.isValid())
    {
        juce::ValueTree wrap ("tmp");
        wrap.appendChild (hostView, nullptr);
        applyViewSettingsFromTree (wrap);
    }

    // Undo/Redo (User-Wunsch: "Je ein Pfeil Icon, Position rechts von
    // Reset") - Icon-Buttons, keine eigene Umrandung (drawArrowIcon()
    // zeichnet direkt, siehe CustomLookAndFeel).
    for (auto* btn : { &undoButton, &redoButton })
    {
        btn->setClickingTogglesState (false);
        btn->setWantsKeyboardFocus (false);
        content.addAndMakeVisible (*btn);
    }
    undoButton.getProperties().set ("arrowIcon", true);
    undoButton.getProperties().set ("arrowDirection", "left");
    redoButton.getProperties().set ("arrowIcon", true);
    redoButton.getProperties().set ("arrowDirection", "right");
    undoButton.onClick = [this] { performUndo(); };
    redoButton.onClick = [this] { performRedo(); };

    // Erster Snapshot = Ausgangszustand beim Fenster-Oeffnen, damit man
    // immer dahin zurueck-undoen kann. Danach reagiert stateChangeListener
    // auf jede weitere Aenderung am APVTS-State-Baum (Parameter UND
    // Section-Lock-Properties, beide liegen auf demselben Baum).
    pushUndoSnapshotNow();
    stateChangeListener = std::make_unique<StateChangeListener> (*this);
    processor.apvts.state.addListener (stateChangeListener.get());
    updateUndoRedoButtonStates();

    // "Show Modulation" (Menu-Item, User-Wunsch) - Startwert aus dem
    // gespeicherten App-weiten Standard.
    {
        juce::PropertiesFile modVisProps (LCRMSAudioProcessor::appPropertiesOptions());
        modulationVisualsEnabled = ! modVisProps.getBoolValue ("modulationVisualsDisabled", false);
        advancedModVisible = modVisProps.getBoolValue ("showAdvancedModulation", false);
    }
    applyAdvancedModVisibility();

    // Initialen Zustand der Polarity-Positions-Buttons setzen (z.B. beim
    // Laden eines Presets, das nicht den Default-Wert hat).
    {
        const int initialPos = juce::jlimit (0, 3, (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_POL_POS)->load()));
        polPosButtons[initialPos]->setToggleState (true, juce::dontSendNotification);
    }

    setResizable (true, true);
    // Feste Seitenverhaeltnis-Sperre: die gesamte GUI wird beim Resizen
    // gleichmaessig skaliert (siehe layoutContent/paintContent + Transform
    // in resized()), statt nur mehr Leerraum zu zeigen.
    setResizeLimits (juce::roundToInt (kDesignW * 0.7f), juce::roundToInt (kDesignH * 0.7f),
                      juce::roundToInt (kDesignW * 1.6f), juce::roundToInt (kDesignH * 1.6f));
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio ((double) kDesignW / (double) kDesignH);

    // Zuletzt per "SAVE"-Button gemerkte Fenstergroesse wiederherstellen,
    // falls vorhanden und innerhalb der Resize-Grenzen - sonst normale
    // Design-Groesse (User-Feedback: "Save size state als Button oben
    // global").
    {
        juce::PropertiesFile savedProps (LCRMSAudioProcessor::appPropertiesOptions());
        const int savedW = savedProps.getIntValue ("windowWidth", 0);
        const int savedH = savedProps.getIntValue ("windowHeight", 0);
        const int minW = juce::roundToInt (kDesignW * 0.7f), maxW = juce::roundToInt (kDesignW * 1.6f);
        // Nur die BREITE wird uebernommen, die Hoehe immer aus dem aktuellen
        // Seitenverhaeltnis gerechnet. Vorher wurde eine alte Groesse 1:1
        // gesetzt - stammte sie aus einer Version mit anderem Verhaeltnis,
        // oeffnete das Fenster verzerrt ("rechts abgeschnitten") und sprang
        // erst beim Anfassen der Ecke in die richtige Groesse.
        juce::ignoreUnused (savedH);
        if (SPACEX_ROW2_VARIANT == 0 && savedW >= minW && savedW <= maxW)
            setSize (savedW, juce::roundToInt ((double) savedW * kDesignH / kDesignW));
        else
        {
            // Runde 174 (User: "Fenster beim ersten Laden viel zu klein"):
            // ohne gemerkte Groesse 25 % ueber der Design-Groesse starten,
            // aber nie groesser als der Bildschirm hergibt (90 % der Breite,
            // 85 % der Hoehe des Hauptbildschirms, ohne Dock/Menueleiste).
            int w = juce::roundToInt (kDesignW * 1.25f);
            if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
            {
                const auto ua = d->userArea;
                const int fitW = juce::jmin (juce::roundToInt (ua.getWidth() * 0.90f),
                                             juce::roundToInt (ua.getHeight() * 0.85f * kDesignW / (float) kDesignH));
                w = juce::jmin (w, fitW);
            }
            w = juce::jlimit (minW, maxW, w);
            setSize (w, juce::roundToInt ((double) w * kDesignH / kDesignW));
        }
    }

    startTimerHz (20);
    // Runde 97 (User: "beim Oeffnen sind Sektionen erst an und gehen dann
    // aus"): der Abgleich mit den Parametern lief bisher erst beim ERSTEN
    // Timer-Tick, also bis zu 50 ms nach dem ersten Anstrich. Bis dahin zeigte
    // die Oberflaeche ihre Konstruktor-Vorgaben - daher das kurze Flackern.
    // Einmal von Hand aufrufen, bevor gezeichnet wird.
    timerCallback();
}

LCRMSAudioProcessorEditor::~LCRMSAudioProcessorEditor()
{
    // Fenstergroesse automatisch merken (User: "soll es sich von alleine
    // merken") - beim Schliessen, nicht bei jedem Pixel waehrend des Ziehens.
    if (SPACEX_ROW2_VARIANT == 0)
    {
        juce::PropertiesFile p (LCRMSAudioProcessor::appPropertiesOptions());
        p.setValue ("windowWidth", getWidth());
        p.setValue ("windowHeight", getHeight());
        p.saveIfNeeded();
    }
    if (stateChangeListener != nullptr)
        processor.apvts.state.removeListener (stateChangeListener.get());
    setLookAndFeel (nullptr);
    stopTimer();
}

// Undo/Redo-Snapshot-Stack (User-Wunsch: "Je ein Pfeil Icon, Position
// rechts von Reset"). Ein Snapshot ist der komplette serialisierte APVTS-
// State-Baum (deckt automatisch auch die Section-Lock-Properties mit ab,
// da beide auf demselben Baum liegen - siehe isSectionLocked()/
// setSectionLocked()). Rueckwaertiges Abschneiden bei einem neuen Schritt
// nach einem Undo (klassisches Undo/Redo-Verhalten: ein neuer Schritt
// verwirft die verworfene "Zukunft").
void LCRMSAudioProcessorEditor::pushUndoSnapshotNow()
{
    juce::MemoryBlock block;
    {
        juce::MemoryOutputStream stream (block, false);
        processor.apvts.state.writeToStream (stream);
    }
    // Kein Duplikat anhaengen, falls sich seit dem letzten Snapshot gar
    // nichts geaendert hat (z.B. Klick ohne tatsaechliche Wertaenderung).
    if (undoIndex >= 0 && undoIndex < undoHistory.size() && undoHistory[undoIndex] == block)
        return;

    if (undoIndex < undoHistory.size() - 1)
        undoHistory.removeRange (undoIndex + 1, undoHistory.size() - undoIndex - 1);

    undoHistory.add (std::move (block));
    ++undoIndex;

    // Historie deckeln (User-Absicherung gegen unbegrenztes Speicherwachstum
    // bei langen Sessions) - aeltestes Element faellt hinten raus.
    constexpr int kMaxUndoSteps = 60;
    if (undoHistory.size() > kMaxUndoSteps)
    {
        undoHistory.remove (0);
        --undoIndex;
    }
    updateUndoRedoButtonStates();
}

// Wird bei JEDER Aenderung am APVTS-State-Baum aufgerufen (siehe
// StateChangeListener) - ein Regler-Drag loest dutzende Einzelereignisse
// aus, soll aber nur EINEN Undo-Schritt erzeugen. Daher hier nur ein
// Zaehler-Reset; der eigentliche Snapshot passiert erst in timerCallback(),
// nachdem eine kurze Weile (siehe kUndoDebounceFrames) nichts mehr passiert
// ist.
void LCRMSAudioProcessorEditor::scheduleUndoSnapshot()
{
    if (undoRedoInProgress)
        return; // Aenderung stammt aus performUndo()/performRedo() selbst - kein neuer Schritt.
    constexpr int kUndoDebounceFrames = 8; // ~400ms bei 20Hz
    undoDebounceFramesLeft = kUndoDebounceFrames;
}

void LCRMSAudioProcessorEditor::performUndo()
{
    if (undoIndex <= 0)
        return;
    --undoIndex;
    undoRedoInProgress = true;
    if (auto tree = juce::ValueTree::readFromData (undoHistory[undoIndex].getData(), undoHistory[undoIndex].getSize());
        tree.isValid())
        processor.apvts.replaceState (tree);
    storeViewSettingsInState();   // View gehoert nicht zu Undo/Redo
    undoRedoInProgress = false;
    updateUndoRedoButtonStates();
}

void LCRMSAudioProcessorEditor::performRedo()
{
    if (undoIndex < 0 || undoIndex >= undoHistory.size() - 1)
        return;
    ++undoIndex;
    undoRedoInProgress = true;
    if (auto tree = juce::ValueTree::readFromData (undoHistory[undoIndex].getData(), undoHistory[undoIndex].getSize());
        tree.isValid())
        processor.apvts.replaceState (tree);
    storeViewSettingsInState();   // View gehoert nicht zu Undo/Redo
    undoRedoInProgress = false;
    updateUndoRedoButtonStates();
}

void LCRMSAudioProcessorEditor::updateUndoRedoButtonStates()
{
    undoButton.setEnabled (undoIndex > 0);
    redoButton.setEnabled (undoIndex >= 0 && undoIndex < undoHistory.size() - 1);
    undoButton.repaint();
    redoButton.repaint();
}

juce::Font LCRMSAudioProcessorEditor::sectionTitleFont() { return titleFont(); }
juce::Font LCRMSAudioProcessorEditor::paramLabelFont()   { return paramFont(); }

void LCRMSAudioProcessorEditor::timerCallback()
{
    if (processor.isBypassedNow() && globalBypassButton.isShowing())
        globalBypassButton.repaint();   // Runde 168: Bypass-Knopf atmet
    // Runde 164 (User: "6 Sektionen, 6 Augen"): der Wuerfel zeigt, wie viele
    // Sektionen gerade an sind (LCR zaehlt nur mit laufender Engine). Nach
    // einem Klick rollt er ~0,4 s durch zufaellige Augen und landet dann dort.
    {
        auto on = [this] (const char* id) { return processor.apvts.getRawParameterValue (id)->load() > 0.5f; };
        int face = (on (LCRMSAudioProcessor::ID_LCR_ENABLED) && on (LCRMSAudioProcessor::ID_GALAXY_ACTIVATE) ? 1 : 0)
                 + (on (LCRMSAudioProcessor::ID_POL_ON) ? 1 : 0) + (on (LCRMSAudioProcessor::ID_DRIFT_ON) ? 1 : 0)
                 + (on (LCRMSAudioProcessor::ID_WIDTHBOOST_ON) ? 1 : 0) + (on (LCRMSAudioProcessor::ID_FLOW_ON) ? 1 : 0)
                 + (on (LCRMSAudioProcessor::ID_RAY_ON) ? 1 : 0);
        if (juce::Time::getMillisecondCounterHiRes() < dieRollUntilMs)
        {
            face = 1 + juce::Random::getSystemRandom().nextInt (6);
            if (face == dieShownFace) face = face % 6 + 1;
        }
        if (face != dieShownFace)
        {
            dieShownFace = face;
            globalChaosButton.getProperties().set ("dieFace", face);
            globalChaosButton.repaint();
        }
    }
    // Runde 143: die Sterne ums Smart-Profil funkeln - nur ihr kleiner Bereich.
    if (! profileStarsArea.isEmpty() && categoryButton.isVisible())
        content.repaint (profileStarsArea);
    updateHintBar();
    bool needsRepaint = false;

    // Auto-Gain-Anzeige im Footer nachziehen (nur bei echter Bewegung).
    {
        const float db = processor.autoGainDb.load (std::memory_order_relaxed);
        if (std::abs (db - lastAutoGainDb) > 0.05f)
        {
            lastAutoGainDb = db;
            content.repaint (autoGainReadoutArea.expanded (4));
        }
        // Runde 110: Rahmen des Sternenfelds in der Sektions-Rahmenfarbe.
        {
            const int fc = (int) themePalette().frameMain.getARGB();
            if ((int) goniometer.getProperties().getWithDefault ("frameColour", 0) != fc)
            {
                goniometer.getProperties().set ("frameColour", fc);
                goniometer.repaint();
            }
        }
        // Runde 110: der Chip im Footer zeigt Wert bzw. OFF.
        const bool agOn = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_AUTO_GAIN)->load() > 0.5f;
        const juce::String agTxt = ! agOn ? juce::String ("OFF")
                                 : (std::abs (lastAutoGainDb) < 0.05f) ? juce::String ("0.0")
                                 : (lastAutoGainDb > 0.0f ? "+" : "") + juce::String (lastAutoGainDb, 1);
        if (autoGainButton.getButtonText() != agTxt)
            autoGainButton.setButtonText (agTxt);
        if (autoGainButton.getToggleState() != agOn)
            autoGainButton.setToggleState (agOn, juce::dontSendNotification);
    }

    // Demo-Absenkung: nur neu zeichnen, wenn sich wirklich etwas bewegt.
    {
        const float duck = processor.demoDuck.load (std::memory_order_relaxed);
        if (std::abs (duck - lastDemoDuck) > 0.004f)
        {
            lastDemoDuck = duck;
            content.repaint();
        }
    }

    // Aktueller Solo-Status zuerst ermitteln - wird gebraucht, um ALLE nicht
    // soloten Sektionen visuell wie "aus" darzustellen (User-Feedback:
    // "Wenn eine Section Solo ist, dann muessen die anderen alle ausgegraut
    // sein so wie wenn sie off sind"). Die Formel hier ist bewusst identisch
    // zu der in PluginProcessor::processBlock() verwendeten, damit die GUI
    // exakt zeigt, was tatsaechlich verarbeitet wird.
    const int currentSolo = juce::jlimit (0, LCRMSAudioProcessor::SOLO_MAX, (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_SOLO_SECTION)->load()));
    const bool soloActive = currentSolo != LCRMSAudioProcessor::SOLO_NONE;
    // Runde 124: Wuerfel bei Solo gedimmt und ohne Wirkung (siehe runMutate).
    {
        const float wantA = soloActive ? 0.35f : 1.0f;
        if (std::abs (globalChaosButton.getAlpha() - wantA) > 0.01f)
        {
            globalChaosButton.setAlpha (wantA);
            globalChaosButton.setTooltip (soloActive ? "Smart is paused while a section is soloed"
                                                     : (mutateCategoryValue > 0
                                                          ? "Smart: rolls a new setting inside the selected profile, and decides which sections belong in it"
                                                          : "Smart: randomize the sound and leave every section switched on"));
        }
    }

    // Solo + Sektion aus ist ein Zustand, den niemand absichtlich sucht: die
    // solierte Sektion ist stumm, alle anderen auch. Schaltet man die solierte
    // Sektion aus, geht Solo daher automatisch mit aus (User). Wer die Sektion
    // im Solo als A/B an- und abschalten will, setzt im Menue
    // "Keep Solo When Section Is Off".
    if (soloActive && ! keepSoloWhenSectionOff)
    {
        struct SoloPair { int solo; const char* id; };
        static const SoloPair soloMap[] = {
            { LCRMSAudioProcessor::SOLO_GALAXY,     LCRMSAudioProcessor::ID_LCR_ENABLED },
            { LCRMSAudioProcessor::SOLO_POLARITY,   LCRMSAudioProcessor::ID_POL_ON },
            { LCRMSAudioProcessor::SOLO_TIMEWARP,   LCRMSAudioProcessor::ID_DRIFT_ON },
            { LCRMSAudioProcessor::SOLO_DIMENSION,  LCRMSAudioProcessor::ID_WIDTHBOOST_ON },
            { LCRMSAudioProcessor::SOLO_HYPERDRIVE, LCRMSAudioProcessor::ID_FLOW_ON },
            { LCRMSAudioProcessor::SOLO_POSITION,   LCRMSAudioProcessor::ID_POS_ON },
            { LCRMSAudioProcessor::SOLO_RAY,        LCRMSAudioProcessor::ID_RAY_ON }
        };
        for (const auto& p : soloMap)
            if (p.solo == currentSolo
                && processor.apvts.getRawParameterValue (p.id)->load() <= 0.5f)
            {
                if (auto* sp = processor.apvts.getParameter (LCRMSAudioProcessor::ID_SOLO_SECTION))
                    sp->setValueNotifyingHost (sp->convertTo0to1 ((float) LCRMSAudioProcessor::SOLO_NONE));
                break;
            }
    }

    auto syncFrameOn = [&] (bool& frameFlag, const char* paramId, int soloValue) -> bool
    {
        const bool rawOn = processor.apvts.getRawParameterValue (paramId)->load() > 0.5f;
        const bool effectiveOn = rawOn && (! soloActive || currentSolo == soloValue);
        if (frameFlag != effectiveOn) { frameFlag = effectiveOn; needsRepaint = true; }
        return effectiveOn;
    };

    // Wichtig: Die Power-Icons schalten nur die DSP-Verarbeitung stumm/aktiv,
    // NICHT die Bedienbarkeit der Regler - alle Parameter bleiben auch bei
    // ausgeschalteter Section einstellbar (z.B. um in Ruhe vorzubereiten,
    // was passiert, sobald man die Section wieder einschaltet). Die Boxen
    // werden trotzdem sichtbar ausgegraut (siehe paintContent/drawGroup),
    // und zusaetzlich verlieren alle Regler/Buttons der Section ihre Farbe
    // (Component-Property "sectionOff", ausgewertet in CustomLookAndFeel) -
    // bedienbar bleiben sie trotzdem. Ist irgendwo Solo aktiv, gilt "aus"
    // fuer alle Sektionen ausser der soloten (siehe syncFrameOn oben).
    const bool isLcrOn        = syncFrameOn (lcrFrameOn, LCRMSAudioProcessor::ID_LCR_ENABLED, LCRMSAudioProcessor::SOLO_GALAXY);
    const bool isDriftOn      = syncFrameOn (driftFrameOn, LCRMSAudioProcessor::ID_DRIFT_ON, LCRMSAudioProcessor::SOLO_TIMEWARP);
    const bool isPolOn        = syncFrameOn (polFrameOn, LCRMSAudioProcessor::ID_POL_ON, LCRMSAudioProcessor::SOLO_POLARITY);
    const bool isWidthBoostOn = syncFrameOn (widthBoostFrameOn, LCRMSAudioProcessor::ID_WIDTHBOOST_ON, LCRMSAudioProcessor::SOLO_DIMENSION);
    const bool isFlowOn       = syncFrameOn (flowFrameOn, LCRMSAudioProcessor::ID_FLOW_ON, LCRMSAudioProcessor::SOLO_HYPERDRIVE);
    const bool isPosOn        = syncFrameOn (posFrameOn, LCRMSAudioProcessor::ID_POS_ON, LCRMSAudioProcessor::SOLO_POSITION);
    const bool isRayOn        = syncFrameOn (rayFrameOn, LCRMSAudioProcessor::ID_RAY_ON, LCRMSAudioProcessor::SOLO_RAY);
    // Pair-Kopplung: Speed/Sync/Bars in Hyperdrive bleiben aktiv gezeichnet,
    // solange RAYE laeuft und gekoppelt ist (siehe Block weiter unten).
    const bool rayPairedForHyper = isRayOn
        && processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_RAY_PAIR)->load() > 0.5f;
    // Runde 116 (User): dasselbe fuer "EQ -> LCR" - EQ-Feld, Punkte und x2 in
    // MID-SIDE bleiben aktiv gezeichnet, solange LCR laeuft (auch Solo) und
    // der EQ dort sitzt.
    const bool eqInLcrLive = isLcrOn
        && processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_MS_EQ_LCR)->load() > 0.5f;

    // Logo-Klick-Bypass: ALLE Regler bekommen zusaetzlich zum grauen
    // Overlay (siehe paintContent) exakt dieselbe graue "Aus"-Farbgebung wie
    // eine einzeln ausgeschaltete Sektion (User-Feedback: "Farben sollen so
    // sein wie wenn die Sektions off sind", vorher wurde nur abgedunkelt).
    const bool uiBypassed = processor.isBypassedNow();   // Logo-Klick ODER Host-Bypass
    // Fuer die Tiefe-Regler UND Mod-Icons gebraucht (siehe unten): global
    // stummgeschaltete Modulation soll dieselbe graue Farbe erzwingen wie
    // wenn die Sektion selbst aus waere (User-Feedback: "Global Mod off
    // sollte auch die Mod Icons und Mod Regler farblich wieder grau
    // machen. Logisch.").
    const bool globalModBypassForColour = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_GLOBAL_MOD_BYPASS)->load() > 0.5f;
    // Bug-Fix: das reine Setzen der Component-Property loeste bisher KEIN
    // Repaint aus (JUCE-Properties sind nur Metadaten, kein Trigger fuer
    // Neuzeichnen) - dadurch blieb ein Regler nach dem Umschalten von Bypass
    // so lange in seiner ALTEN Farbe stehen, bis irgendein ANDERER Grund
    // (z.B. Maus-Hover-Highlight bei Buttons) zufaellig einen Repaint
    // ausgeloest hat. Jetzt wird bei jeder tatsaechlichen Aenderung explizit
    // reagiert (spart unnoetige Repaints, wenn sich nichts geaendert hat).
    auto setSectionOff = [uiBypassed] (juce::Component& c, bool sectionIsOn)
    {
        const bool newOff = uiBypassed || ! sectionIsOn;
        const bool oldOff = c.getProperties().getWithDefault ("sectionOff", false);
        if (oldOff != newOff)
        {
            c.getProperties().set ("sectionOff", newOff);
            c.repaint();
        }
    };
    setSectionOff (gravitySlider, isLcrOn);
    // Runde 117: Schloesser dimmen mit ihrer Sektion (auch bei Solo).
    setSectionOff (lcrLockButton,        isLcrOn);
    setSectionOff (polLockButton,        isPolOn);
    setSectionOff (driftLockButton,      isDriftOn);
    setSectionOff (widthBoostLockButton, isWidthBoostOn);
    setSectionOff (flowLockButton,       isFlowOn);
    setSectionOff (posLockButton,        isPosOn);
    setSectionOff (rayLockButton,        isRayOn);
    setSectionOff (orbitSlider, isLcrOn);
    setSectionOff (horizonSlider, isLcrOn);
    setSectionOff (driftSlider, isDriftOn);
    setSectionOff (bendSlider, isDriftOn);
    setSectionOff (driftBalanceButton, isDriftOn);
    setSectionOff (parallaxAmountSlider, isDriftOn);
    for (auto& b : parallaxModeButtons) setSectionOff (b, isDriftOn);
    {
        const bool dotsOff = uiBypassed || ! isDriftOn;
        if (pxModeDots.off != dotsOff) { pxModeDots.off = dotsOff; pxModeDots.repaint(); }
    }
    setSectionOff (galaxyFilterButton, isLcrOn);
    setSectionOff (dimFilterButton, isWidthBoostOn);
    setSectionOff (posFilterButton, isPosOn);
    // Nehmen Galaxy, Dimension UND Vision den Focus heraus, wirkt die Leiste
    // gerade auf nichts. Sie wird dann sichtbar gedimmt - aber NICHT automatisch
    // abgeschaltet (User-Idee, bewusst nicht umgesetzt: ein Parameter, den man
    // nicht selbst angefasst hat, soll sich auch nicht selbst umlegen).
    prismBand.setAllBypassed (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_PRISM_GALAXY)->load() > 0.5f
                           && processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_PRISM_DIM)->load()    > 0.5f
                           && processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_PRISM_VIS)->load()    > 0.5f);
    setSectionOff (polLButton, isPolOn);
    setSectionOff (polRButton, isPolOn);
    setSectionOff (polLinkButton, isPolOn);
    setSectionOff (polPos1Button, isPolOn);
    setSectionOff (polPos2Button, isPolOn);
    {
        const bool dotsOff = uiBypassed || ! isPolOn;
        if (polPosDots.off != dotsOff) { polPosDots.off = dotsOff; polPosDots.repaint(); }
    }
    setSectionOff (polPos3Button, isPolOn);
    setSectionOff (polPos4Button, isPolOn);
    setSectionOff (sideWidthSlider, isWidthBoostOn);
    setSectionOff (sideBoostSlider, isWidthBoostOn);
    setSectionOff (movementSlider, isFlowOn);
    setSectionOff (pulseButton, isFlowOn);
    setSectionOff (speedRateSlider, isFlowOn || rayPairedForHyper);
    setSectionOff (syncButton, isFlowOn || rayPairedForHyper);
    setSectionOff (speedBox, isFlowOn || rayPairedForHyper);
    // Tilt und Depth haengen jetzt an ihren NEUEN Sektionen, nicht mehr an
    // Vision - sonst wuerden sie von einem Rahmen gedimmt, den es nicht mehr
    // gibt. Width und Elevate sind unsichtbar, ihre Zeilen koennen weg.
    setSectionOff (offsetSlider, isDriftOn);
    setSectionOff (distanceSlider, isWidthBoostOn);
    // Runde 125: EQ aus -> Feld, Punkte und Fader gedimmt wie eine Sektion;
    // der An/Aus-Schalter selbst dimmt nur mit der Sektion.
    const bool msEqOnNow = true;   // Runde 159: kein An/Aus mehr - "aus" ist FLAT
    const bool msEqLive  = (isWidthBoostOn || eqInLcrLive) && msEqOnNow;
    setSectionOff (msEqButton,      msEqLive);
    // Runde 159 (User): bei FLAT tut der Fader nichts - dann grau und
    // nicht bedienbar, so sieht man sofort "hier passiert nichts".
    const bool msEqFlatNow = sideeq::isFlat ((int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_MS_EQ)->load()));
    setSectionOff (msEqAmtSlider,   msEqLive && ! msEqFlatNow);
    // Runde 161 (User): bei FLAT grau, aber weiter bewegbar (vorwaehlen).
    setSectionOff (msEqPowerButton, isWidthBoostOn || eqInLcrLive);
    // Runde 116: EQ -> LCR haengt nur an LCR (wie LINK am Phaser).
    setSectionOff (lcrEqButton, isLcrOn);
    {
        const bool eqMoved = eqInLcrLive && msEqOnNow;
        // Runde 125: Schalter und Fader in Gold, blau wenn der EQ in LCR sitzt.
        const int accentNow = (int) (eqMoved ? pairAccentColour() : altAccentColour()).getARGB();
        if ((int) msEqPowerButton.getProperties().getWithDefault ("powerColour", 0) != accentNow)
        {
            msEqPowerButton.getProperties().set ("powerColour", accentNow);
            msEqPowerButton.repaint();
        }
        if ((int) msEqAmtSlider.getProperties().getWithDefault ("faderColour", 0) != accentNow)
        {
            msEqAmtSlider.getProperties().set ("faderColour", accentNow);
            msEqAmtSlider.repaint();
        }
        if ((bool) lcrEqButton.getProperties().getWithDefault ("pairedGold", false) != eqMoved)
        {
            lcrEqButton.getProperties().set ("pairedGold", eqMoved);
            lcrEqButton.repaint();
        }
        const int want = (int) (eqMoved ? pairAccentColour() : themePalette().frameRaye).getARGB();
        if ((int) msEqButton.getProperties().getWithDefault ("pillColour", 0) != want)
        {
            msEqButton.getProperties().set ("pillColour", want);
            msEqButton.repaint();
        }
        if (msEqDots.paired != eqMoved) { msEqDots.paired = eqMoved; msEqDots.repaint(); }
        // Runde 122 (User): x2 gehoert zum EQ und wird mit blau.
        if ((bool) msEqX2Button.getProperties().getWithDefault ("pairTint", false) != eqMoved)
        {
            msEqX2Button.getProperties().set ("pairTint", eqMoved);
            msEqX2Button.repaint();
        }
    }
    {
        const bool dotsOff = uiBypassed || ! msEqLive;
        if (msEqDots.off != dotsOff) { msEqDots.off = dotsOff; msEqDots.repaint(); }
    }
    setSectionOff (rayStrengthButton, isRayOn);
   #if SPACEX_RAYE_UI == 1
    setSectionOff (rayAmountSlider, isRayOn);
    setSectionOff (rayCharButton, isRayOn);
    setSectionOff (rayFastButton, isRayOn);
    {
        const bool dotsOff = uiBypassed || ! isRayOn;
        if (rayModeDots.off != dotsOff) { rayModeDots.off = dotsOff; rayModeDots.repaint(); }
    }
   #endif
    setSectionOff (rayPairButton, isRayOn);
    // Regler-Beschriftungen: bei Sektion aus deutlich dunkler (User: "hilft
    // nochmal zu sehen, dass die Section off ist"). Farbe aus der Off-
    // Fuellfarbe der Sektion abgeleitet (siehe labelOffColour()).
    {
        // Moon (User): der Text INNERHALB der Sektionen bekommt den Ton, den
        // bisher die Sektionstitel hatten - ganz leicht waermer als das
        // bisherige neutrale Grau; die Titel selbst werden dafuer blaeulich.
        const juce::Colour labelOn = isMoonTheme()
                                   ? themePalette().frameMain.brighter (0.25f).interpolatedWith (themePalette().knob, 0.35f)
                                   : juce::Colour (0xffbec3cb);   // etwas weg vom Weiss (User: Augen)
        const juce::Colour labelOff (labelOffColour());
        auto setLabelOff = [&] (juce::Label& l, bool sectionIsOn)
        {
            const juce::Colour want = (uiBypassed || ! sectionIsOn) ? labelOff : labelOn;
            if (l.findColour (juce::Label::textColourId) != want)
                l.setColour (juce::Label::textColourId, want);
        };
        setLabelOff (gravityLabel,   isLcrOn);
        setLabelOff (orbitLabel,     isLcrOn);
        setLabelOff (horizonLabel,   isLcrOn);
        setLabelOff (driftLabel,     isDriftOn);
        setLabelOff (bendLabel,      isDriftOn);
        setLabelOff (sideWidthLabel, isWidthBoostOn);
        setLabelOff (sideBoostLabel, isWidthBoostOn);
        setLabelOff (movementLabel,  isFlowOn);
        setLabelOff (speedLabel,     isFlowOn || rayPairedForHyper);
        setLabelOff (offsetLabel,    isDriftOn);
        setLabelOff (distanceLabel,  isWidthBoostOn);
        setLabelOff (rayRateLabel,   isRayOn);
        // Runde 53 (User): die beiden AMOUNT-Labels blieben hell, wenn ihre
        // Sektion aus war.
        setLabelOff (parallaxAmountLabel, isDriftOn);
        setLabelOff (rayAmountLabel,      isRayOn);
    }
    // Speed ist bei Pair inaktiv - dann diktiert Hyperdrive die Rate.
    {
        const bool rayPaired = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_RAY_PAIR)->load() > 0.5f;
        setSectionOff (rayRateSlider, isRayOn && ! rayPaired);

        // ===== PAIR SICHTBAR MACHEN =====
        // User: "wenn pair an ist soll visuell klar sein, dass ray an
        // hyperdrive gekoppelt ist -> und auch wenn hyperdrive section off
        // ist soll speed bzw. sync + bars leuchtend bleiben solange pair an
        // ist". Umgesetzt mit der Gold-Variante: der Pair-Knopf leuchtet
        // gold, und Speed/Sync/Bars in Hyperdrive bekommen dieselbe goldene
        // Markierung (Eigenschaft "pairedGold", ausgewertet im LookAndFeel)
        // und bleiben aktiv gezeichnet, solange Pair an ist - denn sie
        // WIRKEN dann ja weiter, nur eben auf RAYE.
        const bool pairLive = isRayOn && rayPaired;
        // Gold nur dort, wo das Tempo gerade WIRKLICH herkommt (User: "entweder
        // speed ODER Sync/Bars - nicht beides"): mit Sync leuchten Sync und
        // Bars, ohne Sync der Speed-Regler.
        const bool hyperSync = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_SPEED_SYNC)->load() > 0.5f;
        struct GoldTarget { juce::Component* c; bool on; };
        const GoldTarget targets[] = {
            { &rayPairButton,  pairLive },
            { &speedRateSlider, pairLive && ! hyperSync },
            { &syncButton,      pairLive &&   hyperSync },
            { &speedBox,        pairLive &&   hyperSync },
        };
        for (const auto& t : targets)
        {
            const bool was = t.c->getProperties().getWithDefault ("pairedGold", false);
            if (was != t.on) { t.c->getProperties().set ("pairedGold", t.on); t.c->repaint(); }
        }
        // Polarity-Marker (siehe paintContent) nachziehen, wenn Position oder
        // Sektionszustand wechseln.
        {
            const int polPosNow = juce::jlimit (0, 3, (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_POL_POS)->load()));
            if (polPosNow != lastPolPosShown) { lastPolPosShown = polPosNow; content.repaint(); }
        }
        if (pairGoldFrameOn != pairLive || pairGoldSync != hyperSync)
        {
            pairGoldFrameOn = pairLive;
            pairGoldSync = hyperSync;
            content.repaint();
        }
        // Stufe des Staerke-Icons aus dem Parameter nachziehen.
        const int lvl = juce::jlimit (0, 3, (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_RAY_STRENGTH)->load()));
        if ((int) rayStrengthButton.getProperties().getWithDefault ("rayLevel", 0) != lvl)
        {
            rayStrengthButton.getProperties().set ("rayLevel", lvl);
            rayStrengthButton.repaint();
        }
    }
    // Mod-Icons: farbig nur, wenn die Sektion an ist UND Modulation nicht
    // global stummgeschaltet ist (das eigene An/Aus des Icons selbst wird
    // schon in CustomLookAndFeel::drawModIcon ueber button.getToggleState()
    // beruecksichtigt).
    setSectionOff (driftModButton, isDriftOn && ! globalModBypassForColour);
    setSectionOff (dimensionModButton, isWidthBoostOn && ! globalModBypassForColour);
    setSectionOff (hyperdriveModButton, isFlowOn && ! globalModBypassForColour);
    setSectionOff (galaxyModButton, isLcrOn && ! globalModBypassForColour);
    setSectionOff (positionModButton, isPosOn && ! globalModBypassForColour);
    // Tiefe-Regler: zusaetzlich zur Sektion und globalem Mod-Bypass jetzt
    // auch vom EIGENEN Mod-Icon-Toggle abhaengig - grau, solange die
    // Modulation fuer diese Sektion nicht eingeschaltet ist, farbig sobald
    // sie es ist (User-Feedback: "Wenn Mod Icon nicht aktiv sollte der
    // Regler daneben grau sein. Nur farbig wenn aktiv. Aber weiterhin
    // einstellbar." - bleibt ueber setSectionOff bedienbar, nur die Farbe
    // aendert sich).
    // Runde 87 (User): steht der Tiefenregler ganz unten, moduliert nichts -
    // dann soll er auch aussehen wie ausgeschaltet, nicht wie aktiv auf null.
    auto depthLive = [] (const juce::Slider& sl) { return sl.getValue() > 0.05; };
    // Runde 104: die Sektions-Mod-Schalter gibt es nicht mehr - an/aus macht
    // nur der globale Schalter. Die Tiefe-Regler (nur noch im Tune-Build
    // sichtbar) haengen deshalb nicht mehr an ihnen.
    setSectionOff (driftModDepthSlider, isDriftOn && ! globalModBypassForColour && depthLive (driftModDepthSlider));
    setSectionOff (dimensionModDepthSlider, isWidthBoostOn && ! globalModBypassForColour && depthLive (dimensionModDepthSlider));
    setSectionOff (hyperdriveModDepthSlider, isFlowOn && ! globalModBypassForColour && depthLive (hyperdriveModDepthSlider));
    setSectionOff (galaxyModDepthSlider, isLcrOn && ! globalModBypassForColour && depthLive (galaxyModDepthSlider));
    setSectionOff (positionModDepthSlider, isPosOn && ! globalModBypassForColour && positionModButton.getToggleState() && depthLive (positionModDepthSlider));
    // VOL, Mono-Check und Mono-Dry sind keiner "Sektion" zugeordnet, sollen
    // bei Bypass aber genauso ausgegraut werden wie alle anderen Regler (bei
    // Bypass hat Mono-Check ohnehin keine Wirkung mehr, siehe DSP).
    setSectionOff (volSlider, ! uiBypassed);
    setSectionOff (panSlider, ! uiBypassed);

    setSectionOff (mixSlider, ! uiBypassed);
    setSectionOff (monoCheckButton, ! uiBypassed);
    setSectionOff (monoDryButton, ! uiBypassed);
    setSectionOff (autoGainButton, ! uiBypassed);   // Runde 174: ganze Fusszeile dimmt im Bypass
    // Power- und Solo-Icons zeigten bisher NUR ihren eigenen Toggle-Status -
    // bei Bypass blieb eine eingeschaltete Sektion daher weiterhin farbig
    // leuchtend, obwohl der Rest der Sektion ausgegraut wurde (User-
    // gemeldeter Bug). Die eigentliche An/Aus-Logik jeder Sektion soll sich
    // NICHT aendern (kommt weiterhin aus dem jeweiligen Toggle-Status) -
    // hier zaehlt daher ausschliesslich Bypass, "sectionIsOn" ist bewusst
    // immer true.
    setSectionOff (lcrPowerButton,        true);
    setSectionOff (lcrSoloButton,         true);
    setSectionOff (driftPowerButton,      true);
    setSectionOff (driftSoloButton,       true);
    setSectionOff (polPowerButton,        true);
    setSectionOff (polSoloButton,         true);
    setSectionOff (widthBoostPowerButton, true);
    setSectionOff (widthBoostSoloButton,  true);
    setSectionOff (flowPowerButton,       true);
    setSectionOff (flowSoloButton,        true);
    setSectionOff (posPowerButton,        true);
    setSectionOff (posSoloButton,         true);

    // Sektions-Titel: etwas dunkler/blasser (45% Deckkraft der Akzentfarbe),
    // wenn die Sektion aus ist oder gerade bypasst wird - User-Feedback: "bin
    // die Farbe an sich gut, aber vielleicht bisschen dunkler/blasser".
    // Aus-Zustand deutlich staerker als frueher (0,45 Alpha reichte nicht -
    // User: "der Section Name ist zwischen on und off noch zu aehnlich in
    // Intensitaet und Helligkeit"). Der Titel rutscht jetzt farblich weit in
    // Richtung der Off-Fuellfarbe der Sektion, bleibt aber lesbar. Pop
    // behaelt sein bisheriges Verhalten.
    auto applyTitleDim = [] (juce::Label& label, juce::Colour fullColour, bool isOn)
    {
        const juce::Colour offCol = isComicTheme()
                                  ? fullColour.withAlpha (0.34f)   // Pop: bei Aus dunkler (User)
                                  : sectionOffFill().interpolatedWith (fullColour, 0.34f).brighter (0.22f).withAlpha (0.88f);
        juce::Colour want = isOn ? fullColour : offCol;
        // Hover-Feedback auf dem Sektionsnamen (User: der Name ist klickbar,
        // hatte aber als einziges Element keinerlei Rueckmeldung). Laeuft hier
        // mit, weil diese Funktion ohnehin bei jedem Timer-Tick durchlaeuft -
        // sonst wuerde applyTitleDim die Hover-Farbe sofort wieder ueberschreiben.
        if (label.isMouseOver (true))
            want = want.brighter (0.40f).withAlpha (1.0f);
        if (label.findColour (juce::Label::textColourId) != want)
            label.setColour (juce::Label::textColourId, want);
    };
    // Titelfarben: Pop bunt wie bisher, sonst aus der Theme-Palette (eine
    // Familie, User: "section header zu viele unterschiedliche Farben").
    {
        const auto pal = themePalette();
        const bool pop = isComicTheme();
        // Moon (User): Titel von Galaxy bis Vision leicht blaeulich, damit sie
        // sich - aehnlich wie eingeschaltete Polarity-Knoepfe - dezent vom
        // Rest abheben. Flat: minimal staerker abgehoben als bisher.
        // Sci-Fi: eigene Titelfarbe je Variante (siehe ThemePalette::title).
        const juce::Colour tWarm = pal.frameMain.brighter (0.25f).interpolatedWith (pal.knob, 0.35f);   // an: Hauch Akzent (User)
        const juce::Colour tMain = pop ? juce::Colour (0xffb968ff)
                                 : isSciFiTheme() ? pal.titleColour()
                                 : isMoonTheme()  ? tWarm.interpolatedWith (juce::Colour (0xff9fc4ff), 0.45f)
                                 : isFlatTheme()  ? tWarm.brighter (0.16f).interpolatedWith (juce::Colour (0xffbcd0e8), 0.22f)
                                                  : tWarm;
        const juce::Colour tGrn  = tMain;
        applyTitleDim (lcrTitleLabel,        pop ? pal.frameGalaxy : tMain, isLcrOn && ! uiBypassed);   // Galaxy: kein eigener Titelton mehr (User: hat den Glow)
        applyTitleDim (driftTitleLabel,      tMain,           isDriftOn && ! uiBypassed);
        applyTitleDim (polTitleLabel,        tMain,           isPolOn && ! uiBypassed);
        applyTitleDim (widthBoostTitleLabel, tGrn,            isWidthBoostOn && ! uiBypassed);
        applyTitleDim (flowTitleLabel,       tMain,           isFlowOn && ! uiBypassed);
        applyTitleDim (posTitleLabel,        tGrn,            isPosOn && ! uiBypassed);
        applyTitleDim (rayTitleLabel,        pal.frameRaye,   isRayOn && ! uiBypassed);
    }

    // Live-Mod-Anzeige auf den 5 modulierbaren Reglern (Drift, Shift,
    // Expand, Boost, Speed) - zeigt per beweglichem Punkt (siehe
    // CustomLookAndFeel::drawRotarySlider, "modLiveActive"/"modLiveValue")
    // die tatsaechlich gerade modulierte Position, waehrend der normale
    // Zeiger weiter die eingestellte Reglerposition zeigt (User-Feedback:
    // "auch visuell sichtbar wie bei Flow Autopan", "bei allen gleich").
    {
        // Globaler Mod-Bypass mit einrechnen, sonst wuerde der Live-Punkt auf
        // den Reglern weiter "aktiv" anzeigen, obwohl die Modulation gerade
        // komplett stummgeschaltet ist (siehe DSP: ID_GLOBAL_MOD_BYPASS).
        const bool globalModBypassRaw = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_GLOBAL_MOD_BYPASS)->load() > 0.5f;
        // Runde 131 (User-Bug: "ohne Wuerfeln keine Mod-Punkte"): die alten
        // Mod-Schalter je Sektion gibt es seit Runde 101 nicht mehr - der DSP
        // haengt nur am globalen Mod-Schalter, die Punkte hingen aber noch an
        // den alten Parametern, die erst der Wuerfel setzte. Jetzt genau wie
        // im DSP. Micropitch wird nie moduliert.
        const bool timewarpModOnRaw   = false;
        const bool dimensionModOnRaw  = ! globalModBypassRaw;
        const bool hyperdriveModOnRaw = ! globalModBypassRaw;
        const bool galaxyModOnRaw     = ! globalModBypassRaw;
        const bool positionModOnRaw   = ! globalModBypassRaw;
        const bool syncOnRaw = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_SPEED_SYNC)->load() > 0.5f;

        const float driftPctRaw  = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_DRIFT)->load();
        const float bendCtRaw    = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_BEND)->load();
        const float widthPctRaw  = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_SIDE_WIDTH)->load();
        const float boostDbRaw   = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_SIDE_BOOST)->load();
        const float sensPctRaw       = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_LCR_SENS)->load();
        const float blendPctRaw      = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_LCR_BLEND)->load();
        const float offsetPctRaw     = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_POS_OFFSET)->load();
        const float posWidthPctRaw   = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_POS_WIDTH)->load();
        const float distancePctRaw   = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_POS_DISTANCE)->load();
        const float elevatePctRaw    = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_POS_ELEVATE)->load();

        // "Show Modulation"-Menue-Schalter (User-Wunsch) - schaltet die
        // beweglichen Live-Anzeigen komplett ab, unabhaengig vom eigentlichen
        // Modulationsstatus.
        // LIFE auf 0 = keine Modulation -> auch keine Mod-Punkte (User, Runde 38).
        const float lifePct = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_LIFE)->load();
        const bool lifeOn = lifePct > 0.05f;
        auto applyLive = [this, lifeOn] (juce::Slider& slider, bool active, float liveValue)
        {
            active = active && modulationVisualsEnabled && lifeOn;
            const bool wasActive = slider.getProperties().getWithDefault ("modLiveActive", false);
            slider.getProperties().set ("modLiveActive", active);
            if (active)
            {
                const float t = juce::jlimit (0.0f, 1.0f, (float) slider.getNormalisableRange().convertTo0to1 (liveValue));
                const float old = slider.getProperties().getWithDefault ("modLiveValue", -1.0f);
                slider.getProperties().set ("modLiveValue", t);
                // Review 1.0.1: nur neu zeichnen, wenn der Punkt sich bewegt oder
                // erscheint - vorher 20x pro Sekunde, auch bei stehender DAW.
                if (! wasActive || std::abs (t - old) > 1.0e-4f)
                    slider.repaint();
            }
            else if (wasActive)
            {
                slider.repaint(); // einmalig, damit der Live-Punkt sauber verschwindet
            }
        };

        applyLive (driftSlider, isDriftOn && timewarpModOnRaw && std::abs (driftPctRaw) > 0.001f,
                   processor.currentDriftLivePercent.load (std::memory_order_relaxed));
        applyLive (bendSlider, isDriftOn && timewarpModOnRaw && std::abs (bendCtRaw) > 0.001f,
                   processor.currentBendLiveCt.load (std::memory_order_relaxed));
        applyLive (sideWidthSlider, isWidthBoostOn && dimensionModOnRaw && std::abs (widthPctRaw - 100.0f) > 0.05f,
                   processor.currentExpandLivePercent.load (std::memory_order_relaxed));
        applyLive (sideBoostSlider, isWidthBoostOn && dimensionModOnRaw && std::abs (boostDbRaw) > 0.01f,
                   processor.currentBoostLiveDb.load (std::memory_order_relaxed));
        applyLive (speedRateSlider, isFlowOn && hyperdriveModOnRaw && ! syncOnRaw,
                   processor.currentSpeedLiveHz.load (std::memory_order_relaxed));
        applyLive (gravitySlider, isLcrOn && galaxyModOnRaw && std::abs (sensPctRaw - 50.0f) > 0.05f,
                   processor.currentGravityLivePercent.load (std::memory_order_relaxed));
        // Anders als bei den anderen Reglern kein Abweichungs-Gate mehr - Orbit
        // moduliert jetzt auch exakt auf Default sichtbar (siehe DSP), soll
        // also auch dort schon die Live-Linie zeigen.
        juce::ignoreUnused (blendPctRaw);
        applyLive (orbitSlider, isLcrOn && galaxyModOnRaw,
                   processor.currentOrbitLivePercent.load (std::memory_order_relaxed));
        // Runde 131: HF Regain und Phaser Amount werden jetzt auch moduliert.
        {
            const float regainRaw = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_LCR_HORIZON)->load();
            applyLive (horizonSlider, isLcrOn && galaxyModOnRaw && regainRaw >= 0.5f,
                       processor.currentRegainLivePercent.load (std::memory_order_relaxed));
            const float rayAmtRaw = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_RAY_AMOUNT)->load();
            applyLive (rayAmountSlider, isRayOn && ! globalModBypassRaw && rayAmtRaw > 0.05f,
                       processor.currentRayAmountLive.load (std::memory_order_relaxed));
        }
        applyLive (offsetSlider, isPosOn && positionModOnRaw && std::abs (offsetPctRaw) > 0.05f,
                   processor.currentOffsetLivePercent.load (std::memory_order_relaxed));
        applyLive (posWidthSlider, isPosOn && positionModOnRaw && std::abs (posWidthPctRaw - 100.0f) > 0.05f,
                   processor.currentPosWidthLivePercent.load (std::memory_order_relaxed));
        // DEPTH sitzt in DIMENSION (Regler distanceSlider, Parameter ID_DEPTH)
        // - der Punkt haengt also am Dimension-Mod, nicht an Position
        // (User Runde 51: "Depth hat immer noch keinen optischen Punkt").
        {
            const float depthPctRaw = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_DEPTH)->load();
            applyLive (distanceSlider, isWidthBoostOn && dimensionModOnRaw && std::abs (depthPctRaw) > 0.05f,
                       processor.currentDepthLivePercent.load (std::memory_order_relaxed));
        }
        juce::ignoreUnused (distancePctRaw);
        applyLive (elevateSlider, isPosOn && positionModOnRaw && std::abs (elevatePctRaw) > 0.05f,
                   processor.currentElevateLivePercent.load (std::memory_order_relaxed));

        // PARALLAX Amount: Punkt zeigt das modulierte Amount (Runde 44 aus dem
        // Processor, der die Modulation jetzt direkt auf Amount anwendet).
        {
            const float amountRaw = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_PARALLAX_AMOUNT)->load();
            applyLive (parallaxAmountSlider, isDriftOn && timewarpModOnRaw && amountRaw > 0.05f,
                       processor.currentParallaxAmountLive.load (std::memory_order_relaxed));
        }

        // Show Advanced Modulation + LIFE unter 100 %: die Tiefe-Regler in den
        // Sektionskoepfen zeigen per Punkt die WIRKSAME Tiefe (Tiefe x Life) -
        // der Punkt wandert beim Drehen an Life sichtbar mit (User, Runde 38).
        {
            const float life01 = juce::jlimit (0.0f, 1.0f, lifePct * 0.01f);
            const bool lifeScaled = advancedModVisible && life01 < 0.999f;
            auto depthDot = [&] (juce::Slider& s, bool modOn, const char* depthId)
            {
                const float d = processor.apvts.getRawParameterValue (depthId)->load();
                applyLive (s, lifeScaled && modOn && d > 0.05f, d * life01);
            };
            depthDot (galaxyModDepthSlider,     galaxyModOnRaw,     LCRMSAudioProcessor::ID_GALAXY_DEPTH);
            depthDot (driftModDepthSlider,      timewarpModOnRaw,   LCRMSAudioProcessor::ID_TIMEWARP_DEPTH);
            depthDot (dimensionModDepthSlider,  dimensionModOnRaw,  LCRMSAudioProcessor::ID_DIMENSION_DEPTH);
            depthDot (hyperdriveModDepthSlider, hyperdriveModOnRaw, LCRMSAudioProcessor::ID_HYPERDRIVE_DEPTH);
        }
    }

    // Solo-Icons mit dem gemeinsamen Choice-Parameter synchron halten -
    // exklusiv, nur das aktuell aktive Solo-Icon leuchtet.
    {
        struct SoloEntry { juce::TextButton* button; int value; };
        SoloEntry soloEntries[] = {
            { &lcrSoloButton,        LCRMSAudioProcessor::SOLO_GALAXY },
            { &driftSoloButton,      LCRMSAudioProcessor::SOLO_TIMEWARP },
            { &polSoloButton,        LCRMSAudioProcessor::SOLO_POLARITY },
            { &widthBoostSoloButton, LCRMSAudioProcessor::SOLO_DIMENSION },
            { &flowSoloButton,       LCRMSAudioProcessor::SOLO_HYPERDRIVE },
            { &posSoloButton,        LCRMSAudioProcessor::SOLO_POSITION },
            { &raySoloButton,        LCRMSAudioProcessor::SOLO_RAY },
        };
        for (auto& entry : soloEntries)
        {
            const bool shouldBeOn = (entry.value == currentSolo);
            if (entry.button->getToggleState() != shouldBeOn)
            {
                entry.button->setToggleState (shouldBeOn, juce::dontSendNotification);
                entry.button->repaint();
            }
        }
    }

    // Korrelationsmesser mit dem aktuellen Wert aus dem Audio-Thread fuettern.
    correlationMeter.setCorrelation (processor.currentCorrelation.load (std::memory_order_relaxed));

    const bool isSyncOn = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_SPEED_SYNC)->load() > 0.5f;
    // Runde 130: nicht mehr abschalten, sondern sperren - Cmd-Klick-Reset
    // und Doppelklick gehen auch bei Sync + Bars.
    if ((bool) speedRateSlider.getProperties().getWithDefault ("syncLocked", false) != isSyncOn)
    {
        speedRateSlider.getProperties().set ("syncLocked", isSyncOn);
        speedRateSlider.repaint();
    }
    // Bar-Auswahl bleibt jetzt auch bei ausgeschaltetem Sync waehlbar (User-
    // Feedback: "Bar Section soll auswaehlbar sein auch wenn Sync off
    // ist.") - nur die "glowActive"-Hervorhebung unten zeigt weiterhin an,
    // ob Sync gerade tatsaechlich wirkt.
    speedBox.setEnabled (true);

    // Hyperdrive-Mod bleibt jetzt auch bei aktivem Bar-Sync bedienbar
    // (User-Feedback: "trotzdem an gehen, wirkt sich dann eben nur auf Flow
    // aus") - moduliert bei Sync intern nur noch Movement/Flow, nicht mehr
    // Speed (siehe DSP), daher hier keine Deaktivierung mehr noetig.

    const bool wasGlow = speedBox.getProperties().getWithDefault ("glowActive", false);
    if (wasGlow != isSyncOn)
    {
        speedBox.getProperties().set ("glowActive", isSyncOn);
        speedBox.repaint();
    }

    // Text der Bar-Auswahl ("1 Bar", "2 Bars", ...) dimmen, wenn Sync aus
    // ist (User-Feedback: "nicht zu hell von der Schrift wenn Sync off ist,
    // damit man direkt sieht 'ach, Sync ist off'") - unabhaengig vom
    // bestehenden Glow-Rahmen oben, der nur den Rand betrifft.
    // Runde 65 (User): reines Weiss war der einzige Punkt in der Oberflaeche,
    // der so hart leuchtete - jetzt derselbe Ton wie "Vol" und die anderen
    // Beschriftungen.
    // Runde 116 (User): Sektion aus -> dieselbe Aus-Farbe wie alle anderen.
    speedBox.setColour (juce::ComboBox::textColourId,
                         ! (isFlowOn || rayPairedForHyper) ? labelOffColour() :
                         isSyncOn ? themePalette().frameRaye.interpolatedWith (juce::Colour (0xfff2f4f8), 0.34f)
                                  : juce::Colour (0xff6a6e78));

    // PARALLAX-Modus-Knoepfe mit dem Parameter synchron halten.
    {
        const int mode = juce::jlimit (0, kPxModes - 1, (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_PARALLAX_MODE)->load()));
       #if SPACEX_PARALLAX_UI == 2
        // Runde 72 (User): "3D FLUX" statt "FLUX" - der Modus verschiebt das
        // Bild je nach Amount mal nach rechts, mal nach links (zwei Wegpunkte
        // mit wanderndem Mix) und gehoert damit zu 3D und DRIFT in die
        // Familie der plastischen Modi, nicht zu den Widenern.
        static const char* const modeNames[kPxModes] = { "VELVET", "HALO", "ILLUSION", "DOUBLE" };
        // Runde 51 (User): in BEIDEN Builds Punkte, kein Fuellbalken mehr -
        // die beiden Builds unterscheiden sich nur noch darin, ob die Punkte
        // IN der Pille oder darunter sitzen.
        if (parallaxModeButtons[0].getButtonText() != modeNames[mode])
            parallaxModeButtons[0].setButtonText (modeNames[mode]);
        // Die Pille traegt die Farbe ihres Sektionstitels (Runde 55). Nur der
        // Editor kennt sie, der LookAndFeel sieht nur den Knopf.
        {
            // Runde 62 (User): derselbe Rahmenton wie bei RAYE - zwei
            // verschiedene Modus-Pillen nebeneinander wirkten unruhig.
            // Bug (User Runde 68): die Farbe kam aus der LIVE-Titelfarbe von
            // RAYE, und die wird gedimmt, sobald RAYE ausgeschaltet ist -
            // dadurch haing die Parallax-Pille am Zustand einer fremden
            // Sektion. Jetzt direkt aus der Palette, ungedimmt; das Dimmen
            // der eigenen Sektion macht der LookAndFeel ueber "sectionOff".
            const int want = (int) themePalette().frameRaye.getARGB();
            if ((int) parallaxModeButtons[0].getProperties().getWithDefault ("pillColour", 0) != want)
            {
                parallaxModeButtons[0].getProperties().set ("pillColour", want);
                parallaxModeButtons[0].repaint();
            }
        }
        if (pxModeDots.index != mode) { pxModeDots.index = mode; pxModeDots.repaint(); }
       #if SPACEX_PX_DIAG_ONLY
        // Runde 88/90: das Modus-Icon folgt dem Modus - nur im Vergleichsbuild.
        if ((int) parallaxModeButtons[0].getProperties().getWithDefault ("pxDiagram", -1) != mode)
        {
            parallaxModeButtons[0].getProperties().set ("pxDiagram", mode);
            parallaxModeButtons[0].repaint();
        }
       #else
        if (parallaxModeButtons[0].getProperties().contains ("pxDiagram"))
        {
            parallaxModeButtons[0].getProperties().remove ("pxDiagram");
            parallaxModeButtons[0].repaint();
        }
       #endif
       #else
        for (int i = 0; i < kPxModes; ++i)
            if (parallaxModeButtons[i].getToggleState() != (i == mode))
                parallaxModeButtons[i].setToggleState (i == mode, juce::dontSendNotification);
       #endif
       #if SPACEX_RAYE_UI == 1
        {
            static const char* const charNames[4] = { "SWEEP", "SHIMMER", "SPIN", "SWIRL" };
            const int c = juce::jlimit (0, 3, (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_RAY_CHAR)->load()));
            if (rayCharButton.getButtonText() != charNames[c])
                rayCharButton.setButtonText (charNames[c]);
            {
                const int want = (int) themePalette().frameRaye.getARGB();   // siehe oben: nicht die gedimmte Live-Farbe
                if ((int) rayCharButton.getProperties().getWithDefault ("pillColour", 0) != want)
                {
                    rayCharButton.getProperties().set ("pillColour", want);
                    rayCharButton.repaint();
                }
            }
           #if SPACEX_PX_DIAG_ONLY
            if ((int) rayCharButton.getProperties().getWithDefault ("rayDiagram", -1) != c)
            {
                rayCharButton.getProperties().set ("rayDiagram", c);
                rayCharButton.repaint();
            }
           #endif
            if (rayModeDots.index != c) { rayModeDots.index = c; rayModeDots.repaint(); }
        }
       #endif
    {
        // Runde 105: Seiten-EQ in MID-SIDE.
        // Runde 133: Anzeige-Reihenfolge FLAT, TIGHT, CLEAR, FOCUS.
        static const char* const eqNames[LCRMSAudioProcessor::kMsEqModes] =
            { "FLAT", "TIGHT", "CLEAR", "FOCUS" };
        const int e = juce::jlimit (0, LCRMSAudioProcessor::kMsEqModes - 1,
                                    (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_MS_EQ)->load()));
        const int eShown = sideeq::displayFromParam (e);
        if (msEqButton.getButtonText() != eqNames[eShown])
            msEqButton.setButtonText (eqNames[eShown]);
        if ((int) msEqButton.getProperties().getWithDefault ("eqDiagram", -1) != e)
        {
            msEqButton.getProperties().set ("eqDiagram", e);
            msEqButton.repaint();
        }
        // Runde 126: die Icon-Form kommt aus tickEqIcon() (60 Hz, fliessend).
        // Farbe: siehe Runde 115 bei setSectionOff (msEqButton) - Gold wie die
        // anderen Icon-Felder, blau wenn der EQ in LCR sitzt.
        if (msEqDots.index != eShown) { msEqDots.index = eShown; msEqDots.repaint(); }
    }
    }

    // Polarity-Positions-Buttons mit dem aktuellen Parameterwert synchron
    // halten (z.B. nach Preset-Wechsel oder Host-Automation).
    {
        juce::TextButton* polPosButtons[4] = { &polPos1Button, &polPos2Button, &polPos3Button, &polPos4Button };
        int currentPos = juce::jlimit (0, 3, (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_POL_POS)->load()));
        // Alte Presets koennen noch auf den gestrichenen Positionen stehen.
        // 0 (vor Galaxy) wird zu EARLY, 3 (hinter Vision) zu LATE - der
        // Parameter wird dabei wirklich umgeschrieben, sonst leuchtet gar
        // kein Knopf und der Zustand waere unsichtbar.
        if (currentPos == 0 || currentPos == 3)
        {
            currentPos = (currentPos == 0) ? 1 : 2;
            if (auto* param = processor.apvts.getParameter (LCRMSAudioProcessor::ID_POL_POS))
                param->setValueNotifyingHost ((float) currentPos / 3.0f);
        }
        // Leuchtet nur, wenn ueberhaupt ein Kanal umgepolt ist (User) - sonst
        // sieht es so aus, als wuerde etwas passieren. Die Position bleibt
        // trotzdem gespeichert.
        const bool anyFlip = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_POL_L)->load() > 0.5f
                          || processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_POL_R)->load() > 0.5f;
        // Ein Knopf (Runde 39): Text = aktuelle Position, leuchtet bei Flip.
        juce::ignoreUnused (polPosButtons);
        const juce::String posText = technicalLabels ? ((currentPos == 2) ? "POST" : "PRE")
                                                    : ((currentPos == 2) ? "LATE" : "EARLY");
        if (polPos2Button.getButtonText() != posText)
            polPos2Button.setButtonText (posText);
        {
            const int pp = (currentPos == 2) ? 1 : 0;
            if ((int) polPos2Button.getProperties().getWithDefault ("ppDiagram", -1) != pp)
            {
                polPos2Button.getProperties().set ("ppDiagram", pp);
                polPos2Button.repaint();
            }
            const int want = (int) themePalette().frameRaye.getARGB();
            if ((int) polPos2Button.getProperties().getWithDefault ("pillColour", 0) != want)
            {
                polPos2Button.getProperties().set ("pillColour", want);
                polPos2Button.repaint();
            }
            if (polPosDots.index != pp) { polPosDots.index = pp; polPosDots.repaint(); }
        }
        if (polPos2Button.getToggleState() != anyFlip)
            polPos2Button.setToggleState (anyFlip, juce::dontSendNotification);
    }

    // Live-Position fuer den Flow-Ring (leuchtender Punkt) aktualisieren.
    {
        const float livePos = processor.currentPanPos.load (std::memory_order_relaxed);
        const float oldPos  = movementSlider.getProperties().getWithDefault ("movementLivePos", 0.0f);
        if (std::abs (livePos - oldPos) > 1.0e-4f)   // Review 1.0.1: nur bei Bewegung neu zeichnen
        {
            movementSlider.getProperties().set ("movementLivePos", livePos);
            movementSlider.repaint();
        }
    }

    // Mono-Check-Icon kontinuierlich neu zeichnen, solange es aktiv ist -
    // fuer das sanfte Puls-Blinken (siehe CustomLookAndFeel::drawMonoIcon).
    // 20Hz reicht fuer einen ruhigen, weichen Puls voellig aus und kostet
    // praktisch nichts (nur ein einzelner kleiner Button).
    if (monoCheckButton.getToggleState())
        monoCheckButton.repaint();

    // Mono-Dry-Button: nur bedienbar, waehrend Mono-Check selbst an ist
    // (User-Feedback: "soll nicht alleine gehen"); und wie das Bypass-Icon
    // gedimmt, solange er inaktiv/deaktiviert ist.
    {
        const bool monoOn = monoCheckButton.getToggleState();
        if (monoDryButton.isEnabled() != monoOn)
        {
            monoDryButton.setEnabled (monoOn);
            monoDryButton.repaint();
        }
        // Das eigene "DRY"-Label mitdimmen, damit Icon und Beschriftung nicht
        // auseinanderlaufen. Runde 174: im Bypass dimmen alle Beschriftungen
        // der Fusszeile mit.
        const float footerA = processor.isBypassedNow() ? 0.40f : 1.0f;
        auto setA = [] (juce::Component& c, float a) { if (std::abs (c.getAlpha() - a) > 0.001f) c.setAlpha (a); };
        setA (monoDryLabel, (monoOn ? 1.0f : 0.35f) * footerA);
        for (auto* l : { &monoCheckLabel, &mixLabel, &panLabel, &volLabel, &autoGainLabel })
            setA (*l, footerA);
        if (monoDryButton.getToggleState())
            monoDryButton.repaint();
    }

    // Solo-Icons sollen wie Mono-/Mod-Icon sanft blinken, solange aktiv
    // (User-Feedback). Zusaetzlich soll der GESAMTE Rahmen der soloten
    // Sektion mitblinken, nicht nur der Button (User-Feedback) - dafuer
    // reicht ein Repaint von content, solange irgendein Solo aktiv ist
    // (drawGroup() berechnet die Puls-Alpha selbst, siehe paintContent()).
    {
        juce::TextButton* soloButtons[6] = { &lcrSoloButton, &driftSoloButton, &polSoloButton,
                                              &widthBoostSoloButton, &flowSoloButton, &posSoloButton };
        for (auto* b : soloButtons)
            if (b->getToggleState())
                b->repaint();
        if (soloActive)
            content.repaint();
    }

    // Mod-Icons ebenso kontinuierlich neu zeichnen, solange sie aktiv sind
    // (sanftes Puls-Alpha, siehe CustomLookAndFeel::drawModIcon).
    if (driftModButton.getToggleState())      driftModButton.repaint();
    if (dimensionModButton.getToggleState())  dimensionModButton.repaint();
    if (hyperdriveModButton.getToggleState() && hyperdriveModButton.isEnabled())
        hyperdriveModButton.repaint();
    if (galaxyModButton.getToggleState())     galaxyModButton.repaint();
    if (positionModButton.getToggleState())   positionModButton.repaint();

    // Globaler Mod-Bypass (Sinuswelle pulsiert nur, solange Mod global
    // laeuft) und Galaxy-Button (Glow pulsiert, solange aktiv) - gleiches
    // Prinzip wie die Mod-Icons oben. Breathe "atmet" dagegen IMMER, egal
    // ob gerade geklickt wurde oder nicht (reine Aktion, kein Zustand).
    if (! globalModBypassButton.getToggleState())
        globalModBypassButton.repaint();
    if (globalGalaxyActivateButton.getToggleState())
        globalGalaxyActivateButton.repaint();
    globalBreatheButton.repaint();
    // Mutate pulsiert ebenfalls staendig (siehe drawMutateContent) - beide
    // Varianten, sonst wuerde die zweite Taste nach einem Klick auf ihrem
    // alten Farbzustand stehen bleiben.
    globalChaosButton.repaint();
    globalChaosSectionsButton.repaint();

    // PRISM-Leiste: Aktiv-Zustand nachfuehren und neu zeichnen, wenn sich die
    // Bandgrenzen geaendert haben (z.B. per Automation oder Preset-Wechsel -
    // beim Ziehen zeichnet die Komponente sich selbst neu).
    {
        const bool prismOn = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_PRISM_ON)->load() > 0.5f;
        prismBand.setActive (prismOn);
        const float lo = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_PRISM_LO)->load();
        const float hi = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_PRISM_HI)->load();
        if (std::abs (lo - lastPrismLo) > 0.5f || std::abs (hi - lastPrismHi) > 0.5f)
        {
            lastPrismLo = lo;
            lastPrismHi = hi;
            prismBand.repaint();
        }
    }

    // Preset-Anzeige: alle 10 Frames (also rund 3x pro Sekunde) pruefen, ob
    // sich seit dem Laden/Speichern etwas geaendert hat. Schnell genug, dass
    // der Stern gefuehlt sofort erscheint, und selten genug, dass die rund 40
    // Parameterabfragen nicht auffallen.
    // Hidden Egg, zweiter Ausloeser: einmal pro Fenster nach etwa acht
    // Minuten Betrieb - fuer alle, die Mutate nie benutzen.
    if (! eggFiredOnOpen && ++eggOpenTicks >= 30 * 60 * 8)
    {
        eggFiredOnOpen = true;
        goniometer.triggerEasterEgg();
    }

    // Alle 3 Frames (10x/s): der Copy-Pfeil soll schon WAEHREND des Drehens
    // aufleuchten, nicht erst beim Loslassen (User).
    if (++presetDirtyTick >= 3)
    {
        presetDirtyTick = 0;
        // Hat ein fremder replaceState() (Host-Recall, verzoegerter Default)
        // das View-Kind verworfen, wieder anhaengen - sonst fehlt es beim
        // naechsten Speichern der Session.
        if (! processor.apvts.state.getChildWithName ("ViewSettings").isValid())
            storeViewSettingsInState();
        const bool dirtyNow = std::abs (computePresetSignature() - presetSignature) > 1.0e-5f;
        if (dirtyNow != presetDirty)
        {
            presetDirty = dirtyNow;
            refreshPresetNameDisplay();
        }

        // Copy leuchtet nur, wenn der Live-Zustand vom anderen A/B-Slot
        // abweicht - sonst gibt es nichts zu kopieren.
        const auto& other = abCurrentIsA ? abSlotB : abSlotA;
        const bool litNow = std::abs (signatureOfTree (processor.apvts.copyState()) - signatureOfTree (other)) > 1.0e-4f;
        if (litNow != abCopyLit)
        {
            abCopyLit = litNow;
            abCopyButton.getProperties().set ("abCopyLit", litNow);
            abCopyButton.repaint();
        }
        // Richtung des Pfeils folgt dem aktiven Buchstaben.
        abCopyButton.getProperties().set ("abCopyToRight", abCurrentIsA);
    }

    // BYP: kein eigenes ButtonAttachment (siehe Konstruktor-Kommentar) - der
    // sichtbare Toggle-Status wird hier aus dem GUI-seitigen Bypass
    // (processor.uiBypassed, auch per Logo-Klick schaltbar) nachgezogen,
    // damit beide Wege synchron aussehen.
    {
        const bool bypassedNow = processor.isBypassedNow();   // Runde 174: auch DAW-Bypass
        if (globalBypassButton.getToggleState() != bypassedNow)
        {
            globalBypassButton.setToggleState (bypassedNow, juce::dontSendNotification);
            globalBypassButton.repaint();
            needsRepaint = true;   // Logo (grau im Bypass) liegt in content
        }
    }

    // Undo/Redo-Snapshot-Entprellung (siehe scheduleUndoSnapshot()): erst
    // wenn seit der letzten Aenderung kUndoDebounceFrames Frames lang
    // NICHTS mehr passiert ist, gilt die "Geste" als abgeschlossen und
    // landet als EIN Schritt in der Historie.
    if (undoDebounceFramesLeft > 0)
    {
        if (--undoDebounceFramesLeft == 0)
            pushUndoSnapshotNow();
    }

    if (needsRepaint)
        content.repaint();
}

void LCRMSAudioProcessorEditor::drawLogo (juce::Graphics& g, juce::Rectangle<float> area)
{
    // Einfaches, modernes Icon: leuchtender Ring mit gekreuzter Achse -
    // angelehnt an das "Space"-Thema, ohne aufwendige Assets. Per Klick
    // schaltbarer GUI-Bypass: im bypassten Zustand wird das Logo neutral
    // grau statt farbig gezeichnet, als klare visuelle Rueckmeldung.
    const bool bypassed = processor.isBypassedNow();
    auto ringCol  = bypassed ? juce::Colour (0xff6a6e78) : lookAndFeel.accent;
    auto glowCol  = bypassed ? juce::Colour (0xff6a6e78) : lookAndFeel.glowAccent;

    auto centre = area.getCentre();
    float r = area.getHeight() * 0.5f * 0.72f;

    if (! bypassed)
    {
        for (int layer = 3; layer >= 1; --layer)
        {
            float rr = r * (1.0f + 0.22f * (float) layer);   // Runde 110: Hof ~30 % kleiner
            g.setColour (glowCol.withAlpha (0.05f * (float) (4 - layer)));
            g.fillEllipse (centre.x - rr, centre.y - rr, rr * 2.0f, rr * 2.0f);
        }
    }

    juce::Path ring;
    ring.addEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);
    g.setColour (ringCol);
    g.strokePath (ring, juce::PathStrokeType (2.0f));

    juce::Path cross;
    float d = r * 0.62f;
    cross.startNewSubPath (centre.x - d, centre.y - d * 0.5f);
    cross.lineTo (centre.x + d, centre.y + d * 0.5f);
    cross.startNewSubPath (centre.x - d, centre.y + d * 0.5f);
    cross.lineTo (centre.x + d, centre.y - d * 0.5f);
    g.setColour (bypassed ? juce::Colour (0xffaaadb5) : juce::Colours::white);
    g.strokePath (cross, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (glowCol);
    g.fillEllipse (centre.x - 2.5f, centre.y - 2.5f, 5.0f, 5.0f);
}

void LCRMSAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Nur ein flacher Hintergrund - der eigentliche Inhalt zeichnet sich in
    // paintContent() auf der skalierten content-Komponente. Falls das
    // Seitenverhaeltnis mal nicht exakt passt, ist der Rand hier gedeckt.
    g.fillAll (juce::Colour (0xff0e0f13));
}

void LCRMSAudioProcessorEditor::paintContent (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0e0f13));

    auto bounds = juce::Rectangle<float> (0, 0, (float) kDesignW, (float) kDesignH);
    g.setColour (themePalette().plate);
    g.fillRoundedRectangle (bounds.reduced (8.0f), 10.0f);
    drawThemePlate (g, bounds.reduced (8.0f), 10.0f);   // Wasserfarbe/Comic: Textur ueber der Platte

    // Titelzeile jetzt mit demselben aeusseren Randabstand wie der Rest der
    // GUI (kOuterMargin) statt bei (0,0) zu kleben - vorher wirkte der Logo-
    // Bereich im Vergleich zu allen anderen Elementen zu eng am Rand
    // platziert (User-Feedback "sieht nicht gut aus, Randabstand").
    auto titleBar = juce::Rectangle<int> (kOuterMargin, kOuterMargin, kDesignW - kOuterMargin * 2, kTitleBarH);
    // Pop: der Kopf bekommt denselben Kasten wie der Footer - ohne ihn wirkt
    // er "draufgesetzt" (User). MUSS vor dem Logo gezeichnet werden, sonst
    // liegt der Kasten darueber.
    if (isComicTheme())
    {
        auto hb = juce::Rectangle<int> (kOuterMargin, kOuterMargin,
                                        kDesignW - kOuterMargin * 2, kTitleBarH).expanded (6, 6).toFloat();
        g.setColour (comicInk());
        g.fillRoundedRectangle (hb.translated (4.0f, 4.0f), 12.0f);
        g.setColour (juce::Colour (0xff2a2450));
        g.fillRoundedRectangle (hb, 12.0f);
        g.setColour (comicInk());
        g.drawRoundedRectangle (hb, 12.0f, 3.0f);
    }

    auto logoArea = titleBar.removeFromLeft (kTitleBarH).reduced (5).toFloat();
    drawLogo (g, logoArea);

    // Runde 111 (User: "davor standen die Profile in einem Rahmen und waren
    // dadurch staerker hervorgehoben - jetzt schweben sie im leeren Raum"):
    // eine weiche Kachel in derselben Sprache wie das Preset-Feld - leichte
    // Flaeche, ein Hauch Rand, keine harte Linie. Pop hat seinen Kopf-Kasten.
    // Runde 112 (User: Kachel "nicht gut"): statt einer Flaeche bekommt das
    // Profil die Sprache der Kopfzeile - ein feiner senkrechter Trennstrich
    // wie zwischen den Knopf-Gruppen, ueber beide Zeilen. Damit ist es eine
    // eigene Gruppe, ohne Kasten.
    // Runde 115 (User): der Strich ist wieder weg - das Profil steht jetzt
    // mittig zwischen Wortmarke und Kopfzeile und braucht ihn nicht.

    // Runde 126 (User: "rechts und links um die Smart-Profile ein Glow, da
    // ist ja noch viel Platz"): ein flacher, breiter Lichtschein hinter dem
    // Icon und zwei feine Lichtlinien nach links und rechts, die nach aussen
    // auslaufen - wie ein Horizont, auf dem das Profil sitzt. Mit gewaehltem
    // Profil kraeftiger. Rein statisch, kostet nichts.
    if (categoryButton.isVisible())
    {
        const auto cb    = categoryButton.getBounds().toFloat();
        const float cx   = cb.getCentreX();
        const float iy   = cb.getY() + cb.getHeight() * 0.34f;   // Hoehe der Icon-Mitte
        const bool armed = mutateCategoryValue > 0;
        // Runde 134: dieselbe Farbe wie das Profil-Icon (in Sci-Fi cyan statt pink).
        const auto col   = themePalette().knob;
        // Platz links bis zum Slogan-Ende, rechts bis zum Preset-Pfeil.
        const float room = juce::jmin (cx - 300.0f, (float) presetPrevButton.getX() - cx - 8.0f);
        const float reach = juce::jlimit (60.0f, 170.0f, room);

        // (1) weicher, flacher Schein - Ellipse ueber eine gestauchte Kreisfuellung
        {
            juce::Graphics::ScopedSaveState keep (g);
            const float hw = reach, hh = 30.0f;
            g.addTransform (juce::AffineTransform::scale (1.0f, hh / hw, cx, iy));
            juce::ColourGradient halo (col.withAlpha (armed ? 0.13f : 0.06f), cx, iy,
                                       col.withAlpha (0.0f), cx + hw, iy, true);
            halo.addColour (0.45, col.withAlpha (armed ? 0.05f : 0.022f));
            g.setGradientFill (halo);
            g.fillEllipse (cx - hw, iy - hw, hw * 2.0f, hw * 2.0f);
        }
        // (2) Runde 146 (User: Variante B "Orbit" testen): eine feine, leicht
        // gekippte Umlaufbahn um das Icon, auf der ein kleiner Planet in ~14 s
        // kreist - vorne hell, hinten gedimmt, dadurch raeumlich. Der Timer
        // zeichnet nur diesen Bereich neu (profileStarsArea).
        {
            const float iconH = cb.getHeight() * 0.64f;
            const float iconW = juce::jmin (cb.getWidth(), iconH * 1.5f);
            const float rx = iconW * 0.66f, ry = iconH * 0.24f;
            const float tilt = juce::degreesToRadians (-10.0f);
            const auto  rot  = juce::AffineTransform::rotation (tilt, cx, iy);
            // Runde 147 (User): ohne Profil leuchtet die Bahn beim Drueberfahren
            // auf - zeigt, dass man hier klicken kann.
            const bool  hot  = ! armed && (categoryButton.isMouseOver (true) || catDots.isMouseOver (true));
            const float lvl  = armed ? 1.0f : hot ? 0.75f : 0.35f;

            juce::Path orbit;
            orbit.addEllipse (cx - rx, iy - ry, rx * 2.0f, ry * 2.0f);
            orbit.applyTransform (rot);
            g.setColour (col.withAlpha (0.28f * lvl + (armed ? 0.0f : 0.01f)));
            g.strokePath (orbit, juce::PathStrokeType (1.1f));

            const double tSec = juce::Time::getMillisecondCounterHiRes() * 0.001;
            const float th = (float) (juce::MathConstants<double>::twoPi * std::fmod (tSec, 14.0) / 14.0);
            juce::Point<float> pl (cx + rx * std::cos (th), iy + ry * std::sin (th));
            pl.applyTransform (rot);
            const float front = 0.5f + 0.5f * std::sin (th);            // unten = vorne
            const float a  = lvl * (0.35f + 0.65f * front);
            const float pr = 2.0f + 0.6f * front;
            juce::ColourGradient pg (col.withAlpha (a * 0.45f), pl.x, pl.y, col.withAlpha (0.0f), pl.x + pr * 3.2f, pl.y, true);
            g.setGradientFill (pg);
            g.fillEllipse (pl.x - pr * 3.2f, pl.y - pr * 3.2f, pr * 6.4f, pr * 6.4f);
            g.setColour (col.withAlpha (a));
            g.fillEllipse (pl.x - pr, pl.y - pr, pr * 2.0f, pr * 2.0f);

            profileStarsArea = orbit.getBounds().expanded (10.0f).getSmallestIntegerContainer();
        }
    }
    else
    {
        profileStarsArea = {};
    }

    // Nur noch der reine Wortmark, vertikal zentriert im Titelbalken - der
    // Claim-Untertitel wirkte "amateurhaft" (User-Feedback) und wurde
    // entfernt.
    // Etwas mehr Luft zwischen Logo und Wortmarke (User-Wunsch).
    auto textArea = titleBar.reduced (8, 0).withTrimmedLeft (10);
    constexpr int titleLineH = 30;
    // Nicht mehr mittig zentriert, sondern auf die zwei Zeilen des rechten
    // Blocks ausgerichtet: Wortmarke auf Hoehe der Aktionszeile, Slogan auf
    // Hoehe der Preset-Zeile (siehe layoutContent(), kRowH/kRowGap).
    textArea.removeFromTop (6);
    textArea.removeFromBottom (5);

    auto titleLine = textArea.removeFromTop (titleLineH);
    // ===== WORTMARK =====
    // User-Wunsch: "Logo Space X und Slogan noch ein bisschen interessanter -
    // das Logo an sich ist gut, aber ich denke da geht noch ein bisschen
    // mehr. Ich will es aber nicht komplett in einen anderen Style."
    //
    // Deshalb bewusst KEIN neuer Stil: gleiche Schrift, gleiche Groesse,
    // gleiche Position. Nur drei Feinheiten, die einen flachen Schriftzug in
    // einen gesetzten verwandeln:
    //
    // 1) Ein weicher Schein dahinter (dieselbe Farbe wie die Akzente im
    //    Feld). Dadurch sitzt die Wortmark nicht mehr "auf" dem Hintergrund,
    //    sondern leuchtet aus ihm heraus - dasselbe Prinzip, das die aktiven
    //    Buttons schon benutzen, hier nur sehr viel schwaecher dosiert.
    // 2) Ein senkrechter Verlauf von Weiss nach kuehlem Grau. Reines Weiss
    //    ueber die ganze Hoehe ist der haeufigste Grund, warum ein Logo
    //    "gedruckt" statt beleuchtet wirkt.
    // 3) "X" in der Akzentfarbe. Der eine hervorgehobene Buchstabe ist das,
    //    was aus einem Schriftzug eine Marke macht - und er greift genau die
    //    Farbe auf, die im Sternenfeld darunter ohnehin dominiert.
    {
        // Wortmarke insgesamt etwas groesser (User), und das "X" als eigene,
        // deutlich groessere Type gesetzt: rund ein Drittel groesser, dafuer
        // naeher an "SPACE" herangerueckt und an derselben Grundlinienmitte
        // ausgerichtet - so liest es sich als eine Marke, nicht als zwei Woerter.
        const juce::Font markFont = juce::Font (juce::FontOptions (27.0f, juce::Font::bold))
                                        .withExtraKerningFactor (0.08f);
        const juce::Font xFont    = juce::Font (juce::FontOptions (48.0f, juce::Font::bold))
                                        .withExtraKerningFactor (0.0f);
        g.setFont (markFont);

        const juce::String wordSpace ("SPACE");
        const juce::String wordX ("X");
        const int spaceW = juce::GlyphArrangement::getStringWidthInt (markFont, wordSpace);

        auto markLine = titleLine;

        // (1) Schein - komplett entfernt (User: "Glow komplett entfernen").

        // (2) "SPACE" mit senkrechtem Verlauf
        juce::ColourGradient markGrad (juce::Colours::white, 0.0f, (float) markLine.getY(),
                                        juce::Colour (0xffb9c2d0), 0.0f, (float) markLine.getBottom(), false);
        g.setGradientFill (markGrad);
        g.drawText (wordSpace, markLine, juce::Justification::centredLeft);

        // (3) "X" in der Akzentfarbe, groesser und mit knappem Abstand.
        g.setFont (xFont);
        // Deutlich groesser und so nah heran, dass es leicht unter das "E"
        // schiebt (User: "kann sogar ueberlappen") - das X ist die Marke,
        // nicht der zweite Teil eines Wortes. expanded() gibt der grossen
        // Type die Hoehe, die die Titelzeile allein nicht hergibt.
        auto xLine = markLine.withTrimmedLeft (spaceW + 1).expanded (0, 14);
        // Runde 110 (User): das X haengt an der Theme-Farbe - vorher war sein
        // Violett die einzige Stelle mit dieser Farbe im ganzen Theme.
        const auto xBase = pairAccentColour();
        juce::ColourGradient xGrad (xBase.interpolatedWith (juce::Colours::white, 0.40f), 0.0f, (float) xLine.getY(),
                                     xBase, 0.0f, (float) xLine.getBottom(), false);
        g.setGradientFill (xGrad);
        g.drawText (wordX, xLine, juce::Justification::centredLeft);

        // Runde 174 (User): BYPASS-Plakette oben im Header, rechts neben der
        // Wortmarke - gut sichtbar und nie im Weg der DEMO-Plakette, die eine
        // Zeile tiefer neben dem Slogan steht.
        if (processor.isBypassedNow())
        {
            const auto f = juce::Font (juce::FontOptions (11.0f, juce::Font::bold)).withExtraKerningFactor (0.18f);
            const float xW    = juce::GlyphArrangement::getStringWidth (xFont, wordX);
            const float chipW = juce::GlyphArrangement::getStringWidth (f, "BYPASS") + 18.0f;
            const juce::Rectangle<float> chip ((float) markLine.getX() + (float) spaceW + 1.0f + xW + 18.0f,
                                               (float) markLine.getCentreY() - 9.0f, chipW, 18.0f);
            const auto acc = themePalette().knob;
            g.setColour (acc.withAlpha (0.16f));
            g.fillRoundedRectangle (chip, 9.0f);
            g.setColour (acc.withAlpha (0.80f));
            g.drawRoundedRectangle (chip.reduced (0.5f), 9.0f, 1.0f);
            g.setFont (f);
            g.setColour (acc.brighter (0.35f));
            g.drawText ("BYPASS", chip, juce::Justification::centred, false);
        }
    }

    // Slogan unter dem Logo (User-Wunsch: "Unter dem Logo ist noch Platz ...
    // hier Slogan 'Spatial Imaging Manipulation'") - nutzt den Freiraum
    // zwischen der Wortmark und der unteren Kante der Titelzeile. Glossy
    // Blau/Tuerkis/Lila-Farbverlauf statt einer flachen Farbe (User-Wunsch:
    // "Mischung aus den Farben blau, tuerkis, lila, modern, glossy"). Rein
    // statisch (kein Timer/Animation) - bleibt CPU-guenstig.
    auto sloganLine = textArea;
    if (sloganLine.getHeight() > 4)
    {
        juce::ColourGradient sloganGrad (juce::Colour (0xff5be3ff), (float) sloganLine.getX(), (float) sloganLine.getCentreY(),
                                          juce::Colour (0xffb26bff), (float) sloganLine.getRight(), (float) sloganLine.getCentreY(), false);
        sloganGrad.addColour (0.5, juce::Colour (0xff33d6c0));

        // Groesser gesetzt (User-Feedback: "Slogan Schrift groesser") und ohne
        // den Auftakt-Punkt ("Slogan darunter ohne das Icon"). Der Punkt war
        // als Satzdetail gedacht, hat aber eine zweite, konkurrierende Form
        // neben das Logo gestellt - und das Logo links ist bereits ein Kreis.
        //
        // Die Breite dafuer ist jetzt da: die globale Button-Zeile rechts ist
        // durch den Auszug der Preset-Bedienung (siehe Preset-Leiste) von rund
        // 690px auf rund 410px geschrumpft.
        // Runde 110 (User): der Slogan begleitet, statt zu rufen - das Cyan
        // kam sonst nirgends vor. Theme-Farbe gedaempft, Sperrung etwas enger.
        juce::ignoreUnused (sloganGrad);
        const juce::Font sloganFont = juce::Font (juce::FontOptions (13.8f, juce::Font::bold))
                                          .withExtraKerningFactor (0.16f);
        g.setFont (sloganFont);
        g.setColour (themePalette().knob.withAlpha (0.72f));
        // Slogan aus Pauls Auswahl. "Spatial Intelligence" transportiert das
        // "smart" ohne das Wort selbst zu benutzen - damit kollidiert es nicht
        // mit den Smart-Knoepfen im Header, und es kollidiert auch nicht mit
        // den Sektionsnamen (Dimension). Aendern ist eine Zeile.
        g.drawText ("SPATIAL INTELLIGENCE", sloganLine, juce::Justification::centredLeft);

        // DEMO-Plakette. Steht bewusst klein neben dem Slogan und nicht als
        // Banner ueber der GUI: das Plugin soll sich im Demo-Modus wie das
        // fertige Produkt anfuehlen, nur eben alle 50 Sekunden kurz leise.
        if (! processor.licensed.load (std::memory_order_relaxed))
        {
            auto f = juce::Font (juce::FontOptions (11.0f, juce::Font::bold)).withExtraKerningFactor (0.18f);
            const float sloganW = juce::GlyphArrangement::getStringWidth (sloganFont, "SPATIAL INTELLIGENCE");
            const float chipW   = juce::GlyphArrangement::getStringWidth (f, "DEMO") + 16.0f;
            juce::Rectangle<float> chip ((float) sloganLine.getX() + sloganW + 14.0f,
                                         (float) sloganLine.getCentreY() - 8.5f, chipW, 17.0f);
            demoChipArea = chip;   // klickbar (siehe mouseUp) und leuchtet waehrend der Absenkung
            g.setColour (themePalette().frameRaye.withAlpha (0.18f));
            g.fillRoundedRectangle (chip, 8.0f);
            g.setColour (themePalette().frameRaye.withAlpha (0.75f));
            g.drawRoundedRectangle (chip.reduced (0.5f), 8.0f, 1.0f);
            g.setFont (f);
            g.drawText ("DEMO", chip, juce::Justification::centred, false);
        }
    }

    // Kleine vertikale Trennstriche in der globalen Button-Zeile (User-
    // Wunsch: neue Reihenfolge mit Gruppen-Trennern), Positionen kommen aus
    // layoutContent().
    // Runde 157: die Linien laufen durch beide Kopfzeilen und blenden oben
    // und unten aus (wie im Footer).
    for (auto x : globalRowSeparatorX)
    {
        const float fx = (float) x + 0.5f, y1 = (float) globalRowSeparatorTop, y2 = (float) globalRowSeparatorBottom;
        juce::ColourGradient grad (juce::Colours::white.withAlpha (0.0f), fx, y1,
                                   juce::Colours::white.withAlpha (0.0f), fx, y2, false);
        grad.addColour (0.5, juce::Colours::white.withAlpha (0.16f));
        g.setGradientFill (grad);
        g.fillRect (fx - 0.5f, y1, 1.0f, y2 - y1);
    }
    g.setColour (juce::Colours::white.withAlpha (0.14f));

    // Trennstriche der zweiten Titelzeile (Preset-Zeile) - gleiche Optik
    // wie in der ersten, damit die beiden Zeilen als EIN Block gelesen werden.
    for (auto x : presetRowSeparatorX)
        g.drawLine ((float) x, (float) presetRowSeparatorTop, (float) x, (float) presetRowSeparatorBottom, 1.0f);

    // Dezente Gruppierungs-Rahmen: zeigen zusammengehoerige Regler, ohne
    // dominant zu wirken. Wenn die Sektion per Power-Icon deaktiviert ist,
    // wird der GESAMTE Kasten (nicht nur die einzelnen Regler) ausgegraut,
    // damit sofort klar ist, dass die Sektion gerade nicht wirkt.
    // "blink" = true fuer die aktuell solote Sektion - blendet dieselbe
    // sanfte Puls-Alpha wie die Icons (Solo/Mono/Mod) auch auf die
    // Fuell-/Linienfarbe des GESAMTEN Rahmens ein (User-Feedback: "soll der
    // ganze Rahmen blinken, nicht nur der Button").
    // Runde 174 (User): im Bypass sehen alle Sektionen aus wie ausgeschaltet.
    const bool bypassFrames = processor.isBypassedNow();
    auto drawGroup = [&] (juce::Rectangle<int> r, juce::Colour c, bool on, bool blink, float strokeWidth = 2.8f)
    {
        if (r.isEmpty()) return;
        on = on && ! bypassFrames;
        auto rf = r.toFloat().reduced (3.0f);
        auto col = on ? c : juce::Colour (0xff545862);
        // Sehr dezenter Aussen-Glow NUR bei eingeschalteter Sektion (User:
        // "noch eine simple Loesung: minimal mehr Glow, wenn section=on").
        // Pop hat seinen eigenen Look und bleibt aussen vor.
        auto onGlow = [&] (float corner)
        {
            if (! on || isComicTheme()) return;
            for (int layer = 3; layer >= 1; --layer)
            {
                const float expand = 1.5f + 2.5f * (float) layer;
                g.setColour (c.withAlpha (0.026f * (float) (4 - layer)));
                g.drawRoundedRectangle (rf.expanded (expand), corner + expand, 2.0f);
            }
        };
        // Deckkraft der Fuellflaeche bei aktiver Sektion reduziert (vorher
        // 0.07f wirkte durch die satten Akzentfarben zu praesent/deckend -
        // User-Feedback). Die Rahmenlinie selbst (lineA) bleibt unveraendert.
        float pulse = 1.0f;
        if (blink)
        {
            const double t = juce::Time::getMillisecondCounterHiRes() * 0.001;
            constexpr double periodSeconds = 2.2;
            pulse = 0.55f + 0.45f * (float) (0.5 + 0.5 * std::sin (juce::MathConstants<double>::twoPi * t / periodSeconds));
        }
        // Frameless (User: "eine ganz leichte Trennung, subtil, aber ohne harte
        // Frames"): keine Kaesten, keine Linien - nur ein minimaler
        // Helligkeitsversatz der EINGESCHALTETEN Sektion gegenueber der Platte
        // und eine hauchduenne Lichtkante oben. Das Auge gruppiert trotzdem,
        // aber nichts davon zieht Aufmerksamkeit auf sich.
        // Outline (User-Idee): die Umkehrung von Flat - ein leichter Rahmen,
        // aber praktisch keine Fuellung (rund 2 % Unterschied zur UI).
        if (layoutOutline())
        {
            // Runde 66 (User): die klare Kontur bleibt - sie ist das Moderne
            // daran. Dazu kommt eine DEZENTE Fuellung (rund ein Drittel der
            // 3D-Deckkraft): ohne sie hat die Sektion zwar eine Kante, aber
            // keine Flaeche, und ein ausgeschalteter Rahmen loest sich im
            // Hintergrund auf. Kein Glow - der gehoerte zu 3D.
            if (on)
            {
                const auto of = outlineFill();
                g.setColour (of.colour.withAlpha (of.alpha * pulse));
                g.fillRoundedRectangle (rf, 10.0f);
                // Ein Hauch Sektionsfarbe von links oben, viel schwaecher als
                // in 3D - gibt der Flaeche Richtung, ohne sie einzufaerben.
                juce::ColourGradient wash (col.withAlpha (0.055f * pulse), rf.getX() + rf.getWidth() * 0.15f, rf.getY(),
                                           col.withAlpha (0.0f), rf.getX() + rf.getWidth() * 0.9f, rf.getY(), true);
                g.setGradientFill (wash);
                g.fillRoundedRectangle (rf, 10.0f);
            }
            g.setColour (col.withAlpha ((on ? 0.40f : 0.28f) * pulse));   // Runde 174: aus 0.10 -> 0.28, an 0.38 -> 0.40
            g.drawRoundedRectangle (rf, 10.0f, on ? 1.4f : 1.0f);
            return;
        }
        if (layoutFrameless())
        {
            if (on)
            {
                g.setColour (juce::Colours::white.withAlpha (0.020f * pulse));
                g.fillRoundedRectangle (rf, 12.0f);
                juce::ColourGradient top (juce::Colours::white.withAlpha (0.034f * pulse), rf.getCentreX(), rf.getY(),
                                          juce::Colours::white.withAlpha (0.0f),           rf.getCentreX(), rf.getY() + rf.getHeight() * 0.55f, false);
                g.setGradientFill (top);
                g.fillRoundedRectangle (rf, 12.0f);
            }
            return;
        }

        // ---- Theme-Varianten der Sektionsflaeche ----
        if (usesWashSections())
        {
            // Dark-Night-Stil (User: "sieht so geil aus" - auch fuer Modern und
            // Day & Night): sehr dunkle Flaeche ohne Korn, weicher Farbhauch der
            // Sektion oben links, keine harte Rahmenlinie - nur ein 1-px-
            // Schimmer plus zwei weiche Saum-Striche. Farben je Theme: washFill().
            const auto wf = washFill();
            // Aus-Sektion: KEINE Fuellung mehr - die Platte scheint unveraendert
            // durch (User: "exakt dieselbe Farbe wie die UI an der Stelle").
            onGlow (10.0f);
            if (on)
            {
                g.setColour (wf.on.withAlpha (wf.aOn));
                g.fillRoundedRectangle (rf, 10.0f);
            }
            if (on)
            {
                juce::ColourGradient wash (col.withAlpha (0.14f * pulse), rf.getX() + rf.getWidth() * 0.15f, rf.getY(),
                                           col.withAlpha (0.0f), rf.getX() + rf.getWidth() * 0.15f + rf.getWidth() * 0.75f, rf.getY(), true);
                g.setGradientFill (wash);
                g.fillRoundedRectangle (rf, 10.0f);
            }
            // Aus: nur noch ein minimaler Rand, damit man sieht, DASS dort eine
            // Sektion liegt - sonst nichts (User).
            // Runde 174 (User): der Aus-Rahmen soll aussehen wie frueher im
            // DAW-Bypass - Schimmer fast wie "an" und ein Hauch der beiden
            // Saum-Striche. Fuellung, Farbhauch und Glow bleiben "an".
            g.setColour (juce::Colours::white.withAlpha (on ? 0.096f : 0.085f));   // Runde 174c: an +6 %
            g.drawRoundedRectangle (rf, 10.0f, 1.0f);
            if (! on)
            {
                g.setColour (col.withAlpha (0.04f));
                g.drawRoundedRectangle (rf.expanded (1.0f), 11.0f, 1.5f);
                g.drawRoundedRectangle (rf.expanded (2.0f), 12.0f, 1.5f);
            }
            // Moon (User: "Kontrast zwischen on und off ist zu gering, und die
            // UI ist schon dunkel genug"): der Rahmen der EINGESCHALTETEN
            // Sektion bekommt zusaetzlich einen klaren Zug Akzentfarbe - das
            // ist der einzige Hebel, der uebrig bleibt, ohne alles abzudunkeln.
            if (isMoonTheme() && on)
            {
                g.setColour (col.withAlpha (0.36f * pulse));   // Runde 174c: +6 %
                g.drawRoundedRectangle (rf, 10.0f, 1.3f);
            }
            if (on)
            {
                g.setColour (col.withAlpha (0.053f * pulse));   // Runde 174c: +6 %
                g.drawRoundedRectangle (rf.expanded (1.0f), 11.0f, 1.5f);
                g.drawRoundedRectangle (rf.expanded (2.0f), 12.0f, 1.5f);
            }
            return;
        }
        if (false)
        {
            g.setColour ((on ? juce::Colour (0xff1e2230) : juce::Colour (0xff0b0d14)).withAlpha (on ? 0.66f : 0.92f));
            g.fillRoundedRectangle (rf, 10.0f);
            {
                juce::ColourGradient light (juce::Colours::white.withAlpha (on ? 0.045f : 0.012f), rf.getX(), rf.getY(),
                                            juce::Colours::black.withAlpha (0.06f), rf.getRight(), rf.getBottom(), false);
                g.setGradientFill (light);
                g.fillRoundedRectangle (rf, 10.0f);
            }
            g.setColour (col.withAlpha ((on ? 0.05f : 0.02f) * pulse));
            g.fillRoundedRectangle (rf, 10.0f);
            if (grainTile.isValid())
            {
                g.saveState();
                juce::Path clip; clip.addRoundedRectangle (rf, 10.0f);
                g.reduceClipRegion (clip);
                g.setTiledImageFill (grainTile, 0, 0, 1.0f);
                g.fillRect (rf);
                g.restoreState();
            }
            // Rahmen: weicher Kontrast, Verlauf oben-links heller, mit Korn.
            {
                const float wFrame = strokeWidth * 0.8f + 1.5f;   // 1-2 px dicker, damit die Textur im Rahmen sichtbar wird (User)
                juce::ColourGradient edge (col.withAlpha ((on ? 0.36f : 0.07f) * pulse), rf.getX(), rf.getY(),
                                           col.withAlpha ((on ? 0.22f : 0.04f) * pulse), rf.getRight(), rf.getBottom(), false);
                g.setGradientFill (edge);
                g.drawRoundedRectangle (rf, 10.0f, wFrame);
                if (grainTile.isValid())
                {
                    juce::Path frame; frame.addRoundedRectangle (rf, 10.0f);
                    juce::Path stroked;
                    juce::PathStrokeType (wFrame).createStrokedPath (stroked, frame);
                    g.saveState();
                    g.reduceClipRegion (stroked);
                    g.setTiledImageFill (grainTile, 0, 0, 1.0f);
                    g.fillRect (rf.expanded (3.0f));
                    g.restoreState();
                }
            }
            return;
        }
        if (isComicTheme())
        {
            // Konturen, Versatz-Schatten, satte Flaeche in Sektionsfarbe.
            g.setColour (comicInk());
            g.fillRoundedRectangle (rf.translated (5.0f, 5.0f), 12.0f);
            g.setColour ((on ? juce::Colour (0xff1e1a3a) : juce::Colour (0xff14112a)).interpolatedWith (col, (on ? 0.34f : 0.05f) * pulse));   // aus: dunkler, kaum Farbe (User)
            g.fillRoundedRectangle (rf, 12.0f);
            g.setColour (comicInk());
            g.drawRoundedRectangle (rf, 12.0f, 3.0f);
            return;
        }
        if (usesFlatPanels())
        {
            // Sci-Fi / Moon: ruhige, gefuellte Flaeche (aus dem "subtil"-
            // Mockup, User), Rahmen nur als feiner Schimmer - "weniger ist mehr".
            // Sci-Fi / Flat: ruhige, gefuellte Flaeche, Rahmen als feiner
            // Schimmer. Der Aus-Zustand ist jetzt deutlich dunkler und der
            // Rahmen dort fast weg (User: bei Flat war on/off am schwersten
            // zu unterscheiden); dafuer bekommt die EINGESCHALTETE Sektion
            // einen kraeftigeren Rahmen und einen Hauch Glow.
            const juce::Colour surf (themeSurface());
            if (on && r != groupLcrArea) onGlow (10.0f);   // Galaxy hat seinen eigenen Glow
            // Aus-Sektion ohne Fuellung; Sci-Fi Dark verzichtet auch im
            // An-Zustand darauf (User-Idee: "quasi nur ein Frame, innen die
            // UI-Farbe").
            if (on)
            {
                // Sci-Fi: halbe Deckkraft - genau zwischen der frueheren
                // gefuellten Variante (0.62) und "gar keine Fuellung" der
                // Dark-Variante. Die beiden sind ein Theme geworden (User).
                g.setColour (surf.withAlpha (isSciFiTheme() ? 0.34f : 0.62f));
                g.fillRoundedRectangle (rf, 10.0f);
            }
            g.setColour (col.withAlpha ((on ? 0.45f : 0.32f) * pulse));   // Runde 174: aus 0.10 -> 0.32, an 0.42 -> 0.45
            g.drawRoundedRectangle (rf, 10.0f, on ? 1.6f : 1.0f);
            return;
        }
        // Runde 174 (User): der Aus-Rahmen war im Dunkeln kaum zu sehen - die
        // Sektionen liessen sich schlecht voneinander abgrenzen. Mittelweg:
        // deutlich heller als frueher (0.10), aber klar unter dem An-Rahmen;
        // Fuellung und Glow bleiben der eingeschalteten Sektion vorbehalten.
        const float lineA = (on ? 0.48f : 0.34f) * pulse;   // Runde 174c: an 0.45 -> 0.48 (User: "ein Ticken leuchtender")
        onGlow (10.0f);
        if (on)
        {
            g.setColour (col.withAlpha (0.035f * pulse));
            g.fillRoundedRectangle (rf, 10.0f);
        }
        g.setColour (col.withAlpha (lineA));
        // Nochmal einen Tick dicker (2.4 -> 2.8f, User-Feedback: "alle
        // Rahmen leicht dicker, aber nicht viel"); die beiden linken
        // Rahmen (Galaxy/Timewarp) bekommen ueber strokeWidth nochmal einen
        // Tick mehr (User-Feedback: "Links beide Rahmen dicker").
        g.drawRoundedRectangle (rf, 10.0f, strokeWidth);
    };

    // Aktuell solote Sektion (falls vorhanden) bekommt statt ihrer normalen
    // Sektionsfarbe einen auffaelligeren Amber-Rahmen (dieselbe Akzentfarbe
    // wie das aktive Solo-Icon selbst) - macht auf einen Blick sichtbar,
    // welche Sektion gerade solot (und damit dauerhaft an ist).
    const int soloState = juce::jlimit (0, LCRMSAudioProcessor::SOLO_MAX, (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_SOLO_SECTION)->load()));
    const juce::Colour soloAmber (0xffffb648);
    auto groupColour = [&] (int soloValue, juce::Colour normalCol)
    {
        return soloState == soloValue ? soloAmber : normalCol;
    };

    // Sektionsfarben: Pop behaelt seine bunten Rahmen, alle anderen Themes
    // eine Familie aus der Theme-Palette (Galaxy und RAYE bleiben eigen).
    const auto pal = themePalette();
    const bool perSection = isComicTheme();
    const juce::Colour cPurple = perSection ? juce::Colour (0xffb968ff) : pal.frameMain;
    const juce::Colour cGreen  = perSection ? juce::Colour (0xff5be3c7) : pal.frameMain;
    const juce::Colour cGalaxy = perSection ? pal.frameGalaxy : pal.frameMain;   // Galaxy ohne eigenen Rahmenton (User: hat den Glow)
    drawGroup (groupLcrArea,        groupColour (LCRMSAudioProcessor::SOLO_GALAXY,     cGalaxy),         lcrFrameOn,        soloState == LCRMSAudioProcessor::SOLO_GALAXY, 3.4f);
    drawGroup (groupDriftArea,      groupColour (LCRMSAudioProcessor::SOLO_TIMEWARP,   cPurple),         driftFrameOn,      soloState == LCRMSAudioProcessor::SOLO_TIMEWARP, 3.4f);
    drawGroup (groupPolArea,        groupColour (LCRMSAudioProcessor::SOLO_POLARITY,   cPurple),         polFrameOn,        soloState == LCRMSAudioProcessor::SOLO_POLARITY);
    drawGroup (groupWidthBoostArea, groupColour (LCRMSAudioProcessor::SOLO_DIMENSION,  cGreen),          widthBoostFrameOn, soloState == LCRMSAudioProcessor::SOLO_DIMENSION);
    drawGroup (groupFlowArea,       groupColour (LCRMSAudioProcessor::SOLO_HYPERDRIVE, cPurple),         flowFrameOn,       soloState == LCRMSAudioProcessor::SOLO_HYPERDRIVE);
    drawGroup (groupPosArea,        groupColour (LCRMSAudioProcessor::SOLO_POSITION,   cGreen),          posFrameOn,        soloState == LCRMSAudioProcessor::SOLO_POSITION);
    // Runde 110 (User): ein Rahmenstil fuer alle - Phaser hatte als einzige
    // Sektion einen goldenen Rahmen. Pop behaelt seine bunten Rahmen.
    drawGroup (groupRayArea,        groupColour (LCRMSAudioProcessor::SOLO_RAY,        perSection ? pal.frameRaye : cPurple), rayFrameOn, soloState == LCRMSAudioProcessor::SOLO_RAY);

    // Pop: Footer in zwei Kaesten wie die Sektionen (Meter..Vol, PRISM) -
    // sonst wirkt er "draufgesetzt" (User).
    if (isComicTheme())
    {
        auto boxOf = [] (std::initializer_list<juce::Component*> cs)
        {
            juce::Rectangle<int> u;
            for (auto* c : cs) u = u.isEmpty() ? c->getBounds() : u.getUnion (c->getBounds());
            return u.expanded (10, 8);
        };
        auto drawBox = [&] (juce::Rectangle<int> b)
        {
            if (b.isEmpty()) return;
            auto rf = b.toFloat();
            g.setColour (comicInk());
            g.fillRoundedRectangle (rf.translated (4.0f, 4.0f), 12.0f);
            g.setColour (juce::Colour (0xff2a2450));
            g.fillRoundedRectangle (rf, 12.0f);
            g.setColour (comicInk());
            g.drawRoundedRectangle (rf, 12.0f, 3.0f);
        };
        // Ein Kasten um alle Footer-Elemente inkl. Beschriftungen und IN/OUT-
        // Meter (User: "lieber einen Kasten, alle Labels drinnen").
        drawBox (boxOf ({ &volInputMeter, &volOutputMeter, &inputMeterLabel, &outputMeterLabel,
                          &monoCheckButton, &monoCheckLabel, &monoDryButton, &monoDryLabel,
                          &mixSlider, &mixLabel, &volSlider, &volLabel,
                          &panSlider, &panLabel,   // Runde 93 (User): Pan stand ausserhalb des Kastens
                          &prismOnButton, &prismBand }));
        // Das "?" unten links bekommt einen eigenen kleinen Kasten. Er darf
        // den Rahmen darueber ueberlappen - das sieht in Pop absichtlich so
        // aus (User).
        drawBox (helpButton.getBounds().expanded (5, 4));
    }

    // RAYE-Pair: goldene Klammer um Speed/Sync/Bars in Hyperdrive - die
    // sichtbare Bruecke zwischen den beiden Sektionen, solange die Kopplung
    // aktiv ist.
    // (Die frueheren goldenen Rahmen um Speed bzw. Sync/Bars sind entfernt -
    //  User: nur noch die Elemente selbst markieren.)

    // ===== POLARITY-POSITION SICHTBAR MACHEN =====
    // Ein kleiner leuchtender Punkt an der Stelle der Kette, an der der
    // Flip gerade sitzt (User-Idee: "die Stelle wo 1-4 ist visuell
    // darstellen ... zwischen den beiden Sections zwischen denen es wirkt").
    // Kette: Galaxy -> Timewarp -> Dimension -> Hyperdrive -> Vision -> RAYE.
    //   1 = vor Galaxy (linke Kante von Galaxy)
    //   2 = nach Galaxy (Fuge Galaxy | Timewarp)
    //   3 = nach Dimension (Fuge Dimension | Hyperdrive)
    //   4 = nach Vision (Fuge Vision | RAYE)
    if (polFrameOn && ! groupLcrArea.isEmpty())
    {
        const int pos = juce::jlimit (0, 3, (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_POL_POS)->load()));
        juce::Point<float> m;
        switch (pos)
        {
            case 0: m = { (float) groupLcrArea.getX() - 9.0f, (float) groupLcrArea.getCentreY() }; break;
            case 1: m = { (float) groupLcrArea.getCentreX(), 0.5f * (float) (groupLcrArea.getBottom() + groupDriftArea.getY()) }; break;
            case 2: m = { (float) groupWidthBoostArea.getCentreX(), 0.5f * (float) (groupWidthBoostArea.getBottom() + groupFlowArea.getY()) }; break;
            // Die Positionen 1 (vor Galaxy) und 4 (hinter Vision) sind
            // gestrichen; ihre Marker faenden ohnehin keinen Rahmen mehr.
            default: m = { (float) groupWidthBoostArea.getCentreX(), 0.5f * (float) (groupWidthBoostArea.getBottom() + groupFlowArea.getY()) }; break;
        }
        // Marker dezent in der Theme-Familie (User) - Pop behaelt sein Lila.
        const juce::Colour polCol = isComicTheme() ? juce::Colour (0xffb968ff)
                                                   : themePalette().frameMain.interpolatedWith (themePalette().knob, 0.30f);
        // Runde 110 (User): sah aus wie ein verirrter Knopf. Jetzt ein Punkt
        // auf einer feinen Linie - liest sich als "hier in der Kette".
        g.setColour (polCol.withAlpha (0.35f));
        g.fillRect (m.x - 12.0f, m.y - 0.5f, 24.0f, 1.0f);
        g.setColour (polCol.withAlpha (0.18f));
        g.fillEllipse (m.x - 6.0f, m.y - 6.0f, 12.0f, 12.0f);
        g.setColour (polCol);
        g.fillEllipse (m.x - 3.0f, m.y - 3.0f, 6.0f, 6.0f);
    }

    // Galaxy visuell deutlicher abgesetzt (User-Wunsch: "Galaxy muss visuell
    // besser getrennt sein. Ist ein starker Einfluss wegen Latenz.") - ein
    // zusaetzlicher, sanft pulsierender Aussen-Glow um den gesamten Galaxy-
    // Rahmen, aber NUR wenn "Activate Galaxy" (ID_GALAXY_ACTIVATE) auch
    // wirklich an ist - genau dann faellt die Host-Latenz tatsaechlich an.
    // Reiner Zusatz-Effekt zum normalen drawGroup()-Rahmen oben, keine
    // Ersetzung. Nutzt dieselbe Zeit-basierte Sinus-Pulsierung wie der
    // Solo-Blink (kein eigener Timer/Member noetig, bleibt CPU-guenstig).
    if (! groupLcrArea.isEmpty())
    {
        // Der Glow zeigt die anfallende Latenz an - er darf aber nicht
        // leuchten, wenn die Galaxy-Sektion selbst aus ist (User: "beim
        // allen Themes: Glow bei Galaxy wenn off muss aus sein -
        // irrefuehrend").
        // Runde 110 (User): kein Extra-Schein mehr um LCR - die Latenz meldet
        // der blaue LCR-Chip oben. Hervorhebung nur noch bei Solo.
        const bool galaxyEngineOn = false
                                 && processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_GALAXY_ACTIVATE)->load() > 0.5f
                                 && processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_LCR_ENABLED)->load() > 0.5f
                                 && ! processor.isBypassedNow()
                                 && ! layoutFrameless();   // ohne Sektionskasten haette der Glow nichts, worum er liegen koennte
        if (galaxyEngineOn)
        {
            // Konstant statt zeitgepulst (User-Bug: RAYE an/aus und Pair-Klick
            // liessen den Galaxy-Glow "atmen" - paintContent zeichnet nur bei
            // Klicks neu, die Sinusphase war dann jedes Mal eine andere).
            // Runde 66 (User, alle Themes): der Glow war zu praesent. Er soll
            // die Latenz melden, nicht die Sektion ueberstrahlen.
            const float pulse = 0.42f;
            auto glowRect = groupLcrArea.toFloat().reduced (3.0f);
            const juce::Colour glowCol (isComicTheme() ? juce::Colour (0xffffb648) : themePalette().frameRaye);   // Theme-Farbe (User); Pop Amber
            for (int layer = 3; layer >= 1; --layer)
            {
                const float expand = 2.0f + 2.4f * (float) layer;
                g.setColour (glowCol.withAlpha (0.05f * pulse * (float) (4 - layer)));
                g.drawRoundedRectangle (glowRect.expanded (expand), 10.0f + expand, 1.6f);
            }
        }
    }
}

// Bypass: halbtransparenter grauer Schleier ueber der GESAMTEN GUI (statt
// jedes einzelne Element separat auszugrauen - guenstiger und optisch
// konsistenter). Wird bewusst in paintOverChildren() gezeichnet (siehe
// ContentComponent), NICHT hier oben in paintContent() - sonst laege der
// Schleier UNTER den Kind-Komponenten (Regler, Goniometer, Korrelations-
// messer) und wuerde von deren eigenem Zeichnen ueberdeckt (genau das war
// der Bug: Goniometer + Korrelationsmesser blieben bei Bypass weiterhin
// voll farbig). So liegt er wirklich ueber allem; nur der Logo-Klick-
// bereich selbst bleibt normal bedienbar (Bypass wieder ausschalten
// funktioniert weiterhin per Klick auf das Logo).
// Runde 126: EQ-Icon - Zielform aus Modus und Fader, die angezeigte Form
// gleitet mit 60 Hz dorthin (Zeitkonstante ~70 ms). Auch ein Moduswechsel
// wird so zu einer Bewegung statt eines Sprungs.
void LCRMSAudioProcessorEditor::tickEqIcon()
{
    const int mode = juce::jlimit (0, LCRMSAudioProcessor::kMsEqModes - 1,
                                   (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_MS_EQ)->load()));
    const float amt = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_MS_EQ_AMT)->load() * 0.01f;
    const auto L = sideeq::lookFor (mode, amt);
    const float tgt[sideeq::kLookFields] = { L.hpX, L.hpSlope, L.hpW, L.sX, L.sG, L.sW, L.mX, L.mG, L.mW };
    bool changed = ! eqLookInit;
    for (int k = 0; k < sideeq::kLookFields; ++k)
    {
        if (! eqLookInit) { eqLookCur[k] = tgt[k]; continue; }
        const float d = tgt[k] - eqLookCur[k];
        if (d == 0.0f) continue;
        eqLookCur[k] = std::abs (d) < 1.0e-4f ? tgt[k] : eqLookCur[k] + d * 0.22f;
        changed = true;
    }
    eqLookInit = true;
    if (! changed) return;
    juce::Array<juce::var> arr;
    for (int k = 0; k < sideeq::kLookFields; ++k) arr.add ((double) eqLookCur[k]);
    msEqButton.getProperties().set ("eqLook", juce::var (arr));
    msEqButton.repaint();
}

// ===== THEME =====
void LCRMSAudioProcessorEditor::setUiTheme (int theme, bool persist)
{
    uiThemeIndex = juce::jlimit (0, kUiThemeCount - 1, theme);
    // "Sci-Fi Dark" (6) gibt es nicht mehr - gespeicherte Einstellungen aus
    // aelteren Versionen landen jetzt auf dem zusammengelegten Sci-Fi (3).
    if (uiThemeIndex == 6) uiThemeIndex = 3;
    // Runde 66 (User): "Moon" ist ganz raus - weder das alte (0) noch das
    // spaetere Silber (5) steht noch zur Auswahl. Gespeicherte Einstellungen
    // landen auf dem neuen Standard Day & Night (4). Uebrig bleiben
    // Day & Night (4), Fairy Tale (1), Sci-Fi (3) und Pop (2).
    if (uiThemeIndex == 0 || uiThemeIndex == 5) uiThemeIndex = 4;
    // Runde 126 (User): "Pop" ist ebenfalls raus - landet auf Day & Night.
    if (uiThemeIndex == 2) uiThemeIndex = 4;
    uiThemeRef() = (UiTheme) uiThemeIndex;
    // Runde 71 (User): jedes Theme hat sein festes Layout. Outline traegt
    // seit Runde 66 eine themeeigene Fuellung, damit ist die Unterscheidung
    // zwischen "3D" und "Outline" gegenstandslos geworden - es bleibt eine
    // Variante, und die Auswahl verschwindet aus dem Menue.
    uiLayoutRef() = 2;
    {
        const auto pal = themePalette();
        lookAndFeel.accent     = pal.knob;   // Reglerwerte
        lookAndFeel.glowAccent = pal.mod;    // Mod-Punkte / aktive Knoepfe
    }
    goniometer.setUiTheme (uiThemeIndex);
    // Runde 138 (User-Bug): das Smart-Profil behielt nach dem Theme-Wechsel
    // die alte Farbe, bis man es umschaltete - Farbe hier gleich mitziehen.
    categoryButton.getProperties().set ("pillColour",
        (int) (mutateCategoryValue > 0 ? themePalette().knob : juce::Colour (0xff7b808b)).getARGB());
    categoryButton.repaint();
    catDots.repaint();
    prismOnButton.getProperties().set ("powerColour", (int) (isComicTheme() ? 0xff8a6ab8u : themePalette().prism.getARGB()));
    mixSlider.getProperties().set ("footerKnob", true);
    volSlider.getProperties().set ("footerKnob", true);
    {
        const auto pal = themePalette();
        if (isComicTheme()) viewPanel.setHeadColours (juce::Colour (0xffb968ff), juce::Colour (0xff5be3c7), juce::Colour (0xff6bb8ff));
        else                viewPanel.setHeadColours (pal.knob, pal.knob.withAlpha (0.85f), pal.knob.withAlpha (0.85f));   // eine Farbe (User)
    }
    if (persist)
    {
        // Gonio-Farbe je Theme als Startwert (User: Modern Blau, Watercolor
        // Gold, Comic Purple, Modern Purple Gruen) - bleibt danach frei waehlbar.
        viewPanel.setGonioColourIndex (gonioColourForTheme (uiThemeIndex));
        applyViewSettings (false);
        juce::PropertiesFile props (LCRMSAudioProcessor::appPropertiesOptions());
        props.setValue ("uiTheme4", uiThemeIndex);
        props.saveIfNeeded();
    }
    // Alle Kinder neu zeichnen lassen (Regler/Knoepfe holen sich das Theme
    // beim Zeichnen aus dem LookAndFeel).
    content.sendLookAndFeelChange();
    content.repaint();
    repaint();
}

// Abgerundetes Rechteck mit leicht unregelmaessigem Rand (Wasserfarbe):
// Punkte entlang des Umrisses werden entlang der Normalen um eine Summe
// weniger Sinus-Wellen verschoben - deterministisch je Sektion (seed).
juce::Path LCRMSAudioProcessorEditor::wobblyRoundedRect (juce::Rectangle<float> r, float corner, float amp, int seed)
{
    juce::Path base;
    base.addRoundedRectangle (r, corner);
    const float len = base.getLength();
    const float step = 7.0f;
    const float ph1 = (float) (seed % 97) * 0.13f, ph2 = (float) (seed % 41) * 0.31f, ph3 = (float) (seed % 23) * 0.57f;
    juce::Path out;
    bool started = false;
    for (float d = 0.0f; d < len; d += step)
    {
        const auto p0 = base.getPointAlongPath (d);
        const auto p1 = base.getPointAlongPath (juce::jmin (len - 0.01f, d + 1.0f));
        auto t = p1 - p0;
        const float tl = std::max (0.001f, std::hypot (t.x, t.y));
        const juce::Point<float> n (-t.y / tl, t.x / tl);
        const float w = amp * (0.55f * std::sin (d * 0.061f + ph1) + 0.30f * std::sin (d * 0.137f + ph2) + 0.15f * std::sin (d * 0.29f + ph3));
        const auto p = p0 + n * w;
        if (! started) { out.startNewSubPath (p); started = true; }
        else            out.lineTo (p);
    }
    out.closeSubPath();
    return out;
}

// Platten-Textur je Theme, einmal in ein Bild gebacken (kein CPU-Aufwand
// pro Frame): Wasserfarbe = dunkle Flaeche mit weichen Nebula-Farbwolken und
// feinem Korn; Comic = flache Flaeche mit Rasterpunkten.
void LCRMSAudioProcessorEditor::drawThemePlate (juce::Graphics& g, juce::Rectangle<float> plate, float corner)
{
    // Flat und Sci-Fi bekommen eine reine Farbplatte ohne jede Textur -
    // bei Flat ist genau das der Punkt des Themes (User).
    if (isFlatTheme() || isSciFiTheme())
        return;
    const int w = (int) std::ceil (plate.getWidth()), h = (int) std::ceil (plate.getHeight());
    if (themePlateFor != uiThemeIndex || themePlate.getWidth() != w || themePlate.getHeight() != h)
    {
        themePlate = juce::Image (juce::Image::ARGB, juce::jmax (1, w), juce::jmax (1, h), true);
        juce::Graphics pg (themePlate);
        const auto full = juce::Rectangle<float> (0, 0, (float) w, (float) h);
        juce::Path clip; clip.addRoundedRectangle (full, corner);
        pg.reduceClipRegion (clip);
        if (isMoonTheme())
        {
            // Moon (User: "die GUI darf dezent was vom Mond haben" - beim
            // Umbenennen von Flat auf Moon uebernommen und noch eine Spur
            // zurueckgenommen): ein Hauch Mondlicht oben links und ein paar
            // sehr blasse Krater unten rechts - transparent, kein Korn.
            juce::ColourGradient light (juce::Colours::white.withAlpha (0.026f), (float) w * 0.10f, (float) h * 0.05f,
                                        juce::Colours::white.withAlpha (0.0f),   (float) w * 0.10f + (float) w * 0.55f, (float) h * 0.05f, true);
            pg.setGradientFill (light);
            pg.fillAll();
            juce::ColourGradient flow (juce::Colour (0xffd8d4cc).withAlpha (0.0f), (float) w * 0.55f, (float) h * 0.55f,
                                       juce::Colour (0xffd8d4cc).withAlpha (0.022f), (float) w, (float) h, false);   // "fliessend", dezent (User)
            pg.setGradientFill (flow);
            pg.fillAll();
            struct Crater { float fx, fy, fr; };
            static const Crater craters[] = { { 0.86f, 0.80f, 0.075f }, { 0.74f, 0.92f, 0.045f }, { 0.94f, 0.96f, 0.032f }, { 0.08f, 0.90f, 0.038f } };
            for (const auto& c : craters)
            {
                const float cx = c.fx * (float) w, cy = c.fy * (float) h, r = c.fr * (float) w;
                juce::ColourGradient bowl (juce::Colours::black.withAlpha (0.072f), cx + r * 0.25f, cy + r * 0.25f,
                                           juce::Colours::black.withAlpha (0.0f), cx + r * 0.25f + r, cy + r * 0.25f, true);
                pg.setGradientFill (bowl);
                pg.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
                pg.setColour (juce::Colours::white.withAlpha (0.032f));
                pg.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.0f);
                pg.setColour (juce::Colours::white.withAlpha (0.021f));   // Lichtkante oben links
                juce::Path lit; lit.addCentredArc (cx, cy, r - 1.0f, r - 1.0f, 0.0f, -2.9f, -1.2f, true);
                pg.strokePath (lit, juce::PathStrokeType (1.6f));
            }
        }
        else if (isDayNightTheme())
        {
            // Day & Night (User: "Akzente, darf auch was Fliessendes sein, aber
            // dezent"): warmer Lichthof oben links (Sonne), zwei sehr blasse
            // Ringe, und unten ein weicher blauer Horizont-Hauch (Erde).
            const float cx = (float) w * 0.06f, cy = (float) h * 0.04f;
            juce::ColourGradient light (juce::Colour (0xfff5c96b).withAlpha (0.07f), cx, cy,
                                        juce::Colour (0xfff5c96b).withAlpha (0.0f), cx + (float) w * 0.50f, cy, true);
            pg.setGradientFill (light);
            pg.fillAll();
            for (int k = 0; k < 2; ++k)
            {
                const float rr = (float) w * (0.22f + 0.14f * (float) k);
                pg.setColour (juce::Colour (0xfff5c96b).withAlpha (0.032f - 0.01f * (float) k));
                pg.drawEllipse (cx - rr, cy - rr, rr * 2.0f, rr * 2.0f, 1.2f);
            }
            juce::ColourGradient horizon (juce::Colour (0xff6fc3ff).withAlpha (0.0f), 0.0f, (float) h * 0.62f,
                                          juce::Colour (0xff6fc3ff).withAlpha (0.045f), 0.0f, (float) h, false);
            pg.setGradientFill (horizon);
            pg.fillAll();
        }
        else if (isWaterTheme())
        {
            const bool dark = true;   // nur noch Dark Night (User: Night geloescht)
            pg.setColour (dark ? juce::Colour (0xff0c0e15) : juce::Colour (0xff10131c));
            pg.fillAll();
            auto blob = [&] (float fx, float fy, float fr, juce::Colour c, float a)
            {
                juce::ColourGradient grad (c.withAlpha (a), fx * (float) w, fy * (float) h,
                                           c.withAlpha (0.0f), fx * (float) w + fr * (float) w, fy * (float) h, true);
                pg.setGradientFill (grad);
                pg.fillAll();
            };
            // Farbwolken nur als Hauch (User: "zu stark texturiert ... eher
            // leicht texturiert, subtil, modern").
            const float bm = dark ? 0.55f : 1.0f;   // Dark Night: Farbwolken halb so stark
            blob (0.86f, 0.12f, 0.50f, juce::Colour (0xff506edc), 0.11f * bm);
            blob (0.74f, 0.88f, 0.46f, juce::Colour (0xffbe6ec8), 0.08f * bm);
            blob (0.18f, 0.55f, 0.42f, juce::Colour (0xff50a0c8), 0.07f * bm);
            blob (0.30f, 0.05f, 0.30f, juce::Colour (0xff8a78d8), 0.06f * bm);
            // Nebula-Foto ganz dezent hinter den Sektionen (rechte Haelfte),
            // nach links bis zur Plugin-Mitte und oben/unten ausblendend (User).
            if (auto neb = juce::ImageFileFormat::loadFrom (SpaceAssets::bg_nebula, (size_t) SpaceAssets::bg_nebulaSize); neb.isValid())
            {
                const int hw = w / 2;
                juce::Image half (juce::Image::ARGB, juce::jmax (1, hw), juce::jmax (1, h), true);
                {
                    juce::Graphics hg (half);
                    const float sc = juce::jmax ((float) hw / (float) neb.getWidth(), (float) h / (float) neb.getHeight());
                    hg.drawImageTransformed (neb, juce::AffineTransform::scale (sc).translated (((float) hw - (float) neb.getWidth() * sc) * 0.5f, ((float) h - (float) neb.getHeight() * sc) * 0.5f));
                }
                juce::Image::BitmapData hb (half, juce::Image::BitmapData::readWrite);
                for (int y = 0; y < hb.height; ++y)
                {
                    const float fy = (float) y / (float) juce::jmax (1, hb.height - 1);
                    const float vy = juce::jlimit (0.0f, 1.0f, juce::jmin (fy / 0.25f, (1.0f - fy) / 0.25f));
                    for (int x = 0; x < hb.width; ++x)
                    {
                        const float fx = (float) x / (float) juce::jmax (1, hb.width - 1);
                        const float vx = juce::jlimit (0.0f, 1.0f, fx / 0.65f);           // links (Plugin-Mitte) = 0
                        const float a  = (dark ? 0.05f : 0.10f) * vx * vy;                 // maximal 10 % (Dark Night 5 %)
                        auto* px = hb.getPixelPointer (x, y);
                        for (int c = 0; c < 4; ++c)
                            px[c] = (juce::uint8) juce::jlimit (0, 255, (int) std::round ((float) px[c] * a));
                    }
                }
                pg.drawImageAt (half, w - hw, 0);
            }
            if (dark)
            {
                // Dark Night (User): die UI besonders rechts unter den Sektionen
                // dunkler werden lassen - weicher Verlauf von der Mitte nach rechts.
                juce::ColourGradient shade (juce::Colours::black.withAlpha (0.0f), (float) w * 0.42f, 0.0f,
                                            juce::Colours::black.withAlpha (0.42f), (float) w, 0.0f, false);
                pg.setGradientFill (shade);
                pg.fillAll();
            }
            // Feines Korn (Papier), halb so stark wie zuvor.
            juce::Image::BitmapData bd (themePlate, juce::Image::BitmapData::readWrite);
            juce::Random rnd (1234);
            for (int y = 0; y < bd.height; ++y)
                for (int x = 0; x < bd.width; ++x)
                {
                    auto* px = bd.getPixelPointer (x, y);   // BGRA (premultiplied)
                    if (px[3] == 0) continue;
                    const int n = rnd.nextInt (9) - 4;
                    for (int c = 0; c < 3; ++c)
                        px[c] = (juce::uint8) juce::jlimit (0, (int) px[3], (int) px[c] + n);
                }
            // Korn-Kachel fuer die Sektionsflaechen (wird gekachelt gefuellt).
            grainTile = juce::Image (juce::Image::ARGB, 96, 96, true);
            juce::Image::BitmapData gt (grainTile, juce::Image::BitmapData::readWrite);
            for (int y = 0; y < gt.height; ++y)
                for (int x = 0; x < gt.width; ++x)
                {
                    auto* px = gt.getPixelPointer (x, y);
                    const int v = rnd.nextInt (256);
                    const juce::uint8 a = (juce::uint8) 22;               // sehr leicht
                    const juce::uint8 l = (juce::uint8) ((v * (int) a) / 255);   // premultiplied
                    px[0] = l; px[1] = l; px[2] = l; px[3] = a;
                }
        }
        else
        {
            pg.setColour (juce::Colour (0xff1c1738));
            pg.fillAll();
            juce::ColourGradient grad (juce::Colour (0xff271f4d), 0.0f, 0.0f, juce::Colour (0xff15112c), (float) w, (float) h, false);
            pg.setGradientFill (grad);
            pg.fillAll();
            pg.setColour (juce::Colours::white.withAlpha (0.075f));
            for (int y = 0, row = 0; y < h; y += 12, ++row)
                for (int x = (row & 1) ? 6 : 0; x < w; x += 12)
                    pg.fillEllipse ((float) x - 1.3f, (float) y - 1.3f, 2.6f, 2.6f);
        }
        themePlateFor = uiThemeIndex;
    }
    g.drawImageAt (themePlate, (int) plate.getX(), (int) plate.getY());
}

// Hinweiszeile unter dem Footer. Liegt in paintOverContent, damit sie auch
// ueber dem abgedunkelten Bereich des View-Panels lesbar bleibt.
void LCRMSAudioProcessorEditor::drawHintBar (juce::Graphics& g)
{
    if (hintBarArea.isEmpty()) return;
    auto r = hintBarArea.toFloat();
    // (Runde 53, User: der Strich lief ueber die ganze UI und stoerte - raus.)

    // ===== AUTO-GAIN-ANZEIGE =====
    // Rechts in derselben Zeile; die Flaeche kommt aus layoutContent(), weil
    // dort auch die Klickflaeche daraufgelegt wird. Im Aus-Zustand steht
    // "off" statt der Zahl - sonst waere der Knopf unsichtbar und man kaeme
    // nicht mehr hin.
    if (! autoGainReadoutArea.isEmpty())
    {
        const bool agOn = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_AUTO_GAIN)->load() > 0.5f;
        const float db = processor.autoGainDb.load (std::memory_order_relaxed);
        const juce::String txt = ! agOn ? juce::String ("off")
                               : (std::abs (db) < 0.05f) ? juce::String ("0.0 dB")
                                                         : juce::String (db, 1) + " dB";
        // "AG" sitzt direkt links vom Wert statt am linken Rand der Flaeche
        // (User Runde 53: "AG bisschen naeher zum dB Feld").
        const auto agFont = juce::Font (juce::FontOptions (12.0f));
        g.setFont (agFont);
        const float txtW = juce::GlyphArrangement::getStringWidth (agFont, txt);
        auto agArea = autoGainReadoutArea.toFloat().withTrimmedRight (txtW + 6.0f);
        g.setColour (themePalette().knob.withAlpha (agOn ? 0.40f : 0.22f));
        g.drawText ("AG", agArea, juce::Justification::centredRight, false);
        g.setColour (themePalette().knob.withAlpha (agOn ? 0.85f : 0.30f));
        g.drawText (txt, autoGainReadoutArea.toFloat(), juce::Justification::centredRight, false);
        r = r.withTrimmedRight ((float) autoGainReadoutArea.getWidth() + 10.0f);
    }
    else
    {
        autoGainReadoutArea = {};
    }

    // (Runde 140: Katzenlogo unten rechts wieder entfernt - User: "sieht nicht gut aus".)

    if (currentHint.isEmpty())
        return;   // Platzhaltertext entfallen (User: "weiss jeder")

    // Bezeichnung bis zum Doppelpunkt fett, Rest normal - dasselbe Muster wie
    // frueher im Tooltip-Fenster.
    // Runde 104 (User): "Infozeile unten von der Schrift genauso wie die
    // oben fuer die Smart-Profile" - dieselbe Groesse, dieselbe Farbe, kein
    // fetter Vorspann mehr.
    // Runde 153 (User): steht vorne der Name des Reglers / Modus / Schalters
    // ("Width:", "Style:", "EQ -> LCR:"), leuchtet er in der Theme-Farbe -
    // dieselbe, in der aktive Bezeichnungen oben stehen. Der Rest bleibt grau.
    const auto hintFont = juce::Font (juce::FontOptions (14.5f));
    g.setFont (hintFont);
    // Runde 162 (User): Tastenbefehle ("Cmd-click: solo") leicht heller als
    // die Erklaerung - man findet sie auf einen Blick. Die Zeile wird dafuer
    // in Stuecke zerlegt und hintereinander gesetzt.
    const juce::Colour bodyCol (0xff8f96a4), cmdCol (0xffc9cdd6);
    auto isCommand = [] (const juce::String& seg)
    {
        return seg.containsIgnoreCase ("click") || seg.startsWith ("Cmd") || seg.startsWith ("Shift")
            || seg.startsWith ("Alt") || seg.startsWith ("Right");
    };
    float x = r.getX();
    auto put = [&] (const juce::String& txt, juce::Colour col)
    {
        if (txt.isEmpty() || x >= r.getRight()) return;
        g.setColour (col);
        const float w = juce::GlyphArrangement::getStringWidth (hintFont, txt);
        g.drawText (txt, juce::Rectangle<float> (x, r.getY(), r.getRight() - x, r.getHeight()),
                    juce::Justification::centredLeft, true);
        x += w;
    };
    juce::String rest = currentHint;
    const int colon = currentHint.indexOf (": ");
    if (colon > 0 && colon <= 28)
    {
        put (currentHint.substring (0, colon), themePalette().knob);   // Name in der Theme-Farbe (Runde 153)
        rest = currentHint.substring (colon);
    }
    const juce::String sep = juce::String::fromUTF8 (" \xc2\xb7 ");
    juce::StringArray parts;   // an " · " zerlegen (fromTokens trennt nur an Einzelzeichen)
    for (juce::String left = rest;;)
    {
        const int at = left.indexOf (sep);
        if (at < 0) { parts.add (left); break; }
        parts.add (left.substring (0, at));
        left = left.substring (at + sep.length());
    }
    for (int i = 0; i < parts.size(); ++i)
    {
        if (i > 0) put (sep, bodyCol);
        put (parts[i], (i > 0 && isCommand (parts[i])) ? cmdCol : bodyCol);
    }
}

void LCRMSAudioProcessorEditor::applyBrightness (float v, bool persist)
{
    uiBrightnessRef() = juce::jlimit (0.0f, 1.0f, v);
    if (persist)
    {
        juce::PropertiesFile p (LCRMSAudioProcessor::appPropertiesOptions());
        p.setValue ("uiBrightness", (double) uiBrightnessRef());
        p.saveIfNeeded();
    }
    if (std::abs ((float) settingsPanel.brightSlider.getValue() - uiBrightnessRef()) > 0.0001f)
        settingsPanel.brightSlider.setValue (uiBrightnessRef(), juce::dontSendNotification);
    content.repaint();
}

void LCRMSAudioProcessorEditor::paintOverContent (juce::Graphics& g)
{
    // Runde 174 (User: "im Dunkeln oder bei falschem Licht sieht man wenig -
    // ein Ticken heller, aber den Look behalten, und bitte eine einfache
    // Methode, die nicht nachgebessert werden muss"): EINE helle Schicht in
    // einem Hauch der Plattenfarbe ueber der ganzen Oberflaeche. Sie hebt
    // die Tiefen deutlich, die Mitten etwas und Weiss praktisch gar nicht -
    // wie ein angehobener Schwarzwert. Weil sie ueber ALLEM liegt, bleiben
    // alle An/Aus- und Bypass-Zustaende untereinander exakt gleich, kein
    // Element braucht eigene Regeln. Das Sternenfeld bleibt schwarz (es hat
    // seinen eigenen Shine-Regler). Bei 100 %: Platte ~0x15 -> ~0x29.
    if (const float br = uiBrightnessRef(); br > 0.001f)
    {
        g.saveState();
        if (goniometer.isVisible())
            g.excludeClipRegion (goniometer.getBounds());
        const auto lift = themePalette().plate.interpolatedWith (juce::Colours::white, 0.80f);
        g.setColour (lift.withAlpha (0.11f * br));
        g.fillAll();
        g.restoreState();
    }

    // Runde 58 (User-Bug: "Info Zeile ist doppelt ... AutoGain scheint auch
    // durch"): paintOverChildren laeuft NACH allen Kindern, die Zeile lag
    // deshalb ueber jedem Overlay. Sie gehoert zur normalen Oberflaeche, also
    // pausiert sie, solange ein Overlay offen ist.
    const bool overlayOpen = settingsPanel.isVisible() || backPanel.isVisible() || tourOverlay.isVisible()
                          || savePanel.isVisible();
    if (! overlayOpen)
        drawHintBar (g);

    // Runde 150: feine senkrechte Trennlinien zwischen den Footer-Gruppen,
    // oben und unten ausgeblendet (wie im Header).
    for (int sx : footerSepX)
    {
        if (sx <= 0 || footerSepBottom <= footerSepTop) continue;
        const float x = (float) sx + 0.5f, y1 = (float) footerSepTop, y2 = (float) footerSepBottom;
        juce::ColourGradient grad (juce::Colours::white.withAlpha (0.0f), x, y1,
                                   juce::Colours::white.withAlpha (0.0f), x, y2, false);
        grad.addColour (0.5, juce::Colours::white.withAlpha (0.13f));
        g.setGradientFill (grad);
        g.fillRect (x - 0.5f, y1, 1.0f, y2 - y1);
    }
    auto bounds = juce::Rectangle<float> (0, 0, (float) kDesignW, (float) kDesignH);

    // View-Panel offen: die Sektionsspalte rechts abdunkeln, damit das Panel
    // darueber klar hervortritt und das Sternenfeld links frei bleibt.
    if (viewPanel.isVisible())
    {
        g.saveState();
        g.excludeClipRegion (viewPanel.getBounds());
        auto rightCol = juce::Rectangle<int> (goniometer.getRight() + 10, kOuterMargin + kTitleBarH,
                                              kDesignW - goniometer.getRight() - 10, kDesignH - kOuterMargin - kTitleBarH);
        // Abgerundet und mit weichem Saum statt harter Kante (User: "sieht
        // kantig aus an den Raendern").
        const juce::Colour dimCol (0xff0a0b0e);
        juce::Path dimPath;
        dimPath.addRoundedRectangle (rightCol.toFloat().reduced (2.0f), 14.0f);
        g.setColour (dimCol.withAlpha (0.88f));   // 0.80 -> 0.88 (User: "noch zu durchsichtig")
        g.fillPath (dimPath);
        for (int i = 1; i <= 5; ++i)
        {
            g.setColour (dimCol.withAlpha (0.30f * (1.0f - (float) i / 6.0f)));
            g.strokePath (dimPath, juce::PathStrokeType ((float) i * 2.0f));
        }
        g.restoreState();
    }

    // Settings-Panel: alles dahinter abdunkeln, damit das Panel klar
    // hervortritt (dasselbe Prinzip wie beim View-Panel, nur ueber die ganze
    // Flaeche, weil das Panel mittig liegt).
    if (settingsPanel.isVisible() || backPanel.isVisible() || savePanel.isVisible())
    {
        g.saveState();
        // Bug (User Runde 71: "komische Grafik an den Ecken"): ausgespart
        // wurde das RECHTECK des Panels - gezeichnet wird es aber mit runden
        // Ecken. In den vier Zwickeln dazwischen lag dadurch weder Panel noch
        // Schleier, und die ungedimmte Oberflaeche schaute durch. Jetzt wird
        // die abgerundete Form ausgespart (Even-Odd-Fuellregel = Loch).
        {
            const auto panelB = (settingsPanel.isVisible() ? settingsPanel.getBounds()
                               : savePanel.isVisible()     ? savePanel.getBounds()
                                                           : backPanel.getBounds()).toFloat();
            juce::Path veil;
            veil.setUsingNonZeroWinding (false);
            veil.addRectangle (bounds);
            veil.addRoundedRectangle (panelB, 12.0f);
            g.reduceClipRegion (veil);
        }
        if (settingsBlur.isValid())
        {
            g.setOpacity (1.0f);
            g.drawImage (settingsBlur, bounds.reduced (8.0f), juce::RectanglePlacement::stretchToFit);
        }
        g.setColour (juce::Colour (0xff0a0b0e).withAlpha (0.62f));
        g.fillRoundedRectangle (bounds.reduced (8.0f), 10.0f);
        g.restoreState();
    }

    // Demo-Modus: wird das Audio leiser, geht die GUI im selben Mass mit
    // runter (User: "wenn das Plugin leiser wird soll es auch gedimmt
    // werden"). So ist sofort klar, dass das Absenken gewollt ist und kein
    // Fehler im Mix.
    {
        const float duck = processor.demoDuck.load (std::memory_order_relaxed);
        if (duck < 0.999f)
        {
            g.setColour (juce::Colour (0xff0a0b0e).withAlpha ((1.0f - duck) * 0.58f));
            g.fillRoundedRectangle (bounds.reduced (8.0f), 10.0f);

            // Runde 53 (User: "wenn fade outs kommen soll DEMO mehr
            // leuchten"): die Plakette wird UEBER dem Schleier noch einmal
            // gezeichnet, im selben Mass heller, wie das Signal leiser wird.
            // So ist die Absenkung nie ein Raetsel - man sieht sofort, wer
            // sie verursacht.
            if (! demoChipArea.isEmpty() && ! processor.licensed.load (std::memory_order_relaxed))
            {
                const float t = juce::jlimit (0.0f, 1.0f, 1.0f - duck);
                const auto col = themePalette().frameRaye;
                for (int layer = 3; layer >= 1; --layer)
                {
                    const float grow = 4.0f * (float) layer;
                    g.setColour (col.withAlpha (0.10f * t * (float) (4 - layer)));
                    g.fillRoundedRectangle (demoChipArea.expanded (grow), 8.0f + grow);
                }
                g.setColour (col.withAlpha (0.18f + 0.55f * t));
                g.fillRoundedRectangle (demoChipArea, 8.0f);
                g.setColour (col.withAlpha (0.75f + 0.25f * t));
                g.drawRoundedRectangle (demoChipArea.reduced (0.5f), 8.0f, 1.0f + t);
                g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)).withExtraKerningFactor (0.18f));
                g.setColour (juce::Colours::white.withAlpha (0.75f + 0.25f * t));
                g.drawText ("DEMO", demoChipArea, juce::Justification::centred, false);
            }
        }
    }

    // Runde 174 (User): keine dunkle Ebene mehr ueber der ganzen GUI im
    // Bypass. Sektionen und Fusszeile werden gedimmt, Header und Meter bleiben
    // normal, und der Power-Knopf leuchtet.
}

void LCRMSAudioProcessorEditor::resized()
{
    // Editor-Fenster kann beliebig (innerhalb der Resize-Limits) gross sein;
    // die content-Komponente bleibt IMMER auf der logischen Design-Groesse
    // und wird per Transform passend skaliert - dadurch werden wirklich
    // alle Elemente (Rahmen, Schrift, Regler, Buttons) mitskaliert.
    const float scaleX = (float) getWidth()  / (float) kDesignW;
    const float scaleY = (float) getHeight() / (float) kDesignH;
    const float scale = juce::jmin (scaleX, scaleY);

    content.setTransform (juce::AffineTransform::scale (scale));
    content.setBounds (0, 0, kDesignW, kDesignH);

    // Gibt der Host beim Oeffnen eine Groesse vor, die nicht zum
    // Seitenverhaeltnis passt, wird sie hier einmal korrigiert - genau das,
    // was bisher erst beim Ziehen an der Ecke passierte.
    const int wantH = juce::roundToInt ((double) getWidth() * kDesignH / kDesignW);
    if (std::abs (getHeight() - wantH) > 2)
    {
        juce::Component::SafePointer<LCRMSAudioProcessorEditor> safe (this);
        juce::MessageManager::callAsync ([safe, wantH]
        {
            if (safe != nullptr && std::abs (safe->getHeight() - wantH) > 2)
                safe->setSize (safe->getWidth(), wantH);
        });
    }
}

void LCRMSAudioProcessorEditor::layoutContent()
{
    // Logo-Klickflaeche exakt ueber dem in paintContent() gezeichneten Logo -
    // beide nutzen dieselbe kTitleBarH-Konstante, damit sie garantiert
    // deckungsgleich bleiben.
    logoButton.setBounds (juce::Rectangle<int> (kOuterMargin, kOuterMargin, kTitleBarH, kTitleBarH).reduced (5));

    // ===== TITELZEILE: ZWEI ZEILEN RECHTS, EIN BLOCK =====
    // Dritter Anlauf, und diesmal mit einer Diagnose statt nur einer neuen
    // Anordnung. Der User hat die Versionen nebeneinander gelegt und
    // festgestellt: "optisch die beste finde ich sogar die Variante ohne
    // Preset Name" (die alte Einzeile) - obwohl er den Preset-Namen
    // ausdruecklich braucht.
    //
    // Was an der Zwischenversion (eigene Preset-Leiste unter der Titelzeile)
    // wirklich stoerte, war nicht der Inhalt, sondern die FORM: zwei Zeilen,
    // beide halb leer, und zwar DIAGONAL - oben rechts die Live-Aktionen mit
    // Leere links davon, darunter links die Presets mit Leere rechts davon.
    // Zwei Treppenstufen aus Luft. Das Auge liest das als "irgendwie
    // vollgepackt", weil nirgends ein ruhiger Block entsteht.
    //
    // Die alte Einzeile wirkte besser, weil sie EIN geschlossener Block war.
    // Also: die zwei Zeilen behalten (der Preset-Name braucht die Breite),
    // aber beide RECHTS buendig als EINEN Block setzen, exakt gleich breit:
    //
    //     BYP  M  M  B  ~  GALAXY  |  Undo Redo A/B Copy  |  Menu
    //     <  [ Preset-Name                 v ]  >  |  Save  Reset
    //
    // Links davon steht das Logo, das ohnehin schon zweizeilig ist
    // (Wortmarke oben, Slogan unten) - zwei Zeilen links, zwei Zeilen
    // rechts, dazwischen Luft. Das ist ein Header, kein Stapel.
    //
    // Gruppen: Zeile 1 = was das Plugin GERADE tut + Werkzeuge fuer den
    // Zustand (Undo/Redo/A-B/Copy, wie bei Pro-Q direkt nebeneinander) +
    // Menue. Zeile 2 = wo bin ich (Preset) und wie sichere/verwerfe ich es.
    // Load-Knopf gibt es nicht - das Namensfeld ist die Liste.
    {
        auto titleBar = juce::Rectangle<int> (kOuterMargin, kOuterMargin, kDesignW - kOuterMargin * 2, kTitleBarH);

        // Abstaende leicht geweitet (User: "wieder 50% zurueck") - der Block
        // waechst von 419 auf ~462px, bleibt aber innerhalb der Sektionskante.
        constexpr int kHamburgerW = 48;
        constexpr int kUndoBtnW = 30;
        constexpr int kIconBtnW = 36;
        constexpr int kGap = 6;
        constexpr int kSepGap = 15;
        constexpr int kSepW = 1;
        constexpr int kSepBlockW = kSepGap * 2 + kSepW;
        constexpr int kRowH = 26;
        constexpr int kRowGap = 6;

        // Vierter Anlauf, zwei Korrekturen des Users:
        //  1) "A/B + Copy gehoert fuer mich eher zu den Presets runter" -
        //     stimmt, beides sind Zustands-Werkzeuge wie Save/Reset.
        //  2) "beide Header duerfen maximal links bis zum linken vertikalen
        //     Rand der Sections gehen" - der Block darf also hoechstens so
        //     breit sein wie die Sektionsspalte rechts. Die beginnt bei
        //     kOuterMargin + gonioSize + 20 = 540, der Block endet bei 1020,
        //     ergibt 480 als Obergrenze. Zeile 1 kommt ohne A/B+Copy auf 419.
        //
        // Zeile 1:  BYP M M B ~ GALAXY | Undo Redo | Menu
        // Zeile 2:  < [ Name ] > | Save Delete | A/B Copy Reset
        // Runde 103 (User): Breathe ist weg, die Zeile heisst jetzt
        // Bypass - Wuerfel - Life - Mod. Der Wuerfel darf etwas groesser sein,
        // LIFE genauso gross wie die frueheren Mod-Regler in den Sektionen.
        // Runde 161 (User: "oben wirkt alles zu gross"): Wuerfel wieder so
        // breit wie die Icon-Knoepfe, LIFE etwas kleiner.
        constexpr int kDiceW   = 36;
        constexpr int kLifeW   = 32;
        constexpr int kLiveW   = kIconBtnW + kGap + kDiceW + kGap + kLifeW + kGap + kIconBtnW;
        constexpr int kGalaxyGap = 12;
        constexpr int kGalaxyW = 62;
        constexpr int kRow1W   = kLiveW + kGalaxyGap + kGalaxyW + kSepBlockW + (kUndoBtnW * 2 + kGap) + kSepBlockW + kHamburgerW;

        // Zeile 2 gleich breit. Das Namensfeld bekommt den Rest - deutlich
        // kleiner als vorher, und das ist so gewollt (User: "kann theoretisch
        // halb so gross sein - so lange werden die Namen normalerweise nie").
        constexpr int kPArrowW = 20;
        constexpr int kPSmallGap = 4;
        constexpr int kABW = 46;
        constexpr int kCopyW = 28;
        constexpr int kFileW   = kUndoBtnW * 2 + kGap;                 // Save, Delete
        constexpr int kStateW  = kABW + kGap + kCopyW + kGap + kUndoBtnW; // A/B, Copy, Reset
        constexpr int kGroupGap = 10;   // Save/Delete | A/B Copy Reset: Luecke statt Strich
        // Runde 129 (User): Namensfeld etwas breiter. Der Block waechst um
        // kNameExtra; Zeile 1 verteilt das auf die Luecke vor LCR und die
        // Trennstriche, damit beide Zeilen links und rechts buendig bleiben.
        constexpr int kNameExtra = 30;
        constexpr int kBlockW  = kRow1W + kNameExtra;
        constexpr int kNameW   = kRow1W - (kPArrowW * 2 + kPSmallGap * 2) - kSepBlockW - kFileW - kGroupGap - kStateW + kNameExtra;

        auto block = titleBar.removeFromRight (kBlockW).withSizeKeepingCentre (kBlockW, kRowH * 2 + kRowGap);
        auto row1 = block.removeFromTop (kRowH);
        block.removeFromTop (kRowGap);
        auto row2 = block.removeFromTop (kRowH);

        // Runde 55 (User-Mockup): das Smart-Profil sitzt als Pille mit Punkten
        // links neben der Global-Zeile - dieselbe Bildsprache wie die Modi in
        // Parallax und RAYE. Dafuer sind die vier Chips ueber dem Sternenfeld
        // weg, und das Feld bekommt deren Hoehe zurueck.
        {
            const int cw = 128, cbH = 30;   // Runde 161 (User): Klickzone reichte fast bis zum Preset-Pfeil - nur so breit wie Icon/Name
            // Vorlaeufige Lage - Runde 119 zentriert das Profil unten in
            // layoutContent() neu (nach dem Sternenfeld). Runde 115: mittig zwischen dem
            // Ende des Slogans (breiteste Zeile der Wortmarke) und dem
            // ersten Icon der Kopfzeile (Power, ~6 px eingerueckt).
            const juce::Font sloganFont = juce::Font (juce::FontOptions (13.8f, juce::Font::bold))
                                              .withExtraKerningFactor (0.16f);
            const int wordRight = titleBar.getX() + kTitleBarH + 18
                                + juce::roundToInt (juce::GlyphArrangement::getStringWidth (sloganFont, "SPATIAL INTELLIGENCE"));
            const int midX = (wordRight + titleBar.getRight() + 6) / 2;
            auto catCol = juce::Rectangle<int> (midX - cw / 2, titleBar.getY(), cw, titleBar.getHeight());
            // Runde 110 (User): das Profil nimmt die Hoehe beider Kopfzeilen -
            // Icon oben, Name darunter, Punkte ganz unten, wie Velvet.
            juce::ignoreUnused (cbH);
            // Runde 113 (User: "etwas groesser, etwas hoeher, Punkte zu nah
            // an LCR"): beginnt 10 px ueber der ersten Zeile und ist hoeher -
            // die Punkte landen dadurch hoeher als vorher.
            categoryButton.setBounds (catCol.getX(), row1.getY() - 12, cw, kRowH * 2 + kRowGap + 10);
            const int dotsW = (kNumCategories + 1) * 10 + 6;
            catDots.setBounds (catCol.getX() + (cw - dotsW) / 2, categoryButton.getBottom() - 4, dotsW, 12);
            categoryButton.setVisible (showMutateCategories);
            catDots.setVisible (showMutateCategories);
        }

        // ===== Runde 157 (User: Header "H1") =====
        // Drei Spalten, die Trennlinien laufen durch BEIDE Zeilen:
        //   Power Wuerfel Life Mod  LCR  |  Undo  Redo  |  Settings
        //   <  [ Preset-Name ]  >  Save  |  A/B   Copy  |  Reset
        // Spalte 1 = spielen / Preset (Save direkt am Namen), Spalte 2 =
        // Verlauf und Vergleich, Spalte 3 = Einstellungen und Zuruecksetzen.
        // Delete ist ins Preset-Menue gewandert (Runde 156).
        juce::ignoreUnused (kFileW, kStateW, kGroupGap, kNameW);
        constexpr int kCol3W = kHamburgerW;
        constexpr int kCol2W = kABW + kGap + kCopyW;                      // breiter als Undo+Redo
        constexpr int kCol1W = kBlockW - kCol3W - kCol2W - kSepBlockW * 2;
        auto colRow1 = row1;
        auto colRow2 = row2;
        auto c1r1 = colRow1.removeFromLeft (kCol1W);
        auto c1r2 = colRow2.removeFromLeft (kCol1W);
        colRow1.removeFromLeft (kSepGap);  colRow2.removeFromLeft (kSepGap);
        const int sepXa = colRow1.getX();
        colRow1.removeFromLeft (kSepW + kSepGap);  colRow2.removeFromLeft (kSepW + kSepGap);
        auto c2r1 = colRow1.removeFromLeft (kCol2W);
        auto c2r2 = colRow2.removeFromLeft (kCol2W);
        colRow1.removeFromLeft (kSepGap);  colRow2.removeFromLeft (kSepGap);
        const int sepXb = colRow1.getX();
        colRow1.removeFromLeft (kSepW + kSepGap);  colRow2.removeFromLeft (kSepW + kSepGap);
        auto c3r1 = colRow1;
        auto c3r2 = colRow2;

        globalRowSeparatorX.clearQuick();
        globalRowSeparatorX.add (sepXa);
        globalRowSeparatorX.add (sepXb);
        globalRowSeparatorTop    = row1.getY() - 3;
        globalRowSeparatorBottom = row2.getBottom() + 3;
        presetRowSeparatorX.clearQuick();

        // Spalte 1, Zeile 1: Live-Gruppe links, LCR rechtsbuendig.
        {
            auto live = c1r1.removeFromLeft (kLiveW);
            globalBypassButton.setBounds (live.removeFromLeft (kIconBtnW));
            live.removeFromLeft (kGap);
            globalChaosButton.setBounds (live.removeFromLeft (kDiceW).withSizeKeepingCentre (kDiceW, kLifeW));   // Runde 153: Hoehe wie LIFE
            globalChaosSectionsButton.setBounds ({});
            live.removeFromLeft (kGap);
            lifeSlider.setBounds (live.removeFromLeft (kLifeW).withSizeKeepingCentre (kLifeW, kLifeW));
            live.removeFromLeft (kGap);
            globalModBypassButton.setBounds (live);
            globalBreatheButton.setVisible (false);
            globalBreatheButton.setBounds ({});
            globalGalaxyActivateButton.setBounds (c1r1.removeFromRight (kGalaxyW));
        }
        // Spalte 1, Zeile 2: < Name > Save
        {
            globalSaveSizeButton.setBounds (c1r2.removeFromRight (kUndoBtnW));
            c1r2.removeFromRight (kGap + 2);
            presetPrevButton.setBounds (c1r2.removeFromLeft (kPArrowW));
            c1r2.removeFromLeft (kPSmallGap);
            presetNextButton.setBounds (c1r2.removeFromRight (kPArrowW));
            c1r2.removeFromRight (kPSmallGap);
            presetNameButton.setBounds (c1r2.reduced (0, 1));
        }
        // Spalte 2: Undo/Redo mittig ueber A/B + Copy.
        {
            auto u = c2r1.withSizeKeepingCentre (kUndoBtnW * 2 + kGap, c2r1.getHeight());
            undoButton.setBounds (u.removeFromLeft (kUndoBtnW));
            u.removeFromLeft (kGap);
            redoButton.setBounds (u.removeFromLeft (kUndoBtnW));
            globalABButton.setBounds (c2r2.removeFromLeft (kABW));
            c2r2.removeFromLeft (kGap);
            abCopyButton.setBounds (c2r2.removeFromLeft (kCopyW));
        }
        // Spalte 3: Settings ueber Reset, beide mittig.
        presetMenuButton.setBounds (c3r1);
        globalResetButton.setBounds (c3r2.withSizeKeepingCentre (kUndoBtnW, c3r2.getHeight()));
        presetDeleteButton.setVisible (false);
        presetDeleteButton.setBounds ({});
    }

    auto area = juce::Rectangle<int> (0, 0, kDesignW, kDesignH).reduced (kOuterMargin);
    area.removeFromTop (kTitleBarH); // Titelbereich (Logo + Wortmark + Claim-Zeile)
    area.removeFromTop (10); // etwas Luft zwischen Titelzeile und erster Regler-Reihe
    // ===== HINWEISZEILE GANZ UNTEN =====
    // User: "ganz unten links ein dezentes ?-Icon, und die Hover-Infos sollen
    // dort in der Zeile unter dem Footer stehen statt direkt an der Maus."
    // Ein fester Streifen ueber die volle Breite; alles darueber rueckt
    // entsprechend nach oben.
    {
        const int hintH = 22;
        auto strip = area.removeFromBottom (hintH);
        area.removeFromBottom (6);
        // Runde 151 (User): das "?" bleibt hier unten links vor der Hinweiszeile.
        helpButton.setBounds (strip.removeFromLeft (hintH));
        strip.removeFromLeft (8);
        hintBarArea = strip;
        // Auto-Gain-Anzeige rechts in derselben Zeile, samt Klickflaeche.
        {
            // Runde 110: die Anzeige ist in den Footer umgezogen.
            autoGainReadoutArea = {};
        }
    }

    // -- Linke Spalte: Goniometer + Korrelationsmesser darunter, gemeinsam
    //    vertikal zentriert. Das Goniometer bleibt quadratisch (Kappe
    //    weiterhin bei 500), der Balken bekommt eine feste kleine Hoehe.
    //    Die Einklapp-Option wurde wieder entfernt (User-Feedback: "Sieht
    //    nicht gut aus und macht glaub zu viele Probleme") - Goniometer und
    //    Korrelationsmesser sind jetzt wieder immer sichtbar; ob sie
    //    laufen/CPU verbrauchen, steuert weiterhin applyVisualsVisibility()
    //    ueber die beiden Hamburger-Menue-Schalter.
    // Der Korrelationsmesser hat KEINE eigene Zeile mehr (User-Feedback:
    // "Correlations Meter ist viel zu klobig, zu amateurhaft... vielleicht
    // einfach IN das obere Feld ganz unten mit einbauen... gar keinen extra
    // Kasten dafuer"). Er liegt jetzt transparent ueber dem unteren
    // Innenrand des Starfields (siehe corrOverlay weiter unten) und braucht
    // deshalb weder Hoehe noch Abstand im vertikalen Aufbau der Spalte.
    const int correlationBarH = 0;
    const int correlationGap = 0;
    // Hoehe/Innenabstand des Overlay-Streifens INNERHALB des Starfields.
    const int corrOverlayH = 14;
    const int corrOverlayInset = 10;

    // Unter dem Korrelationsmesser: Input-/Output-Pegelanzeige, Mono-/Dry-
    // Icon und der Volume-Regler ALLE IN EINER EINZIGEN ZEILE (User-Wunsch:
    // "Es muss unten alles in einer Zeile sein") - vorher lagen die Meter
    // in 2 eigenen Zeilen oberhalb der Icon-Reihe, jetzt sitzen sie als
    // kompakte, duennere Balken-Spalte ganz links in DERSELBEN Zeile. Der
    // globale Bypass-Icon-Button ist NICHT mehr Teil dieser Zeile - er sitzt
    // wieder oben im Header links von Mutate (siehe layoutContent() weiter
    // oben, globalBypassButton).
    const int controlRowGap = 12;
    const int controlRowH = 44;
    const int belowCorrH = controlRowGap + controlRowH;

    // Zeile ueber dem Sternenfeld: Kategorie-Chips links, A/B/C rechts.
    // Gehoert zum Block und drueckt Sternenfeld + Footer entsprechend nach
    // unten (User: "Kategorien groesser, dafuer Starfield runter").
    // Runde 55: aus der 30 px hohen Chip-Zeile wird eine 18 px hohe
    // Infozeile - die Differenz geht an das (quadratische, hoehenbegrenzte)
    // Sternenfeld, es wird dadurch in BEIDE Richtungen groesser.
    const int chipRowH = 18, chipRowGap = 6;
    const int gonioSize = juce::jmin (500, area.getHeight() - correlationBarH - correlationGap - belowCorrH - chipRowH - chipRowGap);

    auto leftColumn = area.removeFromLeft (gonioSize);
    area.removeFromLeft (20);

    // Runde 119 (User): das Smart-Profil steht mittig zwischen dem rechten
    // Rand des Sternenfelds und dem Pfeil "vorheriges Preset" - nicht
    // zwischen Slogan und Kopfzeile (Runde 115 war falsch verstanden).
    if (categoryButton.getWidth() > 0)
    {
        const int leftEnd = leftColumn.getRight();
        const int arrowX  = presetPrevButton.getX() + 4;   // sichtbarer Pfeil
        const int midX    = (leftEnd + arrowX) / 2;
        categoryButton.setBounds (categoryButton.getBounds().withX (midX - categoryButton.getWidth() / 2));
        catDots.setBounds (catDots.getBounds().withX (midX - catDots.getWidth() / 2));
    }

    const int blockH = chipRowH + chipRowGap + gonioSize + correlationGap + correlationBarH + belowCorrH;
    auto block = leftColumn.withSizeKeepingCentre (gonioSize, blockH);
    auto chipRow = block.removeFromTop (chipRowH);
    block.removeFromTop (chipRowGap);
    auto gonioArea = block.removeFromTop (gonioSize);
    goniometer.setBounds (gonioArea);
    // Zahnrad oben rechts im Feld, Panel darunter (innerhalb des Feldes).
    {
        const int gearS = 34;   // 22 -> 34: groessere Klickflaeche (User); das Icon selbst bleibt klein
        viewGearButton.setBounds (gonioArea.getRight() - gearS - 4, gonioArea.getY() + 4, gearS, gearS);
        // Was das gewaehlte Profil tut - eine Zeile, dort wo frueher die
        // vier Chips standen.
        {
            const int ih = juce::jmin (16, chipRow.getHeight());
            auto iconArea = chipRow.removeFromLeft (18);
            smartInfoToggle.setBounds (iconArea.withSizeKeepingCentre (ih, ih));
            smartInfoToggle.setVisible (showMutateCategories && mutateCategoryValue > 0);   // Runde 114: ohne Profil kein (i)
            chipRow.removeFromLeft (6);
            smartInfoLabel.setBounds (chipRow);
            smartInfoLabel.setVisible (showMutateCategories && smartInfoVisible && mutateCategoryValue > 0);
        }
        // Panel NICHT ueber dem Sternenfeld (User: "man sieht zu wenig"),
        // sondern rechts ueber der Sektionsspalte; die wird waehrenddessen
        // abgedunkelt (siehe paintOverContent()). So bleibt das Feld frei,
        // waehrend man an seinen Einstellungen dreht.
        const int panelW = 390;   // groesser (User)
        const int panelH = 452 - 30 - 68 + 46 + 38;   // eine Zeile mehr: "Stars" Show/Hide (User)
        viewPanel.setBounds (gonioArea.getRight() + 20, gonioArea.getY(), panelW, panelH);
        viewGearButton.toFront (false);
    }

    // Settings-Panel: mittig ueber allem. Breiter als das View-Panel, weil
    // drei Spalten nebeneinander stehen.
    {
        const int sw = 760, sh = 630;   // groesser, Platz fuer die Theme-Vorschau (User)
        // Runde 58 (User: "jetzt ist zwar die gesamte Flaeche genutzt, aber
        // auch alles am aeusseren Rand"): wieder eine Karte, aber deutlich
        // grosszuegiger als frueher - genug Rand, damit sie als eigenes Blatt
        // ueber der Oberflaeche liegt.
        juce::ignoreUnused (sw, sh);
        const int pw = kDesignW - 150;
        const int ph = kDesignH - 96;
        settingsPanel.setBounds ((kDesignW - pw) / 2, (kDesignH - ph) / 2, pw, ph);
        // Back Panel: etwas kleiner als die Einstellungen - es ist ein
        // Typenschild, kein Arbeitsbereich.
        const int bw = juce::jmin (620, kDesignW - 160);
        const int bh = juce::jmin (470, kDesignH - 90);
        backPanel.setBounds ((kDesignW - bw) / 2, (kDesignH - bh) / 2 - 8, bw, bh);
        // Runde 149: Save-Preset-Karte, mittig.
        savePanel.setBounds ((kDesignW - 460) / 2, (kDesignH - 276) / 2 - 8, 460, 276);
        if (tourOverlay.isVisible())
            tourOverlay.setBounds (content.getLocalBounds());
        settingsBackdrop.setBounds (0, 0, kDesignW, kDesignH);
    }

    // Korrelationsmesser als transparentes Overlay im unteren Innenbereich
    // des Starfields (kein eigener Kasten, kein eigener Platz in der Spalte).
    // correlationMeter wird NACH dem Goniometer zu content hinzugefuegt und
    // liegt dadurch automatisch darueber.
    correlationMeter.setBounds (gonioArea.getX() + corrOverlayInset,
                                 gonioArea.getBottom() - corrOverlayInset - corrOverlayH,
                                 gonioArea.getWidth() - corrOverlayInset * 2,
                                 corrOverlayH);

    block.removeFromTop (controlRowGap);
    {
        auto controlRow = block.removeFromTop (controlRowH);

        // ===== FOOTER-ZEILE, komplett neu aufgebaut =====
        // User-Feedback: "Die untere Zeile links (in, out, mono usw.) sieht
        // amateurhaft aus... Die Icons und Schriften wirken amateurhaft
        // platziert" + "Die Schrift bei IN/OUT Meter passt nicht zum Rest.
        // Soll gleich sein."
        //
        // Drei Regeln, die den unruhigen Eindruck beheben:
        //  1) EIN Schriftschnitt fuer alles. Vorher war IN/OUT auf 9pt
        //     heruntergesetzt, waehrend MONO/VOL die normale paramFont (12pt
        //     bold) benutzten - zwei verschiedene Schriftgroessen auf engstem
        //     Raum sind der Hauptgrund fuer den amateurhaften Eindruck. Jetzt
        //     ueberall dieselbe paramFont (siehe Konstruktor).
        //  2) EINE gemeinsame Grundlinie fuer alle Beschriftungen unter den
        //     Bedienelementen (MONO, DRY, VOL).
        //  3) Jedes Bedienelement hat GENAU EIN eigenes Label. Vorher teilten
        //     sich Mono-Icon und Dry-Icon EIN einziges, ueber beide gespanntes
        //     "MONO"-Label - das Dry-Icon war dadurch unbeschriftet und der
        //     Text stand mittig zwischen zwei verschiedenen Funktionen.
        //
        // Die Reihe sitzt weiterhin in der rechten Haelfte unter dem
        // Starfield, von dessen Mitte bis buendig an den rechten Rand.
        // Aufteilung der Footer-Zeile (User-Wunsch): der bisherige Inhalt
        // (Meter, Mono, Dry, Vol) wandert in die LINKE Haelfte, PRISM bekommt
        // die rechte - dort, wo vorher die Icons sassen.
        const int rowMid   = controlRow.getX() + controlRow.getWidth() / 2;
        const int rowLeft  = controlRow.getX();
        const int rowTop   = controlRow.getY();
        const int rowH     = controlRow.getHeight();

        const int labelH   = 14;
        const int labelGap = 2;
        const int iconSize = juce::jlimit (18, 26, rowH - labelH - labelGap);

        // ===== Gleiche Abstaende (User) =====
        // Mono - Dry - Mix - Vol und der Abstand von Vol zur PRISM-Kachel
        // sind alle gleich (kGap). Die Reihe wird von RECHTS her gesetzt,
        // beginnend an der PRISM-Kachel; was links uebrig bleibt, bekommt
        // der Meter-Block - und dessen Abstand zu MONO ist bewusst groesser,
        // damit die Schrift nicht an den Balken klebt.
        // Die Reihe wurde ab der MITTE nach links gesetzt, weil rechts frueher
        // die PRISM-Kachel sass. Die ist weg - deshalb ueberlappten IN/OUT und
        // MONO. Jetzt beginnt die Reihe an der rechten Kante.
        juce::ignoreUnused (rowMid);
        // ===== Runde 150 (User: "1 find ich am besten" + Variante 3) =====
        // Drei Gruppen wie im Header, durch feine Linien getrennt:
        //   ?  IN/OUT-Meter  |  Mono  Dry  |  Mix  Pan  Vol  AG
        // Die Reihe fuellt die ganze Breite: feste Abstaende in den Gruppen,
        // die Luft an den Trennlinien waechst mit, der Rest geht an die Meter.
        const int rowRight = controlRow.getRight();
        // Runde 151 (User: "footer kaputt"): die Reihe ist nur ~380 px breit -
        // mit grosszuegigen Abstaenden blieb fuer die Meter nichts uebrig und
        // sie liefen in Mono hinein. Jetzt wird gerechnet: erst die Mindest-
        // Abstaende, die Meter bekommen mindestens 70 px, was dann noch frei
        // ist, verteilt sich auf Abstaende und Meter.
        constexpr int kAgW = 36, kAgH = 22;
        const int blockH    = iconSize + labelGap + labelH;
        const int blockTop  = rowTop + (rowH - blockH) / 2;
        const int meterLabelW = 26;
        const int helpBlock   = 0;   // Runde 151 (User): das "?" gehoert NICHT in den Footer
        const int iconsW      = iconSize * 5 + kAgW;
        int kGap = 12, pad = 9;
        {
            const int fixedMin = helpBlock + meterLabelW + iconsW + kGap * 4 + pad * 4;
            int spare = controlRow.getWidth() - fixedMin - 70;          // 70 = Mindestbreite Meter
            if (spare > 0)
            {
                const int addPad = juce::jmin (spare / 3 / 4, 10);        // ein Drittel in die Trennlinien
                pad  += addPad;  spare -= addPad * 4;
                const int addGap = juce::jmin (spare / 3 / 4, 6);         // etwas in die Gruppen
                kGap += addGap;  spare -= addGap * 4;
            }
        }
        juce::ignoreUnused (kAgH);

        juce::Component* elems[6]  = { &monoCheckButton, &monoDryButton, &mixSlider, &panSlider, &volSlider, &autoGainButton };
        juce::Label*     labels[6] = { &monoCheckLabel,  &monoDryLabel,  &mixLabel,  &panLabel,  &volLabel,  &autoGainLabel  };
        int right = rowRight;
        for (int i = 5; i >= 0; --i)
        {
            const bool isAg = elems[i] == &autoGainButton;
            const int w = isAg ? kAgW : iconSize;
            const int x = right - w;
            // MIX, VOL und PAN 3px groesser als die beiden Icons (User) -
            // wachsen um ihre Mitte, die Luecken bleiben gleich.
            if (isAg)
                elems[i]->setBounds (x, blockTop + (iconSize - kAgH) / 2, kAgW, kAgH);
            else if (elems[i] == &volSlider || elems[i] == &mixSlider || elems[i] == &panSlider)
                elems[i]->setBounds (juce::Rectangle<int> (x, blockTop, iconSize, iconSize).expanded (3));
            else
                elems[i]->setBounds (x, blockTop, iconSize, iconSize);
            labels[i]->setBounds (x - kGap / 2, blockTop + iconSize + labelGap, w + kGap, labelH);
            right = x - kGap;
            if (i == 2)                       // zwischen Mix und Dry: Trennlinie
            {
                right = x - pad;
                footerSepX[1] = right;
                right -= pad;
            }
        }
        footerSepX[0] = right + kGap - pad;   // links von Mono
        footerSepTop  = blockTop - 2;
        footerSepBottom = blockTop + blockH + 2;

        // Meter-Block: IN oben, OUT darunter, unter OUT eine feine dB-Skala.
        {
            const int meterRowH  = 14;
            const int meterVGap  = 3;
            const int scaleH     = 11;
            const int meterLeft  = rowLeft + helpBlock;
            const int meterRight = footerSepX[0] - pad;
            const int stackH = meterRowH * 2 + meterVGap + scaleH;
            // Die beiden Balken stehen mittig zu den Icons, die Skala haengt
            // darunter auf Hoehe der Beschriftungen (Mono, Mix, ...).
            const int barsH = meterRowH * 2 + meterVGap;
            auto stack = juce::Rectangle<int> (meterLeft, blockTop + iconSize / 2 - barsH / 2,
                                               juce::jmax (30, meterRight - meterLeft), stackH);

            auto inRow = stack.removeFromTop (meterRowH);
            inputMeterLabel.setBounds (inRow.removeFromLeft (meterLabelW));
            volInputMeter.setBounds (inRow);

            stack.removeFromTop (meterVGap);

            auto outRow = stack;
            outputMeterLabel.setBounds (outRow.removeFromLeft (meterLabelW).withHeight (meterRowH));
            volOutputMeter.setBounds (outRow);   // Balken oben, Skala darunter
            volOutputMeter.showScale = true;
        }

        // ===== PRISM in der rechten Footer-Haelfte =====
        // Bewusst NICHT als siebte Sektion im Raster rechts: PRISM besteht den
        // Sektions-Test nicht. Alle sechs Sektionen haben Solo, Lock und Mod,
        // weil jede fuer sich einen Klang erzeugt, den man isoliert hoeren
        // kann. "Solo Prism" ergaebe nichts - PRISM erzeugt selbst keinen
        // Klang, es begrenzt nur, WO die anderen wirken. Es liegt quer ueber
        // Orbit, Size, Boost und Width statt neben ihnen, und gehoert damit zu
        // den globalen Werkzeugen hier unten.
        {
            // Runde 31: Die Focus-Leiste ist raus (siehe prismActive im
            // Prozessor). Uebrig bleibt der Wing-Knopf - er stand nur
            // zufaellig in derselben Kachel und hat mit dem Focus nichts zu
            // tun. Er rueckt an den Anfang der frei gewordenen Flaeche; der
            // Rest bleibt bewusst leer, bis entschieden ist, was dort
            // hinkommt.
            const int blockH0  = iconSize + labelGap + labelH;
            const int bandTop  = rowTop + (rowH - blockH0) / 2 + 3;
            const int bandH    = blockH0 - 3;
            prismBand.setVisible (false);
            prismOnButton.setVisible (false);
        }
    }

    auto rightColumn = area;

    // 4 Zeilen: LCR+Polarity, Warp+Size, Flow, und ganz unten die neue,
    // bewusst kleinere Position-Zeile (sekundaere Sektion, siehe Chat).
    const int rowGap = 16;
    // Etwas groesser als vorher (22 -> 24), zusammen mit weniger Innenabstand
    // bei Power-/Mod-Icon unten (User-Feedback: "Mod/On-Off Icon ein
    // kleines bisschen groesser, Solo ein ganz kleines bisschen groesser").
    const int headerH = 24;
    const int frameGap = 14;
    // Etwas hoeher als vorher (92) - User-Feedback: die Regler sassen zu eng
    // am unteren Rahmenrand. Der Puffer entsteht automatisch weiter unten
    // durch das vertikale Zentrieren der Knob-Slots in der jetzt groesseren
    // Restflaeche.
    // Die vierte Zeile ist weg. VISION war eine eigene Sektion fuer vier
    // Regler, von denen zwei gestrichen sind (Width, Elevate) - fuer zwei
    // uebrige lohnt kein eigener Rahmen. Tilt sitzt jetzt bei PARALLAX
    // (beides schiebt das Bild zur Seite, einmal ueber Zeit, einmal ueber
    // Pegel), Depth bei DIMENSION (beides beschreibt die Groesse des Raums).
    // RAYE rueckt dafuer neben HYPERDRIVE in die dritte Zeile.
    juce::Rectangle<int> rayRowArea;
    // Zeile 1 (LCR + Polarity) etwas hoeher, da der Orbit-Kegel Hoehe
    // braucht; Zeile 3 (Flow) etwas niedriger, da dort nur kompakte Regler/
    // Buttons ohne grosse Vertikal-Anforderung sitzen.
    const int totalH = rightColumn.getHeight() - rowGap * 2;
    // Hyperdrive (Zeile 3) bekommt etwas mehr Hoehe (User) - dort sitzt der
    // Speed-Regler mit dem RAYE-Pair-Ring, der vorher an der Kante klemmte.
    constexpr int kVariant = SPACEX_ROW2_VARIANT;
    // Variante A braucht in Reihe 2 Platz fuer zwei Reglerebenen.
    // Runde 55 (User): Reihe 3 (Hyperdrive + RAYE) war im Vergleich zu
    // Reihe 1 gedrueckt. Beide oberen Reihen geben eine Kleinigkeit ab -
    // bewusst nur ein gutes Prozent, sonst kippt das Verhaeltnis.
    // Runde 108 (User): Reihe 3 noch "ein kleines bisschen hoeher", damit das
    // Phaser-Feld so gross wird wie die in Micropitch und Mid-Side. Reihe 1
    // gibt ein Prozent ab (~9 px) - sie ist die hoechste.
    const float row1Frac = (kVariant == 1) ? 0.330f : 0.362f;
    const float row2Frac = (kVariant == 1) ? 0.420f : 0.328f;
    const int row1H = juce::roundToInt ((float) totalH * row1Frac);
    const int row2H = juce::roundToInt ((float) totalH * row2Frac);
    const int row3H = totalH - row1H - row2H;

    // Legt Power-Button + Solo-Icon + Titel-Label oben links in einen Rahmen
    // und liefert den verbleibenden Innenbereich fuer die Regler zurueck.
    // Klickflaeche eines Sektionsnamens auf die Textbreite begrenzen - dieselbe
    // Regel wie in layoutHeader(). Vision und RAYE bauen ihren Kopf von Hand
    // und bekamen dadurch die ganze Restbreite (User: "clickbare range noch zu
    // weit nach rechts").
    auto fitTitle = [] (juce::Label& title, juce::Rectangle<int> area)
    {
        // Runde 53 (User: "Zeilenabstand unregelmaessig ... teilweise
        // abgeschnitten"): Titel werden NIE mehr gestaucht. JUCE quetscht
        // Text horizontal, sobald er nicht passt - dadurch stehen die
        // Buchstaben in einem Titel enger als im naechsten, und genau das
        // sah ungleichmaessig aus. Stattdessen wird die Schrift in halben
        // Punkten kleiner, bis der Name ganz hineinpasst. Wichtig: IMMER von
        // der Basisgroesse aus rechnen, sonst schrumpft ein Titel bei jedem
        // Layout-Durchlauf weiter.
        auto f = sectionTitleFont();
        const int maxW = juce::jmax (40, area.getWidth());
        // Der Zuschlag deckt Rundung und Kerning des letzten Zeichens ab; der
        // Innenrand des Labels ist in styleTitle() auf 0 gesetzt.
        auto widthOf = [&f, &title] { return juce::GlyphArrangement::getStringWidthInt (f, title.getText()) + 6; };
        int textW = widthOf();
        while (textW > maxW && f.getHeight() > 9.5f)
        {
            f = f.withHeight (f.getHeight() - 0.5f);
            textW = widthOf();
        }
        title.setFont (f);
        title.setMinimumHorizontalScale (1.0f);
        title.setBounds (area.withWidth (juce::jmin (maxW, textW)));
    };

    auto layoutHeader = [&] (juce::Rectangle<int> frame, juce::TextButton& powerBtn, juce::TextButton& soloBtn, juce::Label& title, juce::TextButton* modBtn = nullptr, juce::Slider* modDepthSlider = nullptr, juce::TextButton* lockBtn = nullptr, juce::TextButton* filterBtn = nullptr) -> juce::Rectangle<int>
    {
        auto header = frame.removeFromTop (headerH);
        // Power-Button aus dem Header entfernt (User-Wunsch, Layout-Engpass-
        // Loesung "Option 1": "On/Off Button in den Sections entfernen. Name
        // der Sections nach links ... Da section sowieso angeht mit klick
        // auf name waere das eine Option.") - der Klick auf den Titel macht
        // exakt dasselbe (siehe setupClickableTitle()/mouseUp()), daher
        // funktional verlustfrei. Der Button selbst existiert im Code
        // unveraendert weiter (APVTS-Attachment, Solo-Reset-Logik) und wird
        // nur unsichtbar/aus dem Layout genommen - keine doppelte Buchhaltung
        // fuer denselben Zustand noetig.
        powerBtn.setVisible (false);
        // Runde 30, neue Ordnung: NAME ganz links, Bedienelemente ganz rechts.
        //
        // Der frueher getestete Versuch "Name zuerst" war daran gescheitert,
        // dass Solo dann ganz rechts sass ("Name und Solo ist jetzt zu weit
        // auseinander") - Solo klickt man staendig. Mit dem Wegfall von Solo
        // faellt dieser Einwand weg: rechts steht nur noch das Lock, und das
        // setzt man einmal und laesst es liegen.
        //
        // Lock ist der ANKER und sitzt immer ganz aussen rechts. Dadurch
        // stehen die Lock-Icons ueber alle Sektionen hinweg auf einer Linie,
        // egal wie viel sonst noch im Kopf liegt. Das Mod-Paar schiebt sich
        // links davor, mit etwas Luft dazwischen, damit es als eigene Gruppe
        // gelesen wird. Der Name bewegt sich dabei nie.
        soloBtn.setVisible (false);
        // Lock steht GANZ LINKS, direkt vor dem Namen (User).
        if (lockBtn != nullptr)
        {
            const int lockSize = juce::roundToInt (headerH * 0.72f);
            auto lockArea = header.removeFromLeft (lockSize);
            header.removeFromLeft (8);
            lockBtn->setBounds (lockArea.withSizeKeepingCentre (lockSize, lockSize));
        }
        // Optionales Mod-Icon (Sinuswelle) + Tiefe-Regler ganz rechts im
        // Header, nur bei den drei Sektionen mit LFO-Modulation (Timewarp/
        // Dimension/Hyperdrive). War zunaechst genauso klein wie das Icon
        // selbst - User-Feedback danach: "zu klein" - jetzt deutlich groesser
        // (36px statt 22px), darf dafuer leicht ueber die duenne Header-Zeile
        // hinausragen (vertikal mittig zentriert, ausreichend Puffer im
        // Rahmen darunter vorhanden).
        // Runde 101 (User: "Mod Regler und Mod Icons weg"): beide sind aus
        // den Sektionskoepfen verschwunden. Was sie konnten, macht jetzt der
        // globale Mod-Schalter zusammen mit LIFE.
       #if SPACEX_TUNE
        // Tune-Build (Runde 104): nur der Tiefe-Regler kommt zurueck, damit
        // Paul die festen Werte pro Sektion einstellen kann. Das Mod-Icon
        // bleibt weg - an/aus macht weiter der globale Schalter.
        if (modDepthSlider != nullptr)
        {
            const int knobSize = 36;
            auto depthArea = header.removeFromRight (knobSize);
            header.removeFromRight (3);
            modDepthSlider->setVisible (true);
            modDepthSlider->setBounds (depthArea.withSizeKeepingCentre (knobSize, knobSize));
        }
       #else
        if (modDepthSlider != nullptr) { modDepthSlider->setVisible (false); modDepthSlider->setBounds ({}); }
       #endif
        if (modBtn != nullptr)         { modBtn->setVisible (false);         modBtn->setBounds ({}); }
        // Filter-Symbol (nur Galaxy und Dimension): direkt links neben dem
        // Mod-Icon, gleiche Groessenordnung wie das Lock-Icon.
        if (filterBtn != nullptr)
        {
            if ((bool) filterBtn->getProperties().getWithDefault ("headerPill", false))
            {
                // Runde 105: Text-Pille im Kopf (x2 in MID-SIDE) - dieselbe
                // Groesse wie FAST und PAIR beim Phaser.
                const int pw = (int) filterBtn->getProperties().getWithDefault ("headerPillW", 62);   // x2 mit Kurve
                const int ph = juce::jmin (24, headerH);
                auto fArea = header.removeFromRight (pw);
                header.removeFromRight (6);
                filterBtn->setBounds (fArea.withSizeKeepingCentre (pw, ph));
            }
            else
            {
                const int fs = juce::roundToInt (headerH * 0.80f);
                auto fArea = header.removeFromRight (fs);
                header.removeFromRight (5);
                filterBtn->setBounds (fArea.withSizeKeepingCentre (fs, fs));
            }
        }
        // Klickflaeche des Sektionsnamens nur knapp ueber den Text hinaus
        // (User: "bei Hyperdrive ist es zu viel leere Klickflaeche - man
        // merkt es, wenn bei Section off der Name beim Drueberfahren
        // leuchtet"). Betrifft vor allem Polarity, Hyperdrive und RAYE, wo
        // rechts vom Namen nichts mehr kommt.
        {
            const int textW = juce::GlyphArrangement::getStringWidthInt (title.getFont(), title.getText()) + 12;
            title.setBounds (header.withWidth (juce::jmax (40, juce::jmin (header.getWidth(), textW))));
        }
        frame.removeFromTop (4);
        return frame;
    };

    // Zerlegt eine Zeile in zwei gleich breite Rahmen mit Abstand dazwischen.
    // ===== Gemeinsame Regel fuer ALLE Regler-Beschriftungen =====
    // User-Feedback: "Einige Abstaende sind unregelmaessig: z.B. Position der
    // Abstand der Schrift zum unteren Rahmenrand. Timewarp und Dimension -
    // auch hier sind die Abstaende nicht identisch, obwohl die Rahmen auf
    // gleicher Hoehe nebeneinander sind."
    //
    // Ursache (war an mehreren Stellen gleich): Regler und Label wurden als
    // ZWEI unabhaengige Dinge platziert. Mal wurde der Knob in der Flaeche
    // zentriert und das Label einfach darunter gehaengt (Position-Zeile -
    // dadurch ragte das Label bis zu 14px ueber die Flaeche hinaus und klebte
    // am Rahmenrand), mal sass der Knob oben und der gesamte Rest blieb als
    // Luft unten stehen (Timewarp/Dimension).
    //
    // Loesung: Knob + Label sind EIN Block, und dieser Block wird als Ganzes
    // vertikal zentriert. Dadurch ist der Abstand nach oben und unten
    // zwangslaeufig gleich - in jeder Sektion, bei jeder Fenstergroesse.
    constexpr int kKnobLabelH = 14;
    // Runde 68 (User: "irgendwas stoert mich noch"): EINE Groesse fuer alle
    // Knoepfe, die eine Auswahl treffen - DOUBLE, SWEEP, EARLY/LATE und das
    // Bars-Feld. Vorher standen sie in drei Hoehen und vier Breiten
    // nebeneinander, obwohl sie dieselbe Aufgabe haben.
    constexpr int kChoiceW = 88;
    constexpr int kChoiceH = 30;
    auto placeKnobWithLabel = [] (juce::Rectangle<int> slot, juce::Slider& s, juce::Label& l, int knobSize)
    {
        const int blockH = knobSize + kKnobLabelH;
        const int top    = slot.getY() + juce::jmax (0, (slot.getHeight() - blockH) / 2);
        const int knobX  = slot.getX() + (slot.getWidth() - knobSize) / 2;
        s.setBounds (knobX, top, knobSize, knobSize);
        // Label bewusst ueber die volle Slot-Breite (statt nur ueber die
        // Knob-Breite) PLUS 8px in die Luecke zu beiden Nachbarn hinein,
        // damit laengere Namen (DISTANCE, ELEVATE) nicht abgeschnitten
        // werden. Zentrierter Text - die Nachbarn kommen sich nicht ins
        // Gehege.
        l.setBounds (slot.getX() - 8, top + knobSize, slot.getWidth() + 16, kKnobLabelH);
    };

    auto splitFrame = [&] (juce::Rectangle<int> row) -> std::pair<juce::Rectangle<int>, juce::Rectangle<int>>
    {
        auto left = row.removeFromLeft ((row.getWidth() - frameGap) / 2);
        row.removeFromLeft (frameGap);
        return { left, row };
    };

    // ===== Varianten: EINE Hauptgroesse, abgeleitet aus Reihe 2 (Drift) ====
    // Nur aus Reihe 2 gerechnet - nie als Minimum ueber alle Sektionen
    // (dann bestimmt RAYE alles, siehe fruehere Runde).
    int kBig = 0, kSmall = 0;
    if (kVariant != 0)
    {
        const int r2InnerW = (rightColumn.getWidth() - frameGap) / 2 - 20;
        const int r2InnerH = row2H - 20 - headerH;
        if (kVariant == 1)        // A: zwei gross oben, einer klein darunter
        {
            kSmall = 48;
            kBig   = juce::jmin (84, juce::jmin ((r2InnerW - 30) / 2,
                                                 r2InnerH - kKnobLabelH - 8 - kKnobLabelH - kSmall));
        }
        else if (kVariant == 2)   // B: zwei gross, dritter klein daneben
        {
            kSmall = 44;
            kBig   = juce::jmin (84, juce::jmin ((r2InnerW - 30 - 14 - kSmall) / 2, r2InnerH - kKnobLabelH));
        }
        else                      // C: alle drei gleich
        {
            kBig   = juce::jmin (84, juce::jmin ((r2InnerW - 30 - 14) / 3, r2InnerH - kKnobLabelH));
            kSmall = kBig;
        }
    }

    int sharedRayColW = juce::roundToInt ((float) rightColumn.getWidth() * 0.34f);
    // Runde 34 (User): links und rechts vom Bars-Kasten war noch Luft -
    // HYPERDRIVE ein kleines bisschen enger, RAYE bekommt das (nicht viel,
    // hoechstens 28 px) nach links dazu. Rechnung wie im Flow-Layout unten.
    // Vorab gerechnet, weil PARALLAX (Reihe 2) dieselbe Breite bekommt.
    {
        const int hyperW   = rightColumn.getWidth() - sharedRayColW - frameGap;
        const int knobH    = juce::jmin (110, hyperW > 0 ? row3H - 20 - headerH - 4 - 14 : 0);
        const int mvS      = juce::jmin (knobH, (kVariant == 0) ? 88 : kBig);
        const int gapH     = (kVariant == 0) ? 12 : 6;
        const int spdW     = (kVariant == 0) ? juce::jmin (72, knobH + 14) : mvS;
        const int used     = 20 + mvS + gapH + 28 + gapH + spdW + gapH + 28 + ((kVariant == 0) ? 10 : 6) + 80;
        const int spare    = hyperW - used;          // Luft links+rechts vom Kasten
        const int take     = juce::jlimit (0, 28, spare - 20);   // 10 px je Seite bleiben
        sharedRayColW += take;
    }

    // ===== Reihe 1: LCR (links) + Polarity (rechts) =========================
    auto row1 = rightColumn.removeFromTop (row1H);
    juce::Rectangle<int> lcrFrame, polFrame;
    if (kVariant == 0)
    {
        // Runde 36 (User): ECLIPSE so breit wie RAYE, Galaxy bekommt den Rest.
        auto r = row1;
        polFrame = r.removeFromRight (sharedRayColW);
        r.removeFromRight (frameGap);
        lcrFrame = r;
    }
    else
    {
        // Galaxy-Option A (User): Eclipse so breit wie RAYE (34 %), Galaxy
        // den Rest - Reihe 1 und 3 haben dann dieselbe Teilung.
        auto r = row1;
        polFrame = r.removeFromRight (juce::roundToInt ((float) r.getWidth() * 0.34f));
        r.removeFromRight (frameGap);
        lcrFrame = r;
    }
    groupLcrArea = lcrFrame;
    groupPolArea = polFrame;

    // -- LCR: Gravity (Knob) + Dimension (Kegel) - beide dynamisch an die
    //    tatsaechlich verfuegbare Hoehe/Breite des Rahmens angepasst, statt
    //    fester Pixelwerte, damit die vorher leere rechte Haelfte genutzt wird.
    auto lcrInner = layoutHeader (lcrFrame.reduced (10), lcrPowerButton, lcrSoloButton, lcrTitleLabel, &galaxyModButton, &galaxyModDepthSlider, &lcrLockButton, &lcrEqButton);
    {
        // Reihenfolge nach Wichtigkeit (User): ohne ORBIT passiert in Galaxy
        // ueberhaupt nichts, HORIZON bestimmt, worauf Orbit wirken kann, und
        // GRAVITY ist die Feinabstimmung.
        //
        // Die Rangfolge steht aber AUSSCHLIESSLICH in der Reihenfolge, nicht
        // in der Groesse (User: "nicht die beiden Regler unterschiedlich gross
        // machen"). Drei verschieden grosse Regler nebeneinander sehen unruhig
        // aus, und die schmalste Spalte war ausserdem zu eng fuer ihre
        // Beschriftung. Deshalb: der Kegel bekommt seine Breite, der Rest wird
        // in ZWEI GLEICHE Spalten geteilt, beide Regler gleich gross, und die
        // Beschriftung nutzt die volle Spaltenbreite statt nur die des Reglers.
        const int knobAreaH = lcrInner.getHeight() - 14;
        const int availW    = lcrInner.getWidth();
        const int gap       = 8;

        // Runde 48 (User): Orbit sass zu dicht am Rahmen - der ganze Block
        // rueckt nach rechts, die Regler werden entsprechend etwas schmaler.
        const int leftInset = 14;
        lcrInner.removeFromLeft (leftInset);
        const int orbitW = juce::jlimit (42, 74, juce::roundToInt ((float) (availW - leftInset) * 0.22f));
        auto orbitCol = lcrInner.removeFromLeft (orbitW);
        orbitLabel.setBounds (orbitCol.removeFromBottom (14));
        lcrInner.removeFromLeft (gap);

        const int colW  = juce::jmax (40, (lcrInner.getWidth() - gap) / 2);
        const int knobD = (kVariant == 0) ? juce::jlimit (34, 190, juce::jmin (knobAreaH, colW))
                                          : juce::jmin (kBig, juce::jmin (knobAreaH, colW));
        // Runde 60 (User): der Orbit-Fader ist genauso hoch wie die Regler
        // daneben und steht buendig mit ihnen - vorher nahm er die ganze
        // Spaltenhoehe und wirkte dadurch verschoben.
        // Runde 65 (User): der Fader wird nach OBEN laenger, die Unterkante
        // bleibt buendig mit dem Regler daneben - vorher war der ganze Fader
        // verschoben statt gewachsen.
        {
            auto orbitRect = orbitCol.withSizeKeepingCentre (orbitCol.getWidth(),
                                                             juce::jmin (orbitCol.getHeight(), knobD));
            orbitRect.setTop (orbitRect.getY() - 7);
            orbitSlider.setBounds (orbitRect);
        }

        auto horCol = lcrInner.removeFromLeft (colW);
        // Reihenfolge Orbit - Gravity - Air (User). Die mittlere Spalte
        // traegt jetzt Gravity, die rechte Air.
        gravityLabel.setBounds (horCol.removeFromBottom (14));
        gravitySlider.setBounds (horCol.withSizeKeepingCentre (knobD, juce::jmin (horCol.getHeight(), knobD)));
        lcrInner.removeFromLeft (gap);

        auto gravCol = lcrInner;
        horizonLabel.setBounds (gravCol.removeFromBottom (14));
        horizonSlider.setBounds (gravCol.withSizeKeepingCentre (knobD, juce::jmin (gravCol.getHeight(), knobD)));

        // Der Starfield-Mond orientiert sich an der Reglergroesse dieser Sektion.
        goniometer.setGravityKnobDiameter ((float) knobD);

        // Der alte Focus-Knopf sass im Zwischenraum. Focus ist weg (Runde 31),
        // also bekommt er keine Flaeche mehr.
        galaxyFilterButton.setBounds ({});
    }

    // -- Polarity: L/R deutlich groesser, dynamisch an die Rahmenbreite
    //    angepasst; die 4 Positions-Buttons darunter nutzen dieselbe
    //    Gesamtbreite wie L+R zusammen, damit beide Reihen buendig wirken.
    auto polInner = layoutHeader (polFrame.reduced (10), polPowerButton, polSoloButton, polTitleLabel, nullptr, nullptr, &polLockButton);
    {
        // L/R waren vorher ueber die halbe Rahmenhoehe gestreckt (bis 96px) -
        // im Vergleich zu allen anderen Reglern (<=110px Durchmesser, aber
        // rund) wirkte das klobig/unproportional. Jetzt feste, kleinere
        // Groesse wie Pulse/Sync (User-Feedback). Das liess aber viel
        // Leerraum unten im Rahmen stehen ("sieht seltsam aus"), da die Box
        // selbst gleich hoch blieb - deshalb wird der ganze L/R+1-4-Block
        // jetzt vertikal MITTIG im verfuegbaren Bereich platziert, statt oben
        // zu kleben.
        // Runde 110 (User): ØL/ØR sind Icon-Schalter - Ø oben, Buchstabe
        // darunter - und der Link sitzt zwischen ihnen statt in einer eigenen
        // Zeile darueber.
        // Runde 112 (User): Ø und Buchstabe nebeneinander, gleich gross.
        const int lrBtnW = 56, lrBtnH = 32;   // Runde 113: etwas groesser
        const int lrGap = 28;
        const int posBtnGap = 8;
        // Zwei Knoepfe statt vier, und sie tragen jetzt Woerter statt Ziffern -
        // 36 px waren viel zu schmal, im Build stand "EAR..." da. Zusammen
        // exakt so breit wie L+R darueber, damit der Block buendig bleibt.
        const int posBtnH = kChoiceH;
        const int gapV = 18;
        // Link-Button bekommt eine EIGENE Zeile ueber L/R statt sie zu
        // ueberlappen, UND ist jetzt genauso breit wie L+R zusammen (statt
        // eines schmalen, isolierten Icons) - liest sich als Klammer, die
        // beide Buttons darunter verbindet (User-Feedback: "ist nicht
        // richtig" zur vorherigen schmalen Variante).
        // Exakt headerH (= Groesse des On/Off-Icons, User-Feedback: "Icon
        // genau so gross wie z.B. das On/Off Icon") statt eines eigenen,
        // kleineren Werts.
        const int linkAreaH = headerH;
        const int linkGapV = 4;
        juce::ignoreUnused (linkAreaH + linkGapV + lrBtnH + gapV + posBtnH);
        // Runde 78 (User: "immer noch"): zweimal daneben, weil ich von OBEN
        // nach unten gerechnet habe. EARLY/LATE sitzt aber fest auf der
        // Regain-Linie - es ist der einzige Fixpunkt. Bleibt oben zu wenig
        // Platz, fraesst jede Verteilung von oben zuerst den Abstand nach
        // unten weg. Jetzt wird deshalb von UNTEN nach oben gesetzt: der
        // Abstand zu LATE steht zuerst fest, danach L/R, danach der Link.
        // Wird es eng, schrumpft die Link-Zeile - nicht die Luft dazwischen.
        const int lateTop  = (horizonLabel.getY() > 0 ? horizonLabel.getY() - posBtnH
                                                      : polInner.getBottom() - posBtnH);
        // Runde 86 (User): der Abstand L/R -> EARLY/LATE war kleiner als der
        // zum Link-Symbol darueber, dadurch wirkte der Block unten gedraengt.
        // L/R rueckt ein paar Pixel hoch (groesserer Wert hier), EARLY/LATE
        // ein paar tiefer (Versatz unten) - damit stehen beide Luecken gleich.
        // Runde 92 (User: "mach den riesen Abstand zwischen LR und LATE weg"):
        // gemessen waren es 32 px gegen 11 px zum Link darueber. Der Wert hier
        // wirkt +7 px (3 px Versatz von LATE auf die Regain-Linie und je 2 px
        // Innenabstand der beiden Knoepfe), 5 ergibt also dieselbe Luecke.
        const int gapToLate = 12;   // Runde 112: ohne Punkte unter PRE/POST etwas mehr Luft
        const int lrBottom  = lateTop - gapToLate;
        const int lrTop     = lrBottom - lrBtnH;

        auto lrCentered = juce::Rectangle<int> (polInner.getX(), lrTop, polInner.getWidth(), lrBtnH)
                             .withSizeKeepingCentre (lrBtnW * 2 + lrGap, lrBtnH);
        const auto oldL = lrCentered.removeFromLeft (lrBtnW);
        lrCentered.removeFromLeft (lrGap);
        const auto oldR = lrCentered;

        {
            // Mittig zwischen den beiden Ø, auf Hoehe der Icons (ohne die
            // Buchstaben darunter). Runde 154: Link bleibt exakt, wo er war.
            const int linkS = juce::jmin (lrGap - 8, linkAreaH);
            const int iconMidY = lrTop + lrBtnH / 2;
            polLinkButton.setBounds (oldL.getRight() + (lrGap - linkS) / 2, iconMidY - linkS / 2, linkS, linkS);
            juce::ignoreUnused (linkGapV);

            // Runde 154 (User): ØL und ØR 50 % groesser, mittig zum Link-Icon.
            // Die Innenkanten bleiben, wo sie waren (Abstand zum Link gleich),
            // die Knoepfe wachsen nach aussen und gleichmaessig nach oben/unten.
            // Alles andere (Link, PRE/POST) bleibt unveraendert.
            constexpr float kPolScale = 1.3f;   // Runde 157 (User): 1.5 war "etwas zu gross"
            const int bigW = juce::roundToInt ((float) lrBtnW * 1.15f);
            const int bigH = juce::roundToInt ((float) lrBtnH * kPolScale);
            const int midY = polLinkButton.getBounds().getCentreY();
            polLButton.setBounds (oldL.getRight() - bigW, midY - bigH / 2, bigW, bigH);
            polRButton.setBounds (oldR.getX(),            midY - bigH / 2, bigW, bigH);
            for (auto* b : { &polLButton, &polRButton })
                b->getProperties().set ("phaseScale", (double) kPolScale);
        }

        auto posRow = juce::Rectangle<int> (polInner.getX(), lateTop, polInner.getWidth(), posBtnH);
        juce::ignoreUnused (posBtnGap);
        // Runde 64 (User): schmaler als L + R zusammen, und die UNTERKANTE
        // liegt genau auf der Oberkante von "Regain" in der LCR-Sektion
        // daneben - so enden beide Rahmen auf derselben Linie. Die LCR-
        // Sektion wird weiter oben gelegt, horizonLabel steht also schon.
        {
            // Runde 68 (User): der Knopf war fuer sein laengstes Wort viel zu
            // breit - links und rechts stand mehr Luft als Schrift.
            // Runde 108: etwas breiter - Icon und Name stehen nebeneinander.
            auto posRect = posRow.withSizeKeepingCentre (kChoiceW + 22, posBtnH);
            if (horizonLabel.getY() > 0)
                posRect.setY (horizonLabel.getY() - posBtnH + 3);
            polPos2Button.setBounds (posRect);
            // Runde 112 (User): bei zwei Stellungen braucht es keine Punkte.
            polPosDots.setVisible (false);
            polPosDots.setBounds ({});
        }
        polPos3Button.setBounds ({});
        polPos1Button.setBounds ({});
        polPos4Button.setBounds ({});
    }

    rightColumn.removeFromTop (rowGap);

    // ===== Reihe 2: Warp/Drift (links) + Size (rechts) ======================
    auto row2 = rightColumn.removeFromTop (row2H);
    juce::Rectangle<int> driftFrame, wbFrame;
    constexpr bool kNewParallax = (kVariant == 0) && (SPACEX_PARALLAX_UI != 1);
    if (kVariant == 0)
    {
        // Runde 40 (User): PARALLAX wieder LINKS - so kommt man zuerst an die
        // Sektion, die ein Mono-Signal ueberhaupt breit macht, und Dimension
        // hat danach etwas zu tun. Die Groessen bleiben bewusst vertauscht:
        // Parallax klein (so breit wie RAYE), Dimension gross - das Auge
        // orientiert sich daran. Gilt fuer SpaceX und SpaceFX.
        // Runde 41 (User): Parallax GLEICH GROSS wie Dimension, in allen Builds.
        // Runde 49 (User: "dimension ein kleines bisschen weiter nach links
        // vergroessern ... aber nicht viel"): 46 / 54 statt genau haelftig.
        auto r2 = row2;
        // Runde 112 (User): Micropitch ein wenig schmaler, Mid-Side breiter -
        // der Rand rechts in Micropitch war groesser als links in Mid-Side.
        auto a = r2.removeFromLeft (juce::roundToInt ((float) (r2.getWidth() - frameGap) * 0.432f));   // Runde 134 (User): Mid-Side ein paar px breiter
        r2.removeFromLeft (frameGap);
        driftFrame = a; wbFrame = r2;
    }
    else
    {
        auto [a, b] = splitFrame (row2);
        driftFrame = a; wbFrame = b;
    }
    groupDriftArea = driftFrame;
    groupWidthBoostArea = wbFrame;

    // Knob-Hoehe hier bewusst genauso wie in Reihe 3 (Flow/Speed, siehe unten)
    // auf 110 gedeckelt statt die volle verfuegbare Rahmenhoehe zu nutzen -
    // vorher fuehrte die groessere Reihe 2 (35% statt 25% Hoehe) dazu, dass
    // der Regler-Bereich selbst viel hoeher als sein sichtbarer Knob war und
    // das Label dadurch mit deutlich mehr Abstand darunter landete (User-
    // Feedback: Abstand bei Drift/Shift/Expand/Boost passt nicht zu Flow/Speed).
    // Gemeinsame Regler-Groesse fuer BEIDE Rahmen dieser Reihe.
    // User-Feedback: "Timewarp und Dimension - auch hier sind die Abstaende
    // nicht identisch, obwohl die Rahmen auf gleicher Hoehe nebeneinander
    // sind." Ursache: jede Sektion rechnete ihre Knob-Groesse aus ihrer
    // EIGENEN Innenbreite minus ihrem EIGENEN Zwischenraum aus. Timewarp
    // braucht wegen des Balance-Icons 34px Zwischenraum, Dimension nur 20 -
    // dadurch wurden Dimensions Regler 7px groesser, und weil das Label
    // direkt unter dem Regler haengt, sassen die Beschriftungen der beiden
    // Rahmen auf unterschiedlicher Hoehe.
    // Jetzt wird EIN gemeinsamer Wert aus dem GROESSEREN der beiden
    // Zwischenraeume berechnet und in beiden Rahmen benutzt.
    const int row2InnerH   = driftFrame.getHeight() - 20 - headerH; // reduced(10) oben+unten, minus Header
    // Reglergroesse wie bisher aus der HALBEN Reihenbreite - Dimension ist
    // in Variante 0 jetzt breiter, die Regler sollen dadurch nicht wachsen.
    const int row2InnerW   = (kVariant == 0) ? (row2.getWidth() - frameGap) / 2 - 20
                                             : driftFrame.getWidth() - 20;
    const int row2KnobArea = juce::jmin (110, row2InnerH - kKnobLabelH);
    // Jetzt DREI Regler je Rahmen: Drift/Shift/Tilt und Size/Boost/Depth.
    const int driftGap     = 30; // Platz fuer das 24x24px-Balance-Icon
    const int wbGap        = 14;
    int row2KnobSize = (kVariant == 3) ? kBig
                                       : juce::jmin (row2KnobArea, (row2InnerW - driftGap - wbGap) / 3);
    // SpaceFX: die drei alten Parallax-Regler muessen in den schmalen Rahmen
    // passen - Dimension nimmt dieselbe Groesse, damit beide gleich aussehen.
    if (kVariant == 0 && ! kNewParallax)
        row2KnobSize = juce::jmin (row2KnobSize, (driftFrame.getWidth() - 20 - driftGap - wbGap) / 3);

    if (! kNewParallax)   // neues PARALLAX (Runde 34) nur im normalen Build
    {
        parallaxAmountSlider.setVisible (false);
        parallaxAmountLabel.setVisible (false);
        for (auto& b : parallaxModeButtons) b.setVisible (false);
    }

    if (kVariant == 1 || kVariant == 2)
    {
        auto dInner = layoutHeader (driftFrame.reduced (10), driftPowerButton, driftSoloButton, driftTitleLabel, &driftModButton, &driftModDepthSlider, &driftLockButton);
        auto wInner = layoutHeader (wbFrame.reduced (10), widthBoostPowerButton, widthBoostSoloButton, widthBoostTitleLabel, &dimensionModButton, &dimensionModDepthSlider, &widthBoostLockButton);
        dimFilterButton.setBounds ({});

        // Liefert den Slot des ersten Reglers zurueck (fuer das Balance-Icon).
        auto layoutSection = [&] (juce::Rectangle<int> inner, int topGap,
                                  juce::Slider& s1, juce::Label& l1,
                                  juce::Slider& s2, juce::Label& l2,
                                  juce::Slider& s3, juce::Label& l3) -> juce::Rectangle<int>
        {
            if (kVariant == 1)
            {
                // A: Dreieck - zwei grosse oben, der kleine mittig darunter.
                const int blockH = kBig + kKnobLabelH + 8 + kSmall + kKnobLabelH;
                auto block = inner.withSizeKeepingCentre (kBig * 2 + topGap, juce::jmin (inner.getHeight(), blockH));
                auto top = block.removeFromTop (kBig + kKnobLabelH);
                block.removeFromTop (8);
                auto a = top.removeFromLeft (kBig);
                top.removeFromLeft (topGap);
                auto b = top.removeFromLeft (kBig);
                placeKnobWithLabel (a, s1, l1, kBig);
                placeKnobWithLabel (b, s2, l2, kBig);
                placeKnobWithLabel (block.withSizeKeepingCentre (kSmall + 20, block.getHeight()), s3, l3, kSmall);
                return a;
            }
            // B: drei nebeneinander, der dritte klein. Die Beschriftungen
            // stehen auf EINER Linie - der kleine Regler sitzt dafuer unten
            // buendig mit den grossen.
            auto trio = inner.withSizeKeepingCentre (kBig * 2 + kSmall + topGap + 14, inner.getHeight());
            auto a = trio.removeFromLeft (kBig);
            trio.removeFromLeft (topGap);
            auto b = trio.removeFromLeft (kBig);
            trio.removeFromLeft (14);
            auto c = trio.removeFromLeft (kSmall);
            placeKnobWithLabel (a, s1, l1, kBig);
            placeKnobWithLabel (b, s2, l2, kBig);
            const int labelY = s1.getBounds().getBottom();
            s3.setBounds (c.getX() + (c.getWidth() - kSmall) / 2, labelY - kSmall, kSmall, kSmall);
            l3.setBounds (c.getX() - 8, labelY, c.getWidth() + 16, kKnobLabelH);
            return a;
        };

        auto dSlot = layoutSection (dInner, 30, driftSlider, driftLabel, bendSlider, bendLabel, offsetSlider, offsetLabel);
        layoutSection (wInner, (kVariant == 1) ? 30 : 14, sideWidthSlider, sideWidthLabel,
                       sideBoostSlider, sideBoostLabel, distanceSlider, distanceLabel);

        constexpr int balanceIconSize = 24;
        driftBalanceButton.setBounds (dSlot.getRight() + (30 - balanceIconSize) / 2,
                                       driftSlider.getBounds().getCentreY() - balanceIconSize / 2,
                                       balanceIconSize, balanceIconSize);
    }
    else
    {
    // Runde 92 (User: "bei Parallax den Filter-Icon wieder entfernen, Bass
    // Guard immer aktiv haben"): der Schalter ist weg, der 120-Hz-Schutz
    // laeuft fest mit (siehe PluginProcessor).
    parallaxHpButton.setVisible (false);
    parallaxHpButton.setBounds ({});
    // Runde 109: Micropitch hat keine Modulation mehr - auch im Tune-Build
    // kein Tiefe-Regler.
    driftModDepthSlider.setVisible (false);
    driftModDepthSlider.setBounds ({});
    auto driftInner = layoutHeader (driftFrame.reduced (10), driftPowerButton, driftSoloButton, driftTitleLabel, &driftModButton, nullptr, &driftLockButton);

    if (! kNewParallax)
    {
        // Das Regler-Paar wird als Ganzes horizontal zentriert, damit die
        // Restbreite links und rechts gleich gross ist.
        auto trio = driftInner.withSizeKeepingCentre (row2KnobSize * 3 + driftGap + wbGap, driftInner.getHeight());
        auto driftSlot = trio.removeFromLeft (row2KnobSize);
        trio.removeFromLeft (driftGap);
        auto bendSlot = trio.removeFromLeft (row2KnobSize);
        trio.removeFromLeft (wbGap);
        auto tiltSlot = trio.removeFromLeft (row2KnobSize);

        placeKnobWithLabel (driftSlot, driftSlider,  driftLabel,  row2KnobSize);
        placeKnobWithLabel (bendSlot,  bendSlider,   bendLabel,   row2KnobSize);
        placeKnobWithLabel (tiltSlot,  offsetSlider, offsetLabel, row2KnobSize);

        // "Balance"-Icon mittig im Zwischenraum, vertikal auf Reglerhoehe.
        constexpr int balanceIconSize = 24;
        driftBalanceButton.setBounds (driftSlot.getRight() + (driftGap - balanceIconSize) / 2,
                                       driftSlider.getBounds().getCentreY() - balanceIconSize / 2,
                                       balanceIconSize, balanceIconSize);
    }
    else
    {
        // PARALLAX (Runde 34): links der Amount-Regler, rechts vier
        // Modus-Knoepfe im 2x2-Raster. Drift/Shift/Tilt und Balance sind
        // aus der Oberflaeche raus (Parameter bleiben bestehen).
        for (auto* c : std::initializer_list<juce::Component*> {
                 &driftSlider, &driftLabel, &bendSlider, &bendLabel,
                 &offsetSlider, &offsetLabel, &driftBalanceButton })
        {
            c->setVisible (false);
            c->setBounds ({});
        }

        const int gap    = 10;
        const int knobS  = juce::jmin (row2KnobSize, driftInner.getHeight() - kKnobLabelH,
                                       (driftInner.getWidth() - gap) / 2);
        const int btnGap = 6;
       #if SPACEX_PARALLAX_UI == 2
        const int btnW   = juce::jmin ((kChoiceW - 6) / 2, (driftInner.getWidth() - knobS - gap - 6) / 2);   // Runde 68: ergibt kChoiceW
       #if SPACEX_PX_DIAG_ONLY
        // Runde 95 (User: "der Abstand beider Einheiten ist zu gross, Amount
        // ist zu weit links - mach sie gemeinsam mittig"): das Feld bekommt
        // eine feste, knappe Breite; Regler + Feld stehen als EINE Einheit
        // mittig in der Sektion (siehe withSizeKeepingCentre unten).
        const int blockW = juce::jmin (118, driftInner.getWidth() - knobS - gap);
       #else
        const int blockW = btnW * 2 + btnGap;   // Runde 68: ergibt kChoiceW
       #endif
       #else
        // Fuenf Knoepfe: 3 oben, 2 darunter (Runde 45).
        const int btnW   = juce::jmin (70, (driftInner.getWidth() - knobS - gap - btnGap * 2) / 3);
        const int blockW = btnW * 3 + btnGap * 2;
       #endif
        const int btnH   = kChoiceH;   // so hoch wie EARLY/LATE (User)
        auto block = driftInner.withSizeKeepingCentre (knobS + gap + blockW, driftInner.getHeight());
        auto knobSlot = block.removeFromLeft (knobS);
        block.removeFromLeft (gap);
        placeKnobWithLabel (knobSlot, parallaxAmountSlider, parallaxAmountLabel, knobS);

       #if SPACEX_PARALLAX_UI == 2
        // Ein Klick-Knopf statt 2x2 (so breit wie zwei der Rasterknoepfe).
       #if SPACEX_PX_DIAG_ONLY
        // Runde 90/91: Icon oben, Name darunter - ohne Rahmen darf das Feld
        // die Hoehe nehmen, die die Sektion hergibt.
        // Runde 95: etwas hoeher, damit ueber dem Icon Platz fuer den
        // Schimmer bleibt - er wird an den Knopfkanten abgeschnitten.
        const int pillH = juce::jmin ((kDotsInside ? btnH + 6 : btnH) * 2 + 20,
                                      driftInner.getHeight() - 14);
       #else
        const int pillH = kDotsInside ? btnH + 6 : btnH;
       #endif
        parallaxModeButtons[0].setBounds (block.getX(), parallaxAmountSlider.getBounds().getCentreY() - pillH / 2,
                                          blockW, pillH);
        for (int i = 1; i < kPxModes; ++i) parallaxModeButtons[i].setBounds ({});
        {
            auto pb = parallaxModeButtons[0].getBounds();
            const int dotsW = kPxModes * 10 + 6;
            if (kDotsInside)
            {
                parallaxModeButtons[0].getProperties().set ("textYShift", -5.0);
                pxModeDotsArea = { pb.getCentreX() - dotsW / 2, pb.getBottom() - 13, dotsW, 11 };
            }
            else
            {
                parallaxModeButtons[0].getProperties().remove ("textYShift");
               #if SPACEX_PX_DIAG_ONLY
                // Runde 91 (User: "Schrift + Punkte dichter zusammen").
                pxModeDotsArea = { pb.getCentreX() - dotsW / 2, pb.getBottom() - 1, dotsW, 12 };
               #else
                pxModeDotsArea = { pb.getCentreX() - dotsW / 2, pb.getBottom() + 5, dotsW, 12 };
               #endif
            }
            pxModeDots.setBounds (pxModeDotsArea);
        }
       #else
        const int gridH = btnH * 2 + btnGap;
        const int gridY = parallaxAmountSlider.getBounds().getCentreY() - gridH / 2;
        for (int i = 0; i < kPxModes; ++i)
        {
            const int row = (i < 3) ? 0 : 1;
            const int col = (i < 3) ? i : i - 3;
            const int lower = kPxModes - 3;   // Anzahl in der unteren Reihe
            const int rowX = block.getX() + ((row == 0 || lower >= 3) ? 0 : (btnW + btnGap) / 2);
            parallaxModeButtons[i].setBounds (rowX + col * (btnW + btnGap), gridY + row * (btnH + btnGap), btnW, btnH);
        }
       #endif
    }

    auto wbInner = layoutHeader (wbFrame.reduced (10), widthBoostPowerButton, widthBoostSoloButton, widthBoostTitleLabel, &dimensionModButton, &dimensionModDepthSlider, &widthBoostLockButton, &msEqX2Button);
    {
        // Runde 125: An/Aus + Fader im Platz, den der (unsichtbare) x2-Knopf
        // im Kopf reserviert hat.
        auto hb = msEqX2Button.getBounds();
        msEqX2Button.setVisible (false);
        // Runde 159 (User): kein eigener An/Aus-Schalter mehr - "aus" ist
        // FLAT. Der Fader bleibt an seinem Platz, Power-Platz bleibt frei.
        hb.removeFromLeft (4);   // Runde 161 (User): Fader darf breiter sein
        msEqPowerButton.setVisible (false);
        msEqPowerButton.setBounds ({});
        msEqAmtSlider.setBounds (hb.withSizeKeepingCentre (hb.getWidth(), 18));
    }
    {
        // Runde 105: Width, Sides und das Icon-Feld des Seiten-EQ. Das Feld
        // ist so breit wie die in Micropitch und Phaser; alle drei stehen
        // gemeinsam mittig in der Sektion.
        const int eqFieldW = 118;
        const int dimGap = (kVariant == 0) ? juce::jlimit (wbGap, 34, (wbInner.getWidth() - row2KnobSize * 2 - eqFieldW) / 4) : wbGap;
        auto trio = wbInner.withSizeKeepingCentre (row2KnobSize * 2 + eqFieldW + dimGap * 2, wbInner.getHeight());
        auto swSlot = trio.removeFromLeft (row2KnobSize);
        trio.removeFromLeft (dimGap);
        auto sbSlot = trio.removeFromLeft (row2KnobSize);
        trio.removeFromLeft (dimGap);
        auto eqSlot = trio.removeFromLeft (eqFieldW);

        placeKnobWithLabel (swSlot, sideWidthSlider, sideWidthLabel, row2KnobSize);
        placeKnobWithLabel (sbSlot, sideBoostSlider, sideBoostLabel, row2KnobSize);
        distanceSlider.setBounds ({});
        distanceLabel.setBounds ({});
        {
            // Gleiche Hoehe und Lage wie das Feld in Micropitch: mittig auf
            // Hoehe der Reglermitte, Punkte direkt darunter.
            const int eqH = juce::jmin (kChoiceH * 2 + 20, wbInner.getHeight() - 14);
            msEqButton.setBounds (eqSlot.getX(), sideWidthSlider.getBounds().getCentreY() - eqH / 2, eqFieldW, eqH);
            const auto eb = msEqButton.getBounds();
            const int dotsW = LCRMSAudioProcessor::kMsEqModes * 10 + 6;
            msEqDots.setBounds (eb.getCentreX() - dotsW / 2, eb.getBottom() - 1, dotsW, 12);
        }

        dimFilterButton.setBounds ({});   // Focus ist weg (Runde 31)
    }
    }   // Ende Variante 0 / C

    rightColumn.removeFromTop (rowGap);

    // ===== Reihe 3: Flow (Auto-Pan), volle Breite ===========================
    auto row3 = rightColumn.removeFromTop (row3H);
    // RAYE rueckt hier herein - ein Drittel der Breite, genau wie in den
    // beiden Zeilen darueber eine grosse und eine kleine Sektion stehen.
    const int rayColW = sharedRayColW;
    rayRowArea = row3.removeFromRight (rayColW);
    row3.removeFromRight (frameGap);
    groupFlowArea = row3;
    auto flowInner = layoutHeader (row3.reduced (10), flowPowerButton, flowSoloButton, flowTitleLabel, &hyperdriveModButton, &hyperdriveModDepthSlider, &flowLockButton);
    // Gemeinsame Bezugshoehe fuer ALLE Elemente dieser Reihe (= Hoehe des
    // Regler-Bereichs oberhalb des Label-Streifens), damit Move/Pulse/Speed/
    // Sync/Speed-Box garantiert auf gleicher Hoehe sitzen.
    const int knobAreaH = juce::jmin (110, flowInner.getHeight() - 14);
    // Deutlich kompakter als vorher: HYPERDRIVE teilt sich die Zeile jetzt
    // mit RAYE. Pulse und Sync sind Icons statt beschrifteter Knoepfe, das
    // allein spart rund 60 px.
    const int moveSize  = juce::jmin (knobAreaH, (kVariant == 0) ? 88 : kBig);
    // "Icons und Regler enger zusammen" (User) - nur in den Varianten.
    const int hyperGap  = (kVariant == 0) ? 12 : 6;
    const int flowIconS = 28;

    // Auch hier ueber die gemeinsame Regel (siehe placeKnobWithLabel oben),
    // statt den Regler oben anzusetzen und das Label darunter zu haengen -
    // sonst sitzt der Flow-Block anders als der Speed-Block daneben, der
    // schon immer als Ganzes zentriert wurde.
    auto mv = flowInner.removeFromLeft (moveSize);
    placeKnobWithLabel (mv, movementSlider, movementLabel, moveSize);

    flowInner.removeFromLeft (hyperGap);
    auto pulseArea = flowInner.removeFromLeft (flowIconS).withHeight (knobAreaH);
    pulseButton.setBounds (pulseArea.withSizeKeepingCentre (flowIconS, flowIconS));

    flowInner.removeFromLeft (hyperGap);
    const int speedColW = (kVariant == 0) ? juce::jmin (72, knobAreaH + 14) : moveSize;
    auto speedKnobArea = flowInner.removeFromLeft (speedColW).withSizeKeepingCentre (speedColW, knobAreaH + 14);
    speedLabel.setBounds (speedKnobArea.removeFromBottom (14));
    speedRateSlider.setBounds (speedKnobArea);

    flowInner.removeFromLeft (hyperGap);
    auto syncArea = flowInner.removeFromLeft (flowIconS).withHeight (knobAreaH);
    syncButton.setBounds (syncArea.withSizeKeepingCentre (flowIconS, flowIconS));

    flowInner.removeFromLeft ((kVariant == 0) ? 10 : 6);
    auto speedBoxArea = flowInner.withHeight (knobAreaH);
    // Ohne Pfeil (User) reicht deutlich weniger Breite.
    // Runde 68 (User): dieselbe Groesse wie DOUBLE, SWEEP und EARLY - die
    // vier Auswahl-Knoepfe standen vorher in drei verschiedenen Hoehen
    // (24 / 26 / 30) und vier Breiten nebeneinander.
    speedBox.setBounds (speedBoxArea.withSizeKeepingCentre (juce::jmin (kChoiceW, speedBoxArea.getWidth()), kChoiceH));

    // ===== VISION aufgeloest ================================================
    // Die Sektion ist weg. Tilt steht jetzt in PARALLAX, Depth in DIMENSION
    // (siehe oben), Width und Elevate sind gestrichen. Alles, was nur zum
    // Rahmen gehoerte, verschwindet hier - Sichtbarkeit UND Flaeche, sonst
    // bleiben unsichtbare Klickflaechen ueber den neuen Reglern liegen.
    groupPosArea = {};
    for (auto* c : std::initializer_list<juce::Component*> {
             &posPowerButton, &posSoloButton, &posLockButton, &posTitleLabel,
             &positionModButton, &positionModDepthSlider, &posFilterButton,
             &posWidthSlider, &posWidthLabel, &elevateSlider, &elevateLabel })
    {
        c->setVisible (false);
        c->setBounds ({});
    }

    // ===== RAY =====
    // Gleicher Kopfaufbau wie Position (Solo, Lock, Titel), darunter drei
    // Elemente in einer Reihe: Staerke-Icon | Speed | Pair.
    groupRayArea = rayRowArea;
    auto rayFrame = rayRowArea.reduced (10);
    auto rayHeader = rayFrame.removeFromTop (headerH);
    rayPowerButton.setVisible (false);   // wie ueberall: Titel-Klick schaltet
    raySoloButton.setVisible (false);
    {
        // Lock links, dann der Name - dieselbe Ordnung wie in layoutHeader().
        const int lockSize = juce::roundToInt (headerH * 0.72f);
        auto lockArea = rayHeader.removeFromLeft (lockSize);
        rayHeader.removeFromLeft (8);
        rayLockButton.setBounds (lockArea.withSizeKeepingCentre (lockSize, lockSize));
    }
    // Rechts im RAYE-Kopf ist Platz (kein Mod-Icon) - dort sitzt in
    // SpaceXraye2 der Pair-Knopf.
   #if SPACEX_RAYE_UI == 1
    // Runde 54: der TITEL hat Vorrang. Erst wird gerechnet, wie breit sein
    // Name in der Basisschrift ist, und nur der Rest geht an FAST/PAIR -
    // vorher nahmen sich die beiden feste 112 px und der Name wurde gekuerzt
    // ("RA...", "PH..."), sobald die Sektion schmal war.
    const int rayTitleNeed = juce::GlyphArrangement::getStringWidthInt (sectionTitleFont(), rayTitleLabel.getText()) + 14;
    const int rayRightRoom = juce::jmax (0, rayHeader.getWidth() - rayTitleNeed);
    // Runde 110: Icon + Name brauchen etwas mehr Breite als die Chips.
    auto rayHeadRight = rayHeader.removeFromRight (juce::jmin (128, rayRightRoom));
    const auto rayPairHeaderArea = rayHeadRight.removeFromRight (juce::jmin (62, rayHeadRight.getWidth()));
    rayHeadRight.removeFromRight (5);
    const auto rayFastHeaderArea = rayHeadRight;
   #else
    const auto rayPairHeaderArea = rayHeader.removeFromRight (juce::jmin (60, rayHeader.getWidth() / 2));
    const auto rayFastHeaderArea = juce::Rectangle<int>();
   #endif
    juce::ignoreUnused (rayPairHeaderArea, rayFastHeaderArea);
    fitTitle (rayTitleLabel, rayHeader);
    rayFrame.removeFromTop (6);

    {
        const int rayKnobAreaH = juce::jmin (56, rayFrame.getHeight() - 14);
        const int gap = (kVariant == 0) ? 10 : 6;   // enger (User), nur Varianten
        const int slotW = (rayFrame.getWidth() - gap * 2) / 3;

        // Staerke-Icon: quadratisch, so gross wie der Regler daneben, als
        // eigener Block mit Label-Platz darunter (Label ist im Icon selbst
        // nicht noetig - die drei Stufen erklaeren sich durch die Fuellung).
       #if SPACEX_RAYE_UI == 1
        // Runde 49 (User: "dann speed weg. noch simpler."): nur noch zwei
        // Elemente - Amount links, Charakter rechts. Das Tempo macht der
        // Charakter, FAST im Kopf legt 30 % drauf.
        rayStrengthButton.setBounds ({});
        rayRateSlider.setBounds ({});
        rayRateLabel.setBounds ({});
        rayRateSlider.setVisible (false);
        rayRateLabel.setVisible (false);
        juce::ignoreUnused (slotW);
       #if SPACEX_PX_DIAG_ONLY
        // Runde 95 (User: "Amount + Sweep sind nicht mittig - schau immer,
        // dass beide Einheiten GEMEINSAM mittig in der Sektion stehen").
        const int halfW = juce::jmin ((rayFrame.getWidth() - gap) / 2, rayKnobAreaH);
        const int rayFieldW = juce::jmin (118, rayFrame.getWidth() - halfW - gap);
        auto rayGroup = rayFrame.withSizeKeepingCentre (halfW + gap + rayFieldW, rayFrame.getHeight());
        auto slotA = rayGroup.removeFromLeft (halfW);
        rayGroup.removeFromLeft (gap);
        auto slotC = rayGroup;
        placeKnobWithLabel (slotA, rayAmountSlider, rayAmountLabel, halfW);
       #else
        const int halfW = (rayFrame.getWidth() - gap) / 2;
        auto slotA = rayFrame.removeFromLeft (halfW);
        rayFrame.removeFromLeft (gap);
        auto slotC = rayFrame;
        placeKnobWithLabel (slotA, rayAmountSlider, rayAmountLabel, juce::jmin (halfW, rayKnobAreaH));
       #endif
        {
           #if SPACEX_PX_DIAG_ONLY
            // Runde 90 (User: "auch fuer Sweep - schau dass die Box gross
            // genug ist"): Icon oben, Name darunter, also doppelte Hoehe und
            // so breit, wie die halbe Sektion hergibt.
            // Runde 95: das Feld fuellt seinen Platz in der Gruppe - die
            // Gruppe selbst steht mittig (siehe oben).
            const int ph = juce::jmin ((kDotsInside ? kChoiceH + 6 : kChoiceH) * 2 + 20,
                                       rayFrame.getHeight() - 14);
            auto col = slotC.withSizeKeepingCentre (slotC.getWidth(), ph).translated (0, kDotsInside ? -5 : -7);
           #else
            const int pw = juce::jmin (kChoiceW, halfW);
            const int ph = juce::jmin (kDotsInside ? kChoiceH + 6 : kChoiceH, rayKnobAreaH - (kDotsInside ? 8 : 14));
            auto col = slotC.withSizeKeepingCentre (pw, ph).translated (0, kDotsInside ? -7 : -10);
           #endif
            rayCharButton.setBounds (col);
            const int dotsW = 4 * 10 + 6;
            if (kDotsInside)
            {
                rayCharButton.getProperties().set ("textYShift", -5.0);
                rayModeDots.setBounds (col.getCentreX() - dotsW / 2, col.getBottom() - 13, dotsW, 11);
            }
            else
            {
                rayCharButton.getProperties().remove ("textYShift");
               #if SPACEX_PX_DIAG_ONLY
                rayModeDots.setBounds (col.getCentreX() - dotsW / 2, col.getBottom() - 1, dotsW, 12);
               #else
                rayModeDots.setBounds (col.getCentreX() - dotsW / 2, col.getBottom() + 5, dotsW, 12);
               #endif
            }
            // Runde 55 (User: "fast und pair sind mir zu klein").
            // Runde 166 (User: "FAST zu nah am Namen"): FAST und LINK als Paar
            // rechtsbuendig, jeweils genau so breit wie Schalter + Wort.
            {
                const int h  = juce::jmin (24, rayPairHeaderArea.getHeight());
                const int cy = rayPairHeaderArea.getCentreY();
                const int wL = CustomLookAndFeel::ctlContentWidth (rayPairButton);
                const int wF = CustomLookAndFeel::ctlContentWidth (rayFastButton);
                const int right = rayPairHeaderArea.getRight() - 2;
                // Luecke zwischen den beiden: 12 px, wird es neben dem Namen
                // eng, schrumpft sie bis 6 px (der Name hat Vorrang).
                const int minLeft = rayTitleLabel.getRight() + 18;
                const int gapFL = juce::jlimit (6, 12, (right - wL - wF) - minLeft);
                rayPairButton.setBounds (right - wL, cy - h / 2, wL, h);
                rayFastButton.setBounds (right - wL - gapFL - wF, cy - h / 2, wF, h);
                juce::ignoreUnused (rayFastHeaderArea);
            }
        }
       #else
        auto slotA = rayFrame.removeFromLeft (slotW);
        rayFrame.removeFromLeft (gap);
        const int iconSize = juce::jmin (slotW, rayKnobAreaH);
        rayFastButton.setBounds ({});
        rayStrengthButton.setBounds (slotA.withSizeKeepingCentre (iconSize, iconSize).translated (0, -6));

        auto slotB = rayFrame.removeFromLeft (slotW);
        rayFrame.removeFromLeft (gap);
        placeKnobWithLabel (slotB, rayRateSlider, rayRateLabel, juce::jmin (slotW, rayKnobAreaH));

        auto slotC = rayFrame;
        rayPairButton.setBounds (slotC.withSizeKeepingCentre (juce::jmin (60, slotW), juce::jmin (32, rayKnobAreaH)).translated (0, -6));
       #endif
    }
}
