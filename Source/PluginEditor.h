#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GUI/CustomLookAndFeel.h"
#include "GUI/GoniometerComponent.h"
#include "GUI/LevelMeterComponent.h"
#include "SpaceXManualData.h"   // eingebettete Anleitung + Theme-Vorschaubilder

// IDs der Einstellungen. Frueher ein lokales enum im PopupMenu - jetzt auf
// Dateiebene, weil das Settings-Panel dieselben Aktionen ausloest.
enum SpaceXSettingsId
{
    idGalaxyDefault = 1,
    idHideGonioDefault,
    idHideSpaceVisualsDefault,
    idShowModulation,
    idDisableModMovement,
    idReduceAnimations,
    idSaveSizeDefault,
    idSaveStateDefault,
    idOpenPresetFolder,
    idSetPresetFolder,
    idPrismClickJumps,
    idPresetSetsGalaxy,
    idMutatePrism,
    idMutateMix,
    idLockMix,
    idHoverHints,
    idKeepSolo,
    idShowCategories,
    idThemeModern,
    idThemeComic,
    idThemePurple,
    idThemeDay,
    idThemeDark,
    idThemeMoon,
    idThemeSciFiDark,
    idLayoutFrames,
    idLayoutFrameless,
    idLayoutEasy,
    idOpenManual,
    idBandGalaxy,
    idResetSettings,
    idShowHz,
    idCancelSettings,
    idSaveSettings,
    idActivate,
    idAutoGain,
    idBassGuard,
    idShowAdvancedMod,
    idTechnicalLabels,
    idBackPanel
};

// ===== Varianten-Builds fuer den Layout-Vergleich (User) =====
// 0 = normales SpaceX (unveraendert)
// 1 = A: Tilt/Depth klein UNTER den beiden Hauptreglern (Dreieck)
// 2 = B: Tilt/Depth klein, gleicher Platz (dritter Regler)
// 3 = C: alle sechs Regler gleich gross, kleiner
// Gesetzt ueber CMake: -DSPACEX_VARIANT=A|B|C (siehe build_variants.sh).
#ifndef SPACEX_ROW2_VARIANT
 #define SPACEX_ROW2_VARIANT 0
#endif
// Vergleichs-Builds (Runde 41):
//   SPACEX_PARALLAX_UI 0 = Amount + 4 Knoepfe (SpaceX, SpaceXpresets)
//                      1 = drei Regler Drift/Shift/Tilt (SpaceXpara, SpaceXparaCPU)
//                      2 = Amount + EIN Klick-Knopf (SpaceXclick, SpaceXraye)
//   SPACEX_RAYE_UI     1 = RAYE mit Amount + Charakter-Klick (SpaceXraye)
#ifndef SPACEX_PARALLAX_UI
 #define SPACEX_PARALLAX_UI 0
#endif
#ifndef SPACEX_DOTS_INSIDE
 // 1 = Modus-Punkte sitzen IN der Pille (Parallax + RAYE), 0 = darunter.
 #define SPACEX_DOTS_INSIDE 0
#endif
constexpr bool kDotsInside = (SPACEX_DOTS_INSIDE != 0);

#ifndef SPACEX_RAYE_UI
 #define SPACEX_RAYE_UI 0
#endif

class LCRMSAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LCRMSAudioProcessorEditor (LCRMSAudioProcessor&);
    ~LCRMSAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;
    void closeViewPanel();
    void closeSettingsPanel();
    void startTour();
    void showBackPanel();
    void closeBackPanel();
    // Stand beim Oeffnen des Settings-Panels - "Cancel" stellt ihn wieder her
    // (gleiche Idee wie im View-Panel).
    struct SettingsSnapshot
    {
        int  theme = 0, layout = 0;
        bool autoGain = true;
        bool bassGuard = true;
        bool prism = true, mix = false, cats = true, clickEdge = false,
             galaxyStart = false, keepSolo = true, modVis = true, showHz = false, advMod = false,
             techLabels = false;
    };
    SettingsSnapshot settingsSnap;
    void captureSettingsSnapshot();
    void restoreSettingsSnapshot();
    void refreshSettingsPanel();
    void handleSettingsAction (int result);
    juce::ValueTree viewPanelSnapshot;   // Stand beim Oeffnen (fuer Cancel)

private:
    // Die eigentliche GUI wird immer auf einer festen "logischen" Flaeche
    // (kDesignW x kDesignH) angeordnet und gezeichnet. Der Editor selbst
    // darf beliebig resized werden; er skaliert die gesamte content-
    // Komponente per Transform, statt neu zu layouten. So werden beim
    // Vergroessern/Verkleinern wirklich ALLE Elemente (Regler, Rahmen,
    // Schrift, Buttons) mitskaliert.
    static constexpr int kDesignW = 1040;
    // Zurueck auf die urspruengliche Hoehe (+4 fuer die etwas hoehere
    // Titelzeile). Die zwischenzeitliche eigene Preset-Leiste (762) ist
    // wieder weg - siehe Header-Kommentar in layoutContent(): zwei halb
    // leere Zeilen uebereinander waren der Grund, warum das Plugin
    // "vollgepackt" wirkte, obwohl kaum etwas dazugekommen war.
    static constexpr int kDesignH = (SPACEX_ROW2_VARIANT == 0) ? 604 : 700;
    // Varianten-Builds (A/B/C) sind 700 hoch: das Sternenfeld ist quadratisch
    // und waechst mit der Hoehe (~361 -> ~457 px), die Sektionen werden
    // dadurch automatisch ~95 px schmaler. Das normale SpaceX (Variante 0)
    // bleibt exakt wie es ist.
    // 736 -> 604: exakt die Hoehe der weggefallenen vierten Zeile (116 px
    // Rahmen + 16 px Abstand). Alles andere behaelt damit seine bisherige
    // Groesse, es verschiebt sich nichts. Die Breite bleibt vorerst bei
    // 1040 - in Reihe 2 stehen jetzt drei statt zwei Regler pro Rahmen, und
    // beides gleichzeitig zu aendern hiesse nicht mehr zu wissen, woran es
    // liegt, wenn etwas klemmt.
    // Hoehe der Titelzeile. 64 -> 68: rechts sitzen jetzt ZWEI Zeilen
    // (Live-Aktionen oben, Preset-Verwaltung unten) - das spiegelt genau die
    // zwei Textzeilen links (Wortmarke oben, Slogan unten). Zwei mal 26px
    // plus Zwischenraum brauchen die vier Pixel mehr.
    static constexpr int kTitleBarH = 68;
    // Aeusserer Randabstand, den die Regler-Zeilen (siehe layoutContent(),
    // "area.reduced(kOuterMargin)") bereits nutzen - die Titelzeile
    // (Logo + Wortmark) sass bisher bei (0,0), also OHNE diesen Rand,
    // wodurch sie im Vergleich zum Rest der GUI zu eng/unpassend am Rand
    // klebte (User-Feedback). Jetzt teilen sich beide denselben Wert.
    static constexpr int kOuterMargin = 20;
    // Slogan-Zeile unter dem Logo (User-Wunsch: "Unter dem Logo ist noch
    // Platz ... hier Slogan") - eigener Font/Gradient in drawLogo(), keine
    // zusaetzliche Hoehe im Layout noetig (passt in kTitleBarH).

    struct ContentComponent : public juce::Component
    {
        explicit ContentComponent (LCRMSAudioProcessorEditor& e) : editor (e) {}
        void paint (juce::Graphics& g) override { editor.paintContent (g); }
        // Bug-Fix: der Bypass-Verdunkelungs-Schleier lag bisher in paint()
        // (also VOR den Kind-Komponenten Goniometer/Korrelationsmesser) und
        // wurde von deren eigenem Zeichnen wieder ueberdeckt - Goniometer und
        // Korrelationsmesser blieben bei Bypass dadurch weiterhin voll
        // farbig (User-Feedback: "auch ausgrauen ... den visuellen Bereich
        // abdunkeln wenn bypassed"). paintOverChildren() laeuft NACH allen
        // Kindern und legt den Schleier jetzt wirklich ueber alles.
        void paintOverChildren (juce::Graphics& g) override { editor.paintOverContent (g); }
        void resized() override { editor.layoutContent(); }
        LCRMSAudioProcessorEditor& editor;
    };

    // Kleiner horizontaler Korrelationsmesser unter dem Goniometer - reine
    // Anzeige, keine eigene LookAndFeel-Anbindung noetig (self-contained,
    // header-only, damit KEINE neue .cpp/CMakeLists-Aenderung noetig ist).
    class CorrelationMeterComponent : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        void setCorrelation (float c)
        {
            c = juce::jlimit (-1.0f, 1.0f, c);
            // Geglaettet (User: "smoother"): ein Viertel des Wegs je Update.
            const float next = correlation + (c - correlation) * 0.25f;
            if (std::abs (next - correlation) > 0.0005f) { correlation = next; repaint(); }
        }

        // Komplett neu gezeichnet (User-Feedback: "Correlations Meter ist viel
        // zu klobig, zu amateurhaft vom Look. Vielleicht einfach IN das obere
        // Feld ganz unten mit einbauen, weniger dick, etwas dezenter, also gar
        // keinen extra Kasten dafuer").
        //
        // Kein eigener Kasten und KEIN Hintergrund mehr - die Komponente liegt
        // jetzt transparent ueber dem unteren Innenrand des Starfields (siehe
        // layoutContent()), sodass das Sternenfeld dahinter durchscheint. Statt
        // eines dicken Balkens nur noch: eine feine durchgehende Grundlinie
        // ueber die volle Breite, eine etwas hoehere Mittelmarkierung als
        // Nullpunkt, und der eigentliche Messwert als schlanker farbiger
        // Balken, der von der Mitte aus nach links/rechts waechst. Ein
        // schwacher dunkler Schleier direkt hinter der Linie haelt sie auch
        // ueber hellen Sternen lesbar, ohne als Kasten zu wirken.
        void paint (juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            const float cy = b.getCentreY();
            const float midX = b.getCentreX();

            // Sehr dezenter Abdunkel-Schleier statt eines Rahmens/Kastens.
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillRoundedRectangle (b, b.getHeight() * 0.5f);

            // Feine Grundlinie ueber die volle Breite.
            g.setColour (juce::Colours::white.withAlpha (0.14f));
            g.fillRoundedRectangle (b.getX(), cy - 0.5f, b.getWidth(), 1.0f, 0.5f);

            // Nullpunkt-Markierung (Mitte), etwas hoeher als die Grundlinie.
            g.setColour (juce::Colours::white.withAlpha (0.35f));
            g.fillRect (midX - 0.5f, cy - 4.0f, 1.0f, 8.0f);

            // Messwert: schlanker Balken von der Mitte aus.
            const float halfW  = b.getWidth() * 0.5f;
            const float w      = correlation * halfW;
            const float barH   = 3.0f;
            auto barCol = correlation < -0.2f ? juce::Colour (0xffff5b5b)
                        : correlation <  0.4f ? juce::Colour (0xffffb648)
                                              : themePalette().knob;   // je Theme (User)
            g.setColour (barCol);
            if (w >= 0.0f)
                g.fillRoundedRectangle (midX, cy - barH * 0.5f, w, barH, barH * 0.5f);
            else
                g.fillRoundedRectangle (midX + w, cy - barH * 0.5f, -w, barH, barH * 0.5f);
        }

    private:
        float correlation = 0.0f;
    };

    // ===== VIEW-PANEL (Zahnrad im Sternenfeld) =====
    // Kleines aufklappbares Feld mit allen reinen Darstellungsoptionen des
    // Sternenfelds - vier Schalter, vier Regler. Nichts davon aendert Audio.
    // Ersetzt die frueheren Menue-Eintraege (Goniometer, Starfield, Mod
    // Movement, Reduce Animations), die jetzt hier zusammen mit den neuen
    // Reglern (Dim, Brightness, Speed, Density) wohnen. Header-only wie die
    // anderen Hilfskomponenten. Die eigentliche Logik (anwenden, speichern)
    // haengt der Editor ueber onChange an.
    // ===== Einstellungs-Panel =====
    // Ersetzt das alte PopupMenu (User: "Menu blinkt bei jedem Klick ... das
    // Menu soll offen bleiben"). Alles steht gleichzeitig da, in drei Spalten:
    // Theme, Layout/Smart, Verhalten. Die Knoepfe loesen genau dieselben
    // Aktionen aus wie vorher die Menue-Eintraege (siehe SpaceXSettingsId).
    // Klickfaenger hinter dem Settings-Panel: schliesst es und verhindert
    // gleichzeitig, dass man versehentlich an einem Regler dahinter dreht
    // (User: "close when click on the other UI").
    struct BackdropComponent : public juce::Component
    {
        std::function<void()> onClick;
        void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
    };

    class SettingsPanelComponent : public juce::Component
    {
    public:
        static constexpr int kThemes  = 5;
        static constexpr int kLayouts = 3;
        static constexpr int kSmart   = 2;
        static constexpr int kBehav   = 6;

        juce::Label title, themeHead, layoutHead, smartHead, behavHead;
        // Runde 53 (User): eigene Hinweiszeile IM Panel - immer aktiv,
        // unabhaengig vom "?"-Schalter im Hauptfenster.
        juce::Label hintLine;
        juce::TextButton themeBtn[kThemes], layoutBtn[kLayouts], smartBtn[kSmart], behavBtn[kBehav];
        juce::TextButton sizeBtn { "Save Window Size" }, stateBtn { "Save State as Default" },
                         folderBtn { "Preset Folder..." }, manualBtn { "Manual" },
                         aboutBtn { "Back Panel" },
                         resetBtn { "Reset" }, cancelBtn { "Cancel" }, saveBtn { "Save" },
                         licenceBtn { "Activate..." };

        std::function<void (int)> onAction;
        std::function<void()>     onClose;

        // Reihenfolge wie im alten Menue (User-Wunsch aus Runde 23).
        static const int* themeIds()  { static const int a[kThemes]  = { idThemeMoon, idThemeDay, idThemeDark, idThemePurple, idThemeComic }; return a; }
        static const int* layoutIds() { static const int a[kLayouts] = { idLayoutFrames, idLayoutFrameless, idLayoutEasy }; return a; }
        static const int* smartIds()  { static const int a[kSmart]   = { idMutateMix, idShowCategories }; return a; }
        static const int* behavIds()  { static const int a[kBehav]   = { idAutoGain, idBassGuard, idShowModulation, idShowAdvancedMod, idGalaxyDefault, idTechnicalLabels }; return a; }

        SettingsPanelComponent()
        {
            auto head = [this] (juce::Label& l, const char* txt, float size, juce::Colour col)
            {
                l.setText (txt, juce::dontSendNotification);
                l.setJustificationType (juce::Justification::centredLeft);
                l.setFont (juce::Font (juce::FontOptions (size, juce::Font::bold)).withExtraKerningFactor (0.14f));
                l.setColour (juce::Label::textColourId, col);
                l.setInterceptsMouseClicks (false, false);
                addAndMakeVisible (l);
            };
            head (title,      "SETTINGS",  16.0f, juce::Colour (0xffb968ff));
            head (themeHead,  "THEME",     13.0f, juce::Colour (0xff8f96a4));
            head (layoutHead, "LAYOUT",    13.0f, juce::Colour (0xff8f96a4));
            head (smartHead,  "SMART",     13.0f, juce::Colour (0xff8f96a4));
            head (behavHead,  "BEHAVIOUR", 13.0f, juce::Colour (0xff8f96a4));
            hintLine.setFont (juce::Font (juce::FontOptions (12.0f)));
            hintLine.setColour (juce::Label::textColourId, juce::Colour (0xff9aa0ab));
            hintLine.setJustificationType (juce::Justification::centredLeft);
            hintLine.setMinimumHorizontalScale (1.0f);
            hintLine.setInterceptsMouseClicks (false, false);
            addAndMakeVisible (hintLine);
            title.setTooltip ("Click to close");

            static const char* const themeNames[kThemes]  = { "Moon", "Day & Night", "Fireflies", "Sci-Fi", "Pop" };   // altes "Moon" geloescht, "Silver" heisst jetzt "Moon" (User)
            static const char* const layoutNames[kLayouts] = { "3D", "Flat", "Outline" };
            // Runde 31: "Changes Focus", "Focus: Click Moves Edge" und
            // "Show Focus Hz" sind mit dem Focus-Bereich weggefallen,
            // "Keep Solo When Off" mit Solo. Tote Menuepunkte sind genau die
            // Art Ballast, die wir gerade abbauen.
            static const char* const smartNames[kSmart]   = { "Changes Mix", "Show Categories" };
            static const char* const behavNames[kBehav]   = { "Auto Gain", "Bass Guard 120 Hz",
                                                              "Show Modulation",
                                                              "Show Advanced Modulation",
                                                              "Galaxy On Startup (Latency)",
                                                              "Technical Labels" };

            auto setup = [this] (juce::TextButton& b, const char* txt, int id)
            {
                b.setButtonText (txt);
                b.setClickingTogglesState (false);   // Zustand kommt vom Editor, nicht vom Klick
                b.setWantsKeyboardFocus (false);
                b.getProperties().set ("noGlow", true);
                b.onClick = [this, id] { if (onAction) onAction (id); };
                b.addMouseListener (this, false);   // fuer die Hinweiszeile
                addAndMakeVisible (b);
            };
            for (int i = 0; i < kThemes;  ++i) setup (themeBtn[i],  themeNames[i],  themeIds()[i]);
            // Mini-Vorschau unter der Theme-Spalte (User). Reihenfolge wie
            // themeNames. Beim Ueberfahren eines Namens zeigt sie DIESES Theme,
            // sonst das gerade aktive - man kann also durchfahren und sehen,
            // was einen erwartet, ohne etwas umzustellen.
            themeShot[0] = juce::ImageCache::getFromMemory (SpaceXManualData::theme_silver_png,    SpaceXManualData::theme_silver_pngSize);
            themeShot[1] = juce::ImageCache::getFromMemory (SpaceXManualData::theme_daynight_png,  SpaceXManualData::theme_daynight_pngSize);
            themeShot[2] = juce::ImageCache::getFromMemory (SpaceXManualData::theme_fireflies_png, SpaceXManualData::theme_fireflies_pngSize);
            themeShot[3] = juce::ImageCache::getFromMemory (SpaceXManualData::theme_scifi_png,     SpaceXManualData::theme_scifi_pngSize);
            themeShot[4] = juce::ImageCache::getFromMemory (SpaceXManualData::theme_pop_png,       SpaceXManualData::theme_pop_pngSize);
            for (int i = 0; i < kThemes; ++i) themeBtn[i].addMouseListener (this, false);
            for (int i = 0; i < kLayouts; ++i) setup (layoutBtn[i], layoutNames[i], layoutIds()[i]);
            for (int i = 0; i < kSmart;   ++i) setup (smartBtn[i],  smartNames[i],  smartIds()[i]);
            for (int i = 0; i < kBehav;   ++i) setup (behavBtn[i],  behavNames[i],  behavIds()[i]);
            setup (sizeBtn,   "Save Window Size",      idSaveSizeDefault);
            setup (stateBtn,  "Save State as Default", idSaveStateDefault);
            setup (folderBtn, "Preset Folder...",      idOpenPresetFolder);
            setup (manualBtn, "Manual",                idOpenManual);
            setup (aboutBtn,  "Back Panel",            idBackPanel);
            setup (resetBtn,  "Reset",                 idResetSettings);
            setup (licenceBtn, "Activate...", idActivate);
            setup (cancelBtn, "Cancel", idCancelSettings);
            setup (saveBtn,   "Save",   idSaveSettings);
            cancelBtn.setTooltip ("Undo everything changed since opening and close");
            saveBtn.setTooltip ("Keep the changes and close");

            smartBtn[0].setTooltip ("Smart also moves the Mix knob");
            behavBtn[0].setTooltip ("Matches the output level to the input, so bypass is an honest comparison");
            behavBtn[1].setTooltip ("Leaves everything below 120 Hz untouched in Galaxy and Dimension");
            behavBtn[3].setTooltip ("Shows the modulation switch and depth in every section. Life scales them all");
            behavBtn[4].setTooltip ("Galaxy is armed when the plugin opens - adds latency from the start");
            behavBtn[5].setTooltip ("Names the sections and knobs by what they do: LCR, Polarity, MicroPitch, Mid-Side, Autopan, Phaser");
            folderBtn.setTooltip ("Open the folder your presets live in");
            resetBtn.setTooltip ("Back to the factory settings");
        }

        void mouseUp (const juce::MouseEvent& e) override
        {
            // NUR eigene Klicks. Seit die Theme-Knoepfe fuer die Vorschau einen
            // MouseListener auf das Panel haben, landen auch deren Klicks hier -
            // mit Koordinaten RELATIV ZUM KNOPF. Die lagen oft zufaellig in der
            // Titelzeile, und das Panel schloss sich beim Theme-Wechsel
            // (User: "Settings-Fenster soll offen bleiben").
            if (e.eventComponent != this)
                return;
            if (title.getBounds().contains (e.getPosition()) && onClose)
                onClose();
        }

        void mouseEnter (const juce::MouseEvent& e) override
        {
            // Hinweiszeile: der Tooltip des ueberfahrenen Knopfes, ohne dass
            // man dafuer irgendwo etwas einschalten muss.
            if (auto* b = dynamic_cast<juce::Button*> (e.eventComponent))
                if (b->getTooltip().isNotEmpty())
                    hintLine.setText (b->getTooltip(), juce::dontSendNotification);
            for (int i = 0; i < kThemes; ++i)
                if (e.eventComponent == &themeBtn[i]) { hoverTheme = i; repaint (previewArea.expanded (4)); return; }
        }
        void mouseExit (const juce::MouseEvent& e) override
        {
            hintLine.setText ({}, juce::dontSendNotification);
            for (int i = 0; i < kThemes; ++i)
                if (e.eventComponent == &themeBtn[i] && hoverTheme == i) { hoverTheme = -1; repaint (previewArea.expanded (4)); return; }
        }

        void paint (juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            g.setColour (juce::Colour (0xff1e2128));   // minimal heller (User)
            g.fillRoundedRectangle (b, 10.0f);
            g.setColour (juce::Colours::white.withAlpha (0.16f));
            g.drawRoundedRectangle (b.reduced (0.5f), 10.0f, 1.0f);

            // Farbtupfer links neben jedem Theme-Namen - schneller zu treffen
            // als eine reine Textliste.
            static const juce::uint32 dots[kThemes] = { 0xffc9d3e2, 0xffe3b25f, 0xff9a7bff, 0xff5be3ff, 0xffff5fa8 };
            for (int i = 0; i < kThemes; ++i)
            {
                auto r = themeBtn[i].getBounds().toFloat();
                g.setColour (juce::Colour (dots[i]).withAlpha (themeBtn[i].getToggleState() ? 1.0f : 0.55f));
                g.fillEllipse (r.getX() - 15.0f, r.getCentreY() - 4.5f, 9.0f, 9.0f);
            }
            // Vorschau des gerade aktiven (oder ueberfahrenen) Themes.
            {
                int idx = hoverTheme;
                if (idx < 0)
                    for (int i = 0; i < kThemes; ++i)
                        if (themeBtn[i].getToggleState()) { idx = i; break; }
                auto pr = previewArea.toFloat();
                if (idx >= 0 && idx < kThemes && themeShot[idx].isValid() && ! pr.isEmpty())
                {
                    juce::Path clip;
                    clip.addRoundedRectangle (pr, 7.0f);
                    g.saveState();
                    g.reduceClipRegion (clip);
                    // DER Bug (Runde 35): drawImage zeichnet mit der Deckkraft
                    // der zuletzt gesetzten Farbe - das war der Farbtupfer von
                    // Pop (aktiv 1.0, sonst 0.55). Nur mit Pop aktiv war die
                    // Vorschau deshalb voll hell.
                    g.setOpacity (1.0f);
                    g.drawImage (themeShot[idx], pr, juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);
                    g.restoreState();
                    g.setColour (juce::Colours::white.withAlpha (hoverTheme >= 0 ? 0.30f : 0.16f));
                    g.drawRoundedRectangle (pr.reduced (0.5f), 7.0f, 1.0f);
                }
            }
            if (dividerX > 0)
            {
                g.setColour (juce::Colours::white.withAlpha (0.08f));
                g.fillRect ((float) dividerX,  b.getY() + 48.0f, 1.0f, b.getHeight() - 140.0f);
                g.fillRect ((float) dividerX2, b.getY() + 48.0f, 1.0f, b.getHeight() - 140.0f);
            }
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced (16, 13);
            title.setBounds (r.removeFromTop (22));
            r.removeFromTop (8);

            // Zwei Fusszeilen statt einer: sechs Knoepfe nebeneinander waeren
            // so schmal, dass die Beschriftungen abschneiden (User).
            {
                auto row2 = r.removeFromBottom (30);
                r.removeFromBottom (7);
                const int gap = 7;
                const int w = 128;
                licenceBtn.setBounds (row2.removeFromLeft (w + 10));
                saveBtn.setBounds (row2.removeFromRight (w));
                row2.removeFromRight (gap);
                cancelBtn.setBounds (row2.removeFromRight (w));
                row2.removeFromRight (gap);
                resetBtn.setBounds (row2.removeFromRight (w));

                auto row1 = r.removeFromBottom (30);
                r.removeFromBottom (4);
                hintLine.setBounds (r.removeFromBottom (18));
                r.removeFromBottom (7);
                // "Save Window Size" ist weg - die Groesse merkt sich das
                // Plugin jetzt selbst.
                sizeBtn.setVisible (false);
                const int n = 4;
                const int w1 = (row1.getWidth() - gap * (n - 1)) / n;
                juce::TextButton* row[n] = { &stateBtn, &folderBtn, &manualBtn, &aboutBtn };
                int x = row1.getX();
                for (int i = 0; i < n; ++i) { row[i]->setBounds (x, row1.getY(), w1, row1.getHeight()); x += w1 + gap; }
            }

            const int colGap = 18;
            const int colW   = (r.getWidth() - colGap * 2) / 3;
            auto col1 = r.removeFromLeft (colW);          r.removeFromLeft (colGap);
            auto col2 = r.removeFromLeft (colW);          r.removeFromLeft (colGap);
            auto col3 = r;
            dividerX  = col1.getRight() + colGap / 2;
            dividerX2 = col2.getRight() + colGap / 2;

            auto stack = [] (juce::Rectangle<int>& area, juce::Label& hd, juce::TextButton* btns, int n, int indent)
            {
                hd.setBounds (area.removeFromTop (19));
                area.removeFromTop (6);
                for (int i = 0; i < n; ++i)
                {
                    auto row = area.removeFromTop (36);
                    btns[i].setBounds (row.withTrimmedLeft (indent));
                    area.removeFromTop (5);
                }
            };
            stack (col1, themeHead,  themeBtn,  kThemes,  16);
            col1.removeFromTop (12);
            {
                auto pv = col1.withTrimmedLeft (16);
                const int pw = pv.getWidth();
                previewArea = juce::Rectangle<int> (pv.getX(), pv.getY(), pw,
                                                    juce::jmin (pv.getHeight(), (int) ((float) pw / 1.413f)));
            }
            stack (col2, layoutHead, layoutBtn, kLayouts, 0);
            col2.removeFromTop (14);
            stack (col2, smartHead,  smartBtn,  kSmart,   0);
            stack (col3, behavHead,  behavBtn,  kBehav,   0);
        }

    private:
        int dividerX = 0, dividerX2 = 0;
        int hoverTheme = -1;
        juce::Image themeShot[kThemes];
        juce::Rectangle<int> previewArea;
    };


    // ===== BACK PANEL =====
    // Die Rueckseite des Geraetes (User-Wunsch). Hier wird nichts eingestellt:
    // wer es gebaut hat, wie man ihn erreicht, und auf welchen Namen diese
    // Kopie laeuft. Die Kontaktdaten stehen als Konstanten in
    // PluginEditor.cpp (spacexContact) - eine Stelle zum Aendern.
    class BackPanelComponent : public juce::Component
    {
    public:
        juce::Label title, slogan, regHead, regName, byHead, byName,
                    thanksHead, thanksText, footer, qrCaption;
        juce::TextButton mailBtn, webBtn, instaBtn, linksBtn,
                         tourBtn { "Take the Tour" }, manualBtn { "Manual" }, closeBtn { "Close" };
        std::function<void()> onTour;
        std::function<void()> onClose;
        std::function<void()> onManual;
        juce::Image qrImage;

        BackPanelComponent()
        {
            auto lab = [this] (juce::Label& l, float size, juce::Colour col, bool bold,
                               juce::Justification just = juce::Justification::centredLeft)
            {
                l.setFont (juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain))
                               .withExtraKerningFactor (0.06f));
                l.setColour (juce::Label::textColourId, col);
                l.setJustificationType (just);
                l.setInterceptsMouseClicks (false, false);
                addAndMakeVisible (l);
            };
            lab (title,      22.0f, juce::Colour (0xffe4e7ec), true,  juce::Justification::centred);
            lab (slogan,     12.5f, juce::Colour (0xff8f96a4), false, juce::Justification::centred);
            lab (regHead,    11.5f, juce::Colour (0xff8f96a4), true);
            lab (regName,    15.0f, juce::Colour (0xff5be3c7), true);
            lab (byHead,     11.5f, juce::Colour (0xff8f96a4), true);
            lab (byName,     13.5f, juce::Colour (0xffd4d8e0), false);
            lab (thanksHead, 11.5f, juce::Colour (0xff8f96a4), true);
            lab (thanksText, 13.5f, juce::Colour (0xffd4d8e0), false);
            lab (qrCaption,  11.0f, juce::Colour (0xff8f96a4), false, juce::Justification::centred);
            lab (footer,     13.0f, juce::Colour (0xffb5b9c2), false, juce::Justification::centred);
            title.setText ("SPACEX", juce::dontSendNotification);
            slogan.setText ("Stereo imaging, tuned by ear", juce::dontSendNotification);
            regHead.setText ("REGISTERED TO", juce::dontSendNotification);
            byHead.setText ("DESIGNED AND BUILT BY", juce::dontSendNotification);
            thanksHead.setText ("THANKS TO", juce::dontSendNotification);
            footer.setText ("Thanks for your support - happy mixing.", juce::dontSendNotification);
            qrCaption.setText ("Everything in one place", juce::dontSendNotification);

            for (auto* b : { &mailBtn, &webBtn, &instaBtn, &linksBtn, &tourBtn, &manualBtn, &closeBtn })
            {
                b->setWantsKeyboardFocus (false);
                b->getProperties().set ("thinOnFrame", true);
                b->getProperties().set ("noGlow", true);
                addAndMakeVisible (*b);
            }
            closeBtn.onClick = [this] { if (onClose)  onClose(); };
            manualBtn.onClick = [this] { if (onManual) onManual(); };
            tourBtn.onClick   = [this] { if (onTour)   onTour(); };
        }

        void paint (juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            g.setColour (juce::Colour (0xff1e2128));
            g.fillRoundedRectangle (b, 10.0f);
            g.setColour (juce::Colours::white.withAlpha (0.16f));
            g.drawRoundedRectangle (b.reduced (0.5f), 10.0f, 1.0f);

            // Feiner Trennstrich unter dem Kopf - wie auf einer echten
            // Geraeterueckseite das Typenschild vom Rest getrennt ist.
            g.setColour (juce::Colours::white.withAlpha (0.08f));
            g.fillRect (b.getX() + 28.0f, b.getY() + 104.0f, b.getWidth() - 56.0f, 1.0f);

            auto qr = qrArea.toFloat();
            if (! qr.isEmpty())
            {
                if (qrImage.isValid())
                {
                    g.setOpacity (1.0f);
                    g.drawImage (qrImage, qr, juce::RectanglePlacement::centred);
                }
                else
                {
                    // Noch kein QR-Bild hinterlegt: leeres Feld statt eines
                    // gemalten Fantasie-Codes, den niemand scannen kann.
                    g.setColour (juce::Colours::white.withAlpha (0.05f));
                    g.fillRoundedRectangle (qr, 6.0f);
                    g.setColour (juce::Colours::white.withAlpha (0.14f));
                    g.drawRoundedRectangle (qr.reduced (0.5f), 6.0f, 1.0f);
                }
            }
        }

        void resized() override
        {
            // Runde 53 (User: "bisschen noch oben und unten ziehen"): mehr
            // Luft um Titel und Slogan, damit die Schrift nicht gequetscht wirkt.
            auto r = getLocalBounds().reduced (28, 26);
            title.setBounds (r.removeFromTop (34));
            r.removeFromTop (4);
            slogan.setBounds (r.removeFromTop (22));
            r.removeFromTop (32);

            auto bottom = r.removeFromBottom (34);
            {
                const int w = 118, gap = 10;
                auto row = bottom.withSizeKeepingCentre (w * 3 + gap * 2, bottom.getHeight());
                tourBtn.setBounds (row.removeFromLeft (w));
                row.removeFromLeft (gap);
                manualBtn.setBounds (row.removeFromLeft (w));
                row.removeFromLeft (gap);
                closeBtn.setBounds (row);
            }
            footer.setBounds (r.removeFromBottom (26));
            r.removeFromBottom (16);

            auto right = r.removeFromRight (juce::jmin (150, r.getWidth() / 3));
            r.removeFromRight (18);
            {
                const int qs = juce::jmin (right.getWidth(), 116);
                qrArea = { right.getCentreX() - qs / 2, right.getY() + 6, qs, qs };
                qrCaption.setBounds (right.getX(), qrArea.getBottom() + 4, right.getWidth(), 16);
                linksBtn.setBounds (right.getX(), qrCaption.getBottom() + 6, right.getWidth(), 28);
            }

            // Runde 54 (Bug): die drei Links wurden von OBEN gelegt, nachdem
            // die Textzeilen ihren Platz genommen hatten - blieb nichts uebrig,
            // wurden sie zu Striemen oder verschwanden ganz. Jetzt andersherum:
            // die Knoepfe holen sich ihre Hoehe von UNTEN, die Textzeilen
            // teilen sich, was darueber steht. So kann nichts mehr wegfallen.
            {
                const int bh = 28, bgap = 7;
                const int colW = juce::jmax (150, r.getWidth() - r.getWidth() / 4);
                auto place = [&r, bh, colW] (juce::TextButton& b)
                {
                    auto row = r.removeFromBottom (bh);
                    b.setBounds (row.withWidth (juce::jmin (colW, row.getWidth())));
                };
                place (instaBtn); r.removeFromBottom (bgap);
                place (webBtn);   r.removeFromBottom (bgap);
                place (mailBtn);  r.removeFromBottom (18);
            }

            // Was jetzt noch da ist, teilen sich die drei Textzeilen.
            {
                const int lines = 3;
                const int perLine = juce::jmax (40, r.getHeight() / lines);
                auto line = [&r, perLine] (juce::Label& head, juce::Component& value)
                {
                    auto block = r.removeFromTop (juce::jmin (perLine, r.getHeight()));
                    head.setBounds (block.removeFromTop (juce::jmin (16, block.getHeight())));
                    block.removeFromTop (juce::jmin (2, block.getHeight()));
                    value.setBounds (block.removeFromTop (juce::jmin (23, block.getHeight())));
                };
                line (regHead,    regName);
                line (byHead,     byName);
                line (thanksHead, thanksText);
            }
        }

    private:
        juce::Rectangle<int> qrArea;
    };


    // ===== TAKE THE TOUR =====
    // Einmal durch die wichtigsten Punkte (User-Wunsch). Bewusst ein Overlay
    // ueber der echten Oberflaeche statt einer Bilderstrecke: man sieht die
    // Sektion, um die es geht, an ihrem echten Platz. Der Schleier hat ein
    // Loch - das ist der ganze Trick, mehr braucht es nicht.
    class TourOverlay : public juce::Component
    {
    public:
        struct Step { juce::Rectangle<int> target; juce::String head, text; };
        std::vector<Step> steps;
        int index = 0;
        std::function<void()> onFinish;
        juce::TextButton backBtn { "Back" }, nextBtn { "Next" }, skipBtn { "Skip" };

        TourOverlay()
        {
            for (auto* b : { &backBtn, &nextBtn, &skipBtn })
            {
                b->setWantsKeyboardFocus (false);
                b->getProperties().set ("thinOnFrame", true);
                b->getProperties().set ("noGlow", true);
                addAndMakeVisible (*b);
            }
            backBtn.onClick = [this] { if (index > 0) { --index; refresh(); } };
            nextBtn.onClick = [this]
            {
                if (index + 1 < (int) steps.size()) { ++index; refresh(); }
                else if (onFinish) onFinish();
            };
            skipBtn.onClick = [this] { if (onFinish) onFinish(); };
            setInterceptsMouseClicks (true, true);
        }

        void refresh()
        {
            backBtn.setEnabled (index > 0);
            nextBtn.setButtonText (index + 1 < (int) steps.size() ? "Next" : "Done");
            resized();
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            if (steps.empty())
                return;
            auto hole = steps[(size_t) juce::jlimit (0, (int) steps.size() - 1, index)].target
                            .expanded (6).getIntersection (getLocalBounds());

            g.saveState();
            if (! hole.isEmpty())
                g.excludeClipRegion (hole);
            g.setColour (juce::Colour (0xff0a0b0e).withAlpha (0.78f));
            g.fillRect (getLocalBounds());
            g.restoreState();

            if (! hole.isEmpty())
            {
                g.setColour (juce::Colour (0xff5be3c7).withAlpha (0.85f));
                g.drawRoundedRectangle (hole.toFloat().reduced (0.5f), 8.0f, 1.6f);
            }

            auto box = cardArea.toFloat();
            g.setColour (juce::Colour (0xff1e2128));
            g.fillRoundedRectangle (box, 10.0f);
            g.setColour (juce::Colours::white.withAlpha (0.16f));
            g.drawRoundedRectangle (box.reduced (0.5f), 10.0f, 1.0f);

            const auto& st = steps[(size_t) juce::jlimit (0, (int) steps.size() - 1, index)];
            auto inner = cardArea.reduced (18, 14);
            g.setColour (juce::Colour (0xff8f96a4));
            g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)).withExtraKerningFactor (0.14f));
            g.drawText (juce::String (index + 1) + " / " + juce::String ((int) steps.size()),
                        inner.removeFromTop (14), juce::Justification::topRight);
            g.setColour (juce::Colour (0xffe4e7ec));
            g.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)).withExtraKerningFactor (0.05f));
            g.drawText (st.head, inner.removeFromTop (22), juce::Justification::topLeft);
            inner.removeFromTop (4);
            inner.removeFromBottom (36);
            g.setColour (juce::Colour (0xffb5b9c2));
            g.setFont (juce::Font (juce::FontOptions (13.0f)));
            g.drawFittedText (st.text, inner, juce::Justification::topLeft, 5);
        }

        void resized() override
        {
            if (steps.empty())
                return;
            auto target = steps[(size_t) juce::jlimit (0, (int) steps.size() - 1, index)].target;
            const int cw = 330, ch = 150, pad = 14;
            // Die Karte sitzt neben dem markierten Bereich - rechts, wenn dort
            // Platz ist, sonst links, sonst darunter.
            int cx = target.getRight() + pad;
            if (cx + cw > getWidth())  cx = target.getX() - pad - cw;
            if (cx < 0)                cx = juce::jlimit (8, juce::jmax (8, getWidth() - cw - 8), target.getCentreX() - cw / 2);
            int cy = juce::jlimit (8, juce::jmax (8, getHeight() - ch - 8), target.getCentreY() - ch / 2);
            cardArea = { cx, cy, cw, ch };

            auto row = cardArea.reduced (18, 14).removeFromBottom (26);
            const int bw = 72, gap = 8;
            skipBtn.setBounds (row.removeFromLeft (bw));
            nextBtn.setBounds (row.removeFromRight (bw));
            row.removeFromRight (gap);
            backBtn.setBounds (row.removeFromRight (bw));
        }

    private:
        juce::Rectangle<int> cardArea;
    };

    // Regler mit Rechtsklick-Aktion. Ein blosser MouseListener reicht nicht:
    // JUCE-Slider starten auch beim rechten Knopf einen Zieh-Vorgang, der Wert
    // wuerde sich also mitbewegen.
    struct LockableSlider : public juce::Slider
    {
        std::function<void()> onRightClick;
        void mouseDown (const juce::MouseEvent& e) override
        {
            if (e.mods.isPopupMenu()) { if (onRightClick) onRightClick(); return; }
            juce::Slider::mouseDown (e);
        }
    };

    class ViewPanelComponent : public juce::Component
    {
    public:
        // Zwei Gruppen (User: "manchmal unklar ob sich eine Setting auf
        // Goniometer oder Starfield bezieht") - jede mit eigener Ueberschrift.
        juce::Label title, gonioHead, starHead;
        juce::TextButton gonioBtn { "Show" };
        // Starfield: nur noch Show + Mod Movement (Reduce Anim., Color und
        // Intensity entfernt - User: "weniger, einfacher, intuitiver";
        // Density deckt Reduce Anim. ab, Farbe/Intensitaet waren eins).
        juce::TextButton modMoveBtn { "Modulation" }, dimBtn { "Dim" };   // Dim = Shine min/max (wie Cmd+Klick auf die Sonne)
        // Funkelnde Sterne direkt ein-/ausblenden (User: "hide stars button direkt").
        juce::TextButton starsBtn { "Stars" };
        // Die radialen Warp-Linien getrennt schaltbar (User).
        juce::TextButton linesBtn { "Lines" };
        juce::Label      starsLabel;
        // Show: Off | Stars | Full (User: dreistufig statt Schalter).
        juce::TextButton starModeBtn[2];   // Stars | Space (kein Off mehr, User)
        juce::Label starShowLabel, photoLabel, photoNameLabel, gravityLabel;
        // Foto: < Name > (Klick ins Feld schaltet ebenfalls weiter).
        juce::TextButton photoPrevBtn { "<" }, photoNextBtn { ">" };
        // Gravity-Planet: Sci-Fi | Moon | Earth | Night (unabhaengig vom Foto).
        juce::TextButton gravityBtn[4];
        int photoIdx = 0;
        int  gravityIndex() const { for (int k = 1; k < 4; ++k) if (gravityBtn[k].getToggleState()) return k; return 0; }
        void setGravityIndex (int i) { i = juce::jlimit (0, 3, i); for (int k = 0; k < 4; ++k) gravityBtn[k].setToggleState (k == i, juce::dontSendNotification); }
        // Modus-Codes bleiben 1 = Stars, 2 = Space (0 = altes Off wird zu Stars).
        int  starModeIndex() const { return starModeBtn[1].getToggleState() ? 2 : 1; }
        // Bei "Stars" ist die Spur automatisch an -> Mod Movement und der
        // Gonio-Show-Knopf haben dort keine Wirkung und werden ausgegraut (User).
        // Foto-Zeile nur im Modern-Theme waehlbar (Wasserfarbe = Nebula,
        // Comic = gezeichneter Himmel), sonst ausgegraut.
        void setPhotoEnabled (bool on)
        {
            for (auto* c : std::initializer_list<juce::Component*> { &photoPrevBtn, &photoNextBtn, &photoNameLabel, &photoLabel })
            {
                c->setEnabled (on);
                c->setAlpha (on ? 1.0f : 0.40f);
            }
        }
        // Dim-Knopf zeigt "an", wenn Shine unten ist.
        void syncDimBtn() { dimBtn.setToggleState (dimSlider.getValue() < 0.5, juce::dontSendNotification); }
        // Kopfzeilen-Farben je Theme (Pop behaelt seine).
        void setHeadColours (juce::Colour t, juce::Colour gnio, juce::Colour star)
        {
            title.setColour (juce::Label::textColourId, t);
            gonioHead.setColour (juce::Label::textColourId, gnio);
            starHead.setColour (juce::Label::textColourId, star);
        }
        void updateModMoveEnabled()
        {
            const bool space = starModeIndex() == 2;
            for (auto* b : { &modMoveBtn, &gonioBtn })
            {
                b->setEnabled (space);
                b->setAlpha (space ? 1.0f : 0.45f);
            }
        }
        void setStarModeIndex (int i) { starModeBtn[0].setToggleState (i < 2, juce::dontSendNotification); starModeBtn[1].setToggleState (i >= 2, juce::dontSendNotification); }
        int  photoIndex() const { return photoIdx; }
        void setPhotoIndex (int i)
        {
            photoIdx = juce::jlimit (1, GoniometerComponent::kPhotoCount, i);   // 1..n, kein "None" mehr (User)
            photoNameLabel.setText (GoniometerComponent::photoName (photoIdx), juce::dontSendNotification);
        }
        // Farbe: GUI-Farben als Radio-Gruppe.
        // (Texte werden im Konstruktor gesetzt - TextButton hat einen
        //  explicit-Konstruktor, ein Array laesst sich nicht per {} fuellen.)
        juce::TextButton gonioColBtn[2];   // Theme | White (User: mehr braucht es nicht)
        // Scope-Stil (Dots/Lines/Glow) und Groesse (Normal/Small), Radio.
        juce::TextButton gonioStyleBtn[2];   // Glow | Lines (Dots entfernt, User)
        // Speed (fest Mid), Intensity, Blur und Look sind entfallen (User):
        // Glow = Solid, Lines = Soft.
        juce::Label gonioStyleLabel;
        // Tasten-Reihenfolge (User): Style = Glow | Lines | Dots, Size = Small | Normal.
        // Style-Code bleibt 0 = Dots, 1 = Lines, 2 = Glow (Goniometer).
        int gonioStyleIndex() const { return gonioStyleBtn[1].getToggleState() ? 1 : 2; }
        void setGonioStyleIndex (int i) { gonioStyleBtn[0].setToggleState (i != 1, juce::dontSendNotification); gonioStyleBtn[1].setToggleState (i == 1, juce::dontSendNotification); }
        // dimSlider heisst intern weiter so, ist aber jetzt "Shine" (hellt auf).
        juce::Slider dimSlider, speedSlider, densitySlider;
        juce::Label gonioColLabel, dimLabel, speedLabel, densityLabel;
        // Unten: "Make Default" speichert die aktuellen Werte als Startwerte,
        // "Reset" holt die gespeicherten Startwerte zurueck (User-Wunsch).
        // "Save" schreibt die View-Einstellungen ins geladene Preset (User).
        // "Save" schreibt auf den gewaehlten Speicherplatz A/B/C (ueber dem
        // Sternenfeld), "Reset" holt ihn zurueck.
        // Schliessen NUR ueber Save, Cancel oder Klick auf "VIEW" (User).
        // "Save" uebernimmt die Einstellungen fuer diese Session und schliesst,
        // "Save as Default" schreibt sie zusaetzlich als Startwerte (User).
        juce::TextButton saveBtn { "Save as Default" }, saveOnlyBtn { "Save" }, resetBtn { "Reset" }, cancelBtn { "Cancel" };
        std::function<void()> onChange, onReset, onSave, onSaveOnly, onCancel, onClose;
        void mouseUp (const juce::MouseEvent& e) override
        {
            if (title.getBounds().contains (e.getPosition()) && onClose)
                onClose();
        }

        // Palette-Index (0 = Blau, 1 = Gruen, 2 = Violett, 3 = Gold, 4 = Pink) der gewaehlten Farbe.
        static constexpr int kGonioColours = 2;
        int gonioColourIndex() const { for (int k = 1; k < kGonioColours; ++k) if (gonioColBtn[k].getToggleState()) return k; return 0; }
        void setGonioColourIndex (int idx) { for (int k = 0; k < kGonioColours; ++k) gonioColBtn[k].setToggleState (idx == k, juce::dontSendNotification); }

        ViewPanelComponent()
        {
            auto head = [] (juce::Label& l, const char* txt, juce::Colour col, float size, float kern)
            {
                l.setText (txt, juce::dontSendNotification);
                l.setJustificationType (juce::Justification::centredLeft);
                l.setFont (juce::Font (juce::FontOptions (size, juce::Font::bold)).withExtraKerningFactor (kern));
                l.setColour (juce::Label::textColourId, col);
            };
            head (title,     "VIEW",       juce::Colour (0xffb968ff), 16.0f, 0.18f);   // groesser (User)
            head (gonioHead, "GONIOMETER", juce::Colour (0xff5be3c7), 14.0f, 0.14f);
            head (starHead,  "STARFIELD",  juce::Colour (0xff6bb8ff), 14.0f, 0.14f);
            for (auto* l : { &title, &gonioHead, &starHead }) addAndMakeVisible (*l);

            for (auto* b : { &gonioBtn, &modMoveBtn })
            {
                b->setClickingTogglesState (true);
                b->setWantsKeyboardFocus (false);
                addAndMakeVisible (*b);
                b->onClick = [this] { if (onChange) onChange(); };
            }
            for (auto* b : { &saveBtn, &saveOnlyBtn, &resetBtn, &cancelBtn })
            {
                b->setWantsKeyboardFocus (false);
                addAndMakeVisible (*b);
            }
            linesBtn.setClickingTogglesState (true);
            linesBtn.setToggleState (true, juce::dontSendNotification);
            linesBtn.setWantsKeyboardFocus (false);
            addAndMakeVisible (linesBtn);
            linesBtn.onClick = [this] { if (onChange) onChange(); };
            linesBtn.setTooltip ("The radial warp lines - the twinkling stars stay");
            starsBtn.setClickingTogglesState (true);
            starsBtn.setToggleState (true, juce::dontSendNotification);
            starsBtn.setWantsKeyboardFocus (false);
            starsBtn.setButtonText ("Show");
            addAndMakeVisible (starsBtn);
            starsBtn.onClick = [this] { if (onChange) onChange(); };
            dimBtn.setClickingTogglesState (false);
            dimBtn.setWantsKeyboardFocus (false);
            dimBtn.getProperties().set ("noGlow", true);
            addAndMakeVisible (dimBtn);
            dimBtn.onClick = [this] { dimSlider.setValue (dimSlider.getValue() < 0.5 ? 1.0 : 0.0, juce::dontSendNotification); syncDimBtn(); if (onChange) onChange(); };
            saveBtn.onClick        = [this] { if (onSave) onSave(); };
            saveOnlyBtn.onClick    = [this] { if (onSaveOnly) onSaveOnly(); };
            saveOnlyBtn.setTooltip ("Keep these view settings for this session and close");
            resetBtn.onClick       = [this] { if (onReset) onReset(); };
            cancelBtn.onClick      = [this] { if (onCancel) onCancel(); };
            saveBtn.setTooltip ("Save these view settings as your default and close");
            resetBtn.setTooltip ("Back to your saved default");
            cancelBtn.setTooltip ("Close and undo the changes made since opening");
            title.setInterceptsMouseClicks (false, false);   // Klick auf VIEW landet im Panel -> onClose
            title.setTooltip ("Click to close");

            static const char* const starModeNames[2] = { "Stars", "Space" };
            static const char* const gravityNames[4]  = { "Sci-Fi", "Moon", "Earth", "Night" };
            for (int i = 0; i < 4; ++i)
            {
                gravityBtn[i].setButtonText (gravityNames[i]);
                gravityBtn[i].setRadioGroupId (7);
                gravityBtn[i].setClickingTogglesState (true);
                gravityBtn[i].setWantsKeyboardFocus (false);
                addAndMakeVisible (gravityBtn[i]);
                gravityBtn[i].onClick = [this] { if (onChange) onChange(); };
            }
            gravityBtn[0].setToggleState (true, juce::dontSendNotification);
            for (int i = 0; i < 2; ++i)
            {
                starModeBtn[i].setButtonText (starModeNames[i]);
                starModeBtn[i].setRadioGroupId (6);
                starModeBtn[i].setClickingTogglesState (false);   // Klick auf die aktive Wahl wechselt zur anderen (User)
                starModeBtn[i].setWantsKeyboardFocus (false);
                addAndMakeVisible (starModeBtn[i]);
                starModeBtn[i].onClick = [this, i] { const int mine = (i == 0 ? 1 : 2); setStarModeIndex (starModeIndex() == mine ? 3 - mine : mine); if (onChange) onChange(); };
            }
            starModeBtn[1].setToggleState (true, juce::dontSendNotification);
            for (auto* b : { &photoPrevBtn, &photoNextBtn })
            {
                b->setWantsKeyboardFocus (false);
                addAndMakeVisible (*b);
            }
            photoPrevBtn.onClick = [this] { setPhotoIndex ((photoIdx - 1 + GoniometerComponent::kPhotoCount - 1) % GoniometerComponent::kPhotoCount + 1); if (onChange) onChange(); };
            photoNextBtn.onClick = [this] { setPhotoIndex (photoIdx % GoniometerComponent::kPhotoCount + 1); if (onChange) onChange(); };
            photoNameLabel.setJustificationType (juce::Justification::centred);
            photoNameLabel.setFont (juce::Font (juce::FontOptions (15.0f, juce::Font::bold)).withExtraKerningFactor (0.06f));
            photoNameLabel.setColour (juce::Label::textColourId, juce::Colours::white);
            addAndMakeVisible (photoNameLabel);
            setPhotoIndex (1);
            // Kein Leucht-Hof um aktive Knoepfe im Panel (User).
            for (auto* c : getChildren())
                if (auto* b = dynamic_cast<juce::TextButton*> (c))
                    b->getProperties().set ("noGlow", true);


            static const char* const styleNames[2] = { "Glow", "Lines" };
            for (int i = 0; i < 2; ++i)
            {
                gonioStyleBtn[i].setButtonText (styleNames[i]);
                gonioStyleBtn[i].setRadioGroupId (3);
                gonioStyleBtn[i].setClickingTogglesState (false);   // Klick auf die aktive Wahl wechselt zur anderen (User)
                gonioStyleBtn[i].setWantsKeyboardFocus (false);
                addAndMakeVisible (gonioStyleBtn[i]);
                gonioStyleBtn[i].onClick = [this, i] { const int mine = (i == 0 ? 2 : 1); setGonioStyleIndex (gonioStyleIndex() == mine ? 3 - mine : mine); if (onChange) onChange(); };
            }
            gonioStyleBtn[0].setToggleState (true, juce::dontSendNotification);
            // Farb-Radiogruppen (JUCE-Radio-IDs 1 und 2)
            static const char* const gonioNames[kGonioColours] = { "Theme", "White" };
            for (int i = 0; i < kGonioColours; ++i)
            {
                gonioColBtn[i].setButtonText (gonioNames[i]);
                gonioColBtn[i].setRadioGroupId (1);
                // Wie bei Scene und Style: ein Klick auf die bereits aktive
                // Wahl springt zur anderen (User).
                gonioColBtn[i].setClickingTogglesState (false);
                gonioColBtn[i].setWantsKeyboardFocus (false);
                addAndMakeVisible (gonioColBtn[i]);
                gonioColBtn[i].onClick = [this, i]
                {
                    setGonioColourIndex (gonioColourIndex() == i ? (i == 0 ? 1 : 0) : i);
                    if (onChange) onChange();
                };
            }
            gonioColBtn[0].setToggleState (true, juce::dontSendNotification);

            // "Intensity" statt "Saturation" (User) - trifft es fuer beide
            // besser: beim Goniometer die Farbkraft der Spur, beim Sternenfeld
            // die Faerbung der Linien. Starfield endet bei 75 % (User).
            struct Def { juce::Slider* s; juce::Label* l; const char* txt; double lo, hi, def; };
            const Def defs[] = {
                // Shine: nur positiv, kein Mittelanker. Standard = hell, also Dim AUS (User).
                { &dimSlider,      &dimLabel,      "Shine",     0.0,  1.0,  1.0  },
                // Speed: 2 % Minimum (User: "ganz anhalten macht keinen Sinn").
                { &speedSlider,    &speedLabel,    "Speed",     0.02, 2.0,  1.0  },
                { &densitySlider,  &densityLabel,  "Density",   0.2,  1.5,  1.0  },
            };
            for (const auto& d : defs)
            {
                d.s->setSliderStyle (juce::Slider::LinearHorizontal);
                d.s->setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
                d.s->setRange (d.lo, d.hi, 0.001);
                // Regler mit Standardwert INNERHALB des Bereichs: der Standard
                // sitzt genau in der Mitte (Skew), und die Mitte ist auf der
                // Linie markiert; gefuellt wird von der Mitte aus (User:
                // "mittig ausrichten und einheitlich machen").
                const bool centred = d.def > d.lo + 1.0e-6 && d.def < d.hi - 1.0e-6;
                if (centred)
                    d.s->setSkewFactorFromMidPoint (d.def);
                d.s->getProperties().set ("viewCentre", centred);
                d.s->setValue (d.def, juce::dontSendNotification);
                d.s->setDoubleClickReturnValue (true, d.def, juce::ModifierKeys::commandModifier);
                d.s->getProperties().set ("viewSlider", true);
                d.s->setWantsKeyboardFocus (false);
                addAndMakeVisible (*d.s);
                d.s->onValueChange = [this] { if (onChange) onChange(); };

                d.l->setText (d.txt, juce::dontSendNotification);
                d.l->setJustificationType (juce::Justification::centredLeft);
                d.l->setFont (juce::Font (juce::FontOptions (15.0f, juce::Font::bold)).withExtraKerningFactor (0.06f));
                d.l->setColour (juce::Label::textColourId, juce::Colour (0xffb5b9c2));
                addAndMakeVisible (*d.l);
            }
            gonioStyleLabel.setText ("Style", juce::dontSendNotification);
            starShowLabel.setText ("Motion", juce::dontSendNotification);
            starsLabel.setText ("Stars", juce::dontSendNotification);
            photoLabel.setText ("Photo", juce::dontSendNotification);
            gravityLabel.setText ("Gravity", juce::dontSendNotification);
            for (auto* l : { &gonioStyleLabel, &starShowLabel, &starsLabel, &photoLabel, &gravityLabel })
            {
                l->setJustificationType (juce::Justification::centredLeft);
                l->setFont (juce::Font (juce::FontOptions (15.0f, juce::Font::bold)).withExtraKerningFactor (0.06f));
                l->setColour (juce::Label::textColourId, juce::Colour (0xffb5b9c2));
                addAndMakeVisible (*l);
            }
            gonioColLabel.setText ("Color", juce::dontSendNotification);
            gonioColLabel.setJustificationType (juce::Justification::centredLeft);
            gonioColLabel.setFont (juce::Font (juce::FontOptions (15.0f, juce::Font::bold)).withExtraKerningFactor (0.06f));
            gonioColLabel.setColour (juce::Label::textColourId, juce::Colour (0xffb5b9c2));
            addAndMakeVisible (gonioColLabel);
        }

        void paint (juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            g.setColour (juce::Colour (0xff17191f));   // voll deckend (User: "zu durchsichtig")
            g.fillRoundedRectangle (b, 10.0f);
            g.setColour (juce::Colours::white.withAlpha (0.16f));
            g.drawRoundedRectangle (b.reduced (0.5f), 10.0f, 1.0f);
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.fillRect (b.getX() + 14.0f, (float) groupDividerY, b.getWidth() - 28.0f, 1.0f);
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced (14, 12);
            // Unterste Zeile: die beiden Knoepfe, rechtsbuendig.
            {
                auto br = r.removeFromBottom (28);
                r.removeFromBottom (10);
                saveBtn.setBounds (br.removeFromRight (118));
                br.removeFromRight (6);
                saveOnlyBtn.setBounds (br.removeFromRight (56));
                br.removeFromRight (6);
                cancelBtn.setBounds (br.removeFromRight (64));
                br.removeFromRight (6);
                resetBtn.setBounds (br.removeFromRight (58));
            }
            // Etwas groessere Zeilen als zuvor (User: "Schrift ist bisschen
            // klein" / "Kasten von den Zeilen ausfuellen").
            const int rowH = 31, gap = 7, labelW = 100;   // groesser (User: "alles bisschen klein")
            auto sliderRow = [&] (juce::Label& l, juce::Slider& s)
            {
                auto row = r.removeFromTop (rowH);
                l.setBounds (row.removeFromLeft (labelW));
                s.setBounds (row);
                r.removeFromTop (2);
            };
            auto colourRow = [&] (juce::Label& l, juce::TextButton* btns, int n = 3)
            {
                auto row = r.removeFromTop (rowH);
                l.setBounds (row.removeFromLeft (labelW));
                const int w = (row.getWidth() - gap * (n - 1)) / n;
                for (int i = 0; i < n; ++i)
                {
                    btns[i].setBounds (row.removeFromLeft (w));
                    if (i < n - 1) row.removeFromLeft (gap);
                }
                r.removeFromTop (gap);
            };

            title.setBounds (r.removeFromTop (22));
            r.removeFromTop (16);   // mehr Abstand unter der Hauptueberschrift (User)

            // --- Goniometer ---
            auto gh = r.removeFromTop (rowH);
            gonioHead.setBounds (gh.removeFromLeft (labelW + 10));
            gonioBtn.setBounds (gh.removeFromLeft (74));
            r.removeFromTop (gap);
            colourRow (gonioStyleLabel, gonioStyleBtn, 2);
            colourRow (gonioColLabel, gonioColBtn, kGonioColours);

            r.removeFromTop (10);
            groupDividerY = r.getY();
            r.removeFromTop (12);

            // --- Starfield ---
            // Show zuerst (in der Kopfzeile), Mod Movement darunter - der
            // Schalter haengt von "Space" ab (User).
            auto sh = r.removeFromTop (rowH);
            starHead.setBounds (sh.removeFromLeft (labelW + 10));
            {
                const int w = (sh.getWidth() - gap) / 2;
                starModeBtn[0].setBounds (sh.removeFromLeft (w));
                sh.removeFromLeft (gap);
                starModeBtn[1].setBounds (sh);
            }
            r.removeFromTop (gap);
            {
                auto row = r.removeFromTop (rowH);
                starShowLabel.setBounds (row.removeFromLeft (labelW));
                modMoveBtn.setBounds (row.removeFromLeft ((row.getWidth() - gap) / 2));
                row.removeFromLeft (gap);
                dimBtn.setBounds (row);
                r.removeFromTop (gap);
            }
            {
                // Eigene Zeile: die funkelnden Sterne direkt aus- und
                // wieder einblenden - wirkt in beiden Ansichten (User).
                auto row = r.removeFromTop (rowH);
                starsLabel.setBounds (row.removeFromLeft (labelW));
                const int halfW = (row.getWidth() - gap) / 2;
                starsBtn.setBounds (row.removeFromLeft (halfW));
                row.removeFromLeft (gap);
                linesBtn.setBounds (row.removeFromLeft (halfW));
                r.removeFromTop (gap);
            }
            // Foto- und Gravity-Zeile entfernt: beides ist fest je Theme
            // (User: "redundante View-Settings sparen").
            for (auto* c : std::initializer_list<juce::Component*> { &photoLabel, &photoPrevBtn, &photoNextBtn, &photoNameLabel, &gravityLabel,
                                                                    &gravityBtn[0], &gravityBtn[1], &gravityBtn[2], &gravityBtn[3] })
                c->setVisible (false);
            // Shine-Regler entfernt (User): nur noch min/max per Klick auf die
            // Sonne; der Slider bleibt unsichtbar als Wertetraeger.
            dimLabel.setVisible (false);
            dimSlider.setVisible (false);
            sliderRow (speedLabel, speedSlider);
            sliderRow (densityLabel, densitySlider);
        }

    private:
        int groupDividerY = 0;
    };

    // ===== PRISM-Bandleiste =====
    // Waagerechte Frequenzleiste mit zwei ziehbaren Griffen; dazwischen ist
    // der Bereich hervorgehoben, in dem die Verbreiterung wirkt.
    //
    // Bewusst KEINE zwei Drehregler: der Footer ist breit und flach - die
    // denkbar schlechteste Form fuer Drehregler und die beste fuer eine
    // Leiste. Vor allem aber SIEHT man hier den Bereich.
    // Ebenfalls bewusst kein Mehrfach-Klick-Schalter mit festen Stufen:
    // Frequenzen sind stufenlos, feste Werte passen nie.
    //
    // Header-only nach demselben Muster wie CorrelationMeterComponent, damit
    // KEINE neue .cpp und kein CMakeLists-Eintrag noetig ist.
    class PrismBandComponent : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        PrismBandComponent (juce::AudioProcessorValueTreeState& state)
            : apvts (state) {}

        void setActive (bool a) { if (a != active) { active = a; repaint(); } }
        // Alle drei Sektionen nehmen den Focus heraus: die Leiste wirkt dann
        // auf nichts und wird gedimmt gezeichnet (User).
        void setAllBypassed (bool b) { if (b != allBypassed) { allBypassed = b; repaint(); } }

        // Linke Zone der Komponente, in der der Ein/Aus-Knopf sitzt (User:
        // "Einfach einen On/Off Button, der ... in einer verschmolzenen
        // Grafik mit dem Frequenzmeter ist"). Die Leiste zeichnet ihren
        // Hintergrund ueber die volle Breite INKLUSIVE dieser Zone, die
        // eigentliche Frequenzskala beginnt erst dahinter - der Knopf sitzt
        // dadurch sichtbar IN der Leiste, nicht daneben. Der Knopf selbst
        // ist eine normale Schwester-Komponente, die per toFront() ueber
        // dieser liegt.
        void setLeftInset (int px) { leftInset = px; repaint(); }

        // Frequenz als kurzer Text: unter 1000 glatt in Hz, darueber in k mit
        // hoechstens einer Nachkommastelle und Punkt statt Komma (User:
        // "keine Kommas ... 1k bzw 1.1k, 3.4k").
        static juce::String hzText (float hz)
        {
            // Unter 1 kHz auf 5 Hz gerundet - eine Zahl, die bei jedem Pixel
            // um ein paar Hertz springt, liest sich wie ein Messfehler (User).
            if (hz < 1000.0f)
                return juce::String (juce::roundToInt (hz / 5.0f) * 5);
            const float k = hz / 1000.0f;
            if (k < 10.0f)
            {
                const int tenths = juce::roundToInt (k * 10.0f);
                return (tenths % 10 == 0) ? juce::String (tenths / 10) + "k"
                                          : juce::String (tenths / 10) + "." + juce::String (tenths % 10) + "k";
            }
            return juce::String (juce::roundToInt (k)) + "k";
        }
        juce::Rectangle<float> stripBounds() const
        {
            return getLocalBounds().toFloat().reduced (1.0f).withTrimmedLeft ((float) leftInset);
        }

        void paint (juce::Graphics& g) override
        {
            auto full = getLocalBounds().toFloat().reduced (1.0f);
            if (allBypassed) g.setOpacity (0.45f);
            auto b = stripBounds();
            const float x1 = xForHz (hzOf (LCRMSAudioProcessor::ID_PRISM_LO), b);
            const float x2 = xForHz (hzOf (LCRMSAudioProcessor::ID_PRISM_HI), b);

            // Knopf und Skala als ZWEI Flaechen mit Luecke dazwischen (User:
            // "on off button anders machen"): links eine eigene, dunklere
            // Kachel fuer das Power-Icon, rechts die Skala. Zusammengehoerig
            // durch gleiche Hoehe und Rundung, getrennt durch die Fuge.
            // Knopf-Kachel und Skala sind EINE Flaeche ohne Fuge (User); die
            // Kachel ist nur dunkler abgesetzt, und die Trennlinie ist exakt
            // der 1-px-Rahmen der Kachel selbst (gleiche Staerke, gleiche Farbe).
            g.setColour (juce::Colour (0xff14161a));
            g.fillRoundedRectangle (full, 4.0f);
            if (leftInset > 0)
            {
                auto zone = full.withWidth ((float) leftInset);
                juce::Path zp;
                zp.addRoundedRectangle (zone.getX(), zone.getY(), zone.getWidth(), zone.getHeight(),
                                        4.0f, 4.0f, true, false, true, false);
                g.setColour (juce::Colour (0xff101218));
                g.fillPath (zp);
                g.setColour (juce::Colours::white.withAlpha (active ? 0.22f : 0.12f));
                g.strokePath (zp, juce::PathStrokeType (1.0f));
            }

            // Frequenz-Markierungen MIT Beschriftung - stumme Striche werfen
            // nur die Frage auf, was sie bedeuten sollen. 500 Hz statt 3k
            // (User: "Ich moechte 500 Hz als Markierung sehen. 3k nicht
            // wichtig. 5k oder erst wieder 10k.") - fuer Stimmen ist 500 die
            // Grenze zwischen Koerper und Mitten, 3k war ein technischer Wert.
            // 200 - 1k - 10k (User-Korrektur nach 100/500/1k/5k/10k: "nicht
            // ideal"). Drei Marken reichen zur Orientierung; mehr wird Skala.
            static const float tickHz [3] = { 200.0f, 1000.0f, 10000.0f };
            static const char* tickTxt[3] = { "200", "1k", "10k" };
            g.setFont (juce::Font (juce::FontOptions (8.0f)));
            for (int i = 0; i < 3; ++i)
            {
                const float x = xForHz (tickHz[i], b);
                // Beschriftung etwas hoeher, Balken dafuer etwas kuerzer (User).
                g.setColour (juce::Colours::white.withAlpha (0.10f));
                g.fillRect (x - 0.5f, b.getY() + 1.0f, 1.0f, b.getHeight() - 13.0f);
                g.setColour (juce::Colours::white.withAlpha (0.30f));
                g.drawText (tickTxt[i], juce::Rectangle<float> (x - 14.0f, b.getBottom() - 11.5f, 28.0f, 9.0f),
                            juce::Justification::centred, false);
            }

            // Lila wie das Power-Icon in der Kachel (User: "erkennt besser,
            // dass es zusammengehoert").
            auto bandCol = active ? themePalette().prism : juce::Colour (0xff4a4e57);   // je Theme (User)
            g.setColour (bandCol.withAlpha (active ? 0.28f : 0.15f));
            g.fillRoundedRectangle (x1, b.getY() + 1.5f, juce::jmax (2.0f, x2 - x1),
                                     b.getHeight() - 14.0f, 2.0f);

            g.setColour (bandCol.withAlpha (active ? 0.95f : 0.45f));
            g.fillRoundedRectangle (x1 - 2.0f, b.getY(), 4.0f, b.getHeight() - 12.0f, 2.0f);
            g.fillRoundedRectangle (x2 - 2.0f, b.getY(), 4.0f, b.getHeight() - 12.0f, 2.0f);

            // Aussenrahmen um die GANZE Leiste (Kachel + Skala, eine Flaeche).
            g.setColour (juce::Colours::white.withAlpha (0.18f));
            g.drawRoundedRectangle (full, 4.0f, 1.0f);
        
            // Waehrend des Ziehens die angefasste Kante beziffern (User) -
            // sonst sieht man nie, wo genau man landet. Kleines dunkles
            // Schild direkt an der Kante, damit es auf jedem Theme lesbar
            // bleibt; beim Verschieben des ganzen Bereichs beide Kanten.
            if (dragMode >= 0 && showHz)
            {
                auto chip = [&] (float x, const juce::String& txt, bool alignRight)
                {
                    const juce::Font f (juce::FontOptions (10.5f, juce::Font::bold));
                    const float tw = juce::GlyphArrangement::getStringWidth (f, txt) + 10.0f;
                    const float th = 14.0f;
                    float cx = alignRight ? x - tw * 0.5f - 3.0f : x + tw * 0.5f + 3.0f;
                    cx = juce::jlimit (b.getX() + tw * 0.5f, b.getRight() - tw * 0.5f, cx);
                    juce::Rectangle<float> r (cx - tw * 0.5f, b.getCentreY() - th * 0.5f, tw, th);
                    g.setColour (juce::Colour (0xef0b0c10));
                    g.fillRoundedRectangle (r, 3.5f);
                    g.setColour (juce::Colours::white.withAlpha (0.18f));
                    g.drawRoundedRectangle (r.reduced (0.5f), 3.5f, 1.0f);
                    g.setColour (juce::Colours::white.withAlpha (0.92f));
                    g.setFont (f);
                    g.drawText (txt, r, juce::Justification::centred, false);
                };
                if (dragMode == 0 || dragMode == 2)
                    chip (x1, hzText (hzOf (LCRMSAudioProcessor::ID_PRISM_LO)), false);
                if (dragMode == 1 || dragMode == 2)
                    chip (x2, hzText (hzOf (LCRMSAudioProcessor::ID_PRISM_HI)), true);
            }
}

        // Doppelklick schaltet PRISM an/aus, Cmd-Klick setzt den Bereich auf
        // den Standard zurueck - beides Gesten, die man von Reglern kennt.
        void mouseDoubleClick (const juce::MouseEvent&) override
        {
            if (auto* p = apvts.getParameter (LCRMSAudioProcessor::ID_PRISM_ON))
                p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f);
        }

        void mouseDown (const juce::MouseEvent& e) override
        {
            if (e.mods.isCommandDown())
            {
                if (auto* lo = apvts.getParameter (LCRMSAudioProcessor::ID_PRISM_LO))
                    lo->setValueNotifyingHost (lo->getDefaultValue());
                if (auto* hi = apvts.getParameter (LCRMSAudioProcessor::ID_PRISM_HI))
                    hi->setValueNotifyingHost (hi->getDefaultValue());
                repaint();
                dragMode = -1;   // dieser Klick zieht nichts mehr
                return;
            }

            auto b = stripBounds();
            const float xLo = xForHz (hzOf (LCRMSAudioProcessor::ID_PRISM_LO), b);
            const float xHi = xForHz (hzOf (LCRMSAudioProcessor::ID_PRISM_HI), b);
            const float x = (float) e.x;

            // Fangbereich um die Griffe - ohne ihn muesste man die 4px breiten
            // Griffe pixelgenau treffen. Die Flaeche dazwischen bleibt frei
            // fuer das Verschieben des ganzen Bandes.
            constexpr float kGrab = 9.0f;
            if (std::abs (x - xLo) <= kGrab)        dragMode = 0;
            else if (std::abs (x - xHi) <= kGrab)   dragMode = 1;
            else if (x > xLo && x < xHi)            dragMode = 2;
            else if (clickJumps)                    dragMode = (std::abs (x - xLo) < std::abs (x - xHi)) ? 0 : 1;
            else                                    dragMode = -1;   // Klick ins Leere tut nichts (Menue-Option)

            dragStartX  = x;
            dragStartY  = (float) e.y;
            dragStartLo = hzToLog (hzOf (LCRMSAudioProcessor::ID_PRISM_LO));
            dragStartHi = hzToLog (hzOf (LCRMSAudioProcessor::ID_PRISM_HI));

            if (dragMode == 0 || dragMode == 1)
                mouseDrag (e);
        }

        // Menue-Option "Prism: Click Moves Nearest Edge".
        void setClickJumps (bool b) { clickJumps = b; }
        // Hz-Anzeige beim Ziehen - Menue-Option, Standard aus (User).
        void setShowHz (bool b) { showHz = b; }

        // Mausrad/Trackpad: Bereich um seine Mitte gleichmaessig weiten oder
        // engen (User-Wunsch), in kleinen Schritten im logarithmischen Raum -
        // also in Oktaven, nicht in Hz. Nach oben scrollen = weiter.
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
        {
            auto* lo = apvts.getParameter (LCRMSAudioProcessor::ID_PRISM_LO);
            auto* hi = apvts.getParameter (LCRMSAudioProcessor::ID_PRISM_HI);
            if (lo == nullptr || hi == nullptr)
                return;
            const float step = juce::jlimit (-0.06f, 0.06f, wheel.deltaY * 0.25f);
            if (std::abs (step) < 1.0e-5f)
                return;
            float a = hzToLog (hzOf (LCRMSAudioProcessor::ID_PRISM_LO));
            float b = hzToLog (hzOf (LCRMSAudioProcessor::ID_PRISM_HI));
            const float mid = 0.5f * (a + b);
            float half = 0.5f * (b - a) + step;
            half = juce::jlimit (0.5f * kMinWidthLog, 0.5f, half);
            a = juce::jlimit (0.0f, 1.0f, mid - half);
            b = juce::jlimit (0.0f, 1.0f, mid + half);
            setHz (lo, logToHz (a));
            setHz (hi, logToHz (b));
            repaint();
        }

        void mouseMove (const juce::MouseEvent& e) override
        {
            auto b = stripBounds();
            const float xLo = xForHz (hzOf (LCRMSAudioProcessor::ID_PRISM_LO), b);
            const float xHi = xForHz (hzOf (LCRMSAudioProcessor::ID_PRISM_HI), b);
            const float x = (float) e.x;
            constexpr float kGrab = 9.0f;

            if (std::abs (x - xLo) <= kGrab || std::abs (x - xHi) <= kGrab)
                setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
            else if (x > xLo && x < xHi)
                setMouseCursor (juce::MouseCursor::DraggingHandCursor);
            else
                setMouseCursor (juce::MouseCursor::NormalCursor);
        }

        void mouseUp (const juce::MouseEvent&) override
        {
            if (dragMode >= 0) { dragMode = -1; repaint(); }
        }

        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (dragMode < 0)
                return;
            repaint();   // die Hz-Anzeige laeuft mit

            auto b = stripBounds();
            if (b.getWidth() < 1.0f)
                return;

            auto* lo = apvts.getParameter (LCRMSAudioProcessor::ID_PRISM_LO);
            auto* hi = apvts.getParameter (LCRMSAudioProcessor::ID_PRISM_HI);
            if (lo == nullptr || hi == nullptr)
                return;

            // Die Mausposition wird ueber die (verzerrte) Leisten-Skala in Hz
            // und von dort in den REIN LOGARITHMISCHEN Raum umgerechnet. Alle
            // Bereichsrechnungen laufen dort, weil dort gleiche Abstaende auch
            // gleiche Frequenzverhaeltnisse bedeuten - auf der verzerrten
            // Leiste waere das nicht so.
            float curLog = hzToLog (hzForX ((float) e.x, b));
            // Feinmodus wie bei den Reglern: mit Cmd/Strg bewegt sich die Kante
            // nur ein Viertel so weit. Gilt fuer Kanten UND fuer das Verschieben
            // des ganzen Bereichs, weil beide auf curLog aufbauen.
            if (e.mods.isCommandDown())
            {
                const float anchorLog = hzToLog (hzForX (dragStartX, b));
                curLog = anchorLog + (curLog - anchorLog) * 0.25f;
            }

            if (dragMode == 2)
            {
                const float startLog = hzToLog (hzForX (dragStartX, b));
                const float d = curLog - startLog;

                // Senkrecht ziehen = Bereich um seine Mitte weiten/engen, wie
                // mit dem Mausrad (User: "click + hold move -> wie Mausrad/
                // Trackpad"). Nach oben = weiter. Kleine Totzone, damit ein
                // leicht schraeges Verschieben die Breite nicht antastet.
                float dy = dragStartY - (float) e.y;
                constexpr float kDead = 5.0f;
                dy = (std::abs (dy) <= kDead) ? 0.0f : dy - (dy > 0.0f ? kDead : -kDead);
                const float width = juce::jlimit (kMinWidthLog, 1.0f,
                                                  (dragStartHi - dragStartLo) + dy * 0.006f);
                const float mid0  = 0.5f * (dragStartLo + dragStartHi) + d;

                float loT = mid0 - 0.5f * width;
                float hiT = mid0 + 0.5f * width;

                // ===== Gummiband am Rand =====
                // Statt am Rand einfach zu stoppen, faehrt die "lose" Seite
                // nach und das Band schiebt sich in die Ecke zusammen.
                // Wichtig: NICHT 1:1 mit der Maus, sondern gedaempft ueber
                // eine Saettigungskurve - je weiter man druckt, desto weniger
                // passiert zusaetzlich. Dadurch fuehlt es sich gummiartig an
                // statt abrupt, und man kann das Band nicht versehentlich
                // komplett zusammenquetschen.
                const float maxSqueeze = juce::jmax (0.0f, width - kMinWidthLog);
                constexpr float kRubber = 0.25f;   // Daempfung

                if (hiT > 1.0f)
                {
                    const float over = hiT - 1.0f;
                    hiT = 1.0f;
                    loT = (1.0f - width) + maxSqueeze * (1.0f - std::exp (-over / kRubber));
                }
                else if (loT < 0.0f)
                {
                    const float over = -loT;
                    loT = 0.0f;
                    hiT = width - maxSqueeze * (1.0f - std::exp (-over / kRubber));
                }

                setHz (lo, logToHz (juce::jlimit (0.0f, 1.0f, loT)));
                setHz (hi, logToHz (juce::jlimit (0.0f, 1.0f, hiT)));
            }
            else if (dragMode == 0)
            {
                const float maxLo = hzToLog (hzOf (LCRMSAudioProcessor::ID_PRISM_HI)) - kMinWidthLog;
                setHz (lo, logToHz (juce::jlimit (0.0f, 1.0f, juce::jmin (curLog, maxLo))));
            }
            else
            {
                const float minHi = hzToLog (hzOf (LCRMSAudioProcessor::ID_PRISM_LO)) + kMinWidthLog;
                setHz (hi, logToHz (juce::jlimit (0.0f, 1.0f, juce::jmax (curLog, minHi))));
            }
            repaint();
        }

    private:
        int leftInset = 0;
        bool allBypassed = false;
        bool clickJumps = false;   // Klick ins Leere tut nichts (Standard, User)
        bool showHz = false;
        // ===== Zwei verschiedene Abbildungen, bewusst getrennt =====
        //
        // 1) LEISTEN-SKALA (xForHz/hzForX): absichtlich VERZERRT, nicht rein
        //    logarithmisch. Rein logarithmisch bekaeme 20-200 Hz ein volles
        //    Drittel der Breite - ein Bereich, in dem bei Stimmen praktisch
        //    nie etwas passiert -, waehrend sich die interessanten Mitten
        //    draengen. Die Stuetzpunkte unten stauchen daher den Bass und
        //    geben Mitten und Hoehen mehr Platz (User-Wunsch).
        // 2) RECHEN-SKALA (hzToLog/logToHz): rein logarithmisch. Alle
        //    Bereichs- und Verschiebe-Rechnungen laufen hier, denn nur hier
        //    bedeuten gleiche Abstaende gleiche Frequenzverhaeltnisse. Beim
        //    Verschieben bleibt die Bandbreite in Oktaven dadurch exakt
        //    erhalten, auch wenn sich die Breite auf dem Bildschirm aendert.
        // Korrektur (User): der Bass ist der unwichtigste Bereich - unter
        // 200 Hz gibt es Subbass oder eben nicht, dazwischen trifft niemand
        // eine bewusste Entscheidung. Er wird deshalb noch schmaler als
        // urspruenglich; der gewonnene Platz geht an 200 Hz bis 1 kHz, wo
        // die untere Kante tatsaechlich gesetzt wird (jetzt 37 % statt 29 %).
        static constexpr int kNumAnchors = 6;
        static const float* anchorHz()   { static const float v[kNumAnchors] = { 20.0f, 200.0f, 1000.0f, 3000.0f, 10000.0f, 20000.0f }; return v; }
        static const float* anchorPos()  { static const float v[kNumAnchors] = { 0.0f,  0.13f,  0.50f,   0.70f,   0.90f,    1.0f }; return v; }

        static float hzToStrip (float hz)
        {
            hz = juce::jlimit (20.0f, 20000.0f, hz);
            const float* fa = anchorHz();
            const float* pa = anchorPos();
            for (int i = 0; i < kNumAnchors - 1; ++i)
            {
                if (hz <= fa[i + 1])
                {
                    const float t = (std::log (hz) - std::log (fa[i]))
                                  / (std::log (fa[i + 1]) - std::log (fa[i]));
                    return pa[i] + t * (pa[i + 1] - pa[i]);
                }
            }
            return 1.0f;
        }
        static float stripToHz (float n)
        {
            n = juce::jlimit (0.0f, 1.0f, n);
            const float* fa = anchorHz();
            const float* pa = anchorPos();
            for (int i = 0; i < kNumAnchors - 1; ++i)
            {
                if (n <= pa[i + 1])
                {
                    const float t = (n - pa[i]) / (pa[i + 1] - pa[i]);
                    return std::exp (std::log (fa[i]) + t * (std::log (fa[i + 1]) - std::log (fa[i])));
                }
            }
            return 20000.0f;
        }

        static float hzToLog (float hz)
        {
            return juce::jlimit (0.0f, 1.0f,
                (std::log (juce::jlimit (20.0f, 20000.0f, hz)) - std::log (20.0f))
                / (std::log (20000.0f) - std::log (20.0f)));
        }
        static float logToHz (float n)
        {
            return std::exp (std::log (20.0f) + juce::jlimit (0.0f, 1.0f, n)
                             * (std::log (20000.0f) - std::log (20.0f)));
        }

        // Mindest-Bandbreite Faktor 1,5, ausgedrueckt im logarithmischen Raum.
        static constexpr float kMinWidthLog = 0.0587f;  // log(1.5)/log(1000)

        static float xForHz (float hz, juce::Rectangle<float> b) { return b.getX() + hzToStrip (hz) * b.getWidth(); }
        static float hzForX (float x, juce::Rectangle<float> b)  { return stripToHz ((x - b.getX()) / juce::jmax (1.0f, b.getWidth())); }

        float hzOf (const char* id) const
        {
            if (auto* p = apvts.getRawParameterValue (id))
                return p->load();
            return 20.0f;
        }
        static void setHz (juce::RangedAudioParameter* p, float hz)
        {
            p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (20.0f, 20000.0f, hz)));
        }

        juce::AudioProcessorValueTreeState& apvts;
        int   dragMode = -1;         // -1 = nichts, 0 = Lo, 1 = Hi, 2 = ganzes Band
                                     // (Bug: mit 0 stand beim Oeffnen sofort ein Hz-Schild da)
        float dragStartX = 0.0f, dragStartY = 0.0f;
        float dragStartLo = 0.0f, dragStartHi = 1.0f;
        bool  active = false;
    };

    void timerCallback() override;
    void drawLogo (juce::Graphics& g, juce::Rectangle<float> area);
    void paintContent (juce::Graphics& g);
    void paintOverContent (juce::Graphics& g);
    void layoutContent();
    void setupPowerButton (juce::TextButton& button, const juce::String& paramId,
                            std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& attachment,
                            int soloValue);
    void setupSoloButton (juce::TextButton& button, int soloValue);
    // Section-Lock-Icon (User-Wunsch: "Sections ausschliessen" von Chaos/
    // Mutate und Breathe) - toggelt processor.setSectionLocked() fuer die
    // jeweilige Sektion, kein APVTS-Attachment noetig (siehe Processor-
    // Kommentar).
    void setupLockButton (juce::TextButton& button, int soloValue);
    // Gemeinsame Implementierung fuer Logo-Klick UND den BYP-Button (User-
    // Feedback: "Mache es einfach genau so wie wenn man auf das Space X
    // Logo klickt.") - toggelt processor.uiBypassed und stoesst zuverlaessig
    // ein content.repaint() an (fuer den Abdunkel-Schleier in
    // paintOverContent()).
    void toggleUiBypass();
    // Liefert alle APVTS-Parameter-IDs, die zu einer Sektion gehoeren -
    // genutzt von Chaos/Mutate (globalChaosButton.onClick), um gesperrte
    // Sektionen komplett von der Randomisierung auszunehmen.
    static juce::StringArray sectionParamIds (int soloSectionId);
    // Gemeinsame Umsetzung beider Mutate-Tasten. mayDisableSections
    // unterscheidet die beiden Varianten (siehe globalChaosSectionsButton).
    void runMutate (bool mayDisableSections);
    void setupModButton (juce::TextButton& button, const juce::String& paramId,
                          std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& attachment);
    void resetSoloIfMatches (int soloValue);
    void setupClickableTitle (juce::Label& label, const juce::String& powerParamId, int soloValue);
    // Bug-Fix (User-Feedback): Galaxy-Sektion einschalten, waehrend
    // "Activate Galaxy" global aus ist, hatte keine hoerbare Wirkung (die
    // STFT-Engine lief gar nicht). Aktiviert Activate Galaxy automatisch mit,
    // aber NUR beim Einschalten der Sektion - beim Ausschalten bleibt
    // Activate Galaxy bewusst an (User-Bestaetigung).
    void activateGalaxyIfNeeded();
    // Waehrend Mutate/Preset-Laden loesen programmatische Sektions-Schalter
    // KEIN Scharfschalten von Galaxy aus (User: "Kategorien aktiv + Galaxy
    // global off -> Galaxy wird aktiviert").
    bool suppressGalaxyAutoArm = false;
    // Workflow-Beschleunigung (User-Idee): dreht man am Tiefe-Regler einer
    // Mod-Sektion, waehrend deren Mod-Icon aus ist, schaltet das Icon
    // automatisch mit an - reagiert bewusst nur auf echte Wertaenderungen
    // (onValueChange), nicht auf reinen Klick/Fokus, damit ein versehent-
    // liches Antippen nichts ausloest.
    void wireModAutoEnable (juce::Slider& depthSlider, juce::TextButton& modButton, const juce::String& modParamId);
    static juce::Font sectionTitleFont();
    static juce::Font paramLabelFont();

    LCRMSAudioProcessor& processor;
    CustomLookAndFeel lookAndFeel;
    ContentComponent content { *this };

    // Unsichtbare Klickflaeche ueber dem Logo: schaltet einen rein
    // GUI-seitigen Bypass (siehe LCRMSAudioProcessor::uiBypassed) - kein
    // Host-Parameter, nur fuer schnelles A/B im Studio per Mausklick.
    juce::TextButton logoButton;

    // Globale Buttons oben rechts in der Titelzeile (User-Idee): Reset auf
    // Werkseinstellungen, A/B-Vergleich fuer den gesamten Plugin-Zustand,
    // globaler Mod-Bypass fuer alle 3 LFO-Modulationen gleichzeitig.
    juce::TextButton globalResetButton { "RESET" };
    // Text wird nicht mehr per setButtonText() getauscht - "A/B" steht immer
    // da, drawABContent() hebt nur den aktiven Buchstaben farblich hervor.
    juce::TextButton globalABButton;
    // Eigener Inhalt (Sinuswelle + "OFF") statt Text, siehe drawModBypassContent().
    juce::TextButton globalModBypassButton;
    // LIFE: globaler Regler neben dem Mod-Bypass, skaliert alle Tiefen.
    juce::Slider lifeSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lifeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> globalModBypassAttachment;

    // "BYP": neuer globaler Bypass-Button (User-Wunsch, sitzt links von
    // Chaos/Mutate) - schaltet denselben GUI-seitigen Bypass wie der
    // bestehende Logo-Klick (processor.uiBypassed), kein eigener APVTS-
    // Parameter/Attachment - Status wird per timerCallback() synchronisiert.
    juce::TextButton globalBypassButton { "BYP" };
    // A/B haelt zwei komplette Parameter-Snapshots rein GUI-seitig (nicht
    // Teil des gespeicherten Plugin-Zustands) - schneller Vergleich zweier
    // Einstellungen innerhalb einer Session, kein persistentes Preset.
    juce::ValueTree abSlotA, abSlotB;
    // "Copy" neben A/B (User-Wunsch, nach Pro-Q-Vorbild): kopiert den
    // aktuellen Zustand in den jeweils NICHT aktiven Buchstaben. Leuchtet nur,
    // solange sich beide Slots unterscheiden - ist A = B, gibt es nichts zu
    // kopieren, und das Icon erlischt. Der Pfeil zeigt die Kopierrichtung
    // (A aktiv -> nach rechts zu B, B aktiv -> nach links zu A).
    juce::TextButton abCopyButton;
    bool abCopyLit = false;
    static float signatureOfTree (const juce::ValueTree& tree);
    bool abCurrentIsA = true;

    // Globaler "ACTIVATE GALAXY"-Schalter (echter APVTS-Parameter) - schaltet
    // NUR die STFT-Engine/Latenz scharf, siehe ID_GALAXY_ACTIVATE. Bewusst
    // ein simpler Toggle mit der "globalBtn"-Optik (Text hell wenn an,
    // gedimmt wenn aus) - keine eigene Zeichenroutine noetig.
    juce::TextButton globalGalaxyActivateButton { "GALAXY" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> globalGalaxyActivateAttachment;

    // "BREATHE": reine Aktion (kein Zustand) - schaltet in allen 5 Mod-
    // Sektionen den Mod-Toggle an und wuerfelt die Tiefe-Regler neu (User-
    // Idee: "Leben in das Plugin einhauchen"). Kein Text mehr (User-Wunsch:
    // "Dann kann man sich auch das Wort Breathe sparen") - reines Icon,
    // siehe drawBreatheContent(). Der kleine Strich im Icon wechselt bei
    // jedem Klick seine Position (breatheStrokeState, 0..7, siehe onClick).
    juce::TextButton globalBreatheButton;

    // "SAVE": merkt sich die aktuelle Fenstergroesse App-weit (nicht Teil
    // des Plugin-/Preset-Zustands), damit das Plugin beim naechsten
    // Oeffnen in dieser Groesse startet.
    juce::TextButton globalSaveSizeButton { "SAVE" };

    // "LOAD" (User-Wunsch: "Load Button um gespeicherte Presets anzuzeigen.
    // Rechtsklick fuer Option zum Loeschen.") - Linksklick zeigt eine Liste
    // aller gespeicherten Presets zum Laden, Rechtsklick dieselbe Liste zum
    // Loeschen. Ersetzt fuer den Alltag die bisher einzige Route ueber die
    // "Load Preset"/"Delete Preset"-Untermenues im Hamburger-Menue (die
    // bleiben zusaetzlich bestehen).
    // (Der frueher hier deklarierte loadPresetButton ist entfallen - das
    //  Preset-Namensfeld hat seine Funktion uebernommen.)

    // "CHAOS": reine Aktion (kein Zustand) - wuerfelt WIRKLICH ALLE Regler
    // und Mods neu (User-Wunsch), ausgenommen Vol Trim, Mono Check/-Dry und
    // die echten globalen Schalter (Activate Galaxy, globaler Mod-Bypass) -
    // deren Zustand soll durch Chaos nicht veraendert werden. Solo wird
    // ebenfalls nicht angefasst (eigene Entscheidung, nicht Teil der
    // User-Vorgabe: ein zufaelliges Solo waere ein staerkerer Signal-
    // Wegfall als jede Regler-Randomisierung und keine Lautstaerke-
    // Absicherung koennte das sinnvoll abfedern). Siehe chaosTriggerRequested
    // im Processor fuer das Anti-Vol-Jump-Ducking.
    // Text "CHAOS" -> "MUTATE" (User-Wunsch) -> jetzt komplett entfernt
    // (User-Wunsch: "Namen entfernen"), Funktion unveraendert. Icon jetzt
    // ein 2x2-Raster aus 4 kleinen gefuellten Kaestchen in Lila/Tuerkis
    // (mutateColorState, 4 Bits, siehe drawMutateContent()/onClick).
    juce::TextButton globalChaosButton;
    // Zweite Mutate-Taste (User-Idee): dieselbe Funktion, aber sie darf
    // zusaetzlich Sektionen AUSSCHALTEN. Die normale Taste laesst alle
    // Sektionen an und wuerfelt nur Klangparameter. Das sind zwei ehrlich
    // verschiedene Absichten - "gib mir eine andere Farbe" gegenueber "gib
    // mir etwas ganz anderes". Im Icon durch zwei graue statt farbige
    // Kaestchen unterschieden (siehe CustomLookAndFeel::drawMutateContent).
    juce::TextButton globalChaosSectionsButton;

    // -- LCR (Galaxy) --
    juce::TextButton lcrPowerButton;
    juce::TextButton lcrSoloButton;
    // Section-Lock (User-Wunsch) - je ein Icon pro Sektion, siehe
    // setupLockButton().
    juce::TextButton lcrLockButton;
    juce::Label lcrTitleLabel;
    juce::Slider gravitySlider;
    juce::Label gravityLabel;
    juce::Slider orbitSlider; // Kegel-Visualisierung: unten Center, oben L+R ("Orbit")
    juce::Label orbitLabel;
    // HORIZON: obere Grenze des Extraktionsbandes (Bertoms LPF). Sitzt
    // zwischen Gravity ("wie streng") und Orbit ("wie viel"), weil es die
    // Frage "bis wohin" beantwortet.
    juce::Slider horizonSlider;
    juce::Label horizonLabel;
    juce::TextButton galaxyModButton;
    juce::Slider galaxyModDepthSlider;

    // -- Drift (Timewarp: Haas + Shift/Micro-Pitch) --
    juce::TextButton driftPowerButton;
    juce::TextButton driftSoloButton;
    juce::TextButton driftLockButton;
    juce::TextButton driftModButton;
    juce::Slider driftModDepthSlider;
    juce::Label driftTitleLabel;
    juce::Slider driftSlider;
    juce::Label driftLabel;
    // "Balance": kleines Icon direkt neben dem Drift-Label - automatische
    // Gain-Kompensation fuer den Haas-Praezedenzeffekt (User-Feedback,
    // siehe ID_TIMEWARP_BALANCE im Processor).
    juce::TextButton driftBalanceButton;
    // PARALLAX neu: ein Regler + vier Modus-Knoepfe (Runde 34).
    juce::Slider parallaxAmountSlider;
    juce::Label  parallaxAmountLabel;
    static constexpr int kPxModes = 6;   // Double, Wide, Illusion, 3D, Drift, Flux (Runde 47)
    juce::TextButton parallaxModeButtons[kPxModes];
    juce::Rectangle<int> pxModeDotsArea;   // SpaceXclick: Punkte unter dem Klick-Knopf
    juce::Rectangle<float> demoChipArea;   // DEMO-Plakette (leuchtet waehrend der Absenkung)
    // Runde 46: die Punkte sind klickbar - Klick auf einen Punkt waehlt den Modus.
    struct ModeDots : public juce::Component, public juce::SettableTooltipClient
    {
        int count = 5, index = 0;
        bool off = false;
        std::function<void (int)> onPick;
        // Feste Teilung statt "Breite / Anzahl" - sonst stehen die Punkte in
        // Parallax (6) und RAYE (4) unterschiedlich weit auseinander
        // (User Runde 49: "mache sie einheitlich").
        static constexpr float kPitch = 10.0f;
        float firstCentre() const { return ((float) getWidth() - kPitch * (float) count) * 0.5f + kPitch * 0.5f; }
        int dotAt (float x) const
        {
            return juce::jlimit (0, count - 1, (int) std::floor ((x - firstCentre()) / kPitch + 0.5f));
        }
        void paint (juce::Graphics& g) override
        {
            const float d = 4.0f;
            const float x0 = firstCentre();
            for (int i = 0; i < count; ++i)
            {
                const float cx = x0 + kPitch * (float) i;
                const float cy = (float) getHeight() * 0.5f;
                g.setColour (i == index ? themePalette().knob.withAlpha (off ? 0.35f : 1.0f)
                                        : juce::Colours::white.withAlpha (0.16f));
                g.fillEllipse (cx - d * 0.5f, cy - d * 0.5f, d, d);
            }
        }
        void mouseDown (const juce::MouseEvent& e) override { if (onPick) onPick (dotAt ((float) e.x)); }
    };
    ModeDots pxModeDots;
    ModeDots rayModeDots;   // SpaceXraye2: Punkte unter dem Charakter-Knopf
    // SpaceXraye: RAYE Amount (stufenlos) + Charakter-Klick-Knopf.
    juce::Slider rayAmountSlider;
    juce::Label  rayAmountLabel;
    juce::TextButton rayCharButton;
    // Runde 49: kleiner FAST-Knopf im RAYE-Kopf (+30 % Tempo) - Ersatz fuer
    // den entfallenen Speed-Regler.
    juce::TextButton rayFastButton { "FAST" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> rayFastAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rayAmountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> parallaxAmountAttachment;
    void applyParallaxMode();
    // Filter-Bypass je Sektion (siehe LookAndFeel "filterIcon").
    juce::TextButton galaxyFilterButton, dimFilterButton, posFilterButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> galaxyFilterAttachment, dimFilterAttachment, posFilterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> driftBalanceAttachment;
    juce::Slider bendSlider;
    juce::Label bendLabel;

    // -- Polarity (Flip) --
    juce::TextButton polPowerButton;
    juce::TextButton polSoloButton;
    juce::TextButton polLockButton;
    juce::Label polTitleLabel;
    // Kein "ø"-Praefix mehr noetig - "POLARITY" steht bereits im Titel.
    juce::TextButton polLButton { "L" };
    juce::TextButton polRButton { "R" };
    // Position des Polarity-Flips im Signalfluss, waehlbar ueber 4 kleine
    // Radio-Buttons: 1=nach LCR, 2=nach Haas, 3=nach Mid/Side (Default),
    // 4=nach Auto-Pan/Flow (ganz am Ende).
    juce::TextButton polPos1Button { "1" }, polPos2Button { "2" }, polPos3Button { "3" }, polPos4Button { "4" };
    // Kleiner Button ueber L/R: verbindet beide, schaltet sie gemeinsam
    // an/aus (User-Feedback).
    juce::TextButton polLinkButton;

    // -- Mid/Side (Dimension: Expand/Boost) --
    juce::TextButton widthBoostPowerButton;
    juce::TextButton widthBoostSoloButton;
    juce::TextButton widthBoostLockButton;
    juce::TextButton dimensionModButton;
    juce::Slider dimensionModDepthSlider;
    juce::Label widthBoostTitleLabel;
    juce::Slider sideWidthSlider, sideBoostSlider;
    juce::Label sideWidthLabel, sideBoostLabel;

    // -- Auto-Pan ("Hyperdrive") --
    juce::TextButton flowPowerButton;
    juce::TextButton flowSoloButton;
    juce::TextButton flowLockButton;
    juce::TextButton hyperdriveModButton;
    juce::Slider hyperdriveModDepthSlider;
    juce::Label flowTitleLabel;
    juce::Slider movementSlider;
    juce::Label movementLabel;
    juce::TextButton pulseButton { "Pulse" };
    juce::Slider speedRateSlider;
    juce::Label speedLabel;
    juce::TextButton syncButton { "Sync" };
    juce::ComboBox speedBox;

    // -- Position (Offset/Width/Distance/Elevate) --
    juce::TextButton posPowerButton;
    juce::TextButton posSoloButton;
    juce::TextButton posLockButton;
    juce::TextButton monoCheckButton;
    juce::Label monoCheckLabel;
    // Eigenes Label fuer das Dry-Icon (User-Feedback: die Footer-Zeile wirkte
    // amateurhaft) - vorher spannte EIN "MONO"-Label ueber Mono- UND Dry-Icon,
    // das Dry-Icon war dadurch unbeschriftet.
    juce::Label monoDryLabel;
    // A/B-Dry-Vergleich neben dem Mono-Icon: hoert das rohe Eingangssignal
    // (statt des bearbeiteten) in Mono - nur bedienbar, waehrend Mono-Check
    // selbst an ist (User-Feedback: "soll nicht alleine gehen").
    juce::TextButton monoDryButton;
    // Ganz simpler Trim-Regler (+-6dB, ohne Wertetext/Textbox), sitzt mittig
    // ueber dem Mono-Icon (gleiche Breite) und wirkt als allerletzte
    // Gain-Stufe der gesamten Signalkette (nach Mono-Check, "am ende").
    LockableSlider volSlider;
    juce::Label volLabel;
    // Globaler MIX (bearbeitet gegen Original), sitzt zwischen DRY und VOL.
    LockableSlider mixSlider;
    juce::Label mixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    // Hover-Hinweise (Menue "Show Hover Hints"): das Tooltip-Fenster
    // existiert nur, solange die Option an ist.
    // Eigenes Tooltip-Fenster: sucht den Hinweis auch bei den ELTERN der
    // Komponente unter der Maus (Bug "Starfield kein Hint": liegt die Maus
    // ueber einem Kind ohne eigenen Hinweis, gilt der des Sternenfelds).
    struct HintWindow : public juce::TooltipWindow
    {
        HintWindow (juce::Component* parent, int ms) : juce::TooltipWindow (parent, ms) {}
        juce::String getTipFor (juce::Component& c) override
        {
            for (auto* comp = &c; comp != nullptr; comp = comp->getParentComponent())
                if (auto* client = dynamic_cast<juce::TooltipClient*> (comp))
                    if (auto tip = client->getTooltip(); tip.isNotEmpty())
                        return tip;
            return {};
        }
    };
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    void applyHoverHints();
    void applyLayoutMode();
    // Hinweiszeile unter dem Footer: liest den Tooltip des Elements unter der
    // Maus und zeigt ihn dort an (statt als Kaestchen an der Maus, User).
    void updateHintBar();
    void drawHintBar (juce::Graphics& g);
    juce::TextButton helpButton;            // dezentes "?" ganz unten links
    juce::TextButton autoGainButton;        // unsichtbare Klickflaeche ueber der AG-Anzeige
    juce::Rectangle<int> hintBarArea;       // Textbereich rechts daneben
    juce::String currentHint;               // was gerade angezeigt wird
    // Klick ins Sternenfeld: Goniometer-Farbe weiterschalten, 5. Klick = aus.
    void cycleGonioColourFromField (int action = 0);

    // ===== THEME (Modern / Watercolor / Comic), siehe UiTheme im LookAndFeel =====
    int uiThemeIndex = 0;
    juce::Image themePlate;        // gebackene Platten-Textur (Wasserfarbe/Comic), einmal erzeugt
    juce::Image grainTile;         // Korn-Kachel fuer Sektionsflaechen (Watercolor)
    int themePlateFor = -1;
    void setUiTheme (int theme, bool persist);
    static int gonioColourForTheme (int theme);   // Gonio-Standardfarbe je Theme (User)
    void drawThemePlate (juce::Graphics& g, juce::Rectangle<float> plate, float corner);
    static juce::Path wobblyRoundedRect (juce::Rectangle<float> r, float corner, float amp, int seed);
    // ===== Mutate-Kategorie (Chips ueber dem Sternenfeld) =====
    // 0 = keine, 1..6 = Drums, Vocals, Backings, Plucked, Keys, Pads.
    static constexpr int kNumCategories = 4;   // Vocal, Backing, Adlib, FX
    juce::TextButton categoryBtn[kNumCategories];   // alte Chips - nicht mehr im Layout
    // Runde 55: die Kategorie wird wie ein Modus gewaehlt - eine Pille mit
    // Punkten darunter, dieselbe Bildsprache wie Parallax und RAYE. Punkt 0
    // ist "aus", danach die vier Profile. Darunter (ueber dem Sternenfeld)
    // sagt eine Zeile, was das gewaehlte Profil tut.
    juce::TextButton categoryButton;
    ModeDots         catDots;
    juce::Label      smartInfoLabel;
    static juce::String smartInfoTextFor (int cat);
    int mutateCategoryValue = 0;
    bool showMutateCategories = true;   // Menue "Show Mutate Categories"
    // Settings "Technical Labels" (User): Sektions- und Reglernamen als
    // Funktion statt als Vibe-Name.
    bool technicalLabels = false;
    void applyLabelStyle();
    int  mutateCategory() const;
    void setMutateCategory (int cat);
    void applyMutateProfile (juce::Random& rng, int category);
    // Kleine Input/Output-Pegelanzeige links/rechts vom Volume-Regler
    // (User-Wunsch: "Links und rechts von Volume ein kleines Input und
    // Output Meter Pegelanzeige") - reine Anzeige, siehe
    // PluginProcessor::currentInputLevel/currentOutputLevel.
    LevelMeterComponent volInputMeter, volOutputMeter;
    juce::Label inputMeterLabel, outputMeterLabel;
    juce::Label posTitleLabel;
    juce::TextButton positionModButton;
    juce::Slider positionModDepthSlider;
    juce::Slider offsetSlider, posWidthSlider, distanceSlider, elevateSlider;
    juce::Label offsetLabel, posWidthLabel, distanceLabel, elevateLabel;

    // -- RAY (Stereo-Phaser, neue Sektion unten rechts) --
    // Bewusst die knappste Sektion im Plugin: ein 3-Stufen-Icon fuer die
    // Staerke, ein Speed-Regler, ein Pair-Schalter, der den Speed-Regler an
    // Hyperdrive abgibt. Kein Mod-Icon - das Staerke-Icon ist die Tiefe.
    juce::TextButton rayPowerButton;
    juce::TextButton raySoloButton;
    juce::TextButton rayLockButton;
    juce::Label rayTitleLabel;
    juce::TextButton rayStrengthButton;   // 3-Klick-Icon (leicht/mittel/stark)
    juce::Slider rayRateSlider;
    juce::Label rayRateLabel;
    juce::TextButton rayPairButton { "PAIR" };   // gross wie FAST (User)
    juce::Rectangle<int> groupRayArea;
    bool rayFrameOn = true;
    // Goldene Klammer um Speed/Sync/Bars in Hyperdrive, solange Pair aktiv
    // ist (gezeichnet in paintContent()).
    bool pairGoldFrameOn = false;
    bool pairGoldSync = false;
    int lastPolPosShown = -1;
    int eggOpenTicks = 0;
    bool eggFiredOnOpen = false;
    // Voll qualifiziert, weil die Kurz-Aliase (ButtonAttachment/
    // SliderAttachment) erst weiter unten im Header deklariert werden.
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> rayOnAttachment, rayPairAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rayRateAttachment;

    // Goniometer + Korrelationsmesser
    GoniometerComponent goniometer;
    CorrelationMeterComponent correlationMeter;

    // Goniometer/Star-Visuals An/Aus (User-Wunsch, 2. Anlauf: "Gonio und
    // Stars entfernen in der global Leiste. Dafuer im Menu einfach je einen
    // Eintrag hinzufuegen.") - die beiden eigenen Buttons in der globalen
    // Zeile sind komplett entfallen, das Hamburger-Menue ist jetzt die
    // EINZIGE Kontrolle und wirkt sofort live (nicht mehr nur als Start-
    // Standard fuers naechste Oeffnen), siehe showPresetMenu()/
    // applyVisualsVisibility(). Reiner GUI-Session-Zustand (kein APVTS-
    // Parameter) - da kein Button mehr existiert, der ihn haelt, stecken
    // die beiden Flags jetzt direkt hier.
    bool goniometerVisualsOn = true;
    bool starVisualsOn = true;
    // Zwei Starfield-Darstellungsoptionen aus dem Hamburger-Menue (siehe
    // showPresetMenu()). Reine Anzeige-Einstellungen, kein Audio-Einfluss.
    bool starfieldModMovementOn = true;
    bool starfieldReducedAnimations = false;

    // PRISM (frequenzselektive Verbreiterung) - sitzt in der rechten
    // Footer-Haelfte, siehe layoutContent().
    juce::TextButton prismOnButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> prismOnAttachment;
    PrismBandComponent prismBand { processor.apvts };
    juce::Label prismLabel;
    float lastPrismLo = -1.0f, lastPrismHi = -1.0f;

    // Undo/Redo (User-Wunsch: "Je ein Pfeil Icon, Position rechts von
    // Reset") - einfacher Snapshot-Stack auf Basis des kompletten APVTS-
    // State-Baums (deckt automatisch auch Section-Lock-Aenderungen ab, da
    // beide auf demselben Baum liegen). Aenderungen werden ueber den
    // ohnehin laufenden 20Hz-GUI-Timer entprellt (siehe timerCallback()) -
    // ein Regler-Drag erzeugt so EINEN Undo-Schritt statt hunderter
    // Einzelschritte pro Bewegung.
    juce::TextButton undoButton, redoButton;
    juce::Array<juce::MemoryBlock>& undoHistory;   // liegt im Processor (ueberlebt Schliessen des Fensters, User)
    int& undoIndex;
    int undoDebounceFramesLeft = 0;
    bool undoRedoInProgress = false;
    void pushUndoSnapshotNow();
    void scheduleUndoSnapshot();
    void performUndo();
    void performRedo();
    void updateUndoRedoButtonStates();
    struct StateChangeListener : public juce::ValueTree::Listener
    {
        explicit StateChangeListener (LCRMSAudioProcessorEditor& o) : owner (o) {}
        void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override { owner.scheduleUndoSnapshot(); }
        void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override { owner.scheduleUndoSnapshot(); }
        void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override { owner.scheduleUndoSnapshot(); }
        LCRMSAudioProcessorEditor& owner;
    };
    std::unique_ptr<StateChangeListener> stateChangeListener;

    // Preset-/Hamburger-Menue (User-Idee): buendelt App-weite (nicht
    // projektgebundene) Standardeinstellungen fuer neu geoeffnete Plugin-
    // Instanzen - persistiert ueber LCRMSAudioProcessor::appPropertiesOptions().
    // Sitzt rechts vom globalen Mod-Bypass-Button, aber deutlich getrennt
    // (groesserer Gap, siehe layoutContent()).
    juce::TextButton presetMenuButton;
    void showPresetMenu();

    // Preset-System (User-Wunsch: "Save hat keine Funktion ... Kannst du
    // ein Preset Menu einbauen?") - benannte Presets werden App-weit ueber
    // PropertiesFile persistiert (Namensliste unter "presetNames", je ein
    // XML-Snapshot des kompletten APVTS-Zustands unter "presetXml_<Name>").
    // SAVE oeffnet jetzt einen Namens-Dialog statt nur die Fenstergroesse zu
    // merken; Laden/Loeschen sitzen als Untermenues im Hamburger-Menue.
    juce::StringArray getPresetNames() const;
    // Dateibasiertes Preset-System (siehe PluginEditor.cpp, "PRESET-SYSTEM").
    juce::File presetFolder() const;
    juce::File presetFile (const juce::String& name) const;
    void migrateLegacyPresets();
    static bool isDefaultPresetName (const juce::String& name);
    juce::ValueTree defaultPresetTree() const;
    void saveCurrentStateAsDefault();
    void openManual();
    juce::ValueTree presetTree (const juce::String& name) const;
    void promptRenamePreset();
    // Seriennummer eingeben (Settings -> "Activate..."). Siehe Source/Licence.h.
    void promptActivate();
    // Save-/Rename-/Activate-Dialoge in die Theme-Farben bringen - ein
    // AlertWindow zieht sonst das JUCE-Standardgrau (User).
    void styleNameDialog (juce::AlertWindow& w);
    // Ordner-Auswahl ("Set Preset Folder...") muss waehrend des Dialogs leben.
    std::unique_ptr<juce::FileChooser> presetFolderChooser;
    // prefillCurrent = true: Name des geladenen Presets vorausgefuellt
    // (Disketten-Icon = "dieses Preset sichern"); false: leeres Feld
    // ("Save as..." aus der Liste = "unter neuem Namen sichern").
    void promptAndSaveNewPreset (bool prefillCurrent = true);
    // Schreibt den aktuellen Zustand unter diesem Namen (ohne Rueckfrage) -
    // wird sowohl direkt als auch aus der Ueberschreib-Warnung aufgerufen.
    void writePreset (const juce::String& name);
    void loadPreset (const juce::String& name);
    void deletePreset (const juce::String& name);
    void showLoadPresetPopup (bool deleteMode);
    // Ein Preset vor/zurueck (kleine Pfeile neben dem Preset-Menue).
    void stepPreset (int direction);
    // Zuletzt geladenes bzw. gespeichertes Preset. Zwei Aufgaben:
    // (1) SAVE schlaegt diesen Namen wieder vor (User-Wunsch: "wenn Preset
    //     ausgewaehlt war und man save drueckt soll der alte Name noch da
    //     stehen"), (2) die L/R-Pfeile wissen dadurch, wo sie stehen.
    juce::String currentPresetName;
    // Kleine Schrittpfeile links/rechts vom Preset-Namen (User-Wunsch:
    // "kleine Preset Arrows L R") - zum Durchhoeren ohne Menue.
    juce::TextButton presetPrevButton, presetNextButton;
    // Das Namensfeld selbst. Bewusst ein BUTTON und kein Label: ein Klick
    // darauf oeffnet die Preset-Liste. Der Name ist damit gleichzeitig
    // Anzeige und der kuerzeste Weg zum Wechseln - genau das erwartet man
    // von einem Preset-Feld.
    juce::TextButton presetNameButton;
    // Papierkorb (User-Idee) - loescht das GELADENE Preset nach Rueckfrage.
    // Ohne geladenes Preset ist er inaktiv.
    juce::TextButton presetDeleteButton;
    // Ist seit dem Laden/Speichern irgendein Parameter veraendert worden?
    // Dann bekommt der Name einen Stern. Ohne diese Anzeige weiss man nie,
    // ob der gezeigte Name noch das ist, was man gerade hoert.
    bool presetDirty = false;
    float presetSignature = 0.0f;
    int presetDirtyTick = 0;
    float computePresetSignature() const;
    void refreshPresetNameDisplay();
    // Zustand der Galaxy-Sektion im Moment des globalen Abschaltens, damit
    // sie beim Wiedereinschalten zurueckkommt (siehe globalGalaxyActivateButton).
    bool galaxySectionWasOnBeforeDeactivate = true;
    // Trennstriche der zweiten Titelzeile (Preset-Zeile), analog zu
    // globalRowSeparatorX fuer die erste.
    juce::Array<int> presetRowSeparatorX;
    int presetRowSeparatorTop = 0, presetRowSeparatorBottom = 0;
    // Haelt den Namens-Dialog am Leben, solange er offen ist (AlertWindow
    // braucht ein Objekt mit Lebensdauer >= Dialogdauer fuer den async
    // Callback-Modus mit Texteingabe).
    std::unique_ptr<juce::AlertWindow> presetNameDialog;
    float lastDemoDuck = 1.0f;   // siehe timerCallback / paintOverContent
    float lastAutoGainDb = 0.0f; // Anzeige im Footer, siehe timerCallback
    juce::Rectangle<int> autoGainReadoutArea;   // in layoutContent gesetzt, in paintContent gezeichnet

    // Menu-Item "Show Modulation" (User-Wunsch: "add show modulation
    // visuals / feedback / movement") - schaltet die beweglichen Live-Mod-
    // Anzeigen (Punkte/Linien auf Gravity, Orbit, Drift usw., siehe
    // timerCallback()'s applyLive()-Block) sofort fuer die laufende Session
    // an/aus. Persistiert als Standard fuers naechste Oeffnen, wirkt aber
    // (anders als Goniometer/Star-Visuals-Menue-Eintraege) nicht zusaetzlich
    // ueber einen eigenen Live-Button, da es rein dekorativ ist und selten
    // umgeschaltet werden duerfte.
    bool modulationVisualsEnabled = true;
    // "Show Advanced Modulation" (Runde 37): Mod-Icons + Tiefe je Sektion.
    bool advancedModVisible = false;
    void applyAdvancedModVisibility();
    // Menue-Option, siehe showPresetMenu(): Solo bleibt stehen, auch wenn die
    // solierte Sektion ausgeschaltet wird.
    bool keepSoloWhenSectionOff = true;

    // Fasst goniometer.setGoniometerActive()/setSpaceVisualsEnabled() an
    // einer Stelle zusammen - beide haengen von den beiden Hamburger-Menue-
    // Schaltern (Goniometer/Star-Visuals) ab, die jetzt SOFORT wirken
    // (nicht mehr nur "Standard beim naechsten Start"), siehe showPresetMenu().
    void applyVisualsVisibility();
    // View-Panel (Zahnrad im Sternenfeld) - siehe ViewPanelComponent.
    juce::TextButton viewGearButton;
    ViewPanelComponent viewPanel;
    BackdropComponent settingsBackdrop;
    // Weichgezeichnete Momentaufnahme der Oberflaeche hinter dem Panel. Einmal
    // beim Oeffnen erzeugt - ein Blur pro Frame waere zu teuer.
    juce::Image settingsBlur;
    SettingsPanelComponent settingsPanel;
    BackPanelComponent     backPanel;
    TourOverlay            tourOverlay;
    void applyViewSettings (bool persist);
    // ===== View-Einstellungen im Plugin-Zustand =====
    // Die Panel-Werte leben zusaetzlich als Kind "ViewSettings" im APVTS-
    // Baum (kein Parameter, nur Daten): so merkt sich die DAW-Session sie
    // pro Instanz, und Presets koennen sie optional mitnehmen (Menue
    // "Save View Settings with Preset"). Der Standard fuers naechste Oeffnen
    // liegt weiterhin in den Properties ("Make Default" im Panel).
    juce::ValueTree viewSettingsTree() const;
    void storeViewSettingsInState();
    bool applyViewSettingsFromTree (const juce::ValueTree& parent);   // false = kein Kind vorhanden
    void saveViewSettingsToPreset();
    static bool isMixLocked();
    static bool isVolLocked();
    void toggleKnobLock (const char* propName, juce::Slider& knob);   // Menue "Lock Mix Knob"

    // Kleine vertikale Trennstriche in der globalen Button-Zeile (User-
    // Wunsch: neue Reihenfolge mit Gruppen-Trennern) - x-Positionen und
    // Hoehe werden in layoutContent() berechnet, die eigentliche Linie wird
    // in paintContent() gezeichnet.
    juce::Array<int> globalRowSeparatorX;
    int globalRowSeparatorTop = 0, globalRowSeparatorBottom = 0;

    // Dezente Gruppierungs-Rahmen (nur optisch, in layoutContent() berechnet
    // und in paintContent() gezeichnet) plus deren aktueller On/Off-Status
    // (fuer das Ausgrauen der gesamten Box, nicht nur der einzelnen Regler).
    juce::Rectangle<int> groupLcrArea, groupDriftArea, groupPolArea, groupWidthBoostArea, groupFlowArea, groupPosArea;
    bool lcrFrameOn = true, driftFrameOn = true, polFrameOn = true, widthBoostFrameOn = true, flowFrameOn = true, posFrameOn = true;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttachment> lcrAttachment;
    std::unique_ptr<SliderAttachment> gravityAttachment, focusAttachment, horizonAttachment;
    std::unique_ptr<SliderAttachment> driftAttachment, bendAttachment;
    std::unique_ptr<ButtonAttachment> polLAttachment, polRAttachment;
    std::unique_ptr<SliderAttachment> sideWidthAttachment, sideBoostAttachment;
    std::unique_ptr<SliderAttachment> movementAttachment;
    std::unique_ptr<ButtonAttachment> pulseAttachment;
    std::unique_ptr<SliderAttachment> speedRateAttachment;
    std::unique_ptr<ButtonAttachment> syncAttachment;
    std::unique_ptr<ComboAttachment> speedAttachment;

    std::unique_ptr<ButtonAttachment> driftOnAttachment, polOnAttachment, widthBoostOnAttachment, flowOnAttachment;
    std::unique_ptr<ButtonAttachment> timewarpModAttachment, dimensionModAttachment, hyperdriveModAttachment;
    std::unique_ptr<SliderAttachment> timewarpDepthAttachment, dimensionDepthAttachment, hyperdriveDepthAttachment;
    std::unique_ptr<ButtonAttachment> galaxyModAttachment, positionModAttachment;
    std::unique_ptr<SliderAttachment> galaxyDepthAttachment, positionDepthAttachment;

    std::unique_ptr<ButtonAttachment> posOnAttachment, monoCheckAttachment, monoDryAttachment;
    std::unique_ptr<SliderAttachment> offsetAttachment, posWidthAttachment, distanceAttachment, elevateAttachment;
    std::unique_ptr<SliderAttachment> volAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LCRMSAudioProcessorEditor)
};
