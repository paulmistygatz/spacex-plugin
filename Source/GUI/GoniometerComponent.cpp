#ifndef SPACEX_NO_VISUALS
 #define SPACEX_NO_VISUALS 0
#endif
#include "GoniometerComponent.h"
#include "SpaceAssets.h"

// ===== FOTOS (Hintergrund + Gravity) =====
const char* GoniometerComponent::photoName (int idx)
{
    static const char* const names[kPhotoCount + 1] = {
        "None", "Milky Way", "Nebula", "Purple Sky"
    };
    return names[juce::jlimit (0, kPhotoCount, idx)];
}

void GoniometerComponent::setPhoto (int idx)
{
    photoChoice = juce::jlimit (1, kPhotoCount, idx);   // "None" gibt es nicht mehr (User)
    loadPhoto (effectivePhoto());
}

// Gonio-Ansicht (Stars) hat immer Nebula als Hintergrund (User).
// Feste Kombinationen je Theme (User: "wir haben eh fast nur noch fixe
// Kombis"): Modern = Milky Way + Earth, Watercolor = Nebula + Night,
// Comic = gezeichneter Himmel + Moon, Modern Purple = Purple Sky + Sci-Fi.
int GoniometerComponent::effectivePhoto() const
{
    // Stars-Ansicht wechselt das Foto nicht mehr (User: "gleiches Foto, nur
    // viel dunkler"). Modern: kein Foto - fast schwarz + funkelnde Sterne.
    switch (uiTheme)
    {
        case 1:  return kPhotoNebula;   // Dark Night
        case 3:  return 3;              // Sci-Fi: Purple Sky
        default: return 0;              // Modern, Day & Night, Moon, Pop: kein Foto
    }
}

void GoniometerComponent::loadPhoto (int idx)
{
    idx = juce::jlimit (0, kPhotoCount, idx);
    if (idx == photoIndex && (idx == 0 || photoSrc.isValid()))
        return;
    // Ueberblendung: das bisherige Bild bleibt als "prev" und wird in
    // paint()/composeScene() ausgeblendet (User: "alle Uebergaenge smoothen").
    photoScaledPrev = photoScaled;
    photoXfade = photoScaledPrev.isValid() || photoIndex == 0 ? 1.0f : 0.0f;
    photoIndex = idx;
    photoSrc = juce::Image();
    if (idx > 0)
    {
        struct A { const unsigned char* d; int n; };
        static const A assets[kPhotoCount] = {
            { SpaceAssets::bg_milkyway,  SpaceAssets::bg_milkywaySize  },
            { SpaceAssets::bg_nebula,    SpaceAssets::bg_nebulaSize    },
            { SpaceAssets::bg_purplesky, SpaceAssets::bg_purpleskySize },
        };
        photoSrc = juce::ImageFileFormat::loadFrom (assets[idx - 1].d, (size_t) assets[idx - 1].n);
    }
    rescalePhoto();
    repaint();
}

void GoniometerComponent::rescalePhoto()
{
    photoScaled = juce::Image();
    photoScaledPrev = juce::Image();
    photoXfade = 0.0f;
    const int w = getWidth(), h = getHeight();
    if (! photoSrc.isValid() || w <= 0 || h <= 0)
        return;
    // Cover mit 8 % Reserve rundum: das Foto wird in paint() ganz langsam
    // hin- und hergeschoben (User: "leichte Bewegung von stillen Fotos").
    const int cw = (int) std::round (w * 1.03f), ch = (int) std::round (h * 1.03f);
    const float sc = juce::jmax ((float) cw / (float) photoSrc.getWidth(), (float) ch / (float) photoSrc.getHeight());
    const int sw = juce::jmax (1, (int) std::round (photoSrc.getWidth() * sc));
    const int sh = juce::jmax (1, (int) std::round (photoSrc.getHeight() * sc));
    juce::Image big = photoSrc.rescaled (sw, sh, juce::Graphics::highResamplingQuality);
    photoScaled = big.getClippedImage (juce::Rectangle<int> ((sw - cw) / 2, (sh - ch) / 2, cw, ch)).createCopy();
    photoScaledGrey = photoScaled.createCopy();
    photoScaledGrey.desaturate();   // fuer die Stars-Ansicht (weniger Farbe, User)
}

// RGB-JPEG + Alpha-PNG -> ein ARGB-Bild.
static juce::Image combineRgbAlpha (const unsigned char* rgbData, int rgbN, const unsigned char* aData, int aN)
{
    juce::Image rgb   = juce::ImageFileFormat::loadFrom (rgbData, (size_t) rgbN);
    juce::Image alpha = juce::ImageFileFormat::loadFrom (aData,   (size_t) aN);
    if (! rgb.isValid())
        return {};
    juce::Image out (juce::Image::ARGB, rgb.getWidth(), rgb.getHeight(), true);
    juce::Image::BitmapData src (rgb, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dst (out, juce::Image::BitmapData::writeOnly);
    const bool hasAlpha = alpha.isValid() && alpha.getWidth() == rgb.getWidth() && alpha.getHeight() == rgb.getHeight();
    std::unique_ptr<juce::Image::BitmapData> al;
    if (hasAlpha)
        al = std::make_unique<juce::Image::BitmapData> (alpha, juce::Image::BitmapData::readOnly);
    for (int y = 0; y < rgb.getHeight(); ++y)
        for (int x = 0; x < rgb.getWidth(); ++x)
        {
            const juce::Colour c = src.getPixelColour (x, y);
            const float a = hasAlpha ? al->getPixelColour (x, y).getFloatRed() : 1.0f;
            dst.setPixelColour (x, y, c.withAlpha (a));
        }
    return out;
}

// ===== SPRITES (Planeten-Fotos) =====
enum SpriteId { SPR_MARS = 0, SPR_EARTH, SPR_YELLOWMOON, SPR_REDMOON, SPR_NEPTUNE, SPR_SUN, SPR_HOTJUP,
                SPR_DARKBLUE, SPR_ECLIPSE, SPR_ECLIPSEBLUE, SPR_VENUS, SPR_MERCURY, SPR_SATURN, SPR_REDGIANT, SPR_ISS };

const juce::Image& GoniometerComponent::spriteFor (int idx, int targetW)
{
    idx = juce::jlimit (0, kSpriteCount - 1, idx);
    if (! spriteSrc[idx].isValid())
    {
        struct A { const unsigned char* rgb; int rgbN; const unsigned char* a; int aN; };
        #define SPR(n) { SpaceAssets::spr_##n##_rgb, SpaceAssets::spr_##n##_rgbSize, SpaceAssets::spr_##n##_a, SpaceAssets::spr_##n##_aSize }
        static const A assets[kSpriteCount] = {
            SPR(mars), SPR(earth), SPR(yellowmoon), SPR(redmoon), SPR(neptune), SPR(sun), SPR(hotjup),
            SPR(darkblue), SPR(eclipse), SPR(eclipseblue), SPR(venus), SPR(mercury), SPR(saturn), SPR(redgiant), SPR(iss)
        };
        #undef SPR
        spriteSrc[idx] = combineRgbAlpha (assets[idx].rgb, assets[idx].rgbN, assets[idx].a, assets[idx].aN);
    }
    targetW = juce::jmax (2, targetW);
    if (spriteSrc[idx].isValid() && (! spriteScaled[idx].isValid() || std::abs (spriteScaledW[idx] - targetW) > 2))
    {
        const int h = juce::jmax (1, (int) std::round ((float) spriteSrc[idx].getHeight() * (float) targetW / (float) spriteSrc[idx].getWidth()));
        spriteScaled[idx] = spriteSrc[idx].rescaled (targetW, h, juce::Graphics::highResamplingQuality);
        spriteScaledW[idx] = targetW;
    }
    return spriteScaled[idx];
}

void GoniometerComponent::setStarfieldMode (int mode)
{
    // "Off" gibt es nicht mehr (User): das Minimum ist Stars (= Gonio-Ansicht).
    starfieldMode = juce::jlimit (1, 2, mode);
    setSpaceVisualsEnabled (true);
    loadPhoto (effectivePhoto());
}

void GoniometerComponent::setGravityMode (int mode)
{
    gravityChoice = juce::jlimit (0, kGravityModes - 1, mode);
    // Modern Earth, Watercolor Night, Pop Moon, Sci-Fi, Warm Moon, Sci-Fi Green
    // (gezeichnet, gruen), Modern Rose roter Halbmond, Sci-Fi Pink (gezeichnet, RAYE-Farbe)
    // Modern: gezeichnet (dunkel, nur angedeutet - User), Dark Night: Erde
    // bei Nacht, Pop: Mond, Sci-Fi: gezeichnet neon, Day & Night: Erde bei
    // Tag (Stars-Ansicht: bei Nacht), Moon: Mond (leicht gedehnt).
    // 0 Moon, 1 Dark Night, 2 Pop, 3 Sci-Fi, 4 Day & Night, 5 Flat.
    // Flat bekommt einen eigenen, flach gezeichneten Planeten (0) statt des
    // Mondfotos (User: "Flat Gravity Planet erzeugen").
    static const int byTheme[kThemeCount] = { 0, 3, 1, 0, 2, 0 };
    gravityMode   = byTheme[juce::jlimit (0, kThemeCount - 1, uiTheme)];
}

const juce::Image& GoniometerComponent::gravityPhotoFor (int mode)
{
    const int k = juce::jlimit (1, 4, mode) - 1;
    if (! gravityPhoto[k].isValid())
    {
        struct A { const unsigned char* rgb; int rgbN; const unsigned char* a; int aN; };
        static const A assets[4] = {
            { SpaceAssets::grav_moon_rgb,       SpaceAssets::grav_moon_rgbSize,       SpaceAssets::grav_moon_a,       SpaceAssets::grav_moon_aSize       },
            { SpaceAssets::grav_earth_rgb,      SpaceAssets::grav_earth_rgbSize,      SpaceAssets::grav_earth_a,      SpaceAssets::grav_earth_aSize      },
            { SpaceAssets::grav_earthnight_rgb, SpaceAssets::grav_earthnight_rgbSize, SpaceAssets::grav_earthnight_a, SpaceAssets::grav_earthnight_aSize },
            { SpaceAssets::grav_redhalf_rgb,    SpaceAssets::grav_redhalf_rgbSize,    SpaceAssets::grav_redhalf_a,    SpaceAssets::grav_redhalf_aSize    },
        };
        gravityPhoto[k] = combineRgbAlpha (assets[k].rgb, assets[k].rgbN, assets[k].a, assets[k].aN);
    }
    return gravityPhoto[k];
}

GoniometerComponent::GoniometerComponent (LCRMSAudioProcessor& processorToRead)
    : processor (processorToRead), ring (processorToRead.gonioBuffer)
{
    lastReadIndex = ring.writeIndex.load (std::memory_order_acquire);
    generateBackground();

    if (! SPACEX_NO_VISUALS) startTimerHz (30);
}

void GoniometerComponent::respawnStar (FlyingStar& s, bool randomiseRadius)
{
    s.angle  = bgRandom.nextFloat() * juce::MathConstants<float>::twoPi;
    s.radius = randomiseRadius ? bgRandom.nextFloat() * 1.0f : 0.0f;
    s.speed  = 0.16f + bgRandom.nextFloat() * 0.22f;
    s.alpha  = 0.14f + bgRandom.nextFloat() * 0.20f;
}

void GoniometerComponent::generateBackground()
{
    // Fester Seed -> immer dasselbe, "beabsichtigt" wirkende Sternenfeld
    // statt bei jedem Plugin-Start zufaellig anders auszusehen.
    bgRandom = juce::Random (90210);

    bgStars.clear();
    // 54 sind der Normalwert; der Pool ist 1,5x so gross, damit das
    // View-Panel (Density bis 1,5) zusaetzliche Sterne aktivieren kann,
    // ohne dass bei Density 1,0 etwas anders aussieht als vorher.
    bgStars.resize (81);
    // Beim ersten Aufbau ueber die ganze Flaeche verteilen (nicht alle bei
    // 0 starten), damit sofort ein volles Feld zu sehen ist statt eines
    // "Urknalls" beim Plugin-Oeffnen.
    for (auto& s : bgStars)
        respawnStar (s, true);

    // Orbit-Sterne (User-Wunsch: "blaue kleine Sterne werden hinzugefuegt"):
    // fester Pool, davon wird pro Frame je nach Orbit-Reglerwert nur ein
    // Teil tatsaechlich gezeichnet (siehe timerCallback()/activeOrbitStars) -
    // spart, dass bei jeder Reglerbewegung neu allokiert werden muesste.
    orbitStars.clear();
    // Von 24 auf 48 verdoppelt (User-Wunsch: Orbit soll zusaetzlich zur
    // Winkel-Umverteilung auch spuerbar mehr Linien erzeugen). Es werden
    // weiterhin nur so viele davon gezeichnet, wie der Reglerwert vorgibt -
    // ein groesserer Pool kostet also nur dann CPU, wenn Orbit auch
    // aufgedreht ist.
    orbitStars.resize (48);
    for (auto& s : orbitStars)
        respawnStar (s, true);

    // Funkelnde Sterne: fest verteilt, unterschiedliche Groesse/Periode.
    // Der Bereich um die Mondposition (X~0, Y~0,5 oben) bleibt etwas
    // duenner besetzt, damit sie nicht staendig hinter dem Mond blinken.
    twinkles.clear();
    twinkles.resize (84);   // Pool > Standard (46): Density kann bis +30 % dazugeben (User)
    for (auto& t : twinkles)
    {
        t.x = (bgRandom.nextFloat() - 0.5f) * 1.9f;
        t.y = (bgRandom.nextFloat() - 0.5f) * 1.9f;
        t.size   = 0.6f + bgRandom.nextFloat() * 1.1f;
        t.phase  = bgRandom.nextFloat() * juce::MathConstants<float>::twoPi;
        t.period = 1.8f + bgRandom.nextFloat() * 4.5f;
    }
}

GoniometerComponent::~GoniometerComponent()
{
    stopTimer();
}

// Ein einfacher Klick (ohne Ziehen) ins Feld - was passiert, entscheidet
// der Editor (onFieldClick). Das Zahnrad liegt als eigene Komponente
// darueber und faengt seine Klicks selbst.
void GoniometerComponent::mouseDown (const juce::MouseEvent& e)
{
    // Modifier und Maustaste beim Druecken merken: in mouseUp sind die
    // Tasten-Flags bereits geloescht (JUCE), Rechtsklick waere dort unsichtbar.
    pressMods = e.mods;
}

void GoniometerComponent::mouseUp (const juce::MouseEvent& e)
{
    if (! e.mouseWasClicked() || ! onFieldClick)
        return;
    const juce::ModifierKeys m = pressMods;
    // Klick-Zonen (User): Mitte = Goniometer an/aus (Cmd = Farbe, Shift = Look),
    // Sonne = Shine min/max (Cmd = Gonio-Ansicht), Mond = zurueck zu Space,
    // Rest = naechstes Foto (Rechtsklick: vorheriges).
    const float cx = (float) getWidth() * 0.5f, cy = (float) getHeight() * 0.5f;
    const float px = (float) e.x, py = (float) e.y;
    const float centreR = (float) juce::jmin (getWidth(), getHeight()) * 0.20f;
    if (sunHitR > 0.0f && std::hypot (px - sunHitX, py - sunHitY) <= sunHitR)
        onFieldClick (m.isCommandDown() && starfieldMode != 1 ? 6 : 3);   // Sonne/Mond = Ansicht wechseln, Cmd+Sonne = Shine min/max (User)
    else if (starfieldMode == 1)
        onFieldClick (3);                                                  // Gonio-Ansicht: Klick irgendwo = zurueck zu Space
    else if (std::hypot (px - cx, py - cy) <= centreR && (m.isCommandDown() || m.isShiftDown()))
        onFieldClick (m.isCommandDown() ? 4 : 5);                          // Cmd = Farbe, Shift = Look
    else
        onFieldClick (2);                                                  // Klick egal wohin = Goniometer an/aus (User)
}

void GoniometerComponent::setGoniometerActive (bool active)
{
    // Korrektur (User-Feedback: "soll lediglich das gruene technische
    // Goniometer unsichtbar machen, Starfield soll weiter laufen") - blendet
    // NUR die Scope-Spur aus, der 30Hz-Timer laeuft weiter, solange
    // Starfield noch etwas zu zeichnen hat (siehe updateTimerRunning()).
    scopeTraceVisible = active;
    // Bug-Fix (User: "Starfield off + Gonio off friert ein"): steht der
    // Timer, bliebe die letzte Spur im Puffer stehen - beim Ausblenden
    // sofort leeren.
    if (! active && ! traceImage.isNull())
        traceImage.clear (traceImage.getBounds());
    updateTimerRunning();
    // Bug-Fix (frueheres User-Feedback: "Gonio soll natuerlich nicht das
    // ganze Fenster verschwinden lassen sondern wirklich nur das
    // Goniometer") - die Komponente selbst bleibt immer sichtbar (Rahmen/
    // Kreuz/Starfield laufen weiter), nur die Scope-Punkte werden
    // ausgeblendet.
    repaint();
}

void GoniometerComponent::setSpaceVisualsEnabled (bool enabled)
{
    spaceVisualsEnabled = enabled;
    // Bug-Fix (User: "Show aus -> stoppt nur"): das letzte Bild blieb im
    // Nachleucht-Puffer stehen, wenn der Timer gleichzeitig anhielt. Beim
    // Ausblenden den Puffer sofort schwarz machen.
    if (! enabled)
    {
        for (auto* im : { &fadeImage, &twinkleImage, &objectImage, &glowImage, &shootImage, &gravityImage })
            if (! im->isNull())
                im->clear (im->getBounds());
        repaint();
    }
    updateTimerRunning();
}

void GoniometerComponent::updateTimerRunning()
{
    // Der gemeinsame 30Hz-Timer treibt sowohl die Scope-Spur als auch
    // Starfield an - er wird nur dann komplett gestoppt (CPU sparen), wenn
    // BEIDE nichts anzuzeigen haben. Ist mindestens eines der beiden aktiv,
    // muss der Timer weiterlaufen (User-Wunsch: Starfield laeuft auch bei
    // deaktiviertem technischen Goniometer weiter).
   #if SPACEX_NO_VISUALS
    // SpaceXnoV (CPU-Vergleich): Animation komplett aus.
    const bool shouldRun = false;
   #else
    const bool shouldRun = scopeTraceVisible || spaceVisualsEnabled;
   #endif
    if (shouldRun == isTimerRunning())
        return;
    if (shouldRun)
        startTimerHz (30);
    else
        stopTimer();
}

void GoniometerComponent::setGravityKnobDiameter (float diameterPx)
{
    // Radius, nicht Durchmesser - siehe timerCallback()/fillEllipse.
    moonRadiusPx = juce::jmax (2.0f, diameterPx * 0.5f);
}

void GoniometerComponent::resized()
{
    auto bounds = getLocalBounds();
    if (bounds.getWidth() > 0 && bounds.getHeight() > 0)
    {
        for (auto* im : { &fadeImage, &twinkleImage, &objectImage, &glowImage, &shootImage, &gravityImage, &traceImage })
            *im = juce::Image (juce::Image::ARGB, bounds.getWidth(), bounds.getHeight(), true);
        compositeImage = juce::Image (juce::Image::RGB, bounds.getWidth(), bounds.getHeight(), true);
    }
    rescalePhoto();
}

void GoniometerComponent::timerCallback()
{
    if (fadeImage.isNull())
        return;

    // Logo-Klick-Bypass: komplett einfrieren (keine Sterne, kein Fade, kein
    // neuer Scope-Trace) statt weiterzulaufen, waehrend das Signal gar
    // nicht mehr bearbeitet wird.
    if (processor.uiBypassed.load (std::memory_order_relaxed))
        return;

    // ===== EBENEN vorbereiten (siehe Header) =====
    // Nachleuchten = Alpha der transparenten Ebene abschwaechen; Ebenen ohne
    // Nachleuchten werden pro Frame geleert.
    fadeImage.multiplyAllAlphas (0.82f);
    shootImage.multiplyAllAlphas (0.82f);
    twinkleImage.clear (twinkleImage.getBounds());
    objectImage.clear (objectImage.getBounds());
    glowImage.clear (glowImage.getBounds());
    gravityImage.clear (gravityImage.getBounds());
    juce::Graphics gLines (fadeImage), gTwinkle (twinkleImage), gObject (objectImage),
                   gGlow (glowImage), gShoot (shootImage), gGravity (gravityImage);
    // Alle Zeichenabschnitte unten sprechen ueber "g" - das ist ein Zeiger auf
    // die gerade aktive Ebene, der an den Abschnittsgrenzen umgesetzt wird.
    juce::Graphics* gCur = &gLines;
#define g (*gCur)

    const int w = fadeImage.getWidth();
    const int h = fadeImage.getHeight();
    const float cx = (float) w * 0.5f;
    const float cy = (float) h * 0.5f;
    const float maxRadius = (float) juce::jmin (w, h) * 0.47f;

    // "Hyperspace"-Sternenflug: jeder Stern bewegt sich radial vom Zentrum
    // nach aussen und wird dabei schneller (Warp-Beschleunigung), mit einem
    // Streak-Schweif in Bewegungsrichtung. Erreicht ein Stern den Rand,
    // spawnt er nahe der Mitte neu - erzeugt einen endlosen, "teuren"
    // Warp-Effekt statt statischer Planeten-Farbflecken.
    const float bgRadius = (float) juce::jmin (w, h) * 0.5f;

    // Audio-reaktive Sterne: die horizontale Achse ist im Goniometer exakt
    // die Seiten-/Side-Achse (siehe die 45-Grad-Rotation weiter unten: x =
    // (R-L)), die vertikale Achse ist Mid. currentSideEmphasis ist das
    // tatsaechliche, aus dem finalen Audiosignal berechnete Verhaeltnis
    // Side- zu Gesamtenergie (0 = reines Center/Mono, 1 = stark
    // seitenbetont) - live gekoppelt (User-Feedback: "Sterne mit Audio
    // koppeln"), NICHT nur an die Reglerstellung. Da Orbit (LCR-Blend) und
    // Dimension (Expand+Boost) beide direkt in dieses Verhaeltnis
    // einfliessen, ist damit automatisch genau der geforderte "Einfluss
    // durch Orbit oder Dimension (beide Regler)" gegeben, ohne die beiden
    // Regler hier separat abfragen zu muessen.
    float sideEmphasis = processor.currentSideEmphasis.load (std::memory_order_relaxed);

    // ===== SZENEN-ZEIT MIT TRANSPORT-FADE =====
    // Gemeinsame Szenen-Zeit in Sekunden fuer alle langsamen Eigenbewegungen
    // (siehe Eigendrift bei Mond und Planeten). Bewusst EIN gemeinsamer
    // Zeitwert, damit sich die Bewegungen der Objekte zueinander nie
    // "verschlucken" koennen.
    //
    // Neu: das ist keine absolute Uhrzeit mehr, sondern eine eigene Uhr, die
    // pro Frame nur um dt * motionScale weiterlaeuft (User-Wunsch: "alles
    // langsam anhalten und langsam starten wenn DAW Stop/start").
    // motionScale faehrt bei Stop weich auf 0 herunter und bei Play weich
    // wieder hoch - die Szene laeuft also aus wie ein Schwungrad und faehrt
    // genau dort wieder an, statt irgendwo anders wieder aufzutauchen.
    // Ausfuehrliche Begruendung siehe Header.
    {
        const juce::uint32 nowMs = juce::Time::getMillisecondCounter();
        float dt = (lastTickMs == 0) ? (1.0f / 30.0f) : (float) (nowMs - lastTickMs) * 0.001f;
        lastTickMs = nowMs;
        // Nach einem Fensterwechsel oder einer Lastspitze kann dt sehr gross
        // werden - dann wuerde die Szene einen Sprung machen. Deckeln.
        dt = juce::jlimit (0.0f, 0.10f, dt);
        lastFrameDt = dt;   // echte Frame-Dauer, fuer die Regler-Glaettung der Himmelskoerper
        // View-Panel: globale Geschwindigkeit fuer alles Nicht-Reglergesteuerte -
        // fuer Planeten, Kometen, Easter Eggs usw. aber GEDECKELT (User: "nie
        // absurd schnell") und nach unten begrenzt (User: "andere Objekte
        // muessen sich trotzdem noch ein bisschen bewegen"). Nur die
        // Sternlinien folgen dem Regler ungedeckelt (siehe lineSpeedMult).
        dt *= sceneSpeedMul();
        // Funkelnde Sterne: eigene, langsamere Uhr - Standard = 40 % des
        // frueheren Tempos, ganz rechts 110 % (User), laeuft auch bei Stop.
        // Funkeln nie langsamer als die -50-%-Stellung (User: "bei -100 % weiter blinken").
        // Gonio-Ansicht: festes Funkel-Tempo (User: "Speed = 70").
        twinkleClock += lastFrameDt * (starfieldMode == 1 ? 0.70f
                                        : (viewSpeed <= 1.0f ? 0.4f * juce::jmax (0.5f, viewSpeed) : 0.4f + (viewSpeed - 1.0f) * 0.7f));
        photoClock   += lastFrameDt * motionScale;   // Foto-Wanderung stoppt mit der DAW
        // Weiche Uebergaenge (User): Space-Ebenen blenden ein/aus, Shine und
        // Spurgroesse gleiten, Foto ueberblendet.
        auto glide = [this] (float& v, float target, float tau) { v += (target - v) * juce::jlimit (0.0f, 1.0f, lastFrameDt / tau); };
        glide (spaceBlend, starfieldMode >= 2 ? 1.0f : 0.0f, 0.45f);
        glide (shineSm,    effShine(),     0.12f);   // schnell, sonst merkt man die Aenderung nicht (User)
        glide (shineSpaceSm, viewShine,    0.12f);   // folgt immer dem Regler (siehe Deklaration)
        glide (sizeSm,     traceSizeMul(), 0.45f);
        glide (vigSm,      traceOn() ? 1.0f : 0.0f, 0.40f);   // Vignette weich (User: "nicht so ploetzlich")
        if (photoXfade > 0.0f)
        {
            photoXfade -= lastFrameDt / 0.6f;
            if (photoXfade <= 0.0f) { photoXfade = 0.0f; photoScaledPrev = juce::Image(); }
        }

        // Zwei Bedingungen muessen erfuellt sein, damit sich etwas bewegt:
        // der Host muss laufen UND es muessen ueberhaupt Bloecke ankommen.
        // Zweiteres faengt den Fall ab, dass das Plugin-Fenster offen ist,
        // waehrend die Spur gar nicht mehr verarbeitet wird.
        const juce::uint32 lastBlock = processor.lastProcessBlockMs.load (std::memory_order_relaxed);
        const bool blocksArriving = (nowMs - lastBlock) < 500;
        const bool playing = processor.currentTransportPlaying.load (std::memory_order_relaxed) && blocksArriving;

        // Anfahren bewusst schneller als Auslaufen: beim Druecken von Play
        // soll das Bild zuegig "da" sein, beim Stop darf es genuesslich
        // austrudeln. Zeitkonstanten in Sekunden.
        const float target = playing ? 1.0f : 0.0f;
        const float tau    = (target > motionScale) ? 0.55f : 1.40f;
        motionScale += (target - motionScale) * juce::jlimit (0.0f, 1.0f, dt / tau);
        motionScale = juce::jlimit (0.0f, 1.0f, motionScale);

        sceneClock += dt * motionScale;
    }
    const float sceneT = sceneClock;

    // Preset-Menue (User-Idee "Deactivate Space Visuals"): ueberspringt NUR
    // Sterne + reaktive Glows, das eigentliche Vektorskop weiter unten
    // (Nachleucht-Fade + Scope-Punkte) bleibt davon unberuehrt.
    if (spaceVisualsEnabled)
    {
    // View-Panel "Dim": das GANZE Sternenfeld (Sterne, Objekte, Komet,
    // Sternschnuppen) wird ueber eine Transparenz-Ebene abgeschwaecht, die
    // Scope-Spur darunter/danach NICHT (User: "wenn starfield dim darf gonio
    // nicht auch gedimmt werden"). Die Ebene kostet einen Zwischenpuffer,
    // deshalb nur, wenn Dim wirklich aktiv ist.

    // ---- Live-Werte fuer den grossen Starfield/Visuals-Umbau -----------
    // WICHTIG: hier werden die "LivePercent/LiveCt/LiveDb"-Atomics gelesen
    // (nicht mehr die rohen APVTS-Werte) - die enthalten bereits die
    // laufende Modulation (Galaxy-/Timewarp-/Dimension-/Position-Mod), so
    // dass Modulation jetzt auch die Starfield-Bewegungen beeinflusst
    // (User-Wunsch: "Modulation soll die Bewegungen der Objekte
    // beeinflussen - einfach den aktuellen Wert nehmen").
    // ===== Sektions-Gating inkl. SOLO =====
    // Bug (User-Feedback: "Visuals nur wenn Section on ist... ich denke wegen
    // Solo. Weil Solo laesst noch die komplette Animation von den anderen
    // Sections laufen."): das Gating hat bisher NUR den eigenen On/Off-
    // Parameter jeder Sektion abgefragt. Solo schaltet im DSP aber saemtliche
    // ANDEREN Sektionen stumm, ohne deren On/Off-Parameter anzufassen - die
    // Visuals liefen deshalb fuer Sektionen weiter, die gar nicht mehr zu
    // hoeren waren.
    //
    // Die Formel ist exakt dieselbe, die processBlock() fuer die Gains
    // benutzt (siehe driftOnGain/polOnGain/... dort): eine Sektion wirkt,
    // wenn ihr eigener Schalter an ist UND entweder gar kein Solo laeuft
    // oder sie selbst die solo-geschaltete ist. Damit koennen Bild und Ton
    // nicht mehr auseinanderlaufen.
    const int  soloSection = juce::jlimit (0, LCRMSAudioProcessor::SOLO_MAX,   // Bug: 6 schnitt RAYE (7) ab -> keine goldenen Linien bei RAYE-Solo
                                            (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_SOLO_SECTION)->load()));
    const bool soloActive  = soloSection != LCRMSAudioProcessor::SOLO_NONE;

    auto sectionActive = [&] (const char* onParamId, int soloId)
    {
        return processor.apvts.getRawParameterValue (onParamId)->load() > 0.5f
               && (! soloActive || soloSection == soloId);
    };

    const bool  polarityOn    = sectionActive (LCRMSAudioProcessor::ID_POL_ON,        LCRMSAudioProcessor::SOLO_POLARITY);
    const bool  flowOn        = sectionActive (LCRMSAudioProcessor::ID_FLOW_ON,       LCRMSAudioProcessor::SOLO_HYPERDRIVE);
    const bool  galaxyOn      = sectionActive (LCRMSAudioProcessor::ID_LCR_ENABLED,   LCRMSAudioProcessor::SOLO_GALAXY);

    // ===== POLARITY: Sternlinien laufen auf der geflippten Seite rueckwaerts =====
    // Die Flip-Sektion hatte seit dem Planeten-Umbau keine visuelle
    // Entsprechung mehr. Von allen besprochenen Ideen ist das hier die
    // direkteste: normal fliegen die Sterne vom Fluchtpunkt nach aussen -
    // ist L geflippt, laufen sie auf der LINKEN Haelfte nach innen, bei R
    // entsprechend rechts. Phasenumkehr in ihrer reinsten Form: dieselbe
    // Bewegung, nur mit umgekehrtem Vorzeichen. Man sieht es sofort, auch
    // ohne zu wissen, was Polarity bedeutet - und es kostet praktisch nichts.
    const bool polLFlipped = polarityOn && processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_POL_L)->load() > 0.5f;
    const bool polRFlipped = polarityOn && processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_POL_R)->load() > 0.5f;
    const bool  timewarpOn    = sectionActive (LCRMSAudioProcessor::ID_DRIFT_ON,      LCRMSAudioProcessor::SOLO_TIMEWARP);
    const bool  dimensionOn   = sectionActive (LCRMSAudioProcessor::ID_WIDTHBOOST_ON, LCRMSAudioProcessor::SOLO_DIMENSION);
    const bool  positionOn    = sectionActive (LCRMSAudioProcessor::ID_POS_ON,        LCRMSAudioProcessor::SOLO_POSITION);
    // RAY (Phaser): goldene Linien, die mit dem Phaser-LFO mitschwingen.
    const bool  rayOn         = sectionActive (LCRMSAudioProcessor::ID_RAY_ON,        LCRMSAudioProcessor::SOLO_RAY);
    const int   rayLevel      = juce::jlimit (0, 3, (int) std::round (processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_RAY_STRENGTH)->load()));
    // Das RAYE-Pendeln ist Modulation - ohne Mod Movement steht es still (User).
    const float rayLfo        = (rayOn && rayLevel > 0 && modMovementEnabled) ? processor.currentRayLfo.load (std::memory_order_relaxed) : 0.0f;
    // Winkel-Zusatz fuer die gerade gezeichnete Linie (wird pro Stern vor
    // dem Zeichnen gesetzt, siehe Schleife unten) - 0 fuer normale Linien.
    float rayAngleOffset = 0.0f;

    // ===== Bug-Fix: "Visuals gehen erst an wenn DAW laeuft/Audio eingeht" =====
    // Alle current...Live...-Atomics werden AUSSCHLIESSLICH in
    // processBlock() geschrieben. Laeuft gerade kein Audio - Transport
    // gestoppt, Spur inaktiv, oder das Plugin-Fenster wurde gerade erst
    // geoeffnet und es lief noch nie etwas - stehen sie auf ihrem letzten
    // bzw. dem Initialwert. Reglerbewegungen hatten dadurch sichtbar KEINE
    // Wirkung auf das Starfield, obwohl dieser 30Hz-Timer ganz normal
    // weiterlief.
    //
    // Loesung: der Processor hinterlegt bei jedem Block einen Zeitstempel
    // (lastProcessBlockMs). Ist der aelter als 250ms, laeuft offensichtlich
    // kein Audio - dann werden statt der eingefrorenen Live-Werte die rohen
    // APVTS-Reglerwerte gelesen.
    //
    // Das ist nicht nur ein Notbehelf, sondern inhaltlich exakt richtig: die
    // Live-Werte unterscheiden sich vom rohen Reglerwert NUR durch die
    // laufende Modulation, und ohne processBlock() laeuft auch keine
    // Modulation. Die Einheiten sind dabei identisch - der Processor rechnet
    // intern "raw * 0.01" und legt "target * 100" ab, unterm Strich also
    // derselbe Wertebereich wie der rohe Parameter.
    const juce::uint32 msNow = juce::Time::getMillisecondCounter();
    const bool audioRunning = (msNow - processor.lastProcessBlockMs.load (std::memory_order_relaxed)) < 250;

    auto liveOrRaw = [&] (const std::atomic<float>& live, const char* paramId) -> float
    {
        // Menue "Mod Movement in Starfield" aus: immer den rohen Reglerwert
        // nehmen. Der Unterschied zwischen Live- und Rohwert IST genau die
        // Modulation - den Rohwert zu lesen blendet sie also exakt aus, ohne
        // dass irgendwo sonst etwas abgefragt werden muesste.
        if (audioRunning && modMovementEnabled)
            return live.load (std::memory_order_relaxed);
        return processor.apvts.getRawParameterValue (paramId)->load();
    };

    const float gravityRaw  = liveOrRaw (processor.currentGravityLivePercent,  LCRMSAudioProcessor::ID_LCR_SENS);    // 0..100
    const float orbitRaw    = liveOrRaw (processor.currentOrbitLivePercent,    LCRMSAudioProcessor::ID_LCR_BLEND);   // 0..100
    // Runde 50 (User): AIR/Regain bekommt eine eigene Rolle im Sternenfeld
    // (die blauen Linien), Orbit uebernimmt den Planeten, Gravity die
    // Farbintensitaet. Air hat keinen Live-Wert (keine eigene Modulation),
    // deshalb direkt der Reglerstand.
    const float airRaw      = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_LCR_HORIZON)->load();  // 0..100
    // ID_POL_L/ID_POL_R/ID_POL_POS werden im Starfield nicht mehr gebraucht
    // (User-Wunsch: "Polarity Planeten Einfluss wieder loeschen") - die 3
    // neuen Planeten weiter unten sind komplett unabhaengig von Flip/Polarity.
    const float sizeRaw     = liveOrRaw (processor.currentExpandLivePercent,   LCRMSAudioProcessor::ID_SIDE_WIDTH);  // 50..200
    const float dimBoostRaw = liveOrRaw (processor.currentBoostLiveDb,         LCRMSAudioProcessor::ID_SIDE_BOOST);  // 0..6 dB
    const float posWidthRaw = liveOrRaw (processor.currentPosWidthLivePercent, LCRMSAudioProcessor::ID_POS_WIDTH);   // 50..150
    const float shiftRaw    = liveOrRaw (processor.currentBendLiveCt,          LCRMSAudioProcessor::ID_BEND);        // 0..10 ct ("Shift")

    // Alle Sektions-Gates: ist eine Sektion aus, hat ihr Regler-Effekt KEINE
    // sichtbare Auswirkung mehr (User-Wunsch: "Alle grafischen Effekte
    // sollen nur dann eine sichtbare Auswirkung haben, wenn die Sections
    // auch on sind. Wenn off -> keine sichtbaren Effekte.") - der Mond
    // selbst bleibt als einziges, sektionsunabhaengiges Starfield-Element
    // immer sichtbar (wie urspruenglich spezifiziert), nur seine einzelnen
    // Bewegungs-/Groessen-Beitraege werden pro Sektion abgeschaltet.
    const float gravityNorm        = galaxyOn ? juce::jlimit (0.0f, 1.0f, gravityRaw / 100.0f) : 0.0f;
    const float orbitNorm          = galaxyOn ? juce::jlimit (0.0f, 1.0f, orbitRaw / 100.0f) : 0.0f;
    const float airNorm            = galaxyOn ? juce::jlimit (0.0f, 1.0f, airRaw / 100.0f) : 0.0f;
    // Runde 60 (User-Korrektur): Gravity macht wieder NUR den Planeten. Die
    // Farbintensitaet haengt an nichts mehr - ein Regler, eine Wirkung.
    const float colourIntensity    = 1.0f;
    // Size wirkt jetzt staerker auf die Liniendicke (User-Wunsch: "Size soll
    // Linien noch ein bisschen dicker machen"). Nicht der Rohwert wird
    // skaliert, sondern seine ABWEICHUNG von 1.0 - dadurch bleibt die
    // Normaldicke bei Size = 100% exakt unveraendert und nur die Auslenkung
    // nach oben und unten wird groesser (Faktor 1,5). Bereich damit rund
    // 0,25 bis 2,5 statt vorher 0,5 bis 2,0.
    const float sizeThickRaw       = 1.0f + ((sizeRaw / 100.0f) - 1.0f) * 1.5f;
    // Obergrenze 2,8 -> 2,2 (User: "Size max Liniendicke etwas reduzieren").
    const float lineThicknessMult  = dimensionOn ? juce::jlimit (0.35f, 2.2f, sizeThickRaw) : 1.0f;      // "Size" (Dimension)
    const float dimBoostNorm       = dimensionOn ? juce::jlimit (0.0f, 1.0f, dimBoostRaw / 6.0f) : 0.0f;    // "Boost" (Dimension)
    // Gravity faerbt ALLE Sternlinien, nicht nur die blauen (User Runde 50).
    const float lineBrightnessMult = (1.0f + dimBoostNorm * 1.5f) * colourIntensity;
    // Sternlinien laufen auch bei DAW-Stop weiter (User) - kein motionScale
    // mehr, und der Speed-Regler wirkt hier ungedeckelt.
    const float lineSpeedMult      = (1.0f + dimBoostNorm * 1.0f) * viewSpeed;
    // ===== SPEED -> SICHTBARKEIT DER LINIEN =====
    // Sternlinien entstehen nur durch Geschwindigkeit. Je langsamer, desto
    // kuerzer, duenner und blasser werden sie, bis sie unter ~5 % ganz
    // verschwunden sind (User: "das ist vielleicht der effektivste Trick").
    // DAW-Stop: Linien blenden mit motionScale aus (User: "keine Bewegungen mehr").
    const float lineVis            = juce::jlimit (0.0f, 1.0f, (viewSpeed - 0.05f) / 0.30f) * motionScale;
    const float shiftNorm          = timewarpOn ? juce::jlimit (0.0f, 1.0f, shiftRaw / 10.0f) : 0.0f;
    // Amplitude halbiert (User-Wunsch: "Shift -> Staerke der Wellen nur
    // maximal halb so stark", vorher 0.12f).
    const float waveAmp            = shiftNorm * bgRadius * 0.03f;   // max halbiert (User)
    // Die Welle pendelt nur, wenn Mod Movement an ist, und im Tempo des
    // Speed-Reglers (User: "Shift bewegt Linien auch bei Mod Movement aus").
    wavePhase += 0.10f * sceneSpeedMul() * (modMovementEnabled ? 1.0f : 0.0f);

    // ===== WANDERNDER FLUCHTPUNKT + STAUCHUNG/DEHNUNG DES STERNENFELDS =====
    // (User-Idee: "Kann man das Starfield so machen, dass der Mittelpunkt wo
    // alles entsteht sich bewegt? Wuerde das den Eindruck wecken, dass man im
    // Raumschiff sitzt und die Richtung aendert? Und/oder: Stauchen, dehnen")
    //
    // 1) FLUCHTPUNKT: Der Punkt, aus dem die Sterne entspringen, ist im
    //    echten Flug die FLUGRICHTUNG ("focus of expansion"). Verschiebt man
    //    ihn nach links, liest das Gehirn das unmittelbar als "wir drehen
    //    nach links". Tilt und Drift schieben ihn daher seitlich.
    //
    //    WICHTIG - das Vorzeichen ist bewusst UMGEKEHRT zum Mond: der Mond
    //    ist ein Objekt IN der Welt und wandert bei einer Drehung nach rechts
    //    scheinbar nach links (daher dort die Invertierung). Der Fluchtpunkt
    //    ist dagegen kein Weltobjekt, sondern die Blick-/Flugrichtung selbst -
    //    und die wandert MIT der Drehung. Objekte und Fluchtpunkt laufen also
    //    gegenlaeufig, genau wie in echt.
    //
    //    Die Staerke ist absichtlich klein (je max +-0,10, zusammen +-0,20 -
    //    der Mond bewegt sich mit +-0,25 je Regler deutlich mehr): Sterne
    //    sind die FERNSTEN Objekte im Bild und muessen sich nach derselben
    //    Tiefenlogik wie Mond/Planeten (siehe Parallaxe-Kommentar weiter
    //    unten) am wenigsten bewegen. Ein stark springender Fluchtpunkt wirkt
    //    ausserdem hektisch statt raeumlich.
    //
    // 2) STAUCHUNG/DEHNUNG: Width skaliert den horizontalen Abstand jeder
    //    Sternlinie vom Fluchtpunkt. Ueber der Mitte wird das Feld breiter
    //    gezogen, darunter zusammengedrueckt - die direkteste denkbare
    //    Uebersetzung von "Stereobreite" ins Bild. Betrifft bewusst NUR die
    //    Sternlinien: Mond und Planeten bleiben runde Kugeln und wuerden bei
    //    einer Stauchung zu Ellipsen verzerrt.
    const float starTiltRaw  = liveOrRaw (processor.currentOffsetLivePercent, LCRMSAudioProcessor::ID_POS_OFFSET); // -27.5..27.5
    const float starDriftRaw = liveOrRaw (processor.currentDriftLivePercent,  LCRMSAudioProcessor::ID_DRIFT);      // -100..100

    const float starTiltNorm  = positionOn ? juce::jlimit (-1.0f, 1.0f, starTiltRaw / 27.5f)   : 0.0f;
    const float starDriftNorm = timewarpOn ? juce::jlimit (-1.0f, 1.0f, starDriftRaw / 100.0f) : 0.0f;
    // ===== DEHNUNG: jetzt an SIZE statt an WIDTH =====
    // User-Feedback: "Den Effekt von Width finde ich so geil - aber weil ich
    // Width in der Praxis eher seltener nutze, wuerde ich diesen Effekt
    // lieber bei SIZE haben wollen."
    // Umgesetzt als AUFTEILUNG statt als Umzug, damit beide Regler visuell
    // unterscheidbar bleiben - haetten beide dieselbe Geste, koennte man sie
    // im Bild nicht mehr auseinanderhalten:
    //   - SIZE (Dimension) dehnt/staucht das STERNENFELD und macht
    //     zusaetzlich weiterhin die Linien dicker. Size formt den
    //     Stereo-Inhalt selbst, also darf es das "Material" verformen.
    //   - WIDTH (Position) behaelt die radiale Verschiebung von Mond und
    //     Planeten (siehe dort) und verliert die Feld-Dehnung. Width
    //     verschiebt Dinge im Raum, also ruecken die OBJEKTE auseinander.
    // Ergebnis: Size heisst "das Material wird breiter", Width heisst "die
    // Dinge ruecken auseinander".
    //
    // Der Size-Regler laeuft 50..200% um die Mitte 100% - die Spanne ist also
    // von Haus aus unsymmetrisch (halb so schmal, doppelt so breit). Die
    // Normalisierung bildet das bewusst 1:1 ab, statt kuenstlich zu
    // symmetrieren: -0,5 bei 50%, 0 bei 100%, +1,0 bei 200%.
    const float sizeStretchNorm = dimensionOn ? juce::jlimit (-1.0f, 1.0f, (sizeRaw - 100.0f) / 100.0f) : 0.0f;

    // Hinweis zur Richtung: der Fluchtpunkt lief schon immer MIT dem Regler
    // (nicht invertiert). Seit auch die Objekte dem Klang folgen (siehe
    // Mond-Block), verschiebt sich die ganze Szene einheitlich in dieselbe
    // Richtung - nahe Objekte staerker als ferne. Die frueher bewusste
    // Gegenlaeufigkeit von Objekten und Fluchtpunkt entfaellt damit; die
    // Raeumlichkeit kommt weiterhin aus den unterschiedlichen
    // Bewegungsstaerken (Parallaxe), nicht aus der Gegenrichtung.
    const float starOriginX = cx + (starTiltNorm * 0.10f + starDriftNorm * 0.10f) * bgRadius;
    const float starOriginY = cy;
    const float starStretchX = 1.0f + sizeStretchNorm * 0.50f; // 0,75 (schmal) .. 1,50 (breit)

    // Galaxy an -> Sternlinien blau statt weiss (gilt fuer alle Sternlinien,
    // die zusaetzlichen Orbit-Sterne sind ohnehin fest blau).
    const juce::Colour lineColour = tintedLineColour (galaxyOn ? juce::Colour (0xff4fa8ff) : juce::Colours::white);

    // Der frueher hier gezeichnete gelbe Rand-Glow fuer Width ist ENTFERNT
    // (User-Feedback: "Width Regler, die gelben Farben von den Seiten - macht
    // eher den Eindruck als wuerde es schmaler werden, nicht breiter").
    // Das Feedback trifft zu: ein Leuchten an den Seitenraendern wirkt wie
    // eine Einfassung und laesst das Bild optisch ENGER wirken - also genau
    // das Gegenteil dessen, was der Regler tut. Width wird stattdessen von
    // der horizontalen Stauchung/Dehnung des Sternenfelds getragen (siehe
    // starStretchX weiter unten) sowie von der radialen Verschiebung von Mond
    // und Planeten - drei sich widersprechende Darstellungen fuer denselben
    // Regler waren ohnehin zu viel.
    // edgeGlowNorm wird dadurch nicht mehr gebraucht.

    // ===== Der zentrale Gravity-Planet ist ENTFERNT =====
    // User-Feedback: "der blaue Gravity Planet der noch in der Mitte zu sehen
    // ist verhindert aktuell noch sehr, dass es wirklich 3D aussieht."
    // Das trifft den Kern, und der Grund ist strukturell: er sass exakt auf
    // dem Fluchtpunkt - also genau an der Stelle, die im Bild "unendlich weit
    // weg" bedeutet. Ein sichtbares Objekt an dieser Stelle ist ein
    // Widerspruch, den das Auge sofort als Aufkleber auf einer Scheibe liest.
    // Dieser eine Punkt hat das gesamte Bild flachgezogen, egal wie gut alles
    // andere wurde.
    // Gravity wird stattdessen vom grossen, angeschnittenen Planeten am
    // unteren Bildrand getragen (siehe ganz unten in diesem Block) - dort
    // ist der Fluchtpunkt frei, und es entsteht zusaetzlich das nahe Objekt,
    // das der Tiefenstaffelung bisher gefehlt hat.

    // ===== ZEICHEN-REIHENFOLGE = TIEFENSTAFFELUNG =====
    // Die Sternlinien werden jetzt ZUERST gezeichnet, also GANZ HINTEN.
    // Vorher lagen sie ganz vorn (frueherer User-Wunsch: "Sternlinien sollen
    // immer ganz im Vordergrund sein, auch nicht hinter den Planeten"), was
    // aber aktiv gegen die Raeumlichkeit gearbeitet hat: ein Stern ist das
    // fernste Objekt ueberhaupt und KANN nicht vor einem nahen Planeten
    // vorbeiziehen. Solange er das tat, blieb das Bild flach, egal wie gut
    // der Rest wurde. Der damalige Grund - Sterne gingen ueber hellen
    // Planetenflaechen unter - war in Wahrheit ein Symptom der fehlenden
    // Luftperspektive und ist mit ihr geloest; der schwarze Unterstrich als
    // Notbehelf entfaellt dadurch ebenfalls.
    //
    // Reihenfolge von hinten nach vorn:
    //   1. Sternlinien + Orbit-Sterne   (unendlich weit weg)
    //   2. Mond und die 3 Planeten      (mittlere Distanz)
    //   3. Der grosse angeschnittene Planet am unteren Rand (ganz nah)

    // Sternlinien (bestehendes Feld, Farbe je nach Galaxy) + zusaetzliche
    // Orbit-Sterne (fest blau) - beide zusammen ganz zuletzt gezeichnet,
    // damit sie ueber allem anderen liegen (User: "Sternlinien sollen
    // immer ganz im Vordergrund sein, auch nicht hinter den Planeten").
    // Jede Linie bekommt zusaetzlich einen weichen, breiteren Unterstrich
    // bei niedrigerer Deckkraft VOR dem eigentlichen Strich - das sorgt
    // dafuer, dass sie sich auch ueber hellen Planeten-/Mond-Flaechen klar
    // absetzt, OHNE dass die Planeten selbst heller gemacht werden muessten
    // (User-Wunsch: "nicht die Planeten komplett hell machen").
    auto drawStarLine = [&] (FlyingStar& s, juce::Colour colour, float extraBrightnessMult, float extraThicknessMult)
    {
        // Sternlinien lassen sich im View-Panel abschalten (User).
        if (! starLinesVisible) return;
        // Auf welcher Bildhaelfte fliegt dieser Stern? cos < 0 = links.
        // Der Winkel eines Sterns aendert sich nur beim Respawn, die Seite
        // bleibt also waehrend seines ganzen Flugs stabil.
        const bool starOnLeft = std::cos (s.angle) < 0.0f;
        const bool starReversed = (starOnLeft && polLFlipped) || (! starOnLeft && polRFlipped);

        const float step = s.speed * (0.15f + s.radius * 0.9f) * 0.03f * lineSpeedMult;

        // Beim Respawn wird der Winkel neu gewuerfelt, aber BEWUSST innerhalb
        // derselben Bildhaelfte. Wuerde er frei neu gezogen, koennte ein Stern
        // direkt nach dem Respawn auf der jeweils anderen Seite landen - dort
        // liefe er sofort in die Gegenrichtung und wuerde im naechsten Frame
        // erneut respawnen. Das Ergebnis waeren flackernde Sterne genau an den
        // Raendern. Die Seite festzuhalten verhindert das vollstaendig.
        auto respawnSameSide = [&] (bool atOuterEdge)
        {
            float a = (bgRandom.nextFloat() - 0.5f) * juce::MathConstants<float>::pi; // -90..+90 Grad = rechts
            if (starOnLeft)
                a += juce::MathConstants<float>::pi;
            s.angle  = a;
            s.speed  = 0.16f + bgRandom.nextFloat() * 0.22f;
            s.alpha  = 0.14f + bgRandom.nextFloat() * 0.20f;
            s.radius = atOuterEdge ? 1.35f : 0.0f;
        };

        if (starReversed)
        {
            s.radius -= step;
            if (s.radius <= 0.0f)
                respawnSameSide (true);   // faellt in den Fluchtpunkt -> aussen neu
        }
        else
        {
            s.radius += step;
            if (s.radius > 1.35f)
                respawnSameSide (false);  // verlaesst das Bild -> in der Mitte neu
        }

        // Kein Clamp mehr auf 1.0 - Linien duerfen ueber den sichtbaren
        // Rand hinauslaufen, werden erst vom Bildrand/Rundrahmen-Clip in
        // paint() beschnitten statt hart bei 1.0 zu stoppen.
        const float headR = s.radius;
        // Bei wenig Speed werden die Linien kuerzer (siehe lineVis).
        const float tailR = juce::jmax (0.0f, headR - (0.05f + headR * 0.12f) * (0.35f + 0.65f * lineVis));

        // ===== ORBIT: Umverteilung statt "mehr Linien" =====
        // User-Feedback: "Orbit hat soundtechnisch einen sehr starken
        // Einfluss. Im Moment macht Orbit nur 'mehr Linien'. Es extrahiert ja
        // die Seiten."
        // Genau deshalb ist eine Dehnung hier das falsche Bild: Orbit macht
        // nichts breiter, es VERSCHIEBT Energie aus der Mitte nach L und R.
        // Die ehrliche Entsprechung ist also eine Umverteilung der
        // Flugrichtungen: bei Orbit = 0 fliegen die Sterne gleichmaessig in
        // alle Richtungen, mit steigendem Orbit draengen sie sich immer
        // staerker nach links und rechts und raeumen die Mitte.
        //
        // Die Warp-Funktion a' = a - c * sin(2a) zieht jeden Winkel zur
        // naechstgelegenen Horizontalen (0 oder 180 Grad) und laesst genau
        // diese beiden Richtungen als Fixpunkte unveraendert - deshalb wirkt
        // es wie ein Auseinanderziehen und nicht wie ein Verdrehen. Kostet
        // eine einzige Sinus-Rechnung pro Stern.
        // Die zusaetzlichen Orbit-Sterne bleiben erhalten - beides zusammen
        // ergibt "mehr Seitenenergie" statt nur "mehr Linien".
        const float orbitAngle = s.angle - (orbitNorm * 0.55f) * std::sin (2.0f * s.angle) + rayAngleOffset;

        const float cosA = std::cos (orbitAngle);
        const float sinA = std::sin (orbitAngle);
        // Ursprung ist NICHT mehr fest die Bildmitte, sondern der wandernde
        // Fluchtpunkt (siehe starOriginX/starOriginY oben) - die Sterne
        // entspringen dadurch immer aus der aktuellen "Flugrichtung".
        float hx = starOriginX + bgRadius * headR * cosA;
        float hy = starOriginY + bgRadius * headR * sinA;
        float tx = starOriginX + bgRadius * tailR * cosA;
        float ty = starOriginY + bgRadius * tailR * sinA;

        // Shift: wellenfoermige seitliche Auslenkung quer zur Flugrichtung.
        if (waveAmp > 0.001f)
        {
            const float perpX = -sinA, perpY = cosA;
            const float offH = waveAmp * std::sin (wavePhase + headR * 6.0f + orbitAngle * 2.0f);
            const float offT = waveAmp * std::sin (wavePhase + tailR * 6.0f + orbitAngle * 2.0f);
            hx += perpX * offH; hy += perpY * offH;
            tx += perpX * offT; ty += perpY * offT;
        }

        // Width: horizontale Stauchung/Dehnung um den Fluchtpunkt herum -
        // ganz zum Schluss, damit ALLES (inkl. der Shift-Welle) gleichmaessig
        // mitskaliert wird.
        if (std::abs (starStretchX - 1.0f) > 0.001f)
        {
            hx = starOriginX + (hx - starOriginX) * starStretchX;
            tx = starOriginX + (tx - starOriginX) * starStretchX;
        }

        const float clampedHeadR = juce::jlimit (0.0f, 1.0f, headR);
        // Luftperspektive: ein Stern nahe dem Fluchtpunkt ist noch weit weg
        // und dadurch schwach, ein Stern am Rand fliegt gerade dicht an uns
        // vorbei und ist hell. Die Kurve ist bewusst quadratisch statt linear
        // (vorher 0.25 + r*0.75) - dadurch bleiben ferne Sterne deutlich
        // laenger zurueckhaltend und der Helligkeitssprung passiert erst
        // dort, wo sie wirklich nah sind. Genau das erzeugt den Eindruck von
        // Distanz statt einer gleichmaessig hellen Streuung.
        const float fade = 0.10f + clampedHeadR * clampedHeadR * 0.90f;

        const float horizontalness = cosA * cosA; // 1 = an den Seiten, 0 = oben/unten
        const float sideBoost = sideEmphasis * horizontalness;
        const float brightness = juce::jlimit (0.0f, 1.0f, s.alpha * fade * (1.0f + sideBoost * 2.5f) * extraBrightnessMult * lineVis);
        // Grunddicke etwas duenner als frueher (0.6/1.1 -> 0.5/0.95, User),
        // bei wenig Speed zusaetzlich duenner, und eine harte Obergrenze,
        // damit Size + Orbit zusammen keine Balken mehr ergeben (User).
        const float coreThickness = juce::jmin (3.6f,
            (0.5f + clampedHeadR * 0.95f) * (1.0f + sideBoost * 0.7f) * lineThicknessMult * extraThicknessMult
                * (0.45f + 0.55f * lineVis));

        // Der frueher hier gezeichnete schwarze Unterstrich ist ENTFALLEN.
        // Er war noetig, solange die Sterne VOR den Planeten lagen und sich
        // dort sonst nicht absetzten. Seit die Sterne hinter den Objekten
        // liegen (siehe Zeichen-Reihenfolge oben) waere er nur noch ein
        // dunkler Rand um jede Linie - also genau der matschige Look, den
        // wir loswerden wollen.
        if (brightness <= 0.002f)
            return;   // bei Speed nahe Minimum: Linie bewegt sich noch, ist aber unsichtbar
        g.setColour (colour.withAlpha (brightness));
        g.drawLine (tx, ty, hx, hy, coreThickness);
    };

    // "Reduce Animations": nur jede zweite Sternlinie zeichnen UND bewegen.
    // Bewusst ueber den Schleifen-Schritt geloest statt ueber eine kleinere
    // Liste - so bleibt der Sternpool unveraendert und beim Zurueckschalten
    // ist das volle Feld sofort wieder da, ohne Neuaufbau.
    gCur = &gTwinkle;
    // ===== FUNKELNDE STERNE (ganz hinten) =====
    // Zuerst gezeichnet, also hinter allem. Helligkeit atmet mit sceneT,
    // haelt also beim DAW-Stop mit an. Ein kurzer Aufblitz-Anteil (hohe
    // Potenz) sorgt dafuer, dass sie wirklich funkeln statt nur zu pulsieren.
    // Density wirkt auch aufs Funkeln (User): nach oben staerker (bis +30 %),
    // nach unten sanfter (bis -25 %) - "Sterne sollen nie zu wenig werden".
    const float twinkleDensity = starfieldMode == 1 ? 1.0f : viewDensity;   // Gonio-Ansicht: Density fest 100 (User)
    const float twinkleMul = twinkleDensity >= 1.0f ? 1.0f + (twinkleDensity - 1.0f) * 0.6f
                                                    : 1.0f - (1.0f - twinkleDensity) * 0.31f;
    const size_t twinkleCount = twinkleVisible
                              ? (size_t) juce::jlimit (0, (int) twinkles.size(), (int) std::round (46.0f * twinkleMul * (themeModern() ? 1.35f : 1.0f)))   // gezeichneter Himmel: mehr Sterne (User)
                              : (size_t) 0;
    for (size_t ti = 0; ti < twinkleCount; ++ti)
    {
        const auto& t = twinkles[ti];
        const float ph = twinkleClock / t.period * juce::MathConstants<float>::twoPi + t.phase;
        const float base = 0.5f + 0.5f * std::sin (ph);
        const float spark = std::pow (base, 6.0f);
        const float a = 0.10f + base * 0.22f + spark * 0.45f;
        const float px = cx + t.x * bgRadius;
        const float py = cy + t.y * bgRadius;
        const float r  = t.size * (0.8f + spark * 0.7f);
        g.setColour (juce::Colour (0xffdfe8ff).withAlpha (juce::jlimit (0.0f, 1.0f, a)));
        g.fillEllipse (px - r, py - r, r * 2.0f, r * 2.0f);
        if (spark > 0.6f)
        {
            // kleines Kreuz beim Aufblitzen
            g.setColour (juce::Colours::white.withAlpha ((spark - 0.6f) * 1.2f));
            g.drawLine (px - r * 2.6f, py, px + r * 2.6f, py, 0.8f);
            g.drawLine (px, py - r * 2.6f, px, py + r * 2.6f, 0.8f);
        }
    }

    // ===== KLEINER MOND (Gonio-Ansicht) =====
    // Steht an der Stelle der Sonne, wenn Space aus ist - dezent, grau, mit
    // drei Kratern. Klick darauf schaltet zurueck nach Space ("Licht an").
    // Blendet gegenlaeufig zur Sonne (1 - spaceBlend).
    if (spaceBlend < 0.99f)
    {
        gCur = &gGlow;
        const float mb = 1.0f - spaceBlend;
        const float mx = (float) w * 0.10f + std::sin (sceneT / 53.0f * juce::MathConstants<float>::twoPi + 0.4f) * 0.0015f * bgRadius;
        const float my = (float) h * 0.14f + std::cos (sceneT / 72.0f * juce::MathConstants<float>::twoPi + 0.4f) * 0.0012f * bgRadius;
        const float mr = moonRadiusPx * 0.24f;
        if (spaceBlend < 0.5f) { sunHitX = mx; sunHitY = my; sunHitR = mr * 2.4f; sunDrawR = mr; }

        if (themeFlat())
        {
            // Flat (User): alles flach - eine solide Sichel ohne Verlauf und
            // ohne Schein, dazu ein duenner, ebenso solider Konturring.
            const juce::Colour cr (0xffd8dee8), cr2 (0xff6f7787);
            const float R = mr * 1.05f;
            juce::Path sickle;
            sickle.addEllipse (mx - R, my - R, R * 2.0f, R * 2.0f);
            sickle.addEllipse (mx - R + R * 0.62f, my - R - R * 0.18f, R * 2.0f, R * 2.0f);
            sickle.setUsingNonZeroWinding (false);
            g.saveState();
            juce::Path only; only.addEllipse (mx - R, my - R, R * 2.0f, R * 2.0f);
            g.reduceClipRegion (only);
            g.setColour (cr2.withAlpha (0.55f * mb));
            g.fillEllipse (mx - R, my - R, R * 2.0f, R * 2.0f);
            g.setColour (cr.withAlpha (0.95f * mb));
            g.fillPath (sickle);
            g.restoreState();
            g.setColour (cr.withAlpha (0.45f * mb));
            g.drawEllipse (mx - R, my - R, R * 2.0f, R * 2.0f, 1.2f);
            gCur = &gLines;
        }
        else if (themeModern())
        {
            // Modern-Familie (User: Eclipse "passt nicht"): eine schmale,
            // elegante Mondsichel - kein Kreis, kein Krater, nur die Sichel
            // mit weichem Schein in der Theme-Farbe.
            const juce::Colour cr (uiTheme == 4 ? 0xfffff0c2 : uiTheme == 5 ? 0xffeef0f4 : 0xffe8eefc);   // Day & Night warm, Moon silbrig, Modern kuehl
            const float R = mr * 1.05f;
            juce::Path sickle;
            sickle.addEllipse (mx - R, my - R, R * 2.0f, R * 2.0f);
            sickle.addEllipse (mx - R + R * 0.62f, my - R - R * 0.18f, R * 2.0f, R * 2.0f);   // "Schatten"-Scheibe nach oben rechts versetzt
            sickle.setUsingNonZeroWinding (false);
            g.saveState();
            juce::Path only; only.addEllipse (mx - R, my - R, R * 2.0f, R * 2.0f);
            g.reduceClipRegion (only);
            const float hr = R * 2.2f;
            juce::ColourGradient halo (cr.withAlpha (0.30f * mb), mx - R * 0.4f, my + R * 0.2f, cr.withAlpha (0.0f), mx - R * 0.4f + hr, my + R * 0.2f, true);
            g.setGradientFill (halo);
            g.fillEllipse (mx - R * 0.4f - hr, my + R * 0.2f - hr, hr * 2.0f, hr * 2.0f);
            g.setColour (cr.withAlpha (0.95f * mb));
            g.fillPath (sickle);
            g.restoreState();
            gCur = &gLines;
        }
        else if (themeSciFi())
        {
            // Sonnenfinsternis statt Mond (User: Mond "zu comic"): schwarze
            // Scheibe, duenne helle Korona, kleiner Diamantring oben rechts.
            const juce::Colour cor (0xffd48cff);
            const float hr = mr * 2.4f;
            juce::ColourGradient halo (cor.withAlpha (0.55f * mb), mx, my, cor.withAlpha (0.0f), mx + hr, my, true);
            halo.addColour (0.42, cor.withAlpha (0.55f * mb));
            halo.addColour (0.60, cor.withAlpha (0.12f * mb));
            g.setGradientFill (halo);
            g.fillEllipse (mx - hr, my - hr, hr * 2.0f, hr * 2.0f);
            g.setColour (juce::Colour (0xff05060a).withAlpha (mb));
            g.fillEllipse (mx - mr, my - mr, mr * 2.0f, mr * 2.0f);
            g.setColour (cor.withAlpha (0.9f * mb));
            g.drawEllipse (mx - mr, my - mr, mr * 2.0f, mr * 2.0f, 1.2f);
            const float dx = mx + mr * 0.72f, dy = my - mr * 0.70f;
            g.setColour (juce::Colours::white.withAlpha (mb));
            g.fillEllipse (dx - mr * 0.16f, dy - mr * 0.16f, mr * 0.32f, mr * 0.32f);
            g.setColour (cor.withAlpha (0.35f * mb));
            g.fillEllipse (dx - mr * 0.45f, dy - mr * 0.45f, mr * 0.9f, mr * 0.9f);
            gCur = &gLines;
        }
        else
        {
        // Watercolor: warmer, gemalter Ton statt Grau (User).
        const juce::Colour mHi (themeWater() ? 0xffefe3c6 : 0xffd8dbe2), mLo (themeWater() ? 0xff8f8468 : 0xff8a8e98);
        juce::ColourGradient mg (mHi.withAlpha (0.95f * mb), mx - mr * 0.3f, my - mr * 0.3f,
                                 mLo.withAlpha (0.95f * mb), mx + mr, my + mr, true);
        g.setGradientFill (mg);
        g.fillEllipse (mx - mr, my - mr, mr * 2.0f, mr * 2.0f);
        static const float cLon[3] = { -0.5f, 0.6f, 0.1f }, cLat[3] = { 0.3f, -0.2f, -0.55f }, cRel[3] = { 0.20f, 0.14f, 0.11f };
        for (int i = 0; i < 3; ++i)
        {
            const float sx = std::sin (cLon[i]) * std::cos (cLat[i]), sy = std::sin (cLat[i]), sz = std::cos (cLon[i]) * std::cos (cLat[i]);
            const float rr = cRel[i] * mr;
            g.setColour (juce::Colour (0xff6d717b).withAlpha (0.60f * mb));
            g.fillEllipse (mx + sx * mr - rr * sz, my - sy * mr - rr, rr * sz * 2.0f, rr * 2.0f);
        }
        // Schattenseite unten rechts
        g.saveState();
        juce::Path clip; clip.addEllipse (mx - mr, my - mr, mr * 2.0f, mr * 2.0f);
        g.reduceClipRegion (clip);
        juce::ColourGradient sh (juce::Colours::transparentBlack, mx - mr * 0.3f, my - mr * 0.3f,
                                 juce::Colours::black.withAlpha (0.55f * mb), mx + mr * 0.9f, my + mr * 0.9f, true);
        sh.addColour (0.45, juce::Colours::transparentBlack);
        g.setGradientFill (sh);
        g.fillEllipse (mx - mr, my - mr, mr * 2.0f, mr * 2.0f);
        g.restoreState();
        gCur = &gLines;
        }
    }

    // View "Stars" (User, Runde 19): die Szene wird weiterhin gezeichnet -
    // beim Zusammensetzen unten bekommen die Objekte nur noch rund 10 %
    // Deckkraft und der Gravity-Planet 80 %. Nur die Sternlinien bleiben
    // aussen vor (die lenken am meisten ab). Frueher wurde hier alles
    // uebersprungen; deshalb steht der Block jetzt immer offen.
    {
    gCur = &gLines;
    const int starStep = reducedAnimations ? 2 : 1;

    // ===== RAY: goldene Linien =====
    // User-Idee aus dem Visual-Pool: "goldene Linien hinzufuegen (aehnlich
    // wie die blauen Linien) -> Ray (Phaser)". Je nach Staerke wird jede
    // fuenfte, vierte oder dritte Linie golden und schwingt seitlich mit dem
    // Phaser-LFO - das ist genau das, was ein Phaser tut: er verschiebt
    // Anteile des Signals periodisch, ohne sie zu bewegen. Die Linien bleiben
    // an ihrem Platz, pendeln aber. Gold, weil es sonst nur an den
    // Sternschnuppen vorkommt und sich klar vom Blau/Weiss abhebt.
    // Linienfarbe je Theme (User: "Linien ans Gonio/Theme anpassen"): Modern
    // blau-weiss, Watercolor Gold, Pop Gold, Sci-Fi Violett.
    static const juce::uint32 rayColByTheme[kThemeCount] = { 0xffb9d2ff, 0xffdabd76, 0xffffc247, 0xffd48cff, 0xffe6c9a0, 0xffe2d3a4 };   // Modern, Dark Night, Pop, Sci-Fi, Day & Night, Moon
    const juce::Colour rayGold (rayColByTheme[juce::jlimit (0, kThemeCount - 1, uiTheme)]);
    // Mehr goldene Linien je Stufe (User: "je nach Intensitaet auch noch ein
    // paar extra gelbe Linien"): jede 6., 3., 2. Linie.
    static const int rayEveryByLevel[4] = { 0, 6, 3, 2 };   // Off, leicht, mittel, stark
    const int rayEvery = rayOn ? rayEveryByLevel[rayLevel] : 0;
    const float rayWobble = rayLfo * (0.02f + 0.035f * (float) rayLevel) * lineVis;
    // View-Panel Density: Anteil der Sterne aus dem Pool, der ueberhaupt
    // gezeichnet wird (der Pool ist gross genug fuer > 1, siehe generateBackground()).
    constexpr float kBaseStars = 54.0f;
    const size_t starCount = (size_t) juce::jlimit (0, (int) bgStars.size(), (int) std::round (kBaseStars * viewDensity));
    for (size_t si = 0; si < starCount; si += (size_t) starStep)
    {
        const bool golden = rayEvery > 0 && (si % (size_t) rayEvery) == 0;
        rayAngleOffset = golden ? rayWobble : 0.0f;
        if (golden)
            drawStarLine (bgStars[si], rayGold, lineBrightnessMult * 1.25f, 1.35f);
        else
            drawStarLine (bgStars[si], lineColour, lineBrightnessMult, 1.0f);
    }
    rayAngleOffset = 0.0f;

    // AIR: die zusaetzlichen blauen Sterne (Runde 50, User: "Air -> blaue
    // Linien"). Der Regler steuert Anzahl, Dicke/Laenge und Leuchtkraft;
    // Gravity legt ueber colourIntensity die Farbstaerke darueber.
    if (airNorm > 0.001f)
    {
        int activeOrbitStars = juce::jlimit (0, (int) orbitStars.size(), (int) std::round (airNorm * (float) orbitStars.size()));
        if (reducedAnimations)
            activeOrbitStars /= 2;
        const auto airColour = juce::Colour (0xff6bb8ff).withMultipliedSaturation (juce::jmin (1.6f, colourIntensity));
        for (int i = 0; i < activeOrbitStars; ++i)
            drawStarLine (orbitStars[(size_t) i], airColour,
                          lineBrightnessMult * (0.6f + airNorm * 0.8f),
                          1.6f * (0.7f + airNorm * 0.6f));   // 2.0 -> 1.6 (User: max Dicke reduziert)
    }

    gCur = &gObject;
    // ===== HIMMELSKOERPER (Redesign) =====
    // Ersetzt den Mond, die drei Farbplaneten und den alten Kometen (User:
    // "sieht aus wie Scheinwerfer"). Jetzt fuenf feste, prozedural und
    // realistisch gezeichnete Objekte - Ringplanet, Baenderplanet, Erde,
    // kleine Sonne, Spiralgalaxie - plus ein Pool aus hoechstens einem
    // Kometen und zwei Asteroiden, die nur gelegentlich und weit im
    // Hintergrund durchs Feld ziehen. Alles liegt in der Objekt-Ebene
    // (Shine-Cap 80 %, siehe paint()), Shine wirkt also auf alles gleich.
    //
    // Bewegung: dieselbe Logik wie bisher (Tilt/Drift -> X, Elevate -> Y,
    // Width radial, Distance schrumpft, Parallaxe nach Groesse), nur
    // deutlich dezenter (User: "Planeten sollten sich nicht so viel
    // bewegen") und ueber geglaettete Werte, damit Preset-Wechsel und
    // Reglerspruenge gleiten statt springen.
    {
        const float fieldW = (float) w;
        const float fieldH = (float) h;
        const float unitR  = moonRadiusPx;   // Referenzradius (aus dem Gravity-Knopf)

        // ---- Reglerwerte, geglaettet (~0,45 s) ----
        const float driftRaw    = liveOrRaw (processor.currentDriftLivePercent,    LCRMSAudioProcessor::ID_DRIFT);        // -100..100
        const float tiltRaw     = liveOrRaw (processor.currentOffsetLivePercent,   LCRMSAudioProcessor::ID_POS_OFFSET);   // -27.5..27.5
        const float distanceRaw = liveOrRaw (processor.currentDistanceLivePercent, LCRMSAudioProcessor::ID_POS_DISTANCE); // 0..100
        const float elevateRaw  = liveOrRaw (processor.currentElevateLivePercent,  LCRMSAudioProcessor::ID_POS_ELEVATE);  // -100..100

        auto smooth = [&] (float& v, float target)
        {
            v += (target - v) * juce::jlimit (0.0f, 1.0f, lastFrameDt / 0.45f);
        };
        smooth (smDrift,    timewarpOn ? juce::jlimit (-1.0f, 1.0f, driftRaw / 100.0f) : 0.0f);
        smooth (smTilt,     positionOn ? juce::jlimit (-1.0f, 1.0f, tiltRaw / 27.5f) : 0.0f);
        smooth (smElevate,  positionOn ? juce::jlimit (-1.0f, 1.0f, elevateRaw / 100.0f) : 0.0f);
        smooth (smWidth,    positionOn ? juce::jlimit (-1.0f, 1.0f, (posWidthRaw - 100.0f) / 50.0f) : 0.0f);
        smooth (smDistance, positionOn ? juce::jlimit (0.0f, 1.0f, distanceRaw / 100.0f) : 0.0f);

        // Rund 45 % der frueheren Bewegungsamplituden.
        constexpr float kMove = 0.45f;
        const float driftArcDip = std::abs (smDrift) * 0.15f;
        const float distShrink  = 1.0f - smDistance * 0.35f;

        // Position eines Objekts: Basis als Anteil von Feldbreite/-hoehe,
        // dazu die Reglerverschiebung, ueber die Parallaxe skaliert
        // (naeher/groesser = bewegt sich mehr), plus winzige Eigendrift.
        auto objectPos = [&] (float fx, float fy, float parallax, float ownPeriod, float ownPhase,
                              float& px, float& py)
        {
            const float par  = parallax * distShrink * kMove;
            const float offX = (smDrift * 0.25f + smTilt * 0.25f) * par;
            const float offY = (smElevate * 0.20f - driftArcDip) * par;
            const float bx = fx - 0.5f, by = fy - 0.5f;
            const float bl = std::sqrt (bx * bx + by * by);
            const float dx = bl > 1.0e-4f ? bx / bl : 0.0f;
            const float dy = bl > 1.0e-4f ? by / bl : 0.0f;
            const float push = smWidth * 0.45f * par;
            const float ownX = std::sin (sceneT / ownPeriod * juce::MathConstants<float>::twoPi + ownPhase) * 0.010f * parallax;
            const float ownY = std::cos (sceneT / (ownPeriod * 1.37f) * juce::MathConstants<float>::twoPi + ownPhase) * 0.008f * parallax;
            px = fieldW * fx + (offX + dx * push + ownX) * bgRadius;
            py = fieldH * fy + (-offY + dy * push + ownY) * bgRadius;
        };

        // Kugelprojektion eines Oberflaechenpunkts (Laenge/Breite) auf die
        // Scheibe. sz = Tiefe (>0 Vorderseite; staucht am Rand waagerecht).
        auto project = [] (float lon, float lat, float& sx, float& sy, float& sz) -> bool
        {
            const float cl = std::cos (lat);
            sx = std::sin (lon) * cl;
            sy = std::sin (lat);
            sz = std::cos (lon) * cl;
            return sz > 0.02f;
        };

        // Einheitliche Beleuchtung von oben links (dort steht auch die
        // Sonne): Schattenseite unten rechts, Glanzpunkt oben links.
        auto sphereShade = [&] (float px, float py, float r, float shadowA, float shineA)
        {
            g.saveState();
            juce::Path clip;
            clip.addEllipse (px - r, py - r, r * 2.0f, r * 2.0f);
            g.reduceClipRegion (clip);
            juce::ColourGradient sh (juce::Colours::transparentBlack, px - r * 0.35f, py - r * 0.35f,
                                     juce::Colours::black.withAlpha (shadowA), px + r * 0.95f, py + r * 0.95f, true);
            sh.addColour (0.45, juce::Colours::transparentBlack);
            g.setGradientFill (sh);
            g.fillEllipse (px - r, py - r, r * 2.0f, r * 2.0f);
            g.restoreState();

            const float sr = r * 0.30f;
            const float hx = px - r * 0.40f, hy = py - r * 0.40f;
            juce::ColourGradient hl (juce::Colours::white.withAlpha (shineA), hx, hy,
                                     juce::Colours::white.withAlpha (0.0f), hx + sr, hy, true);
            g.setGradientFill (hl);
            g.fillEllipse (hx - sr, hy - sr, sr * 2.0f, sr * 2.0f);
        };

        // Waagerechte Oberflaechenbaender (Jupiter/Saturn), auf die Kugel
        // geclippt. lat0/lat1 in Radiant, -pi/2 = Suedpol.
        struct Band { float lat0, lat1; juce::uint32 col; float alpha; };
        // Planetenfarbe je Theme (User: der goldene Ringplanet passt nicht
        // ueberall): Modern fast farblos, Sci-Fi violett, sonst wie gehabt.
        auto tintPlanet = [this] (juce::Colour c)
        {
            if (uiTheme == 0) return c.withHue (0.60f).withSaturation (0.12f);   // kuehl, fast farblos (User)
            if (uiTheme == 4) return c.withHue (0.10f).withSaturation (0.40f);   // Day & Night: warm
            if (uiTheme == 5) return c.withHue (0.60f).withSaturation (0.05f);   // Moon: kuehles Silber
            if (themeSciFi())   // Neon violett
                return c.withHue (0.76f).withSaturation (juce::jmin (1.0f, c.getSaturation() * 1.6f + 0.30f));
            return c;
        };
        auto drawBands = [&] (float px, float py, float r, const Band* bands, int n)
        {
            g.saveState();
            juce::Path clip;
            clip.addEllipse (px - r, py - r, r * 2.0f, r * 2.0f);
            g.reduceClipRegion (clip);
            for (int i = 0; i < n; ++i)
            {
                const float y0 = py - std::sin (bands[i].lat1) * r;
                const float y1 = py - std::sin (bands[i].lat0) * r;
                g.setColour (tintPlanet (juce::Colour (bands[i].col)).withAlpha (bands[i].alpha));
                g.fillRect (px - r, y0, r * 2.0f, y1 - y0);
            }
            g.restoreState();
        };

        // ===== KOMETEN & ASTEROIDEN (Pool, max. 1 + 2) =====
        // Weit hinten, deshalb VOR den Planeten gezeichnet (die verdecken
        // sie). Tilt/Width/Drift haben keinen Einfluss - sie ziehen ihre
        // eigene Bahn. Geschwindigkeit, Groesse und Helligkeit sind je
        // Auftritt zufaellig, aber alle im dezenten Bereich.
        {
            const float dtScene = (1.0f / 30.0f) * motionScale * sceneSpeedMul();
            int nComet = 0, nAst = 0;
            for (auto& t : transients)
                if (t.active) { if (t.comet) ++nComet; else ++nAst; }

            transientWait -= dtScene;
            if (transientWait <= 0.0f)
            {
                transientWait = 25.0f + bgRandom.nextFloat() * 50.0f;   // nur gelegentlich
                const bool wantComet = (nComet == 0) && (nAst >= 2 || bgRandom.nextFloat() < 0.4f);
                const bool wantAst   = ! wantComet && nAst < 2;
                if (wantComet || wantAst)
                {
                    for (auto& t : transients)
                    {
                        if (t.active)
                            continue;
                        t.active = true;
                        t.comet  = wantComet;
                        // Start knapp ausserhalb einer zufaelligen Kante,
                        // Richtung schraeg ins Feld hinein.
                        const int   edge  = bgRandom.nextInt (4);
                        const float along = 0.10f + bgRandom.nextFloat() * 0.80f;
                        const float ang   = (bgRandom.nextFloat() - 0.5f) * 1.2f;
                        float dirX = 0.0f, dirY = 0.0f;
                        switch (edge)
                        {
                            case 0:  t.x = -0.08f; t.y = along;  dirX =  std::cos (ang); dirY = std::sin (ang); break;
                            case 1:  t.x =  1.08f; t.y = along;  dirX = -std::cos (ang); dirY = std::sin (ang); break;
                            case 2:  t.x = along;  t.y = -0.08f; dirX =  std::sin (ang); dirY = std::cos (ang); break;
                            default: t.x = along;  t.y =  1.08f; dirX =  std::sin (ang); dirY = -std::cos (ang); break;
                        }
                        const float speed = t.comet ? (0.05f + bgRandom.nextFloat() * 0.05f)
                                                    : (0.025f + bgRandom.nextFloat() * 0.025f);
                        t.vx = dirX * speed;
                        t.vy = dirY * speed;
                        t.size = t.comet ? (0.006f + bgRandom.nextFloat() * 0.005f)
                                         : (0.008f + bgRandom.nextFloat() * 0.010f);
                        t.bright   = 0.6f + bgRandom.nextFloat() * 0.4f;
                        t.spin     = bgRandom.nextFloat() * juce::MathConstants<float>::twoPi;
                        t.spinRate = (bgRandom.nextFloat() - 0.5f) * 1.2f;
                        t.seed     = bgRandom.nextInt (1000);
                        break;
                    }
                }
            }

            for (auto& t : transients)
            {
                if (! t.active)
                    continue;
                t.x    += t.vx * dtScene;
                t.y    += t.vy * dtScene;
                t.spin += t.spinRate * dtScene;
                if (t.x < -0.12f || t.x > 1.12f || t.y < -0.12f || t.y > 1.12f)
                {
                    t.active = false;
                    continue;
                }
                // Weiches Ein-/Ausblenden an den Raendern.
                const float m  = juce::jmin (juce::jmin (t.x, 1.0f - t.x), juce::jmin (t.y, 1.0f - t.y));
                const float ef = juce::jlimit (0.0f, 1.0f, (m + 0.02f) / 0.10f) * t.bright;
                if (ef <= 0.001f)
                    continue;

                const float sx = fieldW * t.x;
                const float sy = fieldH * t.y;

                // Kometen leuchten selbst -> Leucht-Ebene (eigene Shine-Kurve,
                // siehe paint()); Asteroiden sind matt -> Objekt-Ebene.
                gCur = t.comet ? &gGlow : &gObject;
                if (t.comet)
                {
                    // Schweif entgegen der Flugrichtung, duenn und lang statt
                    // breiter Lichtkegel.
                    const float vl = std::sqrt (t.vx * t.vx + t.vy * t.vy);
                    const float tx = vl > 1.0e-5f ? -t.vx / vl : 0.7f;
                    const float ty = vl > 1.0e-5f ? -t.vy / vl : 0.7f;
                    const float nucR    = bgRadius * t.size;
                    const float tailLen = bgRadius * (0.14f + t.size * 8.0f);
                    const float ex = sx + tx * tailLen, ey = sy + ty * tailLen;
                    const juce::Colour ice (0xffc4e2ff);

                    juce::ColourGradient dust (ice.withAlpha (0.16f * ef), sx, sy, ice.withAlpha (0.0f), ex, ey, false);
                    g.setGradientFill (dust);
                    g.drawLine (sx, sy, ex, ey, juce::jmax (1.5f, nucR * 3.0f));

                    juce::ColourGradient ion (juce::Colours::white.withAlpha (0.38f * ef), sx, sy,
                                              ice.withAlpha (0.0f), sx + tx * tailLen * 0.85f, sy + ty * tailLen * 0.85f, false);
                    g.setGradientFill (ion);
                    g.drawLine (sx, sy, sx + tx * tailLen * 0.85f, sy + ty * tailLen * 0.85f, juce::jmax (1.0f, nucR * 0.9f));

                    juce::ColourGradient glow (ice.withAlpha (0.30f * ef), sx, sy, ice.withAlpha (0.0f), sx + nucR * 2.6f, sy, true);
                    g.setGradientFill (glow);
                    g.fillEllipse (sx - nucR * 2.6f, sy - nucR * 2.6f, nucR * 5.2f, nucR * 5.2f);

                    g.setColour (juce::Colours::white.withAlpha (0.75f * ef));
                    g.fillEllipse (sx - nucR, sy - nucR, nucR * 2.0f, nucR * 2.0f);
                }
                else
                {
                    // Unregelmaessiger, langsam rotierender Brocken.
                    const float ar = bgRadius * t.size;
                    juce::Path rock;
                    for (int i = 0; i < 7; ++i)
                    {
                        const float a  = t.spin + (float) i / 7.0f * juce::MathConstants<float>::twoPi;
                        const float rv = ar * (0.70f + 0.30f * (float) ((t.seed * 31 + i * 17) % 10) / 9.0f);
                        const float vx = sx + std::cos (a) * rv, vy = sy + std::sin (a) * rv;
                        if (i == 0) rock.startNewSubPath (vx, vy); else rock.lineTo (vx, vy);
                    }
                    rock.closeSubPath();
                    juce::ColourGradient rg (juce::Colour (0xffa39e97).withAlpha (0.55f * ef), sx - ar, sy - ar,
                                             juce::Colour (0xff4e4a46).withAlpha (0.55f * ef), sx + ar, sy + ar, false);
                    g.setGradientFill (rg);
                    g.fillPath (rock);
                }
            }
            gCur = &gObject;
        }

        // ===== SZENEN-SPRITES (alle Szenen ausser Comic/Stars) =====
        // Echte Planeten-Fotos als Ausschnitte, gleiche Parallaxe-Logik wie
        // die gezeichneten Objekte. Selbstleuchter (Sonne, Eclipse) in die
        // Leucht-Ebene.
        // ===== DIE FUENF OBJEKTE =====
        // Reihenfolge = Tiefe: Galaxie und Sonne ganz hinten, dann Erde,
        // dann die beiden grossen Planeten.
        {

        // --- Spiralgalaxien (weit weg) ---
        // Zwei Stueck (User): die kleine unten rechts leicht violett, dazu
        // eine etwas groessere, farbigere links unten. Beide drehen sehr
        // langsam (eine Umdrehung in gut fuenf Minuten).
        auto drawGalaxy = [&] (float gx, float gy, float rg, float theta, float squash,
                               juce::Colour coreCol, juce::Colour innerArm, juce::Colour outerArm, float armAlpha)
        {
            g.saveState();
            g.addTransform (juce::AffineTransform::scale (1.0f, squash, gx, gy)
                                .followedBy (juce::AffineTransform::rotation (theta, gx, gy)));

            juce::ColourGradient disc (outerArm.withAlpha (0.16f), gx, gy, outerArm.withAlpha (0.0f), gx + rg, gy, true);
            g.setGradientFill (disc);
            g.fillEllipse (gx - rg, gy - rg, rg * 2.0f, rg * 2.0f);

            for (int arm = 0; arm < 2; ++arm)
            {
                for (int i = 0; i < 24; ++i)
                {
                    const float tt  = (float) i / 23.0f;
                    const float a   = tt * 2.6f * juce::MathConstants<float>::pi + (float) arm * juce::MathConstants<float>::pi;
                    const float rr  = rg * (0.12f + 0.88f * tt);
                    const float dsz = rg * (0.11f - 0.06f * tt);
                    const juce::Colour c = innerArm.interpolatedWith (outerArm, tt);
                    g.setColour (c.withAlpha (armAlpha * (1.0f - tt * 0.7f)));
                    g.fillEllipse (gx + std::cos (a) * rr - dsz, gy + std::sin (a) * rr - dsz, dsz * 2.0f, dsz * 2.0f);
                }
            }

            juce::ColourGradient core (coreCol.withAlpha (0.85f), gx, gy, coreCol.withAlpha (0.0f), gx + rg * 0.35f, gy, true);
            g.setGradientFill (core);
            g.fillEllipse (gx - rg * 0.35f, gy - rg * 0.35f, rg * 0.7f, rg * 0.7f);
            g.restoreState();
        };
        if (themeWater() || uiTheme == 2)   // Galaxien nur Dark Night/Pop (User: in den anderen Themes zu viel)
        {
            float gx, gy;
            // Dark Night (User): die rechte Galaxie minimal tiefer.
            objectPos (0.82f, themeWater() ? 0.60f : 0.56f, 0.10f, 61.0f, 1.3f, gx, gy);
            drawGalaxy (gx, gy, unitR * 0.62f * distShrink,
                        0.45f + sceneT * (juce::MathConstants<float>::twoPi / 320.0f), 0.48f,
                        juce::Colour (0xfff3e6ff), juce::Colour (0xffe0c8ff), juce::Colour (0xffa98cff), 0.22f);
        }
        if (themeWater() || uiTheme == 2)
        {
            float gx, gy;
            // Dark Night (User): die linke Galaxie nach oben rechts, ungefaehr
            // dorthin, wo in den anderen Themes der grosse Planet steht.
            if (themeWater()) objectPos (0.36f, 0.46f, 0.12f, 71.0f, 3.9f, gx, gy);
            else              objectPos (0.13f, 0.60f, 0.12f, 71.0f, 3.9f, gx, gy);
            drawGalaxy (gx, gy, unitR * 0.86f * distShrink,
                        -0.35f + sceneT * (juce::MathConstants<float>::twoPi / 380.0f), 0.42f,
                        juce::Colour (0xfffff0dc), juce::Colour (0xffffb08a), juce::Colour (0xff6fb6ff), 0.26f);
        }

        // --- Kleine Sonne (oben links = die Lichtquelle der Szene) ---
        // Leuchtet selbst -> Leucht-Ebene (siehe paint()). In der Gonio-
        // Ansicht steht an ihrer Stelle ein kleiner Mond (siehe unten); die
        // Sonne blendet mit spaceBlend aus.
        gCur = &gGlow;
        if (spaceBlend > 0.01f)
        {
            float sx, sy;
            objectPos (0.10f, 0.14f, 0.15f, 53.0f, 0.4f, sx, sy);
            const float rs     = unitR * (uiTheme == 2 ? 0.34f : 0.28f) * distShrink;   // Pop: die groessere "Sunny"-Sonne (User)
            sunHitX = sx; sunHitY = sy; sunHitR = rs * 2.2f; sunDrawR = rs;   // Klickflaeche (siehe mouseUp), Leuchtkoerper selbst fuer den Hover
            const float sb = spaceBlend;   // Ein-/Ausblenden ueber die Alphas
            const float breath = 1.0f + 0.05f * std::sin (sceneT / 6.0f * juce::MathConstants<float>::twoPi);
            const float glowR  = rs * 3.4f * breath;

            // Sci-Fi: Neon-Sonne (violett/magenta mit weissem Kern und
            // duennem Leuchtring) passend zum Gravity-Planeten (User).
            const bool neon = themeSciFi();
            // Sonnenfarben je Theme: Modern kuehl weiss-blau (eher ein heller
            // Stern, User), Rose zart rosa, Sci-Fi Neon (violett / gruen / pink),
            // sonst Gold. Reihenfolge: Halo A/B/C, Koerper A/B, Neonringe.
            struct SunCols { juce::uint32 hA, hB, hC, bA, bB, ring; };
            static const SunCols sunByTheme[kThemeCount] = {
                // Die Kerne sind bewusst nicht mehr fast reines Weiss (User:
                // "leuchtende Objekte wie die Sonne sollten etwas sanfter
                // wirken") - ein paar Prozent reichen, um das Stechen zu nehmen.
                { 0xffdfe8ff, 0xff9fc4ff, 0xffb9d2ff, 0xffc3d6f8, 0xffdfe9fb, 0 },            // Modern
                { 0xffffe0a0, 0xffff9a3c, 0xffffc860, 0xffffb340, 0xfff4e0a6, 0 },            // Fireflies
                { 0xffffe0a0, 0xffff9a3c, 0xffffc860, 0xffffb340, 0xfffff0b0, 0 },            // Pop (bleibt knallig)
                { 0xffd48cff, 0xffff6fb1, 0xffb968ff, 0xffdcb8f6, 0xffe6d4f2, 0xffff6fb1 },   // Sci-Fi
                { 0xffffe0a0, 0xffff9a3c, 0xffffc860, 0xffffb340, 0xfff4e0a6, 0 },            // Day & Night
                { 0xffe2e5eb, 0xffb6bac4, 0xffcbced6, 0xffd4d7de, 0xffe6e8ee, 0 }             // Silver
            };
            const SunCols& sc = sunByTheme[juce::jlimit (0, kThemeCount - 1, uiTheme)];
            const juce::Colour hA (sc.hA), hB (sc.hB), hC (sc.hC), bA (sc.bA), bB (sc.bB);
            if (themeFlat())
            {
                // Flat (User): keine Verlaeufe - eine solide Scheibe und zwei
                // flache, konzentrische Ringe als "Strahlung".
                for (int k = 2; k >= 1; --k)
                {
                    const float rr = rs * (1.0f + 0.55f * (float) k);
                    g.setColour (hC.withAlpha ((0.16f - 0.06f * (float) (k - 1)) * sb));
                    g.fillEllipse (sx - rr, sy - rr, rr * 2.0f, rr * 2.0f);
                }
                g.setColour (bB.withAlpha (sb));
                g.fillEllipse (sx - rs, sy - rs, rs * 2.0f, rs * 2.0f);
                g.setColour (hB.withAlpha (0.85f * sb));
                g.drawEllipse (sx - rs, sy - rs, rs * 2.0f, rs * 2.0f, 1.4f);
            }
            else
            {
            juce::ColourGradient halo (hA.withAlpha (0.50f * sb), sx, sy, hB.withAlpha (0.0f), sx + glowR, sy, true);
            halo.addColour (0.30, hC.withAlpha (0.18f * sb));
            g.setGradientFill (halo);
            g.fillEllipse (sx - glowR, sy - glowR, glowR * 2.0f, glowR * 2.0f);

            juce::ColourGradient body (juce::Colours::white.withAlpha (sb), sx, sy, bA.withAlpha (sb), sx + rs, sy, true);
            body.addColour (0.55, bB.withAlpha (sb));
            g.setGradientFill (body);
            g.fillEllipse (sx - rs, sy - rs, rs * 2.0f, rs * 2.0f);
            }
            if (neon)   // mehrere, nach aussen schwaecher werdende Ringe (User)
            {
                for (int k = 0; k < 4; ++k)
                {
                    const float rr = rs * (1.30f + 0.28f * (float) k);
                    g.setColour (juce::Colour (sc.ring).withAlpha ((0.42f - 0.10f * (float) k) * sb));
                    g.drawEllipse (sx - rr, sy - rr, rr * 2.0f, rr * 2.0f, 1.0f);
                }
            }
            if (uiTheme == 2)   // Pop: die "Sunny"-Sonne - weiche, lange Strahlen statt Zacken (User)
            {
                for (int k = 0; k < 8; ++k)
                {
                    const float a0 = (float) k / 8.0f * juce::MathConstants<float>::twoPi + sceneT * 0.03f;
                    const float r0 = rs * 1.35f, r1 = rs * (k % 2 == 0 ? 3.0f : 2.4f);
                    const float x0 = sx + std::cos (a0) * r0, y0 = sy + std::sin (a0) * r0;
                    const float x1 = sx + std::cos (a0) * r1, y1 = sy + std::sin (a0) * r1;
                    juce::ColourGradient ray (hA.withAlpha (0.22f * sb), x0, y0, hA.withAlpha (0.0f), x1, y1, false);
                    g.setGradientFill (ray);
                    g.drawLine (x0, y0, x1, y1, 1.6f);
                }
            }
        }
        gCur = &gObject;

        // --- Erde (oben rechts) --- nur im Comic-Theme (User: "die beiden
        // blauen Planeten sehen nicht gut aus" - in Modern/Watercolor bleibt
        // nur der Ringplanet, dezent im Hintergrund).
        if (uiTheme == 2 || uiTheme == 0)
        {
            // Pop (User): Ringplanet weg, dafuer die Erde an seiner Stelle - gross.
            // Moon (User, Runde 20): die Erde klein und dezent oben rechts -
            // sie ist von Flat hierher gewandert, Flat bekommt dafuer die
            // Raumstation ("farblich passt die Erde dort nicht").
            const bool small = (uiTheme != 2);
            float ex, ey;
            if (small) objectPos (0.74f, 0.20f, 0.45f, 27.0f, 1.7f, ex, ey);
            else       objectPos (0.39f, 0.46f, 0.80f, 23.0f, 0.0f, ex, ey);
            const float r    = unitR * (small ? 0.40f : 0.92f) * distShrink;
            const float spin = sceneT * (juce::MathConstants<float>::twoPi / 70.0f);

            juce::ColourGradient ocean (juce::Colour (0xff3a7fe0), ex - r * 0.3f, ey - r * 0.3f,
                                        juce::Colour (0xff1c3f88), ex + r, ey + r, true);
            g.setGradientFill (ocean);
            g.fillEllipse (ex - r, ey - r, r * 2.0f, r * 2.0f);

            g.saveState();
            juce::Path clip;
            clip.addEllipse (ex - r, ey - r, r * 2.0f, r * 2.0f);
            g.reduceClipRegion (clip);
            // Kontinente: je zwei ueberlappende Flecken pro Landmasse.
            static const float landLon[8] = { -0.40f, -0.15f,  1.30f,  1.55f,  2.60f,  2.85f,  4.10f,  4.35f };
            static const float landLat[8] = {  0.35f,  0.05f, -0.10f, -0.45f,  0.50f,  0.15f, -0.30f,  0.05f };
            static const float landRel[8] = {  0.30f,  0.22f,  0.26f,  0.20f,  0.24f,  0.30f,  0.18f,  0.22f };
            const juce::Colour landA (0xff5f9c4c), landB (0xffa5945a);
            for (int i = 0; i < 8; ++i)
            {
                float sx, sy, sz;
                if (! project (landLon[i] + spin, landLat[i], sx, sy, sz))
                    continue;
                const float rr = landRel[i] * r;
                const juce::Colour c = (i % 4 < 2) ? landA : landB;
                g.setColour (c.withAlpha (0.90f * juce::jmin (1.0f, sz * 3.0f)));
                g.fillEllipse (ex + sx * r - rr * sz, ey - sy * r - rr * 0.75f, rr * sz * 2.0f, rr * 1.5f);
            }
            // Wolken: schneller als die Oberflaeche, halbtransparent.
            static const float cloudLon[5] = { 0.30f, 1.90f, 3.10f, 4.60f, 5.60f };
            static const float cloudLat[5] = { -0.55f, 0.60f, 0.10f, -0.20f, 0.75f };
            for (int i = 0; i < 5; ++i)
            {
                float sx, sy, sz;
                if (! project (cloudLon[i] + spin * 1.18f, cloudLat[i], sx, sy, sz))
                    continue;
                const float rr = r * 0.22f;
                g.setColour (juce::Colours::white.withAlpha (0.32f * juce::jmin (1.0f, sz * 3.0f)));
                g.fillEllipse (ex + sx * r - rr * sz * 1.6f, ey - sy * r - rr * 0.45f, rr * sz * 3.2f, rr * 0.9f);
            }
            g.restoreState();

            sphereShade (ex, ey, r, 0.55f, 0.30f);
            // Atmosphaerensaum.
            g.setColour (juce::Colour (0xff8fd0ff).withAlpha (0.35f));
            g.drawEllipse (ex - r - 0.6f, ey - r - 0.6f, r * 2.0f + 1.2f, r * 2.0f + 1.2f, 1.2f);
            g.setColour (juce::Colour (0xff8fd0ff).withAlpha (0.12f));
            g.drawEllipse (ex - r - 1.8f, ey - r - 1.8f, r * 2.0f + 3.6f, r * 2.0f + 3.6f, 1.8f);
            if (small)   // Moon: dezent - abdunkeln und entsaettigen (User: "aber dezent!")
            {
                g.setColour (juce::Colour (0xff15161a).withAlpha (0.50f));
                g.fillEllipse (ex - r - 2.0f, ey - r - 2.0f, r * 2.0f + 4.0f, r * 2.0f + 4.0f);
            }
        }

        // --- Blauer Baenderplanet (rechts vom Ringplaneten) ---
        // Neu (User): keine harten waagerechten Streifen mehr, sondern
        // gebogene Baender mit weichen Uebergaengen, und eine kuehle
        // Blaupalette, damit er sich klar vom sandfarbenen Ringplaneten
        // absetzt. Die Baender sind Paths mit gewoelbten Ober-/Unterkanten
        // (Kruemmung zum Pol hin staerker) und senkrechtem Farbverlauf zum
        // jeweiligen Nachbarband.
        if (uiTheme == 2)
        {
            float jx, jy;
            objectPos (0.75f, 0.25f, 0.62f, 31.0f, 4.3f, jx, jy);   // weiter schraeg rechts oben (User)
            const float r    = unitR * 0.62f * distShrink;
            const float spin = sceneT * (juce::MathConstants<float>::twoPi / 58.0f);

            g.setColour (juce::Colour (0xff3b6fd8));
            g.fillEllipse (jx - r, jy - r, r * 2.0f, r * 2.0f);

            g.saveState();
            juce::Path clip;
            clip.addEllipse (jx - r, jy - r, r * 2.0f, r * 2.0f);
            g.reduceClipRegion (clip);
            // Bandgrenzen (Breitengrade) und Farben je Band (Sued -> Nord).
            static const float edgeLat[9] = { -1.57f, -1.05f, -0.70f, -0.38f, -0.10f, 0.18f, 0.50f, 0.85f, 1.57f };
            static const juce::uint32 bandCol[8] = { 0xff2e5cc0, 0xff5a8ae8, 0xff3364cc, 0xff7fa8f0,
                                                     0xff3d70d6, 0xff6c98ec, 0xff2f5fc4, 0xff5583e0 };
            auto edgeY = [&] (float lat) { return jy - std::sin (lat) * r; };
            auto bow   = [&] (float lat) { return -std::sin (lat) * r * 0.16f; };   // Woelbung: an den Raendern zum Pol hin
            for (int i = 0; i < 8; ++i)
            {
                const float y0 = edgeY (edgeLat[i + 1]), y1 = edgeY (edgeLat[i]);
                const float c0 = bow (edgeLat[i + 1]),  c1 = bow (edgeLat[i]);
                juce::Path band;
                band.startNewSubPath (jx - r * 1.05f, y0 - c0);
                band.quadraticTo (jx, y0 + c0, jx + r * 1.05f, y0 - c0);
                band.lineTo (jx + r * 1.05f, y1 - c1);
                band.quadraticTo (jx, y1 + c1, jx - r * 1.05f, y1 - c1);
                band.closeSubPath();
                const juce::Colour cThis (bandCol[i]);
                const juce::Colour cPrev (bandCol[juce::jmax (0, i - 1)]);
                const juce::Colour cNext (bandCol[juce::jmin (7, i + 1)]);
                juce::ColourGradient grad (cThis.interpolatedWith (cNext, 0.5f), jx, y0,
                                           cThis.interpolatedWith (cPrev, 0.5f), jx, y1, false);
                grad.addColour (0.5, cThis);
                g.setGradientFill (grad);
                g.fillPath (band);
            }
            // Dunkler Sturmfleck, wandert mit der Rotation.
            {
                float sx, sy, sz;
                if (project (spin + 1.0f, -0.30f, sx, sy, sz))
                {
                    const float rr = r * 0.13f;
                    g.setColour (juce::Colour (0xff1d3a80).withAlpha (0.80f * juce::jmin (1.0f, sz * 3.0f)));
                    g.fillEllipse (jx + sx * r - rr * 1.5f * sz, jy - sy * r - rr, rr * 3.0f * sz, rr * 2.0f);
                }
                // Heller Wolkenwirbel
                if (project (spin + 3.2f, 0.42f, sx, sy, sz))
                {
                    const float rr = r * 0.09f;
                    g.setColour (juce::Colour (0xffd8e8ff).withAlpha (0.55f * juce::jmin (1.0f, sz * 3.0f)));
                    g.fillEllipse (jx + sx * r - rr * 1.6f * sz, jy - sy * r - rr, rr * 3.2f * sz, rr * 2.0f);
                }
            }
            g.restoreState();
            sphereShade (jx, jy, r, 0.60f, 0.24f);
        }

        // --- Sci-Fi: Neon-Planet (dunkle violette Kugel, leuchtende Baender,
        // duenner Neonring) statt des gezeichneten Ringplaneten (User) ---
        if (themeSciFi())
        {
            float px, py;
            objectPos (0.39f, 0.46f, 0.80f, 23.0f, 0.0f, px, py);
            const float r = unitR * 0.62f * distShrink;
            // Neonfarben je Sci-Fi-Variante: Glow, Koerper hell/dunkel, Ring A/B.
            struct NeonCols { juce::uint32 glow, bodyA, bodyB, ringA, ringB; };
            const NeonCols nc { 0xffb968ff, 0xff2a1b4a, 0xff0c0718, 0xffff6fb1, 0xffd48cff };   // dunkler (User)
            // Ring als flache Ellipse, leicht gekippt; hintere Haelfte VOR dem
            // Koerper gezeichnet, vordere Haelfte DANACH und deckend (User: der
            // Planet schien durch den Ring, besonders mit Dim).
            const float ringRX = r * 1.62f, ringRY = r * 0.46f;
            const juce::AffineTransform ringT = juce::AffineTransform::rotation (-0.16f, px, py);
            auto neonRing = [&] (float alphaMult)
            {
                juce::Path ringP;
                ringP.addEllipse (px - ringRX, py - ringRY, ringRX * 2.0f, ringRY * 2.0f);
                g.setColour (juce::Colour (0xff07050c).withAlpha (alphaMult));          // deckender dunkler Kern - nichts scheint durch
                g.strokePath (ringP, juce::PathStrokeType (3.4f), ringT);
                g.setColour (juce::Colour (nc.ringA).withAlpha (alphaMult));
                g.strokePath (ringP, juce::PathStrokeType (1.7f), ringT);
                juce::Path ring2;
                ring2.addEllipse (px - ringRX * 1.16f, py - ringRY * 1.16f, ringRX * 2.32f, ringRY * 2.32f);
                g.setColour (juce::Colour (nc.ringB).withAlpha (0.55f * alphaMult));
                g.strokePath (ring2, juce::PathStrokeType (1.0f), ringT);
            };
            juce::ColourGradient glow (juce::Colour (nc.glow).withAlpha (0.15f), px, py, juce::Colour (nc.glow).withAlpha (0.0f), px + r * 1.9f, py, true);
            g.setGradientFill (glow);
            g.fillEllipse (px - r * 1.9f, py - r * 1.9f, r * 3.8f, r * 3.8f);
            {
                g.saveState();   // hintere Ringhaelfte (obere Haelfte in Ringkoordinaten)
                juce::Path back;
                back.addRectangle (px - ringRX * 1.3f, py - ringRY * 1.3f - 4.0f, ringRX * 2.6f, ringRY * 1.3f + 4.0f);
                g.reduceClipRegion (back, ringT);
                neonRing (0.80f);
                g.restoreState();
            }
            juce::ColourGradient body (juce::Colour (nc.bodyA), px - r * 0.35f, py - r * 0.35f, juce::Colour (nc.bodyB), px + r, py + r, true);
            g.setGradientFill (body);
            g.fillEllipse (px - r, py - r, r * 2.0f, r * 2.0f);
            static const Band nBands[] = {
                { -1.10f, -0.85f, 0xffd48cff, 0.40f }, { -0.55f, -0.42f, 0xffff6fb1, 0.32f },
                { -0.15f,  0.05f, 0xffe9c8ff, 0.42f }, {  0.35f,  0.48f, 0xffb968ff, 0.36f },
                {  0.80f,  0.95f, 0xffff6fb1, 0.28f }
            };   // Baender gedaempft (User: Planeten dunkler)
            drawBands (px, py, r, nBands, (int) (sizeof (nBands) / sizeof (nBands[0])));
            {
                g.saveState();
                juce::Path discClip; discClip.addEllipse (px - r, py - r, r * 2.0f, r * 2.0f);
                g.reduceClipRegion (discClip);   // Schatten NUR auf der Kugel (vorher als "zweiter Planet" sichtbar)
                juce::ColourGradient shd (juce::Colours::transparentBlack, px - r * 0.2f, py, juce::Colours::black.withAlpha (0.55f), px + r, py, false);
                g.setGradientFill (shd);
                g.fillEllipse (px - r, py - r, r * 2.0f, r * 2.0f);
                g.restoreState();
            }
            {
                // Ringschatten auf der Kugel (schmales dunkles Band).
                g.saveState();
                juce::Path discClip; discClip.addEllipse (px - r, py - r, r * 2.0f, r * 2.0f);
                g.reduceClipRegion (discClip);
                juce::Path shadowRing;
                shadowRing.addEllipse (px - ringRX, py - ringRY - r * 0.10f, ringRX * 2.0f, ringRY * 2.0f);
                g.setColour (juce::Colours::black.withAlpha (0.30f));
                g.strokePath (shadowRing, juce::PathStrokeType (2.6f), ringT);
                g.restoreState();
            }
            {
                g.saveState();   // vordere Ringhaelfte, voll deckend
                juce::Path front;
                front.addRectangle (px - ringRX * 1.3f, py, ringRX * 2.6f, ringRY * 1.3f + 4.0f);
                g.reduceClipRegion (front, ringT);
                neonRing (1.0f);
                g.restoreState();
            }

            // --- UFO (User): kleine Untertasse oben rechts, schwebt leicht,
            // Positionslichter blinken, schwacher Lichtkegel nach unten. ---
            {
                float ux, uy;
                objectPos (0.80f, 0.19f, 0.30f, 17.0f, 2.0f, ux, uy);   // etwas Richtung Mitte (User)
                uy += std::sin (sceneT * 1.1f) * unitR * 0.06f;   // Schweben
                const float ur = unitR * 0.34f * distShrink;
                const juce::Colour edge (nc.ringB), light (nc.ringA);
                // Lichtkegel (sehr dezent)
                {
                    juce::Path cone;
                    cone.addTriangle (ux - ur * 0.35f, uy + ur * 0.15f, ux + ur * 0.35f, uy + ur * 0.15f, ux + ur * 0.15f, uy + ur * 1.9f);
                    cone.addTriangle (ux - ur * 0.35f, uy + ur * 0.15f, ux + ur * 0.15f, uy + ur * 1.9f, ux - ur * 0.75f, uy + ur * 1.9f);
                    juce::ColourGradient cg (juce::Colour (nc.glow).withAlpha (0.16f), ux, uy, juce::Colour (nc.glow).withAlpha (0.0f), ux, uy + ur * 1.9f, false);
                    g.setGradientFill (cg);
                    g.fillPath (cone);
                }
                // Kuppel
                g.setColour (juce::Colour (0xff141020));
                g.fillEllipse (ux - ur * 0.42f, uy - ur * 0.62f, ur * 0.84f, ur * 0.80f);
                juce::ColourGradient dome (juce::Colour (0xffffffff).withAlpha (0.35f), ux - ur * 0.15f, uy - ur * 0.50f, light.withAlpha (0.10f), ux + ur * 0.3f, uy, true);
                g.setGradientFill (dome);
                g.fillEllipse (ux - ur * 0.42f, uy - ur * 0.62f, ur * 0.84f, ur * 0.80f);
                g.setColour (edge.withAlpha (0.75f));
                g.drawEllipse (ux - ur * 0.42f, uy - ur * 0.62f, ur * 0.84f, ur * 0.80f, 1.0f);
                // Scheibe (Untertasse)
                juce::ColourGradient hull (juce::Colour (0xff2c2a3e), ux, uy - ur * 0.26f, juce::Colour (0xff0d0c16), ux, uy + ur * 0.26f, false);
                g.setGradientFill (hull);
                g.fillEllipse (ux - ur, uy - ur * 0.26f, ur * 2.0f, ur * 0.52f);
                g.setColour (edge.withAlpha (0.95f));
                g.drawEllipse (ux - ur, uy - ur * 0.26f, ur * 2.0f, ur * 0.52f, 1.2f);
                // Positionslichter (laufendes Blinken)
                for (int k = 0; k < 4; ++k)
                {
                    const float ph = std::fmod (sceneT * 1.6f + (float) k * 0.25f, 1.0f);
                    const float a  = 0.25f + 0.75f * juce::jmax (0.0f, std::sin (ph * juce::MathConstants<float>::twoPi));
                    const float lx = ux - ur * 0.72f + ur * 0.48f * (float) k;
                    const float ly = uy + ur * 0.16f;
                    g.setColour (light.withAlpha (a));
                    g.fillEllipse (lx - 1.4f, ly - 1.4f, 2.8f, 2.8f);
                }
            }
        }
        else if (themeModern())
        {
            // Modern / Day & Night / Moon: minimaler Planet - dunkle Kugel, nur
            // eine duenne beleuchtete Sichel, kein Ring (User). Day & Night:
            // klein und dezent oben rechts (User), Moon: hat stattdessen die Erde.
            if (uiTheme != 5)
            {
            float px, py;
            if (uiTheme == 4) objectPos (0.78f, 0.20f, 0.45f, 27.0f, 1.7f, px, py);   // Richtung Mitte (User)
            else              objectPos (0.39f, 0.46f, 0.80f, 23.0f, 0.0f, px, py);
            const float r = unitR * (uiTheme == 4 ? 0.40f : 0.56f) * distShrink;
            const juce::Colour rim (uiTheme == 4 ? 0xffffe0a0 : 0xffcfe6ff);   // Day & Night warm, Modern kuehl
            juce::ColourGradient body (juce::Colour (0xff1a1d26), px, py, juce::Colour (0xff07080c), px + r, py + r, true);
            g.setGradientFill (body);
            g.fillEllipse (px - r, py - r, r * 2.0f, r * 2.0f);
            g.saveState();
            juce::Path disc; disc.addEllipse (px - r, py - r, r * 2.0f, r * 2.0f);
            g.reduceClipRegion (disc);
            juce::ColourGradient lit (rim.withAlpha (0.55f), px - r * 0.95f, py - r * 0.55f, rim.withAlpha (0.0f), px - r * 0.25f, py + r * 0.15f, false);
            g.setGradientFill (lit);
            g.fillEllipse (px - r, py - r, r * 2.0f, r * 2.0f);
            g.restoreState();
            g.setColour (rim.withAlpha (0.65f));
            juce::Path crescent;
            crescent.addCentredArc (px, py, r - 0.6f, r - 0.6f, 0.0f, -2.6f, -0.9f, true);
            g.strokePath (crescent, juce::PathStrokeType (1.2f));
            }

            // ===== ZWEITES GROSSES OBJEKT (User: "mindestens 2 von diesen
            // grossen Objekten, sonst ist das doof") =====
            // Es muss kein Planet sein - ein Satellit oder eine Raumstation
            // passt genauso, Hauptsache die Farben passen zum Theme.
            //   Moon        -> Raumstation oben rechts
            //   Day & Night -> Satellit links der Mitte
            //   Flat        -> flacher Planet links der Mitte

            // --- Raumstation: Ring mit Nabe, vier Speichen, Fensterreihe ---
            auto drawStation = [&] (float cx, float cy, float rr, juce::Colour hull, juce::Colour edge, juce::Colour lit)
            {
                const float ringRX = rr, ringRY = rr * 0.42f;
                const juce::AffineTransform tilt = juce::AffineTransform::rotation (-0.30f, cx, cy);
                const float spin = sceneT * 0.06f;
                juce::Path ring;
                ring.addEllipse (cx - ringRX, cy - ringRY, ringRX * 2.0f, ringRY * 2.0f);
                g.setColour (hull);
                g.strokePath (ring, juce::PathStrokeType (rr * 0.22f), tilt);
                g.setColour (edge.withAlpha (0.85f));
                g.strokePath (ring, juce::PathStrokeType (1.1f), tilt);
                // Speichen zur Nabe
                for (int k = 0; k < 4; ++k)
                {
                    const float a = (float) k / 4.0f * juce::MathConstants<float>::twoPi + 0.4f;
                    const juce::Point<float> p = juce::Point<float> (cx + std::cos (a) * ringRX, cy + std::sin (a) * ringRY).transformedBy (tilt);
                    g.setColour (hull.brighter (0.10f));
                    g.drawLine (cx, cy, p.x, p.y, rr * 0.055f);
                }
                // Nabe
                g.setColour (hull.darker (0.25f));
                g.fillEllipse (cx - rr * 0.26f, cy - rr * 0.26f, rr * 0.52f, rr * 0.52f);
                g.setColour (edge.withAlpha (0.9f));
                g.drawEllipse (cx - rr * 0.26f, cy - rr * 0.26f, rr * 0.52f, rr * 0.52f, 1.0f);
                // Fenster laufen langsam mit der Drehung mit
                for (int k = 0; k < 10; ++k)
                {
                    const float a = (float) k / 10.0f * juce::MathConstants<float>::twoPi + spin;
                    const juce::Point<float> p = juce::Point<float> (cx + std::cos (a) * ringRX, cy + std::sin (a) * ringRY).transformedBy (tilt);
                    const float fade = 0.35f + 0.65f * (0.5f + 0.5f * std::sin (a * 3.0f + sceneT * 0.5f));
                    g.setColour (lit.withAlpha (0.75f * fade));
                    g.fillEllipse (p.x - rr * 0.035f, p.y - rr * 0.035f, rr * 0.07f, rr * 0.07f);
                }
            };

            // --- Satellit: Koerper, zwei Solarsegel, kleine Antenne ---
            auto drawSatellite = [&] (float cx, float cy, float rr, juce::Colour hull, juce::Colour panel, juce::Colour lit)
            {
                const float drift = std::sin (sceneT * 0.22f) * rr * 0.06f;
                cy += drift;
                const juce::AffineTransform tilt = juce::AffineTransform::rotation (-0.22f, cx, cy);
                g.saveState();
                g.addTransform (tilt);
                // Solarsegel links und rechts
                for (int sgn = -1; sgn <= 1; sgn += 2)
                {
                    const float x0 = cx + (float) sgn * rr * 0.34f;
                    const float wdt = rr * 0.78f, hgt = rr * 0.52f;
                    const juce::Rectangle<float> pr (sgn < 0 ? x0 - wdt : x0, cy - hgt * 0.5f, wdt, hgt);
                    g.setColour (panel);
                    g.fillRect (pr);
                    g.setColour (panel.brighter (0.45f).withAlpha (0.55f));
                    for (int k = 1; k < 4; ++k)
                        g.drawLine (pr.getX() + pr.getWidth() * (float) k / 4.0f, pr.getY(),
                                    pr.getX() + pr.getWidth() * (float) k / 4.0f, pr.getBottom(), 0.8f);
                    g.drawLine (pr.getX(), pr.getCentreY(), pr.getRight(), pr.getCentreY(), 0.8f);
                    g.setColour (hull.withAlpha (0.9f));
                    g.drawRect (pr, 1.0f);
                    // Mast zum Koerper
                    g.drawLine (cx + (float) sgn * rr * 0.18f, cy, x0, cy, rr * 0.05f);
                }
                // Koerper
                const juce::Rectangle<float> body (cx - rr * 0.20f, cy - rr * 0.30f, rr * 0.40f, rr * 0.60f);
                g.setColour (hull);
                g.fillRoundedRectangle (body, rr * 0.08f);
                g.setColour (hull.brighter (0.35f));
                g.drawRoundedRectangle (body, rr * 0.08f, 1.0f);
                // Parabolantenne nach unten links
                g.setColour (hull.brighter (0.15f));
                g.drawLine (cx, body.getBottom(), cx - rr * 0.22f, body.getBottom() + rr * 0.22f, rr * 0.04f);
                g.setColour (hull.brighter (0.25f));
                g.fillEllipse (cx - rr * 0.34f, body.getBottom() + rr * 0.14f, rr * 0.26f, rr * 0.18f);
                // Blinklicht
                const float blink = 0.25f + 0.75f * juce::jmax (0.0f, std::sin (sceneT * 1.9f));
                g.setColour (lit.withAlpha (blink));
                g.fillEllipse (body.getRight() - rr * 0.05f, body.getY() - rr * 0.02f, rr * 0.09f, rr * 0.09f);
                g.restoreState();
            };

            // --- Flacher Planet: solide Scheibe, solide Baender, solider Ring ---
            auto drawFlatPlanet = [&] (float cx, float cy, float rr, juce::Colour base, juce::Colour band, juce::Colour ringCol)
            {
                const float ringRX = rr * 1.70f, ringRY = rr * 0.44f;
                const juce::AffineTransform tilt = juce::AffineTransform::rotation (-0.20f, cx, cy);
                juce::Path ring;
                ring.addEllipse (cx - ringRX, cy - ringRY, ringRX * 2.0f, ringRY * 2.0f);
                g.setColour (ringCol.withAlpha (0.55f));       // hintere Haelfte schwaecher
                g.strokePath (ring, juce::PathStrokeType (rr * 0.13f), tilt);
                g.setColour (base);
                g.fillEllipse (cx - rr, cy - rr, rr * 2.0f, rr * 2.0f);
                g.saveState();
                juce::Path disc; disc.addEllipse (cx - rr, cy - rr, rr * 2.0f, rr * 2.0f);
                g.reduceClipRegion (disc);
                g.setColour (band);
                g.fillRect (cx - rr, cy - rr * 0.52f, rr * 2.0f, rr * 0.26f);
                g.fillRect (cx - rr, cy + rr * 0.14f, rr * 2.0f, rr * 0.40f);
                g.setColour (base.darker (0.35f));
                g.fillRect (cx - rr, cy - rr * 0.10f, rr * 2.0f, rr * 0.16f);
                g.restoreState();
                g.setColour (ringCol.brighter (0.25f));
                g.drawEllipse (cx - rr, cy - rr, rr * 2.0f, rr * 2.0f, 1.2f);
                g.saveState();                                  // vordere Ringhaelfte deckend
                juce::Path front;
                front.addRectangle (cx - ringRX * 1.3f, cy, ringRX * 2.6f, ringRY * 1.3f + 4.0f);
                g.reduceClipRegion (front, tilt);
                g.setColour (ringCol);
                g.strokePath (ring, juce::PathStrokeType (rr * 0.13f), tilt);
                g.restoreState();
            };

            if (themeDayNight())
            {
                // Satellit weiter aus der Mitte heraus (User).
                float ox, oy;
                objectPos (0.27f, 0.52f, 0.72f, 23.0f, 0.0f, ox, oy);
                drawSatellite (ox, oy, unitR * 0.62f * distShrink,
                               juce::Colour (0xff6b6152), juce::Colour (0xff23405e), juce::Colour (0xffe0b98a));
            }
            else if (themeFlat())
            {
                // Flat hat jetzt ZWEI flache Objekte: den Ringplaneten links
                // der Mitte und - statt der Erde, die farblich nicht passte
                // (User) - eine Raumstation oben rechts in Silberblau.
                float ox, oy;
                objectPos (0.36f, 0.46f, 0.78f, 23.0f, 0.0f, ox, oy);
                drawFlatPlanet (ox, oy, unitR * 0.58f * distShrink,
                                juce::Colour (0xff4a5160), juce::Colour (0xff5d6675), juce::Colour (0xffb9c2d0));
                objectPos (0.76f, 0.20f, 0.45f, 27.0f, 1.7f, ox, oy);
                drawStation (ox, oy, unitR * 0.40f * distShrink,
                             juce::Colour (0xff3a3f4a), juce::Colour (0xffd8dee8), juce::Colour (0xffb9c2d0));
            }
        }
        else if (themeWater())   // Ringplanet nur noch Night/Dark Night (Pop: Erde, User)
        {
            float px, py;
            objectPos (0.39f, 0.46f, 0.80f, 23.0f, 0.0f, px, py);
            const float r    = unitR * 0.70f * distShrink;
            const float spin = sceneT * (juce::MathConstants<float>::twoPi / 66.0f);
            const float ringRX = r * 2.15f, ringRY = r * 0.60f;
            const juce::AffineTransform ringT = juce::AffineTransform::rotation (-0.22f, px, py);

            auto fillRings = [&] (float alphaMult)
            {
                struct RingBand { float outer, inner; juce::uint32 col; float alpha; };
                static const RingBand rb[] = {
                    { 1.00f, 0.87f, 0xffd9c8a6, 0.50f },   // A-Ring
                    { 0.83f, 0.62f, 0xffe6d6b4, 0.72f },   // B-Ring (Cassini-Teilung dazwischen)
                    { 0.62f, 0.50f, 0xffcbb896, 0.26f }    // C-Ring
                };
                for (const auto& b : rb)
                {
                    juce::Path annulus;
                    annulus.addEllipse (px - ringRX * b.outer, py - ringRY * b.outer, ringRX * b.outer * 2.0f, ringRY * b.outer * 2.0f);
                    annulus.addEllipse (px - ringRX * b.inner, py - ringRY * b.inner, ringRX * b.inner * 2.0f, ringRY * b.inner * 2.0f);
                    annulus.setUsingNonZeroWinding (false);
                    g.setColour (tintPlanet (juce::Colour (b.col)).withAlpha (juce::jmin (1.0f, b.alpha * alphaMult)));
                    g.fillPath (annulus, ringT);
                }
            };

            fillRings (0.70f);   // hintere Haelfte (wird vom Planeten teilweise verdeckt)

            g.setColour (tintPlanet (juce::Colour (0xffd9c39c)));
            g.fillEllipse (px - r, py - r, r * 2.0f, r * 2.0f);
            static const Band sBands[] = {
                { -1.57f, -0.95f, 0xffc4a878, 0.55f }, { -0.95f, -0.55f, 0xffe4d2ae, 0.55f },
                { -0.55f, -0.20f, 0xffcdb387, 0.55f }, { -0.20f,  0.15f, 0xffe9dab8, 0.55f },
                {  0.15f,  0.55f, 0xffcbb185, 0.55f }, {  0.55f,  0.95f, 0xffe0cda9, 0.55f },
                {  0.95f,  1.57f, 0xffbfa374, 0.55f }
            };
            drawBands (px, py, r, sBands, (int) (sizeof (sBands) / sizeof (sBands[0])));
            // Ein heller Sturmfleck als Rotationsmarke.
            {
                float sx, sy, sz;
                if (project (spin, 0.30f, sx, sy, sz))
                {
                    const float rr = r * 0.09f;
                    g.setColour (tintPlanet (juce::Colour (0xfff5ead0)).withAlpha (0.70f * juce::jmin (1.0f, sz * 3.0f)));
                    g.fillEllipse (px + sx * r - rr * 1.4f * sz, py - sy * r - rr, rr * 2.8f * sz, rr * 2.0f);
                }
            }
            // Ringschatten auf der Kugel (schmales dunkles Band knapp unter dem Aequator).
            {
                g.saveState();
                juce::Path clip;
                clip.addEllipse (px - r, py - r, r * 2.0f, r * 2.0f);
                g.reduceClipRegion (clip);
                g.setColour (juce::Colours::black.withAlpha (0.22f));
                juce::Path shadow;
                shadow.addEllipse (px - r * 1.3f, py - r * 0.36f, r * 2.6f, r * 0.62f);
                shadow.addEllipse (px - r * 1.3f, py - r * 0.26f, r * 2.6f, r * 0.42f);
                shadow.setUsingNonZeroWinding (false);
                g.fillPath (shadow, ringT);
                g.restoreState();
            }
            sphereShade (px, py, r, 0.55f, 0.24f);

            // Vordere Ringhaelfte darueber (untere Haelfte in Ringkoordinaten).
            g.saveState();
            juce::Path front;
            front.addRectangle (px - ringRX - 2.0f, py, ringRX * 2.0f + 4.0f, ringRY + 4.0f);
            g.reduceClipRegion (front, ringT);
            fillRings (1.35f);   // vordere Haelfte deckender, damit sie klar VOR dem Planeten liegt (User)
            g.restoreState();
        }
        }
    }

    // ===== STERNENZERSTOERER (zweites Easter Egg) =====
    // Zieht alle paar Minuten sehr langsam (~40 s) und dunkel durchs obere
    // Drittel. Reine Silhouette: flacher Keil, kleiner Bruecken-Turm, zwei
    // schwache Triebwerkspunkte. Liegt in der Objekt-Ebene (max. 80 %).
    {
        const float dtScene = (1.0f / 30.0f) * motionScale * sceneSpeedMul();
        if (destroyerT < 0.0f)
        {
            destroyerWait -= dtScene;
            if (destroyerWait <= 0.0f)
            {
                destroyerT = 0.0f;
                destroyerY = 0.18f + bgRandom.nextFloat() * 0.30f;
                destroyerLeftToRight = bgRandom.nextBool();
            }
        }
        else
        {
            destroyerT += dtScene / 40.0f;
            if (destroyerT >= 1.0f)
            {
                destroyerT = -1.0f;
                destroyerWait = 120.0f + bgRandom.nextFloat() * 120.0f;   // 2-4 Minuten Pause
            }
            else
            {
                const float u  = destroyerLeftToRight ? destroyerT : 1.0f - destroyerT;
                const float dx = (float) w * (-0.15f + u * 1.30f);
                // Leichter Bogen statt starrer Horizontale (User); Richtung
                // des Bogens haengt von der gewuerfelten Hoehe ab, der Rumpf
                // neigt sich passend zur Flugbahn.
                const float arcSign = destroyerY > 0.33f ? -1.0f : 1.0f;
                const float arcAmp  = (float) h * 0.05f;
                const float dy = (float) h * destroyerY + arcSign * arcAmp * std::sin (u * juce::MathConstants<float>::pi);
                const float slope = arcSign * arcAmp * juce::MathConstants<float>::pi * std::cos (u * juce::MathConstants<float>::pi) / ((float) w * 1.30f);
                const float len = (float) juce::jmin (w, h) * 0.20f;
                const float dir = destroyerLeftToRight ? 1.0f : -1.0f;
                // Rand-Ein-/Ausblenden
                const float edge = juce::jlimit (0.0f, 1.0f, juce::jmin (destroyerT, 1.0f - destroyerT) * 8.0f);

                g.saveState();
                g.addTransform (juce::AffineTransform::rotation (std::atan (slope), dx, dy));
                juce::Path hull;
                hull.startNewSubPath (dx + dir * len * 0.55f, dy);                 // Bug (Spitze)
                hull.lineTo (dx - dir * len * 0.45f, dy - len * 0.10f);            // Heck oben
                hull.lineTo (dx - dir * len * 0.45f, dy + len * 0.12f);            // Heck unten
                hull.closeSubPath();
                g.setColour (juce::Colour (0xff9aa0ab).withAlpha (0.55f * edge));
                g.fillPath (hull);
                // Bruecken-Turm
                g.setColour (juce::Colour (0xffb4b9c4).withAlpha (0.55f * edge));
                g.fillRect (dx - dir * len * 0.22f - len * 0.03f, dy - len * 0.17f, len * 0.06f, len * 0.09f);
                g.fillRect (dx - dir * len * 0.22f - len * 0.015f, dy - len * 0.21f, len * 0.03f, len * 0.05f);
                // Triebwerke
                g.setColour (juce::Colour (0xff8fd0ff).withAlpha (0.45f * edge));
                g.fillEllipse (dx - dir * len * 0.46f - 1.2f, dy - len * 0.04f - 1.2f, 2.4f, 2.4f);
                g.fillEllipse (dx - dir * len * 0.46f - 1.2f, dy + len * 0.05f - 1.2f, 2.4f, 2.4f);
                g.restoreState();
            }
        }
    }

    gCur = &gGravity;
    // ===== HIDDEN EGG: Raumschiff =====
    // Klein, halbdunkel, einmal quer durchs Bild in gut drei Sekunden, mit
    // leichtem Bogen und blauem Triebwerksschweif. Bewusst zurueckhaltend
    // gezeichnet (Silhouette statt Details) - es soll ein Augenzwinkern
    // sein, kein Sprite aus einem Spiel.
    if (eggT >= 0.0f)
    {
        eggT += (1.0f / 30.0f) / 3.4f * motionScale;
        if (eggT >= 1.0f)
            eggT = -1.0f;
        else
        {
            const float u = eggT;
            const float ex = cx + (-1.3f + u * 2.6f) * bgRadius;
            const float ey = cy + (0.25f - 0.35f * std::sin (u * juce::MathConstants<float>::pi)) * bgRadius;
            const float sz = bgRadius * 0.034f;
            const float tilt = -0.18f * std::cos (u * juce::MathConstants<float>::pi); // Nase folgt der Bahn
            const float edgeFade = juce::jlimit (0.0f, 1.0f, juce::jmin (u, 1.0f - u) / 0.12f);

            juce::Path ship;
            ship.startNewSubPath (sz * 1.6f, 0.0f);            // Nase
            ship.lineTo (-sz * 0.6f, -sz * 0.75f);              // Fluegel oben
            ship.lineTo (-sz * 1.2f, -sz * 0.35f);
            ship.lineTo (-sz * 1.0f, 0.0f);
            ship.lineTo (-sz * 1.2f,  sz * 0.35f);
            ship.lineTo (-sz * 0.6f,  sz * 0.75f);              // Fluegel unten
            ship.closeSubPath();
            ship.applyTransform (juce::AffineTransform::rotation (tilt).translated (ex, ey));

            // Triebwerksschweif
            const float tx = ex - std::cos (tilt) * sz * 1.0f;
            const float ty = ey - std::sin (tilt) * sz * 1.0f;
            const float tailLen = sz * 5.0f;
            juce::ColourGradient tg (juce::Colour (0xff6bb8ff).withAlpha (0.55f * edgeFade), tx, ty,
                                     juce::Colour (0xff6bb8ff).withAlpha (0.0f),
                                     tx - std::cos (tilt) * tailLen, ty - std::sin (tilt) * tailLen, false);
            g.setGradientFill (tg);
            g.drawLine (tx, ty, tx - std::cos (tilt) * tailLen, ty - std::sin (tilt) * tailLen, sz * 0.5f);

            g.setColour ((themeSciFi() ? juce::Colour (0xff0b0912) : juce::Colour (0xff2a2e38)).withAlpha (0.95f * edgeFade));   // Sci-Fi: fast schwarz (User)
            g.fillPath (ship);
            g.setColour ((themeSciFi() ? juce::Colour (0xff4a4468) : juce::Colour (0xffb7bac2)).withAlpha (0.85f * edgeFade));
            g.strokePath (ship, juce::PathStrokeType (1.0f));
            // Cockpit-Licht
            g.setColour (juce::Colour (0xff5be3c7).withAlpha (0.9f * edgeFade));
            g.fillEllipse (ex + std::cos (tilt) * sz * 0.5f - 1.3f, ey + std::sin (tilt) * sz * 0.5f - 1.3f, 2.6f, 2.6f);
        }
    }

    gCur = &gShoot;
    // ===== FLOW (HYPERDRIVE): tempo-synchrone Sternschnuppen =====
    // User-Vorgabe: "Flow, Pulse, Speed, Sync - letztlich ist nur der finale
    // 'Flow' Output Wert relevant." Genau so umgesetzt: die Ausloesung haengt
    // NICHT an den vier Reglern einzeln, sondern am tatsaechlichen
    // Pan-Ausgabewert des Auto-Pan-LFO. Bei jedem Nulldurchgang nach oben
    // faellt genau eine Sternschnuppe. Dadurch ist sie automatisch
    // tempo-synchron, sobald Sync aktiv ist - ohne dass hier irgendetwas
    // ueber BPM oder Taktarten wissen muss. Pulse und Speed wirken ebenfalls
    // mit, weil sie die Form und Frequenz genau dieses LFO veraendern.
    //
    // Die Staerke von Flow steuert Helligkeit und Laenge: bei Flow = 0 faellt
    // gar nichts (der LFO steht dann ohnehin still).
    {
        const float flowRaw  = liveOrRaw (processor.currentMovementLivePercent, LCRMSAudioProcessor::ID_MOVEMENT);
        const float flowNorm = flowOn ? juce::jlimit (0.0f, 1.0f, std::abs (flowRaw) / 100.0f) : 0.0f;
        const float panPos   = processor.currentPanPos.load (std::memory_order_relaxed);

        // ===== Ausloesung an den UMKEHRPUNKTEN statt am Nulldurchgang =====
        // User-Idee: "Macht es noch mehr Sinn nicht bei Nulldurchgaengen
        // sondern bei max L und max R Werten (Umkehrpunkten) eine
        // Sternschnuppe auf der jeweiligen Seite abzufeuern?"
        // Ja, und deutlich: am Nulldurchgang steht der Pan genau in der
        // Mitte - dort ist visuell gar nichts los, die Sternschnuppe fiel
        // also im unauffaelligsten Moment und an einer beliebigen Stelle.
        // Am Umkehrpunkt ist der Pan dagegen maximal ausgelenkt; das ist der
        // Moment, in dem die Bewegung am deutlichsten "irgendwo" ist. Feuert
        // man dort auf genau dieser Seite, zeigt die Sternschnuppe nicht nur
        // DASS sich etwas bewegt, sondern WOHIN - sie wird zur Anzeige statt
        // zur Dekoration. Zusaetzlich ergibt das zwei Ereignisse pro Periode
        // (einmal links, einmal rechts), die im Wechsel die Pan-Bewegung
        // selbst nachzeichnen.
        //
        // Erkennung: der Umkehrpunkt ist der Vorzeichenwechsel der STEIGUNG.
        // Die zusaetzliche Schwelle auf |panPos| verhindert Fehlausloesungen,
        // wenn der LFO nahe null nur minimal zittert.
        const float delta = panPos - lastFlowPan;
        const bool turned = (delta <= 0.0f && lastFlowDelta > 0.0f)
                          || (delta >= 0.0f && lastFlowDelta < 0.0f);

        const size_t maxShooting = reducedAnimations ? 4u : 8u;
        if (modMovementEnabled && motionScale > 0.5f && flowNorm > 0.02f && turned && std::abs (panPos) > 0.15f && shootingStars.size() < maxShooting)   // Sternschnuppen haengen an Mod Movement (User)
        {
            // Seite folgt dem Vorzeichen des Pan-Werts: max R -> rechte
            // Bildhaelfte, max L -> linke.
            const bool onRight = panPos > 0.0f;

            ShootingStar sh;
            // Start in der jeweiligen Haelfte, im oberen Bildbereich.
            const float halfW = (float) w * 0.5f;
            sh.x = (onRight ? halfW : 0.0f) + bgRandom.nextFloat() * halfW;
            sh.y = bgRandom.nextFloat() * (float) h * 0.40f;

            // Flugrichtung schraeg nach unten und nach AUSSEN (zur eigenen
            // Seite hin) - verstaerkt die Aussage "die Bewegung ist dort".
            const float speedPx = bgRadius * (0.020f + 0.016f * flowNorm);
            sh.vx = (onRight ? 1.0f : -1.0f) * speedPx * (0.70f + bgRandom.nextFloat() * 0.45f);
            sh.vy = speedPx * (0.45f + bgRandom.nextFloat() * 0.35f);
            sh.life = 1.0f;
            shootingStars.push_back (sh);
        }
        lastFlowDelta = delta;
        lastFlowPan = panPos;

        // Bewegen, ausblenden, zeichnen. Der Schweif ist einfach die Strecke,
        // die sie in den letzten Frames zurueckgelegt haette - als Verlauf von
        // hell (Kopf) nach transparent (Ende).
        for (int i = (int) shootingStars.size() - 1; i >= 0; --i)
        {
            auto& sh = shootingStars[(size_t) i];
            // Auch die Sternschnuppen laufen beim DAW-Stop weich aus - Weg
            // UND Lebensdauer werden mit motionScale gebremst, sonst wuerden
            // sie an Ort und Stelle stehenbleiben und trotzdem wegfaden.
            sh.x += sh.vx * motionScale;
            sh.y += sh.vy * motionScale;
            sh.life -= 0.022f * juce::jmax (motionScale, 0.35f);   // bei Stop trotzdem ausblenden (User)

            if (sh.life <= 0.0f || sh.x < -bgRadius || sh.x > (float) w + bgRadius || sh.y > (float) h + bgRadius)
            {
                shootingStars.erase (shootingStars.begin() + i);
                continue;
            }

            const float tailLen = 9.0f;
            const float tx = sh.x - sh.vx * tailLen;
            const float ty = sh.y - sh.vy * tailLen;
            const float a  = juce::jlimit (0.0f, 1.0f, sh.life) * (0.45f + 0.55f * flowNorm);

            // Gold statt Weiss (User: "gold waere besser weil sie sehen aus
            // wie die weissen Sternlinien"). Stimmt - in Weiss waren sie von
            // den normalen Sternlinien nicht zu unterscheiden und gingen als
            // eigenes Ereignis unter. Gold ist im ganzen Bild sonst nur am
            // Innenrand vertreten, hebt sich also klar vom kuehlen Blau/Weiss
            // des Sternenfelds ab.
            static const juce::uint32 shootColByTheme[kThemeCount] = { 0xffdfe8ff, 0xffe2c37a, 0xffffc247, 0xffd48cff, 0xffffd166, 0xffe6e8ee };   // Modern weiss-blau, Dark Night gedeckt, Pop Gold, Sci-Fi violett, Day & Night gold, Moon silber (User)
            const juce::Colour shootGold (shootColByTheme[juce::jlimit (0, kThemeCount - 1, uiTheme)]);

            juce::ColourGradient trail (shootGold.withAlpha (a), sh.x, sh.y,
                                         shootGold.withAlpha (0.0f), tx, ty, false);
            g.setGradientFill (trail);
            g.drawLine (tx, ty, sh.x, sh.y, 1.8f);

            // Heller Kopf - leicht aufgehellt, damit die Spitze gluht.
            g.setColour (shootGold.brighter (0.5f).withAlpha (a));
            g.fillEllipse (sh.x - 1.8f, sh.y - 1.8f, 3.6f, 3.6f);
        }
    }

    } // Objekte, Linien, Sternschnuppen
    // Gravity hat einen eigenen Block - er ist in beiden Ansichten sichtbar.
    {
    gCur = &gGravity;
    // ===== GRAVITY: grosser angeschnittener Planet am unteren Bildrand =====
    // Das NAECHSTE Objekt der Szene, deshalb ganz zuletzt gezeichnet (vor
    // allem anderen). Ein Objekt, das vom Bildrand abgeschnitten wird, ist
    // der staerkste Naehe-Hinweis, den es gibt - das Auge ergaenzt den Rest
    // ausserhalb des Ausschnitts von selbst und ordnet die Szene dadurch
    // raeumlich: ganz nah dieser Planet, mittig Mond und Planeten, ganz fern
    // die Sterne.
    {
        // Geglaetteter Wert statt des harten gravityNorm - verhindert den
        // Sprung beim Ein-/Ausschalten der Sektion (siehe gravityVisSmoothed
        // im Header). Bei 30Hz entspricht 0.08 einer Angleichzeit von rund
        // einer halben Sekunde: schnell genug, dass Reglerbewegungen direkt
        // wirken, langsam genug, dass ein Schaltvorgang gleitet.
        // Runde 60 (User): zurueck zu Gravity - der Planet ist seine Anzeige.
        gravityVisSmoothed += (gravityNorm - gravityVisSmoothed) * 0.08f;
        gravVisSlow        += (gravityVisSmoothed - gravVisSlow) * 0.004f;   // ~8 s, folgt nur dem Reglerstand
        // Stars-Ansicht: nur noch 10 % der Modulationsbewegung (User).
        const float gravRaw  = gravityVisSmoothed;
        const float gravCalm = gravVisSlow + (gravRaw - gravVisSlow) * 0.10f;
        const float gravVis  = juce::jlimit (0.0f, 1.0f, gravCalm + (gravRaw - gravCalm) * spaceBlend);
        if (gravityMode < 5)   // 5 = kein Gravity-Planet (z. B. Purple Sky: Berge aus dem Foto)
        {

        const float fieldW = (float) w;
        const float fieldH = (float) h;

        // Radius deutlich groesser als das Feld selbst -> es ist immer nur
        // ein flacher Bogen zu sehen, nie eine ganze Kugel.
        // Runde 72 (User, Sci-Fi bei Gravity 100): je groesser der Radius,
        // desto flacher der Bogen - und desto weiter liefen die Randlicht-
        // Baender am unteren Rand auseinander. Die Spanne von 0.55 auf 0.28
        // halbiert: der Planet waechst weiterhin sichtbar mit Gravity, behaelt
        // aber seine Kruemmung.
        const float planetR = fieldW * (1.15f + gravVis * 0.28f);
        // Mittelpunkt liegt WEIT unterhalb des Feldes; mehr Gravity rueckt
        // ihn nach oben, der Bogen schiebt sich also hoeher ins Bild.
        // Zwei Korrekturen nach User-Feedback:
        //  - Grundwert 0.92 -> 0.935: der Planet sitzt bei Gravity-Default
        //    etwas tiefer ("Der Planet bei Gravity default ein kleines
        //    bisschen tiefer").
        //  - Spanne 0.10 -> 0.075: die maximale Hoehe bei vollem Gravity
        //    entspricht jetzt ungefaehr dem, was vorher bei 3 Uhr zu sehen
        //    war ("planet hoehe -> maximal so wie jetzt wenn gravity auf
        //    3 uhr ist").
        const float planetCY = fieldH + planetR * (0.935f - gravVis * 0.075f);
        const float planetCX = fieldW * 0.5f;
        const float horizonY = planetCY - planetR;

        // Gezeichneter Gravity-Planet: Sci-Fi Nachtblau mit kuehlem Randlicht;
        // Modern (User: "einen Planeten einfach modern machen, eher dunkel,
        // andeuten") fast schwarz, nur eine feine kuehle Kante, kaum Glow.
        const bool  minimalPlanet = themeModern();
        const bool  flatPlanet    = themeFlat();   // Flat: ohne Verlauf, mit klarer Kantenlinie (User)
        const juce::Colour planetDeep (flatPlanet ? 0xff262b34 : minimalPlanet ? 0xff1a1c22 : 0xff1b2a3f);   // gezeichneter Himmel: dunkles, warmes Grau, klar vom Schwarz abgesetzt
        const juce::Colour planetRim  (flatPlanet ? 0xffb9c2d0 : minimalPlanet ? 0xffd8dce6 : 0xff7fc4ff);

        // Die Kante des Planeten als Pfad - ALLES, was mit der Kante zu tun
        // hat, wird daraus abgeleitet.
        juce::Path limb;
        limb.addEllipse (planetCX - planetR, planetCY - planetR, planetR * 2.0f, planetR * 2.0f);

        if (gravityMode == 0)
        {
        // --- Koerper ---
        g.saveState();
        g.reduceClipRegion (limb);
        if (flatPlanet)
        {
            // Flat (User): keine Verlaeufe - eine solide Flaeche, darunter ein
            // zweiter, etwas dunklerer Streifen als einziges "Relief".
            g.setColour (planetDeep);
            g.fillRect (0.0f, horizonY, fieldW, fieldH - horizonY);
            g.setColour (planetDeep.darker (0.40f));
            g.fillRect (0.0f, horizonY + (fieldH - horizonY) * 0.45f, fieldW, fieldH);
        }
        else
        {
            juce::ColourGradient bodyGrad (planetDeep.brighter (0.12f), planetCX, horizonY,
                                            planetDeep.darker (0.55f),  planetCX, fieldH, false);
            g.setGradientFill (bodyGrad);
            g.fillRect (0.0f, horizonY, fieldW, fieldH - horizonY);
        }
        g.restoreState();

        // --- Randlicht ---
        // Bug-Fix (User: "Leuchtbalken fixen"): Rand und Atmosphaere wurden
        // vorher als waagerechte RECHTECKE ueber die volle Breite gezeichnet.
        // Der Planetenbogen faellt zu den Seiten hin aber um gut ein Zehntel
        // der Feldbreite ab - ein waagerechtes Band trifft die Kante deshalb
        // nur in der Bildmitte und steht links und rechts frei in der Luft.
        // Genau das war der Balken mit den harten, geraden Kanten.
        // Jetzt wird stattdessen die KANTE SELBST mehrfach nachgezeichnet:
        // von breit und fast unsichtbar bis schmal und hell. Dadurch folgt
        // das Leuchten automatisch der Kruemmung, egal wie hoch der Planet
        // gerade steht.
        const float rimA    = (0.18f + gravVis * 0.62f) * (minimalPlanet ? 0.55f : 1.0f);
        const float rimBase = juce::jmax (5.0f, bgRadius * (0.05f + gravVis * 0.15f)) * (minimalPlanet ? 0.7f : 1.0f);

        if (! flatPlanet)
        {
            struct RimPass { float widthMult; float alphaMult; };
            static const RimPass passes[] = { { 2.4f, 0.10f }, { 1.2f, 0.22f }, { 0.45f, 0.46f } };
            for (const auto& p : passes)
            {
                g.setColour (planetRim.withAlpha (rimA * p.alphaMult));
                g.strokePath (limb, juce::PathStrokeType (rimBase * p.widthMult));
            }
        }

        // Schmale, scharfe Lichtkante direkt auf der Silhouette. Flat hat nur
        // diese eine Linie, dafuer etwas kraeftiger - kein Leuchten (User).
        g.setColour (planetRim.withAlpha (flatPlanet ? (0.42f + gravVis * 0.30f)
                                                     : minimalPlanet ? (0.22f + gravVis * 0.25f) : (0.30f + gravVis * 0.40f)));
        g.strokePath (limb, juce::PathStrokeType (flatPlanet ? 1.8f : minimalPlanet ? 1.1f : 1.4f));
        } // gravityMode == 0
        else
        {
            // ===== GRAVITY ALS FOTO (Mond / Erde / Roter Mond) =====
            // Das Foto ist so vorbereitet, dass alles oberhalb der
            // Planetenkante transparent ist (Alpha-Maske). Die Kante wird
            // mit ihrem Scheitel (apexY) auf die horizonY-Hoehe gelegt, die
            // auch der gezeichnete Planet benutzt - Gravity hebt also beide
            // gleich. Breite > Feldbreite, damit nur ein flacher Bogen bleibt.
            struct P { float apexY, widthMul; };
            static const P params[4] = { { 0.102f, 1.35f }, { 0.141f, 1.35f }, { 0.211f, 1.30f }, { 0.004f, 1.45f } };
            // Erde (User: "quasi ein Strich" - bei Sci-Fi geht der Planet an den
            // Seiten frueher unter, stimmiger): das Foto wird nur als Textur
            // benutzt und auf die GEZEICHNETE Planetenkante (limb, gleiche
            // Kruemmung wie Sci-Fi) geclippt; die eigene, fast gerade Fotokante
            // liegt oberhalb und wird weggeschnitten. Saum wie beim Sci-Fi-Planeten.
            // Day & Night (User): Erde bei Tag, in der Stars-Ansicht bei Nacht -
            // beide auf dieselbe gezeichnete Kante geclippt (gleiche Form),
            // mit spaceBlend ueberblendet.
            const bool dayNight = themeDayNight();
            const bool curved   = (gravityMode == 2) || dayNight;
            // Skalierte Kopie zwischenspeichern (User: "ruckelt") - nur neu
            // rechnen, wenn sich Breite oder Modus aendern (2-px-Raster).
            // stretchX > 1 dehnt das Foto in die Breite (Moon: Mond breiter, User).
            auto prepare = [&] (int mode, juce::Image& cache, int& cMode, int& cW, float stretchX) -> bool
            {
                const juce::Image& img = gravityPhotoFor (mode);
                if (! img.isValid()) return false;
                const P& pp = params[juce::jlimit (1, 4, mode) - 1];
                const int scaledWi = juce::jmax (2, (int) std::round (fieldW * pp.widthMul * (1.0f + gravVis * 0.45f) * 0.5f) * 2);
                if (! cache.isValid() || cMode != mode || cW != scaledWi)
                {
                    const int hh = juce::jmax (1, (int) std::round ((float) img.getHeight() * (float) scaledWi / (float) img.getWidth() / stretchX));
                    cache = img.rescaled (scaledWi, hh, juce::Graphics::mediumResamplingQuality);
                    if (mode == 3)   // Night immer horizontal gespiegelt (User)
                    {
                        juce::Image flipped (juce::Image::ARGB, scaledWi, hh, true);
                        juce::Graphics fg (flipped);
                        fg.drawImageTransformed (cache, juce::AffineTransform::scale (-1.0f, 1.0f).translated ((float) scaledWi, 0.0f));
                        cache = flipped;
                    }
                    cMode = mode; cW = scaledWi;
                }
                return true;
            };
            auto drawOne = [&] (const juce::Image& im, int mode, float alpha)
            {
                const P& pp = params[juce::jlimit (1, 4, mode) - 1];
                const float scaledW = (float) im.getWidth();
                const float scaledH = (float) im.getHeight();
                const float x0 = (fieldW - scaledW) * 0.5f;
                const float y0 = horizonY - pp.apexY * scaledH - (curved ? fieldH * 0.07f : 0.0f);
                g.saveState();
                if (curved)
                    g.reduceClipRegion (limb);
                g.setOpacity (alpha);
                g.drawImageAt (im, (int) std::round (x0), (int) std::round (y0));
                // Unter dem Foto (falls es das Feld nicht bis unten fuellt)
                // bleibt es dunkel - Planetenkoerper.
                if (y0 + scaledH < fieldH)
                {
                    g.setColour (juce::Colour (0xff050608).withAlpha (alpha));
                    g.fillRect (0.0f, y0 + scaledH - 1.0f, fieldW, fieldH - (y0 + scaledH) + 1.0f);
                }
                g.restoreState();
            };
            const float moonStretch = (uiTheme == 5 && gravityMode == 1) ? 1.20f : 1.0f;
            if (gravityMode <= 4 && prepare (gravityMode, gravScaled, gravScaledMode, gravScaledW, moonStretch))
            {
                drawOne (gravScaled, gravityMode, 1.0f);
                if (dayNight && spaceBlend < 0.99f && prepare (3, gravScaledN, gravScaledNMode, gravScaledNW, 1.0f))
                    drawOne (gravScaledN, 3, 1.0f - spaceBlend);   // Nacht-Erde blendet in der Stars-Ansicht ein
                if (curved)
                {
                    const juce::Colour atmo (dayNight ? juce::Colour (0xff9fd2ff).interpolatedWith (juce::Colour (0xff6f8fc8), 1.0f - spaceBlend) : juce::Colour (0xff9fd2ff));
                    const float rimA    = (0.18f + gravVis * 0.62f) * 0.75f;
                    // Bug (User): bei hoher Gravity wurde der Saum so breit,
                    // dass die drei Ringe sichtbar auseinanderfielen - bei
                    // einem gezeichneten Sci-Fi-Planeten geht das durch, bei
                    // der echten Erde sieht es falsch aus. Deshalb ist die
                    // Saumbreite jetzt hart gedeckelt und fuer Day & Night
                    // noch einmal deutlich kleiner als bei den gezeichneten
                    // Planeten (User: "ich will ihn hier noch kleiner haben").
                    const float rimMax  = bgRadius * (dayNight ? 0.075f : 0.16f);
                    const float rimBase = juce::jmin (rimMax, juce::jmax (4.0f, bgRadius * (dayNight ? (0.022f + gravVis * 0.038f)
                                                                                                    : (0.05f  + gravVis * 0.15f))));
                    struct RimPass { float widthMult; float alphaMult; };
                    static const RimPass passes[] = { { 2.4f, 0.10f }, { 1.2f, 0.22f }, { 0.45f, 0.46f } };
                    // Bug (User): "die Erde bei Nacht hat nach INNEN einen
                    // Glow". strokePath legt die Linie MITTIG auf die Kante -
                    // die Haelfte landete also auf dem Planeten. Jetzt wird
                    // alles, was innerhalb der Kante liegt, weggeschnitten:
                    // Rechteck minus Planetenscheibe (Even-Odd-Fuellregel).
                    juce::Path outsideLimb;
                    outsideLimb.addRectangle (-rimMax * 3.0f, -rimMax * 3.0f, fieldW + rimMax * 6.0f, fieldH + rimMax * 6.0f);
                    outsideLimb.addPath (limb);
                    outsideLimb.setUsingNonZeroWinding (false);
                    g.saveState();
                    g.reduceClipRegion (outsideLimb);
                    for (const auto& p : passes)
                    {
                        g.setColour (atmo.withAlpha (rimA * p.alphaMult));
                        g.strokePath (limb, juce::PathStrokeType (rimBase * p.widthMult));
                    }
                    g.restoreState();
                    // Die schmale, scharfe Lichtkante darf mittig sitzen -
                    // 1,3 px fallen auf dem Planeten nicht auf.
                    g.setColour (atmo.withAlpha (0.25f + gravVis * 0.35f));
                    g.strokePath (limb, juce::PathStrokeType (1.3f));
                }
            }
        }
        } // gravityMode < 5
    }
    } // starfieldMode >= 2 (Full)

    } // spaceVisualsEnabled

    int writeNow = ring.writeIndex.load (std::memory_order_acquire);

    // Scope-Trace (die gruenen Vektorskop-Punkte): nur zeichnen, wenn
    // scopeTraceVisible - Starfield oben ist davon unberuehrt (User-
    // Korrektur: "Goniometer off soll lediglich das gruene technische
    // Goniometer unsichtbar machen, Starfield soll weiter laufen"). Der
    // Lese-Index wird trotzdem immer nachgezogen, damit beim Wieder-
    // Einschalten kein Rueckstand aufgeholt werden muss.
    // Spur-Puffer: Nachleuchten durch Abschwaechen der Alpha (transparenter
    // Puffer), gleiche Rate wie beim Sternenfeld.
#undef g
    const int   effSpeed   = gonioSpeed;
    const int   effStyle   = gonioStyle;
    // Auto-Large (User): nur Funkelsterne + kein Foto -> das Feld ist leer,
    // die Spur darf gross sein.
    // Groesse automatisch (Size-Button entfernt, User): Stars -> gross, Space -> klein.

    const float effSat     = gonioSat;
    static const float fadeBySpeed[3] = { 0.93f, 0.88f, 0.82f };   // Slow / Mid / Fast
    if (! traceImage.isNull())
        traceImage.multiplyAllAlphas (fadeBySpeed[effSpeed]);
    juce::Graphics gt (traceImage.isNull() ? fadeImage : traceImage);

    if (traceOn())
    {
        int idx = lastReadIndex;

        int samplesToDraw = writeNow - idx;
        if (samplesToDraw < 0) samplesToDraw += GonioRingBuffer::size;
        samplesToDraw = juce::jmin (samplesToDraw, GonioRingBuffer::size - 1);

        // Aktuellen Spitzenpegel in diesem Block ermitteln, um die
        // Auto-Skalierung nachzuziehen (schneller Anstieg, langsamer Abfall).
        float blockPeak = 0.0f;
        const int stride = 2;
        for (int n = 0; n < samplesToDraw; n += stride)
        {
            int i = (idx + n) % GonioRingBuffer::size;
            float l = ring.left[(size_t) i];
            float r = ring.right[(size_t) i];
            float mag = juce::jmax (std::abs (l), std::abs (r));
            blockPeak = juce::jmax (blockPeak, mag);
        }

        // Anstieg UND Abfall werden sanft nachgefuehrt (statt beim Anstieg
        // sofort zu springen).
        if (blockPeak > peakEnvelope)
            peakEnvelope += (blockPeak - peakEnvelope) * 0.35f; // schneller, aber sanfter Anstieg
        else
            peakEnvelope = juce::jmax (0.05f, peakEnvelope * 0.985f); // langsamer Abfall, nie unter -26dB Referenz

        // Auch wenn gar keine Bloecke mehr ankommen (Pause): samplesToDraw
        // waere 0 und es bliebe nur ein winziger Punkt (Bug, User).
        const bool idle = blockPeak < 1.0e-3f || samplesToDraw < 8;
        // Weicher Uebergang in den Ruhezustand (User: "kein harter Wechsel").
        idleSm += ((idle ? 1.0f : 0.0f) - idleSm) * juce::jlimit (0.0f, 1.0f, lastFrameDt / 0.6f);
        // Grundform fest (Seed), dazu ein Zittern je Frame wie bei echtem
        // Audio - klein und nicht stark (User).
        juce::Random idleRnd ((juce::int64) 4711);
        juce::Random jitRnd  ((juce::int64) juce::Time::getMillisecondCounter());
        if (idle) samplesToDraw = 700;
        const float idleR = 0.05f * (float) juce::jmin (w, h);   // etwa Sonnengroesse (User)

        // View: "Size" klein = 70 % der Flaeche.
        // Lines wirkt groesser als Glow -> 10 % kleiner (User); Groesse gleitet.
        const float scale = sizeSm * (effStyle == 1 ? 0.90f : 1.0f) * maxRadius / juce::jlimit (0.05f, 1.4f, peakEnvelope);

        // "Intensity" ist die DECKKRAFT der Spur, nicht ihre Farbsaettigung
        // (Korrektur: die weisse Wolke im Screenshot war eine fast
        // entsaettigte, voll deckende Spur). Farbe bleibt satt (0.62), die
        // Alpha laeuft 0.05..0.5 - das alte "voll" ist jetzt das Maximum/2.
        // Look Soft: sehr niedrige Deckkraft je Punkt -> weiche Dichtewolke
        // (der "edle" Look bei Intensity 0 %), in paint() mehrfach
        // uebereinandergelegt, damit sie trotzdem leuchtet.
        // Glow = immer Solid, Lines = immer Soft (10 % weniger weich als
        // zuvor) - die Look-Knoepfe sind entfallen (User).
        const bool  softLook = (effStyle == 1);
        const float inten = (softLook ? 0.09f   // Soft +20 % (User)
                                      : juce::jlimit (0.05f, 1.0f, 0.05f + 0.95f * effSat)) * (idle ? 0.55f * juce::jmax (0.15f, idleSm) : 1.0f);   // Stille: leicht gedimmt, weich
        const float halo  = gonioGlow;                                               // Schein-Anteil um jeden Punkt
        // Pastell (Saettigung 0.28): genau der Look aus den User-Presets
        // (die alte "Intensity" war eine Saettigung und stand dort bei
        // ~0,15-0,30) - hell, weich, nicht knallig.
        // Soft: heller Kern (Richtung Weiss), damit die halbtransparente
        // Wolke nicht dunkler/satter wirkt als Solid (User).
        const juce::Colour tcBase = traceColour();
        const juce::Colour tc = softLook ? tcBase.interpolatedWith (juce::Colours::white, 0.48f) : tcBase;
        juce::Path linePath;
        bool lineStarted = false;
        // Glow zeichnet breiter, deshalb nur jeden 3. Punkt (sonst Wolke).
        // Lines: jeden 4. Punkt - die Linie bleibt geschlossen, der Pfad hat
        // aber nur halb so viele Segmente (User: "Lines ruckelt sehr").
        const int step = (effStyle != 1) ? stride + 1 : stride * 2;

        // Stille (DAW-Stop): statt eines leeren Flecks eine kleine, ruhige
        // Zufallswolke (30 % Groesse), damit man sieht, dass das Gonio an ist
        // (User). Fester Seed je Sekunde -> steht still, wechselt gelegentlich.

        auto gauss = [&idleRnd]()
        {
            float u = juce::jmax (1.0e-6f, idleRnd.nextFloat()), v = idleRnd.nextFloat();
            return juce::jlimit (-2.2f, 2.2f, std::sqrt (-2.0f * std::log (u)) * std::cos (6.2832f * v));
        };

        for (int n = 0; n < samplesToDraw; n += step)
        {
            int i = (idx + n) % GonioRingBuffer::size;
            float l = ring.left[(size_t) i];
            float r = ring.right[(size_t) i];
            // 45 Grad Rotation: x = (R - L), y = -(L + R)
            float x = cx + (r - l) * 0.7071f * scale;
            float y = cy - (l + r) * 0.7071f * scale;
            if (idle)
            {
                // Feste Pixelgroesse, unabhaengig von der Auto-Skalierung;
                // sanfte Eigenbewegung je Punkt (User: "soll sich bewegen").
                const float tsec = (float) (juce::Time::getMillisecondCounter() % 100000) * 0.001f;
                const float jx = (jitRnd.nextFloat() - 0.5f) * idleR * 0.35f, jy = (jitRnd.nextFloat() - 0.5f) * idleR * 0.45f;
                x = cx + gauss() * idleR * 0.42f + std::sin (tsec * 0.9f + (float) n * 0.037f) * idleR * 0.10f + jx;
                y = cy + gauss() * idleR * 0.56f + std::cos (tsec * 0.7f + (float) n * 0.051f) * idleR * 0.10f + jy;
            }

            // Innerhalb der Flaeche halten, falls ein kurzer Peak ueber die
            // aktuelle Huellkurve hinausschiesst.
            x = juce::jlimit (0.0f, (float) w, x);
            y = juce::jlimit (0.0f, (float) h, y);

            if (effStyle == 1)
            {
                // Linie: alle Punkte verbunden - das klassische Vektorskop.
                if (! lineStarted) { linePath.startNewSubPath (x, y); lineStarted = true; }
                else                 linePath.lineTo (x, y);
            }
            else if (effStyle == 2)
            {
                // Weiche Punkte: breiter Schein, kleiner heller Kern.
                // Schein bewusst schwach - bei langem Nachleuchten summiert er
                // sich sonst zu einer grauen Wolke.
                gt.setColour (tc.withAlpha (inten * (0.08f + 0.30f * halo)));
                gt.fillEllipse (x - 3.0f, y - 3.0f, 6.0f, 6.0f);
                gt.setColour (tc.withAlpha (inten));
                gt.fillEllipse (x - 0.9f, y - 0.9f, 1.8f, 1.8f);
            }
            else
            {
                // (Dots entfernt - alles, was nicht Lines ist, ist Glow.)
                gt.setColour (tc.withAlpha (inten * (0.08f + 0.30f * halo)));
                gt.fillEllipse (x - 3.0f, y - 3.0f, 6.0f, 6.0f);
                gt.setColour (tc.withAlpha (inten));
                gt.fillEllipse (x - 0.9f, y - 0.9f, 1.8f, 1.8f);
            }
        }
        if (effStyle == 1 && lineStarted)
        {
            // Nur EIN duenner Strich (der breite Halo-Strich war der teure
            // Teil); Weichheit kommt aus der niedrigen Deckkraft + dem
            // Nachlegen in paint().
            juce::ignoreUnused (halo);
            gt.setColour (tc.withAlpha (juce::jmin (1.0f, inten * 1.6f)));
            gt.strokePath (linePath, juce::PathStrokeType (1.0f));
        }
    }

    lastReadIndex = writeNow;
    composeScene();
    repaint();
}

// ===== SZENE ZUSAMMENSETZEN (einmal je Frame, im Timer) =====
void GoniometerComponent::composeScene()
{
    if (compositeImage.isNull())
        return;
    compositeImage.clear (compositeImage.getBounds(), juce::Colour (0xff050608));
    juce::Graphics g (compositeImage);
    auto bounds = compositeImage.getBounds().toFloat();
    const float shineEff = shineSm;
    {
        // ===== SHINE: jede Objektart hat ihr eigenes Maximum =====

        // Basis = 10 % (alles "90 % geschwaerzt", User: "wirkt viel edler").
        // Der Regler hebt alle Arten IN RELATION an - bei 100 % erreicht jede
        // ihr Maximum: Planeten/Komet/Zerstoerer 80 %, Gravity-Planet und
        // Hidden Egg 70 %, funkelnde Sterne und Sternschnuppen 60 %,
        // Sternlinien (alle Farben) 50 %. Standard 50 % liegt dazwischen.
        auto level = [shineEff] (float cap) { return 0.10f + (cap - 0.10f) * shineEff; };
        // Selbstleuchter (Sonne, Kometen, Sternschnuppen, funkelnde Sterne):
        // eigene Kurve - schon in Ruhestellung 10 % ueber den anderen und
        // mit Shine deutlich staerker aufhellend (User: "alles, was in echt
        // leuchtet, soll in Relation anders skaliert sein").
        auto lumLevel = [shineEff] (float cap) { return juce::jmin (1.0f, 0.20f + (cap - 0.20f) * std::pow (shineEff, 0.75f)); };
        auto layer = [&] (const juce::Image& im, float alpha)
        {
            if (im.isNull()) return;
            g.setOpacity (juce::jlimit (0.0f, 1.0f, alpha));
            g.drawImageAt (im, 0, 0);
        };
        // Hintergrundfoto ganz hinten (nur wenn Starfield nicht Off). Eigene
        // Kurve: auch bei Shine ganz links noch erkennbar, bei voll 100 %.
        // Foto: deutlich dunkler als zuvor (User: "der alte 0 %-Wert soll
        // der neue 100 %-Wert sein"), 0,12..0,40. Dazu eine sehr langsame
        // Wanderung (elliptisch, ~1 min) und ein leichtes Atmen der
        // Helligkeit, damit das Standbild lebt.
        if (spaceVisualsEnabled && photoScaled.isValid())
        {
            // Bewegung auf 20 % (User): +-0,7 % Wanderung, +-1,2 % Atmen.
            const float ax = bounds.getWidth()  * 0.007f, ay = bounds.getHeight() * 0.007f;
            const float ox = -bounds.getWidth()  * 0.015f + ax * std::sin (photoClock / 61.0f * juce::MathConstants<float>::twoPi);
            const float oy = -bounds.getHeight() * 0.015f + ay * std::cos (photoClock / 47.0f * juce::MathConstants<float>::twoPi);
            const float breath = 1.0f + 0.012f * std::sin (photoClock / 23.0f * juce::MathConstants<float>::twoPi);
            const float photoMul = themeModern() ? 0.85f : uiTheme == 1 ? 0.82f : 1.0f;   // Modern -15 %, Dark Night -18 % (User)
            const float photoA = juce::jlimit (0.0f, 1.0f, (0.12f + 0.28f * shineEff) * breath * photoMul);
            g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);
            if (photoXfade > 0.0f && photoScaledPrev.isValid())
            {
                g.setOpacity (photoA * photoXfade);
                g.drawImageTransformed (photoScaledPrev, juce::AffineTransform::translation (ox, oy), false);
            }
            // Stars-Ansicht: gleiches Foto, deutlich dunkler und 20 % weniger
            // Farbe (User) - blendet mit spaceBlend.
            const float starsMix = 1.0f - spaceBlend;
            g.setOpacity (photoA * (1.0f - photoXfade) * (1.0f - 0.45f * starsMix));
            g.drawImageTransformed (photoScaled, juce::AffineTransform::translation (ox, oy), false);
            if (starsMix > 0.01f && photoScaledGrey.isValid())
            {
                g.setOpacity (photoA * (1.0f - photoXfade) * (1.0f - 0.45f * starsMix) * 0.20f * starsMix);
                g.drawImageTransformed (photoScaledGrey, juce::AffineTransform::translation (ox, oy), false);
            }
        }
        else if (photoXfade > 0.0f && photoScaledPrev.isValid())
        {
            // Foto -> "None": altes Bild ausblenden.
            g.setOpacity (juce::jlimit (0.0f, 1.0f, (0.12f + 0.28f * shineEff) * photoXfade * (themeModern() ? 0.85f : uiTheme == 1 ? 0.82f : 1.0f)));
            g.drawImageTransformed (photoScaledPrev, juce::AffineTransform::translation (-bounds.getWidth() * 0.015f, -bounds.getHeight() * 0.015f), false);
        }
        // spaceBlend: Space-Ebenen blenden beim Wechsel Stars <-> Space weich
        // ein/aus (User: "alle Uebergaenge smoothen").
        layer (fadeImage,    level (0.66f) * spaceBlend);                           // Sternlinien (ganz hinten; 0,50 -> 0,66, User)
        layer (twinkleImage, 0.45f + 0.55f * std::pow (shineEff, 0.75f));           // funkelnde Sterne (Startwert hoch, User)
        layer (glowImage,    0.50f + 0.50f * std::pow (shineEff, 0.6f));             // Sonne/Mond, Kometen: dimmen leicht mit, flachere Kurve (User)
        // Objekte liegen UEBER den Sternen (User: "Zerstoerer darf nicht
        // durchsichtig sein"): volle Deckkraft, die Shine-Kurve wirkt als
        // Abdunklung (schwarz, auf die Alpha-Maske begrenzt) statt als Alpha.
        // Stars-Ansicht (User): die Objekte des Themes bleiben sichtbar, aber
        // rund 90 % gedimmt - "ganz dezent eben".
        const float objVis  = spaceBlend + (1.0f - spaceBlend) * 0.10f;
        layer (objectImage, objVis);
        if (! objectImage.isNull())
        {
            // Bug (User, Pop): beim Wechsel in die Stars-Ansicht blitzten die
            // Planeten kurz auf. Ursache war, dass die Abdunklung mit
            // spaceBlend verschwand, waehrend die Deckkraft nur bis 10 %
            // fiel - in der Mitte des Uebergangs war das Objekt also heller
            // als vorher. Die Abdunklung haengt jetzt allein am Shine-Regler
            // (shineSpaceSm, springt beim Ansichtswechsel nicht) und bleibt
            // waehrend des ganzen Uebergangs konstant.
            const float capObj  = (uiTheme == 2 ? 0.80f : 0.62f);
            const float lvlObj  = 0.10f + (capObj - 0.10f) * shineSpaceSm;
            const float darkObj = juce::jlimit (0.0f, 1.0f, 1.0f - lvlObj);
            g.saveState();
            g.reduceClipRegion (objectImage, juce::AffineTransform());
            g.setColour (juce::Colours::black.withAlpha (darkObj));
            g.fillAll();
            g.restoreState();
        }
        layer (shootImage,   lumLevel (0.85f) * spaceBlend);                        // Sternschnuppen
        // Gravity bleibt in beiden Ansichten voll DECKEND (User: "nie
        // durchsichtig"). Das Dimmen der Stars-Ansicht laeuft deshalb ueber
        // die Abdunklung weiter unten und nicht ueber die Deckkraft - nur so
        // sieht man die 20 % in JEDEM Theme gleich, unabhaengig davon, was
        // hinter dem Planeten liegt (Bug: bei Dark Night, Pop und Sci-Fi war
        // davon nichts zu sehen).
        layer (gravityImage, 1.0f);
        // Gravity folgt Shine mit derselben 80 %-Kurve wie die Planeten (User),
        // aber als Abdunklung statt Transparenz: schwarz, auf die Alpha-Maske
        // des Gravity-Bildes begrenzt.
        if (! gravityImage.isNull())
        {
            const float darkBase = (gravityMode == 1 ? 0.18f : gravityMode == 2 ? 0.23f : gravityMode == 3 ? 0.03f : gravityMode == 4 ? 0.14f : 0.08f);   // Mond +10 %, Erde +15 %, Night -5 % (User)
            // Stars-Ansicht: 20 % dunkler als in Space - in allen Themes
            // gleich. Day & Night tauscht dort auf die ohnehin dunklere
            // Nacht-Erde und bekommt deshalb nur 10 % zusaetzlich (User).
            const float starsDark = (1.0f - spaceBlend) * (themeDayNight() ? 0.10f : 0.20f);
            const float dark = juce::jlimit (0.0f, 1.0f,
                                             (themeDayNight() ? (0.23f * spaceBlend + 0.03f * (1.0f - spaceBlend)) : darkBase)
                                             + starsDark);
            g.saveState();
            g.reduceClipRegion (gravityImage, juce::AffineTransform());
            g.setColour (juce::Colours::black.withAlpha (dark));
            g.fillAll();
            g.restoreState();
        }

        // ===== UMGEKEHRTE VIGNETTE hinter der Spur (User) =====
        // Sobald die Spur an ist und dahinter etwas Helles liegt (Foto oder
        // die volle Weltraum-Szene), wird die Feldmitte abgedunkelt - je
        // mehr Shine, desto staerker - damit die Spur immer lesbar bleibt.
        // Nur Funkelsterne ohne Foto brauchen das nicht.
        if (vigSm > 0.01f && (photoScaled.isValid() || starfieldMode >= 2))
        {
            const float shineN = juce::jlimit (0.0f, 1.0f, (shineEff - 0.03f) / (kShineMax - 0.03f));
            const float vigA   = 0.80f * (0.40f + 0.60f * shineN) * vigSm * (themeModern() ? 0.65f : 1.0f);   // -10 % (User); Modern schwaecher (Planet verschwand darunter)
            const float r0     = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f * (sizeSm * 1.25f);
            const float cxv = bounds.getCentreX(), cyv = bounds.getCentreY();
            juce::ColourGradient vig (juce::Colours::black.withAlpha (vigA), cxv, cyv,
                                      juce::Colours::black.withAlpha (0.0f), cxv + r0 * 1.53f, cyv, true);   // nach aussen 10 % frueher weg (User)
            vig.addColour (0.45, juce::Colours::black.withAlpha (vigA * 0.85f));
            g.setOpacity (1.0f);
            g.setGradientFill (vig);
            g.fillEllipse (cxv - r0 * 1.53f, cyv - r0 * 1.53f, r0 * 3.06f, r0 * 3.06f);
        }

    }
}

void GoniometerComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Feld-Hintergrund: echtes Schwarz (die Ebenen sind transparent).
    g.setColour (juce::Colour (0xff050608));
    g.fillRoundedRectangle (bounds, 8.0f);

    if (! fadeImage.isNull())
    {
        g.saveState();
        juce::Path clip;
        clip.addRoundedRectangle (bounds, 8.0f);
        g.reduceClipRegion (clip);
        // Die ganze Szene (Foto, Ebenen, Vignette) wird im Timer einmal in
        // compositeImage zusammengesetzt - paint() zeichnet nur noch EIN
        // Bild plus die Spur (User: "Goniometer ruckelt"; vorher wurden hier
        // sieben Ebenen je Frame durch die Fenster-Skalierung gezogen).
        g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);
        if (compositeImage.isValid())
            g.drawImageAt (compositeImage, 0, 0);

        // Hover ueber Sonne bzw. Mond (User): dort schaltet ein Klick die
        // Ansicht um - bisher gab es dafuer keinerlei Rueckmeldung. Ein
        // weicher Ring genuegt; er wird ueber dem fertigen Composite
        // gezeichnet und kostet nichts.
        if (sunHitR > 0.0f && isMouseOverOrDragging())
        {
            const auto m = getMouseXYRelative().toFloat();
            if (m.getDistanceFrom (juce::Point<float> (sunHitX, sunHitY)) <= sunHitR)
            {
                const float hr = juce::jmax (4.0f, sunDrawR * 1.18f);   // nur der Leuchtkoerper (User)
                juce::ColourGradient halo (juce::Colours::white.withAlpha (0.0f), sunHitX, sunHitY,
                                           juce::Colours::white.withAlpha (0.16f), sunHitX + hr, sunHitY, true);
                halo.addColour (0.72, juce::Colours::white.withAlpha (0.05f));
                g.setGradientFill (halo);
                g.fillEllipse (sunHitX - hr, sunHitY - hr, hr * 2.0f, hr * 2.0f);
                g.setColour (juce::Colours::white.withAlpha (0.22f));
                g.drawEllipse (sunHitX - hr, sunHitY - hr, hr * 2.0f, hr * 2.0f, 1.0f);
            }
        }
        if (traceOn() && ! traceImage.isNull())
        {
            g.setOpacity (1.0f);
            g.drawImageAt (traceImage, 0, 0);
            if (gonioStyle == 1)
                g.drawImageAt (traceImage, 0, 0);   // Lines = Soft: einmal nachlegen (leuchtet, bleibt weich)
        }
        g.restoreState();
    }

    // Das Fadenkreuz ist ENTFERNT (User-Wunsch: "Achsen-Linien im Starfield
    // komplett entfernen - sieht glaub besser aus oder?"). Zustimmung: die
    // beiden Linien waren das einzige Element, das den Blick als technisches
    // Diagramm gerahmt hat statt als Fenster nach draussen. Ohne sie wirkt
    // die Flaeche wie ein Ausschnitt aus einer Szene, nicht wie ein
    // Messgeraet - und der Korrelationsmesser unten uebernimmt die
    // technische Orientierung ohnehin schon.

    // Dezenter warmer Glow entlang der INNENKANTE (User-Wunsch: "Im inneren
    // Rand dezente gelber Glow"). Vier schmale Farbverlaeufe von der Kante
    // nach innen, die nach innen hin auf 0 auslaufen - dadurch entsteht der
    // Eindruck eines Bullauges/Fensters mit warm beleuchteter Fassung, statt
    // eines flachen Kastens. Ganz bewusst sehr schwach gehalten, damit es
    // als Atmosphaere und nicht als Effekt gelesen wird.
    {
        static const juce::uint32 glowByTheme[kThemeCount] = { 0xffb9d2ff, 0xffe2c37a, 0xffffd166, 0xffb968ff, 0xffffd166, 0xffc4c7cf };   // Rand: Modern kuehl, Dark Night gedeckt, Pop/Day & Night gold, Sci-Fi violett, Moon silber (User)
        const juce::Colour warm (glowByTheme[juce::jlimit (0, kThemeCount - 1, uiTheme)]);
        const float gw = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.16f;
        // Bug-Fix (User: "Brightness macht Starfield komisch gelb"): der warme
        // Randglow wurde bisher mit voller Staerke ueber die abgedunkelte
        // Szene gelegt und dominierte dann. Er skaliert jetzt mit Brightness
        // und Dim mit.
        // Der Glow liegt ueber der schwarzen Dim-Flaeche - er dimmt mit.
        // Fester Wert (User): entspricht dem alten Shine-50 %-Wert minus 20 %.
        const float a  = 0.046f;

        // Verteilung (User): unten voll und dezent, links nach oben hin
        // abnehmend, rechts schwaecher, oben nur noch ~5 %. Im Ambient
        // gefaellt der Glow rundum - dort blendet er auf alle Kanten zurueck.
        auto edgeW = [] (float normal) { return normal; };
        const float wBottom = 1.0f;
        const float wLeftLo = edgeW (0.85f), wLeftHi  = edgeW (0.25f);
        const float wRightLo = edgeW (0.45f), wRightHi = edgeW (0.05f);
        const float wTopL = edgeW (0.20f), wTopR = edgeW (0.05f);

        g.saveState();
        juce::Path clip;
        clip.addRoundedRectangle (bounds, 8.0f);
        g.reduceClipRegion (clip);

        // Ein Band = Verlauf von der Kante nach innen. Damit die Staerke
        // auch ENTLANG der Kante verlaufen kann (unten voll, oben schwach),
        // wird das Band in 8 Streifen geteilt, jeder mit eigener Alpha -
        // ein einzelner linearer Verlauf kann nur eine Richtung.
        constexpr int kStrips = 8;
        auto bandV = [&] (juce::Rectangle<float> r, bool leftEdge, float wBottomEdge, float wTopEdge)
        {
            const float xIn  = leftEdge ? r.getX()     : r.getRight();
            const float xOut = leftEdge ? r.getRight() : r.getX();
            const float sh = r.getHeight() / (float) kStrips;
            for (int k = 0; k < kStrips; ++k)
            {
                const float tTop = ((float) k + 0.5f) / (float) kStrips;       // 0 = oben, 1 = unten
                const float wk = wTopEdge + (wBottomEdge - wTopEdge) * tTop;
                auto strip = juce::Rectangle<float> (r.getX(), r.getY() + sh * (float) k, r.getWidth(), sh + 0.5f);
                g.setGradientFill (juce::ColourGradient (warm.withAlpha (a * wk), xIn, strip.getCentreY(),
                                                          warm.withAlpha (0.0f), xOut, strip.getCentreY(), false));
                g.fillRect (strip);
            }
        };
        auto bandH = [&] (juce::Rectangle<float> r, bool topEdge, float wLeftEdge, float wRightEdge)
        {
            const float yIn  = topEdge ? r.getY()      : r.getBottom();
            const float yOut = topEdge ? r.getBottom() : r.getY();
            const float sw = r.getWidth() / (float) kStrips;
            for (int k = 0; k < kStrips; ++k)
            {
                const float t = ((float) k + 0.5f) / (float) kStrips;          // 0 = links, 1 = rechts
                const float wk = wLeftEdge + (wRightEdge - wLeftEdge) * t;
                auto strip = juce::Rectangle<float> (r.getX() + sw * (float) k, r.getY(), sw + 0.5f, r.getHeight());
                g.setGradientFill (juce::ColourGradient (warm.withAlpha (a * wk), strip.getCentreX(), yIn,
                                                          warm.withAlpha (0.0f), strip.getCentreX(), yOut, false));
                g.fillRect (strip);
            }
        };

        bandV ({ bounds.getX(), bounds.getY(), gw, bounds.getHeight() }, true,  wLeftLo,  wLeftHi);
        bandV ({ bounds.getRight() - gw, bounds.getY(), gw, bounds.getHeight() }, false, wRightLo, wRightHi);
        bandH ({ bounds.getX(), bounds.getY(), bounds.getWidth(), gw }, true,  wTopL, wTopR);
        bandH ({ bounds.getX(), bounds.getBottom() - gw, bounds.getWidth(), gw }, false, wBottom, wBottom);

        g.restoreState();
    }


    // Dickerer Rahmen als vorher (1.0 -> 1.8px, User-Feedback: "Rahmen um
    // das Feld mit den Sternen dicker").
    g.setColour (juce::Colours::white.withAlpha (0.32f));
    g.drawRoundedRectangle (bounds.reduced (0.9f), 8.0f, 1.8f);
}
