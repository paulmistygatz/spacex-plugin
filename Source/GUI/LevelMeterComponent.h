#pragma once
#include <JuceHeader.h>
#include "CustomLookAndFeel.h"

// Kleine, minimalistische Pegelanzeige (User-Wunsch: "Links und rechts von
// Volume ein kleines Input und Output Meter Pegelanzeige") - liest nur den
// bereits im Processor abgelegten rohen Block-Peak (siehe currentInputLevel/
// currentOutputLevel in PluginProcessor.h), macht selbst noch ein einfaches
// Attack/Decay fuer eine ruhige Anzeige und zeichnet einen simplen
// horizontalen Balken (Platz unter dem Korrelationsmesser, User-Wunsch:
// "Input Meter und Output Meter, Horizontal, je links von beiden IN bzw.
// OUT"). Header-only nach demselben Muster wie CustomLookAndFeel.h - kein
// eigenes CMakeLists-Entry noetig.
class LevelMeterComponent : public juce::Component, private juce::Timer
{
public:
    explicit LevelMeterComponent (std::atomic<float>& levelToRead) : level (levelToRead)
    {
        startTimerHz (30);
    }

    ~LevelMeterComponent() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        g.setColour (juce::Colour (0xff14161a));
        g.fillRoundedRectangle (bounds, 3.0f);

        // Bug-Fix/Korrektur (User-Feedback: die Meter waren praktisch nicht
        // zu sehen) - eine immer sichtbare, schwache "Leerspur" ueber die
        // volle Breite, damit man den Meter-Schlitz auch OHNE Signal klar
        // erkennt (vorher gab es ohne Pegel nur den fast unsichtbaren
        // Rahmen zu sehen).
        auto trackArea = bounds.reduced (1.5f);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.fillRoundedRectangle (trackArea, 2.0f);

        const float lvl = juce::jlimit (0.0f, 1.0f, displayLevel);
        if (lvl > 0.01f)
        {
            auto fillArea = trackArea;
            fillArea = fillArea.removeFromLeft (fillArea.getWidth() * lvl);

            // Immer dieselbe Farbe (User: "in / out meter sollen immer nur die
            // Standardfarbe haben; keine Farbaenderung bei hoeherem Pegel").
            // Die Rot/Gelb-Stufen waren Clipping-Warnungen - aber die Skala
            // ist ohnehin dB-basiert mit -48 dB als Nullpunkt, ein voller
            // Balken heisst hier 0 dBFS und nicht "zu laut". Warnfarben
            // waeren also falsche Alarme.
            g.setColour (themePalette().knob);   // Meterfarbe je Theme (User)
            g.fillRoundedRectangle (fillArea, 2.0f);
        }

        // Etwas kraeftigerer Rahmen als der vorherige, fast unsichtbare
        // Versuch (0.12 -> 0.22), damit die Meter-Kontur klar erkennbar
        // bleibt, aber immer noch dezenter als die urspruengliche Version.
        g.setColour (juce::Colours::white.withAlpha (0.22f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 3.0f, 1.0f);
    }

private:
    void timerCallback() override
    {
        // Deutlich traeger/ruhiger als vorher (User-Wunsch: "smoother, nicht
        // so schnell") - sowohl der Anstieg (0.6 -> 0.22) als auch der
        // Abfall (x0.90 -> x0.93 pro Frame @30Hz) wurden verlangsamt, damit
        // die Anzeige nicht mehr bei jedem kurzen Peak nervoes zuckt.
        // ===== dB-Skalierung statt linearer Anzeige =====
        // User-Feedback: "In und out meter skalieren. Aktuell zu tief, auch
        // bei relativ hohem Pegel. Will mehr Balken sehen."
        // Ursache: der Processor legt den ROHEN linearen Block-Peak ab, und
        // der wurde bisher direkt als Balkenlaenge benutzt. Linear heisst
        // aber, dass ein Signal bei -12 dBFS - also ein voellig normaler,
        // gesunder Pegel - nur ein Viertel des Balkens fuellt. Deshalb sah
        // die Anzeige selbst bei lauten Signalen leer aus.
        // Pegelanzeigen skalieren deshalb IMMER in Dezibel. Mit -48 dBFS als
        // Nullpunkt fuellt -12 dBFS jetzt drei Viertel des Balkens, -6 dBFS
        // knapp neun Zehntel - das entspricht dem, was man von einem Meter
        // erwartet.
        const float rawLinear = juce::jlimit (0.0f, 1.5f, level.load (std::memory_order_relaxed));
        constexpr float kFloorDb = -48.0f;
        const float db  = juce::Decibels::gainToDecibels (rawLinear, kFloorDb);
        const float raw = juce::jlimit (0.0f, 1.0f, (db - kFloorDb) / -kFloorDb);
        // Nochmals traeger (User: "in / out meter sollen langsamer
        // reagieren"): Anstieg 0.22 -> 0.12, Abfall x0.93 -> x0.965 je Frame.
        // Das ist naeher an einem VU als an einem Peak-Meter - fuer eine
        // reine "wie laut ist es ungefaehr"-Anzeige das richtige Verhalten.
        if (raw > displayLevel)
            displayLevel += (raw - displayLevel) * 0.12f;
        else
            displayLevel *= 0.965f;

        repaint();
    }

    std::atomic<float>& level;
    float displayLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeterComponent)
};
