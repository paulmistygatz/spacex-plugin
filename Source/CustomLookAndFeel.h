#pragma once
#include <JuceHeader.h>

// Modernes, reduziertes LookAndFeel: dunkler Hintergrund, flache Regler,
// leuchtende Akzente (angelehnt an Nuro Audio). Bewusst simpel gehalten,
// low CPU beim Zeichnen.
class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        setColour (juce::Slider::rotarySliderFillColourId, accent);
        setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff2a2d33));
        setColour (juce::Slider::thumbColourId, juce::Colours::white);
        setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::trackColourId, juce::Colour (0xff2a2d33));
        setColour (juce::Slider::backgroundColourId, juce::Colour (0xff2a2d33));
        setColour (juce::Label::textColourId, juce::Colour (0xffcfd3da));
        setColour (juce::ToggleButton::tickColourId, accent);
        setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff444750));
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff20232a));
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff33363f));
        setColour (juce::ComboBox::textColourId, juce::Colours::white);
        setColour (juce::TextButton::buttonColourId, juce::Colour (0xff20232a));
        setColour (juce::TextButton::buttonOnColourId, glowAccent);
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xffcfd3da));
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    }

    // Rotary-Regler. Fuer Regler, deren Default-Wert exakt in der Mitte
    // ihres Bereichs liegt (12-Uhr-Position = neutral: Gravity, Drift,
    // Width), wird ueber die Component-Property "centerOut" ein anderes
    // Fuellverhalten aktiviert: der Ring beginnt bei 12 Uhr leer und
    // waechst je nach Drehrichtung nach links oder rechts, statt immer
    // vom Reglerstart an gefuellt zu sein.
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                            float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                            juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto centre = bounds.getCentre();
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Regler bleiben bei ausgeschalteter Section weiterhin bedienbar
        // (siehe PluginEditor::timerCallback), sollen dann aber NICHT mehr
        // farbig/leuchtend wirken - die Component-Property "sectionOff"
        // erzwingt hier die neutrale/graue Darstellung, unabhaengig vom
        // technischen isEnabled()-Status.
        const bool offVisual = slider.getProperties().getWithDefault ("sectionOff", false) || ! slider.isEnabled();

        float trackThickness = radius * 0.18f;

        juce::Path track;
        track.addCentredArc (centre.x, centre.y, radius - trackThickness, radius - trackThickness,
                              0.0f, rotaryStartAngle, rotaryEndAngle, true);
        // Etwas heller als die getönten Gruppenrahmen dahinter, damit der
        // unlackierte Ring nicht optisch verschwindet.
        g.setColour (juce::Colour (0xff454952));
        g.strokePath (track, juce::PathStrokeType (trackThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // "Movement"-Ring: statt eines normalen Zeigers waechst der Ring
        // symmetrisch von 12 Uhr aus nach links UND rechts, je nach Movement-
        // Wert (0% = Punkt oben, 100% = voller Schwenkbereich). Zusaetzlich
        // zeigt ein leuchtender Punkt die aktuelle Auto-Pan-Position in
        // Echtzeit (beeinflusst von Speed/Sync/Pulse) - Component-Property
        // "movementRing" aktiviert diesen Modus, "movementLivePos" (-1..1)
        // liefert die Live-Position.
        const bool movementRing = slider.getProperties().getWithDefault ("movementRing", false);
        if (movementRing)
        {
            const float midAngle = (rotaryStartAngle + rotaryEndAngle) * 0.5f;
            const float halfSweep = (rotaryEndAngle - rotaryStartAngle) * 0.5f;
            const float t = juce::jlimit (0.0f, 1.0f, sliderPos);

            if (t > 0.001f)
            {
                juce::Path ring;
                ring.addCentredArc (centre.x, centre.y, radius - trackThickness, radius - trackThickness,
                                     0.0f, midAngle, midAngle + t * halfSweep, true);
                ring.addCentredArc (centre.x, centre.y, radius - trackThickness, radius - trackThickness,
                                     0.0f, midAngle - t * halfSweep, midAngle, true);
                auto ringCol = offVisual ? juce::Colour (0xff555861) : accent;
                g.setColour (ringCol);
                g.strokePath (ring, juce::PathStrokeType (trackThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            g.setColour (juce::Colour (0xff1c1e23));
            g.fillEllipse (centre.x - radius * 0.28f, centre.y - radius * 0.28f, radius * 0.56f, radius * 0.56f);

            if (! offVisual)
            {
                const float livePos = juce::jlimit (-1.0f, 1.0f, (float) slider.getProperties().getWithDefault ("movementLivePos", 0.0f));
                const float dotAngle = midAngle + livePos * halfSweep;
                const float dotRadius = radius - trackThickness;
                const float dx = centre.x + dotRadius * std::sin (dotAngle);
                const float dy = centre.y - dotRadius * std::cos (dotAngle);
                g.setColour (glowAccent.withAlpha (0.30f));
                g.fillEllipse (dx - 6.0f, dy - 6.0f, 12.0f, 12.0f);
                g.setColour (glowAccent);
                g.fillEllipse (dx - 3.0f, dy - 3.0f, 6.0f, 6.0f);
            }
            return;
        }

        const bool centerOut = slider.getProperties().getWithDefault ("centerOut", false);

        juce::Path value;
        if (centerOut)
        {
            const float midAngle = (rotaryStartAngle + rotaryEndAngle) * 0.5f;
            const float fillStart = juce::jmin (midAngle, angle);
            const float fillEnd   = juce::jmax (midAngle, angle);
            if (fillEnd - fillStart > 0.001f)
                value.addCentredArc (centre.x, centre.y, radius - trackThickness, radius - trackThickness,
                                      0.0f, fillStart, fillEnd, true);
        }
        else
        {
            value.addCentredArc (centre.x, centre.y, radius - trackThickness, radius - trackThickness,
                                  0.0f, rotaryStartAngle, angle, true);
        }
        auto col = offVisual ? juce::Colour (0xff555861) : accent;
        g.setColour (col);
        g.strokePath (value, juce::PathStrokeType (trackThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        float pointerLength = radius * 0.55f;
        juce::Path pointer;
        pointer.startNewSubPath (centre.x, centre.y);
        pointer.lineTo (centre.x + pointerLength * std::sin (angle), centre.y - pointerLength * std::cos (angle));
        g.setColour (juce::Colours::white.withAlpha (offVisual ? 0.4f : 1.0f));
        g.strokePath (pointer, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour (juce::Colour (0xff1c1e23));
        g.fillEllipse (centre.x - radius * 0.28f, centre.y - radius * 0.28f, radius * 0.56f, radius * 0.56f);

        // Live-Mod-Anzeige (Drift/Shift/Expand/Boost/Speed): zusaetzlich zum
        // normalen Zeiger (der die EINGESTELLTE Position zeigt) ein
        // leuchtender Punkt, der die tatsaechlich gerade MODULIERTE Position
        // live zeigt - gleicher visueller Stil wie der Live-Punkt beim
        // Flow-Regler (movementRing). Component-Properties "modLiveActive"
        // (bool) + "modLiveValue" (0..1, normalisierte Reglerposition).
        if (! offVisual && slider.getProperties().getWithDefault ("modLiveActive", false))
        {
            const float liveT = juce::jlimit (0.0f, 1.0f, (float) slider.getProperties().getWithDefault ("modLiveValue", 0.0f));
            const float liveAngle = rotaryStartAngle + liveT * (rotaryEndAngle - rotaryStartAngle);
            const float dotRadius = radius - trackThickness;
            const float dx = centre.x + dotRadius * std::sin (liveAngle);
            const float dy = centre.y - dotRadius * std::cos (liveAngle);
            g.setColour (glowAccent.withAlpha (0.30f));
            g.fillEllipse (dx - 6.0f, dy - 6.0f, 12.0f, 12.0f);
            g.setColour (glowAccent);
            g.fillEllipse (dx - 3.0f, dy - 3.0f, 6.0f, 6.0f);
        }
    }

    // Leuchtender Pillen-Button fuer LCR/Sync-Toggle und Polarity-Icons:
    // kein klassisches Kaestchen/Haekchen mehr, stattdessen ein Icon-Button,
    // der im aktiven Zustand sanft glimmt (mehrere ueberlagerte,
    // transparente Kreise simulieren einen Glow ohne teuren Blur).
    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                const juce::Colour&, bool shouldDrawButtonAsHighlighted,
                                bool shouldDrawButtonAsDown) override
    {
        // Unsichtbare Klickflaeche (z.B. das Logo) - zeichnet nichts.
        if (button.getProperties().getWithDefault ("invisibleHit", false))
            return;

        // Minimalistischer Ein/Aus-Icon-Button (Power-Symbol), fuer die
        // Section-Bypass-Schalter oben links in jeder Regler-Gruppe.
        if (button.getProperties().getWithDefault ("powerIcon", false))
        {
            drawPowerIcon (g, button);
            return;
        }

        // Kleine Buchstaben-Icons (S = Solo, M = Mono-Check), gleicher
        // reduzierter Stil wie das Power-Icon: nur Ring + Buchstabe leuchten,
        // kein Hintergrund-Kaestchen.
        if (button.getProperties().getWithDefault ("soloIcon", false))
        {
            drawLetterIcon (g, button, "S", juce::Colour (0xffffb648));
            return;
        }
        if (button.getProperties().getWithDefault ("monoIcon", false))
        {
            drawMonoIcon (g, button);
            return;
        }
        // Section-Lock-Icon (User-Wunsch: "Sections ausschliessen" von
        // Chaos/Mutate und Breathe) - kleines Schloss-Symbol im Section-
        // Header, offen/gedimmt wenn unlocked, geschlossen und farbig wenn
        // gesperrt.
        if (button.getProperties().getWithDefault ("lockIcon", false))
        {
            drawLockIcon (g, button);
            return;
        }
        // Undo/Redo-Pfeil-Icon (User-Wunsch) - kein Hintergrund-Kaestchen,
        // reiner Pfeil, siehe drawArrowIcon().
        if (button.getProperties().getWithDefault ("arrowIcon", false))
        {
            drawArrowIcon (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
        }
        // Bypass-Icon fuer den Mono-Dry-A/B-Vergleich (Kreis mit Diagonale,
        // international als "Bypass"/"Aus" erkennbar) - sitzt neben dem
        // Mono-Icon, nur bedienbar/aktivierbar waehrend Mono-Check an ist.
        if (button.getProperties().getWithDefault ("bypassIcon", false))
        {
            drawBypassIcon (g, button);
            return;
        }
        // Link-Icon (kleiner Button ueber L/R): zwei geschwungene Linien, die
        // symbolisch beide Buttons verbinden - bewusst NIE farbig (User-
        // Feedback: "soll nie farbig sein, das lenkt ab"), reagiert nur auf
        // sectionOff (Polarity aus/Bypass) und jetzt zusaetzlich auf Hover/
        // Down, damit trotzdem erkennbar ist, dass es ein klickbarer Button
        // ist (User-Feedback: "so dass man bei Mouse hover merkt, dass es
        // ein Button ist... wie die anderen Buttons auch, nur halt grau").
        if (button.getProperties().getWithDefault ("linkIcon", false))
        {
            drawLinkIcon (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
        }
        // Mod-Icons (Timewarp/Dimension/Hyperdrive): einfache Sinuswelle,
        // an/aus wie Solo/Power - keine eigene Tiefe/Depth einstellbar
        // (User-Feedback: "Ein Icon reicht aus. Vielleicht eine Sinuswelle.
        // Das versteht jeder.").
        if (button.getProperties().getWithDefault ("modIcon", false))
        {
            drawModIcon (g, button);
            return;
        }
        // "Balance"-Icon neben Drift: zwei nach innen zeigende Pfeile
        // (symbolisiert "zur Mitte zurueckholen"), siehe drawBalanceIcon.
        if (button.getProperties().getWithDefault ("balanceIcon", false))
        {
            drawBalanceIcon (g, button);
            return;
        }
        // Galaxy-Button bleibt bewusst auffaelliger (siehe
        // drawGalaxyButtonBackground) - MUSS vor der generischen globalBtn-
        // Pruefung unten kommen, da er zusaetzlich "globalBtn" gesetzt hat
        // (fuer die Schriftgroesse in drawButtonText).
        if (button.getProperties().getWithDefault ("galaxyBtn", false))
        {
            drawGalaxyButtonBackground (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
        }

        // Bug-Fix: RESET/A-B/globaler Mod-Bypass fielen bisher durch zum
        // generischen Pillen-Hintergrund weiter unten - der reagiert auf
        // Mouse-Hover mit einem sichtbar aufgehellten Hintergrund (User-
        // Feedback: "Global Mod Off bug -> mouse hover veraendert
        // FarbHelligkeit"). Diese sind reine Text/Icon-Buttons ohne eigene
        // Hintergrundflaeche (siehe drawButtonText/drawABContent/
        // drawModBypassContent/drawBypassToggleContent/drawMutateContent/
        // drawBreatheContent) und sollen daher hier komplett transparent
        // bleiben - den einheitlichen Hover-Effekt fuer die ganze globale
        // Zeile (User-Wunsch: "Ueberall Mouse Hover Effekte") zeichnet
        // drawButtonText() zuletzt, ueber dem jeweiligen Inhalt. Bypass/
        // Mutate/Breathe bekommen jetzt bewusst DIESELBE schlichte Icon+Text-
        // Optik wie der globale Mod-Bypass (User-Feedback: "Mod On/Off sieht
        // sehr gut aus ... jeder der 3 Buttons sieht anders aus" - jetzt
        // einheitlich).
        if (button.getProperties().getWithDefault ("globalBtn", false)
            || button.getProperties().getWithDefault ("abIcon", false)
            || button.getProperties().getWithDefault ("modBypassIcon", false)
            || button.getProperties().getWithDefault ("bypassToggleIcon", false)
            || button.getProperties().getWithDefault ("mutateIcon", false)
            || button.getProperties().getWithDefault ("breatheIcon", false))
            return;

        auto bounds = button.getLocalBounds().toFloat().reduced (2.0f);
        // Bei ausgeschalteter Section wird der Button unabhaengig vom
        // technischen Toggle-Zustand als "aus" GEZEICHNET (bleibt aber ganz
        // normal klickbar) - siehe drawRotarySlider fuer denselben Ansatz.
        const bool isOn = button.getToggleState() && ! button.getProperties().getWithDefault ("sectionOff", false);
        const float cornerSize = bounds.getHeight() * 0.5f;

        if (isOn)
        {
            auto centre = bounds.getCentre();
            float maxR = bounds.getWidth() * 0.75f;
            for (int layer = 4; layer >= 1; --layer)
            {
                float r = maxR * ((float) layer / 4.0f);
                g.setColour (glowAccent.withAlpha (0.05f * (float) (5 - layer)));
                g.fillEllipse (centre.x - r, centre.y - r * 0.7f, r * 2.0f, r * 1.4f);
            }
        }

        juce::Colour base = isOn ? juce::Colour (0xff2a1f33) : juce::Colour (0xff1c1e23);
        if (shouldDrawButtonAsDown) base = base.brighter (0.1f);
        else if (shouldDrawButtonAsHighlighted) base = base.brighter (0.05f);

        g.setColour (base);
        g.fillRoundedRectangle (bounds, cornerSize);

        g.setColour (isOn ? glowAccent : juce::Colour (0xff3a3d45));
        g.drawRoundedRectangle (bounds, cornerSize, isOn ? 1.6f : 1.0f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        if (button.getProperties().getWithDefault ("powerIcon", false)
            || button.getProperties().getWithDefault ("invisibleHit", false)
            || button.getProperties().getWithDefault ("soloIcon", false)
            || button.getProperties().getWithDefault ("monoIcon", false)
            || button.getProperties().getWithDefault ("modIcon", false)
            || button.getProperties().getWithDefault ("bypassIcon", false)
            || button.getProperties().getWithDefault ("linkIcon", false)
            || button.getProperties().getWithDefault ("balanceIcon", false)
            || button.getProperties().getWithDefault ("lockIcon", false)
            || button.getProperties().getWithDefault ("arrowIcon", false))
            return; // Icon ist bereits vollstaendig in drawButtonBackground/drawLetterIcon gezeichnet bzw. unsichtbare Klickflaeche.

        const bool isGlobalRowButton = button.getProperties().getWithDefault ("globalBtn", false)
                                     || button.getProperties().getWithDefault ("abIcon", false)
                                     || button.getProperties().getWithDefault ("modBypassIcon", false)
                                     || button.getProperties().getWithDefault ("bypassToggleIcon", false)
                                     || button.getProperties().getWithDefault ("mutateIcon", false)
                                     || button.getProperties().getWithDefault ("breatheIcon", false);

        // Globaler A/B-Vergleich: statt den Button-Text bei jedem Klick
        // zwischen "A" und "B" zu tauschen (User-Feedback: "soll immer A/B
        // drauf stehen"), steht immer "A/B" da - nur der gerade AKTIVE
        // Buchstabe leuchtet hell, der andere bleibt gedimmt.
        if (button.getProperties().getWithDefault ("abIcon", false))
        {
            drawABContent (g, button);
        }
        // Globaler Mod-Bypass: "MOD" allein war nicht klar genug (User-
        // Feedback) - jetzt eine kleine Sinuswelle (Symbol fuer Modulation)
        // plus der Text "OFF", der rot aufleuchtet, sobald die Modulation
        // gerade tatsaechlich global stummgeschaltet ist.
        else if (button.getProperties().getWithDefault ("modBypassIcon", false))
        {
            drawModBypassContent (g, button);
        }
        // Neue, vereinheitlichte Icon+Text-Buttons (User-Feedback: "Jeder der
        // 3 Buttons sieht anders aus" -> jetzt alle im Stil von Mod On/Off).
        else if (button.getProperties().getWithDefault ("bypassToggleIcon", false))
        {
            drawBypassToggleContent (g, button);
        }
        else if (button.getProperties().getWithDefault ("mutateIcon", false))
        {
            drawMutateContent (g, button);
        }
        else if (button.getProperties().getWithDefault ("breatheIcon", false))
        {
            drawBreatheContent (g, button);
        }
        else
        {
            const bool isOn = button.getToggleState() && ! button.getProperties().getWithDefault ("sectionOff", false);
            g.setColour (isOn ? juce::Colours::white : juce::Colour (0xffb5b9c2));
            // Globale Buttons (RESET) bekommen dieselbe feste Schriftgroesse wie
            // die Parameter-Labels (EXPAND/BOOST/...) statt der sonst ueblichen,
            // an die Button-Hoehe gekoppelten Autoskalierung (User-Feedback:
            // "alle Schriften in der Global-Zeile sollen so gross sein wie
            // Expand, Boost usw."). "bigGlobalIcon" (nur das Hamburger-Menue)
            // bekommt eine deutlich groessere, eigene Schrift (User-Wunsch:
            // "Hamburger Menu Icon deutlich groesser").
            if (button.getProperties().getWithDefault ("bigGlobalIcon", false))
                g.setFont (hamburgerIconFont());
            else if (button.getProperties().getWithDefault ("globalBtn", false))
                g.setFont (globalRowFont());
            else
                g.setFont (juce::Font (juce::FontOptions (button.getHeight() * 0.42f, juce::Font::bold)).withExtraKerningFactor (0.04f));
            g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
        }

        // Einheitlicher, dezenter Mouse-Hover-/Klick-Effekt fuer die gesamte
        // globale Button-Zeile (User-Wunsch: "Ueberall Mouse Hover Effekte ...
        // sollte relativ einheitlich aussehen alles") - wird zuletzt
        // gezeichnet, liegt also ueber dem jeweiligen Inhalt, veraendert aber
        // keine der bestehenden Icon-/Text-Farblogiken.
        if (isGlobalRowButton && (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown))
        {
            auto hb = button.getLocalBounds().toFloat().reduced (1.0f);
            // Deutlich auffaelliger als vorher (0.06/0.10 -> 0.13/0.20,
            // User-Feedback: "Save, Reset und Undo/Redo -> mouse hover
            // auffaelliger gestalten"). Gilt fuer Save/Reset/A-B/Mod-Bypass
            // usw. (siehe isGlobalRowButton oben); Undo/Redo bekommen den
            // Effekt separat in drawArrowIcon(), da sie hier frueh
            // returnen.
            g.setColour (juce::Colours::white.withAlpha (shouldDrawButtonAsDown ? 0.20f : 0.13f));
            g.fillRoundedRectangle (hb, hb.getHeight() * 0.30f);
        }
    }

    // Einheitliche Schriftgroesse fuer die gesamte globale Button-Zeile
    // (Reset/A-B/Mod-Bypass) - exakt dieselbe wie die Parameter-Labels
    // (EXPAND/BOOST/...), siehe drawButtonText/drawABContent/drawModBypassContent.
    static juce::Font globalRowFont()
    {
        return juce::Font (juce::FontOptions (12.0f, juce::Font::bold)).withExtraKerningFactor (0.06f);
    }

    // Deutlich groessere Schrift nur fuer das Hamburger-Menue-Symbol.
    static juce::Font hamburgerIconFont()
    {
        return juce::Font (juce::FontOptions (22.0f, juce::Font::bold));
    }

    // Schlichtes Ein/Aus-Symbol (Kreis mit oben unterbrochenem Ring + kurzer
    // Linie) - versteht jeder sofort, ohne Text. Leuchtet in der Akzentfarbe
    // wenn aktiv, ist ansonsten dezent/gedimmt (matcht den Rest der GUI).
    void drawPowerIcon (juce::Graphics& g, juce::Button& button)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
        const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto centre = bounds.getCentre();
        // Bug-Fix: bei Logo-Bypass blieb das Power-Icon farbig, obwohl der
        // Rest der Sektion ausgegraut wurde - der Power-PARAMETER selbst
        // bleibt beim Bypass ja unveraendert an, nur "sectionOff" (hier neu
        // ausgewertet) weiss vom Bypass-Zustand.
        const bool sectionIsOff = button.getProperties().getWithDefault ("sectionOff", false);
        const bool isOn = button.getToggleState() && ! sectionIsOff;
        auto col = isOn ? accent : juce::Colour (0xff5a5e68);

        // WICHTIG: hier bewusst KEINE Hintergrund-Flaeche/Glow-Kreis mehr -
        // die sah bei aktivem Icon wie ein "Kaestchen" um das Icon herum aus
        // (harte Alpha-Stufen ohne echten Blur). Es soll nur das Icon selbst
        // (Ring + Linie) farbig leuchten, kein umgebender Hintergrund.
        juce::Path ring;
        // Kreis mit einer Luecke oben (ca. 70 Grad), klassisches Power-Symbol.
        ring.addCentredArc (centre.x, centre.y, r * 0.62f, r * 0.62f, 0.0f,
                             juce::MathConstants<float>::pi * 0.28f,
                             juce::MathConstants<float>::twoPi - juce::MathConstants<float>::pi * 0.28f, true);
        g.setColour (col);
        g.strokePath (ring, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.drawLine (centre.x, centre.y - r * 0.75f, centre.x, centre.y - r * 0.15f, 1.6f);
    }

    // Solo-Icon: bewusst NUR der nackte, fette Buchstabe ohne jeden Kreis/
    // Rahmen drumherum (User-Feedback: "Einfach ein S ohne Kreis drum
    // herum" - mit Ring war der Ein/Aus-Zustand kaum zu erkennen).
    void drawLetterIcon (juce::Graphics& g, juce::Button& button, const juce::String& letter, juce::Colour onColour)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
        const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        // Bug-Fix: dasselbe "sectionOff ignoriert" Problem wie beim
        // Power-Icon - bei Bypass soll auch das Solo-"S" ausgrauen.
        const bool sectionIsOff = button.getProperties().getWithDefault ("sectionOff", false);
        const bool isOn = button.getToggleState() && ! sectionIsOff;
        // Off-Farbe deutlich aufgehellt (vorher 0x5a5e68 - vor dunklem
        // Hintergrund kaum erkennbar, siehe User-Feedback "fast nicht
        // sichtbar"), plus groesserer Font, damit der Buchstabe auch ohne
        // Ring gut lesbar bleibt.
        float alpha = 1.0f;
        if (isOn)
        {
            // "Soll auch blinken wie die anderen" (User-Feedback) - gleiches
            // sanftes Alpha-Pulsieren wie Mono-/Mod-Icon, damit eine aktive
            // Solo-Sektion (die dauerhaft an bleibt) genauso "lebt".
            const double t = juce::Time::getMillisecondCounterHiRes() * 0.001;
            constexpr double periodSeconds = 2.2;
            alpha = 0.55f + 0.45f * (float) (0.5 + 0.5 * std::sin (juce::MathConstants<double>::twoPi * t / periodSeconds));
        }
        auto col = (isOn ? onColour : juce::Colour (0xff888d99)).withAlpha (isOn ? alpha : 1.0f);

        g.setColour (col);
        g.setFont (juce::Font (juce::FontOptions (r * 1.5f, juce::Font::bold)));
        g.drawText (letter, bounds.toNearestInt(), juce::Justification::centred);
    }

    // Mono-Check-Icon: kleines Lautsprecher-Symbol mit EINEM zentralen
    // Treiber (statt zwei fuer L/R) - eindeutiger als der Buchstabe "M",
    // der zu leicht mit "Mute" verwechselt wird (User-Feedback). Im aktiven
    // Zustand pulsiert es sanft (Alpha-Sinus statt hartem An/Aus, User-
    // Feedback: "soll leicht blinken, aber smooth") - PluginEditor sorgt per
    // Timer fuer kontinuierliches Repaint, waehrend der Button aktiv ist.
    void drawMonoIcon (juce::Graphics& g, juce::Button& button)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (2.0f);
        const float s = juce::jmin (bounds.getWidth(), bounds.getHeight());
        auto centre = bounds.getCentre();
        // Bug-Fix: Mono-Icon ignorierte "sectionOff" bisher komplett - bei
        // Bypass hat Mono-Check ohnehin keine Wirkung mehr (Verarbeitung wird
        // komplett uebersprungen), sollte also ebenfalls ausgrauen.
        const bool sectionIsOff = button.getProperties().getWithDefault ("sectionOff", false);
        const bool isOn = button.getToggleState() && ! sectionIsOff;

        float alpha = 1.0f;
        if (isOn)
        {
            const double t = juce::Time::getMillisecondCounterHiRes() * 0.001;
            constexpr double periodSeconds = 2.2;
            alpha = 0.55f + 0.45f * (float) (0.5 + 0.5 * std::sin (juce::MathConstants<double>::twoPi * t / periodSeconds));
        }
        // Auffaelligeres Rot statt der Akzentfarbe, wenn Mono-Check aktiv ist
        // (User-Feedback: soll sich klar von "Solo" (Amber) unterscheiden und
        // deutlich signalisieren "nur temporaer gedacht, nicht vergessen").
        static const juce::Colour monoOnColour (0xffff5b5b);
        auto col = (isOn ? monoOnColour : juce::Colour (0xff5a5e68)).withAlpha (isOn ? alpha : 1.0f);
        g.setColour (col);

        juce::Rectangle<float> cabinet (centre.x - s * 0.28f, centre.y - s * 0.34f, s * 0.56f, s * 0.68f);
        g.drawRoundedRectangle (cabinet, 2.0f, 1.4f);

        g.drawEllipse (centre.x - s * 0.14f, centre.y - s * 0.14f, s * 0.28f, s * 0.28f, 1.4f);
        g.fillEllipse (centre.x - s * 0.035f, centre.y - s * 0.035f, s * 0.07f, s * 0.07f);
    }

    // Section-Lock-Icon (User-Wunsch: "Sections ausschliessen" von Chaos/
    // Mutate und Breathe, Variante "Lock Icon pro Section") - klassisches
    // Schloss-Symbol (Buegel + Kaestchen). User-Feedback ("Lock Symbol
    // grafisch gleich bleiben. Nur Farbe ändern reicht. Nimm das Icon für
    // das geschlossene Schloss. ... geht dann auch ein bisschen größer"):
    // die Grafik bleibt jetzt IMMER die geschlossene Schloss-Form (kein
    // Formwechsel zu einem offenen/weggeklappten Buegel mehr) - locked/
    // unlocked unterscheidet sich nur noch per Farbe/Alpha. Zusaetzlich
    // per kleinerem reduced()-Inset und groesseren s-Faktoren an die
    // restlichen Icons der Zeile (Power/Solo) angeglichen.
    void drawLockIcon (juce::Graphics& g, juce::Button& button)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
        const float s = juce::jmin (bounds.getWidth(), bounds.getHeight());
        auto centre = bounds.getCentre();
        const bool locked = button.getToggleState();

        static const juce::Colour lockOnColour (0xffffb648);
        auto col = locked ? lockOnColour : juce::Colour (0xff5a5e68);
        g.setColour (col.withAlpha (locked ? 1.0f : 0.7f));

        const float bodyW = s * 0.58f;
        const float bodyH = s * 0.46f;
        juce::Rectangle<float> body (centre.x - bodyW * 0.5f, centre.y - bodyH * 0.05f, bodyW, bodyH);
        g.fillRoundedRectangle (body, 1.6f);

        // Immer geschlossen: voller Bogen, beide Enden reichen bis in den
        // Kaestchen-Koerper hinein - unabhaengig vom Lock-Status.
        const float shackleR = s * 0.23f;
        const float shackleCy = body.getY() - shackleR * 0.15f;
        juce::Path shackle;
        shackle.addCentredArc (centre.x, shackleCy, shackleR, shackleR, 0.0f,
                                juce::MathConstants<float>::pi, juce::MathConstants<float>::twoPi, true);
        g.strokePath (shackle, juce::PathStrokeType (s * 0.13f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.fillEllipse (centre.x - s * 0.05f, centre.y + bodyH * 0.02f, s * 0.10f, s * 0.10f);
    }

    // Bypass-Icon fuer den Mono-Dry-A/B-Vergleich: klassisches "Signal
    // umgeht die Schaltung"-Symbol - eine Linie fuehrt herein, macht einen
    // Bogen UM ein kleines Kaestchen (die "Verarbeitung") herum, statt
    // hindurchzugehen, und endet rechts mit einer Pfeilspitze (User-
    // Feedback: "ein Symbol, das symbolisiert, dass eine technische
    // Schaltung umgangen wird"). Nur bedienbar/aktivierbar, waehrend
    // Mono-Check selbst an ist (User-Feedback: "soll nicht alleine gehen") -
    // im deaktivierten Zustand daher deutlich gedimmt, unabhaengig vom
    // eigenen Toggle-Status.
    void drawBypassIcon (juce::Graphics& g, juce::Button& button)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (3.0f);
        const bool sectionIsOff = button.getProperties().getWithDefault ("sectionOff", false);
        const bool isOn = button.getToggleState() && button.isEnabled() && ! sectionIsOff;

        static const juce::Colour dryOnColour (0xffff9a4d);
        auto col = isOn ? dryOnColour : juce::Colour (0xff5a5e68);
        g.setColour (col.withAlpha (button.isEnabled() ? 1.0f : 0.4f));

        const float y = bounds.getCentreY();
        const float boxW = bounds.getWidth() * 0.34f;
        const float boxH = bounds.getHeight() * 0.52f;
        juce::Rectangle<float> box (bounds.getCentreX() - boxW * 0.5f, y - boxH * 0.5f, boxW, boxH);
        g.drawRoundedRectangle (box, 1.5f, 1.4f);

        juce::Path bypass;
        bypass.startNewSubPath (bounds.getX(), y);
        bypass.lineTo (box.getX() - 1.5f, y);
        bypass.quadraticTo (bounds.getCentreX(), bounds.getY(), box.getRight() + 1.5f, y);
        bypass.lineTo (bounds.getRight() - 6.0f, y);
        g.strokePath (bypass, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path arrow;
        arrow.startNewSubPath (bounds.getRight() - 8.0f, y - 3.5f);
        arrow.lineTo (bounds.getRight(), y);
        arrow.lineTo (bounds.getRight() - 8.0f, y + 3.5f);
        g.strokePath (arrow, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Link-Icon (3. Anlauf, nach Wunsch-Bild des Users): eine duenne,
    // dezente "Bruecke" ueber L+R - eine fast waagerechte Linie mit sanft
    // nach unten auslaufenden, geschwungenen Enden (Richtung L bzw. R) und
    // einem kleinen Punkt in der Mitte. Bewusst sehr duenn (1.0px statt
    // vorher 1.8px) und in gedaempftem Grauweiss statt der satten Akzent-
    // farbe gehalten (User-Feedback: "Den Button selbst bitte dezent
    // halten"). Reagiert nur auf sectionOff (Polarity aus/Bypass), keine
    // eigene glimmende On/Off-Optik, da der Link-Button selbst keinen
    // eigenen Zustand haelt (reine Aktion).
    // 5. Anlauf (User-Feedback zur reinen Linien-Version: "Verbindungs-
    // linien feiner", "ein Icon in der Mitte - genau so gross wie z.B. das
    // On/Off-Icon, ein Link-Symbol"). Jetzt zwei Elemente: (1) zwei duenne
    // Linien, die von L bzw. R aus schraeg zur Mitte laufen (rein
    // dekorativ, zeigt "verbindet beides"), und (2) ein richtiges, klar
    // erkennbares Kettenglied-/Link-Symbol (zwei ueberlappende, leicht
    // gedrehte Kapsel-Ringe) in der Mitte - exakt so gross wie das Power-
    // Icon (Button-Hoehe als Referenz, nicht die volle, viel breitere
    // Button-Breite).
    void drawLinkIcon (juce::Graphics& g, juce::Button& button, bool highlighted = false, bool down = false)
    {
        auto full = button.getLocalBounds().toFloat();
        const bool sectionIsOff = button.getProperties().getWithDefault ("sectionOff", false);
        // Bewusst NIE farbig/leuchtend (User-Feedback: "Der Link Button soll
        // nie farbig sein. Das lenkt ab.") - immer dezentes, neutrales Grau.
        // Auf Hover/Klick wird es HELLER statt farbig (User-Feedback: "wie
        // die anderen Buttons auch, nur halt grau" - reine Helligkeits-
        // Rueckmeldung, damit klar wird, dass es ein Button ist).
        auto lineCol = sectionIsOff ? juce::Colour (0xff3d4046) : juce::Colour (0xff6d7178);
        auto iconCol = sectionIsOff ? juce::Colour (0xff45484f) : juce::Colour (0xff8a8e97);
        if (! sectionIsOff)
        {
            if (down)              { lineCol = lineCol.brighter (0.5f); iconCol = iconCol.brighter (0.5f); }
            else if (highlighted)  { lineCol = lineCol.brighter (0.28f); iconCol = iconCol.brighter (0.28f); }
        }

        // Icon-Groesse = Button-Hoehe (identisch zur Groesse des On/Off-
        // Icons, das ebenfalls in einer headerH-hohen, quadratischen Box
        // gezeichnet wird) - unabhaengig davon, wie breit dieser Button
        // (spannt L+R) tatsaechlich ist.
        const float iconSize = full.getHeight();
        auto iconBounds = full.withSizeKeepingCentre (iconSize, iconSize).reduced (1.0f);

        // Duenne, dezente Verbindungslinien vom Icon Richtung L bzw. R -
        // jetzt WAAGERECHT statt schraeg (User-Feedback: "nicht schraeg
        // sondern horizontal, wirkt nicht stimmig zum restlichen Design")
        // und reichen bewusst nur bis zur Mitte der L/R-Buttons statt fast
        // bis zum aeusseren Rand (User-Feedback: "nicht so weit nach aussen,
        // eher dezent bis zur Mitte der Buttons").
        const float lineY = full.getCentreY();
        const float endInsetL = full.getX() + full.getWidth() * 0.27f;
        const float endInsetR = full.getRight() - full.getWidth() * 0.27f;
        g.setColour (lineCol.withAlpha (sectionIsOff ? 0.45f : 0.6f));
        g.drawLine (endInsetL, lineY, iconBounds.getX() - 2.0f, lineY, 0.6f);
        g.drawLine (iconBounds.getRight() + 2.0f, lineY, endInsetR, lineY, 0.6f);

        // Klassisches Kettenglied-Symbol: zwei ueberlappende, leicht
        // gedrehte Kapsel-Ringe.
        g.setColour (iconCol);
        auto drawLinkRing = [&] (float angleDeg, float offset)
        {
            const float w = iconSize * 0.62f, h = iconSize * 0.32f;
            juce::Path capsule;
            capsule.addRoundedRectangle (-w * 0.5f, -h * 0.5f, w, h, h * 0.5f);
            const auto transform = juce::AffineTransform::rotation (juce::degreesToRadians (angleDeg))
                                        .translated (iconBounds.getCentreX() - offset * 0.35f,
                                                     iconBounds.getCentreY() + offset * 0.35f);
            capsule.applyTransform (transform);
            g.strokePath (capsule, juce::PathStrokeType (iconSize * 0.14f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        };
        drawLinkRing (-40.0f, -iconSize * 0.16f);
        drawLinkRing (-40.0f,  iconSize * 0.16f);
    }

    // Mod-Icon: eine einfache, gezeichnete Sinuswelle - "einfaches
    // An/Aus"-Symbol fuer die LFO-Modulation von Timewarp/Dimension/
    // Hyperdrive (User-Feedback: "Vielleicht eine Sinuswelle. Das versteht
    // jeder."). Kein eigener Depth-Regler, daher genuegt ein statisches
    // Icon, dessen Farbe/Helligkeit den An/Aus-Status zeigt - bei aktivem
    // Icon zusaetzlich ein sehr sanftes Alpha-Pulsieren (langsam, passend
    // zur Modulationsgeschwindigkeit selbst), damit sofort klar ist, dass
    // hier etwas "lebt".
    void drawModIcon (juce::Graphics& g, juce::Button& button)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (2.0f);
        const float w = bounds.getWidth();
        const float h = bounds.getHeight();
        // Bug-Fix: Icon zeigte bisher "an" (farbig), auch wenn die ganze
        // Sektion per Power-Icon ausgeschaltet war - "sectionOff" (dasselbe
        // Property wie bei allen anderen Reglern der Sektion) muss hier
        // ebenfalls den grauen Aus-Zustand erzwingen.
        const bool sectionIsOff = button.getProperties().getWithDefault ("sectionOff", false);
        // toggledOn = der ECHTE Mod-An/Aus-Status, unabhaengig von der
        // Section - wird jetzt auch fuers Pulsieren benutzt (User-Feedback:
        // "Wenn Section off soll Mod Icon pulsieren aber ohne Farbe. Ich
        // will erkennen ob Mod an oder aus ist, auch wenn Section off
        // ist."). Farbig ("colored") bleibt weiterhin an sectionOff
        // gekoppelt - nur EIN- oder AUSGRAUT bekommt jetzt einen eigenen,
        // von der Farbe unabhaengigen Puls.
        const bool toggledOn = button.getToggleState() && button.isEnabled();
        const bool colored = toggledOn && ! sectionIsOff;

        float alpha = 1.0f;
        if (toggledOn)
        {
            // Deutlich staerkerer Ausschlag als vorher (0.65-1.0 -> 0.28-1.0)
            // und etwas schnellerer Takt, damit das Pulsieren klar sichtbar
            // ist statt nur ein leichtes Flackern (User-Feedback: "sollen in
            // ihrer Leuchtkraft staerker pulsieren").
            const double t = juce::Time::getMillisecondCounterHiRes() * 0.001;
            constexpr double periodSeconds = 2.4;
            alpha = 0.28f + 0.72f * (float) (0.5 + 0.5 * std::sin (juce::MathConstants<double>::twoPi * t / periodSeconds));
        }
        auto baseCol = colored ? accent : juce::Colour (0xff5a5e68);
        auto col = baseCol.withAlpha (toggledOn ? alpha : (button.isEnabled() ? 1.0f : 0.4f));
        g.setColour (col);

        juce::Path wave;
        const int steps = 24;
        for (int i = 0; i <= steps; ++i)
        {
            const float t = (float) i / (float) steps;
            const float x = bounds.getX() + t * w;
            const float y = bounds.getCentreY() - std::sin (t * juce::MathConstants<float>::twoPi) * (h * 0.32f);
            if (i == 0) wave.startNewSubPath (x, y);
            else        wave.lineTo (x, y);
        }
        // Minimal dicker als vorher (1.6 -> 2.0px, User-Feedback: "Mod
        // Icons in den Sections minimal dicker").
        g.strokePath (wave, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // "Balance"-Icon (neben Drift): zwei Pfeile, die von aussen nach innen
    // zueinander zeigen - symbolisiert "wird zur Mitte zurueckgeholt"
    // (User-Feedback: automatische Gain-Kompensation fuer den durch Haas
    // verschobenen Praezedenzeffekt). Simples An/Aus, kein Pulsieren (kein
    // LFO dahinter, anders als die Mod-Icons).
    void drawBalanceIcon (juce::Graphics& g, juce::Button& button)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
        const bool sectionIsOff = button.getProperties().getWithDefault ("sectionOff", false);
        const bool isOn = button.getToggleState() && button.isEnabled() && ! sectionIsOff;
        auto col = (isOn ? accent : juce::Colour (0xff5a5e68)).withAlpha (button.isEnabled() ? 1.0f : 0.4f);
        g.setColour (col);

        const float cy = bounds.getCentreY();
        const float cx = bounds.getCentreX();
        const float headLen = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.32f;
        const float strokeW = 1.5f;

        // Linker Pfeil (zeigt nach rechts, zur Mitte).
        g.drawLine (bounds.getX(), cy, cx - headLen * 0.5f, cy, strokeW);
        juce::Path leftHead;
        leftHead.startNewSubPath (cx - headLen, cy - headLen * 0.55f);
        leftHead.lineTo (cx - headLen * 0.15f, cy);
        leftHead.lineTo (cx - headLen, cy + headLen * 0.55f);
        g.strokePath (leftHead, juce::PathStrokeType (strokeW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Rechter Pfeil (zeigt nach links, zur Mitte).
        g.drawLine (bounds.getRight(), cy, cx + headLen * 0.5f, cy, strokeW);
        juce::Path rightHead;
        rightHead.startNewSubPath (cx + headLen, cy - headLen * 0.55f);
        rightHead.lineTo (cx + headLen * 0.15f, cy);
        rightHead.lineTo (cx + headLen, cy + headLen * 0.55f);
        g.strokePath (rightHead, juce::PathStrokeType (strokeW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Inhalt des globalen A/B-Buttons: "A/B" steht immer da, nur der gerade
    // aktive Buchstabe leuchtet (weiss/hell), der inaktive bleibt gedimmt -
    // macht auf einen Blick klar, WAS der Button kann und WO man gerade ist,
    // statt bei jedem Klick den Text auszutauschen (User-Feedback).
    void drawABContent (juce::Graphics& g, juce::Button& button)
    {
        auto bounds = button.getLocalBounds().toFloat();
        const bool isA = button.getProperties().getWithDefault ("abIsA", true);
        const bool sectionIsOff = button.getProperties().getWithDefault ("sectionOff", false);
        // Aktiver Buchstabe leuchtet jetzt farbig (Akzent-Violett, dieselbe
        // Glow-Farbe wie bei aktiven Toggle-Buttons) statt nur weiss - User-
        // Feedback: "A und B sollen leuchten, also farbig sein".
        const juce::Colour activeCol = sectionIsOff ? juce::Colour (0xff8f93a8) : glowAccent;
        const juce::Colour dimCol (0xff6a6e78);

        // Groesser als der Rest der globalen Zeile (User-Wunsch: "muss von
        // der Schrift her groesser sein, A und B").
        g.setFont (juce::Font (juce::FontOptions (17.0f, juce::Font::bold)).withExtraKerningFactor (0.06f));

        const float w = bounds.getWidth();
        juce::Rectangle<float> aArea      (bounds.getX(),               bounds.getY(), w * 0.42f, bounds.getHeight());
        juce::Rectangle<float> slashArea  (bounds.getX() + w * 0.42f,   bounds.getY(), w * 0.16f, bounds.getHeight());
        juce::Rectangle<float> bArea      (bounds.getX() + w * 0.58f,   bounds.getY(), w * 0.42f, bounds.getHeight());

        g.setColour (isA ? activeCol : dimCol);
        g.drawText ("A", aArea.toNearestInt(), juce::Justification::centred);
        g.setColour (dimCol.withAlpha (0.7f));
        g.drawText ("/", slashArea.toNearestInt(), juce::Justification::centred);
        g.setColour (! isA ? activeCol : dimCol);
        g.drawText ("B", bArea.toNearestInt(), juce::Justification::centred);
    }

    // Inhalt des globalen Mod-Bypass-Buttons: kleine Sinuswelle (dasselbe
    // Symbol wie bei den Sektions-Mod-Icons, damit der Bezug zu "Modulation"
    // sofort klar ist) + der Text "OFF" daneben - die Kombination sagt klar
    // "das hier schaltet Modulation aus", statt nur "MOD" (User-Feedback:
    // "nicht klar was er macht von Namen her"). "OFF" leuchtet auffaellig
    // rot, wenn die Modulation gerade tatsaechlich global stumm ist; die
    // Welle selbst pulsiert sanft (wie die anderen Mod-Icons), solange die
    // Modulation normal laeuft.
    // Korrektur (User-Feedback: "Das letzte Mal habe ich gesagt, dass Mod
    // off rot sein soll. Aber du hast direkt das Icon geaendert. Es reicht,
    // wenn die Sinuswelle zu sehen ist, aber einfach rot.") - zurueck auf
    // die einfache Sinuswelle in BEIDEN Zustaenden, kein Hintergrund-Fill
    // und kein durchgestrichenes Mute-Icon mehr. Nur die Wellenfarbe zeigt
    // an/aus (rot statt gruen), der Rest bleibt wie vorher (ON/OFF-Text).
    void drawModBypassContent (juce::Graphics& g, juce::Button& button)
    {
        // Mehr Innenabstand links (User-Feedback: "das Mod Icon ist zu nah
        // am Rand des Buttons") - die Sinuswelle klebte vorher fast am
        // Button-Rand, jetzt deutlich mehr Luft rundherum.
        auto fullBounds = button.getLocalBounds().toFloat();
        auto bounds = fullBounds.reduced (9.0f, 3.0f);
        const bool sectionIsOff = button.getProperties().getWithDefault ("sectionOff", false);
        // bypassOn = true bedeutet: globale Modulation ist AUS (der Parameter
        // heisst "Bypass"). modActive ist die fuer den User verstaendlichere
        // Umkehrung ("Mod steht an/laeuft gerade").
        const bool bypassOn = button.getToggleState() && ! sectionIsOff;
        const bool modActive = ! bypassOn;

        static const juce::Colour onGreen (0xff3ddc73);
        static const juce::Colour offRed  (0xffff5b5b);

        auto iconArea = bounds.removeFromLeft (bounds.getHeight());
        bounds.removeFromLeft (4.0f);

        const double t = juce::Time::getMillisecondCounterHiRes() * 0.001;
        constexpr double periodSeconds = 2.4;
        const float pulse = 0.5f + 0.5f * (float) (0.5 + 0.5 * std::sin (juce::MathConstants<double>::twoPi * t / periodSeconds));
        auto waveCol = sectionIsOff ? juce::Colour (0xff5a5e68)
                                    : (modActive ? onGreen.withAlpha (pulse) : offRed);
        g.setColour (waveCol);
        juce::Path wave;
        const int steps = 14;
        const float w = iconArea.getWidth(), h = iconArea.getHeight();
        for (int i = 0; i <= steps; ++i)
        {
            const float tt = (float) i / (float) steps;
            const float x = iconArea.getX() + tt * w;
            const float y = iconArea.getCentreY() - std::sin (tt * juce::MathConstants<float>::twoPi) * (h * 0.34f);
            if (i == 0) wave.startNewSubPath (x, y); else wave.lineTo (x, y);
        }
        g.strokePath (wave, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        auto textCol = sectionIsOff ? juce::Colour (0xff5a5e68) : (bypassOn ? offRed : onGreen);
        g.setColour (textCol);
        g.setFont (globalRowFont());
        g.drawText (modActive ? "ON" : "OFF", bounds.toNearestInt(), juce::Justification::centred);
    }

    // Inhalt des neuen globalen Bypass-Buttons (BYP): dasselbe Power-Ring-
    // Icon wie die Section-Bypass-Schalter + fester Text "BYP" - rot, wenn
    // das gesamte Plugin gerade per UI-Bypass stummgeschaltet ist, sonst
    // gruen (wie "an" bei Mod On/Off). Gleicher Icon-links/Text-rechts-Aufbau
    // wie drawModBypassContent, fuer eine einheitliche Optik der ganzen
    // Gruppe (User-Feedback: "Jeder der 3 Buttons sieht anders aus").
    void drawBypassToggleContent (juce::Graphics& g, juce::Button& button)
    {
        auto fullBounds = button.getLocalBounds().toFloat();
        auto bounds = fullBounds.reduced (9.0f, 3.0f);
        const bool bypassed = button.getToggleState();

        static const juce::Colour onGreen (0xff3ddc73);
        static const juce::Colour offRed  (0xffff5b5b);
        auto col = bypassed ? offRed : onGreen;

        auto iconArea = bounds.removeFromLeft (bounds.getHeight());
        bounds.removeFromLeft (4.0f);

        g.setColour (col);
        auto centre = iconArea.getCentre();
        const float r = iconArea.getHeight() * 0.5f;
        juce::Path ring;
        ring.addCentredArc (centre.x, centre.y, r * 0.62f, r * 0.62f, 0.0f,
                             juce::MathConstants<float>::pi * 0.28f,
                             juce::MathConstants<float>::twoPi - juce::MathConstants<float>::pi * 0.28f, true);
        g.strokePath (ring, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.drawLine (centre.x, centre.y - r * 0.75f, centre.x, centre.y - r * 0.15f, 1.6f);

        g.setColour (col);
        g.setFont (globalRowFont());
        g.drawText ("BYP", bounds.toNearestInt(), juce::Justification::centred);
    }

    // Inhalt des Mutate(Chaos)-Buttons (User-Wunsch, 2. Anlauf: "Namen
    // entfernen ... 4 kleine Kaestchen in den Farben unseres Schemas (lila
    // und tuerkis) ... abwechselnd und in 2 Reihen ... gefuellt ... bei
    // jedem Mal drauf klicken aendert sich die Farbe ... randomly"). Kein
    // Text mehr, das Icon nutzt die komplette Button-Flaeche. "mutateColorState"
    // (4 Bits, je eines pro Kaestchen) entscheidet lila (glowAccent) vs.
    // tuerkis (accent), wird bei jedem Klick neu gewuerfelt (siehe
    // globalChaosButton.onClick) - soll den Eindruck erwecken, dass die
    // Sektionen darunter randomisiert werden.
    void drawMutateContent (juce::Graphics& g, juce::Button& button)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (10.0f, 6.0f);
        const int colorState = (int) button.getProperties().getWithDefault ("mutateColorState", 0b0101);

        const float cell = juce::jmin (bounds.getWidth() * 0.42f, bounds.getHeight() * 0.42f);
        const float gap = cell * 0.28f;
        const float gridW = cell * 2.0f + gap;
        const float gridH = cell * 2.0f + gap;
        const float x0 = bounds.getCentreX() - gridW * 0.5f;
        const float y0 = bounds.getCentreY() - gridH * 0.5f;

        for (int row = 0; row < 2; ++row)
        {
            for (int col = 0; col < 2; ++col)
            {
                const int bitIndex = row * 2 + col;
                const bool isPurple = ((colorState >> bitIndex) & 1) != 0;
                g.setColour (isPurple ? glowAccent : accent);
                juce::Rectangle<float> r (x0 + (float) col * (cell + gap), y0 + (float) row * (cell + gap), cell, cell);
                g.fillRoundedRectangle (r, cell * 0.2f);
            }
        }
    }

    // Inhalt des Breathe-Buttons (User-Wunsch, 2. Anlauf: "Fuege einen
    // kleinen Strich in das Logo ein, damit es aehnlich aussieht wie die
    // Regler ... Jedes Mal, wenn man auf das Icon klickt veraendert sich
    // die Position des Striches ... Dann kann man sich auch das Wort
    // Breathe sparen"). Kein Text mehr, das Icon nutzt die komplette
    // Button-Flaeche. Pulsierender Kreis bleibt (User: "Das Pulsierende
    // gefaellt mir aber, lass das so"), zusaetzlich ein kurzer Strich vom
    // Zentrum zum Rand - Position (8 feste Winkel) wechselt bei jedem Klick
    // ueber "breatheStrokeState" (siehe globalBreatheButton.onClick),
    // aehnlich einem Regler-Zeiger.
    void drawBreatheContent (juce::Graphics& g, juce::Button& button)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (6.0f, 4.0f);
        auto c = bounds.getCentre();

        const double t = juce::Time::getMillisecondCounterHiRes() * 0.001;
        constexpr double periodSeconds = 3.2;
        const float breath = 0.5f + 0.5f * (float) (0.5 + 0.5 * std::sin (juce::MathConstants<double>::twoPi * t / periodSeconds));

        const float maxR = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.30f;
        const float r = maxR * (0.6f + 0.4f * breath);
        g.setColour (accent.withAlpha (0.35f + 0.25f * breath));
        g.fillEllipse (c.x - r * 1.6f, c.y - r * 1.6f, r * 3.2f, r * 3.2f);
        g.setColour (accent.withAlpha (0.75f + 0.25f * breath));
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

        // Regler-aehnlicher Strich: 8 feste Positionen im Kreis um das Icon.
        const int strokeState = (int) button.getProperties().getWithDefault ("breatheStrokeState", 0);
        const float angle = (float) strokeState * (juce::MathConstants<float>::twoPi / 8.0f);
        const float strokeR1 = maxR * 1.15f;
        const float strokeR2 = maxR * 1.75f;
        const juce::Point<float> p1 (c.x + std::cos (angle) * strokeR1, c.y + std::sin (angle) * strokeR1);
        const juce::Point<float> p2 (c.x + std::cos (angle) * strokeR2, c.y + std::sin (angle) * strokeR2);
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.drawLine (p1.x, p1.y, p2.x, p2.y, 2.0f);
    }

    // Undo/Redo-Pfeil-Icon (User-Feedback, 2. Anlauf: "Undo und Redo Pfeile
    // eher so (grobe Richtung). Reihenfolge umgekehrt." - Referenzbild
    // zeigte das klassische runde "Verlauf"-Pfeilsymbol statt der vorher
    // gebauten geraden Schaft+Dreieck-Form). Ein knapp dreiviertel
    // geschlossener Kreisbogen mit Pfeilspitze am offenen Ende, kanonisch
    // fuer REDO im Uhrzeigersinn gezeichnet - Undo wird per horizontaler
    // Spiegelung exakt daraus abgeleitet, damit beide garantiert echte
    // Spiegelbilder sind (behebt gleichzeitig die "Reihenfolge umgekehrt"-
    // Ruecknmeldung: die Drehrichtung von Undo ist jetzt die exakte
    // Umkehrung von Redo, nicht mehr unabhaengig/inkonsistent gezeichnet).
    // Richtung per "arrowDirection" Component-Property ("left" = Undo,
    // "right" = Redo). Gedimmt, wenn kein Undo/Redo mehr moeglich ist.
    void drawArrowIcon (juce::Graphics& g, juce::Button& button, bool highlighted, bool down)
    {
        // Undo/Redo hatten bisher KEINERLEI Hover-/Klick-Feedback (User-
        // Feedback: "Mouse hover auffaelliger gestalten" bezog sich u.a. auf
        // diese beiden - vorher wurde highlighted/down hier gar nicht erst
        // uebergeben). Gleicher Pillen-Hover wie bei Save/Reset (siehe
        // drawButtonText()), nur direkt hier gezeichnet, da der Pfeil
        // komplett in drawButtonBackground() lebt und drawButtonText() fuer
        // "arrowIcon" ganz oben frueh returned.
        auto fullBounds = button.getLocalBounds().toFloat().reduced (1.0f);
        if (down || highlighted)
        {
            g.setColour (juce::Colours::white.withAlpha (down ? 0.20f : 0.13f));
            g.fillRoundedRectangle (fullBounds, fullBounds.getHeight() * 0.30f);
        }

        auto bounds = button.getLocalBounds().toFloat().reduced (5.0f);
        const bool isUndo = button.getProperties().getWithDefault ("arrowDirection", "left").toString() == "left";
        auto col = juce::Colour (0xffb5b9c2).withAlpha (button.isEnabled() ? 1.0f : 0.3f);
        if (button.isEnabled() && (down || highlighted))
            col = juce::Colours::white;

        const float s = juce::jmin (bounds.getWidth(), bounds.getHeight());
        auto c = bounds.getCentre();
        const float r = s * 0.33f;

        // Winkel-Konvention von addCentredArc: 0 Grad = oben (12 Uhr),
        // steigend im Uhrzeigersinn. 280-Grad-Bogen, 80 Grad Luecke oben,
        // damit Anfang und Ende des Bogens klar sichtbar auseinanderklaffen.
        constexpr float fromDeg = 40.0f;
        constexpr float toDeg   = 320.0f;

        juce::Path arc;
        arc.addCentredArc (c.x, c.y, r, r, 0.0f, juce::degreesToRadians (fromDeg), juce::degreesToRadians (toDeg), true);

        // Pfeilspitze am Ende des Bogens: bei steigendem Winkel (= Bewegung
        // im Uhrzeigersinn) zeigt die Bewegungsrichtung dort exakt in
        // Richtung (cos th, sin th) - siehe Punkt-Parametrisierung oben
        // (x = cx + r*sin th, y = cy - r*cos th; Ableitung nach th ergibt
        // genau diesen Tangentenvektor).
        const float th = juce::degreesToRadians (toDeg);
        const juce::Point<float> tip (c.x + r * std::sin (th), c.y - r * std::cos (th));
        const juce::Point<float> dir (std::cos (th), std::sin (th));
        const juce::Point<float> normal (-dir.y, dir.x);
        const float headLen = s * 0.30f, headW = s * 0.28f;
        const auto tipFwd  = tip + dir * (headLen * 0.5f);
        const auto backC   = tip - dir * (headLen * 0.5f);
        const auto corner1 = backC + normal * (headW * 0.5f);
        const auto corner2 = backC - normal * (headW * 0.5f);
        juce::Path head;
        head.addTriangle (tipFwd, corner1, corner2);

        if (isUndo)
        {
            const auto mirror = juce::AffineTransform::scale (-1.0f, 1.0f, c.x, c.y);
            arc.applyTransform (mirror);
            head.applyTransform (mirror);
        }

        g.setColour (col);
        g.strokePath (arc, juce::PathStrokeType (s * 0.14f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.fillPath (head);
    }

    // "Galaxy muss DEUTLICH auffaelliger sein wenn aktiv" (User-Wunsch) -
    // pulsierender Glow-Hintergrund statt der neutralen, transparenten
    // globalBtn-Optik, die alle anderen Buttons dieser Zeile nutzen.
    // Farb-Konvention (User-Wunsch: "Galaxy global button muss die gleiche
    // Farbe haben wie der Rahmen um Galaxy. Merke dir das."): DERSELBE Blau-
    // Ton wie der Gruppen-Rahmen der Galaxy/LCR-Sektion (0xff4fa8ff, siehe
    // PluginEditor::layoutContent()/paintContent() groupColour(SOLO_GALAXY,
    // ...) und applyTitleDim(lcrTitleLabel, ...)) statt der allgemeinen
    // lila glowAccent-Farbe, die sonst fuer aktive Toggles benutzt wird.
    void drawGalaxyButtonBackground (juce::Graphics& g, juce::Button& button, bool highlighted, bool down)
    {
        static const juce::Colour galaxyBlue (0xff4fa8ff);
        auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
        const float cornerSize = bounds.getHeight() * 0.5f;
        if (button.getToggleState())
        {
            const double t = juce::Time::getMillisecondCounterHiRes() * 0.001;
            constexpr double periodSeconds = 1.8;
            const float pulse = 0.5f + 0.5f * (float) (0.5 + 0.5 * std::sin (juce::MathConstants<double>::twoPi * t / periodSeconds));
            g.setColour (galaxyBlue.withAlpha (0.16f + 0.12f * pulse));
            g.fillRoundedRectangle (bounds, cornerSize);
            g.setColour (galaxyBlue.withAlpha (0.55f + 0.35f * pulse));
            g.drawRoundedRectangle (bounds, cornerSize, 1.6f);
        }
        if (highlighted || down)
        {
            g.setColour (juce::Colours::white.withAlpha (down ? 0.10f : 0.06f));
            g.fillRoundedRectangle (bounds, cornerSize);
        }
    }

    // "Orbit"-Regler: statt eines generischen Balkens ein Kegel/Dreieck -
    // unten spitz zulaufend = neutral/aus (0%, Default), oben breit = voll
    // "Richtung L+R" (100%). Frueher (Range -100..100) war die Spitze "nur
    // Center" und die Mitte der neutrale Punkt - seit der Range-Aenderung
    // auf 0..100 (User-Feedback: "Orbit soll gar nicht schmaelern ... nur
    // noch Werte oberhalb von default position") faellt die "nur Center"-
    // Haelfte komplett weg und die Spitze selbst IST der Neutralpunkt, daher
    // entfaellt die vorherige, separate graue Mittelmarkierung. Aktiviert
    // ueber die Component-Property "focusStyle" (Slider muss LinearVertical
    // sein).
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                            float sliderPos, float minSliderPos, float maxSliderPos,
                            const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        const bool focusStyle = slider.getProperties().getWithDefault ("focusStyle", false);
        if (! focusStyle || style != juce::Slider::LinearVertical)
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
            return;
        }

        const bool offVisual = slider.getProperties().getWithDefault ("sectionOff", false) || ! slider.isEnabled();

        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
        auto top = bounds.getY() + 6.0f;
        auto bottom = bounds.getBottom() - 6.0f;
        auto cx = bounds.getCentreX();

        // Deutlich breiter als zuvor, damit der Kegel klar als Form erkennbar
        // ist (vorher zu schmal/unauffaellig).
        const float topHalfWidth = juce::jmin (bounds.getWidth() * 0.5f - 2.0f, 40.0f);

        // Kegel-Umriss: Spitze unten (neutral/aus), breit oben (voll L+R).
        juce::Path cone;
        cone.startNewSubPath (cx, bottom);
        cone.lineTo (cx - topHalfWidth, top);
        cone.lineTo (cx + topHalfWidth, top);
        cone.closeSubPath();

        // Immer sichtbar, auch wenn der Regler deaktiviert ist (vorher zu
        // dunkel/kontrastarm - war der eigentliche "unsichtbar"-Bug).
        g.setColour (juce::Colour (0xff454952));
        g.strokePath (cone, juce::PathStrokeType (1.4f));

        // Fuellung von der Spitze bis zur aktuellen Position.
        const float fillTopY = juce::jmax (top, sliderPos);
        const float t = juce::jlimit (0.0f, 1.0f, (bottom - fillTopY) / juce::jmax (1.0f, bottom - top));
        const float fillHalfWidth = topHalfWidth * t;

        juce::Path fill;
        fill.startNewSubPath (cx, bottom);
        fill.lineTo (cx - fillHalfWidth, fillTopY);
        fill.lineTo (cx + fillHalfWidth, fillTopY);
        fill.closeSubPath();

        auto fillCol = offVisual ? juce::Colour (0xff555861) : accent;
        g.setColour (fillCol.withAlpha (0.85f));
        g.fillPath (fill);

        // Live-Mod-Anzeige fuer Orbit (Galaxy-Mod) - gleiches Prinzip wie
        // der leuchtende Punkt bei den Rotary-Reglern (siehe
        // drawRotarySlider, "modLiveActive"/"modLiveValue"), hier aber als
        // waagerechte Linie statt eines Punkts, weil Orbit kein Drehregler
        // ist, sondern dieser Kegel-Fader (User-Feedback: "quasi eine
        // horizontale Linie ... natuerlich immer nur so breit wie die
        // aktuelle Stelle im Dreieck").
        //
        // ECHTER BUG-FIX (endlich gefunden, User-Feedback "Orbit Modulation
        // bewegt sich immer noch nicht" - mehrfach gemeldet): die vorherige
        // Version rechnete den Live-Wert ueber die Funktionsparameter
        // minSliderPos/maxSliderPos um. Fuer einen NORMALEN (nicht Zwei-/
        // Drei-Werte-)Slider liefert JUCE dort aber NICHT die Pixel-
        // Positionen von Minimum/Maximum, sondern zwei Kopien von sliderPos
        // selbst (der aktuellen Reglerposition)! liveY landete dadurch immer
        // exakt auf sliderPos, egal welchen Wert liveT hatte - die Live-
        // Linie lag optisch IMMER exakt auf dem normalen Positions-Punkt und
        // "bewegte sich" dadurch nie sichtbar davon weg. Jetzt wird liveT
        // stattdessen direkt auf die bereits bekannte Kegel-Achse (top=Wert
        // 100%, bottom=Wert 0%, siehe Fuellung weiter oben im selben Stil)
        // abgebildet - unabhaengig von JUCEs sliderPos/minSliderPos/
        // maxSliderPos.
        if (! offVisual && slider.getProperties().getWithDefault ("modLiveActive", false))
        {
            const float liveT = juce::jlimit (0.0f, 1.0f, (float) slider.getProperties().getWithDefault ("modLiveValue", 0.0f));
            const float liveY = juce::jmap (liveT, 0.0f, 1.0f, bottom, top);
            const float tCone = juce::jlimit (0.0f, 1.0f, (bottom - liveY) / juce::jmax (1.0f, bottom - top));
            const float liveHalfWidth = topHalfWidth * tCone;

            g.setColour (glowAccent.withAlpha (0.30f));
            g.drawLine (cx - liveHalfWidth, liveY, cx + liveHalfWidth, liveY, 5.0f);
            g.setColour (glowAccent);
            g.drawLine (cx - liveHalfWidth, liveY, cx + liveHalfWidth, liveY, 2.0f);
        }

        // Aktuelle Position als leuchtender Punkt - ebenfalls ueber der Live-
        // Linie, bleibt also immer als eigener (tuerkiser) Punkt erkennbar.
        auto dotCol = offVisual ? juce::Colour (0xff777b85) : accent;
        g.setColour (dotCol.withAlpha (0.25f));
        g.fillEllipse (cx - 8.0f, sliderPos - 8.0f, 16.0f, 16.0f);
        g.setColour (juce::Colours::white);
        g.fillEllipse (cx - 2.0f, sliderPos - 2.0f, 4.0f, 4.0f);
    }

    // Groesserer, klar lesbarer Text fuer die Sync-Raten-Box.
    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions (15.0f, juce::Font::bold)).withExtraKerningFactor (0.03f);
    }

    // ComboBox mit Glow-Rahmen, wenn die Component-Property "glowActive"
    // gesetzt ist (genutzt fuer die Sync-Raten-Box, wenn Sync aktiv ist).
    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                        int, int, int, int, juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (1.0f);
        const bool glow = box.getProperties().getWithDefault ("glowActive", false)
                           && ! box.getProperties().getWithDefault ("sectionOff", false);

        g.setColour (findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (bounds, 6.0f);

        g.setColour (glow ? glowAccent : findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (bounds, 6.0f, glow ? 1.6f : 1.0f);

        juce::Rectangle<int> arrowZone (width - 22, 0, 18, height);
        juce::Path path;
        path.startNewSubPath ((float) arrowZone.getX() + 3.0f, (float) arrowZone.getCentreY() - 3.0f);
        path.lineTo ((float) arrowZone.getCentreX(), (float) arrowZone.getCentreY() + 3.0f);
        path.lineTo ((float) arrowZone.getRight() - 3.0f, (float) arrowZone.getCentreY() - 3.0f);
        g.setColour (juce::Colour (0xff9ba0aa));
        g.strokePath (path, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    juce::Colour accent { 0xff5be3c7 };
    juce::Colour glowAccent { 0xffb968ff };
};
