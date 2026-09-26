#pragma once

#ifndef SPACEX_PX_DIAG_ONLY
 #define SPACEX_PX_DIAG_ONLY 0
#endif
#include <JuceHeader.h>
#include "../DSP/SideEq.h"

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
inline UiTheme& uiThemeRef()   { static UiTheme t = UiTheme::DayNight; return t; }   // Runde 66 (User): Day & Night ist das Standard-Theme
inline bool     isDarkNightTheme() { return uiThemeRef() == UiTheme::DarkNight; }
inline bool     isWaterTheme() { return isDarkNightTheme(); }   // Watercolor-Material (Nebula-Platte, Korn)
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
// Runde 174 (User): "Brightness" im Settings-Panel, 0..1, global wie das Theme.
// Wirkt als EINE helle Schicht ueber der ganzen Oberflaeche (ausser dem
// Sternenfeld), siehe paintOverContent - jedes Element hebt sich im selben
// Verhaeltnis, An/Aus-Zustaende bleiben untereinander exakt gleich.
inline float& uiBrightnessRef() { static float b = 0.0f; return b; }
// Runde 174 (User, Beta-Vergleich): L/R als Orbit statt als Balken (Settings).
inline bool& uiLrOrbitRef() { static bool b = false; return b; }
// Runde 175 (User, Beta-Vergleich): Width-Regler mit Keil statt Zeiger (Settings).
inline bool& uiWidthWedgeRef() { static bool b = false; return b; }
inline bool  layoutFrameless()  { return uiLayoutRef() == 1; }
inline bool  layoutOutline()    { return uiLayoutRef() == 2; }
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
inline ThemePalette themePaletteRaw()
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
            // Runde 75 (User): der Titelton eine Spur zurueckgenommen
            // (d9689a -> b8547f). In Sci-Fi trugen Titel und Modus-Pillen
            // exakt dasselbe Pink, die Pillen verschwanden dadurch in den
            // Ueberschriften - in den anderen Themes stehen sie klar davor.
            // Jetzt ist das Gefaelle ueberall gleich.
            return { juce::Colour (0xff8be9ff), juce::Colour (0xffe07aa8), juce::Colour (0xff7d63c9), juce::Colour (0xff7d63c9),
                     juce::Colour (0xffd9689a), juce::Colour (0xff8e70c6), juce::Colour (0xff8e70c6), juce::Colour (0xff14111e),
                     juce::Colour (0xffb8547f) };
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

// Runde 174 (User: "das ganze Plugin bei allen Themes etwas zu dunkel ...
// so ein Ticken heller"): die Platte aller Themes hebt sich um 3 % Richtung
// Weiss. Alles, was aus der Platte abgeleitet ist (Sektionsflaechen,
// Felder), zieht automatisch mit - der Charakter bleibt, nur die Tiefen
// werden etwas offener.
inline ThemePalette themePalette()
{
    auto p = themePaletteRaw();
    p.plate = p.plate.interpolatedWith (juce::Colours::white, 0.03f);
    return p;
}

// Farbe der RAYE-Pair-Kopplung (Pair-Knopf + gekoppelte Hyperdrive-Teile).
// Sci-Fi (User, Runde 38): die blaue Akzentfarbe - sonst war Pair genau so
// pink wie Sync in Hyperdrive und nicht zu unterscheiden.
// Gold, das FAST in den hellen Themes traegt (Runde 61).
inline juce::Colour altAccentColour()
{
    return themePalette().frameRaye.withMultipliedSaturation (isWaterTheme() ? 0.6f : 1.0f);
}
inline juce::Colour pairAccentColour()
{
    // Runde 61 (User): in Moon, Day & Night und Fireflies waren FAST und PAIR
    // vertauscht - PAIR ist die wichtigere Aussage ("das Tempo kommt von
    // woanders") und bekommt deshalb ueberall die Hauptakzentfarbe. FAST
    // traegt dafuer das Gold. In Sci-Fi war es schon richtig, dort aendert
    // sich nichts. Gilt genauso fuer SYNC in Autopan, das dieselbe Farbe
    // benutzt.
    if (isSciFiTheme())
        return themePalette().knob;
    return themePalette().mod;
}
inline juce::Colour iconOffColour()
{
    // EIN einziges "grayed out" fuer alle Themes (User: "einheitliches grayed
    // out"). Frueher hatte jede Theme-Familie ihren eigenen Grauton - genau
    // das war die Uneinheitlichkeit. Jetzt wird der Ton aus der Plattenfarbe
    // abgeleitet: derselbe Abstand zur UI in jedem Theme, also ueberall
    // derselbe Eindruck, ohne dass ein kalter Grauton in einem warmen Theme
    // (oder umgekehrt) als Fremdkoerper sitzt.
    return themePalette().plate.interpolatedWith (juce::Colours::white, 0.30f);
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
inline juce::Colour controlOnFill()  { return themePalette().plate.interpolatedWith (juce::Colour (0xff2a1f33), 0.5f); }
// Neutrale Flaeche fuer Felder, die kein An/Aus kennen (Bars).
inline juce::Colour controlIdleFill(){ return themePalette().plate.interpolatedWith (juce::Colours::white, 0.045f); }

// Sektionsflaeche der flachen Themes (Sci-Fi, Moon); wird mit 62 % (an)
// bzw. 30 % (aus) ueber die Platte gelegt.
inline juce::Colour themeSurfaceRaw()
{
    switch (uiThemeRef())
    {
        case UiTheme::SciFi:
        case UiTheme::SciFiDark:    return juce::Colour (0xff1b1630);   // dunkler (User: "zu hell")
        case UiTheme::Flat:         return juce::Colour (0xff222327);
        default:                    return juce::Colour (0xff1e2434);
    }
}
inline juce::Colour themeSurface()   // Runde 174: 3 % heller, wie die Platte (siehe themePalette)
{
    return themeSurfaceRaw().interpolatedWith (juce::Colours::white, 0.03f);
}
// Runde 66 (User): Fuellung der Sektionen im Outline-Layout, JE THEME.
// Outline ist die neue Basis (klare Kontur, kein Glow) - wie viel Flaeche
// darunter noetig ist, unterscheidet sich aber deutlich:
//   Day & Night  passt so, wie es ist (User: "sieht super aus").
//   Fairy Tale   war in 3D der klare Sieger - hier darf die Flaeche kraeftiger
//                sein, damit die Sektionen dieselbe Tiefe bekommen.
//   Sci-Fi       traegt seinen Look ueber die violetten Rahmen; die Flaeche
//                bleibt ein Hauch in Richtung derselben Farbe (User:
//                "miniminiminimal").
struct OutlineFill { juce::Colour colour; float alpha; };
inline OutlineFill outlineFill()
{
    switch (uiThemeRef())
    {
        case UiTheme::DayNight:  return { juce::Colour (0xff141826), 0.21f };
        case UiTheme::DarkNight: return { juce::Colour (0xff13152a), 0.40f };   // Fairy Tale
        case UiTheme::SciFi:
        case UiTheme::SciFiDark: return { juce::Colour (0xff271c42), 0.15f };   // Richtung Rahmenviolett
        default:                 return { juce::Colour (0xff141826), 0.21f };
    }
}
// "Wash"-Sektionen: Fuellfarbe + Deckkraft fuer an / aus je Theme.
struct WashFill { juce::Colour on, off; float aOn, aOff; };
inline WashFill washFillRaw()
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
inline WashFill washFill()   // Runde 174: 3 % heller, wie die Platte (siehe themePalette)
{
    auto w = washFillRaw();
    w.on = w.on.interpolatedWith (juce::Colours::white, 0.03f);
    return w;
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
    return themePalette().plate;
}
inline juce::Colour knobRingOffColour()  { return sectionOffFill().withMultipliedBrightness (1.15f).interpolatedWith (juce::Colours::white, 0.025f); }   // noch naeher an der Flaeche (User)
inline juce::Colour knobValueOffColour() { return knobRingOffColour().interpolatedWith (juce::Colours::white, 0.08f); }
inline juce::Colour knobCentreOffColour(){ return sectionOffFill().withMultipliedBrightness (0.85f); }
// Runde 66 (User): der Reglerkopf war ein fester Wert (1c1e23). Auf der fast
// schwarzen 3D-Flaeche wirkte er dadurch erhaben, auf der etwas helleren
// Outline-Flaeche versank er - beim Gravity-Regler schwebte der Zeiger frei.
// Jetzt leitet er sich aus der Platte ab und liegt IMMER einen Schritt ueber
// dem, worauf er sitzt. Das ist der "Koerper", der 3D interessanter machte -
// er bleibt also erhalten, auch wenn der Rahmen klar und flach wird.
inline juce::Colour knobCapColour()
{
    return themePalette().plate.interpolatedWith (juce::Colours::white, isWaterTheme() ? 0.10f : 0.075f);
}
// Schrift bei ausgeschalteter Sektion: noch naeher an die UI-Farbe heran
// (User, Runde 22: "Schrift der Regler und Buttons Richtung UI-Hintergrund").
inline juce::Colour labelOffColour()     { return sectionOffFill().interpolatedWith (juce::Colours::white, 0.13f); }

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
        // Runde 113 (User): ist Autopan-Speed an den Phaser gekoppelt (LINK),
        // liegt ein leichter blauer Schein UM den Regler - ein weicher Ring,
        // hinter allem anderen gezeichnet. Pop behaelt seinen Tinten-Ring.
        if ((bool) slider.getProperties().getWithDefault ("pairedGold", false) 
            && ! (bool) slider.getProperties().getWithDefault ("sectionOff", false))
        {
            const float halo = juce::jmin ((float) juce::jmin (width, height) * 0.5f, radius * 1.32f);
            const auto blue = pairAccentColour();
            juce::ColourGradient ringGlow (blue.withAlpha (0.0f), centre.x, centre.y,
                                           blue.withAlpha (0.0f), centre.x + halo, centre.y, true);
            ringGlow.addColour (0.62, blue.withAlpha (0.0f));
            ringGlow.addColour (0.80, blue.withAlpha (0.20f));
            g.setGradientFill (ringGlow);
            g.fillEllipse (centre.x - halo, centre.y - halo, halo * 2.0f, halo * 2.0f);
        }
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
        const bool offVisual = slider.getProperties().getWithDefault ("sectionOff", false) || ! slider.isEnabled()
                            || (bool) slider.getProperties().getWithDefault ("syncLocked", false);   // Runde 130

        // Runde 110 (User): die Spur eine Spur duenner (Pop behaelt seine).
        float trackThickness = radius * 0.16f;


        juce::Path track;
        track.addCentredArc (centre.x, centre.y, radius - trackThickness, radius - trackThickness,
                              0.0f, rotaryStartAngle, rotaryEndAngle, true);
        // Etwas heller als die getönten Gruppenrahmen dahinter, damit der
        // unlackierte Ring nicht optisch verschwindet.
        const juce::Colour ringOnCol = isSciFiTheme() ? juce::Colour (0xff3a2f5a)   // Sci-Fi an: dunkles Violett statt Grau (User, Option A)
                                                      : juce::Colour (0xff454952);
        if (offVisual)
            g.setColour (knobRingOffColour());   // aus: kaum heller als die Off-Fuellfarbe der Sektion (User, alle Themes)
        else
        {
            // Runde 174 (User, Entwurf E4): die Bahn einer eingeschalteten
            // Sektion traegt am Anfang 19 % Theme-Farbe (User: 30 % minus ein Drittel) und laeuft zum Ende
            // hin ins normale Ringgrau aus - der Knopf wirkt "bereit" und
            // zeigt die Richtung, auch bei 0 %.
            const float tr = radius - trackThickness;
            const juce::Point<float> pS (centre.x + tr * std::sin (rotaryStartAngle), centre.y - tr * std::cos (rotaryStartAngle));
            const juce::Point<float> pE (centre.x + tr * std::sin (rotaryEndAngle),   centre.y - tr * std::cos (rotaryEndAngle));
            g.setGradientFill (juce::ColourGradient (ringOnCol.interpolatedWith (themePalette().knob, 0.19f), pS,
                                                     ringOnCol, pE, false));
        }
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

            g.setColour (knobCapColour());
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
        // Runde 110 (User): PAIR zeigt sich am Speed-Regler nicht mehr als
        // voller Ring, sondern nur als blauer Wertebogen (blau = gekoppelt).
        if (! offVisual && (bool) slider.getProperties().getWithDefault ("pairedGold", false))
            col = pairAccentColour();
        const bool hfSparkle = ! offVisual && (bool) slider.getProperties().getWithDefault ("hfSparkle", false);
        if (hfSparkle && ! centerOut && angle - rotaryStartAngle > 0.001f)
        {
            // Runde 175 (User, HF Regain): der Bogen bleibt golden, nur vom
            // Zeiger aus rueckwaerts laeuft ein Stueck ins Blau - je hoeher der
            // Wert, desto laenger. Gold = Mitte, Blau = Seiten.
            const float r2   = radius - trackThickness;
            const float frac = 0.15f + 0.45f * sliderPos;
            constexpr int kSeg = 40;
            for (int i = 0; i < kSeg; ++i)
            {
                const float q0 = (float) i / kSeg, q1 = (float) (i + 1) / kSeg, q = 0.5f * (q0 + q1);
                float bl = juce::jlimit (0.0f, 1.0f, (q - (1.0f - frac)) / frac);
                bl = bl * bl * (3.0f - 2.0f * bl);
                juce::Path seg;
                seg.addCentredArc (centre.x, centre.y, r2, r2, 0.0f,
                                   rotaryStartAngle + (angle - rotaryStartAngle) * q0,
                                   rotaryStartAngle + (angle - rotaryStartAngle) * q1 + 0.004f, true);
                g.setColour (col.interpolatedWith (glowAccent, bl));
                g.strokePath (seg, juce::PathStrokeType (trackThickness, juce::PathStrokeType::curved,
                                                         (i == 0 || i == kSeg - 1) ? juce::PathStrokeType::rounded : juce::PathStrokeType::butt));
            }
            col = glowAccent;   // Schimmer an der Spitze in Blau
        }
        else
        {
            g.setColour (col);
            g.strokePath (value, juce::PathStrokeType (trackThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        // Runde 110: ein winziger Schimmer an der Spitze des Wertebogens -
        // dieselbe Sprache wie die Icon-Felder, ganz leise.
        if (! offVisual && ! value.isEmpty())
        {
            const float tipR = radius - trackThickness;
            const juce::Point<float> tip (centre.x + tipR * std::sin (angle), centre.y - tipR * std::cos (angle));
            softIconGlow (g, tip, trackThickness * 2.2f, col, 0.9f);
        }

        const bool wedgeMode = uiWidthWedgeRef() && (bool) slider.getProperties().getWithDefault ("widthWedge", false);
        if (! wedgeMode)
        {
            float pointerLength = radius * 0.55f;
            juce::Path pointer;
            pointer.startNewSubPath (centre.x, centre.y);
            pointer.lineTo (centre.x + pointerLength * std::sin (angle), centre.y - pointerLength * std::cos (angle));
            g.setColour (juce::Colours::white.withAlpha (offVisual ? 0.30f : 1.0f));
            g.strokePath (pointer, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            g.setColour (offVisual ? knobCentreOffColour() : knobCapColour());
            g.fillEllipse (centre.x - radius * 0.28f, centre.y - radius * 0.28f, radius * 0.56f, radius * 0.56f);
        }
        else
        {
            // Runde 175 (User, Width Variante B): statt des Zeigers ein Keil in
            // der Knopfmitte - Spitze unten, oeffnet nach oben. Width geht
            // 50..200 %: bei 50 % halb so breit wie das Original, nie ein
            // Strich. Die gestrichelte Linie zeigt 100 %.
            const float inner = radius - trackThickness * 1.9f;
            g.setColour (offVisual ? knobCentreOffColour() : knobCapColour());
            g.fillEllipse (centre.x - inner, centre.y - inner, inner * 2.0f, inner * 2.0f);
            const float w    = juce::jlimit (0.5f, 2.0f, (float) slider.getValue() * 0.01f);
            const float len  = inner * 1.25f;
            const float yTip = centre.y + inner * 0.62f, yTop = yTip - len;
            const float half = 0.46f * w * len * 0.55f;
            const float hO   = 0.46f * len * 0.55f;
            juce::Path clipC; clipC.addEllipse (centre.x - inner, centre.y - inner, inner * 2.0f, inner * 2.0f);
            g.saveState();
            g.reduceClipRegion (clipC);
            const auto wCol = offVisual ? knobValueOffColour() : accent;
            juce::Path wedge;
            wedge.startNewSubPath (centre.x, yTip);
            wedge.lineTo (centre.x - half, yTop);
            wedge.quadraticTo (centre.x, yTop - len * 0.08f, centre.x + half, yTop);
            wedge.closeSubPath();
            g.setGradientFill (juce::ColourGradient (wCol.withAlpha (0.95f), centre.x, yTip,
                                                     (offVisual ? wCol : wCol.interpolatedWith (glowAccent, juce::jlimit (0.0f, 1.0f, w - 1.0f))).withAlpha (0.25f),
                                                     centre.x, yTop, false));
            g.fillPath (wedge);
            juce::Path edges;
            edges.startNewSubPath (centre.x - half, yTop);
            edges.lineTo (centre.x, yTip);
            edges.lineTo (centre.x + half, yTop);
            g.setColour (wCol.withAlpha (0.9f));
            g.strokePath (edges, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            juce::Path orig;
            orig.startNewSubPath (centre.x - hO, yTop);
            orig.lineTo (centre.x, yTip);
            orig.lineTo (centre.x + hO, yTop);
            juce::Path dashed;
            const float dashes[] = { 2.0f, 3.0f };
            juce::PathStrokeType (1.0f).createDashedStroke (dashed, orig, dashes, 2);
            g.setColour (juce::Colours::white.withAlpha (offVisual ? 0.08f : 0.18f));
            g.fillPath (dashed);
            g.restoreState();
        }

        // Runde 175 (User, HF Regain): Funken spruehen aus der Knopfmitte zu
        // den Seiten - golden in der Mitte, blau nach aussen. Dezent: wenige,
        // kleine Punkte, Menge mit dem Wert.
        if (hfSparkle && sliderPos > 0.01f)
        {
            const double tS = juce::Time::getMillisecondCounterHiRes() * 0.001;
            const int n = juce::roundToInt (2.0f + 6.0f * sliderPos);
            for (int i = 0; i < n; ++i)
            {
                const float seed  = (float) i * 12.9898f;
                const float dir   = (i % 2) ? 1.0f : -1.0f;
                const float speed = 0.9f + (std::sin (seed) * 0.5f + 0.5f) * 0.9f;
                const float p     = (float) std::fmod (tS * speed * (0.8 + sliderPos) + (std::sin (seed * 3.1f) * 0.5f + 0.5f), 1.0);
                const float reach = radius * (0.50f + 0.45f * sliderPos);   // bleibt innerhalb der Komponente (sonst abgeschnitten)
                const float sx = centre.x + dir * p * reach;
                const float sy = centre.y + std::sin (seed * 7.7f) * radius * 0.35f * p - p * p * 5.0f;
                const float a  = juce::jlimit (0.0f, 1.0f, sliderPos * (1.0f - p * 0.8f) * (0.55f + 0.45f * (float) std::sin (tS * 9.0 + seed)));
                const auto sc = juce::Colour (0xffffecc8).interpolatedWith (glowAccent.interpolatedWith (juce::Colours::white, 0.25f),
                                                                           juce::jmin (1.0f, p * 1.3f));
                const float rr = 2.4f;
                juce::ColourGradient sg (sc.withAlpha (a), sx, sy, sc.withAlpha (0.0f), sx + rr, sy, true);
                g.setGradientFill (sg);
                g.fillEllipse (sx - rr, sy - rr, rr * 2.0f, rr * 2.0f);
            }
        }

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
            // Runde 110: an = Schimmer wie die anderen Icons.
            if (button.getToggleState() && ! (bool) button.getProperties().getWithDefault ("sectionOff", false))
            {
                const auto lb = button.getLocalBounds().toFloat();
                softIconGlow (g, lb.getCentre(), juce::jmin (lb.getWidth(), lb.getHeight()) * 0.5f,
                              themePalette().frameRaye, 1.0f);
            }
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
            // Runde 61 (User): Sinus und Puls sind zwei gleichwertige Formen,
            // kein An/Aus. Beide leuchten gleich - welche gewaehlt ist, sagt
            // die Kurve selbst.
            // Runde 108: ohne Kaestchen, dafuer der Schimmer der Icon-Felder.
            const bool glowStyle = (bool) button.getProperties().getWithDefault ("glowIcon", false);
            const bool hotP = shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown;
            if (glowStyle)
            {
                if (! off)
                    softIconGlow (g, b.getCentre(), juce::jmin (b.getWidth(), b.getHeight()) * 0.5f,
                                  themePalette().frameRaye, shouldDrawButtonAsDown ? 1.9f : hotP ? 1.55f : 1.0f);
            }
            else if (! button.getProperties().getWithDefault ("noPlate", false))
                drawSmallIconPlate (g, b, ! off, off);
            b = b.reduced (s2 * 0.18f);   // Runde 161 (User): Icon etwas groesser (0.24 -> 0.18)
            auto box = b.withSizeKeepingCentre (b.getWidth(), b.getHeight() * 0.62f);
            juce::Colour col = glowStyle ? (off ? iconOffColour().withAlpha (0.55f)
                                                : themePalette().frameRaye.interpolatedWith (juce::Colour (0xfff2f4f8), 0.34f))
                                         : smallIconColour (! off);
            if (! glowStyle && (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown))
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
            g.strokePath (w, juce::PathStrokeType (juce::jmax (1.2f, s2 * (glowStyle ? 0.10f : 0.085f)),
                                                   juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
            if (! glowStyle)
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
            // Runde 108: ohne Kaestchen; an = blauer Schimmer (gekoppelt ans
            // Songtempo), aus = gedimmt.
            const bool glowStyle = (bool) button.getProperties().getWithDefault ("glowIcon", false);
            const bool hotS = shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown;
            if (glowStyle)
            {
                if (on && ! off)
                    softIconGlow (g, b.getCentre(), juce::jmin (b.getWidth(), b.getHeight()) * 0.5f,
                                  pairAccentColour(), shouldDrawButtonAsDown ? 1.9f : hotS ? 1.55f : 1.0f);
                else if (hotS && ! off)
                    softIconGlow (g, b.getCentre(), juce::jmin (b.getWidth(), b.getHeight()) * 0.5f,
                                  pairAccentColour(), 0.5f);
            }
            else if (! button.getProperties().getWithDefault ("noPlate", false))
                drawSmallIconPlate (g, b, on && ! off, off);
            b = b.reduced (s2 * 0.20f);   // Runde 161 (User): Icon etwas groesser (0.26 -> 0.20)
            juce::Colour col = glowStyle ? ((on && ! off) ? pairAccentColour().interpolatedWith (juce::Colour (0xfff2f4f8), 0.34f)
                                                          : iconOffColour().withAlpha (0.60f))
                                         : smallIconColour (on && ! off);
            if (! glowStyle && (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown))
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
            if (! glowStyle)
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
                drawSmallIconPlate (g, b, ! bypassed,
                                    button.getProperties().getWithDefault ("sectionOff", false));
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
                drawSmallIconPlate (g, b, mode != 0,
                                    button.getProperties().getWithDefault ("sectionOff", false));
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
            // "helpLetter": derselbe Kreis auch fuer andere Ein/Aus-Hinweise
            // (Runde 56: das kleine "i" der Smart-Infozeile).
            g.drawText (button.getProperties().getWithDefault ("helpLetter", "?").toString(),
                        button.getLocalBounds(), juce::Justification::centred, false);
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
        // Runde 122: "pairTint" faerbt nur blau, ohne den Schalter als "an"
        // zu zeichnen (x2, wenn der EQ in LCR sitzt).
        const bool gold = button.getProperties().getWithDefault ("pairedGold", false);
        const bool tint = button.getProperties().getWithDefault ("pairTint", false);
        // "altAccent" (Runde 61): FAST traegt in den hellen Themes das Gold,
        // das frueher PAIR hatte - siehe pairAccentColour(). In Sci-Fi bleibt
        // alles wie es war.
        const bool altAcc = button.getProperties().getWithDefault ("altAccent", false) && ! isSciFiTheme();
        const juce::Colour btnAccent = (gold || tint) ? pairAccentColour() : altAcc ? altAccentColour() : glowAccent;

        // Runde 110 (User: "die letzten Kaesten weg"): Icon-Schalter ohne
        // Flaeche. ØL/ØR: Icon oben, Buchstabe darunter. FAST/PAIR/x2: Icon
        // links, Name rechts. An = Schimmer + Farbe, aus = gedimmt.
        {
            const int  hdrIcon = (int)  button.getProperties().getWithDefault ("hdrIcon", 0);
            const bool phase   = (bool) button.getProperties().getWithDefault ("phaseIcon", false);
            if (hdrIcon > 0 || phase)
            {
                const auto lb     = button.getLocalBounds().toFloat();
                const bool secOff = button.getProperties().getWithDefault ("sectionOff", false);
                const bool lit    = button.getToggleState() || gold;
                const bool hot    = shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown;
                // Runde 166 (User: "oben B, beim Phaser D"): statt Schein ein
                // eindeutiger Zustands-Anzeiger links vom Wort - 1 = LED-Punkt
                // (EQ -> LCR), 2 = Mini-Schalter (FAST, LINK). Wort und
                // Anzeiger stehen rechtsbuendig im Knopf.
                if (const int ctl = (int) button.getProperties().getWithDefault ("ctlStyle", 0); ctl > 0)
                {
                    drawCtlIndicator (g, button, ctl, lit, secOff, hot, softChipAccent (button));
                    return;
                }
                juce::Rectangle<float> iconR, textR;
                // Weicher, kantenloser Schein hinter dem ganzen Schalter, wenn
                // an (Runde 111) - fuer Text-Schalter und Ø gleich.
                if (! secOff && (lit || hot))
                {
                    // Runde 161 (User): OeL/OeR minimal weniger Schein.
                    const float a0 = (lit ? (shouldDrawButtonAsDown ? 0.075f : hot ? 0.062f : 0.05f) : 0.022f)
                                     * (phase ? 0.72f : 1.0f);
                    for (int k = 0; k < 4; ++k)
                    {
                        auto rr = lb.reduced (2.0f + 2.2f * (float) k, 1.0f + 1.2f * (float) k);
                        if (rr.getHeight() < 2.0f) break;
                        g.setColour (btnAccent.withAlpha (a0));
                        g.fillRoundedRectangle (rr, rr.getHeight() * 0.5f);
                    }
                }
                // FAST und PAIR: nur der Name (Runde 111). x2 behaelt seine
                // Kurve (Runde 112, User: "x2 wirkt verloren - Kurve dazu").
                if (! phase && hdrIcon != 3)
                    return;
                if (phase)
                    phaseLayout (button, lb, iconR, textR);
                else
                    hdrIconLayout (button, lb, iconR, textR);
                const auto  c = iconR.getCentre();
                const float s = juce::jmin (iconR.getWidth(), iconR.getHeight());
                juce::ignoreUnused (hot);
                juce::Colour col = iconToggleColour (btnAccent, lit, secOff,
                                                     shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown);
                // Runde 165 (User: "L/R aus ist schon sehr praegnant - an oder
                // nicht?"): seit sie groesser sind, braucht AUS mehr Abstand
                // zu AN - deutlich zurueckgenommen, beim Hover etwas heller.
                if (phase && ! lit && ! secOff)
                    col = phaseOffColour (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown);
                g.setColour (col);
                const juce::PathStrokeType st (juce::jmax (1.3f, s * (phase ? 0.13f : 0.095f)),
                                               juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
                if (phase)
                {
                    // Ø - so hoch wie der Buchstabe daneben (Runde 112)
                    const float r = s * 0.42f;
                    juce::Path ring;
                    ring.addEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
                    g.strokePath (ring, st);
                    juce::Path slash;
                    slash.startNewSubPath (c.x - r * 1.05f, c.y + r * 1.05f);
                    slash.lineTo (c.x + r * 1.05f, c.y - r * 1.05f);
                    g.strokePath (slash, st);
                }
                else if (hdrIcon == 1)
                {
                    // FAST: Doppelpfeil
                    const float h = s * 0.26f, w = s * 0.20f;
                    juce::Path p;
                    for (float dx : { -s * 0.13f, s * 0.11f })
                    {
                        p.startNewSubPath (c.x + dx - w * 0.5f, c.y - h);
                        p.lineTo (c.x + dx + w * 0.5f, c.y);
                        p.lineTo (c.x + dx - w * 0.5f, c.y + h);
                    }
                    g.strokePath (p, st);
                }
                else if (hdrIcon == 2)
                {
                    // PAIR: zwei Kettenglieder
                    const float w = s * 0.46f, h = s * 0.28f;
                    juce::Path a, b;
                    a.addRoundedRectangle (c.x - w + w * 0.14f, c.y - h * 0.5f, w, h, h * 0.5f);
                    b.addRoundedRectangle (c.x - w * 0.14f,     c.y - h * 0.5f, w, h, h * 0.5f);
                    g.strokePath (a, st);
                    g.strokePath (b, st);
                }
                else
                {
                    // x2: dieselbe Kurve zweimal - flach und steil
                    const float w = s * 0.42f;
                    juce::Path flat, steep;
                    flat.startNewSubPath (c.x - w, c.y + s * 0.16f);
                    flat.cubicTo (c.x - w * 0.1f, c.y + s * 0.16f, c.x + w * 0.1f, c.y + s * 0.03f, c.x + w, c.y + s * 0.03f);
                    steep.startNewSubPath (c.x - w, c.y + s * 0.24f);
                    steep.cubicTo (c.x, c.y + s * 0.24f, c.x, c.y - s * 0.22f, c.x + w, c.y - s * 0.22f);
                    g.setColour (col.withMultipliedAlpha (0.5f));
                    g.strokePath (flat, st);
                    g.setColour (col);
                    g.strokePath (steep, st);
                }
                return;
            }
        }

        // Runde 112 (User: "AG ohne Box, dezent"): nur der Wert.
        if ((bool) button.getProperties().getWithDefault ("plainValue", false))
            return;

        // Runde 108: Soft-Chip statt Umriss-Pille (FAST, PAIR, x2, L/R).
        if ((bool) button.getProperties().getWithDefault ("softChip", false))
        {
            drawSoftChip (g, bounds, btnAccent, button.getToggleState() || gold,
                          (bool) button.getProperties().getWithDefault ("sectionOff", false),
                          shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
        }

        // View-Panel-Knoepfe ohne den Leucht-Hof (User: "leuchtende Kaesten").
        // "softOn" (Runde 62, User): EARLY/LATE soll im aktiven Zustand
        // nicht so laut sein wie L und R darueber - kein Leucht-Hof, und der
        // Rahmen kommt weiter unten gedaempft.
        const bool softOn = button.getProperties().getWithDefault ("softOn", false);
        if ((isOn || gold) && ! softOn && ! button.getProperties().getWithDefault ("noGlow", false))
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
        const juce::Colour onFull = gold           ? juce::Colour (0xff2e2a1c)
                                                   : juce::Colour (0xff2a1f33);
        // Knoepfe in den Panels (Settings/View) tragen "noGlow". Ihr
        // An-Zustand war bisher controlOnFill() - in Pop eine kraeftige Pille,
        // in allen anderen Themes aber praktisch die Panelfarbe (User: "nur
        // Pop bleibt im Menue hell, alle anderen sind dunkel"). Sie bekommen
        // deshalb ihre eigene, deutlich sichtbare Faerbung, die sich aus der
        // Theme-Farbe ableitet statt aus der Plattenfarbe.
        const bool panelBtn = button.getProperties().getWithDefault ("noGlow", false);
        // "modePill" (Runde 51, User): die Modus-Knoepfe von Parallax und RAYE
        // sind KEINE An/Aus-Schalter - sie zeigen nur, welcher Eintrag gerade
        // gewaehlt ist. Eine gefuellte Pille sah aber aus wie ein eingeschalteter
        // Knopf und passte damit nicht zu den Knoepfen in Eclipse & Co. Also nur
        // ein Umriss - der bleibt bewusst auch dann stehen, wenn die Sektion aus
        // ist, weil man den Modus dort trotzdem umstellen koennen soll.
        const bool modePill = button.getProperties().getWithDefault ("modePill", false);
        const bool fillIt = (panelBtn || isOn || offButPaired) && ! modePill;
        if (fillIt)
        {
            // An-Zustand auf halbem Weg zwischen "unsichtbar" und dem alten
            // An-Ton (User: "genau zwischen off und der aktuellen on-Staerke").
            juce::Colour base = gold ? themePalette().plate.interpolatedWith (onFull, 0.5f) : controlOnFill();
            if (panelBtn)
                base = isOn ? juce::Colour (0xff1e2128).interpolatedWith (themePalette().knob, 0.34f)
                            : juce::Colour (0xff2a2e37);
            // Knopf an, Sektion aus: nur noch ein Hauch heller als die Platte -
            // sichtbar, aber nicht laut.
            if (offButPaired && ! isOn)
                base = themePalette().plate.brighter (0.07f);
            if (shouldDrawButtonAsDown) base = base.brighter (0.1f);
            else if (shouldDrawButtonAsHighlighted) base = base.brighter (0.05f);
            g.setColour (base);
            g.fillRoundedRectangle (bounds, cornerSize);
        }
        else if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
        {
           #if SPACEX_PX_DIAG_ONLY
            // Runde 92 (User: "Hover-Box weg machen"): wo ein Icon sitzt, gibt
            // es keinen Kasten - der Schimmer dahinter wird stattdessen
            // kraeftiger (siehe unten).
            if ((int) button.getProperties().getWithDefault ("pxDiagram",  -1) < 0
             && (int) button.getProperties().getWithDefault ("rayDiagram", -1) < 0
             && (int) button.getProperties().getWithDefault ("eqDiagram",  -1) < 0
             && (int) button.getProperties().getWithDefault ("profileDiagram", -1) < 0
             && (int) button.getProperties().getWithDefault ("ppDiagram",  -1) < 0)
           #endif
            {
                // Ohne Flaeche braucht der Aus-Zustand trotzdem Hover-Feedback.
                g.setColour (juce::Colours::white.withAlpha (shouldDrawButtonAsDown ? 0.08f : 0.045f));
                g.fillRoundedRectangle (bounds, cornerSize);
            }
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

        // Runde 93 (User: "Pop Art Grafik kaputt"): in Pop kam der Comic-
        // Kasten zuerst und sprang aus der Funktion - der Icon-Knopf bekam
        // also den Rahmen, den er gerade NICHT haben soll, und gar kein Icon.
        // Traegt er ein Icon, ueberspringt er den Comic-Zweig.
        const bool diagPill = (int) button.getProperties().getWithDefault ("pxDiagram",  -1) >= 0
                           || (int) button.getProperties().getWithDefault ("rayDiagram", -1) >= 0
                           || (int) button.getProperties().getWithDefault ("eqDiagram",  -1) >= 0
                           || (int) button.getProperties().getWithDefault ("profileDiagram", -1) >= 0
                           || (int) button.getProperties().getWithDefault ("ppDiagram",  -1) >= 0;
        juce::ignoreUnused (diagPill);
        // Pair bei ausgeschalteter RAYE-Sektion: Zustand trotzdem sichtbar
        // (hellerer Rand, User) - Property "pairedGold" bleibt gesetzt.
        // Sektion aus: GAR KEIN Rahmen mehr (User: "im Moment sehen die
        // Buttons auch im section=off-Zustand noch so aus, als waere die
        // Section an"). Der eigene An-Zustand bleibt allein ueber die etwas
        // hellere Flaeche sichtbar - klicken kann man sie weiterhin, sie sind
        // nur nicht mehr laut.
        if (modePill)
        {
            // Runde 55 (User): die Pille traegt die Akzentfarbe IHRER Sektion
            // (kommt als "pillColour" aus dem Editor, weil nur der die
            // Titelfarbe kennt), bewusst zurueckgenommen - sie soll den
            // Rahmen zeigen, nicht leuchten. Dieselbe Deckkraft wie das
            // Bars-Feld daneben, damit am Ende alles einheitlich aussieht.
            auto pillCol = button.getProperties().contains ("pillColour")
                             ? juce::Colour ((juce::uint32) (int) button.getProperties()["pillColour"])
                             : themePalette().frameMain;
            // "pillStrong" (Runde 60): das Smart-Profil traegt einen etwas
            // kraeftigeren Rand als die Modus-Pillen in den Sektionen und
            // bekommt eine leichte Fuellung, sobald wirklich ein Profil
            // gewaehlt ist - "kein Profil" muss man auf den ersten Blick
            // davon unterscheiden koennen (User).
            const bool strong = button.getProperties().getWithDefault ("pillStrong", false);
            const bool armed  = button.getProperties().getWithDefault ("pillArmed", false);
            if (strong && armed && ! diagPill)
            {
                g.setColour (pillCol.withAlpha (0.14f));
                g.fillRoundedRectangle (bounds, cornerSize);
            }
            const bool hasDiagram = diagPill;
            // Runde 69 (User): "miniminimal weniger hell" - 0.62 -> 0.54.
            const float pillAlpha = sectionIsOffNow ? 0.28f : (strong && ! armed ? 0.34f : 0.54f);
           #if SPACEX_PX_DIAG_ONLY
            // Runde 91 (User: "kannst du die beiden Boxen entfernen"): traegt
            // der Knopf ein Icon, ersetzt der Schimmer dahinter den Rahmen.
            if (! hasDiagram)
           #endif
            {
                g.setColour (pillCol.withAlpha (pillAlpha));
                g.drawRoundedRectangle (bounds, cornerSize, strong ? 1.65f : 1.35f);   // eine Spur kraeftiger (User Runde 61)
            }

            // Runde 88 (User, nach dem Nuro-Supernova-Vorbild): ein winziges
            // DIAGRAMM links im Knopf, das zeigt, was der Modus tut - nicht
            // ein Signet, das nur huebsch ist. Gezeichnet, nicht geladen:
            // reine Pfade, also scharf in jeder Groesse und automatisch in
            // der Farbe der Pille.
            // Gelesen wird es als Draufsicht: Mitte = Zentrum, aussen = Seiten.
            //   0 VELVET   zwei schmale Striche dicht an der Mitte
            //   1 HALO     ein Ring um die scharfe Mitte
            //   2 ILLUSION weiter auseinander, Mitte noch klar
            //   3 DOUBLE   am weitesten, die Mitte loest sich auf
            // Runde 108: Icon LINKS neben dem Namen - Smart-Profil und PRE/POST.
            {
                const int pdia  = (int) button.getProperties().getWithDefault ("profileDiagram", -1);
                const int ppdia = (int) button.getProperties().getWithDefault ("ppDiagram", -1);
                if (pdia >= 0 || ppdia >= 0)
                {
                    juce::Rectangle<float> iconR, textR;
                    if ((bool) button.getProperties().getWithDefault ("iconAbove", false))
                        iconR = button.getLocalBounds().toFloat().withHeight ((float) button.getHeight() * 0.64f);   // Runde 110
                    else
                        inlineIconLayout (button, button.getLocalBounds().toFloat(), iconR, textR);
                    // PRE/POST leuchtet nur, wenn ueberhaupt umgepolt wird (User,
                    // Runde 62) - sonst sieht es aus, als passiere etwas.
                    // "Kein Profil" leuchtet nie.
                    const bool dim = sectionIsOffNow || (ppdia >= 0 && ! button.getToggleState()) || pdia == 0;
                    float a  = dim ? 0.42f : 0.90f;
                    const auto  c  = iconR.getCentre();
                    if (! dim)
                        softIconGlow (g, c, juce::jmin (iconR.getHeight() * 0.5f, (float) button.getHeight() * 0.5f),
                                      pillCol, shouldDrawButtonAsDown ? 1.9f : shouldDrawButtonAsHighlighted ? 1.55f : 1.0f);
                    // Runde 113 (User): PRE/POST sieht in JEDEM Zustand genauso
                    // aus wie ØL/ØR - dieselbe Farbregel, gekoppelt.
                    if (ppdia >= 0)
                    {
                        pillCol = iconToggleColour (altAccentColour(), button.getToggleState(), sectionIsOffNow,
                                                    shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown);
                        a = 1.0f;
                    }
                    const float sc = juce::jmin (iconR.getWidth() / 48.0f, iconR.getHeight() / 32.0f);
                    auto P = [&] (float x, float y) { return juce::Point<float> (c.x + (x - 24.0f) * sc, c.y + (y - 16.0f) * sc); };
                    auto dot = [&] (float x, float y, float r, float alpha)
                    {
                        g.setColour (pillCol.withAlpha (a * alpha));
                        const auto q = P (x, y);
                        g.fillEllipse (q.x - r * sc, q.y - r * sc, r * 2.0f * sc, r * 2.0f * sc);
                    };
                    auto stroke = [&] (const juce::Path& pth, float w, float alpha)
                    {
                        g.setColour (pillCol.withAlpha (a * alpha));
                        g.strokePath (pth, juce::PathStrokeType (w * sc, juce::PathStrokeType::curved,
                                                                 juce::PathStrokeType::rounded));
                    };
                    if (ppdia >= 0)
                    {
                        // Kette: Linie, in der Mitte die Breiten-Stufe als Kaestchen,
                        // der Punkt davor (PRE) oder dahinter (POST).
                        juce::Path line;
                        line.startNewSubPath (P (4.0f, 16.0f));
                        line.lineTo (P (44.0f, 16.0f));
                        stroke (line, 2.0f, 0.45f);
                        juce::Path blk;
                        const auto tl = P (18.0f, 9.0f);
                        blk.addRoundedRectangle (tl.x, tl.y, 12.0f * sc, 14.0f * sc, 3.0f * sc);
                        stroke (blk, 2.0f, 0.75f);
                        dot (ppdia == 1 ? 38.0f : 10.0f, 16.0f, 3.8f, 1.0f);
                    }
                    else
                    {
                        // Wo sitzt die Quelle im Stereobild? Dieselbe Sprache
                        // wie die Micropitch-Icons.
                        switch (pdia)
                        {
                            case 1:   // LEAD VOCAL: fest in der Mitte, weit drumherum
                            {
                                juce::Path l, r;
                                l.startNewSubPath (P (10.0f, 6.0f));
                                l.cubicTo (P (4.0f, 12.0f), P (4.0f, 20.0f), P (10.0f, 26.0f));
                                r.startNewSubPath (P (38.0f, 6.0f));
                                r.cubicTo (P (44.0f, 12.0f), P (44.0f, 20.0f), P (38.0f, 26.0f));
                                stroke (l, 2.0f, 0.45f);
                                stroke (r, 2.0f, 0.45f);
                                dot (24.0f, 16.0f, 5.5f, 1.0f);
                                break;
                            }
                            case 2:   // BACKINGS: Runde 145 (User) "Chor" - drei Stimmen, die
                            {         // sich ueberlappen. Die vier Punkte sahen aus wie die Auswahlpunkte.
                                for (float x : { 16.0f, 32.0f })
                                {
                                    juce::Path ring;
                                    const auto q = P (x, 16.0f);
                                    ring.addEllipse (q.x - 8.0f * sc, q.y - 8.0f * sc, 16.0f * sc, 16.0f * sc);
                                    stroke (ring, 2.0f, 0.6f);
                                }
                                juce::Path mid;
                                const auto q = P (24.0f, 16.0f);
                                mid.addEllipse (q.x - 8.0f * sc, q.y - 8.0f * sc, 16.0f * sc, 16.0f * sc);
                                stroke (mid, 2.0f, 1.0f);
                                break;
                            }
                            case 3:   // ADLIBS: verstreut, in Bewegung
                            {
                                dot (9.0f, 10.0f, 3.0f, 1.0f);
                                dot (37.0f, 21.0f, 3.0f, 1.0f);
                                dot (31.0f, 7.0f, 2.2f, 0.6f);
                                dot (15.0f, 24.0f, 2.2f, 0.6f);
                                juce::Path m;
                                m.startNewSubPath (P (14.0f, 10.0f));
                                m.cubicTo (P (19.0f, 9.0f), P (22.0f, 11.0f), P (24.0f, 14.0f));
                                stroke (m, 1.6f, 0.5f);
                                break;
                            }
                            case 4:   // SEND FX: nur Raum, keine Mitte
                            {
                                const float radii[3] = { 5.0f, 10.0f, 14.5f };
                                const float widths[3] = { 2.0f, 1.6f, 1.2f };
                                const float alphas[3] = { 0.75f, 0.45f, 0.22f };
                                for (int k = 0; k < 3; ++k)
                                {
                                    juce::Path ring;
                                    ring.addEllipse (c.x - radii[k] * sc, c.y - radii[k] * sc,
                                                     radii[k] * 2.0f * sc, radii[k] * 2.0f * sc);
                                    stroke (ring, widths[k], alphas[k]);
                                }
                                break;
                            }
                            default:  // KEIN PROFIL: gestrichelter Kreis
                            {
                                juce::Path ring, dashed;
                                ring.addEllipse (c.x - 9.0f * sc, c.y - 9.0f * sc, 18.0f * sc, 18.0f * sc);
                                const float dashes[2] = { 3.0f * sc, 4.0f * sc };
                                juce::PathStrokeType (2.0f * sc).createDashedStroke (dashed, ring, dashes, 2);
                                g.setColour (pillCol.withAlpha (a * 0.8f));
                                g.fillPath (dashed);
                                break;
                            }
                        }
                    }
                    return;
                }
            }
            const int diag = (int) button.getProperties().getWithDefault ("pxDiagram", -1);
            const int rdia = (int) button.getProperties().getWithDefault ("rayDiagram", -1);
            const int edia = (int) button.getProperties().getWithDefault ("eqDiagram",  -1);
            if (diag >= 0 || rdia >= 0 || edia >= 0)
            {
                // Runde 113 (User): ist die Sektion aus, traegt das Icon die
                // Farbe seiner Schrift - eine Spur dunkler als das Gold, und
                // kein Schimmer.
                const juce::Colour icoCol = sectionIsOffNow ? labelOffColour() : pillCol;
               #if SPACEX_PX_DIAG_ONLY
                // Runde 91 (User): kein Rahmen mehr - das Icon gross oben, die
                // Schrift dicht ueber den Punkten. Dahinter ein weicher
                // Farbschimmer statt einer Box (Nuro-Vorbild).
                auto db = bounds.withHeight (bounds.getHeight() * 0.66f);
                // Runde 105 (User): alle Icons einheitlich etwas groesser
                // (Obergrenze 3.2 -> 3.8, ~ +20 %), soweit die Feldhoehe es hergibt.
                const float sc = juce::jmin (3.8f, db.getHeight() / 13.0f);
               #else
                auto db = bounds.withWidth (16.0f).translated (7.0f, 0.0f);
                const float sc = 1.0f;
               #endif
                const float cx = db.getCentreX(), cy = db.getCentreY();
                // Runde 157 (User: "Flat minimal duenner und minimal weniger
                // Glow"): 1 = ganz flach, 0 = deutliche Kurve. Morpht mit.
                float eqFlat01 = 0.0f;
                if (edia >= 0)
                {
                    sideeq::Look Lf = sideeq::lookFor (edia, 0.0f);
                    if (auto* arr = button.getProperties()["eqLook"].getArray())
                        if (arr->size() == sideeq::kLookFields)
                        {
                            Lf.hpSlope = (float) (double) (*arr)[1];
                            Lf.sG      = (float) (double) (*arr)[4];
                            Lf.mG      = (float) (double) (*arr)[7];
                        }
                    eqFlat01 = 1.0f - juce::jlimit (0.0f, 1.0f, juce::jmax (Lf.hpSlope / 4.0f,
                                                                            std::abs (Lf.sG) / 3.0f,
                                                                            std::abs (Lf.mG) / 3.0f));
                }
               #if SPACEX_PX_DIAG_ONLY
                {
                    // Der Schimmer liegt HINTER dem Icon: ein weicher runder
                    // Verlauf in der Sektionsfarbe, der nach aussen ausgeht.
                    // Runde 92 (User): der Hover zeigt sich als STAERKERER
                    // Schimmer statt als Kasten.
                    const float hot = (shouldDrawButtonAsDown ? 1.9f
                                    : shouldDrawButtonAsHighlighted ? 1.55f : 1.0f) * (1.0f - 0.22f * eqFlat01);
                    // Runde 95 (User: "Grafikbug: Glow cut off"): der Knopf
                    // schneidet an seinen eigenen Kanten ab, der Schimmer darf
                    // also nie ueber sie hinausreichen.
                    const float room = juce::jmin (juce::jmin (cy - bounds.getY(), bounds.getBottom() - cy),
                                                   bounds.getWidth() * 0.5f);
                    const float gr = juce::jmin (11.0f * sc * (hot > 1.0f ? 1.10f : 1.0f), room);
                    juce::ColourGradient grad (icoCol.withAlpha (juce::jmin (0.65f, (sectionIsOffNow ? 0.0f : 0.26f) * hot)), cx, cy,
                                               icoCol.withAlpha (0.0f), cx + gr, cy, true);
                    grad.addColour (0.45, icoCol.withAlpha (juce::jmin (0.40f, (sectionIsOffNow ? 0.0f : 0.13f) * hot)));
                    g.setGradientFill (grad);
                    g.fillEllipse (cx - gr, cy - gr, gr * 2.0f, gr * 2.0f);
                }
               #endif
                const float a  = sectionIsOffNow ? 1.0f : 0.85f;
                g.setColour (icoCol.withAlpha (a));
                if (edia >= 0)
                {
                    // Runde 126 (User): EINE Linie fuer die Seiten, bei FOCUS
                    // eine zweite, duennere fuer die Mitte. Gezeichnete Form
                    // (sideeq::Look), die fliessend morpht; der Hochpass geht
                    // als Gerade nach unten raus und blendet dort aus. Dazu
                    // eine ganz feine 0-dB-Linie.
                    sideeq::Look L = sideeq::lookFor (edia, 0.0f);
                    if (auto* arr = button.getProperties()["eqLook"].getArray())
                        if (arr->size() == sideeq::kLookFields)
                        {
                            float v[sideeq::kLookFields];
                            for (int k = 0; k < sideeq::kLookFields; ++k) v[k] = (float) (double) (*arr)[k];
                            L = { v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8] };
                        }
                    const float w = 20.0f * sc, amp = 6.5f * sc;
                    const float x0 = cx - w * 0.5f;
                    const float y0 = cy - amp * 0.15f;             // 0 dB leicht ueber der Mitte
                    const juce::Rectangle<float> clipR (x0 - 2.0f, y0 - amp * 1.25f, w + 4.0f, amp * 2.45f);

                    g.setColour (icoCol.withAlpha (0.16f * a));
                    g.drawHorizontalLine ((int) std::round (y0), x0, x0 + w);

                    auto pathFor = [&] (bool side)
                    {
                        juce::Path pth;
                        constexpr int N = 96;
                        for (int i = 0; i <= N; ++i)
                        {
                            const float t = (float) i / (float) N;
                            const float x = x0 + w * t;
                            const float y = juce::jlimit (clipR.getY() - 4.0f, clipR.getBottom() + 4.0f,
                                                          y0 - amp * sideeq::lookY (L, side, t));
                            if (i == 0) pth.startNewSubPath (x, y); else pth.lineTo (x, y);
                        }
                        return pth;
                    };
                    juce::Graphics::ScopedSaveState ss (g);
                    g.reduceClipRegion (clipR.toNearestInt());
                    // Nach unten ausblenden: der Hochpass verschwindet weich.
                    auto fillFaded = [&] (const juce::Path& stroke, float alpha)
                    {
                        juce::ColourGradient fade (icoCol.withAlpha (alpha), 0.0f, y0 + amp * 0.85f,
                                                   icoCol.withAlpha (0.0f),  0.0f, clipR.getBottom(), false);
                        g.setGradientFill (fade);
                        g.fillPath (stroke);
                    };
                    const float mAlpha = juce::jlimit (0.0f, 1.0f, std::abs (L.mG) / 1.5f);
                    if (mAlpha > 0.01f)
                    {
                        juce::Path st;
                        juce::PathStrokeType (1.1f * sc, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)
                            .createStrokedPath (st, pathFor (false));
                        fillFaded (st, a * 0.55f * mAlpha);
                    }
                    juce::Path st;
                    juce::PathStrokeType ((1.45f - 0.22f * eqFlat01) * sc, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)
                        .createStrokedPath (st, pathFor (true));
                    fillFaded (st, a);
                }
                else if (rdia >= 0)
                {
                    // RAYE-Charakter als Bewegungsbild:
                    //   0 SWEEP    eine lange, ruhige Welle
                    //   1 SHIMMER  viele kleine, schnelle
                    //   2 SPIN     ein Kreis mit laufendem Punkt
                    //   3 SWIRL    eine Spirale
                    juce::Path pth;
                    if (rdia == 0 || rdia == 1)
                    {
                        const float w = 14.0f * sc, h = (rdia == 0 ? 6.4f : 3.4f) * sc;
                        const float cyc = rdia == 0 ? 1.0f : 3.0f;
                        for (int i = 0; i <= 40; ++i)
                        {
                            const float t = (float) i / 40.0f;
                            const float x = cx - w * 0.5f + w * t;
                            const float y = cy - h * 0.5f * std::sin (t * juce::MathConstants<float>::twoPi * cyc);
                            if (i == 0) pth.startNewSubPath (x, y); else pth.lineTo (x, y);
                        }
                    }
                    else if (rdia == 2)
                    {
                        const float r = 5.2f * sc;
                        pth.addEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
                    }
                    else
                    {
                        for (int i = 0; i <= 64; ++i)
                        {
                            const float t   = (float) i / 64.0f;
                            const float ang = t * juce::MathConstants<float>::twoPi * 1.7f;
                            const float r   = (1.0f + t * 4.4f) * sc;
                            const float x = cx + std::cos (ang) * r, y = cy + std::sin (ang) * r;
                            if (i == 0) pth.startNewSubPath (x, y); else pth.lineTo (x, y);
                        }
                    }
                    g.strokePath (pth, juce::PathStrokeType (1.2f * sc, juce::PathStrokeType::curved,
                                                             juce::PathStrokeType::rounded));
                    if (rdia == 2)
                        g.fillEllipse (cx + 5.2f * sc - 1.4f * sc, cy - 1.4f * sc, 2.8f * sc, 2.8f * sc);
                }
                else
                {
                    // Parallax als Draufsicht: Mitte = Zentrum, aussen = Seiten.
                    const float spread[4] = { 2.6f, 0.0f, 5.0f, 6.6f };
                    // Runde 96 (User): VELVET sah aus wie ILLUSION und DOUBLE -
                    // es ist aber der leiseste der beiden mittigen Modi. Also
                    // derselbe Ring wie HALO, nur enger.
                    if (diag == 0 || diag == 1)
                    {
                        const float r = (diag == 1 ? 5.4f : 3.4f) * sc;
                        // VELVET ist der leisere der beiden: enger Ring UND
                        // schwaecher gezeichnet (User).
                        if (diag == 0) g.setColour (icoCol.withAlpha (a * 0.50f));
                        g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.1f * sc);
                        g.setColour (icoCol.withAlpha (a));
                        g.fillEllipse (cx - 1.3f * sc, cy - 1.3f * sc, 2.6f * sc, 2.6f * sc);
                    }
                    else
                    {
                        const float sp = spread[juce::jlimit (0, 3, diag)] * sc;
                        const float h  = (diag == 3 ? 7.0f : 5.4f) * sc;
                        const float w  = 2.2f * sc;
                        g.fillRoundedRectangle (cx - sp - w * 0.5f, cy - h * 0.5f, w, h, w * 0.5f);
                        g.fillRoundedRectangle (cx + sp - w * 0.5f, cy - h * 0.5f, w, h, w * 0.5f);
                        // Die Mitte: bei DOUBLE nur noch angedeutet.
                        g.setColour (icoCol.withAlpha (a * (diag == 3 ? 0.30f : 1.0f)));
                        g.fillEllipse (cx - 1.2f * sc, cy - 1.2f * sc, 2.4f * sc, 2.4f * sc);
                    }
                }
            }
            return;
        }
        if (sectionIsOffNow)
            return;
        g.setColour ((isOn || gold) ? (softOn ? btnAccent.withAlpha (0.46f) : btnAccent)
                                    : juce::Colour (0xff3a3d45));
        // "thinOnFrame" (Polarity L/R und 1-4): der An-Rahmen ist eine Spur
        // duenner, damit diese Knoepfe nicht mehr Gewicht bekommen als die
        // Regler daneben (User: "wirklich nur mini mini mini duenner").
        const float onThickness = softOn ? 1.2f
                                : button.getProperties().getWithDefault ("thinOnFrame", false) ? 1.35f : 1.6f;
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
        juce::ignoreUnused (shortcutKeyText, icon);
        const auto pal = themePalette();
        // Runde 161 (User): Eintraege mit eigener Farbe (Rename, Delete,
        // Preset Folder) sind Befehle, keine Presets - kleiner und leiser.
        const bool subtle = textColourToUse != nullptr && textColourToUse->getAlpha() > 0;
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
        if (subtle)
            col = ! isActive ? juce::Colour (0xff4e535d)
                : isHighlighted ? juce::Colour (0xffc3c8d2) : *textColourToUse;
        if (isTicked) col = pal.knob;
        g.setColour (col);
        g.setFont (subtle ? getPopupMenuFont().withHeight (13.5f) : getPopupMenuFont());
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
    // ===== Runde 108: MODERNE FORMEN (User: "modern, ohne Boxen") =====
    // Soft-Chip: die Form fuer An/Aus. Keine Linie mehr - aus ist eine kaum
    // sichtbare Flaeche, an ist getoent, leuchtet von innen und traegt einen
    // ganz feinen Rand in derselben Farbe. Pop behaelt seine Comic-Pillen.
    static void drawSoftChip (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour acc,
                              bool lit, bool secOff, bool hot, bool down)
    {
        const float cr = r.getHeight() * 0.5f;
        if (lit)
        {
            const float k = secOff ? 0.45f : 1.0f;
            g.setColour (acc.withAlpha ((0.13f + (hot ? 0.04f : 0.0f) + (down ? 0.04f : 0.0f)) * k));
            g.fillRoundedRectangle (r, cr);
            if (! secOff)
            {
                juce::Path clip;
                clip.addRoundedRectangle (r, cr);
                g.saveState();
                g.reduceClipRegion (clip);
                const auto c = r.getCentre();
                const float rx = r.getWidth() * 0.62f;
                juce::ColourGradient grad (acc.withAlpha (hot ? 0.30f : 0.22f), c.x, c.y,
                                           acc.withAlpha (0.0f), c.x + rx, c.y, true);
                g.setGradientFill (grad);
                g.fillEllipse (c.x - rx, c.y - rx, rx * 2.0f, rx * 2.0f);
                g.restoreState();
            }
            g.setColour (acc.withAlpha (0.24f * k));
            g.drawRoundedRectangle (r.reduced (0.5f), cr, 1.0f);
        }
        else
        {
            g.setColour (juce::Colours::white.withAlpha (secOff ? 0.025f : down ? 0.10f : hot ? 0.075f : 0.045f));
            g.fillRoundedRectangle (r, cr);
        }
    }

    // Weicher runder Schimmer hinter einem Icon (dieselbe Sprache wie die
    // Icon-Felder). strength 1 = normal, groesser = Hover/gedrueckt.
    static void softIconGlow (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour col, float strength)
    {
        if (r <= 1.0f) return;
        juce::ColourGradient grad (col.withAlpha (juce::jmin (0.60f, 0.26f * strength)), c.x, c.y,
                                   col.withAlpha (0.0f), c.x + r, c.y, true);
        grad.addColour (0.45, col.withAlpha (juce::jmin (0.35f, 0.12f * strength)));
        g.setGradientFill (grad);
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    }

    // Icon LINKS neben dem Namen - fuer Knoepfe, deren Hoehe fuer Icon ueber
    // Name nicht reicht (Smart-Profil, PRE/POST). Icon und Schrift stehen als
    // Gruppe mittig. drawButtonBackground und drawButtonText rechnen beide
    // hiermit, damit Icon und Schrift nie auseinanderlaufen.
    static void inlineIconLayout (juce::Button& b, juce::Rectangle<float> area,
                                  juce::Rectangle<float>& iconR, juce::Rectangle<float>& textR)
    {
        const auto f   = unifiedButtonFont (b.getHeight());
        const float h  = area.getHeight();
        const float iw = h * 1.45f;             // 48:32 wie die Icon-Zeichnungen
        const float gap = h * 0.16f;
        const float tw = juce::GlyphArrangement::getStringWidth (f, b.getButtonText()) + 2.0f;
        const float x0 = area.getCentreX() - (iw + gap + tw) * 0.5f;
        iconR = { x0, area.getY(), iw, h };
        textR = { x0 + iw + gap, area.getY(), tw + 6.0f, h };
    }

    // Runde 112 (User: "Ø und L/R sollen dieselbe Farbe haben"): EINE Farbe
    // fuer Icon und Schrift eines Icon-Schalters.
    static juce::Colour phaseOffColour (bool hot)
    {
        return hot ? juce::Colour (0xff9aa0ab) : juce::Colour (0xff5f6470);
    }

    static juce::Colour iconToggleColour (juce::Colour acc, bool lit, bool secOff, bool hot)
    {
        // Runde 116 (User): Sektion aus -> dieselbe Farbe wie VELVET/FLAT und
        // die Regler-Namen (labelOffColour), nicht mehr gedimmtes Gold.
        // Sektion an, Schalter aus: deutlich lesbarer als vorher ("zu dunkel,
        // mit Box sah man es besser") - der neutrale Ton der Soft-Chips.
        return secOff ? (lit ? labelOffColour() : labelOffColour().withAlpha (0.60f))
             : lit    ? acc.interpolatedWith (juce::Colour (0xfff2f4f8), 0.30f)
                      : (hot ? juce::Colour (0xffc3c8d2) : juce::Colour (0xff8f96a4));
    }

    static juce::Font phaseFont()
    {
        return juce::Font (juce::FontOptions (14.5f, juce::Font::bold)).withExtraKerningFactor (0.08f);
    }
    // Runde 154 (User: "ØL und ØR 50 % groesser"): Faktor pro Knopf ueber die
    // Property "phaseScale" - Ø und Buchstabe wachsen gemeinsam.
    static juce::Font phaseFontFor (juce::Button& b)
    {
        const auto f = phaseFont();
        const float k = (float) (double) b.getProperties().getWithDefault ("phaseScale", 1.0);
        return f.withHeight (f.getHeight() * k);
    }

    // Runde 112: Ø und Buchstabe NEBENEINANDER und gleich gross (User: "das
    // Icon soll nicht groesser sein als L - L und R sind nicht unwichtiger").
    static void phaseLayout (juce::Button& b, juce::Rectangle<float> area,
                             juce::Rectangle<float>& iconR, juce::Rectangle<float>& textR)
    {
        const auto f    = phaseFontFor (b);   // Runde 113/154
        const float cap = f.getHeight() * 0.74f;
        const float iw  = cap * 1.2f, gap = 3.0f * f.getHeight() / 14.5f;
        const float tw  = juce::GlyphArrangement::getStringWidth (f, b.getButtonText()) + 1.0f;
        const float x0  = area.getCentreX() - (iw + gap + tw) * 0.5f;
        iconR = { x0, area.getCentreY() - cap * 0.5f, iw, cap };
        textR = { x0 + iw + gap, area.getY(), tw + 4.0f, area.getHeight() };
    }

    // Runde 110: Icon links + Name rechts fuer die kleinen Schalter in den
    // Sektionskoepfen (FAST, PAIR, x2) - schmaleres Icon als bei den Feldern.
    static void hdrIconLayout (juce::Button& b, juce::Rectangle<float> area,
                               juce::Rectangle<float>& iconR, juce::Rectangle<float>& textR)
    {
        const auto f   = unifiedButtonFont (b.getHeight());
        const float h  = area.getHeight();
        const float iw = h * 1.05f;
        const float gap = h * 0.12f;
        const float tw = juce::GlyphArrangement::getStringWidth (f, b.getButtonText()) + 2.0f;
        const float x0 = area.getCentreX() - (iw + gap + tw) * 0.5f;
        iconR = { x0, area.getY(), iw, h };
        textR = { x0 + iw + gap, area.getY(), tw + 6.0f, h };
    }

    // Farbe eines Soft-Chips - dieselbe Logik wie btnAccent in
    // drawButtonBackground: Gold = an (altAccent), Blau = gekoppelt.
    // Runde 166: Schrift der kleinen Kopf-Schalter (EQ -> LCR, FAST, LINK).
    static juce::Font hdrTextFont (juce::Button& b)
    {
        const float sz = (float) (double) b.getProperties().getWithDefault ("hdrTextSize", 11.5);
        return juce::Font (juce::FontOptions (sz, juce::Font::bold)).withExtraKerningFactor (0.07f);
    }
    // Breite, die Anzeiger + Wort brauchen (fuer das Layout im Editor).
    static int ctlContentWidth (juce::Button& b)
    {
        const int ctl = (int) b.getProperties().getWithDefault ("ctlStyle", 0);
        const float tw = juce::GlyphArrangement::getStringWidth (hdrTextFont (b), b.getButtonText());
        return juce::roundToInt (tw) + (ctl == 2 ? 20 : 13) + 1;
    }
    static void drawCtlIndicator (juce::Graphics& g, juce::Button& b, int ctl, bool lit, bool secOff, bool hot, juce::Colour acc)
    {
        const auto lb = b.getLocalBounds().toFloat();
        const float tw = juce::GlyphArrangement::getStringWidth (hdrTextFont (b), b.getButtonText());
        const float cy = lb.getCentreY();
        const float k  = secOff ? 0.45f : 1.0f;
        if (ctl == 1)
        {
            // LED: aus = leerer Ring, an = gefuellter Punkt mit Schein.
            const float cx = lb.getRight() - tw - 8.0f;
            if (lit && ! secOff)
            {
                juce::ColourGradient glow (acc.withAlpha (0.30f), cx, cy, acc.withAlpha (0.0f), cx + 8.0f, cy, true);
                g.setGradientFill (glow);
                g.fillEllipse (cx - 8.0f, cy - 8.0f, 16.0f, 16.0f);
            }
            if (lit)
            {
                g.setColour (acc.withAlpha (k));
                g.fillEllipse (cx - 3.4f, cy - 3.4f, 6.8f, 6.8f);
            }
            else
            {
                g.setColour ((hot ? juce::Colour (0xff9aa0ab) : juce::Colour (0xff6d7280)).withMultipliedAlpha (k));
                g.drawEllipse (cx - 3.2f, cy - 3.2f, 6.4f, 6.4f, 1.3f);
            }
            return;
        }
        // Mini-Schalter: Spur 15 x 9, Knopf rechts = an. Kompakt - im
        // Phaser-Kopf ist neben dem Namen nur ~125 px Platz fuer beide.
        const float tw2 = 15.0f, th = 9.0f;
        const float x0 = lb.getRight() - tw - 5.0f - tw2;
        const juce::Rectangle<float> track (x0, cy - th * 0.5f, tw2, th);
        g.setColour (lit ? acc.withAlpha (0.45f * k) : juce::Colour (0xff2a2d35).withMultipliedAlpha (secOff ? 0.6f : 1.0f));
        g.fillRoundedRectangle (track, th * 0.5f);
        const float kr = 3.1f;
        const float kx = lit ? track.getRight() - th * 0.5f : track.getX() + th * 0.5f;
        const juce::Colour knob = lit ? acc.interpolatedWith (juce::Colours::white, 0.65f)
                                      : (hot ? juce::Colour (0xff9aa0ab) : juce::Colour (0xff6d7280));
        g.setColour (knob.withMultipliedAlpha (k));
        g.fillEllipse (kx - kr, cy - kr, kr * 2.0f, kr * 2.0f);
    }

    juce::Colour softChipAccent (juce::Button& b) const
    {
        const bool gold   = b.getProperties().getWithDefault ("pairedGold", false)
                         || (bool) b.getProperties().getWithDefault ("pairTint", false);
        const bool altAcc = b.getProperties().getWithDefault ("altAccent", false) && ! isSciFiTheme();
        return gold ? pairAccentColour() : altAcc ? altAccentColour() : glowAccent;
    }

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
        else if (button.getProperties().contains ("contactIcon"))
        {
            drawContactContent (g, button, (int) button.getProperties()["contactIcon"]);
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
            g.setColour (secOff ? labelOffColour()
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
            {
                // Runde 60 (User: "Early, Bars sind fett; Shimmer, Double,
                // Fast, Pair sind duenn - genau die MITTE von beidem nehmen
                // und auf ALLE Buttons anwenden"): eine einzige Regel fuer
                // jede Beschriftung. Vorher hingen Groesse und Sperrung an
                // der Knopfart, und je nach Knopfhoehe kam etwas anderes raus.
                if (button.getProperties().getWithDefault ("modePill", false))
                {
                    // Runde 68 (User: "die Schrift ... da geht noch was"):
                    // Rahmen und Beschriftung waren zwei verschiedene Farben -
                    // goldener Ring, weisses Wort. Die Pille liest sich damit
                    // als zwei Teile. Jetzt traegt die Schrift denselben Ton
                    // wie ihr Rahmen, nur heller gezogen, damit sie lesbar
                    // bleibt: ein Element statt zwei.
                    auto pc = button.getProperties().contains ("pillColour")
                                ? juce::Colour ((juce::uint32) (int) button.getProperties()["pillColour"])
                                : juce::Colour (0xffc3c8d2);
                    g.setColour (secOff ? labelOffColour()
                                        : pc.interpolatedWith (juce::Colour (0xfff2f4f8), 0.34f));   // Runde 69: etwas weniger Richtung Weiss
                }
                if (button.getProperties().getWithDefault ("goldText", false))
                    g.setColour (juce::Colour (0xffd9b45f));   // lnk.bio, Unterstuetzen (User Runde 60)
                g.setFont (unifiedButtonFont (button.getHeight()));
                // Runde 174: feste, lesbare Groesse fuer Knoepfe in Karten (Tour).
                if (button.getProperties().contains ("btnFontPx"))
                    g.setFont (juce::Font (juce::FontOptions ((float) (double) button.getProperties()["btnFontPx"], juce::Font::bold))
                                   .withExtraKerningFactor (0.04f));
            }
            // "textYShift" (Runde 49): schiebt die Beschriftung nach oben,
            // wenn die Modus-Punkte INNERHALB der Pille sitzen.
            const int textShiftY = (int) (double) button.getProperties().getWithDefault ("textYShift", 0.0);
            // Runde 88: liegt links ein Diagramm, bekommt die Schrift den Rest
            // der Pille und bleibt darin mittig - sonst saesse sie auf dem Bild.
            // Runde 108: Soft-Chips tragen die Schrift in ihrer eigenen Farbe -
            // an hell getoent, aus zurueckgenommen.
            if ((bool) button.getProperties().getWithDefault ("softChip", false)
                && ! (bool) button.getProperties().getWithDefault ("galaxyBtn", false))
            {
                const bool secOffC = button.getProperties().getWithDefault ("sectionOff", false);
                const bool litC = button.getToggleState() || (bool) button.getProperties().getWithDefault ("pairedGold", false);
                g.setColour (secOffC ? labelOffColour()
                                     : litC ? softChipAccent (button).interpolatedWith (juce::Colour (0xfff2f4f8), 0.42f)
                                            : juce::Colour (0xff8f96a4));
            }
            // Runde 112: AG - nur der Wert, gedaempft; aus = OFF, noch leiser.
            if ((bool) button.getProperties().getWithDefault ("plainValue", false))
            {
                // Runde 174: im Bypass (sectionOff) wie aus gezeichnet.
                const bool on  = button.getToggleState() && ! (bool) button.getProperties().getWithDefault ("sectionOff", false);
                const bool hot = shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown;
                g.setColour (on ? themePalette().knob.withAlpha (hot ? 1.0f : 0.85f)
                                : iconOffColour().withAlpha (hot ? 0.85f : 0.55f));
                g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)).withExtraKerningFactor (0.04f));
                g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
                return;
            }
            // Runde 110: Icon-Schalter (ØL/ØR, FAST, PAIR, x2).
            if (((int) button.getProperties().getWithDefault ("hdrIcon", 0) > 0
                    || (bool) button.getProperties().getWithDefault ("phaseIcon", false)))
            {
                const bool secOffT = button.getProperties().getWithDefault ("sectionOff", false);
                const bool litT    = button.getToggleState() || (bool) button.getProperties().getWithDefault ("pairedGold", false);
                // Runde 112: Schrift in DERSELBEN Farbe wie das Icon.
                g.setColour (iconToggleColour (softChipAccent (button), litT, secOffT,
                                               shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown));
                juce::Rectangle<float> iconR, textR;
                if ((bool) button.getProperties().getWithDefault ("phaseIcon", false))
                {
                    if (! litT && ! secOffT)   // Runde 165: aus = zurueckgenommen (wie das Ø)
                        g.setColour (phaseOffColour (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown));
                    g.setFont (phaseFontFor (button));
                    phaseLayout (button, button.getLocalBounds().toFloat(), iconR, textR);
                    g.drawText (button.getButtonText(), textR, juce::Justification::centredLeft, false);
                }
                else if ((int) button.getProperties().getWithDefault ("hdrIcon", 0) == 3)
                {
                    hdrIconLayout (button, button.getLocalBounds().toFloat(), iconR, textR);
                    g.drawText (button.getButtonText(), textR, juce::Justification::centredLeft, false);
                }
                else
                {
                    // Runde 111: nur der Name, mittig.
                    // Runde 161 (User: "EQ -> LCR, FAST, LINK zu klein"):
                    // eigene Schriftgroesse per Property.
                    if (button.getProperties().contains ("hdrTextSize"))
                        g.setFont (hdrTextFont (button));
                    if ((int) button.getProperties().getWithDefault ("ctlStyle", 0) > 0)
                    {
                        g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centredRight, false);
                        return;
                    }
                    g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
                }
                return;
            }
            // Runde 108: Icon links, Name rechts daneben (Smart-Profil, PRE/POST).
            if ((int) button.getProperties().getWithDefault ("profileDiagram", -1) >= 0
             || (int) button.getProperties().getWithDefault ("ppDiagram", -1) >= 0)
            {
                // Runde 112 (User): PRE/POST-Schrift dimmt genau wie ihr Icon -
                // Sektion aus ODER gar nichts umgepolt.
                if ((int) button.getProperties().getWithDefault ("ppDiagram", -1) >= 0)
                    g.setColour (iconToggleColour (altAccentColour(), button.getToggleState(),
                                                   (bool) button.getProperties().getWithDefault ("sectionOff", false),
                                                   shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown));
                if ((bool) button.getProperties().getWithDefault ("iconAbove", false))
                {
                    // Runde 110: Smart-Profil wie die Icon-Felder - Name unter dem Icon.
                    auto lbT = button.getLocalBounds();
                    g.drawText (button.getButtonText(),
                                lbT.withTrimmedTop (juce::roundToInt ((float) lbT.getHeight() * 0.64f)).translated (0, textShiftY),
                                juce::Justification::centred, false);
                    return;
                }
                juce::Rectangle<float> iconR, textR;
                inlineIconLayout (button, button.getLocalBounds().toFloat(), iconR, textR);
                g.drawText (button.getButtonText(), textR.translated (0.0f, (float) textShiftY),
                            juce::Justification::centredLeft, false);
                return;
            }
            auto textArea = button.getLocalBounds().translated (0, textShiftY);
            if ((int) button.getProperties().getWithDefault ("pxDiagram",  -1) >= 0
             || (int) button.getProperties().getWithDefault ("rayDiagram", -1) >= 0
             || (int) button.getProperties().getWithDefault ("eqDiagram",  -1) >= 0)
            {
               #if SPACEX_PX_DIAG_ONLY
                // Runde 91 (User): Schrift so weit runter wie moeglich, damit
                // sie mit den Punkten darunter eine Einheit bildet.
                textArea = textArea.withTrimmedTop (juce::roundToInt (textArea.getHeight() * 0.64f));
               #else
                textArea = textArea.withTrimmedLeft (21);
               #endif
            }
            g.drawText (button.getButtonText(), textArea, juce::Justification::centred);
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
    // Die eine Beschriftungsschrift fuer alle Knoepfe (Runde 60).
    static juce::Font unifiedButtonFont (int buttonHeight)
    {
        return juce::Font (juce::FontOptions (juce::jmin ((float) buttonHeight * 0.40f, 13.0f), juce::Font::bold))
                   .withExtraKerningFactor (0.10f);
    }

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
        // Runde 110 (User: "die letzten Kaesten"): kein Gehaeuse-Rechteck
        // mehr - nur die Membran, und an leuchtet sie (Rot bleibt: "nur zum
        // Abhoeren, nicht vergessen").
        if (isOn)
            softIconGlow (g, centre, s * 0.5f, monoOnColour, 0.8f + 0.6f * alpha);
        g.setColour (col);
        g.drawEllipse (centre.x - s * 0.30f, centre.y - s * 0.30f, s * 0.60f, s * 0.60f, 1.4f);
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
        // Runde 117 (User): Sektion aus oder durch Solo stumm -> das Schloss
        // dimmt wie alles andere. Lock hebt Solo nicht auf, ein volles Orange
        // wuerde "diese Sektion laeuft" suggerieren. Gesperrt bleibt als
        // schwaches Orange erkennbar.
        const bool secOff = button.getProperties().getWithDefault ("sectionOff", false);
        g.setColour (col.withAlpha (secOff ? (locked ? 0.35f : 0.40f)
                                           : (locked ? 1.0f  : 0.7f)));

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

        // Runde 110 (User): Reset ist keine Gefahr - neutral wie die anderen
        // Werkzeuge, Rot erst, wenn die Maus darauf steht.
        auto col = juce::Colour (0xffb5b9c2).withAlpha (button.isEnabled() ? 1.0f : 0.3f);
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

        // Runde 110: neutral - Blau heisst "gekoppelt", das ist Kopieren nicht.
        auto col = lit ? juce::Colour (0xffd7dbe4) : iconOffColour();
        if (lit && (down || highlighted))
            col = juce::Colours::white;
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
        // Runde 110 (User): Soft-Flaeche wie die Chips statt Umriss - das Feld
        // war das letzte Element der Kopfzeile mit Linie.
        const float corner = bounds.getHeight() * 0.5f;
        g.setColour (juce::Colours::white.withAlpha (down ? 0.10f : highlighted ? 0.075f : 0.05f));
        g.fillRoundedRectangle (bounds, corner);

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
            { -0.30f, isOn ? themePalette().prism.brighter (0.25f) : offGrey },   // je Theme (User)
            {  0.02f, isOn ? themePalette().prism : offGrey },
            {  0.34f, isOn ? themePalette().prism.darker (0.25f) : offGrey }
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

        // Runde 112 (User: "Link-Icon ist nicht umgesetzt"): zwischen ØL und
        // ØR zwei waagerechte Kettenglieder, leise wie die Aus-Schalter.
        if ((bool) button.getProperties().getWithDefault ("noLinkLines", false))
        {
            const float s  = juce::jmin (full.getWidth(), full.getHeight());
            const auto  c  = full.getCentre();
            const float w  = s * 0.52f, h = s * 0.34f;
            const bool  hotL = highlighted || down;
            g.setColour (sectionIsOff ? iconOffColour().withAlpha (0.30f)
                                      : iconOffColour().withAlpha (down ? 1.0f : hotL ? 0.85f : 0.55f));
            juce::Path a, b;
            a.addRoundedRectangle (c.x - w + w * 0.16f, c.y - h * 0.5f, w, h, h * 0.5f);
            b.addRoundedRectangle (c.x - w * 0.16f,     c.y - h * 0.5f, w, h, h * 0.5f);
            const juce::PathStrokeType st (juce::jmax (1.2f, s * 0.09f), juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded);
            g.strokePath (a, st);
            g.strokePath (b, st);
            return;
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
        // Runde 110: sitzt der Link zwischen ØL und ØR, braucht er keine Linien.
        if (! (bool) button.getProperties().getWithDefault ("noLinkLines", false))
        {
            g.setColour (lineCol.withAlpha (sectionIsOff ? 0.45f : 0.6f));
            g.drawLine (endInsetL, lineY, iconBounds.getX() - 2.0f, lineY, 0.6f);
            g.drawLine (iconBounds.getRight() + 2.0f, lineY, endInsetR, lineY, 0.6f);
        }

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
    void drawSmallIconPlate (juce::Graphics& g, juce::Rectangle<float> b, bool on, bool sectionOff = false)
    {
        const float cr = juce::jmin (b.getWidth(), b.getHeight()) * 0.28f;
        // Die FLAECHE sagt nicht mehr, ob der Knopf an ist (User: "die kleinen
        // Buttons leuchten viel zu stark ... sie sollen dezent sein"). Sie ist
        // immer die ruhige Grundflaeche; an/aus steht allein im Symbol darueber.
        // Vorbild ist der RAYE-Stufenknopf bei ausgeschalteter Sektion, den der
        // User als "perfekt" bezeichnet hat.
        // Runde 62 (User, mehrfach): ist die SEKTION aus, bekommt der Knopf
        // gar keine Flaeche mehr - genau wie die Modus-Pillen daneben. Die
        // graue Fuellung war das einzige, was in einer ausgeschalteten
        // Sektion noch hell stand.
        if (! sectionOff)
        {
            g.setColour (controlIdleFill());
            g.fillRoundedRectangle (b, cr);
        }
        g.setColour (juce::Colours::white.withAlpha (sectionOff ? 0.06f : (on ? 0.13f : 0.07f)));
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
        drawSmallIconPlate (g, plate, lit, sectionIsOff);

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
        // Runde 110: A und B sind gleichwertig - aktiv heisst in beiden Faellen
        // dieselbe Farbe (Gold = an).
        const juce::Colour activeA = sectionIsOff ? iconOnInOffSection() : themePalette().knob;
        const juce::Colour activeB = activeA;
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
            // Runde 111: Theme-Farbe statt Ampel-Gruen (Kopfzeile ruhiger).
            juce::ignoreUnused (onGreen, offRed);
            g.setColour (themePalette().knob.withAlpha (pulse));
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

        // Runde 110 (User: "Kopfzeile ruhiger"): kein Ampel-Gruen/Rot mehr.
        // An = die Theme-Farbe, Bypass = gedimmt - wie jedes andere Icon.
        auto col = themePalette().knob.interpolatedWith (juce::Colour (0xfff2f4f8), 0.15f);
        auto centre = bounds.getCentre();
        const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;

        // Runde 168 (User: "Bypass soll erkennbarer sein, wenn alles gedimmt
        // ist"): im Bypass ist dieser Knopf das EINZIGE, was noch leuchtet -
        // ein ruhig atmender Schein in der Theme-Farbe und ein heller Ring
        // darum. So sieht man sofort, warum alles grau ist und wo man
        // zurueckschaltet. Der Editor-Timer zeichnet ihn dafuer neu.
        if (bypassed)
        {
            const double t = juce::Time::getMillisecondCounterHiRes() * 0.001;
            const float breath = 0.5f + 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * t / 2.4);
            const auto acc = themePalette().knob;
            const auto lb  = button.getLocalBounds().toFloat();
            softIconGlow (g, lb.getCentre(), juce::jmin (lb.getWidth(), lb.getHeight()) * 0.5f, acc, 1.1f + 0.9f * breath);
            // Runde 174 (User: "Power Button sollte visuell hervorgehoben
            // werden, wenn aktiv"): kraeftiger als Runde 168 - getoente
            // Flaeche, satterer Ring, Symbol in der Akzentfarbe. Die dunkle
            // Ebene ueber der GUI, die ihn vorher mit abgedunkelt hat, ist weg.
            g.setColour (acc.withAlpha (0.14f + 0.08f * breath));
            g.fillEllipse (centre.x - r * 0.98f, centre.y - r * 0.98f, r * 1.96f, r * 1.96f);
            g.setColour (acc.withAlpha (0.60f + 0.30f * breath));
            g.drawEllipse (centre.x - r * 0.98f, centre.y - r * 0.98f, r * 1.96f, r * 1.96f, 1.6f);
            col = acc.brighter (0.35f);
        }
        g.setColour (col);
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
    // Back Panel (Runde 56, User: "Instagram Logo besser als Button", "Email
    // Icon besser als langer Text"): Symbol links, Adresse daneben. Bewusst
    // GENERISCHE Zeichen - Briefumschlag, Globus, Fotoapparat - und nicht die
    // Markenzeichen der Dienste: fremde Logos gehoeren nicht nachgebaut, und
    // erkennbar ist es so genauso.
    void drawContactContent (juce::Graphics& g, juce::Button& button, int kind)
    {
        auto b = button.getLocalBounds().toFloat().reduced (10.0f, 5.0f);
        const bool hot = button.isMouseOver();
        const auto col = juce::Colour (0xffb5b9c2).interpolatedWith (themePalette().knob, hot ? 0.75f : 0.25f);
        const float ih = juce::jmin (b.getHeight(), 15.0f);
        juce::Rectangle<float> icon (b.getX(), b.getCentreY() - ih * 0.5f, ih * 1.25f, ih);
        g.setColour (col);

        if (kind == 0)          // Briefumschlag
        {
            auto e = icon.reduced (0.0f, ih * 0.14f);
            g.drawRoundedRectangle (e, 2.0f, 1.3f);
            juce::Path flap;
            flap.startNewSubPath (e.getX() + 1.5f, e.getY() + 2.0f);
            flap.lineTo (e.getCentreX(), e.getCentreY() + 1.5f);
            flap.lineTo (e.getRight() - 1.5f, e.getY() + 2.0f);
            g.strokePath (flap, juce::PathStrokeType (1.3f));
        }
        else if (kind == 1)     // Globus
        {
            auto c = icon.withSizeKeepingCentre (ih, ih).reduced (0.5f);
            g.drawEllipse (c, 1.3f);
            g.drawEllipse (c.reduced (c.getWidth() * 0.30f, 0.0f), 1.1f);
            g.drawLine (c.getX(), c.getCentreY(), c.getRight(), c.getCentreY(), 1.1f);
        }
        else                    // Fotoapparat (generisch, kein Markenzeichen)
        {
            auto body = icon.withTrimmedTop (ih * 0.18f);
            g.drawRoundedRectangle (body, 2.5f, 1.3f);
            g.drawRoundedRectangle (juce::Rectangle<float> (body.getX() + body.getWidth() * 0.28f,
                                                            icon.getY(), body.getWidth() * 0.26f, ih * 0.22f), 1.0f, 1.2f);
            const float r = body.getHeight() * 0.30f;
            g.drawEllipse (body.getCentreX() - r, body.getCentreY() - r, r * 2.0f, r * 2.0f, 1.3f);
        }

        g.setColour (col);
        g.setFont (juce::Font (juce::FontOptions (13.0f)).withExtraKerningFactor (0.02f));
        g.drawText (button.getButtonText(), b.withTrimmedLeft (icon.getWidth() + 10.0f).toNearestInt(),
                    juce::Justification::centredLeft, false);
    }

    void drawMutateContent (juce::Graphics& g, juce::Button& button)
    {
        // Runde 161 (User: D1 "gefaellt mir gar nicht", "probier deinen"):
        // ein echter Wuerfel (D2) - abgerundetes Quadrat mit Augen, im
        // Linienstil der Nachbar-Icons und etwa so gross wie Power. Jeder Wurf
        // zeigt eine andere Augenzahl (mutateColorState). Mit Smart-Profil
        // leuchtet er in der Profilfarbe, leicht gefuellt und mit Schein;
        // ohne Profil steht er gedaempft in derselben Farbe.
        const auto lb = button.getLocalBounds().toFloat();
        const int colorState = (int) button.getProperties().getWithDefault ("mutateColorState", 4);
        const bool armed = button.getProperties().getWithDefault ("categoryArmed", false);
        const bool hot   = button.isOver() || button.isDown();
        const auto acc   = themePalette().knob;

        const float side = juce::jmin (21.0f, juce::jmin (lb.getWidth(), lb.getHeight()) * 0.64f);
        const auto  c    = lb.getCentre();
        const juce::Rectangle<float> die (c.x - side * 0.5f, c.y - side * 0.5f, side, side);
        const float corner = side * 0.26f;
        const float stroke = juce::jmax (1.3f, side * 0.075f);

        if (armed)
            softIconGlow (g, c, juce::jmin (lb.getWidth(), lb.getHeight()) * 0.5f, acc, hot ? 1.2f : 0.85f);

        const juce::Colour col = armed ? acc.interpolatedWith (juce::Colour (0xfff2f4f8), hot ? 0.28f : 0.14f)
                                       : acc.withAlpha (hot ? 0.95f : 0.70f);
        if (armed)
        {
            g.setColour (acc.withAlpha (0.14f));
            g.fillRoundedRectangle (die, corner);
        }
        g.setColour (col);
        g.drawRoundedRectangle (die.reduced (stroke * 0.5f), corner, stroke);

        // Augen: 3x3-Raster. Runde 164: die Augenzahl = aktive Sektionen
        // (0..6), gesetzt vom Editor ("dieFace"), beim Wurf kurz rollend.
        juce::ignoreUnused (colorState);
        const int face = juce::jlimit (0, 6, (int) button.getProperties().getWithDefault ("dieFace", 6));
        static constexpr int kFaces[7][9] = {
            {0,0,0, 0,0,0, 0,0,0},
            {0,0,0, 0,1,0, 0,0,0},   // 1
            {1,0,0, 0,0,0, 0,0,1},   // 2
            {1,0,0, 0,1,0, 0,0,1},   // 3
            {1,0,1, 0,0,0, 1,0,1},   // 4
            {1,0,1, 0,1,0, 1,0,1},   // 5
            {1,0,1, 1,0,1, 1,0,1} }; // 6
        const float step = side * 0.25f;
        const float pr   = juce::jmax (1.2f, side * 0.085f);
        for (int i = 0; i < 9; ++i)
            if (kFaces[face][i] != 0)
            {
                const float px = c.x + (float) (i % 3 - 1) * step;
                const float py = c.y + (float) (i / 3 - 1) * step;
                g.fillEllipse (px - pr, py - pr, pr * 2.0f, pr * 2.0f);
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
        // Runde 108: auch LCR wird ein Soft-Chip (blau = gekoppelt: die Engine
        // bringt Latenz mit). Das langsame Atmen bleibt, nur leiser.
        if ((bool) button.getProperties().getWithDefault ("softChip", false))
        {
            const double t = juce::Time::getMillisecondCounterHiRes() * 0.001;
            const float pulse = (float) (0.5 + 0.5 * std::sin (juce::MathConstants<double>::twoPi * t / 1.8));
            drawSoftChip (g, bounds, galaxyBlue.withMultipliedBrightness (0.85f + 0.25f * pulse),
                          button.getToggleState(), false, highlighted, down);
            return;
        }
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
        // Runde 125: Mini-Fader im MID-SIDE-Kopf (EQ-Menge). Duenne Spur,
        // Fuellung bis zum Griff, Strich bei 50 % (= Kurve wie abgestimmt).
        if (slider.getProperties().getWithDefault ("miniFader", false))
        {
            const bool off = slider.getProperties().getWithDefault ("sectionOff", false);
            const auto acc = slider.getProperties().contains ("faderColour")
                               ? juce::Colour ((juce::uint32) (int) slider.getProperties()["faderColour"])
                               : altAccentColour();
            const auto col = off ? labelOffColour() : acc;
            const float cy = (float) y + (float) height * 0.5f;
            const float x0 = (float) x, x1 = (float) (x + width);
            const float px = juce::jlimit (x0, x1, sliderPos);
            const float th = 2.4f;
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.fillRoundedRectangle (x0, cy - th * 0.5f, x1 - x0, th, th * 0.5f);
            g.setColour (col.withAlpha (off ? 0.55f : 0.80f));
            g.fillRoundedRectangle (x0, cy - th * 0.5f, juce::jmax (0.0f, px - x0), th, th * 0.5f);
            const float mx = (x0 + x1) * 0.5f;
            g.setColour (juce::Colours::white.withAlpha (off ? 0.14f : 0.26f));
            g.fillRect (mx - 0.5f, cy - 4.0f, 1.0f, 8.0f);
            const float r = slider.isMouseOverOrDragging() && ! off ? 5.0f : 4.4f;
            g.setColour (col);
            g.fillEllipse (px - r, cy - r, r * 2.0f, r * 2.0f);
            return;
        }
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

        const bool offVisual = slider.getProperties().getWithDefault ("sectionOff", false) || ! slider.isEnabled()
                            || (bool) slider.getProperties().getWithDefault ("syncLocked", false);   // Runde 130

        // Runde 174 (User, Entwurf C): L/R als drei Balken L · C · R. C steht
        // fest, L und R wachsen mit dem Wert - bei 0 % sind alle drei gleich
        // hoch (Original), nach oben kommen die Seiten dazu. Kein technischer
        // Fader, sondern ein Bild dessen, was passiert.
        if ((bool) slider.getProperties().getWithDefault ("lcrBars", false))
        {
            auto b = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
            const float top = b.getY() + 4.0f, bottom = b.getBottom() - 4.0f, h = bottom - top;
            const float cx  = b.getCentreX();
            const float valT = juce::jlimit (0.0f, 1.0f, (float) slider.valueToProportionOfLength (slider.getValue()));
            const double tSec = juce::Time::getMillisecondCounterHiRes() * 0.001;   // fuer die leise Animation
            const bool modLive = ! offVisual && (bool) slider.getProperties().getWithDefault ("modLiveActive", false);
            const float liveT  = juce::jlimit (0.0f, 1.0f, (float) slider.getProperties().getWithDefault ("modLiveValue", 0.0f));
            const juce::Colour cNeutral = offVisual ? knobRingOffColour()
                                                    : themePalette().frameMain.interpolatedWith (juce::Colour (0xffc9c5be), 0.35f);
            // Weicher Leuchtpunkt (radialer Verlauf) - fuer Staub und Monde.
            auto glowDot = [&] (float gx, float gy, float r, juce::Colour c, float a)
            {
                if (a <= 0.003f || r <= 0.1f) return;
                juce::ColourGradient gr (c.withAlpha (a), gx, gy, c.withAlpha (0.0f), gx + r, gy, true);
                gr.addColour (0.35, c.withAlpha (a * 0.55f));
                g.setGradientFill (gr);
                g.fillEllipse (gx - r, gy - r, r * 2.0f, r * 2.0f);
            };

            // ===== Stil "Orbit" (Settings > L/R Orbit, Runde 174, Beta-Vergleich) =====
            // Bei 0 % sind C, L und R gleich gross (ausbalanciert). Aufdrehen:
            // die Monde L und R wachsen und wandern ein Stueck nach aussen, C
            // gibt sein Licht an sie ab und ist bei 100 % verschwunden. Die
            // ganze Flaeche ist der Regler; eine leuchtende Kante zeigt den Wert.
            if (uiLrOrbitRef())
            {
                const float R  = b.getWidth() * 0.5f;
                const float cy = top + h * 0.46f;
                const float sp = R * (0.50f + 0.20f * valT);
                const float sR = R * (0.18f + 0.12f * valT);
                const float cR = R * 0.18f * (1.0f - valT);
                const auto gold = offVisual ? knobValueOffColour() : accent;
                const auto blue = offVisual ? knobValueOffColour() : glowAccent;

                // Griff-Flaeche mit Pegel-Schleier und leuchtender Kante
                const auto area = juce::Rectangle<float> (b.getX() + 2.0f, top, b.getWidth() - 4.0f, h);
                const float vy  = juce::jmap (valT, bottom, top);
                g.setColour (juce::Colours::white.withAlpha (0.025f));
                g.fillRoundedRectangle (area, 9.0f);
                g.setColour (juce::Colours::white.withAlpha (offVisual ? 0.04f : 0.07f));
                g.drawRoundedRectangle (area.reduced (0.5f), 9.0f, 1.0f);
                {
                    juce::Path clip; clip.addRoundedRectangle (area, 9.0f);
                    g.saveState();
                    g.reduceClipRegion (clip);
                    if (! offVisual)
                    {
                        g.setGradientFill (juce::ColourGradient (blue.withAlpha (0.10f), 0.0f, bottom, gold.withAlpha (0.16f), 0.0f, vy, false));
                        g.fillRect (area.getX(), vy, area.getWidth(), bottom - vy);
                    }
                    juce::ColourGradient edge (gold.withAlpha (0.0f), area.getX(), vy, gold.withAlpha (0.0f), area.getRight(), vy, false);
                    edge.addColour (0.5, gold.withAlpha (offVisual ? 0.35f : 0.9f));
                    g.setGradientFill (edge);
                    g.fillRect (area.getX(), vy - 1.0f, area.getWidth(), 2.0f);
                    g.restoreState();
                }
                g.setColour (offVisual ? knobValueOffColour() : juce::Colour (0xfff2f4f8));
                g.fillRoundedRectangle (area.getX() - 3.0f, vy - 3.0f, 6.0f, 6.0f, 2.0f);
                g.fillRoundedRectangle (area.getRight() - 3.0f, vy - 3.0f, 6.0f, 6.0f, 2.0f);

                // Umlaufbahn
                g.setColour (juce::Colours::white.withAlpha (0.08f));
                g.drawEllipse (cx - sp, cy - sp * 0.32f, sp * 2.0f, sp * 0.64f, 1.0f);
                // Licht fliesst von C zu L und R
                if (! offVisual && valT > 0.01f && valT < 0.995f)
                    for (int s = -1; s <= 1; s += 2)
                        for (int i = 0; i < 8; ++i)
                        {
                            const float p = (float) std::fmod (tSec * 0.45 * (0.6 + valT) + i / 8.0, 1.0);
                            glowDot (cx + (float) s * sp * p, cy - std::sin (p * juce::MathConstants<float>::pi) * 5.0f * valT,
                                     4.0f, gold, 0.35f * valT * (1.0f - std::abs (p - 0.5f)));
                        }
                // C
                if (cR > 0.3f)
                {
                    if (! offVisual) glowDot (cx, cy, cR * 2.4f, gold, 0.5f * (1.0f - valT));
                    g.setColour (gold.interpolatedWith (juce::Colours::white, 0.5f).withAlpha (0.1f + 0.8f * (1.0f - valT)));
                    g.fillEllipse (cx - cR, cy - cR, cR * 2.0f, cR * 2.0f);
                }
                // L und R
                for (int s = -1; s <= 1; s += 2)
                {
                    const float mx = cx + (float) s * sp;
                    const float my = cy + (offVisual ? 0.0f : (float) std::sin (tSec * 1.2 + s) * 1.2f);
                    if (! offVisual) glowDot (mx, my, sR * 2.4f, blue, 0.30f + 0.35f * valT);
                    g.setColour (blue.interpolatedWith (juce::Colours::white, 0.4f).withAlpha (offVisual ? 0.6f : 0.95f));
                    g.fillEllipse (mx - sR, my - sR, sR * 2.0f, sR * 2.0f);
                }
                // Modulation: Punkt an der Kante rechts, auf der Live-Hoehe
                if (modLive)
                {
                    const float ly = juce::jmap (liveT, bottom, top);
                    g.setColour (juce::Colour (0xcc0a0b0e));
                    g.fillEllipse (area.getRight() - 4.2f, ly - 4.2f, 8.4f, 8.4f);
                    g.setColour (glowAccent);
                    g.fillEllipse (area.getRight() - 2.9f, ly - 2.9f, 5.8f, 5.8f);
                }
                return;
            }

            // ===== Stil "Balken" (Standard, Entwurf 7 "Kombi mit mehr Luft") =====
            // Bei 0 % stehen L, C und R gleich hoch (55 %). Aufdrehen: C sinkt
            // auf null, L und R steigen bis ganz oben. Staub fliegt von C zu
            // den Seiten, solange sich das Verhaeltnis verschiebt.
            const float bw  = juce::jmax (6.0f, b.getWidth() / 3.9f);   // Balken fuellen die Breite (Layout setzt sie passend)
            const float gap = bw * 0.45f;
            constexpr float kBase = 0.55f;
            auto sideH = [&] (float t) { return h * (kBase + (1.0f - kBase) * t); };
            const float cH  = h * kBase * (1.0f - valT);
            const float rad = juce::jmin (5.0f, bw * 0.35f);
            const auto sideCol = offVisual ? knobValueOffColour() : accent.interpolatedWith (glowAccent, valT);
            const bool fairyGrad = isDarkNightTheme() || isDayNightTheme();   // Fairy Tale + Day & Night: unten Gold, oben Blau
            auto bar = [&] (float bx, float fillH, juce::Colour col, float alpha, bool side)
            {
                const juce::Rectangle<float> tr (bx, top, bw, h);
                g.setColour (juce::Colour (0xff23262c));
                g.fillRoundedRectangle (tr, rad);
                g.setColour (offVisual ? knobRingOffColour() : juce::Colour (0xff454952));
                g.drawRoundedRectangle (tr.reduced (0.5f), rad, 1.0f);
                if (fillH <= 0.2f) return;
                juce::Path clip; clip.addRoundedRectangle (tr, rad);
                g.saveState();
                g.reduceClipRegion (clip);
                if (fairyGrad && side && ! offVisual)
                    // Runde 175 (User, Farbregel "Gold = Mitte, Blau = Seiten"):
                    // unten Gold, nach oben Blau - je weiter L/R steigen, desto blauer.
                    g.setGradientFill (juce::ColourGradient (accent.withAlpha (alpha),     bx, bottom,
                                                             glowAccent.withAlpha (alpha), bx, top, false));
                else
                    g.setColour (col.withAlpha (alpha));
                g.fillRect (bx, bottom - fillH, bw, fillH);
                if (side && ! offVisual)
                {
                    g.setColour (juce::Colours::white.withAlpha (0.85f));
                    g.fillRect (bx, bottom - fillH - 1.0f, bw, 2.0f);
                }
                g.restoreState();
            };
            const float xL = cx - bw * 1.5f - gap, xC = cx - bw * 0.5f, xR = cx + bw * 0.5f + gap;
            bar (xL, sideH (valT), sideCol, 0.90f, true);
            bar (xC, cH, cNeutral, offVisual ? 0.6f : 0.85f, false);
            bar (xR, sideH (valT), sideCol, 0.90f, true);
            // Staub von C zu den Seiten (nur waehrend sich etwas verschiebt)
            if (! offVisual && valT > 0.01f && valT < 0.99f)
                for (int i = 0; i < 6; ++i)
                {
                    const float p = (float) std::fmod (tSec * 0.5 + i / 6.0, 1.0);
                    const float s = (i % 2) ? 1.0f : -1.0f;
                    const float dx = cx + s * (bw + gap) * p;
                    const float dy = bottom - cH - 4.0f - std::sin (p * juce::MathConstants<float>::pi) * 14.0f;
                    glowDot (dx, dy, 4.5f, themePalette().knob, 0.5f * valT * (1.0f - valT) * 4.0f * (1.0f - std::abs (p - 0.5f) * 1.4f));
                }

            // Live-Modulation: wie an jedem Drehregler ein Punkt - hier oben
            // auf beiden Seitenbalken.
            if (modLive)
            {
                const float ly = bottom - sideH (liveT);
                for (float bx : { xL, xR })
                {
                    const float dx = bx + bw * 0.5f;
                    g.setColour (juce::Colour (0xcc0a0b0e));
                    g.fillEllipse (dx - 4.2f, ly - 4.2f, 8.4f, 8.4f);
                    g.setColour (glowAccent);
                    g.fillEllipse (dx - 2.9f, ly - 2.9f, 5.8f, 5.8f);
                }
            }
            return;
        }

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

        // Runde 57 (User-Foto): JUCE rechnet sliderPos ueber die GANZE
        // Komponentenhoehe, meine Bahn ist aber oben und unten um 6 px
        // eingerueckt - bei 0 % sass der Wertstrich deshalb 6 px UNTER der
        // Kapsel und sah aus wie ein abgefallenes Teil. Die Position kommt
        // jetzt direkt aus dem Wert, und der Strich laeuft innerhalb der
        // Bahn, mit halber Strichhoehe Abstand zu den runden Enden.
        const float markH = 3.0f;
        const float valT = juce::jlimit (0.0f, 1.0f,
                                         (float) slider.valueToProportionOfLength (slider.getValue()));
        // Bug (User Runde 79): die Fuellung lief von travelBot bis travelTop,
        // also 2,5 px INNERHALB der Bahn - oben blieb dadurch die dunkle
        // Kappe der Bahn stehen, obwohl der Regler auf 100 % stand. Und weil
        // die Fuellung ein Rechteck ueber die VOLLE Breite war, standen ihre
        // Ecken dort, wo die Kapsel schon rund wird, seitlich heraus. Beides
        // sah aus, als laege die Fuellung neben der Form.
        // Jetzt: die Fuellung nutzt die GANZE Bahn (top..bottom) und wird auf
        // deren Form beschnitten - voll heisst voll, leer heisst leer.
        const float fillTopY = juce::jmap (valT, bottom, top);
        const float t = valT;

        juce::Path clipTrack;
        clipTrack.addRoundedRectangle (track, trackR);

        auto valueCol = offVisual ? knobValueOffColour() : accent.interpolatedWith (glowAccent, t);
        if (t > 0.001f)
        {
            g.saveState();
            g.reduceClipRegion (clipTrack);
            g.setColour (valueCol.withAlpha (0.90f));
            g.fillRect (track.getX(), fillTopY, track.getWidth(), bottom - fillTopY);
            g.restoreState();
        }
        const float travelTop = top;
        const float travelBot = bottom;

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
            const float liveY = juce::jmap (liveT, travelBot, travelTop);
            // Runde 55 (User-Skizze): getauscht. Die MODULATION ist jetzt der
            // Punkt - genau wie an jedem Drehregler im Plugin - und der Wert
            // darunter der Strich. Vorher war es andersherum, und Orbit war
            // damit das einzige Element, bei dem der Punkt nicht die
            // Modulation meinte.
            g.setColour (juce::Colour (0xcc0a0b0e));
            g.fillEllipse (cx - 4.6f, liveY - 4.6f, 9.2f, 9.2f);
            g.setColour (glowAccent);
            g.fillEllipse (cx - 3.2f, liveY - 3.2f, 6.4f, 6.4f);
        }

        // Aktuelle Position als leuchtender Punkt - ebenfalls ueber der Live-
        // Linie, bleibt also immer als eigener (tuerkiser) Punkt erkennbar.
        // Der einstellbare Wert ist der Strich - und zwar INNERHALB der Bahn,
        // nicht breiter als sie (User-Foto Runde 57: sonst liest man ihn als
        // eigenes Element unter dem Fader statt als dessen Stellung).
        auto dotCol = offVisual ? juce::Colour (0xff777b85) : valueCol;
        {
            g.saveState();
            g.reduceClipRegion (clipTrack);
            // Bug (User Runde 71): an den Endanschlaegen lag der weisse
            // Strich mitten in der runden Kappe der Bahn. Der Zuschnitt machte
            // daraus oben einen weissen Knubbel und unten einen hellen Rest
            // in der leeren Bahn. An den Enden sagt aber die FUELLUNG schon
            // alles - ganz voll oder ganz leer sieht man ohne Strich -, also
            // blendet er dort weich aus, statt sich zu verformen.
            const float endFade = juce::jlimit (0.0f, 1.0f,
                                                juce::jmin (valT, 1.0f - valT) / 0.07f);
            if (endFade > 0.001f)
            {
                g.setColour (dotCol.withAlpha (0.35f * endFade));
                g.fillRect (track.getX(), fillTopY - markH * 1.6f, track.getWidth(), markH * 3.2f);
                g.setColour (juce::Colours::white.withAlpha (endFade));
                g.fillRect (track.getX(), fillTopY - markH * 0.5f, track.getWidth(), markH);
            }
            g.restoreState();
        }
    }

    // Groesserer, klar lesbarer Text fuer die Sync-Raten-Box.
    juce::Font getComboBoxFont (juce::ComboBox& box) override
    {
        // Runde 149: Ordnerfeld im Save-Dialog - normale Lesegroesse wie das
        // Namensfeld daneben, keine Versalien-Knopfschrift.
        if ((bool) box.getProperties().getWithDefault ("dialogField", false))
            return juce::Font (juce::FontOptions (16.0f));
        // Runde 61 (User: "BARS ist immer noch fetter als Spin, Drift, Early"):
        // das Bars-Feld ist eine ComboBox und lief deshalb an der gemeinsamen
        // Knopfschrift vorbei. Jetzt dieselbe Regel wie ueberall.
        // Runde 68 (User: "Bars passt irgendwie auch nicht dazu"): exakt
        // dieselbe Schrift wie die Modus-Pillen. Die Zahl liest sich jetzt
        // ueber die Versalien gross genug, nicht ueber einen Sonderwert.
        return unifiedButtonFont (box.getHeight());
    }

    // ComboBox mit Glow-Rahmen, wenn die Component-Property "glowActive"
    // gesetzt ist (genutzt fuer die Sync-Raten-Box, wenn Sync aktiv ist).
    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                        int, int, int, int, juce::ComboBox& box) override
    {
        // Runde 149: Eingabefeld-Form im Save-Dialog (wie das Namensfeld:
        // dunkle Mulde, feiner Rand, im Hover/Offen etwas heller) + Pfeil.
        if ((bool) box.getProperties().getWithDefault ("dialogField", false))
        {
            auto f = juce::Rectangle<float> (0, 0, (float) width, (float) height);
            const bool hot = box.isMouseOver (true) || box.isPopupActive();
            g.setColour (juce::Colour (0xff14161b));
            g.fillRoundedRectangle (f, 8.0f);
            g.setColour (box.isPopupActive() ? themePalette().knob.withAlpha (0.60f)
                                             : juce::Colours::white.withAlpha (hot ? 0.18f : 0.10f));
            g.drawRoundedRectangle (f.reduced (0.5f), 8.0f, box.isPopupActive() ? 1.3f : 1.0f);
            const float cx = (float) width - 18.0f, cy = (float) height * 0.5f;
            juce::Path chevron;
            chevron.startNewSubPath (cx - 4.5f, cy - 2.2f);
            chevron.lineTo (cx, cy + 2.6f);
            chevron.lineTo (cx + 4.5f, cy - 2.2f);
            g.setColour (juce::Colour (0xff9ba0aa).withAlpha (hot ? 1.0f : 0.75f));
            g.strokePath (chevron, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            return;
        }
        auto bounds = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (1.0f);
        const bool glow = box.getProperties().getWithDefault ("glowActive", false)
                           && ! box.getProperties().getWithDefault ("sectionOff", false);

        // Bars-Feld folgt jetzt dem Theme wie alles andere (User: "das Bars-
        // Menu sollte sich auch ans Theme anpassen, bei ALLEN Themes") - die
        // festen Werte 20232a / 33363f waren ein Fremdkoerper in jedem Theme
        // ausser dem urspruenglichen.
        // Runde 68 (User): keine eigene Flaeche mehr. Das Bars-Feld ist eine
        // Auswahl wie DOUBLE oder SHIMMER - also auch dieselbe Form: Kapsel,
        // nur Umriss, kein Fuellton. Es war das einzige Element in dieser
        // Reihe, das aus der Familie fiel.
        const bool boxSectionOff = box.getProperties().getWithDefault ("sectionOff", false);

        const bool goldBox = box.getProperties().getWithDefault ("pairedGold", false);
        {
            // Runde 108: Soft-Chip wie die Schalter. Blau = gekoppelt (Sync,
            // Takte) - so steht es in der Farbregel, die der User freigegeben hat.
            // Runde 112 (User: "PAIR hat keine Auswirkung mehr auf BARS"):
            // wieder wie in Runde 64 - normal Gold, bei aktivem PAIR Blau.
            drawSoftChip (g, bounds, goldBox ? pairAccentColour() : themePalette().frameRaye,
                          glow || goldBox, boxSectionOff, box.isMouseOver (true), false);
        }

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
        if ((bool) box.getProperties().getWithDefault ("dialogField", false))
        {
            label.setBounds (2, 1, box.getWidth() - 34, box.getHeight() - 2);
            label.setBorderSize ({ 0, 10, 0, 0 });
            label.setFont (getComboBoxFont (box));
            label.setJustificationType (juce::Justification::centredLeft);
            return;
        }
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
