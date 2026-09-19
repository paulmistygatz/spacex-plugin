#include "GoniometerComponent.h"

GoniometerComponent::GoniometerComponent (LCRMSAudioProcessor& processorToRead)
    : processor (processorToRead), ring (processorToRead.gonioBuffer)
{
    lastReadIndex = ring.writeIndex.load (std::memory_order_acquire);
    generateBackground();

    startTimerHz (30);
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
    bgStars.resize (54);
    // Beim ersten Aufbau ueber die ganze Flaeche verteilen (nicht alle bei
    // 0 starten), damit sofort ein volles Feld zu sehen ist statt eines
    // "Urknalls" beim Plugin-Oeffnen.
    for (auto& s : bgStars)
        respawnStar (s, true);
}

GoniometerComponent::~GoniometerComponent()
{
    stopTimer();
}

void GoniometerComponent::setGoniometerActive (bool active)
{
    if (active == isTimerRunning())
        return;
    if (active)
        startTimerHz (30);
    else
        stopTimer();
    // Bug-Fix (User-Feedback: "Gonio soll natuerlich nicht das ganze
    // Fenster verschwinden lassen sondern wirklich nur das Goniometer") -
    // frueher machte setVisible(false) die KOMPLETTE Komponente (also die
    // ganze quadratische Flaeche links im Fenster) unsichtbar, was wie ein
    // fehlendes Fenster-Stueck aussah. Jetzt bleibt die Komponente sichtbar
    // (Rahmen/Kreuz/letztes Bild frieren einfach ein) - nur der 30Hz-Timer
    // (und damit die CPU-Last des Vektorskops) wird tatsaechlich gestoppt.
    repaint();
}

void GoniometerComponent::setSpaceVisualsEnabled (bool enabled)
{
    spaceVisualsEnabled = enabled;
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
        fadeImage = juce::Image (juce::Image::ARGB, bounds.getWidth(), bounds.getHeight(), true);
        juce::Graphics g (fadeImage);
        g.fillAll (juce::Colours::black);
    }
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

    // Nachleucht-Effekt: vorheriges Bild leicht abdunkeln
    juce::Graphics g (fadeImage);
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.fillRect (fadeImage.getBounds());

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
    const float sideEmphasis = processor.currentSideEmphasis.load (std::memory_order_relaxed);

    // Preset-Menue (User-Idee "Deactivate Space Visuals"): ueberspringt NUR
    // Sterne + reaktive Glows, das eigentliche Vektorskop weiter unten
    // (Nachleucht-Fade + Scope-Punkte) bleibt davon unberuehrt.
    if (spaceVisualsEnabled)
    {
    for (auto& s : bgStars)
    {
        s.radius += s.speed * (0.15f + s.radius * 0.9f) * 0.03f;
        if (s.radius > 1.05f)
            respawnStar (s, false);

        const float headR = juce::jlimit (0.0f, 1.0f, s.radius);
        const float tailR = juce::jmax (0.0f, headR - (0.05f + headR * 0.12f));

        const float cosA = std::cos (s.angle);
        const float sinA = std::sin (s.angle);
        const float hx = cx + bgRadius * headR * cosA;
        const float hy = cy + bgRadius * headR * sinA;
        const float tx = cx + bgRadius * tailR * cosA;
        const float ty = cy + bgRadius * tailR * sinA;

        const float fade = 0.25f + headR * 0.75f; // dezent nahe Mitte, heller am Rand

        // Je naeher ein Stern an der horizontalen (Seiten-)Achse liegt, desto
        // staerker reagiert er auf Seiten-Betonung im Signal - Sterne oben/
        // unten (Mid-Achse) bleiben davon unbeeinflusst.
        const float horizontalness = cosA * cosA; // 1 = an den Seiten, 0 = oben/unten
        const float sideBoost = sideEmphasis * horizontalness;
        const float brightness = juce::jlimit (0.0f, 1.0f, s.alpha * fade * (1.0f + sideBoost * 2.5f));

        g.setColour (juce::Colours::white.withAlpha (brightness));
        g.drawLine (tx, ty, hx, hy, (0.6f + headR * 1.1f) * (1.0f + sideBoost * 0.7f));
    }

    // Der Mond (Starfield-Redesign, ersetzt die alten Flow-/Width-Boost-
    // "Planeten" komplett - siehe Kommentar bei moonColour/moonRadiusPx in
    // GoniometerComponent.h fuer die vollstaendige Spezifikation). Immer
    // sichtbar (kein Ein-/Ausblenden wie bei den alten Planeten), Groesse
    // kommt live von setGravityKnobDiameter().
    {
        const float driftRaw = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_DRIFT)->load();      // -100..100
        const float tiltRaw  = processor.apvts.getRawParameterValue (LCRMSAudioProcessor::ID_POS_OFFSET)->load(); // -27.5..27.5

        const float driftNorm = juce::jlimit (-1.0f, 1.0f, driftRaw / 100.0f);
        const float tiltNorm  = juce::jlimit (-1.0f, 1.0f, tiltRaw / 27.5f);

        // "Regler nach links -> Mond nach rechts" = invertiert. Jeder
        // Regler traegt einzeln maximal +-0,25 bei, addiert (User-Wunsch)
        // ergibt das zusammen maximal +-0,5 auf der X-Achse.
        const float moonNormX = juce::jlimit (-0.5f, 0.5f, (-driftNorm * 0.25f) + (-tiltNorm * 0.25f));
        // Y bleibt vorerst fix bei 0,5 (obere Haelfte) - User-Ankuendigung:
        // "ich moechte noch mehr Einfluss erzeugen im naechsten Build",
        // daher absichtlich als eigene, leicht auffindbare Konstante statt
        // fest inline verrechnet.
        constexpr float moonNormY = 0.5f;

        // +Y = oben, analog zur Vorzeichenkonvention des Scope-Traces weiter
        // unten (dort: y = cy - (l+r)*... -> positiver Wert = nach oben).
        const float mx = cx + moonNormX * bgRadius;
        const float my = cy - moonNormY * bgRadius;

        juce::ColourGradient grad (moonColour.withAlpha (0.70f), mx, my,
                                    moonColour.withAlpha (0.0f), mx + moonRadiusPx, my, true);
        g.setGradientFill (grad);
        g.fillEllipse (mx - moonRadiusPx, my - moonRadiusPx, moonRadiusPx * 2.0f, moonRadiusPx * 2.0f);
    }
    } // spaceVisualsEnabled

    int writeNow = ring.writeIndex.load (std::memory_order_acquire);
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

    // Anstieg UND Abfall werden jetzt sanft nachgefuehrt (statt beim
    // Anstieg sofort zu springen). Das reduziert das "Springen" der
    // Skalierung bei sich schnell aenderndem Pegel (z.B. beim Drehen des
    // Drift-Reglers mit einem Testsignal), ohne die reale Bewegung ganz
    // zu verschlucken - der Anstieg bleibt spuerbar schneller als der Abfall.
    if (blockPeak > peakEnvelope)
        peakEnvelope += (blockPeak - peakEnvelope) * 0.35f; // schneller, aber sanfter Anstieg
    else
        peakEnvelope = juce::jmax (0.05f, peakEnvelope * 0.985f); // langsamer Abfall, nie unter -26dB Referenz

    const float scale = maxRadius / juce::jlimit (0.05f, 1.4f, peakEnvelope);

    g.setColour (juce::Colour (0xff5be3c7));

    for (int n = 0; n < samplesToDraw; n += stride)
    {
        int i = (idx + n) % GonioRingBuffer::size;
        float l = ring.left[(size_t) i];
        float r = ring.right[(size_t) i];

        // 45 Grad Rotation: x = (R - L), y = -(L + R)
        float x = cx + (r - l) * 0.7071f * scale;
        float y = cy - (l + r) * 0.7071f * scale;

        // Innerhalb der Flaeche halten, falls ein kurzer Peak ueber die
        // aktuelle Huellkurve hinausschiesst.
        x = juce::jlimit (0.0f, (float) w, x);
        y = juce::jlimit (0.0f, (float) h, y);

        g.fillEllipse (x - 1.0f, y - 1.0f, 2.0f, 2.0f);
    }

    lastReadIndex = writeNow;
    repaint();
}

void GoniometerComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xff14161a));
    g.fillRoundedRectangle (bounds, 8.0f);

    if (! fadeImage.isNull())
    {
        g.saveState();
        juce::Path clip;
        clip.addRoundedRectangle (bounds, 8.0f);
        g.reduceClipRegion (clip);
        g.drawImageAt (fadeImage, 0, 0);
        g.restoreState();
    }

    // Fadenkreuz (keine Beschriftung mehr - M/S versteht sich von selbst)
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawLine (cx, bounds.getY(), cx, bounds.getBottom());
    g.drawLine (bounds.getX(), cy, bounds.getRight(), cy);

    // Dickerer Rahmen als vorher (1.0 -> 1.8px, User-Feedback: "Rahmen um
    // das Feld mit den Sternen dicker").
    g.setColour (juce::Colours::white.withAlpha (0.32f));
    g.drawRoundedRectangle (bounds.reduced (0.9f), 8.0f, 1.8f);
}
