#pragma once
#ifndef SPACEX_PX_DIAG_ONLY
 #define SPACEX_PX_DIAG_ONLY 1
#endif
#include "GUI/CustomLookAndFeel.h"   // direkt aus SpaceX (../Source/GUI)

// Der feste Fairy-Tale-Look aus SpaceX fuer alle Einzel-Plugins.
// Gezeichnet wird mit SpaceX' eigenem CustomLookAndFeel (Theme "DarkNight",
// Layout Outline) - Regler, Icons, Knoepfe sehen dadurch exakt aus wie in
// SpaceX. Hier liegt nur, was in SpaceX im Editor selbst steckt: Platte,
// Sektionsrahmen, Titel, Auswahlpunkte, Meter-Knopf.
namespace fairy
{
    inline void useFairyTaleTheme()
    {
        uiThemeRef()  = UiTheme::DarkNight;
        uiLayoutRef() = 2;   // Outline
    }

    inline juce::Colour labelOn()  { return juce::Colour (0xffbec3cb); }
    inline juce::Colour labelOff() { return labelOffColour(); }
    inline juce::Colour titleOn()  { const auto p = themePalette(); return p.frameMain.brighter (0.25f).interpolatedWith (p.knob, 0.35f); }
    inline juce::Colour titleOff() { return sectionOffFill().interpolatedWith (titleOn(), 0.34f).brighter (0.22f).withAlpha (0.88f); }

    inline juce::Font paramFont() { return juce::Font (juce::FontOptions (12.0f, juce::Font::bold)).withExtraKerningFactor (0.06f); }
    inline juce::Font titleFont() { return juce::Font (juce::FontOptions (13.0f, juce::Font::bold)).withExtraKerningFactor (0.12f); }

    // Bilder mit Pixelarbeit immer als CPU-Bild (Windows/Direct2D, SpaceX Runde 189).
    inline juce::Image makeLayer (int w, int h)
    {
        return juce::Image (juce::Image::ARGB, juce::jmax (1, w), juce::jmax (1, h), true, juce::SoftwareImageType());
    }

    // ----- Hintergrund (SpaceX drawThemePlate, Zweig Dark Night) -----
    inline juce::Image makePlate (int w, int h)
    {
        auto img = makeLayer (w, h);
        {
            juce::Graphics pg (img);
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

    // ----- Sektionsrahmen (SpaceX drawGroup, Zweig layoutOutline) -----
    inline void drawSectionFrame (juce::Graphics& g, juce::Rectangle<int> r, bool on)
    {
        auto rf = r.toFloat().reduced (3.0f);
        const auto col = on ? themePalette().frameMain : juce::Colour (0xff545862);
        if (on)
        {
            const auto of = outlineFill();
            g.setColour (of.colour.withAlpha (of.alpha));
            g.fillRoundedRectangle (rf, 10.0f);
            juce::ColourGradient wash (col.withAlpha (0.055f), rf.getX() + rf.getWidth() * 0.15f, rf.getY(),
                                       col.withAlpha (0.0f), rf.getX() + rf.getWidth() * 0.9f, rf.getY(), true);
            g.setGradientFill (wash);
            g.fillRoundedRectangle (rf, 10.0f);
        }
        g.setColour (col.withAlpha (on ? 0.40f : 0.28f));
        g.drawRoundedRectangle (rf, 10.0f, on ? 1.4f : 1.0f);
    }

    // ----- LookAndFeel: SpaceX + zwei Ergaenzungen -----
    class LookAndFeel : public CustomLookAndFeel
    {
    public:
        LookAndFeel()
        {
            useFairyTaleTheme();
            const auto pal = themePalette();
            accent     = pal.knob;
            glowAccent = pal.mod;
            setColour (juce::Label::textColourId, labelOn());
        }

        // Gezeichnet wird die geglaettete Anzeige-Position (KnobGlide).
        void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                               float a0, float a1, juce::Slider& s) override
        {
            if (s.getProperties().contains ("displayPos"))
                pos = (float) s.getProperties()["displayPos"];
            CustomLookAndFeel::drawRotarySlider (g, x, y, w, h, pos, a0, a1, s);
        }

        // Ecke zum Skalieren: drei feine Striche im Rahmenton.
        void drawCornerResizer (juce::Graphics& g, int w, int h, bool over, bool dragging) override
        {
            g.setColour (themePalette().frameMain.withAlpha (over || dragging ? 0.65f : 0.30f));
            const float s = (float) juce::jmin (w, h);
            for (float f : { 0.30f, 0.55f, 0.80f })
                g.drawLine ((float) w - s * f, (float) h - 2.0f, (float) w - 2.0f, (float) h - s * f, 1.2f);
        }
    };

    // ----- Klickbarer Sektionsname = Ein/Aus (wie in SpaceX) -----
    class Title : public juce::Component
    {
    public:
        std::function<void()> onClick;
        juce::String text;
        bool on = true;

        int preferredWidth() const { return juce::roundToInt (juce::GlyphArrangement::getStringWidth (titleFont(), text)) + 10; }
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

    // ----- Auswahlpunkte unter einem Modusfeld (SpaceX ModeDots) -----
    class ModeDots : public juce::Component
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
                g.setColour (i == index ? themePalette().knob.withAlpha (off ? 0.35f : 1.0f) : juce::Colours::white.withAlpha (0.16f));
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
    // Raute (die Achsen des Goniometers) mit einer schmalen Linse in der
    // Mitte - so sieht ein breites Mono-Signal im Goniometer aus.
    class MeterToggle : public juce::Component
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
            const auto col = open ? themePalette().knob : iconOffColour();
            if (open || hot)
                CustomLookAndFeel::softIconGlow (g, c, s * 1.25f, col, open ? (hot ? 1.55f : 1.0f) : 0.7f);

            juce::Path d;
            d.startNewSubPath (c.x, c.y - s);
            d.lineTo (c.x + s, c.y);
            d.lineTo (c.x, c.y + s);
            d.lineTo (c.x - s, c.y);
            d.closeSubPath();
            g.setColour (col.withAlpha (open ? 0.75f : (hot ? 0.85f : 0.60f)));
            g.strokePath (d, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

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

    // ----- Weiches Nachgleiten der Regleranzeige -----
    // Laeuft im Takt des Monitors (VBlank) und fuehrt die angezeigte Position
    // mit ~35 ms dem echten Wert nach. Der Parameter selbst wird nicht
    // verzoegert. onFrame: weitere Anzeigen im selben Takt (EQ-Icon).
    class KnobGlide
    {
    public:
        explicit KnobGlide (juce::Component& owner) : vblank (&owner, [this] (double t) { tick (t); }) {}
        std::function<void()> onFrame;

        void add (juce::Slider& s)
        {
            sliders.push_back (&s);
            s.getProperties().set ("displayPos", (float) s.valueToProportionOfLength (s.getValue()));
        }
    private:
        void tick (double t)
        {
            const double dt = lastT > 0.0 ? juce::jlimit (0.0, 0.1, t - lastT) : 1.0 / 60.0;
            lastT = t;
            const float k = 1.0f - (float) std::exp (-dt / 0.035);
            for (auto* s : sliders)
            {
                const float target = (float) s->valueToProportionOfLength (s->getValue());
                float cur = (float) s->getProperties().getWithDefault ("displayPos", target);
                if (std::abs (target - cur) < 0.0005f)
                {
                    if (cur != target) { s->getProperties().set ("displayPos", target); s->repaint(); }
                    continue;
                }
                cur += (target - cur) * k;
                s->getProperties().set ("displayPos", cur);
                s->repaint();
            }
            if (onFrame) onFrame();
        }
        std::vector<juce::Slider*> sliders;
        double lastT = 0.0;
        juce::VBlankAttachment vblank;
    };
}
