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

    // Runde 150 (Footer-Variante 1): unter dem OUT-Meter eine feine dB-Skala.
    bool showScale = false;

    void paint (juce::Graphics& g) override
    {
        // Runde 150: schmaler Balken (5 px) mittig in einer 14-px-Zone, damit
        // der Schein an der Spitze Platz hat; rechts 6 px Luft fuer den Schein.
        auto full = getLocalBounds().toFloat();
        auto zone = full.withHeight (juce::jmin (full.getHeight(), 14.0f));
        auto track = zone.withTrimmedRight (6.0f).withSizeKeepingCentre (zone.getWidth() - 6.0f, 5.0f);
        track.setX (zone.getX());
        const float cr = track.getHeight() * 0.5f;
        const auto acc = themePalette().knob;

        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.fillRoundedRectangle (track, cr);

        const float lvl = juce::jlimit (0.0f, 1.0f, displayLevel);
        if (lvl > 0.01f)
        {
            auto fillArea = track.withWidth (juce::jmax (track.getHeight(), track.getWidth() * lvl));
            // Immer dieselbe Farbe (User) - nur von blass zur leuchtenden Spitze.
            juce::ColourGradient grad (acc.withAlpha (0.35f), fillArea.getX(), 0.0f,
                                       acc, fillArea.getRight(), 0.0f, false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (fillArea, cr);
            // Leuchtende Spitze
            const juce::Point<float> tip (fillArea.getRight() - cr, fillArea.getCentreY());
            const float gr = 7.0f;
            juce::ColourGradient glow (acc.withAlpha (0.18f + 0.30f * lvl), tip.x, tip.y,
                                       acc.withAlpha (0.0f), tip.x + gr, tip.y, true);
            g.setGradientFill (glow);
            g.fillEllipse (tip.x - gr, tip.y - gr, gr * 2.0f, gr * 2.0f);
        }
        // Runde 110: duenne Spitzenmarke (haelt kurz, sinkt dann langsam).
        if (peakHold > 0.02f)
        {
            const float px = track.getX() + track.getWidth() * juce::jlimit (0.0f, 1.0f, peakHold);
            g.setColour (acc.interpolatedWith (juce::Colours::white, 0.35f).withAlpha (0.85f));
            g.fillRoundedRectangle (px - 1.0f, track.getY() - 1.0f, 2.0f, track.getHeight() + 2.0f, 1.0f);
        }

        if (showScale && full.getHeight() > zone.getHeight() + 6.0f)
        {
            // Skala in dB (Nullpunkt -48, siehe timerCallback): -24, -12, -6, 0.
            static constexpr float marks[] = { -24.0f, -12.0f, -6.0f, 0.0f };
            const float ty = zone.getBottom() - 2.0f;
            g.setFont (juce::Font (juce::FontOptions (8.5f, juce::Font::bold)));
            for (float db : marks)
            {
                const float x = track.getX() + track.getWidth() * (db + 48.0f) / 48.0f;
                g.setColour (juce::Colours::white.withAlpha (0.16f));
                g.fillRect (x - 0.5f, ty, 1.0f, 3.0f);
                if (db == -6.0f) continue;   // zu eng neben der 0 - nur Strich
                g.setColour (juce::Colour (0xff6d7280));
                g.drawText (juce::String ((int) db), juce::Rectangle<float> (x - 12.0f, ty + 3.0f, 24.0f, 9.0f),
                            juce::Justification::centred, false);
            }
        }
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
        // Spitzenmarke: ~1,5 s halten, dann sanft nachsinken.
        if (displayLevel >= peakHold)
        {
            peakHold = displayLevel;
            peakHoldFrames = 45;
        }
        else if (peakHoldFrames > 0)
            --peakHoldFrames;
        else
            peakHold *= 0.97f;

        repaint();
    }

    std::atomic<float>& level;
    float displayLevel = 0.0f;
    float peakHold = 0.0f;       // Runde 110
    int   peakHoldFrames = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeterComponent)
};
