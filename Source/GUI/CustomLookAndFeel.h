#pragma once
#include <JuceHeader.h>

// Modernes, reduziertes LookAndFeel: dunkler Hintergrund, flache Regler,
// leuchtende Akzente (angelehnt an Nuro Audio). Bewusst simpel gehalten,
// low CPU beim Zeichnen.
// ===== THEMES (User: "alle 3 Themes") =====
// 0 Modern (bisheriger Look), 1 Watercolor (weiche Flecken, Korn, Nebula-
// Toene), 2 Comic (Konturen, Versatz-Schatten, Rasterpunkte). Layout, Schrift,
// Kopf- und Fusszeile bleiben in allen Themes gleich - nur das "Material"
// von Platte, Sektionen, Reglern und Knoepfen wechselt.
// Themes (User, Runde 21). Die Enum-Werte bleiben aus Kompatibilitaets-
// gruenden stehen, auch wenn Namen wechseln:
//   0 UiTheme::Moon      = Menue "Moon"        (frueher "Modern")
//   1 UiTheme::DarkNight = Menue "Dark Night"
//   2 UiTheme::Comic     = Menue "Pop"
//   3 UiTheme::SciFi     = Menue "Sci-Fi"      - traegt jetzt die Palette der
//                          frueheren Variante "Sci-Fi Frame" (violette Rahmen,
//                          pinke Titel). Die beiden anderen Varianten sind raus.
//   4 UiTheme::DayNight  = Menue "Day & Night"
//   5 UiTheme::Flat      = Menue "Flat"        (frueher "Moon")
//   6 UiTheme::SciFiDark = Menue "Sci-Fi Dark" - wie Sci-Fi, aber die Sektionen
//                          haben KEINE eigene Fuellfarbe: nur Rahmen, innen die
//                          Plattenfarbe (User-Idee).
// Menue-Reihenfolge: Flat, Moon, Day & Night, Dark Night, Sci-Fi, Sci-Fi Dark, Pop.
enum class UiTheme { Moon = 0, DarkNight = 1, Comic = 2, SciFi = 3, DayNight = 4, Flat = 5,
                     SciFiDark = 6 };
constexpr int kUiThemeCount = 7;
inline UiTheme& uiThemeRef()   { static UiTheme t = UiTheme::SciFi; return t; }   // Sci-Fi = Standard beim Oeffnen (User)
inline bool     isDarkNightTheme() { return uiThemeRef() == UiTheme::DarkNight; }
inline bool     isWaterTheme() { return isDarkNightTheme(); }   // Watercolor-Material (Nebula-Platte, Korn)
inline bool     isComicTheme() { return uiThemeRef() == UiTheme::Comic; }
inline bool     isDayNightTheme() { return uiThemeRef() == UiTheme::DayNight; }
inline bool     isFlatTheme()  { return uiThemeRef() == UiTheme::Flat; }
inline bool     isMoonTheme() { return uiThemeRef() == UiTheme::Moon; }
// Sci-Fi gibt es in drei Varianten, die sich nur in Rahmen-/Titelfarbe
// unterscheiden - alles andere (Platte, Neon-Objekte, Sonne) ist identisch,
// deshalb fragt der ganze Zeichencode weiterhin nur isSciFiTheme() ab.
inline bool     isSciFiFamily()   { const auto t = uiThemeRef(); return t == UiTheme::SciFi || t == UiTheme::SciFiDark; }
inline bool     isSciFiTheme()    { return isSciFiFamily(); }
// Sci-Fi Dark: Sektionen ohne eigene Fuellfarbe (nur Rahmen + Plattenfarbe).
inline bool     isSciFiDarkTheme() { return uiThemeRef() == UiTheme::SciFiDark; }
inline bool     usesDrawnSky()  { return isMoonTheme() || isDayNightTheme() || isFlatTheme(); }   // kein Foto, flache Platte
// Sektionsstil "Wash" (aus Dark Night, User: "das Geile von Dark Night auf
// Modern uebertragen"): sehr dunkle Flaeche, weicher Farbhauch, 1-px-Schimmer.
// Sci-Fi und Moon behalten die gefuellte Flaeche mit feinem Rahmen.
inline bool     usesWashSections() { return isWaterTheme() || isMoonTheme() || isDayNightTheme(); }
inline bool     usesFlatPanels()     { return isSciFiTheme() || isFlatTheme(); }
// Layout-Modi (Menue "Layout"), global wie das Theme, kein Preset-Bestandteil.
//   0 "3D"      - der bisherige Look mit Kasten und Fuellung (Standard)
//   1 "Flat"     - keine Kaesten; die Sektionen werden nur noch durch einen
//                  sehr leichten Helligkeitsversatz getrennt (User: "geht jetzt
//                  bei allen Themes wunderbar")
//   2 "Outline"  - die Umkehrung davon (User-Idee): ein leichter Rahmen, aber
//                  praktisch keine Fuellung - nur rund 2 % Unterschied zur UI
// Der frueher hier eingebaute "Simple"-Modus ist wieder raus (User: "macht
// optisch schonmal gar keinen Sinn"). Pop hat keine Layout-Varianten - dort
// traegt der Kasten den ganzen Look.
inline int&  uiLayoutRef()      { static int m = 0; return m; }
inline bool  layoutFrameless()  { return uiLayoutRef() == 1 && ! isComicTheme(); }
inline bool  layoutOutline()    { return uiLayoutRef() == 2 && ! isComicTheme(); }
inline juce::Colour comicInk() { return juce::Colour (0xff0d0a1e); }
// Aus-Zustand der Icons (Power/Solo/Lock/Mod/...): im Comic dunkle Tinte,
// bei Watercolor etwas heller, sonst das bisherige Grau (User: "Icons wenn
// off nicht gut zu erkennen").


// Ein Bedienelement, das EINGESCHALTET ist, waehrend seine Sektion AUS ist:
// der Zustand soll erkennbar bleiben, aber nicht leuchten (User: "ich will
// zwar sehen, dass ein Icon an oder aus ist, aber aktive Icons in
// deaktivierten Sections sollen nicht stark leuchten oder zu hell sein - der
// Unterschied soll eher minimal sein"). Deshalb: derselbe Grundton wie der
// Aus-Zustand, nur ein Hauch heller.

// ===== FARBSYSTEM JE THEME (User: "zu viele Farben ... ein einheitlicher Look") =====
// Eine Akzentfamilie pro Theme: Reglerwerte, Mod-Punkte, Sektionsrahmen,
// PRISM, Mutate-Kategorien und die RAYE-Linien im Sternenfeld ziehen an
// einem Strang. Pop (Comic) behaelt seine bunten Sektionen bewusst.
// "mod" (Mod-Punkte auf den Reglern) muss sich klar vom Reglerring (knob)
// abheben (User: "weiss auf weiss"). Galaxy hat keinen eigenen Rahmen-/
// Titelton mehr (User: "hat sowieso einen Glow") - frameGalaxy nur noch Pop.
struct ThemePalette
{
    juce::Colour knob, mod, frameMain, frameGalaxy, frameRaye, prism, chip, plate;
    // Sektionstitel. Frueher immer = frameMain; seit den Sci-Fi-Varianten
    // eigenstaendig, damit Rahmen und Titel getrennt eingefaerbt werden
    // koennen. 0 = "nimm frameMain" (alle Themes ausser den Varianten).
    juce::Colour title { juce::Colour (0x00000000) };
    juce::Colour titleColour() const { return title.getAlpha() == 0 ? frameMain : title; }
};
inline ThemePalette themePalette()
{
    switch (uiThemeRef())
    {
        case UiTheme::DarkNight:    // Dark Night: sehr dunkles Lila + Gold (User: "sieht so geil aus"), 5 % heller als zuvor
            return { juce::Colour (0xffdabd76), juce::Colour (0xff7fb0ff), juce::Colour (0xff8093b8), juce::Colour (0xff6090cc),
                     juce::Colour (0xffdabd76), juce::Colour (0xffdabd76), juce::Colour (0xffdabd76), juce::Colour (0xff15171d) };
        // Sci-Fi (User-Entscheidung Runde 21): violette Rahmen, pinke Titel -
        // die Rahmen sind die groesste Farbflaeche, dort stoert das Pink am
        // meisten. RAYE behaelt Pink und hebt sich dadurch von selbst ab.
        // Runde 28: "Sci-Fi" und "Sci-Fi Dark" sind EIN Theme geworden (User) -
        // der Unterschied war nur die Plattenfarbe und ob die Sektionen eine
        // Fuellung haben. Die Platte liegt jetzt genau zwischen den beiden
        // (120f1b / 171321 -> 14111e), die Fuellung bei halber Deckkraft.
        case UiTheme::SciFi:
        case UiTheme::SciFiDark:
            return { juce::Colour (0xff8be9ff), juce::Colour (0xffe07aa8), juce::Colour (0xff7d63c9), juce::Colour (0xff7d63c9),
                     juce::Colour (0xffd9689a), juce::Colour (0xff8e70c6), juce::Colour (0xff8e70c6), juce::Colour (0xff14111e),
                     juce::Colour (0xffd9689a) };
        case UiTheme::DayNight:     // Day & Night: Reglergold (User: "sehr gut"), Mods Himmelblau, Rahmen warmes Grau, Platte neutral-dunkel (nicht braun)
            return { juce::Colour (0xffe0b98a), juce::Colour (0xff6fc3ff), juce::Colour (0xff8a8474), juce::Colour (0xff8a8474),
                     juce::Colour (0xffe2c37a), juce::Colour (0xffd9b283), juce::Colour (0xffd9b283), juce::Colour (0xff141518) };
        case UiTheme::Flat:         // Flat: kuehles Silber mit einem Hauch Grau-Blau
            // User (Runde 21): "insgesamt minimal mehr Farbe, aber nur 5 %" -
            // Rahmen und Regler ziehen leicht ins Grau-Blau, die Mods bleiben
            // als warmer Gegenpol stehen.
            // Kontrast bewusst zurueckgenommen (User: "manche Themes sind
            // unangenehm fuer die Augen ... sehr helle weisse Schrift auf zu
            // dunklem Hintergrund"). Die Schrift geht ein Stueck vom Weiss weg
            // UND die Platte einen Hauch hoch - beides zusammen nimmt dem
            // Sprung die Haerte, ohne dass das Theme seinen Charakter verliert.
            return { juce::Colour (0xffc3cbd8), juce::Colour (0xffc09f66), juce::Colour (0xff798396), juce::Colour (0xff798396),
                     juce::Colour (0xffd6c89c), juce::Colour (0xffadb5c2), juce::Colour (0xffadb5c2), juce::Colour (0xff1c1e24) };
        case UiTheme::Comic:        // Pop: bisherige Farben
            return { juce::Colour (0xff5be3c7), juce::Colour (0xffb968ff), juce::Colour (0xffb968ff), juce::Colour (0xff4fa8ff),
                     juce::Colour (0xffffc247), juce::Colour (0xffb968ff), juce::Colour (0xff5be3c7), juce::Colour (0xff17191f) };
        default:                    // Modern: graeulich-dunkel-warm (User: "zu blaeulich"), Mods Blau, Gold nur bei RAYE
            return { juce::Colour (0xffc9c5be), juce::Colour (0xff97b8ee), juce::Colour (0xff86847e), juce::Colour (0xff86847e),
                     juce::Colour (0xffd6b975), juce::Colour (0xffc9c5be), juce::Colour (0xffc9c5be), juce::Colour (0xff1a1a1d) };
    }
}

// Farbe der RAYE-Pair-Kopplung (Pair-Knopf + gekoppelte Hyperdrive-Teile).
// Sci-Fi (User, Runde 38): die blaue Akzentfarbe - sonst war Pair genau so
// pink wie Sync in Hyperdrive und nicht zu unterscheiden.
inline juce::Colour pairAccentColour()
{
    if (isSciFiTheme())
        return themePalette().knob;
    return themePalette().frameRaye.withMultipliedSaturation (isWaterTheme() ? 0.6f : 1.0f);
}
inline juce::Colour iconOffColour()
{
    // EIN einziges "grayed out" fuer alle Themes (User: "einheitliches grayed
    // out"). Frueher hatte jede Theme-Familie ihren eigenen Grauton - genau
    // das war die Uneinheitlichkeit. Jetzt wird der Ton aus der Plattenfarbe
    // abgeleitet: derselbe Abstand zur UI in jedem Theme, also ueberall
    // derselbe Eindruck, ohne dass ein kalter Grauton in einem warmen Theme
    // (oder umgekehrt) als Fremdkoerper sitzt.
    return isComicTheme() ? juce::Colour (0xff7a7496)
                          : themePalette().plate.interpolatedWith (juce::Colours::white, 0.30f);
}

inline juce::Colour iconOnInOffSection() { return iconOffColour().interpolatedWith (juce::Colours::white, 0.16f); }
// EINE Farbe fuer alle kleinen Zwei-Zustand-Knoepfe zwischen den Reglern
// (3x Focus, Balance in Timewarp, RAYE-Stufe). Vorher hatte jeder seine
// eigene - Balance nahm den Akzent, Focus den Reglerton - und in Pop fiel
// genau das auf (User: "das Icon in den Focus-Knoepfen hat eine andere Farbe
// als das Center-Icon bei Timewarp"). Bewusst zurueckhaltend: diese Knoepfe
// sollen erkennbar sein, nicht leuchten.
inline juce::Colour smallIconColour (bool lit)
{
    return lit ? themePalette().knob.interpolatedWith (juce::Colour (0xffe8ecf3), 0.30f).withAlpha (0.86f)
               : iconOffColour().withAlpha (0.55f);
}
// Flaeche eines EINGESCHALTETEN Bedienelements (Knopf, RAYE-Kachel, Bars-Feld).
// Eine Quelle fuer alle - vorher hatte jedes seinen eigenen festen Hex-Wert,
// der in keinem Theme passte (User: "grau passt einfach nicht mehr").
inline juce::Colour controlOnFill()  { return isComicTheme() ? themePalette().chip.withMultipliedSaturation (0.7f).darker (0.15f)
                                                              : themePalette().plate.interpolatedWith (juce::Colour (0xff2a1f33), 0.5f); }
// Neutrale Flaeche fuer Felder, die kein An/Aus kennen (Bars).
inline juce::Colour controlIdleFill(){ return isComicTheme() ? juce::Colour (0xff3a3266)   // Pop: derselbe Ton wie ein ausgeschalteter Pop-Knopf
                                                              : themePalette().plate.interpolatedWith (juce::Colours::white, 0.045f); }

// Sektionsflaeche der flachen Themes (Sci-Fi, Moon); wird mit 62 % (an)
// bzw. 30 % (aus) ueber die Platte gelegt.
inline juce::Colour themeSurface()
{
    switch (uiThemeRef())
    {
        case UiTheme::SciFi:
        case UiTheme::SciFiDark:    return juce::Colour (0xff1b1630);   // dunkler (User: "zu hell")
        case UiTheme::Flat:         return juce::Colour (0xff222327);
        default:                    return juce::Colour (0xff1e2434);
    }
}
// "Wash"-Sektionen: Fuellfarbe + Deckkraft fuer an / aus je Theme.
struct WashFill { juce::Colour on, off; float aOn, aOff; };
inline WashFill washFill()
{
    switch (uiThemeRef())
    {
        // Aus-Zustand nochmal deutlich dunkler (User: "noch dunkler machen
        // wenn section=off - einfacher geht's nicht").
        // Die Off-Werte werden seit Runde 21 nicht mehr benutzt (Aus-Sektionen
        // sind transparent, siehe sectionOffFill) - sie bleiben nur der
        // Vollstaendigkeit halber stehen. Die An-Deckkraft ist wieder etwas
        // geringer, damit die Platte durchkommt (User: "Dark Night ist zu
        // dunkel geworden insgesamt").
        case UiTheme::Moon:   return { juce::Colour (0xff1c1c20), juce::Colour (0xff060607), 0.62f, 0.0f };   // warmes Grau
        case UiTheme::DayNight: return { juce::Colour (0xff141826), juce::Colour (0xff05060b), 0.62f, 0.0f };   // tiefes Navy + Gold
        default:                return { juce::Colour (0xff13152a), juce::Colour (0xff07080f), 0.56f, 0.0f };   // Dark Night
    }
}
// Effektive Fuellfarbe einer AUSGESCHALTETEN Sektion (Platte + Flaeche) -
// Bezugsfarbe fuer Reglerringe, Reglermitte und Labels bei Sektion aus
// (User: "fast genauso dunkel wie die Off-Fuellfarbe").
inline juce::Colour sectionOffFill()
{
    // User (Runde 21, die entscheidende Beobachtung): "besonders elegant sieht
    // es aus, wenn Sektionen, die off sind, EXAKT dieselbe Farbe haben wie die
    // UI an der Stelle - also 100 % Transparenz der Fuellung, evtl. ein
    // minimaler Rand." Damit loest sich auch das alte Problem, dass die
    // Off-Farbe in manchen Themes nie passen wollte: es gibt keine eigene
    // Off-Farbe mehr, nur noch die Platte. Pop behaelt seinen Comic-Look.
    if (isComicTheme()) return juce::Colour (0xff14112a);
    return themePalette().plate;
}
inline juce::Colour knobRingOffColour()  { return sectionOffFill().withMultipliedBrightness (1.15f).interpolatedWith (juce::Colours::white, 0.025f); }   // noch naeher an der Flaeche (User)
inline juce::Colour knobValueOffColour() { return knobRingOffColour().interpolatedWith (juce::Colours::white, 0.08f); }
inline juce::Colour knobCentreOffColour(){ return sectionOffFill().withMultipliedBrightness (0.85f); }
// Schrift bei ausgeschalteter Sektion: noch naeher an die UI-Farbe heran
// (User, Runde 22: "Schrift der Regler und Buttons Richtung UI-Hintergrund").
inline juce::Colour labelOffColour()     { return sectionOffFill().interpolatedWith (juce::Colours::white, 0.13f); }
// Comic: Versatz-Schatten (vor dem Element zeichnen) und Kontur (danach).
inline void comicShadow  (juce::Graphics& g, juce::Rectangle<float> b, float corner, float off = 2.5f)
{
    if (! isComicTheme()) return;
    g.setColour (comicInk());
    g.fillRoundedRectangle (b.translated (off, off), corner);
}
inline void comicOutline (juce::Graphics& g, juce::Rectangle<float> b, float corner, float w = 2.2f)
{
    if (! isComicTheme()) return;
    g.setColour (comicInk());
    g.drawRoundedRectangle (b, corner, w);
}

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
        // Ganz subtiles Hover-Feedback auch auf den Reglern (User: "so 3 %,
        // einfach nur ganz subtil") - liegt HINTER dem Regler, hebt ihn also
        // nur leicht vom Hintergrund ab, statt ihn zu ueberdecken.
        if (slider.isMouseOverOrDragging())
        {
            // Bug (User: "Hover erzeugt ein Achteck"): der Kreis war GROESSER
            // als die Komponente und wurde an deren rechteckigen Kanten
            // abgeschnitten - aus dem Kreis wurde ein Vieleck. Jetzt bleibt er
            // innerhalb des Reglers.
            const float hr = radius * 0.98f;
            g.setColour (juce::Colours::white.withAlpha (0.035f));
            g.fillEllipse (centre.x - hr, centre.y - hr, hr * 2.0f, hr * 2.0f);
        }
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Regler bleiben bei ausgeschalteter Section weiterhin bedienbar
        // (siehe PluginEditor::timerCallback), sollen dann aber NICHT mehr
        // farbig/leuchtend wirken - die Component-Property "sectionOff"
        // erzwingt hier die neutrale/graue Darstellung, unabhaengig vom
        // technischen isEnabled()-Status.
        const bool offVisual = slider.getProperties().getWithDefault ("sectionOff", false) || ! slider.isEnabled();

        float trackThickness = radius * 0.18f;

        // Comic: Scheibe mit Kontur und Versatz-Schatten unter dem Ring.
        if (isComicTheme())
        {
            // Section aus: deutlich grau, ohne Schatten (User: "viel
            // deutlicher sichtbar, wenn die Section off ist").
            const float rr = radius + 1.5f;
            if (! offVisual)
            {
                g.setColour (comicInk());
                g.fillEllipse (centre.x - rr + 3.0f, centre.y - rr + 3.0f, rr * 2.0f, rr * 2.0f);
            }
            g.setColour (offVisual ? sectionOffFill() : juce::Colour (0xff3a3266));
            g.fillEllipse (centre.x - rr, centre.y - rr, rr * 2.0f, rr * 2.0f);
            g.setColour (offVisual ? knobRingOffColour() : comicInk());
            g.drawEllipse (centre.x - rr, centre.y - rr, rr * 2.0f, rr * 2.0f, 2.4f);
        }

        juce::Path track;
        track.addCentredArc (centre.x, centre.y, radius - trackThickness, radius - trackThickness,
                              0.0f, rotaryStartAngle, rotaryEndAngle, true);
        // Etwas heller als die getönten Gruppenrahmen dahinter, damit der
        // unlackierte Ring nicht optisch verschwindet.
        g.setColour (offVisual ? knobRingOffColour()   // aus: kaum heller als die Off-Fuellfarbe der Sektion (User, alle Themes)
                   : isComicTheme() ? juce::Colour (0xff5a5390)
                   : isSciFiTheme() ? juce::Colour (0xff3a2f5a)   // Sci-Fi an: dunkles Violett statt Grau (User, Option A)
                                    : juce::Colour (0xff454952));
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
        else if (angle - rotaryStartAngle > 0.001f)
        {
            // Runde 46 (User): bei 0 KEIN Bogen - die runde Linienkappe
            // zeichnete sonst einen leuchtenden Punkt, der wie "1 %" aussah.
            value.addCentredArc (centre.x, centre.y, radius - trackThickness, radius - trackThickness,
                                  0.0f, rotaryStartAngle, angle, true);
        }
        auto col = offVisual ? knobValueOffColour() : accent;
        if (! offVisual && slider.getProperties().getWithDefault ("footerKnob", false))
        {
            // Footer (MIX/VOL): je Theme etwas zurueckgenommen bzw. eigener Ton (User)
            switch (uiThemeRef())
            {
                case UiTheme::DarkNight:    col = juce::Colour (0xffbea672); break;
                case UiTheme::SciFi:
                case UiTheme::SciFiDark:    col = juce::Colour (0xffa87ce0); break;
                case UiTheme::Flat:         col = juce::Colour (0xffb6c2d2); break;   // Footer mit demselben Hauch Blau (User)
                default: break;
            }
        }
        g.setColour (col);
        g.strokePath (value, juce::PathStrokeType (trackThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        float pointerLength = radius * 0.55f;
        juce::Path pointer;
        pointer.startNewSubPath (centre.x, centre.y);
        pointer.lineTo (centre.x + pointerLength * std::sin (angle), centre.y - pointerLength * std::cos (angle));
        g.setColour (juce::Colours::white.withAlpha (offVisual ? 0.30f : 1.0f));
        g.strokePath (pointer, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour (offVisual ? knobCentreOffColour() : isWaterTheme() ? juce::Colour (0xff2b303f) : juce::Colour (0xff1c1e23));
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
            // Runde 47 (User-Bug): auf dem eigenen Wertbogen verschwand der
            // Punkt, weil beide fast dieselbe Farbe haben - dunkler Ring
            // darum, dann bleibt er ueberall sichtbar.
            g.setColour (juce::Colour (0xcc0a0b0e));
            g.fillEllipse (dx - 4.6f, dy - 4.6f, 9.2f, 9.2f);
            g.setColour (glowAccent);
            g.fillEllipse (dx - 3.0f, dy - 3.0f, 6.0f, 6.0f);
        }

        // ===== RAYE-PAIR-RING (ganz zum Schluss) =====
        // Ring um den Hyperdrive-Speed-Regler, solange er das Tempo fuer RAYE
        // liefert. Zwei Buglagen aus der Praxis, beide hier geloest:
        //  1. Er wurde frueher VOR der Pop-Scheibe gezeichnet und von ihr
        //     ueberdeckt. Deshalb steht er jetzt am ENDE der Funktion - nach
        //     Scheibe, Track, Wertbogen und Zeiger. Nichts kann ihn mehr
        //     zudecken.
        //  2. Der Speed-Regler ist deutlich kleiner als die uebrigen Regler;
        //     ein Abstand als Bruchteil des Radius wurde dadurch winzig und
        //     der Ring sass als "Wuergering" auf der schwarzen Pop-Kontur.
        //     Der Abstand hat jetzt einen festen Mindestwert in Pixeln, und
        //     Pop bekommt zusaetzlich Luft fuer seine 2,4-px-Tintenkontur.
        // Kleines Schloss oben rechts am Regler, wenn er per Rechtsklick
        // gesperrt wurde (User) - Presets, A/B, Reset und Smart lassen ihn dann
        // in Ruhe.
        if (slider.getProperties().getWithDefault ("knobLocked", false))
        {
            const float ls = juce::jmax (7.0f, radius * 0.42f);
            const float lx = centre.x + radius * 0.62f;
            const float ly = centre.y - radius * 0.86f;
            juce::Rectangle<float> body (lx - ls * 0.5f, ly - ls * 0.18f, ls, ls * 0.62f);
            g.setColour (juce::Colour (0xe60b0c10));
            g.fillRoundedRectangle (body.expanded (2.2f, 3.6f), 3.0f);
            g.setColour (themePalette().frameRaye.withAlpha (0.95f));
            g.fillRoundedRectangle (body, ls * 0.16f);
            juce::Path shackle;
            const float sr = ls * 0.28f;
            shackle.addCentredArc (body.getCentreX(), body.getY(), sr, sr, 0.0f,
                                   -juce::MathConstants<float>::halfPi,
                                    juce::MathConstants<float>::halfPi, true);
            g.strokePath (shackle, juce::PathStrokeType (juce::jmax (1.1f, ls * 0.14f)));
        }

        if (slider.getProperties().getWithDefault ("pairedGold", false))
        {
            // DIE eigentliche Ursache (User: "wird ja immer schlimmer, bei Pop
            // oben und unten abgeschnitten - der Bereich ist einfach zu Ende"):
            // der Ring lag AUSSERHALB der Slider-Komponente und wurde an deren
            // Kante gekappt. Statt das Layout umzubauen passt sich der Ring
            // jetzt dem an, was tatsaechlich Platz hat: er wird so gross wie
            // moeglich gezeichnet, aber nie groesser als die Komponente.
            const float th        = isComicTheme() ? 2.2f : 1.6f;
            const float halfBox   = juce::jmin ((float) width, (float) height) * 0.5f;
            const float maxRadius = halfBox - th * 0.5f - 0.5f;
            const float wish      = radius + (isComicTheme() ? juce::jmax (7.0f, radius * 0.16f)
                                                             : juce::jmax (4.0f, radius * 0.09f));
            const float pr = juce::jmin (wish, maxRadius);
            g.setColour (pairAccentColour());
            g.drawEllipse (centre.x - pr, centre.y - pr, pr * 2.0f, pr * 2.0f, th);
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
            iconHover (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
        }

        // Kleine Buchstaben-Icons (S = Solo, M = Mono-Check), gleicher
        // reduzierter Stil wie das Power-Icon: nur Ring + Buchstabe leuchten,
        // kein Hintergrund-Kaestchen.
        if (button.getProperties().getWithDefault ("soloIcon", false))
        {
            drawLetterIcon (g, button, "S", juce::Colour (0xffffb648));
            iconHover (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
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
            iconHover (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
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
        // Preset-Namensfeld: eigenes Aussehen, siehe drawPresetNameField().
        if (button.getProperties().getWithDefault ("presetNameField", false))
        {
            drawPresetNameField (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
        }
        if (button.getProperties().getWithDefault ("saveIcon", false))
        {
            drawSaveIcon (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
        }
        if (button.getProperties().getWithDefault ("resetIcon", false))
        {
            drawResetIcon (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
        }
        if (button.getProperties().getWithDefault ("abCopyIcon", false))
        {
            drawCopyIcon (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
        }
        if (button.getProperties().getWithDefault ("trashIcon", false))
        {
            drawTrashIcon (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
        }
        if (button.getProperties().getWithDefault ("gearIcon", false))
        {
            drawGearIcon (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
        }
        // Einstellungen: drei Schieberegler. Das Zahnrad ist schon fuer das
        // View-Panel im Sternenfeld vergeben (User: "hamburger sieht zu
        // oldschool aus ... lieber ein Zahnrad? Oder was anderes?") - zwei
        // Zahnraeder nebeneinander waeren die schlechtere Antwort.
        if (button.getProperties().getWithDefault ("settingsIcon", false))
        {
            auto b = button.getLocalBounds().toFloat().reduced (4.0f);
            const float s2 = juce::jmin (b.getWidth(), b.getHeight());
            auto box = b.withSizeKeepingCentre (s2, s2 * 0.78f);
            const bool hot = shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown;
            const juce::Colour col = juce::Colour (0xffb5b9c2)
                                        .interpolatedWith (juce::Colours::white, hot ? 0.55f : 0.0f);
            g.setColour (col);
            const float th   = juce::jmax (1.3f, s2 * 0.075f);
            const float knob = juce::jmax (2.0f, s2 * 0.125f);
            // Drei Linien, jede mit einem Regler an einer anderen Position.
            const float xs[3] = { 0.62f, 0.34f, 0.72f };
            for (int i = 0; i < 3; ++i)
            {
                const float y = box.getY() + box.getHeight() * (0.16f + 0.34f * (float) i);
                g.fillRoundedRectangle (box.getX(), y - th * 0.5f, box.getWidth(), th, th * 0.5f);
                const float cx = box.getX() + box.getWidth() * xs[i];
                g.fillEllipse (cx - knob, y - knob, knob * 2.0f, knob * 2.0f);
            }
            return;
        }
        // Filter-Symbol in den Sektionskoepfen von Galaxy und Dimension: eine
        // kleine Bandkurve. Leuchtet sie, arbeitet die Sektion NUR innerhalb
        // des Filters (Standard). Mit Schraegstrich ist sie am Filter vorbei.
        // PULSE: Sinus oder geglaetteter Puls - das Symbol zeigt, was
        // anliegt, statt das Wort hinzuschreiben (User: "wie bei einem
        // Synth"). Spart in HYPERDRIVE die Breite, die RAYE daneben braucht.
        if (button.getProperties().getWithDefault ("pulseIcon", false))
        {
            auto b = button.getLocalBounds().toFloat().reduced (1.5f);
            const float s2 = juce::jmin (b.getWidth(), b.getHeight());
            const bool on  = button.getToggleState();
            const bool off = button.getProperties().getWithDefault ("sectionOff", false);
            if (! button.getProperties().getWithDefault ("noPlate", false))
                drawSmallIconPlate (g, b, on && ! off);
            b = b.reduced (s2 * 0.24f);
            auto box = b.withSizeKeepingCentre (b.getWidth(), b.getHeight() * 0.62f);
            juce::Colour col = smallIconColour (on && ! off);
            if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
                col = col.interpolatedWith (juce::Colours::white, 0.30f);

            juce::Path w;
            const float x0 = box.getX(), x1 = box.getRight();
            const float yTop = box.getY(), yBot = box.getBottom(), yMid = box.getCentreY();
            if (on)
            {
                // Rechteckpuls mit weichen Ecken - "geglaetteter Puls".
                w.startNewSubPath (x0, yBot);
                w.lineTo (x0 + (x1 - x0) * 0.22f, yBot);
                w.lineTo (x0 + (x1 - x0) * 0.22f, yTop);
                w.lineTo (x0 + (x1 - x0) * 0.62f, yTop);
                w.lineTo (x0 + (x1 - x0) * 0.62f, yBot);
                w.lineTo (x1, yBot);
            }
            else
            {
                // Sinus - eine Periode.
                w.startNewSubPath (x0, yMid);
                w.quadraticTo (x0 + (x1 - x0) * 0.25f, yTop - (yMid - yTop) * 0.35f,
                               x0 + (x1 - x0) * 0.50f, yMid);
                w.quadraticTo (x0 + (x1 - x0) * 0.75f, yBot + (yBot - yMid) * 0.35f,
                               x1, yMid);
            }
            g.setColour (col);
            g.strokePath (w, juce::PathStrokeType (juce::jmax (1.2f, s2 * 0.085f),
                                                   juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
            iconHover (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
        }
        // SYNC: eine Note. An = die Geschwindigkeit haengt am Songtempo,
        // aus = freie Rate in Hz.
        if (button.getProperties().getWithDefault ("syncIcon", false))
        {
            auto b = button.getLocalBounds().toFloat().reduced (1.5f);
            const float s2 = juce::jmin (b.getWidth(), b.getHeight());
            const bool on  = button.getToggleState();
            const bool off = button.getProperties().getWithDefault ("sectionOff", false);
            if (! button.getProperties().getWithDefault ("noPlate", false))
                drawSmallIconPlate (g, b, on && ! off);
            b = b.reduced (s2 * 0.26f);
            juce::Colour col = smallIconColour (on && ! off);
            if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
                col = col.interpolatedWith (juce::Colours::white, 0.30f);
            g.setColour (col);

            const float headW = b.getWidth() * 0.54f;
            const float headH = headW * 0.74f;
            const float headX = b.getX();
            const float headY = b.getBottom() - headH;
            g.fillEllipse (headX, headY, headW, headH);
            const float stemW = juce::jmax (1.2f, s2 * 0.075f);
            g.fillRect (headX + headW - stemW, b.getY(), stemW, b.getHeight() - headH * 0.5f);
            // Faehnchen
            juce::Path flag;
            flag.startNewSubPath (headX + headW, b.getY() + stemW * 0.5f);
            flag.quadraticTo (b.getRight(), b.getY() + b.getHeight() * 0.22f,
                              headX + headW * 0.96f, b.getY() + b.getHeight() * 0.42f);
            g.strokePath (flag, juce::PathStrokeType (stemW, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
            iconHover (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
        }
        if (button.getProperties().getWithDefault ("filterIcon", false))
        {
            auto b = button.getLocalBounds().toFloat().reduced (1.5f);
            const float s2 = juce::jmin (b.getWidth(), b.getHeight());
            // Der Ein/Aus-Knopf der Focus-Leiste benutzt dasselbe Symbol
            // (User), hat aber die direkte Polaritaet: an = Focus wirkt. Bei
            // den Sektions-Knoepfen heisst an = Sektion NIMMT den Focus nicht
            // mit. In beiden Faellen gilt also: leuchtet = Focus zaehlt hier.
            const bool direct   = button.getProperties().getWithDefault ("focusDirect", false);
            const bool bypassed = direct ? ! button.getToggleState() : button.getToggleState();
            // Sichtbare Knopf-Flaeche (User: "mach ein Button rein") - ohne sie
            // schwebt das Symbol zwischen den Reglern.
            if (! button.getProperties().getWithDefault ("noPlate", false))
                drawSmallIconPlate (g, b, ! bypassed);
            b = b.reduced (s2 * 0.22f);
            auto box = b.withSizeKeepingCentre (b.getWidth(), b.getHeight() * 0.8f);
            const bool off      = button.getProperties().getWithDefault ("sectionOff", false);
            // Bewusst zurueckhaltend: der Knopf soll erkennbar sein, aber nicht
            // heller strahlen als die Regler daneben (User).
            juce::Colour col = smallIconColour (! bypassed && ! off);
            if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
                col = col.interpolatedWith (juce::Colours::white, 0.30f);

            juce::Path curve;
            const float y0 = box.getBottom(), y1 = box.getY();
            curve.startNewSubPath (box.getX(), y0);
            curve.quadraticTo (box.getX() + box.getWidth() * 0.28f, y0,
                               box.getX() + box.getWidth() * 0.40f, y1);
            curve.lineTo (box.getX() + box.getWidth() * 0.60f, y1);
            curve.quadraticTo (box.getX() + box.getWidth() * 0.72f, y0,
                               box.getRight(), y0);
            g.setColour (col);
            g.strokePath (curve, juce::PathStrokeType (juce::jmax (1.2f, s2 * 0.085f),
                                                       juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
            // Frueher lag hier zusaetzlich ein Schraegstrich fuer "aus". Das war
            // dieselbe Information zweimal (Licht aus UND durchgestrichen) und
            // hat den Knopf technisch wirken lassen - Leuchten oder nicht reicht.
            iconHover (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
        }
        // WING: drei Stufen, die Form sagt welche - waagerechte Linie, nach oben
        // geneigt, nach unten geneigt. Property "wingMode" 0/1/2.
        if (button.getProperties().getWithDefault ("wingIcon", false))
        {
            auto b = button.getLocalBounds().toFloat().reduced (1.5f);
            const int mode = (int) button.getProperties().getWithDefault ("wingMode", 0);
            if (! button.getProperties().getWithDefault ("noPlate", false))
                drawSmallIconPlate (g, b, mode != 0);
            auto box = b.reduced (juce::jmin (b.getWidth(), b.getHeight()) * 0.24f);
            juce::Colour col = smallIconColour (mode != 0);
            if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
                col = col.interpolatedWith (juce::Colours::white, 0.28f);
            g.setColour (col);
            const float th = juce::jmax (1.3f, box.getHeight() * 0.16f);
            const float dy = (mode == 0) ? 0.0f : box.getHeight() * 0.30f;
            const float yL = box.getCentreY() + ((mode == 1) ? dy : (mode == 2 ? -dy : 0.0f));
            const float yR = box.getCentreY() + ((mode == 1) ? -dy : (mode == 2 ? dy : 0.0f));
            g.drawLine (box.getX(), yL, box.getRight(), yR, th);
            return;
        }
        // "?" ganz unten links - schaltet die Hinweiszeile an/aus (User).
        if (button.getProperties().getWithDefault ("helpIcon", false))
        {
            auto b = button.getLocalBounds().toFloat();
            const float s2 = juce::jmin (b.getWidth(), b.getHeight());
            auto ring = b.withSizeKeepingCentre (s2 - 2.0f, s2 - 2.0f);
            const bool on = button.getToggleState();
            const juce::Colour col = on ? themePalette().knob
                                        : iconOffColour().interpolatedWith (juce::Colours::white,
                                                                            (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown) ? 0.30f : 0.0f);
            if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
            {
                g.setColour (juce::Colours::white.withAlpha (shouldDrawButtonAsDown ? 0.10f : 0.055f));
                g.fillEllipse (ring);
            }
            g.setColour (col.withAlpha (on ? 0.85f : 0.60f));
            g.drawEllipse (ring, 1.2f);
            g.setColour (col);
            g.setFont (juce::Font (juce::FontOptions (s2 * 0.62f, juce::Font::bold)));
            g.drawText ("?", button.getLocalBounds(), juce::Justification::centred, false);
            return;
        }
        if (button.getProperties().getWithDefault ("pauseIcon", false))
        {
            drawPauseIcon (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
        }
        // Kategorie (Mutate-Profil): Form und Hover wie der GALAXY-Knopf im
        // Header, nur mit unserem Gruen - Schrift bleibt weiss wie dort.
        if (button.getProperties().getWithDefault ("chipBtn", false))
        {
            const juce::Colour green (themePalette().chip);
            auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
            const float cornerSize = bounds.getHeight() * 0.5f;
            const bool on = button.getToggleState();
            if (isComicTheme())   // Pop: Kontur + Versatz-Schatten wie die Sektionen (User)
            {
                comicShadow (g, bounds.reduced (1.0f), cornerSize, 2.0f);
                g.setColour (on ? green.withMultipliedSaturation (0.75f) : juce::Colour (0xff3a3266));
                g.fillRoundedRectangle (bounds.reduced (1.0f), cornerSize);
                comicOutline (g, bounds.reduced (1.0f), cornerSize, 2.0f);
                g.setColour (on ? juce::Colour (0xff1a1633) : juce::Colour (0xffe8e2f5));
                g.setFont (chipFont());
                g.drawText (button.getButtonText().toUpperCase(), button.getLocalBounds(), juce::Justification::centred, false);
                return;
            }
            if (on)
            {
                g.setColour (green.withAlpha (0.20f));
                g.fillRoundedRectangle (bounds, cornerSize);
                g.setColour (green.withAlpha (0.80f));
                g.drawRoundedRectangle (bounds, cornerSize, 1.6f);
            }
            if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
            {
                g.setColour (juce::Colours::white.withAlpha (shouldDrawButtonAsDown ? 0.10f : 0.06f));
                g.fillRoundedRectangle (bounds, cornerSize);
            }
            g.setColour (on ? juce::Colours::white : juce::Colour (0xffb5b9c2));
            g.setFont (chipFont());
            g.drawText (button.getButtonText().toUpperCase(), button.getLocalBounds(), juce::Justification::centred, false);
            return;
        }
        // View-Speicherplatz A/B/C: nackter Buchstabe. Gewaehlt = Gruen
        // (Akzent), belegt = hell, leer = dunkel. Kein Rahmen.
        if (button.getProperties().getWithDefault ("viewSlotBtn", false))
        {
            const bool selected = button.getToggleState();
            const bool saved    = button.getProperties().getWithDefault ("slotSaved", false);
            juce::Colour col = selected ? accent
                             : saved    ? juce::Colour (0xffb5b9c2)
                                        : juce::Colour (0xff4a4e57);
            if (shouldDrawButtonAsHighlighted && ! selected)
                col = col.brighter (0.3f);
            g.setColour (col);
            g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)).withExtraKerningFactor (0.08f));
            g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
            return;
        }
        if (button.getProperties().getWithDefault ("rayStrengthIcon", false))
        {
            drawRayStrengthIcon (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            iconHover (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
        }
        // PRISM-Icon (eigenes Symbol, siehe drawPrismIcon) - vorher benutzte
        // der PRISM-Button faelschlich "bypassIcon" und sah dadurch exakt aus
        // wie der Dry-Bypass daneben.
        if (button.getProperties().getWithDefault ("prismIcon", false))
        {
            drawPrismIcon (g, button);
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
            iconHover (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
        }
        // "Balance"-Icon neben Drift: zwei nach innen zeigende Pfeile
        // (symbolisiert "zur Mitte zurueckholen"), siehe drawBalanceIcon.
        if (button.getProperties().getWithDefault ("balanceIcon", false))
        {
            drawBalanceIcon (g, button);
            iconHover (g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
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
        // RAYE-Pair: Gold statt Violett fuer den Pair-Knopf selbst und fuer
        // Sync in Hyperdrive, solange die Kopplung aktiv ist.
        const bool gold = button.getProperties().getWithDefault ("pairedGold", false);
        const juce::Colour btnAccent = gold ? pairAccentColour() : glowAccent;   // Pair je Theme (User)

        // View-Panel-Knoepfe ohne den Leucht-Hof (User: "leuchtende Kaesten").
        if ((isOn || gold) && ! button.getProperties().getWithDefault ("noGlow", false))
        {
            // Der Schein ist breiter als der Knopf und wurde bisher an dessen
            // rechteckigen Raendern abgeschnitten - auf einer hellen Flaeche
            // (View-Panel) sah man den Kasten deutlich (User-Bug: "button glow
            // rechteckig"). Er wird jetzt auf die Pillenform des Knopfes
            // begrenzt, damit die Kante der Form folgt statt dem Rechteck.
            juce::Path clip;
            clip.addRoundedRectangle (button.getLocalBounds().toFloat(), cornerSize + 2.0f);
            g.saveState();
            g.reduceClipRegion (clip);
            auto centre = bounds.getCentre();
            float maxR = bounds.getWidth() * 0.75f;
            for (int layer = 4; layer >= 1; --layer)
            {
                float r = maxR * ((float) layer / 4.0f);
                g.setColour (btnAccent.withAlpha (0.05f * (float) (5 - layer)));
                g.fillEllipse (centre.x - r, centre.y - r * 0.7f, r * 2.0f, r * 1.4f);
            }
            g.restoreState();
        }

        // Gilt fuer ALLE Knoepfe (Pulse, Sync, Polarity L/R, 1-4, Pair ...):
        // ist die Sektion aus, bleibt der eigene An-Zustand als hellerer
        // Rand sichtbar (User: "visuelles Feedback ueberall").
        const bool sectionIsOffNow = button.getProperties().getWithDefault ("sectionOff", false);
        const bool offButPaired = (gold || button.getToggleState()) && sectionIsOffNow;
        // "Grayed out" = die Farbe der UI an dieser Stelle (User, Runde 22).
        // Ein ausgeschalteter Knopf bekommt GAR KEINE eigene Flaeche mehr -
        // damit ist er in jedem Theme automatisch einheitlich, weil er schlicht
        // das zeigt, was ohnehin dahinter liegt: in einer eingeschalteten
        // Sektion deren Flaeche, in einer ausgeschalteten die Platte. Das war
        // der Punkt, an dem die frueheren sieben Sonderfaelle pro Theme nie
        // wirklich zusammenpassten. Pop behaelt seine gefuellten Pillen.
        const juce::Colour onFull = isComicTheme() ? btnAccent.withMultipliedSaturation (0.7f).darker (0.15f)
                                  : gold           ? juce::Colour (0xff2e2a1c)
                                                   : juce::Colour (0xff2a1f33);
        // Knoepfe in den Panels (Settings/View) tragen "noGlow". Ihr
        // An-Zustand war bisher controlOnFill() - in Pop eine kraeftige Pille,
        // in allen anderen Themes aber praktisch die Panelfarbe (User: "nur
        // Pop bleibt im Menue hell, alle anderen sind dunkel"). Sie bekommen
        // deshalb ihre eigene, deutlich sichtbare Faerbung, die sich aus der
        // Theme-Farbe ableitet statt aus der Plattenfarbe.
        const bool panelBtn = button.getProperties().getWithDefault ("noGlow", false);
        const bool fillIt = isComicTheme() || panelBtn || isOn || offButPaired;
        if (fillIt)
        {
            // An-Zustand auf halbem Weg zwischen "unsichtbar" und dem alten
            // An-Ton (User: "genau zwischen off und der aktuellen on-Staerke").
            juce::Colour base = isComicTheme() ? (isOn ? onFull : juce::Colour (0xff3a3266))
                                               : (gold ? themePalette().plate.interpolatedWith (onFull, 0.5f) : controlOnFill());
            // Pop, Sektion AUS: die gefuellte Pille war heller als alles andere
            // in der Sektion und zog dadurch den Blick auf sich (User: "zu sehr
            // auffallend jetzt die ganzen Buttons"). Sie rueckt jetzt dicht an
            // die Sektionsflaeche heran - sichtbar als Knopf, aber still.
            if (isComicTheme() && sectionIsOffNow)
                base = juce::Colour (0xff1f1a36);
            if (panelBtn && ! isComicTheme())
                base = isOn ? juce::Colour (0xff1e2128).interpolatedWith (themePalette().knob, 0.34f)
                            : juce::Colour (0xff2a2e37);
            // Knopf an, Sektion aus: nur noch ein Hauch heller als die Platte -
            // sichtbar, aber nicht laut.
            if (offButPaired && ! isOn && ! isComicTheme())
                base = themePalette().plate.brighter (0.07f);
            if (shouldDrawButtonAsDown) base = base.brighter (0.1f);
            else if (shouldDrawButtonAsHighlighted) base = base.brighter (0.05f);
            comicShadow (g, bounds, cornerSize);
            g.setColour (base);
            g.fillRoundedRectangle (bounds, cornerSize);
        }
        else if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
        {
            // Ohne Flaeche braucht der Aus-Zustand trotzdem Hover-Feedback.
            g.setColour (juce::Colours::white.withAlpha (shouldDrawButtonAsDown ? 0.08f : 0.045f));
            g.fillRoundedRectangle (bounds, cornerSize);
        }

        // Parallax-Klick-Knopf, Variante B (Runde 45): Fuellung von links,
        // waechst mit dem Modus (1 = leer, letzter = voll).
        {
            const double fill = button.getProperties().getWithDefault ("modeFill", -1.0);
            if (fill > 0.0)
            {
                juce::Path clipP; clipP.addRoundedRectangle (bounds, cornerSize);
                g.saveState();
                g.reduceClipRegion (clipP);
                g.setColour (btnAccent.withAlpha (sectionIsOffNow ? 0.12f : 0.30f));
                g.fillRect (bounds.withWidth (bounds.getWidth() * (float) fill));
                g.restoreState();
            }
        }

        // Pair bei ausgeschalteter RAYE-Sektion: Zustand trotzdem sichtbar
        // (hellerer Rand, User) - Property "pairedGold" bleibt gesetzt.
        if (isComicTheme())
        {
            comicOutline (g, bounds, cornerSize);
            if (offButPaired) { g.setColour (juce::Colour (0xff625d7d)); g.drawRoundedRectangle (bounds.reduced (2.0f), cornerSize, 1.0f); }   // nochmal dezenter (User)
            return;
        }
        // Sektion aus: GAR KEIN Rahmen mehr (User: "im Moment sehen die
        // Buttons auch im section=off-Zustand noch so aus, als waere die
        // Section an"). Der eigene An-Zustand bleibt allein ueber die etwas
        // hellere Flaeche sichtbar - klicken kann man sie weiterhin, sie sind
        // nur nicht mehr laut.
        if (sectionIsOffNow)
            return;
        g.setColour ((isOn || gold) ? btnAccent : juce::Colour (0xff3a3d45));
        // "thinOnFrame" (Polarity L/R und 1-4): der An-Rahmen ist eine Spur
        // duenner, damit diese Knoepfe nicht mehr Gewicht bekommen als die
        // Regler daneben (User: "wirklich nur mini mini mini duenner").
        const float onThickness = button.getProperties().getWithDefault ("thinOnFrame", false) ? 1.35f : 1.6f;
        g.drawRoundedRectangle (bounds, cornerSize, (isOn || gold) ? onThickness : 1.0f);
    }

    // ===== PRESET-POPUP =====
    // Die Preset-Liste war das letzte Stueck JUCE-Standard im Plugin: grauer
    // Kasten, graue Schrift, blaue Auswahl (User: "nicht dieses Grau, lieber
    // was Schoeneres"). Jetzt dieselbe Sprache wie der Rest - Plattenfarbe,
    // weicher Rahmen in der Theme-Farbe, Auswahl als abgerundeter Streifen
    // im Akzent statt als harter blauer Balken.
    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
    {
        // Bewusst KEINE abgerundeten Ecken: das Menuefenster selbst ist
        // rechteckig und deckend, runde Ecken liessen dort helle Zipfel stehen
        // (User: "kleiner Grafikbug, Ecken sind weiss"). Also ganze Flaeche
        // fuellen und nur die Kante zeichnen.
        auto b = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
        const auto pal = themePalette();
        g.fillAll (pal.plate.interpolatedWith (juce::Colours::white, 0.05f));
        g.setColour (pal.frameMain.withAlpha (0.38f));
        g.drawRect (b, 1.0f);
    }

    int getPopupMenuBorderSize() override { return 7; }

    juce::Font getPopupMenuFont() override
    {
        return juce::Font (juce::FontOptions (15.0f)).withExtraKerningFactor (0.01f);
    }

    void getIdealPopupMenuItemSize (const juce::String& text, bool isSeparator,
                                    int standardMenuItemHeight, int& idealWidth, int& idealHeight) override
    {
        if (isSeparator) { idealHeight = 11; idealWidth = 50; return; }
        auto f = getPopupMenuFont();
        idealHeight = standardMenuItemHeight > 0 ? juce::jmax (standardMenuItemHeight, 26) : 27;
        idealWidth  = (int) juce::GlyphArrangement::getStringWidth (f, text) + 46;
    }

    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu, const juce::String& text,
                            const juce::String& shortcutKeyText, const juce::Drawable* icon,
                            const juce::Colour* textColourToUse) override
    {
        juce::ignoreUnused (shortcutKeyText, icon, textColourToUse);
        const auto pal = themePalette();
        auto r = area.toFloat().reduced (5.0f, 1.0f);

        if (isSeparator)
        {
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.fillRect (r.getX() + 4.0f, r.getCentreY(), r.getWidth() - 8.0f, 1.0f);
            return;
        }

        if (isHighlighted && isActive)
        {
            g.setColour (pal.knob.withAlpha (0.14f));
            g.fillRoundedRectangle (r, 6.0f);
        }

        juce::Colour col = isActive ? juce::Colour (0xffdfe3ea) : juce::Colour (0xff6d7280);
        if (isTicked) col = pal.knob;
        g.setColour (col);
        g.setFont (getPopupMenuFont());
        auto textArea = r.withTrimmedLeft (24.0f).withTrimmedRight (hasSubMenu ? 20.0f : 6.0f);
        g.drawText (text, textArea, juce::Justification::centredLeft, true);

        if (isTicked)
        {
            // Kein Haken-Glyph, sondern ein kleiner Punkt in der Akzentfarbe -
            // dasselbe Zeichen, das im Plugin sonst "das hier ist aktiv" sagt.
            g.setColour (pal.knob);
            g.fillEllipse (r.getX() + 9.0f, r.getCentreY() - 3.0f, 6.0f, 6.0f);
        }
        if (hasSubMenu)
        {
            juce::Path p;
            const float cx = r.getRight() - 12.0f, cy = r.getCentreY();
            p.startNewSubPath (cx - 2.5f, cy - 4.0f);
            p.lineTo (cx + 2.5f, cy);
            p.lineTo (cx - 2.5f, cy + 4.0f);
            g.setColour (col.withAlpha (0.75f));
            g.strokePath (p, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    // ===== HOVER HINTS =====
    // JUCE zeichnet Tooltips sonst als kleinen gelben Kasten mit winziger
    // Schrift. Hier stattdessen (User): groessere Schrift, die Bezeichnung
    // bis zum Doppelpunkt fett, ein weicher abgerundeter Rahmen in der
    // Theme-Farbe und deutlich mehr Innenabstand.
    static juce::TextLayout layoutHintText (const juce::String& text, juce::Colour colour, float maxWidth)
    {
        juce::AttributedString a;
        a.setJustification (juce::Justification::topLeft);
        a.setLineSpacing (2.0f);
        const juce::Font boldF  (juce::FontOptions (14.5f, juce::Font::bold));
        const juce::Font plainF (juce::FontOptions (14.5f));
        const int colon = text.indexOfChar (':');
        if (colon > 0 && colon <= 22)   // "Gravity: ..." -> Name fett, Rest normal
        {
            a.append (text.substring (0, colon + 1), boldF,  colour);
            a.append (text.substring (colon + 1),    plainF, colour);
        }
        else
        {
            a.append (text, plainF, colour);
        }
        juce::TextLayout tl;
        tl.createLayout (a, maxWidth);
        return tl;
    }

    juce::Rectangle<int> getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos,
                                           juce::Rectangle<int> parentArea) override
    {
        auto tl = layoutHintText (tipText, juce::Colours::black, 380.0f);
        const int w = (int) (tl.getWidth()  + 28.0f);
        const int h = (int) (tl.getHeight() + 20.0f);
        return juce::Rectangle<int> (screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 14) : screenPos.x + 26,
                                     screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 8)  : screenPos.y + 8,
                                     w, h).constrainedWithin (parentArea);
    }

    void drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height) override
    {
        const juce::Rectangle<float> b (0.0f, 0.0f, (float) width, (float) height);
        const float corner = 9.0f;
        g.setColour (juce::Colour (0xf0161920));
        g.fillRoundedRectangle (b, corner);
        g.setColour (themePalette().frameMain.withAlpha (0.42f));
        g.drawRoundedRectangle (b.reduced (0.75f), corner, 1.4f);
        layoutHintText (text, juce::Colour (0xffe4e8ef), (float) width - 28.0f)
            .draw (g, juce::Rectangle<float> (14.0f, 10.0f, (float) width - 28.0f, (float) height - 20.0f));
    }

    // Dezentes Hover-/Klick-Feedback fuer die Icon-Knoepfe in den Sektionen
    // (Solo, Lock, Mod, On/Off, RAYE-Stufe). Die hatten bisher gar keins
    // (User: "faellt mir erst jetzt auf") - gerade in der rahmenlosen
    // Darstellung ist das der einzige Hinweis, dass dort etwas klickbar ist.
    static void iconHover (juce::Graphics& g, juce::Button& button, bool highlighted, bool down)
    {
        if (! highlighted && ! down) return;
        auto hb = button.getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (juce::Colours::white.withAlpha (down ? 0.13f : 0.075f));
        g.fillRoundedRectangle (hb, juce::jmin (hb.getHeight(), hb.getWidth()) * 0.30f);
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
            || button.getProperties().getWithDefault ("prismIcon", false)
            || button.getProperties().getWithDefault ("presetNameField", false)
            || button.getProperties().getWithDefault ("saveIcon", false)
            || button.getProperties().getWithDefault ("resetIcon", false)
            || button.getProperties().getWithDefault ("abCopyIcon", false)
            || button.getProperties().getWithDefault ("trashIcon", false)
            || button.getProperties().getWithDefault ("gearIcon", false)
            || button.getProperties().getWithDefault ("pauseIcon", false)
            || button.getProperties().getWithDefault ("viewSlotBtn", false)
            || button.getProperties().getWithDefault ("chipBtn", false)
            || button.getProperties().getWithDefault ("rayStrengthIcon", false)
            || button.getProperties().getWithDefault ("linkIcon", false)
            || button.getProperties().getWithDefault ("balanceIcon", false)
            || button.getProperties().getWithDefault ("lockIcon", false)
            || button.getProperties().getWithDefault ("arrowIcon", false)
            || button.getProperties().getWithDefault ("pulseIcon", false)
            || button.getProperties().getWithDefault ("syncIcon", false))
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
        else if (button.getProperties().contains ("slotLetter"))
        {
            // "Save A": das Wort normal, der Buchstabe in Gruen wie oben ueber
            // dem Sternenfeld (User).
            const juce::String letter = button.getProperties()["slotLetter"].toString();
            auto f = juce::Font (juce::FontOptions (button.getHeight() * 0.42f, juce::Font::bold)).withExtraKerningFactor (0.04f);
            g.setFont (f);
            const juce::String word ("Save ");
            const float wWord   = juce::GlyphArrangement::getStringWidth (f, word);
            const float wLetter = juce::GlyphArrangement::getStringWidth (f, letter);
            const float x0 = button.getLocalBounds().toFloat().getCentreX() - (wWord + wLetter) * 0.5f;
            auto area = button.getLocalBounds().toFloat();
            g.setColour (juce::Colour (0xffb5b9c2));
            g.drawText (word, juce::Rectangle<float> (x0, area.getY(), wWord + 2.0f, area.getHeight()), juce::Justification::centredLeft, false);
            g.setColour (juce::Colour (0xff5be3c7));
            g.drawText (letter, juce::Rectangle<float> (x0 + wWord, area.getY(), wLetter + 2.0f, area.getHeight()), juce::Justification::centredLeft, false);
        }
        else
        {
            const bool secOff = button.getProperties().getWithDefault ("sectionOff", false);
            const bool isOn = button.getToggleState() && ! secOff;
            // Sektion aus: auch die Beschriftung rueckt Richtung UI-Farbe
            // (User) - derselbe Ton wie die Regler-Labels daneben.
            // Reines Weiss sticht auf den dunklen Platten unangenehm heraus
            // (User: Augen) - der An-Zustand geht ein Stueck davon weg.
            // Pop behaelt auch in einer AUSGESCHALTETEN Sektion seine gefuellte
            // Pille (0xff3a3266), und die ist HELLER als die Sektionsflaeche.
            // Die dunkle Off-Schrift der anderen Themes verschwindet darauf
            // spurlos (User: "man erkennt es nicht"). Dort deshalb ein
            // gedaempfter heller Ton statt eines dunklen.
            g.setColour (secOff ? (isComicTheme() ? juce::Colour (0xff7d7799) : labelOffColour())
                                : isOn ? juce::Colour (0xffe4e7ec) : juce::Colour (0xffb5b9c2));
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
                // Gedeckelt (User: "Pulse Font feiner - ist glaub groesser als
                // Sync"): die Groesse haengt an der Button-Hoehe, und Pulse ist
                // hoeher als Sync. Mit dem Deckel sind alle Sektions-Knoepfe
                // gleich gross beschriftet.
                g.setFont (juce::Font (juce::FontOptions (juce::jmin (button.getHeight() * 0.42f, 14.5f), juce::Font::bold)).withExtraKerningFactor (0.04f));
            // "textYShift" (Runde 49): schiebt die Beschriftung nach oben,
            // wenn die Modus-Punkte INNERHALB der Pille sitzen.
            const int textShiftY = (int) (double) button.getProperties().getWithDefault ("textYShift", 0.0);
            g.drawText (button.getButtonText(), button.getLocalBounds().translated (0, textShiftY), juce::Justification::centred);
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
        // Optionale eigene An-Farbe (Property "powerColour", ARGB als int) -
        // PRISM nutzt Lila, damit Kachel und Leiste als EIN Element lesbar sind.
        auto onCol = accent;
        if (button.getProperties().contains ("powerColour"))
            onCol = juce::Colour ((juce::uint32) (int) button.getProperties()["powerColour"]);
        auto col = isOn ? onCol : iconOffColour();

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
        auto col = (isOn ? monoOnColour : iconOffColour()).withAlpha (isOn ? alpha : 1.0f);
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
        auto col = locked ? lockOnColour : iconOffColour();
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
        auto col = isOn ? dryOnColour : iconOffColour();
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

    // Gemeinsamer Hover-Hintergrund fuer die kleinen Icon-Buttons der
    // Titelzeile (dieselbe Pille wie bei Undo/Redo), damit sich alle
    // Werkzeuge dort gleich anfuehlen.
    void drawToolHover (juce::Graphics& g, juce::Button& button, bool highlighted, bool down)
    {
        auto fullBounds = button.getLocalBounds().toFloat().reduced (1.0f);
        if (down || highlighted)
        {
            g.setColour (juce::Colours::white.withAlpha (down ? 0.20f : 0.13f));
            g.fillRoundedRectangle (fullBounds, fullBounds.getHeight() * 0.30f);
        }
    }

    // ===== SPEICHERN-ICON (Diskette) =====
    // User-Idee: "Save koennte auch ein Disketten Symbol sein." Ja - das
    // Symbol ist aelter als die meisten Nutzer, aber es ist das einzige, das
    // ohne Beschriftung als "speichern" gelesen wird. Ein Pfeil in eine
    // Schublade oder eine Wolke braeuchte immer einen zweiten Blick.
    // Hier bewusst nur die drei Formen, die die Diskette ausmachen: die
    // Umrisskante mit abgeschnittener Ecke, der Metallschieber oben und das
    // Etikett unten.
    void drawSaveIcon (juce::Graphics& g, juce::Button& button, bool highlighted, bool down)
    {
        drawToolHover (g, button, highlighted, down);
        auto b = button.getLocalBounds().toFloat();
        const float s = juce::jmin (b.getWidth(), b.getHeight()) * 0.56f;
        auto r = juce::Rectangle<float> (s, s).withCentre (b.getCentre());

        auto col = juce::Colour (0xffb5b9c2).withAlpha (button.isEnabled() ? 1.0f : 0.3f);
        if (button.isEnabled() && (down || highlighted))
            col = juce::Colours::white;
        g.setColour (col);

        // Umriss mit abgeschnittener Ecke oben rechts
        const float cut = s * 0.22f;
        juce::Path body;
        body.startNewSubPath (r.getX(), r.getY());
        body.lineTo (r.getRight() - cut, r.getY());
        body.lineTo (r.getRight(), r.getY() + cut);
        body.lineTo (r.getRight(), r.getBottom());
        body.lineTo (r.getX(), r.getBottom());
        body.closeSubPath();
        g.strokePath (body, juce::PathStrokeType (1.4f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));

        // Schieber oben (schmales Rechteck, leicht eingerueckt)
        g.drawRect (juce::Rectangle<float> (r.getX() + s * 0.22f, r.getY() + 1.0f, s * 0.40f, s * 0.26f), 1.2f);
        // Etikett unten (gefuellt, breiter)
        g.fillRect (juce::Rectangle<float> (r.getX() + s * 0.18f, r.getBottom() - s * 0.32f, s * 0.64f, s * 0.22f));
    }

    // ===== RESET-ICON (Kreispfeil) =====
    // User-Idee: "Reset koennte auch ein Kreis mit Pfeil Icon sein - aehnlich
    // wie Undo ... vielleicht in Rot."
    // Aehnlich wie Undo ist genau die Gefahr, deshalb drei Unterschiede,
    // die zusammen eindeutig sind: (1) fast geschlossener Kreis (300 Grad)
    // statt Halbbogen, (2) andere Zeile, (3) warmes Rot statt Grau. Rot
    // gedaempft - es soll "Achtung, setzt alles zurueck" sagen, nicht
    // "Fehler".
    void drawResetIcon (juce::Graphics& g, juce::Button& button, bool highlighted, bool down)
    {
        drawToolHover (g, button, highlighted, down);
        auto b = button.getLocalBounds().toFloat();
        const float s = juce::jmin (b.getWidth(), b.getHeight());
        auto c = b.getCentre();
        const float r = s * 0.27f;

        auto col = juce::Colour (0xffd9645c).withAlpha (button.isEnabled() ? 0.9f : 0.3f);
        if (button.isEnabled() && (down || highlighted))
            col = juce::Colour (0xffff7f76);
        g.setColour (col);

        // Bogen von 60 Grad bis 360 Grad (0 = 12 Uhr, im Uhrzeigersinn),
        // Spitze am Ende oben.
        const float fromRad = juce::degreesToRadians (60.0f);
        const float toRad   = juce::degreesToRadians (360.0f);
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, r, r, 0.0f, fromRad, toRad, true);
        g.strokePath (arc, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Pfeilspitze am Bogenende (12 Uhr), Tangente zeigt nach rechts.
        const float tipX = c.x;
        const float tipY = c.y - r;
        const float headLen = s * 0.17f;
        juce::Path head;
        head.startNewSubPath (tipX - headLen * 0.55f, tipY - headLen * 0.6f);
        head.lineTo (tipX + headLen * 0.5f, tipY);
        head.lineTo (tipX - headLen * 0.55f, tipY + headLen * 0.6f);
        g.strokePath (head, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ===== ZAHNRAD (View-Panel im Sternenfeld) =====
    // Sitzt halbtransparent in der Ecke des Sternenfelds; wird beim Hover
    // und bei geoeffnetem Panel (Toggle-Status) voll sichtbar.
    void drawGearIcon (juce::Graphics& g, juce::Button& button, bool highlighted, bool down)
    {
        auto b = button.getLocalBounds().toFloat();
        const bool open = button.getToggleState();
        // Die Klickflaeche ist groesser als das Icon (User) - gezeichnet wird
        // weiterhin mit 22 px Bezugsgroesse.
        const float s = juce::jmin (22.0f, juce::jmin (b.getWidth(), b.getHeight()));
        auto c = b.getCentre();
        const float rOuter = s * 0.34f;
        const float rInner = s * 0.24f;
        const float alpha = (open || highlighted || down) ? 0.95f : 0.45f;

        juce::Path gear;
        constexpr int teeth = 8;
        for (int i = 0; i < teeth * 2; ++i)
        {
            const float a0 = juce::MathConstants<float>::twoPi * (float) i / (float) (teeth * 2);
            const float a1 = juce::MathConstants<float>::twoPi * (float) (i + 1) / (float) (teeth * 2);
            const float r = (i % 2 == 0) ? rOuter : rInner;
            const float x0 = c.x + std::cos (a0) * r, y0 = c.y + std::sin (a0) * r;
            const float x1 = c.x + std::cos (a1) * r, y1 = c.y + std::sin (a1) * r;
            if (i == 0) gear.startNewSubPath (x0, y0);
            gear.lineTo (x1, y1);
        }
        gear.closeSubPath();
        g.setColour ((open ? glowAccent : juce::Colour (0xffd7dbe4)).withAlpha (alpha));
        g.fillPath (gear);
        g.setColour (juce::Colour (0xff14161a).withAlpha (alpha));
        g.fillEllipse (c.x - s * 0.11f, c.y - s * 0.11f, s * 0.22f, s * 0.22f);
    }

    // ===== PAUSE (Sternenfeld anhalten), sitzt links neben dem Zahnrad =====
    // Smart-Kategorien - groesser gesetzt (User: "deren Namen ich sowieso
    // bisschen groesser haben will").
    static juce::Font chipFont()
    {
        return juce::Font (juce::FontOptions (12.8f, juce::Font::bold)).withExtraKerningFactor (0.10f);
    }

    void drawPauseIcon (juce::Graphics& g, juce::Button& button, bool highlighted, bool down)
    {
        auto b = button.getLocalBounds().toFloat();
        const bool paused = button.getToggleState();
        const float s = juce::jmin (b.getWidth(), b.getHeight());
        auto c = b.getCentre();
        const float alpha = (paused || highlighted || down) ? 0.95f : 0.45f;
        g.setColour ((paused ? glowAccent : juce::Colour (0xffd7dbe4)).withAlpha (alpha));
        if (paused)
        {
            // Play-Dreieck: Klick setzt fort.
            juce::Path tri;
            tri.addTriangle (c.x - s * 0.16f, c.y - s * 0.22f, c.x - s * 0.16f, c.y + s * 0.22f, c.x + s * 0.24f, c.y);
            g.fillPath (tri);
        }
        else
        {
            const float w = s * 0.13f, h = s * 0.44f, gap = s * 0.10f;
            g.fillRoundedRectangle (c.x - gap - w, c.y - h * 0.5f, w, h, 1.5f);
            g.fillRoundedRectangle (c.x + gap,     c.y - h * 0.5f, w, h, 1.5f);
        }
    }

    // ===== PAPIERKORB (Preset loeschen) =====
    // Deckel mit Griff, Korb mit zwei Rillen. Gedaempft, nur beim Hover
    // heller - loeschen soll kein Blickfang sein.
    void drawTrashIcon (juce::Graphics& g, juce::Button& button, bool highlighted, bool down)
    {
        drawToolHover (g, button, highlighted, down);
        auto b = button.getLocalBounds().toFloat();
        const float s = juce::jmin (b.getWidth(), b.getHeight()) * 0.52f;
        auto r = juce::Rectangle<float> (s * 0.78f, s).withCentre (b.getCentre());

        auto col = juce::Colour (0xffb5b9c2).withAlpha (button.isEnabled() ? 1.0f : 0.28f);
        if (button.isEnabled() && (down || highlighted))
            col = juce::Colours::white;
        g.setColour (col);

        // Deckel + Griff
        const float lidY = r.getY() + s * 0.16f;
        g.drawLine (r.getX() - s * 0.08f, lidY, r.getRight() + s * 0.08f, lidY, 1.4f);
        g.drawLine (r.getCentreX() - s * 0.14f, r.getY() + s * 0.02f, r.getCentreX() + s * 0.14f, r.getY() + s * 0.02f, 1.4f);
        // Korb (leicht nach unten verjuengt)
        juce::Path body;
        body.startNewSubPath (r.getX() + s * 0.02f, lidY + s * 0.10f);
        body.lineTo (r.getRight() - s * 0.02f, lidY + s * 0.10f);
        body.lineTo (r.getRight() - s * 0.10f, r.getBottom());
        body.lineTo (r.getX() + s * 0.10f, r.getBottom());
        body.closeSubPath();
        g.strokePath (body, juce::PathStrokeType (1.3f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
        // zwei Rillen
        g.drawLine (r.getCentreX() - s * 0.12f, lidY + s * 0.24f, r.getCentreX() - s * 0.10f, r.getBottom() - s * 0.14f, 1.1f);
        g.drawLine (r.getCentreX() + s * 0.12f, lidY + s * 0.24f, r.getCentreX() + s * 0.10f, r.getBottom() - s * 0.14f, 1.1f);
    }

    // ===== COPY-ICON (A/B) =====
    // Ein Pfeil in Kopierrichtung: nach rechts, wenn A aktiv ist (A -> B),
    // nach links, wenn B aktiv ist (B -> A). Leuchtet in der Akzentfarbe nur,
    // solange A und B sich unterscheiden - sonst gibt es nichts zu kopieren
    // und das Icon liegt grau da. Bewusst kein Doppelpfeil (der hiesse
    // "tauschen").
    void drawCopyIcon (juce::Graphics& g, juce::Button& button, bool highlighted, bool down)
    {
        drawToolHover (g, button, highlighted, down);
        auto b = button.getLocalBounds().toFloat();
        const bool lit = button.getProperties().getWithDefault ("abCopyLit", false);
        const bool toRight = button.getProperties().getWithDefault ("abCopyToRight", true);
        const float s = juce::jmin (b.getWidth(), b.getHeight());
        auto c = b.getCentre();

        auto col = lit ? glowAccent : iconOffColour();
        if (lit && (down || highlighted))
            col = juce::Colour (0xffcf9bff);
        g.setColour (col.withAlpha (button.isEnabled() ? 1.0f : 0.3f));

        // Schaft + Spitze, plus ein kleiner Doppelstrich am Schaftanfang als
        // "Kopie"-Zeichen (zwei Blaetter).
        const float len = s * 0.46f;
        const float dir = toRight ? 1.0f : -1.0f;
        const float x0 = c.x - dir * len * 0.5f;
        const float x1 = c.x + dir * len * 0.5f;
        g.drawLine (x0, c.y, x1, c.y, 1.6f);
        juce::Path head;
        head.startNewSubPath (x1 - dir * s * 0.14f, c.y - s * 0.14f);
        head.lineTo (x1, c.y);
        head.lineTo (x1 - dir * s * 0.14f, c.y + s * 0.14f);
        g.strokePath (head, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // zwei kurze Striche am Schaftanfang
        g.drawLine (x0, c.y - s * 0.12f, x0, c.y + s * 0.12f, 1.4f);
        g.drawLine (x0 - dir * s * 0.08f, c.y - s * 0.12f, x0 - dir * s * 0.08f, c.y + s * 0.12f, 1.4f);

        if (lit)
        {
            // dezenter Schein, damit "es gibt etwas zu kopieren" auffaellt
            g.setColour (col.withAlpha (0.18f));
            g.fillEllipse (c.x - s * 0.34f, c.y - s * 0.34f, s * 0.68f, s * 0.68f);
        }
    }

    // ===== RAY: STAERKE-ICON (3 Stufen) =====
    // User-Vorgabe: "ein Icon, das man dreimal klicken kann ... jeder Schritt
    // farblich hervorgehoben (Fuelleffekt von unten oder Glow)".
    // Form: ein Strahl - drei nach rechts oben laufende Linien, die sich
    // auffaechern. Stufe 1 = die unterste Linie leuchtet, Stufe 2 = zwei,
    // Stufe 3 = alle drei, dazu ein Glow, der mit der Stufe waechst. Man
    // sieht die Stufe also ohne Zahl, und das Symbol selbst sagt "Ray".
    // Aus (sectionOff): alles grau, kein Glow.
    void drawRayStrengthIcon (juce::Graphics& g, juce::Button& button, bool highlighted, bool down)
    {
        // Ohne Glow (User: "der glow sieht billig aus"). Stattdessen macht
        // die Stufe sich ueber drei Dinge bemerkbar, die alle scharf und
        // flach sind: wie viele Strahlen leuchten, wie kraeftig die Kachel
        // umrandet ist, und ein kleiner Stufen-Punkt unten. Kein Weichzeichner
        // irgendwo - was leuchtet, ist eine klare Linie.
        auto b = button.getLocalBounds().toFloat().reduced (2.0f);
        const bool sectionIsOff = button.getProperties().getWithDefault ("sectionOff", false);
        // 0 = Off (Sektion an, aber kein Effekt), 1..3 = leicht/mittel/stark.
        const int level = juce::jlimit (0, 3, (int) button.getProperties().getWithDefault ("rayLevel", 0));
        const juce::Colour gold (themePalette().frameRaye.withMultipliedSaturation (isWaterTheme() ? 0.6f : 1.0f));   // wie Pair (User)
        const juce::Colour grey (iconOffColour());
        const float s = juce::jmin (b.getWidth(), b.getHeight());
        auto tile = juce::Rectangle<float> (s, s).withCentre (b.getCentre());
        const float corner = s * 0.18f;

        // Kachel jetzt exakt nach demselben Rezept wie die Polarity-Knoepfe
        // (User: "Vorbild: Polarity Icons im gleichen Theme - das ist perfekt"):
        // bei ausgeschalteter Sektion GAR KEINE Flaeche, sonst die gemeinsame
        // An-Flaeche. Der feste Grauton 1c1e23 passte in keinem Theme.
        if (! sectionIsOff)
        {
            // Auch hier traegt die Flaeche keinen Zustand mehr (User: "wenn
            // click state 1-3 ist die Fuellung ploetzlich gelb"). Die Stufe
            // steht in den Strahlen und den Punkten - das reicht.
            juce::Colour fill = controlIdleFill();
            if (down) fill = fill.brighter (0.10f);
            else if (highlighted) fill = fill.brighter (0.05f);
            g.setColour (fill);
            g.fillRoundedRectangle (tile, corner);
        }
        else if (down || highlighted)
        {
            g.setColour (juce::Colours::white.withAlpha (down ? 0.08f : 0.045f));
            g.fillRoundedRectangle (tile, corner);
        }
        const float rimAlpha = sectionIsOff ? 0.18f : (0.20f + 0.09f * (float) level);
        g.setColour ((sectionIsOff ? grey : gold).withAlpha (rimAlpha));
        g.drawRoundedRectangle (tile, corner, sectionIsOff ? 1.0f : 1.4f);

        // Drei Strahlen aus einem Ursprung unten links
        const float ox = tile.getX() + s * 0.24f;
        const float oy = tile.getBottom() - s * 0.26f;
        const float len = s * 0.54f;
        static const float angles[3] = { -16.0f, -42.0f, -68.0f };
        for (int i = 0; i < 3; ++i)
        {
            const bool lvl = i < level;                 // Stufe auch bei Sektion aus sichtbar (User)
            const bool on  = ! sectionIsOff && lvl;
            const float a = juce::degreesToRadians (angles[i]);
            const float ex = ox + std::cos (a) * len;
            const float ey = oy + std::sin (a) * len;
            g.setColour (on ? gold : (lvl ? iconOnInOffSection() : grey.withAlpha (0.35f)));
            g.drawLine (ox, oy, ex, ey, on ? 2.1f : (lvl ? 1.7f : 1.2f));   // aktiv minimal duenner (User)
            if (on)
            {
                g.setColour (juce::Colour (0xffe8ecf3).withAlpha (0.85f));
                g.fillEllipse (ex - 1.6f, ey - 1.6f, 3.2f, 3.2f);
            }
        }
        g.setColour ((sectionIsOff || level == 0) ? grey : gold);
        g.fillEllipse (ox - 2.4f, oy - 2.4f, 4.8f, 4.8f);

        // Stufen-Punkte unten rechts: 1-3 kleine Punkte, gefuellt bis zur Stufe
        for (int i = 0; i < 3; ++i)
        {
            const float px = tile.getRight() - s * 0.13f - (float) (2 - i) * s * 0.11f;
            const float py = tile.getBottom() - s * 0.12f;
            const bool on = ! sectionIsOff && i < level;
            g.setColour (on ? gold : (i < level ? iconOnInOffSection() : grey.withAlpha (0.35f)));
            if (on) g.fillEllipse (px - 1.6f, py - 1.6f, 3.2f, 3.2f);
            else    g.drawEllipse (px - 1.4f, py - 1.4f, 2.8f, 2.8f, 0.9f);
        }
    }

    // ===== PRESET-NAMENSFELD =====
    // Der einzige Ort im Plugin, an dem steht, welches Preset gerade geladen
    // ist. Es ist gleichzeitig der Knopf, der die Preset-Liste oeffnet - wie
    // bei Pro-Q, wo der Name zwischen den beiden Pfeilen ebenfalls die Liste
    // aufklappt und es gar keinen Load-Knopf gibt.
    //
    // Gestaltung: ein leicht eingelassenes Feld, nicht ein weiterer Knopf.
    // Ein Feld sagt "hier steht ein Wert", ein Knopf sagt "hier passiert
    // etwas" - und der Wert ist hier das Wichtigere. Dass man es trotzdem
    // anklicken kann, verraet der kleine Pfeil rechts und der Hover-Effekt.
    //
    // Ohne geladenes Preset steht "INIT" da, dann aber deutlich gedaempfter:
    // ein Platzhalter darf nicht so aussehen wie ein echter Name.
    void drawPresetNameField (juce::Graphics& g, juce::Button& button, bool highlighted, bool down)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        const bool isEmpty = button.getProperties().getWithDefault ("presetNameEmpty", false);
        const float corner = bounds.getHeight() * 0.32f;

        // Eingelassene Flaeche
        g.setColour (juce::Colour (0xff101218).withAlpha (down ? 0.95f : 0.80f));
        g.fillRoundedRectangle (bounds, corner);

        g.setColour (juce::Colours::white.withAlpha (highlighted || down ? 0.22f : 0.11f));
        g.drawRoundedRectangle (bounds, corner, 1.0f);

        // Kleiner Aufklapp-Pfeil rechts - das Zeichen dafuer, dass hier eine
        // Liste dranhaengt.
        const float chevCX = bounds.getRight() - 11.0f;
        const float chevCY = bounds.getCentreY();
        {
            juce::Path chev;
            chev.startNewSubPath (chevCX - 3.2f, chevCY - 1.6f);
            chev.lineTo (chevCX,          chevCY + 2.0f);
            chev.lineTo (chevCX + 3.2f,   chevCY - 1.6f);
            g.setColour (juce::Colours::white.withAlpha (highlighted || down ? 0.70f : 0.40f));
            g.strokePath (chev, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Name
        auto textArea = bounds.reduced (10.0f, 0.0f).withTrimmedRight (12.0f);
        g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)).withExtraKerningFactor (0.02f));
        g.setColour (isEmpty ? juce::Colours::white.withAlpha (0.30f)
                             : juce::Colour (0xffe8ecf5).withAlpha (highlighted || down ? 1.0f : 0.92f));
        g.drawText (button.getButtonText(), textArea.toNearestInt(),
                     juce::Justification::centredLeft, true);
    }

    // ===== PRISM-ICON =====
    // User-Feedback: "Das Icon fuer Prism ist nicht gut, sieht aus wie das
    // Bypass Icon." Stimmt - es WAR das Bypass-Icon: der Button hatte
    // schlicht die Eigenschaft "bypassIcon" gesetzt. Zwei Bedienelemente mit
    // identischem Symbol nebeneinander im selben Footer ist der schlimmste
    // Fall, weil man nicht einmal merkt, dass man sich vertut.
    //
    // Jetzt ein eigenes Symbol, und zwar das woertliche: ein Prisma. Ein
    // weisser Strahl faellt von links ein, bricht sich im Dreieck und tritt
    // rechts als aufgefaecherter Farbstrahl wieder aus. Das erklaert in
    // einem Bild genau das, was die Funktion tut - aus dem vollen Spektrum
    // einen Ausschnitt herausgreifen - und es kann mit keinem anderen Icon
    // im Plugin verwechselt werden.
    //
    // Im Aus-Zustand bleiben die Farben weg (alles grau), damit der Zustand
    // auch ohne Vergleich erkennbar ist.
    void drawPrismIcon (juce::Graphics& g, juce::Button& button)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (2.5f);
        const bool sectionIsOff = button.getProperties().getWithDefault ("sectionOff", false);
        const bool isOn = button.getToggleState() && button.isEnabled() && ! sectionIsOff;
        const float enabledAlpha = button.isEnabled() ? 1.0f : 0.4f;

        const juce::Colour offGrey (iconOffColour());
        const juce::Colour beamCol = isOn ? juce::Colour (0xffc9ced8) : offGrey;   // dezenter (User)

        // Das Dreieck sitzt leicht links der Mitte, damit rechts Platz fuer
        // den aufgefaecherten Austritt bleibt.
        const float triW = bounds.getWidth()  * 0.42f;
        const float triH = bounds.getHeight() * 0.72f;
        const float triCX = bounds.getX() + bounds.getWidth() * 0.44f;
        const float triCY = bounds.getCentreY();

        juce::Path tri;
        tri.startNewSubPath (triCX,             triCY - triH * 0.5f);   // Spitze oben
        tri.lineTo          (triCX + triW*0.5f, triCY + triH * 0.5f);   // unten rechts
        tri.lineTo          (triCX - triW*0.5f, triCY + triH * 0.5f);   // unten links
        tri.closeSubPath();

        g.setColour ((isOn ? juce::Colour (0xffb7bac2) : offGrey).withAlpha (enabledAlpha));
        // Hinweis: der dritte Parameter von PathStrokeType ist der EndCapStyle,
        // NICHT ein zweiter JointStyle - das Dreieck ist geschlossen, also ist
        // butt hier ohnehin richtig. Spitze Ecken kommen aus dem JointStyle
        // "mitered" (nicht "mitred").
        g.strokePath (tri, juce::PathStrokeType (1.2f, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));

        // Eintretender Strahl von links, trifft die linke Flanke.
        const float hitX = triCX - triW * 0.16f;
        const float hitY = triCY + triH * 0.10f;
        g.setColour (beamCol.withAlpha (enabledAlpha * 0.95f));
        g.drawLine (bounds.getX(), triCY - triH * 0.06f, hitX, hitY, 1.3f);

        // Austretender Faecher: drei Strahlen mit leicht unterschiedlichen
        // Winkeln - im Ein-Zustand farbig (das ist der Wiedererkennungswert),
        // im Aus-Zustand grau.
        struct Ray { float dy; juce::Colour col; };
        const Ray rays[3] = {
            { -0.30f, isOn ? (isComicTheme() ? juce::Colour (0xffd97a62) : themePalette().prism.brighter (0.25f)) : offGrey },   // je Theme (User)
            {  0.02f, isOn ? (isComicTheme() ? juce::Colour (0xff6cc9a2) : themePalette().prism)                  : offGrey },
            {  0.34f, isOn ? (isComicTheme() ? juce::Colour (0xff6e9ddc) : themePalette().prism.darker (0.25f))   : offGrey }
        };
        const float exitX = triCX + triW * 0.20f;
        const float exitY = triCY + triH * 0.06f;
        for (const auto& r : rays)
        {
            g.setColour (r.col.withAlpha (enabledAlpha * (isOn ? 0.95f : 0.8f)));
            g.drawLine (exitX, exitY, bounds.getRight(), exitY + r.dy * bounds.getHeight(), 1.2f);
        }
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
        if (toggledOn && sectionIsOff)
            alpha = 0.80f;   // Sektion aus: Zustand sichtbar, aber keine Bewegung mehr (User)
        else if (toggledOn)
        {
            // Deutlich staerkerer Ausschlag als vorher (0.65-1.0 -> 0.28-1.0)
            // und etwas schnellerer Takt, damit das Pulsieren klar sichtbar
            // ist statt nur ein leichtes Flackern (User-Feedback: "sollen in
            // ihrer Leuchtkraft staerker pulsieren").
            const double t = juce::Time::getMillisecondCounterHiRes() * 0.001;
            constexpr double periodSeconds = 2.4;
            alpha = 0.28f + 0.72f * (float) (0.5 + 0.5 * std::sin (juce::MathConstants<double>::twoPi * t / periodSeconds));
        }
        auto baseCol = colored ? accent : iconOffColour();
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
    // Gemeinsame Knopf-Flaeche fuer die kleinen Zwei-Zustand-Knoepfe zwischen
    // den Reglern (Balance in Timewarp, Focus in Galaxy/Dimension). Ein Ort,
    // damit sie in jedem Theme gleich aussehen - auch in Pop (User).
    void drawSmallIconPlate (juce::Graphics& g, juce::Rectangle<float> b, bool on)
    {
        const float cr = juce::jmin (b.getWidth(), b.getHeight()) * 0.28f;
        // Die FLAECHE sagt nicht mehr, ob der Knopf an ist (User: "die kleinen
        // Buttons leuchten viel zu stark ... sie sollen dezent sein"). Sie ist
        // immer die ruhige Grundflaeche; an/aus steht allein im Symbol darueber.
        // Vorbild ist der RAYE-Stufenknopf bei ausgeschalteter Sektion, den der
        // User als "perfekt" bezeichnet hat.
        if (isComicTheme())
        {
            comicShadow (g, b, cr, 2.0f);
            g.setColour (juce::Colour (0xff272044));
            g.fillRoundedRectangle (b, cr);
            comicOutline (g, b, cr, 2.0f);
            return;
        }
        g.setColour (controlIdleFill());
        g.fillRoundedRectangle (b, cr);
        g.setColour (juce::Colours::white.withAlpha (on ? 0.13f : 0.07f));
        g.drawRoundedRectangle (b.reduced (0.5f), cr, 1.0f);
    }

    void drawBalanceIcon (juce::Graphics& g, juce::Button& button)
    {
        auto plate = button.getLocalBounds().toFloat().reduced (1.5f);
        const bool sectionIsOff = button.getProperties().getWithDefault ("sectionOff", false);
        // Bug (User): bei ausgeschalteter Sektion zeigte das Icon immer den
        // Aus-Zustand - ein Klick sah dadurch aus, als passiere nichts. Die
        // FORM folgt jetzt immer dem Schalter, nur die Farbe wird gedaempft.
        // Bedienbar war der Knopf schon vorher (Sektion aus heisst hier nie
        // "gesperrt", siehe Power-Icon-Kommentar).
        const bool isOn = button.getToggleState();
        const bool lit  = isOn && button.isEnabled() && ! sectionIsOff;
        drawSmallIconPlate (g, plate, lit);

        auto bounds = plate.reduced (juce::jmin (plate.getWidth(), plate.getHeight()) * 0.20f);
        auto col = smallIconColour (lit);
        if (! button.isEnabled()) col = col.withMultipliedAlpha (0.4f);
        g.setColour (col);

        // Ein Zustand, wie bei den Focus-Knoepfen (User): immer die beiden
        // Pfeile zur Mitte, an oder aus sagt allein das Leuchten. Zwei
        // verschiedene Formen waren dieselbe Information doppelt.
        const float cy = bounds.getCentreY();
        const float cx = bounds.getCentreX();
        const float headLen = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.42f;
        const float strokeW = 1.6f;
        const float gapIn   = headLen * 0.25f;
        juce::PathStrokeType st (strokeW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

        g.drawLine (bounds.getX(), cy, cx - gapIn, cy, strokeW);
        g.drawLine (bounds.getRight(), cy, cx + gapIn, cy, strokeW);
        juce::Path a;
        a.startNewSubPath (cx - gapIn - headLen * 0.75f, cy - headLen * 0.55f);
        a.lineTo (cx - gapIn, cy);
        a.lineTo (cx - gapIn - headLen * 0.75f, cy + headLen * 0.55f);
        a.startNewSubPath (cx + gapIn + headLen * 0.75f, cy - headLen * 0.55f);
        a.lineTo (cx + gapIn, cy);
        a.lineTo (cx + gapIn + headLen * 0.75f, cy + headLen * 0.55f);
        g.strokePath (a, st);
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
        // A und B in verschiedenen Farben (User-Wunsch): A violett, B tuerkis -
        // so sieht man auch aus dem Augenwinkel, welcher Slot gerade spielt.
        const juce::Colour activeA = sectionIsOff ? iconOnInOffSection() : glowAccent;
        const juce::Colour activeB = sectionIsOff ? iconOnInOffSection() : accent;
        const juce::Colour dimCol (0xff6a6e78);

        // Groesser als der Rest der globalen Zeile (User-Wunsch: "muss von
        // der Schrift her groesser sein, A und B").
        g.setFont (juce::Font (juce::FontOptions (17.0f, juce::Font::bold)).withExtraKerningFactor (0.06f));

        const float w = bounds.getWidth();
        juce::Rectangle<float> aArea      (bounds.getX(),               bounds.getY(), w * 0.38f, bounds.getHeight());
        juce::Rectangle<float> slashArea  (bounds.getX() + w * 0.38f,   bounds.getY(), w * 0.24f, bounds.getHeight());
        juce::Rectangle<float> bArea      (bounds.getX() + w * 0.62f,   bounds.getY(), w * 0.38f, bounds.getHeight());

        // Bug (User: "A...B Icon - ist das ein Bug?"): ja. drawText ersetzt
        // Text, der nicht in sein Rechteck passt, standardmaessig durch "...".
        // Seit der Button schmaler ist, war das 16%-Feld fuer den Schraegstrich
        // bei 17pt zu eng - aus "/" wurde "...". Ellipsen hier ueberall aus,
        // ein einzelnes Zeichen darf notfalls ueber sein Feld hinausragen.
        g.setColour (isA ? activeA : dimCol);
        g.drawText ("A", aArea.toNearestInt(), juce::Justification::centred, false);
        g.setColour (dimCol.withAlpha (0.7f));
        g.drawText ("/", slashArea.toNearestInt(), juce::Justification::centred, false);
        g.setColour (! isA ? activeB : dimCol);
        g.drawText ("B", bArea.toNearestInt(), juce::Justification::centred, false);
    }

    // Inhalt des globalen Mod-Bypass-Buttons: kleine Sinuswelle (dasselbe
    // Symbol wie bei den Sektions-Mod-Icons, damit der Bezug zu "Modulation"
    // sofort klar ist) + der Text "OFF" daneben - die Kombination sagt klar
    // "das hier schaltet Modulation aus", statt nur "MOD" (User-Feedback:
    // "nicht klar was er macht von Namen her"). "OFF" leuchtet auffaellig
    // rot, wenn die Modulation gerade tatsaechlich global stumm ist; die
    // Welle selbst pulsiert sanft (wie die anderen Mod-Icons), solange die
    // Modulation normal laeuft.
    // Bug-Fix (User-Feedback: "wenn Mod global off ist, ist der Text
    // abgeschnitten") - der ON/OFF-Text ist komplett entfernt. Statt eines
    // Text-Labels zeigt jetzt der Wellen-Stil selbst eindeutig an, ob Mod
    // aktiv ist. Weiterer Feinschliff (User-Wunsch): "on -> gruenes Mod Icon
    // soll sich horizontal leicht ausdehnen und wieder zusammenziehen" (echtes
    // "Atmen" per horizontaler Stauchung/Dehnung um die Mitte, nicht nur
    // Alpha-Pulsieren) - "off -> statisch, keine Bewegung; ein Strich
    // durchziehen oder ein Stoppschild ueber die rote Welle" (statische,
    // feine DURCHGEHENDE rote Welle PLUS ein diagonaler Strich quer
    // drueber, wie ein Verbotssymbol - die fruehere gestrichelte Variante
    // wirkte zu grob, User-Wunsch: "rote Linie feiner, durchgehender Sinus
    // + durchgestrichene Linie"). Dickerer Strich als vorher (1.5 -> 2.2px) und
    // etwas groessere Amplitude, damit das Icon genauso praesent wirkt wie
    // die Mutate-/Breathe-Icons links davon (User-Wunsch: "Button soll so
    // gross sein wie die anderen Buttons links davon").
    void drawModBypassContent (juce::Graphics& g, juce::Button& button)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (6.0f, 5.0f);
        const bool sectionIsOff = button.getProperties().getWithDefault ("sectionOff", false);
        // bypassOn = true bedeutet: globale Modulation ist AUS (der Parameter
        // heisst "Bypass"). modActive ist die fuer den User verstaendlichere
        // Umkehrung ("Mod steht an/laeuft gerade").
        const bool bypassOn = button.getToggleState() && ! sectionIsOff;
        const bool modActive = ! bypassOn;

        static const juce::Colour onGreen (0xff3ddc73);
        static const juce::Colour offRed  (0xffff5b5b);

        const float centreX = bounds.getCentreX();
        // Horizontale "Atem"-Stauchung/Dehnung, NUR wenn Mod aktiv laeuft -
        // im Aus-Zustand bleibt die Welle komplett statisch (User-Wunsch).
        float stretch = 1.0f;
        if (! sectionIsOff && modActive)
        {
            const double t = juce::Time::getMillisecondCounterHiRes() * 0.001;
            constexpr double periodSeconds = 2.6;
            stretch = 1.0f + 0.16f * (float) std::sin (juce::MathConstants<double>::twoPi * t / periodSeconds);
        }

        juce::Path wave;
        const int steps = 24;
        const float w = bounds.getWidth(), h = bounds.getHeight();
        for (int i = 0; i <= steps; ++i)
        {
            const float tt = (float) i / (float) steps;
            const float rawX = bounds.getX() + tt * w;
            const float x = centreX + (rawX - centreX) * stretch;
            const float y = bounds.getCentreY() - std::sin (tt * juce::MathConstants<float>::twoPi) * (h * 0.38f);
            if (i == 0) wave.startNewSubPath (x, y); else wave.lineTo (x, y);
        }

        if (sectionIsOff)
        {
            g.setColour (iconOffColour());
            g.strokePath (wave, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else if (modActive)
        {
            const double t = juce::Time::getMillisecondCounterHiRes() * 0.001;
            constexpr double periodSeconds = 2.4;
            const float pulse = 0.55f + 0.45f * (float) (0.5 + 0.5 * std::sin (juce::MathConstants<double>::twoPi * t / periodSeconds));
            g.setColour (onGreen.withAlpha (pulse));
            g.strokePath (wave, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else
        {
            // ===== "Mod aus" = FLACHE LINIE, kein Querstrich =====
            // Die vorherige Loesung (gestrichelte bzw. feine Welle PLUS ein
            // diagonaler Strich darueber) ist ersatzlos entfernt.
            // Grund, und der wiegt schwer: der kurze diagonale Strich ueber
            // der Sinuskurve ergab zusammen eine Form, die einem Hakenkreuz
            // aehnlich sah. Das ist inakzeptabel, und zwar unabhaengig von
            // Winkel oder Laenge - jede Variante "Diagonale ueber Welle"
            // bleibt in dieser Gefahr. Deshalb faellt das Konzept komplett
            // weg statt nachjustiert zu werden.
            //
            // Die Alternative ist ohnehin die bessere Ikonografie: Modulation
            // ist Bewegung, also zeigt "an" eine Welle und "aus" eine flache
            // Linie. Das ist genau die Sprache, die man von LFO-Anzeigen
            // kennt, braucht keinen zusaetzlichen Marker, kann nie mit der
            // Welle verschmelzen und ist auf einen Blick eindeutig.
            const float midY = bounds.getCentreY();
            g.setColour (offRed);
            g.drawLine (bounds.getX(), midY, bounds.getRight(), midY, 2.0f);
        }
    }

    // Inhalt des globalen Bypass-Buttons (BYP): dasselbe Power-Ring-Icon wie
    // die Section-Bypass-Schalter - rot, wenn das gesamte Plugin gerade per
    // UI-Bypass stummgeschaltet ist, sonst gruen (wie "an" bei Mod On/Off).
    // Kein Text mehr (User-Wunsch: "davor stand dort 'BYP' - das kann aber
    // weg, weil das Icon alleine schon reicht") - der Button ist jetzt nur
    // noch schmal (siehe kIconBtnW in layoutContent()), das Icon nutzt
    // die komplette Flaeche statt nur die linke Haelfte.
    void drawBypassToggleContent (juce::Graphics& g, juce::Button& button)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (3.0f);
        const bool bypassed = button.getToggleState();

        static const juce::Colour onGreen (0xff3ddc73);
        static const juce::Colour offRed  (0xffff5b5b);
        auto col = bypassed ? offRed : onGreen;

        g.setColour (col);
        auto centre = bounds.getCentre();
        const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        juce::Path ring;
        ring.addCentredArc (centre.x, centre.y, r * 0.62f, r * 0.62f, 0.0f,
                             juce::MathConstants<float>::pi * 0.28f,
                             juce::MathConstants<float>::twoPi - juce::MathConstants<float>::pi * 0.28f, true);
        g.strokePath (ring, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.drawLine (centre.x, centre.y - r * 0.75f, centre.x, centre.y - r * 0.15f, 1.6f);
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
        // "mutateBig" (Runde 50): der Wuerfel ersetzt bei aktiver Smart-
        // Kategorie beide Knoepfe und wird entsprechend deutlich groesser.
        const bool bigCube = button.getProperties().getWithDefault ("mutateBig", false);
        auto bounds = button.getLocalBounds().toFloat().reduced (bigCube ? 16.0f : 10.0f, bigCube ? 2.0f : 6.0f);
        const int colorState = (int) button.getProperties().getWithDefault ("mutateColorState", 0b0101);

        // Ist eine Smart-Kategorie gewaehlt, bekommt dieser Wuerfel einen
        // dezenten Hof - man sieht dann sofort, dass die Kategorie hier oben
        // wirkt und der andere Wuerfel gesperrt ist (User).
        if (button.getProperties().getWithDefault ("categoryArmed", false))
        {
            const auto c = button.getLocalBounds().toFloat().getCentre();
            for (int layer = 3; layer >= 1; --layer)
            {
                const float rr = bounds.getWidth() * (0.55f + 0.22f * (float) layer);
                g.setColour (themePalette().chip.withAlpha (0.055f * (float) (4 - layer)));
                g.fillEllipse (c.x - rr, c.y - rr * 0.8f, rr * 2.0f, rr * 1.6f);
            }
        }

        const float cell = juce::jmin (bounds.getWidth() * 0.42f, bounds.getHeight() * (bigCube ? 0.60f : 0.42f));
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

                // Variante "Mutate inkl. Sektionen" (zweite Taste): zwei der
                // vier Kaestchen werden grau statt farbig gezeichnet. Das
                // vermittelt ohne Text, dass dieser Wurf auch Sektionen
                // AUSSCHALTEN darf, waehrend die normale Taste alle anlaesst.
                // Bewusst dieselbe Bildsprache (gleiches 2x2-Raster, gleiche
                // Groesse) und bewusst FESTE Positionen (Diagonale) - haette
                // man die grauen Felder mitgewuerfelt, waere das Icon nicht
                // mehr wiedererkennbar.
                const bool sectionsVariant = button.getProperties().getWithDefault ("mutateSectionsIcon", false);
                const bool greyed = sectionsVariant && (bitIndex == 1 || bitIndex == 2);

                const bool isPurple = ((colorState >> bitIndex) & 1) != 0;
                g.setColour (greyed ? juce::Colour (0xff4a4e57)
                                    : (isPurple ? glowAccent : accent));
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

        // ===== SCHLICHTER CHEVRON (Preset vor/zurueck) =====
        // Die Preset-Pfeile teilen sich zwar den Hover-Rahmen mit Undo/Redo,
        // duerfen aber NICHT wie sie aussehen: ein Undo-Bogen bedeutet
        // "rueckgaengig", und das waere hier schlicht gelogen - die Pfeile
        // blaettern nur durch die Preset-Liste. Ein einfacher Winkel ist das
        // uebliche Zeichen dafuer und in dieser Groesse (13px) ausserdem das
        // einzige, was ueberhaupt noch lesbar ist.
        if (button.getProperties().getWithDefault ("chevronArrow", false))
        {
            auto b = button.getLocalBounds().toFloat().reduced (3.5f);
            const bool forward = button.getProperties().getWithDefault ("arrowForward", true);
            auto colC = juce::Colour (0xffb5b9c2).withAlpha (button.isEnabled() ? 1.0f : 0.3f);
            if (button.isEnabled() && (down || highlighted))
                colC = juce::Colours::white;

            const float cyM = b.getCentreY();
            const float halfH = juce::jmin (b.getHeight(), b.getWidth() * 1.6f) * 0.34f;
            const float x0 = forward ? b.getX() + b.getWidth() * 0.30f : b.getRight() - b.getWidth() * 0.30f;
            const float x1 = forward ? b.getRight() - b.getWidth() * 0.22f : b.getX() + b.getWidth() * 0.22f;

            juce::Path chev;
            chev.startNewSubPath (x0, cyM - halfH);
            chev.lineTo (x1, cyM);
            chev.lineTo (x0, cyM + halfH);

            g.setColour (colC);
            g.strokePath (chev, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            return;
        }

        auto bounds = button.getLocalBounds().toFloat().reduced (5.0f);
        const bool isUndo = button.getProperties().getWithDefault ("arrowDirection", "left").toString() == "left";
        auto col = juce::Colour (0xffb5b9c2).withAlpha (button.isEnabled() ? 1.0f : 0.3f);
        if (button.isEnabled() && (down || highlighted))
            col = juce::Colours::white;

        const float s = juce::jmin (bounds.getWidth(), bounds.getHeight());
        auto c = bounds.getCentre();
        const float r = s * 0.36f;

        // Winkel-Konvention von addCentredArc: 0 Grad = oben (12 Uhr),
        // steigend im Uhrzeigersinn.
        // Bogen deutlich verkuerzt (vorher 40..320 Grad = 280 Grad, also fast
        // ein voller Kreis - User-Feedback: "Undo und Redo Icons erkennt man
        // nicht gut, der Pfeil ist zu lange, sieht fast aus wie ein Kreis").
        // Jetzt 175 Grad: ein klar lesbarer Halbkreis-Haken statt eines
        // geschlossen wirkenden Rings. Das Bogen-ENDE (toDeg) bleibt
        // unveraendert, damit Pfeilspitze und Undo-Spiegelung unveraendert
        // korrekt bleiben - nur der Anfang wandert nach hinten.
        // Zweite Korrektur (User: "Undo/Redo immer noch nicht gut erkennbar.
        // Versuch mal die Pfeile bis 12 Uhr zu machen und die Spitze noch
        // klarer erkennbar."): Der Bogen endet jetzt exakt bei 12 Uhr
        // (360 Grad). Dort ist die Tangente genau waagerecht, die Spitze
        // zeigt also sauber nach rechts (Redo) bzw. nach links (Undo, per
        // Spiegelung) - das ist die kanonische Form, die man von Undo/Redo
        // kennt. Vorher endete er bei 320 Grad, wo die Spitze schraeg nach
        // oben zeigte und dadurch weniger eindeutig war.
        constexpr float fromDeg = 170.0f;
        constexpr float toDeg   = 360.0f;

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
        // Kopf nochmals deutlich vergroessert (0.36/0.34 -> 0.46/0.44) -
        // die Spitze ist das Einzige, was einen Pfeil von einem Kreisbogen
        // unterscheidet, also darf sie ruhig dominant sein.
        const float headLen = s * 0.46f, headW = s * 0.44f;
        // Der Kopf sitzt jetzt KOMPLETT hinter dem Bogenende statt mittig
        // darauf (User: "Pfeilspitzen erst nach 12 Uhr. Momentan hoeren sie
        // bei 12 Uhr auf. Sie sollen dort beginnen."). Vorher lag die halbe
        // Spitze noch auf dem Bogen und wurde von ihm verschluckt. Die
        // Rueckkante ueberlappt den Bogen nur minimal, damit keine Luecke
        // zwischen Bogen und Spitze entsteht.
        const auto tipFwd  = tip + dir * headLen;
        const auto backC   = tip - dir * (headLen * 0.12f);
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
        g.strokePath (arc, juce::PathStrokeType (s * 0.12f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
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
        // View-Panel-Regler (Zahnrad im Sternenfeld): schmaler waagerechter
        // Schlitz mit Fuellung und rundem Griff - flach, ohne Textbox.
        if (slider.getProperties().getWithDefault ("viewSlider", false) && style == juce::Slider::LinearHorizontal)
        {
            auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
            const float cy = bounds.getCentreY();
            const float trackH = 4.0f;
            auto track = juce::Rectangle<float> (bounds.getX() + 6.0f, cy - trackH * 0.5f, bounds.getWidth() - 12.0f, trackH);
            const float px = juce::jlimit (track.getX(), track.getRight(), sliderPos);
            if (slider.getProperties().getWithDefault ("hueSlider", false))
            {
                // Farbton-Regler: erste 4% grau = Standardfarbe, danach der
                // Regenbogen. Die Fuellung entfaellt - die Farbe unter dem
                // Griff IST der Wert.
                const float grayW = track.getWidth() * 0.04f;
                g.setColour (juce::Colour (0xff6a6e78));
                g.fillRoundedRectangle (track.withWidth (grayW), 2.0f);
                juce::ColourGradient rainbow (juce::Colour::fromHSV (0.0f, 0.7f, 0.95f, 1.0f), track.getX() + grayW, cy,
                                              juce::Colour::fromHSV (1.0f, 0.7f, 0.95f, 1.0f), track.getRight(), cy, false);
                for (int i = 1; i < 6; ++i)
                    rainbow.addColour ((double) i / 6.0, juce::Colour::fromHSV ((float) i / 6.0f, 0.7f, 0.95f, 1.0f));
                g.setGradientFill (rainbow);
                g.fillRect (track.withTrimmedLeft (grayW));
            }
            else if (slider.getProperties().getWithDefault ("viewCentre", false))
            {
                // Standardwert in der Mitte: Fuellung laeuft von der Mitte
                // zum Griff, die Mitte selbst ist als Strich markiert.
                const float mx = track.getCentreX();
                g.setColour (juce::Colour (0xff3a3d45));
                g.fillRoundedRectangle (track, 2.0f);
                g.setColour (glowAccent.withAlpha (0.85f));
                if (px >= mx)
                    g.fillRoundedRectangle (juce::Rectangle<float> (mx, track.getY(), px - mx, trackH), 2.0f);
                else
                    g.fillRoundedRectangle (juce::Rectangle<float> (px, track.getY(), mx - px, trackH), 2.0f);
                g.setColour (juce::Colours::white.withAlpha (0.55f));
                g.fillRect (mx - 0.75f, cy - 6.0f, 1.5f, 12.0f);
            }
            else
            {
                g.setColour (juce::Colour (0xff3a3d45));
                g.fillRoundedRectangle (track, 2.0f);
                g.setColour (glowAccent.withAlpha (0.85f));
                g.fillRoundedRectangle (track.withRight (px), 2.0f);
            }
            g.setColour (juce::Colour (0xffe8ecf5));
            g.fillEllipse (px - 5.0f, cy - 5.0f, 10.0f, 10.0f);
            g.setColour (juce::Colour (0xff14161a));
            g.fillEllipse (px - 2.0f, cy - 2.0f, 4.0f, 4.0f);
            return;
        }

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

        // Runde 50 (User): kein Kegel mehr, sondern ein normaler senkrechter
        // Fader. Die FARBE wandert mit dem Wert - unten die ruhige
        // Akzentfarbe, oben die kraeftige Glow-Farbe. Damit sieht man den
        // Stand auch dann, wenn die Fuellhoehe im Augenwinkel untergeht.
        const float trackW = juce::jmin (bounds.getWidth() * 0.34f, 14.0f);
        auto track = juce::Rectangle<float> (cx - trackW * 0.5f, top, trackW, bottom - top);
        const float trackR = trackW * 0.5f;

        g.setColour (juce::Colour (0xff23262c));
        g.fillRoundedRectangle (track, trackR);
        g.setColour (offVisual ? knobRingOffColour() : juce::Colour (0xff454952));
        g.drawRoundedRectangle (track.reduced (0.5f), trackR, 1.2f);

        // Fuellung von unten bis zur aktuellen Position.
        const float fillTopY = juce::jlimit (top, bottom, sliderPos);
        const float t = juce::jlimit (0.0f, 1.0f, (bottom - fillTopY) / juce::jmax (1.0f, bottom - top));

        auto valueCol = offVisual ? knobValueOffColour() : accent.interpolatedWith (glowAccent, t);
        if (t > 0.001f)
        {
            juce::Path fill;
            fill.addRoundedRectangle (track.getX(), fillTopY, track.getWidth(), bottom - fillTopY,
                                      trackR, trackR, false, false, true, true);
            g.setColour (valueCol.withAlpha (0.90f));
            g.fillPath (fill);
        }

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
            const float liveHalfWidth = trackW * 0.5f + 3.0f;

            g.setColour (glowAccent.withAlpha (0.30f));
            g.drawLine (cx - liveHalfWidth, liveY, cx + liveHalfWidth, liveY, 5.0f);
            g.setColour (glowAccent);
            g.drawLine (cx - liveHalfWidth, liveY, cx + liveHalfWidth, liveY, 2.0f);
        }

        // Aktuelle Position als leuchtender Punkt - ebenfalls ueber der Live-
        // Linie, bleibt also immer als eigener (tuerkiser) Punkt erkennbar.
        auto dotCol = offVisual ? juce::Colour (0xff777b85) : valueCol;
        g.setColour (dotCol.withAlpha (0.25f));
        g.fillEllipse (cx - 9.0f, sliderPos - 9.0f, 18.0f, 18.0f);
        g.setColour (juce::Colours::white);
        g.fillEllipse (cx - 2.5f, sliderPos - 2.5f, 5.0f, 5.0f);
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

        // Bars-Feld folgt jetzt dem Theme wie alles andere (User: "das Bars-
        // Menu sollte sich auch ans Theme anpassen, bei ALLEN Themes") - die
        // festen Werte 20232a / 33363f waren ein Fremdkoerper in jedem Theme
        // ausser dem urspruenglichen.
        const bool boxSectionOff = box.getProperties().getWithDefault ("sectionOff", false);
        if (! boxSectionOff)
        {
            g.setColour (controlIdleFill());
            g.fillRoundedRectangle (bounds, 6.0f);
        }

        const bool goldBox = box.getProperties().getWithDefault ("pairedGold", false);
        const juce::Colour boxOutline = isComicTheme() ? comicInk()
                                      : boxSectionOff ? iconOffColour().withAlpha (0.35f)
                                      : goldBox       ? pairAccentColour()
                                      : glow          ? glowAccent
                                                      : themePalette().frameMain.withAlpha (0.55f);
        g.setColour (boxOutline);
        g.drawRoundedRectangle (bounds, 6.0f, (glow || goldBox) ? 1.6f : 1.0f);

        if (box.getProperties().getWithDefault ("noArrow", false))
            return;   // ohne Pfeil (User: jeder erkennt ein Dropdown)
        juce::Rectangle<int> arrowZone (width - 22, 0, 18, height);
        juce::Path path;
        path.startNewSubPath ((float) arrowZone.getX() + 3.0f, (float) arrowZone.getCentreY() - 3.0f);
        path.lineTo ((float) arrowZone.getCentreX(), (float) arrowZone.getCentreY() + 3.0f);
        path.lineTo ((float) arrowZone.getRight() - 3.0f, (float) arrowZone.getCentreY() - 3.0f);
        g.setColour (boxSectionOff ? iconOffColour() : juce::Colour (0xff9ba0aa));
        g.strokePath (path, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Ohne Pfeil gehoert die ganze Breite dem Text, und er steht mittig.
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        if (box.getProperties().getWithDefault ("noArrow", false))
        {
            label.setBounds (1, 1, box.getWidth() - 2, box.getHeight() - 2);
            label.setFont (getComboBoxFont (box));
            label.setJustificationType (juce::Justification::centred);
            return;
        }
        juce::LookAndFeel_V4::positionComboBoxText (box, label);
    }

    juce::Colour accent { 0xff5be3c7 };
    juce::Colour glowAccent { 0xffb968ff };
};
