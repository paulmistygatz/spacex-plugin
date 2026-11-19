#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Der feste Fairy-Tale-Look aus SpaceX (Theme "DarkNight", Layout Outline),
// fuer alle Einzel-Module. Farben, Masse und Zeichenwege sind aus
// Source/GUI/CustomLookAndFeel.h und PluginEditor.cpp von SpaceX
// uebernommen - nur ohne Theme-Umschaltung.
namespace fairy
{
    // ----- Palette (themePalette() fuer DarkNight) -----
    inline const juce::Colour knob      { 0xffdabd76 };   // Reglergold
    inline const juce::Colour frameMain { 0xff8093b8 };
    inline const juce::Colour frameRaye { 0xffdabd76 };   // Modusname / Icons
    inline juce::Colour plate()         { return juce::Colour (0xff15171d).interpolatedWith (juce::Colours::white, 0.03f); }
    inline juce::Colour knobCap()       { return plate().interpolatedWith (juce::Colours::white, 0.10f); }
    inline juce::Colour ringOff()       { return plate().withMultipliedBrightness (1.15f).interpolatedWith (juce::Colours::white, 0.025f); }
    inline juce::Colour valueOff()      { return ringOff().interpolatedWith (juce::Colours::white, 0.08f); }
    inline juce::Colour centreOff()     { return plate().withMultipliedBrightness (0.85f); }
    inline juce::Colour labelOff()      { return plate().interpolatedWith (juce::Colours::white, 0.13f); }
    inline juce::Colour iconOff()       { return plate().interpolatedWith (juce::Colours::white, 0.30f); }
    inline const juce::Colour labelOn   { 0xffbec3cb };
    inline juce::Colour titleOn()       { return frameMain.brighter (0.25f).interpolatedWith (knob, 0.35f); }
    inline juce::Colour titleOff()      { return plate().interpolatedWith (titleOn(), 0.34f).brighter (0.22f).withAlpha (0.88f); }

    inline juce::Font paramFont()  { return juce::Font (juce::FontOptions (12.0f, juce::Font::bold)).withExtraKerningFactor (0.06f); }
    inline juce::Font titleFont()  { return juce::Font (juce::FontOptions (13.0f, juce::Font::bold)).withExtraKerningFactor (0.12f); }
    inline juce::Font modeFont (int h) { return juce::Font (juce::FontOptions (juce::jmin ((float) h * 0.40f, 13.0f), juce::Font::bold)).withExtraKerningFactor (0.10f); }

    // Bilder, auf denen Pixel bearbeitet werden, immer als CPU-Bild (unter
    // Windows sind native Bilder Direct2D-Bilder - siehe SpaceX Runde 189).
    inline juce::Image makeLayer (int w, int h)
    {
        return juce::Image (juce::Image::ARGB, juce::jmax (1, w), juce::jmax (1, h), true, juce::SoftwareImageType());
    }

    inline void softGlow (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour col, float strength)
    {
        if (r <= 1.0f) return;
        juce::ColourGradient grad (col.withAlpha (juce::jmin (0.60f, 0.26f * strength)), c.x, c.y,
                                   col.withAlpha (0.0f), c.x + r, c.y, true);
        grad.addColour (0.45, col.withAlpha (juce::jmin (0.35f, 0.12f * strength)));
        g.setGradientFill (grad);
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    }

    // ----- Hintergrund (drawThemePlate, Zweig isWaterTheme / Dark Night) -----
    inline juce::Image makePlate (int w, int h, float corner)
    {
        auto img = makeLayer (w, h);
        {
            juce::Graphics pg (img);
            juce::Path clip; clip.addRoundedRectangle (0.0f, 0.0f, (float) w, (float) h, corner);
            pg.reduceClipRegion (clip);
            pg.setColour (juce::Colour (0xff0c0e15));
            pg.fillAll();
            auto blob = [&] (float fx, float fy, float fr, juce::Colour c, float a)
            {
                juce::ColourGradient grad (c.withAlpha (a), fx * (float) w, fy * (float) h,
                                           c.withAlpha (0.0f), fx * (float) w + fr * (float) w, fy * (float) h, true);
                pg.setGradientFill (grad);
                pg.fillAll();
            };
            constexpr float bm = 0.55f;
            blob (0.86f, 0.12f, 0.50f, juce::Colour (0xff506edc), 0.11f * bm);
            blob (0.74f, 0.88f, 0.46f, juce::Colour (0xffbe6ec8), 0.08f * bm);
            blob (0.18f, 0.55f, 0.42f, juce::Colour (0xff50a0c8), 0.07f * bm);
            blob (0.30f, 0.05f, 0.30f, juce::Colour (0xff8a78d8), 0.06f * bm);
            juce::ColourGradient shade (juce::Colours::black.withAlpha (0.0f), (float) w * 0.42f, 0.0f,
                                        juce::Colours::black.withAlpha (0.42f), (float) w, 0.0f, false);
            pg.setGradientFill (shade);
            pg.fillAll();
        }
        // Feines Korn
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::readWrite);
        juce::Random rnd (1234);
        for (int y = 0; y < bd.height; ++y)
            for (int x = 0; x < bd.width; ++x)
            {
                auto* px = bd.getPixelPointer (x, y);
                if (px[3] == 0) continue;
                const int n = rnd.nextInt (9) - 4;
                for (int c = 0; c < 3; ++c)
                    px[c] = (juce::uint8) juce::jlimit (0, (int) px[3], (int) px[c] + n);
            }
        return img;
    }

    // ----- Sektionsrahmen (drawGroup, Zweig layoutOutline) -----
    inline void drawSectionFrame (juce::Graphics& g, juce::Rectangle<int> r, bool on)
    {
        auto rf = r.toFloat().reduced (3.0f);
        const auto col = on ? frameMain : juce::Colour (0xff545862);
        if (on)
        {
            g.setColour (juce::Colour (0xff13152a).withAlpha (0.40f));
            g.fillRoundedRectangle (rf, 10.0f);
            juce::ColourGradient wash (col.withAlpha (0.055f), rf.getX() + rf.getWidth() * 0.15f, rf.getY(),
                                       col.withAlpha (0.0f), rf.getX() + rf.getWidth() * 0.9f, rf.getY(), true);
            g.setGradientFill (wash);
            g.fillRoundedRectangle (rf, 10.0f);
        }
        g.setColour (col.withAlpha (on ? 0.40f : 0.28f));
        g.drawRoundedRectangle (rf, 10.0f, on ? 1.4f : 1.0f);
    }

    // ----- Drehregler (drawRotarySlider, ohne Sonderfaelle) -----
    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel()
        {
            setColour (juce::Label::textColourId, labelOn);
            setColour (juce::TooltipWindow::textColourId, juce::Colour (0xffd8dbe2));
        }

        void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                               float sliderPos, float startAngle, float endAngle, juce::Slider& slider) override
        {
            auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
            const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
            const auto centre = bounds.getCentre();
            const bool off = (bool) slider.getProperties().getWithDefault ("sectionOff", false) || ! slider.isEnabled();

            if (slider.isMouseOverOrDragging())
            {
                const float hr = radius * 0.98f;
                g.setColour (juce::Colours::white.withAlpha (0.035f));
                g.fillEllipse (centre.x - hr, centre.y - hr, hr * 2.0f, hr * 2.0f);
            }

            const float angle = startAngle + sliderPos * (endAngle - startAngle);
            const float track = radius * 0.16f;
            const float tr = radius - track;

            juce::Path ring;
            ring.addCentredArc (centre.x, centre.y, tr, tr, 0.0f, startAngle, endAngle, true);
            if (off)
                g.setColour (ringOff());
            else
            {
                const juce::Colour ringOn (0xff454952);
                const juce::Point<float> pS (centre.x + tr * std::sin (startAngle), centre.y - tr * std::cos (startAngle));
                const juce::Point<float> pE (centre.x + tr * std::sin (endAngle),   centre.y - tr * std::cos (endAngle));
                g.setGradientFill (juce::ColourGradient (ringOn.interpolatedWith (knob, 0.19f), pS, ringOn, pE, false));
            }
            g.strokePath (ring, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            juce::Path value;
            if (angle - startAngle > 0.001f)
                value.addCentredArc (centre.x, centre.y, tr, tr, 0.0f, startAngle, angle, true);
            const auto col = off ? valueOff() : knob;
            g.setColour (col);
            g.strokePath (value, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            if (! off && ! value.isEmpty())
                softGlow (g, { centre.x + tr * std::sin (angle), centre.y - tr * std::cos (angle) }, track * 2.2f, col, 0.9f);

            const float pl = radius * 0.55f;
            juce::Path pointer;
            pointer.startNewSubPath (centre.x, centre.y);
            pointer.lineTo (centre.x + pl * std::sin (angle), centre.y - pl * std::cos (angle));
            g.setColour (juce::Colours::white.withAlpha (off ? 0.30f : 1.0f));
            g.strokePath (pointer, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            g.setColour (off ? centreOff() : knobCap());
            g.fillEllipse (centre.x - radius * 0.28f, centre.y - radius * 0.28f, radius * 0.56f, radius * 0.56f);
        }

        void drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height) override
        {
            auto r = juce::Rectangle<float> (0, 0, (float) width, (float) height);
            g.setColour (juce::Colour (0xf0101220));
            g.fillRoundedRectangle (r, 5.0f);
            g.setColour (frameMain.withAlpha (0.45f));
            g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
            g.setColour (findColour (juce::TooltipWindow::textColourId));
            g.setFont (juce::Font (juce::FontOptions (12.0f)));
            g.drawFittedText (text, r.reduced (8.0f, 4.0f).toNearestInt(), juce::Justification::centredLeft, 3);
        }

        juce::Rectangle<int> getTooltipBounds (const juce::String& text, juce::Point<int> pos, juce::Rectangle<int> parent) override
        {
            const auto f = juce::Font (juce::FontOptions (12.0f));
            const int w = juce::jmin (300, juce::roundToInt (juce::GlyphArrangement::getStringWidth (f, text)) + 20);
            const int h = 24;
            int x = pos.x - w / 2, y = pos.y + 18;
            if (y + h > parent.getBottom()) y = pos.y - h - 8;
            x = juce::jlimit (parent.getX(), juce::jmax (parent.getX(), parent.getRight() - w), x);
            return { x, y, w, h };
        }
    };

    // ----- Klickbarer Sektionsname = Ein/Aus (wie in SpaceX) -----
    class Title : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        std::function<void()> onClick;
        juce::String text;
        bool on = true;

        void paint (juce::Graphics& g) override
        {
            auto c = on ? titleOn() : titleOff();
            if (isMouseOver()) c = c.brighter (0.40f).withAlpha (1.0f);
            g.setColour (c);
            g.setFont (titleFont());
            g.drawText (text, getLocalBounds(), juce::Justification::centredLeft, false);
        }
        void mouseEnter (const juce::MouseEvent&) override { repaint(); }
        void mouseExit  (const juce::MouseEvent&) override { repaint(); }
        void mouseUp (const juce::MouseEvent& e) override
        {
            if (getLocalBounds().contains (e.getPosition()) && onClick) onClick();
        }
    };

    // ----- Auswahlpunkte unter einem Modusfeld (ModeDots) -----
    class ModeDots : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        int count = 4, index = 0;
        bool off = false;
        std::function<void (int)> onPick;
        static constexpr float kPitch = 10.0f;

        float centreOf (int i) const { return ((float) getWidth() - kPitch * (float) count) * 0.5f + kPitch * 0.5f + kPitch * (float) i; }
        void paint (juce::Graphics& g) override
        {
            const float d = 4.0f, cy = (float) getHeight() * 0.5f;
            for (int i = 0; i < count; ++i)
            {
                g.setColour (i == index ? knob.withAlpha (off ? 0.35f : 1.0f) : juce::Colours::white.withAlpha (0.16f));
                g.fillEllipse (centreOf (i) - d * 0.5f, cy - d * 0.5f, d, d);
            }
        }
        void mouseDown (const juce::MouseEvent& e) override
        {
            int best = 0; float bestD = 1.0e9f;
            for (int i = 0; i < count; ++i)
                if (const float dx = std::abs ((float) e.x - centreOf (i)); dx < bestD) { bestD = dx; best = i; }
            if (onPick) onPick (best);
        }
    };

    // ----- Knopf fuer das Goniometer (auf-/zuklappen) -----
    // Zeichnung: eine kleine Raute (die Achsen des Goniometers) mit einer
    // schmalen Punktwolke in der Mitte. Gold, wenn das Meter offen ist.
    class MeterToggle : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        bool open = false;
        std::function<void()> onClick;

        void paint (juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            const auto c = b.getCentre();
            const float s = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 2.0f;
            const bool hot = isMouseOver();
            const auto col = open ? knob : iconOff();
            if (open || hot)
                softGlow (g, c, s * 1.25f, col, open ? (hot ? 1.55f : 1.0f) : 0.7f);

            juce::Path d;
            d.startNewSubPath (c.x, c.y - s);
            d.lineTo (c.x + s, c.y);
            d.lineTo (c.x, c.y + s);
            d.lineTo (c.x - s, c.y);
            d.closeSubPath();
            g.setColour (col.withAlpha (open ? 0.75f : (hot ? 0.85f : 0.60f)));
            g.strokePath (d, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Die Wolke: eine schmale, senkrechte Linse (so sieht ein
            // breites Mono-Signal im Goniometer aus), darin die Mitte.
            const float lw = s * 0.34f, lh = s * 1.10f;
            g.setColour (col.withAlpha (open ? 0.42f : (hot ? 0.40f : 0.26f)));
            g.fillEllipse (c.x - lw * 0.5f, c.y - lh * 0.5f, lw, lh);
            g.setColour (col.withAlpha (open ? 1.0f : (hot ? 0.95f : 0.70f)));
            g.fillEllipse (c.x - 1.6f, c.y - 1.6f, 3.2f, 3.2f);
        }
        void mouseEnter (const juce::MouseEvent&) override { repaint(); }
        void mouseExit  (const juce::MouseEvent&) override { repaint(); }
        void mouseUp (const juce::MouseEvent& e) override
        {
            if (getLocalBounds().contains (e.getPosition()) && onClick) onClick();
        }
    };
}
