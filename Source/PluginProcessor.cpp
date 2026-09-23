#include "DSP/FastMath.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Sync-Raten fuer Auto-Pan "Speed" (bei aktivem Sync), in Takten (4/4 angenommen).
    const float speedBarValues[] = { 1.0f/16.0f, 1.0f/8.0f, 1.0f/4.0f, 1.0f/2.0f, 1.0f, 2.0f, 4.0f, 8.0f };
    constexpr int speedDefaultIndex = 6; // "4 Bars" - bewusst eher gemaechlich als Default
}

float LCRMSAudioProcessor::driftPercentToMs (float absPercent) noexcept
{
    absPercent = juce::jlimit (0.0f, 100.0f, absPercent);
    if (absPercent <= 80.0f)
        return (absPercent / 80.0f) * 2.0f;
    return 2.0f + ((absPercent - 80.0f) / 20.0f) * 18.0f;
}

float LCRMSAudioProcessor::driftMsToPercent (float ms) noexcept
{
    ms = juce::jlimit (0.0f, 20.0f, ms);
    if (ms <= 2.0f)
        return ms * 40.0f;
    return 80.0f + (ms - 2.0f) / 18.0f * 20.0f;
}

// Siehe Kommentar an der Deklaration (PluginProcessor.h) - Section-Lock als
// simple Property direkt auf dem APVTS-State-Baum statt eines eigenen
// Parameters, dadurch automatisch Teil des gespeicherten Zustands.
bool LCRMSAudioProcessor::isSectionLocked (int soloSectionId) const
{
    return apvts.state.getProperty (juce::Identifier ("sectionLocked_" + juce::String (soloSectionId)), false);
}

void LCRMSAudioProcessor::setSectionLocked (int soloSectionId, bool locked)
{
    apvts.state.setProperty (juce::Identifier ("sectionLocked_" + juce::String (soloSectionId)), locked, nullptr);
}

// Tiefe-Regler-Kurve (User-Korrektur nach initialer Version): Modulation
// bezieht sich jetzt nicht mehr auf die volle Parameter-Range, sondern
// relativ auf den vom Nutzer eingestellten Wert selbst (bzw. dessen Abstand
// vom Neutralwert) - sonst konnte ein muehsam eingestellter Reglerwert bei
// hoher Tiefe durch die Modulation komplett ueberschrieben/bedeutungslos
// werden. Maximal moeglicher Ausschlag bei 100% Tiefe-Regler daher bewusst
// klein gehalten: +-30% des eingestellten Werts, linear zur Reglerstellung
// (50% Tiefe-Regler = +-15%, 100% = +-30%). Vorher +-20% - User-Feedback:
// "Effekt soll groesser sein... wenn jetzt zB 20% moduliert hat, dann
// sollte es jetzt 30% werden. Betrifft alle Sections."
float LCRMSAudioProcessor::modDepthCurve (float knob01) noexcept
{
    constexpr float kMaxRelativeDepth = 0.30f;
    return juce::jlimit (0.0f, 1.0f, knob01) * kMaxRelativeDepth;
}

juce::PropertiesFile::Options LCRMSAudioProcessor::appPropertiesOptions()
{
    juce::PropertiesFile::Options o;
    o.applicationName = "SpaceX";
    o.filenameSuffix = "settings";
    o.folderName = "SpaceX";
    o.osxLibrarySubFolder = "Application Support";
    return o;
}

LCRMSAudioProcessor::LCRMSAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    // Bewusst NICHT im APVTS (kein Preset-/Mutate-Bestandteil), nur fuer den
    // Host-Bypass (siehe getBypassParameter im Header).
    addParameter (hostBypassParam = new juce::AudioParameterBool (juce::ParameterID { "hostBypass", 1 }, "Bypass", false));
    {
        juce::PropertiesFile props (appPropertiesOptions());
        licensed.store (spacex::isValidSerial (props.getValue ("licence")), std::memory_order_relaxed);
    }
    pGalaxyActivate = apvts.getRawParameterValue (ID_GALAXY_ACTIVATE);
    pLcrEnabled = apvts.getRawParameterValue (ID_LCR_ENABLED);
    pLcrSens    = apvts.getRawParameterValue (ID_LCR_SENS);
    pLcrBlend   = apvts.getRawParameterValue (ID_LCR_BLEND);
    pPolL       = apvts.getRawParameterValue (ID_POL_L);
    pPolR       = apvts.getRawParameterValue (ID_POL_R);
    pPolPos     = apvts.getRawParameterValue (ID_POL_POS);
    pTimewarpBalance = apvts.getRawParameterValue (ID_TIMEWARP_BALANCE);
    pDrift      = apvts.getRawParameterValue (ID_DRIFT);
    pBend       = apvts.getRawParameterValue (ID_BEND);
    pSideWidth  = apvts.getRawParameterValue (ID_SIDE_WIDTH);
    pSideBoost  = apvts.getRawParameterValue (ID_SIDE_BOOST);
    pMovement   = apvts.getRawParameterValue (ID_MOVEMENT);
    pSpeed      = apvts.getRawParameterValue (ID_SPEED);
    pSpeedRate  = apvts.getRawParameterValue (ID_SPEED_RATE);
    pSpeedSync  = apvts.getRawParameterValue (ID_SPEED_SYNC);
    pPulse      = apvts.getRawParameterValue (ID_PULSE);

    pDriftOn      = apvts.getRawParameterValue (ID_DRIFT_ON);
    pPolOn        = apvts.getRawParameterValue (ID_POL_ON);
    pWidthBoostOn = apvts.getRawParameterValue (ID_WIDTHBOOST_ON);
    pFlowOn       = apvts.getRawParameterValue (ID_FLOW_ON);

    pPosOn       = apvts.getRawParameterValue (ID_POS_ON);
    pRayOn       = apvts.getRawParameterValue (ID_RAY_ON);
    pRayStrength = apvts.getRawParameterValue (ID_RAY_STRENGTH);
    pRayAmount   = apvts.getRawParameterValue (ID_RAY_AMOUNT);
    pRayChar     = apvts.getRawParameterValue (ID_RAY_CHAR);
    pRayRate     = apvts.getRawParameterValue (ID_RAY_RATE);
    pRayPair     = apvts.getRawParameterValue (ID_RAY_PAIR);
    pRayFast     = apvts.getRawParameterValue (ID_RAY_FAST);
    pPosOffset   = apvts.getRawParameterValue (ID_POS_OFFSET);
    pPosWidth    = apvts.getRawParameterValue (ID_POS_WIDTH);
    pPrismOn     = apvts.getRawParameterValue (ID_PRISM_ON);
    pPrismGalaxy = apvts.getRawParameterValue (ID_PRISM_GALAXY);
    pPrismDim    = apvts.getRawParameterValue (ID_PRISM_DIM);
    pPrismVis    = apvts.getRawParameterValue (ID_PRISM_VIS);
    pAutoGain    = apvts.getRawParameterValue (ID_AUTO_GAIN);
    pBassGuard   = apvts.getRawParameterValue (ID_BASS_GUARD);
    pHorizon     = apvts.getRawParameterValue (ID_LCR_HORIZON);
    pDepth       = apvts.getRawParameterValue (ID_DEPTH);
    pPrismLo     = apvts.getRawParameterValue (ID_PRISM_LO);
    pPrismHi     = apvts.getRawParameterValue (ID_PRISM_HI);
    pPosDistance = apvts.getRawParameterValue (ID_POS_DISTANCE);
    pPosElevate  = apvts.getRawParameterValue (ID_POS_ELEVATE);
    pMonoCheck   = apvts.getRawParameterValue (ID_MONO_CHECK);
    pMonoDry     = apvts.getRawParameterValue (ID_MONO_DRY);
    pSoloSection = apvts.getRawParameterValue (ID_SOLO_SECTION);
    pVolTrim     = apvts.getRawParameterValue (ID_VOL_TRIM);
    pOutPan      = apvts.getRawParameterValue (ID_OUT_PAN);
    pMix         = apvts.getRawParameterValue (ID_MIX);

    pTimewarpMod   = apvts.getRawParameterValue (ID_TIMEWARP_MOD);
    pDimensionMod  = apvts.getRawParameterValue (ID_DIMENSION_MOD);
    pHyperdriveMod = apvts.getRawParameterValue (ID_HYPERDRIVE_MOD);
    pTimewarpDepth   = apvts.getRawParameterValue (ID_TIMEWARP_DEPTH);
    pDimensionDepth  = apvts.getRawParameterValue (ID_DIMENSION_DEPTH);
    pHyperdriveDepth = apvts.getRawParameterValue (ID_HYPERDRIVE_DEPTH);
    pGalaxyMod       = apvts.getRawParameterValue (ID_GALAXY_MOD);
    pGalaxyDepth     = apvts.getRawParameterValue (ID_GALAXY_DEPTH);
    pPositionMod     = apvts.getRawParameterValue (ID_POSITION_MOD);
    pPositionDepth   = apvts.getRawParameterValue (ID_POSITION_DEPTH);
    pGlobalModBypass = apvts.getRawParameterValue (ID_GLOBAL_MOD_BYPASS);
    pLife = apvts.getRawParameterValue (ID_LIFE);
    pParallaxMode   = apvts.getRawParameterValue (ID_PARALLAX_MODE);
    pParallaxAmount = apvts.getRawParameterValue (ID_PARALLAX_AMOUNT);

    // Preset-/Hamburger-Menue (User-Idee): App-weite Standardwerte fuer NEU
    // geoeffnete Plugin-Instanzen, per PropertiesFile (siehe
    // appPropertiesOptions()). Greift NUR, solange der Host danach keinen
    // eigenen gespeicherten Zustand nachlaedt - ein echtes Projekt
    // ueberschreibt das hier Gesetzte ganz normal per
    // setStateInformation()/apvts.replaceState(), genau wie es die
    // eingebauten Parameter-Defaults aus createParameterLayout() auch
    // ohnehin schon immer tut (Konstruktor laeuft IMMER vor
    // setStateInformation). User-Bestaetigung: ein komplett gespeicherter
    // "Zustand als Standard" hat Vorrang vor dem einzelnen "Activate Galaxy
    // als Standard"-Haekchen.
    // Bug-Fix ("Galaxy on startup geht nicht", 2. Anlauf): der vorherige
    // Ansatz (per Timer 60ms nach Konstruktion "Activate Galaxy" reaktiv
    // auf 1.0 setzen) war grundsaetzlich race-anfaellig - manche Hosts
    // rufen setStateInformation() schon so frueh (z.B. beim Nachladen
    // eines eigenen gecachten Default-Snapshots fuer schnelleres Laden),
    // dass hasReceivedExternalState bereits VOR dem verzoegerten Callback
    // gesetzt wird und der Default nie mehr greifen kann. Jetzt stattdessen
    // robust geloest: das "galaxyActivateDefault"-Haekchen wird direkt beim
    // Anlegen des Parameters in createParameterLayout() gelesen und als
    // ECHTER Konstruktions-Default von ID_GALAXY_ACTIVATE verwendet - genau
    // wie jeder andere eingebaute Parameter-Default, der zuverlaessig VOR
    // jedem moeglichen setStateInformation()-Aufruf existiert. Der Timer
    // hier bleibt nur noch fuer das komplette "Zustand als Standard"-
    // Snapshot-Feature (defaultPluginState) zustaendig, das per Definition
    // erst nach Konstruktion angewendet werden kann (es ersetzt den
    // gesamten State-Baum).
    factoryState = apvts.copyState();

    juce::Timer::callAfterDelay (60, [this]
    {
        if (hasReceivedExternalState)
            return;

        juce::PropertiesFile props (appPropertiesOptions());
        const juce::String savedDefaultStateXml = props.getValue ("defaultPluginState", {});
        bool appliedFullDefault = false;
        if (savedDefaultStateXml.isNotEmpty())
        {
            if (auto xml = juce::XmlDocument::parse (savedDefaultStateXml))
            {
                auto tree = juce::ValueTree::fromXml (*xml);
                if (tree.isValid() && tree.hasType (apvts.state.getType()))
                {
                    apvts.replaceState (tree);
                    appliedFullDefault = true;
                }
            }
        }
        juce::ignoreUnused (appliedFullDefault);
    });
}

// ===== PARALLAX-MODI (Runde 44) =====
// Werte aus den Presets des Users (Drift %, Shift ct, Tilt %, Mix %, Pegel dB).
// Mix war beim Einstellen der GLOBALE Mix - hier ist es der Parallax-eigene.
const LCRMSAudioProcessor::ParallaxModeDef& LCRMSAudioProcessor::parallaxModeDef (int mode) noexcept
{
    // Runde 75 (User): neue Reihenfolge - erst die drei, die das Bild BEWEGEN
    // (Flux, Halo, 3D), dann die Widener, jeweils ansteigend.
    static const ParallaxModeDef defs[kParallaxModes] = {
        // 1 FLUX: zwei Wegpunkte, der Mix wandert - als einziger Modus
        //   bewegt Amount das Signal an verschiedene Stellen statt den
        //   Effekt nur groesser zu machen.
        { 2, false, false, 1.0000f, { {    5.1f, 0.00f, -7.0f, 41.0f, 0.00f, 0.0f },
                                      {    5.1f, 0.00f, -7.0f,100.0f, 0.00f, 0.0f } } },
        // 2 HALO (Runde 75, aus Pauls Preset "1 NEU DRIFT"): Drifts Werte,
        //   aber der Amount-Weg endet bei 64,9 % - und der Modus zieht sich
        //   ueber seine eigene Balance (+19,1 % bei vollem Amount) selbst
        //   wieder in die Mitte. Ergebnis ist die weiche Umrandung, die auch
        //   auf vollen Drums traegt.
        { 1, true,  false, 0.6490f, { {  -19.3f, 0.76f,  8.0f, 39.8f, 2.01f, 19.1f } } },
        // 3 3D ("3D"): altes Ultra bei 72 %, Mix 100 %
        { 1, true,  false, 1.0000f, { {  -90.3f, 0.09f, 27.5f, 38.0f, 0.17f, 0.0f } } },
        // 4 DRIFT ("Drift"): altes Ultra bei 97 %, Mix 100 %, dazu etwas mehr
        //   Tilt nach rechts (User: "soll wieder mittiger klingen")
        { 1, true,  false, 1.0000f, { {  -19.3f, 0.76f,  8.0f, 39.8f, 2.01f, 0.0f } } },
        // 5 DOUBLE - Amount = Mix bis 36,7 %
        { 1, true,  true,  1.0000f, { { -100.0f, 0.00f, 27.5f, 36.7f, 0.00f, 0.0f } } },
        // 6 WIDE ("AAA Wide Neu"): altes Ultra ganz aufgedreht, Mix 83 % -
        //   zusammengerechnet 33,2 % Parallax-Mix. Amount = nur der Mix.
        { 1, true,  false, 1.0000f, { {   -6.6f, 0.88f,  0.0f, 33.2f, 2.33f, 0.0f } } },
        // 7 ILLUSION (frueher Wide): zwei Wegpunkte, Amount endet bei 3 Uhr
        { 2, false, true,  0.8125f, { {  -38.6f, 6.58f,  0.0f, 30.7f, 0.00f, 0.0f },
                                      { -100.0f, 6.39f,  6.9f, 32.9f, 0.00f, 0.0f } } }
    };
    return defs[juce::jlimit (0, kParallaxModes - 1, mode)];
}

LCRMSAudioProcessor::ParallaxPoint LCRMSAudioProcessor::evalParallaxMode (int mode, float amount01) noexcept
{
    const auto& d = parallaxModeDef (mode);
    amount01 = juce::jlimit (0.0f, 1.0f, amount01) * d.amountMax;
    if (d.amountIsMix)
    {
        ParallaxPoint p = d.pts[0];
        p.mixPct = d.pts[0].mixPct * amount01;          // nie ueber den Preset-Wert
        p.gainDb = d.pts[0].gainDb * amount01;
        p.panPct = d.pts[0].panPct * amount01;          // Balance faehrt mit (Runde 75)
        return p;
    }
    // Punkt 0 ist immer "alles auf 0", danach die Punkte des Modus -
    // gleichmaessig ueber den Amount-Weg verteilt.
    const ParallaxPoint zero { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    const int segs = d.numPoints;                       // Null + numPoints Punkte
    const float pos = amount01 * (float) segs;
    const int   i0  = juce::jlimit (0, segs - 1, (int) std::floor (pos));
    const float t   = pos - (float) i0;
    const auto& a = (i0 == 0) ? zero : d.pts[i0 - 1];
    const auto& b = d.pts[i0];
    auto lerp = [t] (float x, float y) { return x + (y - x) * t; };
    return { lerp (a.driftPct, b.driftPct), lerp (a.bendCt, b.bendCt), lerp (a.tiltPct, b.tiltPct),
             lerp (a.mixPct, b.mixPct), lerp (a.gainDb, b.gainDb), lerp (a.panPct, b.panPct) };
}

// SpaceXraye (Runde 41): vier Charaktere. Sweep = das bisherige RAYE.
const LCRMSAudioProcessor::RayCharacter& LCRMSAudioProcessor::rayCharacterFor (int index) noexcept
{
    //                                   centreHz sweepMul fbMul mixMul stereoOffset rateMul
    // Runde 48: staerker voneinander abgesetzt (User hoerte kaum Unterschied).
    static const RayCharacter chars[4] = { {  800.0f, 1.00f, 1.00f, 1.00f, 0.25f,  1.00f },    // Sweep: breiter, langsamer Schwung
                                           { 4500.0f, 0.45f, 0.70f, 1.00f, 0.25f,  2.50f },    // Shimmer: fein, weit oben, schnell
                                           {  380.0f, 1.45f, 1.45f, 1.10f, 0.50f,  0.70f },    // Spin: tief, L/R gegenlaeufig -> Drehung
                                           { 1600.0f, 1.20f, 0.90f, 1.00f, 0.125f, 0.35f } };  // Swirl: sehr langsam, weich
    return chars[juce::jlimit (0, 3, index)];
}

// RAYE-Stufe (0..3) -> Tiefe 0..1: Off=0, Light=1/3, Medium=2/3, Strong=1.
float LCRMSAudioProcessor::rayStrengthToDepth (float strengthIndex) noexcept
{
    return juce::jlimit (0.0f, 3.0f, std::round (strengthIndex)) / 3.0f;
}

// Butterworth 2. Ordnung (Q = 1/sqrt(2)), Standard-RBJ-Formeln, direkt auf
// a0 normalisiert - passt damit ohne Umbau in dieselbe BiquadState::process()
// (Direct Form II transposed), die schon fuer Elevate benutzt wird.
void LCRMSAudioProcessor::updateHighpassCoeffs (BiquadCoeffs& c, double sampleRate, float freqHz) noexcept
{
    const double w0 = 2.0 * juce::MathConstants<double>::pi
                      * juce::jlimit (10.0, sampleRate * 0.45, (double) freqHz) / sampleRate;
    const double cosW = std::cos (w0);
    const double alpha = std::sin (w0) / (2.0 * 0.70710678);
    const double a0 = 1.0 + alpha;

    c.b0 = (float) (((1.0 + cosW) * 0.5) / a0);
    c.b1 = (float) ((-(1.0 + cosW)) / a0);
    c.b2 = c.b0;
    c.a1 = (float) ((-2.0 * cosW) / a0);
    c.a2 = (float) ((1.0 - alpha) / a0);
}

// Hochschelf (RBJ-Cookbook, S = 1), auf a0 normalisiert wie die uebrigen.
// Zweite Stufe der K-Gewichtung fuer Auto Gain.
void LCRMSAudioProcessor::updateHighShelfCoeffs (BiquadCoeffs& c, double sampleRate, float freqHz, float gainDb) noexcept
{
    const double A  = std::pow (10.0, (double) gainDb / 40.0);
    const double w0 = 2.0 * juce::MathConstants<double>::pi
                      * juce::jlimit (10.0, sampleRate * 0.45, (double) freqHz) / sampleRate;
    const double cosW = std::cos (w0);
    const double alpha = std::sin (w0) * 0.5 * std::sqrt (2.0);
    const double twoSqrtAalpha = 2.0 * std::sqrt (A) * alpha;
    const double a0 = (A + 1.0) - (A - 1.0) * cosW + twoSqrtAalpha;

    c.b0 = (float) ((A * ((A + 1.0) + (A - 1.0) * cosW + twoSqrtAalpha)) / a0);
    c.b1 = (float) ((-2.0 * A * ((A - 1.0) + (A + 1.0) * cosW)) / a0);
    c.b2 = (float) ((A * ((A + 1.0) + (A - 1.0) * cosW - twoSqrtAalpha)) / a0);
    c.a1 = (float) ((2.0 * ((A - 1.0) - (A + 1.0) * cosW)) / a0);
    c.a2 = (float) (((A + 1.0) - (A - 1.0) * cosW - twoSqrtAalpha) / a0);
}

void LCRMSAudioProcessor::updateLowpassCoeffs (BiquadCoeffs& c, double sampleRate, float freqHz) noexcept
{
    const double w0 = 2.0 * juce::MathConstants<double>::pi
                      * juce::jlimit (10.0, sampleRate * 0.45, (double) freqHz) / sampleRate;
    const double cosW = std::cos (w0);
    const double alpha = std::sin (w0) / (2.0 * 0.70710678);
    const double a0 = 1.0 + alpha;

    c.b0 = (float) (((1.0 - cosW) * 0.5) / a0);
    c.b1 = (float) ((1.0 - cosW) / a0);
    c.b2 = c.b0;
    c.a1 = (float) ((-2.0 * cosW) / a0);
    c.a2 = (float) ((1.0 - alpha) / a0);
}

void LCRMSAudioProcessor::updatePeakingCoeffs (BiquadCoeffs& c, double sampleRate, float freqHz, float gainDb, float q) noexcept
{
    const float A = std::pow (10.0f, gainDb / 40.0f);
    const float w0 = 2.0f * juce::MathConstants<float>::pi * freqHz / (float) sampleRate;
    const float alpha = std::sin (w0) / (2.0f * q);
    const float cosw0 = std::cos (w0);

    const float b0 = 1.0f + alpha * A;
    const float b1 = -2.0f * cosw0;
    const float b2 = 1.0f - alpha * A;
    const float a0 = 1.0f + alpha / A;
    const float a1 = -2.0f * cosw0;
    const float a2 = 1.0f - alpha / A;

    c.b0 = b0 / a0; c.b1 = b1 / a0; c.b2 = b2 / a0;
    c.a1 = a1 / a0; c.a2 = a2 / a0;
}

juce::AudioProcessorValueTreeState::ParameterLayout LCRMSAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Aktiviert die Galaxy/LCR-Engine strukturell (Latenz an/aus) - separat
    // vom Power-Icon der Sektion selbst (ID_LCR_ENABLED, jetzt reiner
    // Bypass ohne Latenz-Aenderung), siehe ID_GALAXY_ACTIVATE-Kommentar im
    // Header. Der Default-Wert wird HIER, synchron waehrend der Konstruktion
    // (also garantiert VOR jedem moeglichen setStateInformation()-Aufruf),
    // aus dem "Activate Galaxy on startup"-Menu-Haekchen (PropertiesFile)
    // gelesen - Bug-Fix fuer "Galaxy on startup geht nicht" (siehe
    // Kommentar beim Konstruktor-Timer weiter oben fuer den vollen Kontext).
    const bool galaxyActivateDefaultFromMenu = juce::PropertiesFile (appPropertiesOptions()).getBoolValue ("galaxyActivateDefault", false);
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_GALAXY_ACTIVATE, 1 }, "Activate Galaxy", galaxyActivateDefaultFromMenu));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_LCR_ENABLED, 1 }, "LCR", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_LCR_SENS, 1 }, "Gravity",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, "%"));

    // Orbit (GUI-Label, Parameter-ID unveraendert): Range-Aenderung von
    // -100..100 auf 0..100 (User-Feedback: "Orbit soll gar nicht schmaelern.
    // Warum? Weil ehrlich der Unterschied nur bei L+R gross ist. Sobald das
    // Signal mittig wird - egal auf welchem Weg, klingt es immer sehr
    // aehnlich. Und das kann ich ja mit Size und Width perfekt einstellen.")
    // - die vorherige negative Haelfte ("nur Center") entfaellt komplett,
    // Orbit reguliert jetzt ausschliesslich Richtung "mehr L+R" oberhalb
    // seines Default-Werts (0 = neutral, unveraendert). Siehe auch die
    // angepasste Kegel-Zeichnung (CustomLookAndFeel::drawLinearSlider,
    // "focusStyle") und die Modulations-Clamps weiter unten.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_LCR_BLEND, 1 }, "Dimension",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, "%"));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_POL_L, 1 }, "Polarity L", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_POL_R, 1 }, "Polarity R", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ID_POL_POS, 1 }, "Polarity Position",
        juce::StringArray { "1", "2", "3", "4" }, POL_POS_AFTER_DIMENSION));

    // Reine An/Aus-Option, siehe ID_TIMEWARP_BALANCE-Kommentar im Header.
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_TIMEWARP_BALANCE, 1 }, "Balance", false));

    // SpaceXraye (Runde 41): Amount stufenlos + Charakter. In den anderen
    // Builds vorhanden, aber ohne Wirkung.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_RAY_AMOUNT, 1 }, "Ray Amount",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 33.3f, "%"));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ID_RAY_CHAR, 1 }, "Ray Character",
        juce::StringArray { "Sweep", "Shimmer", "Spin", "Swirl" }, 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ID_PARALLAX_MODE, 1 }, "Parallax Mode",
        juce::StringArray { "Flux", "Halo", "3D", "Drift", "Double", "Wide", "Illusion" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_PARALLAX_AMOUNT, 1 }, "Parallax Amount",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_DRIFT, 1 }, "Drift",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f, "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_BEND, 1 }, "Bend",
        // Runde 43 (User): "3 Uhr = neues Max" - der alte Bereich 0..10 ct
        // endet jetzt bei 8 ct (dort stand bisher die 3-Uhr-Stellung).
        juce::NormalisableRange<float> (0.0f, 8.0f, 0.01f), 0.0f, "ct"));

    // "Size" darf auf der Minus-Seite (verschmaelern) nur noch bis 50%
    // gehen, nicht mehr ganz bis Mono/0% (User-Feedback: "soll gar nicht so
    // viel schmaelern koennen") - Plus-Seite bleibt bei 200% unveraendert.
    // Skew-Faktor sorgt dafuer, dass der Default (100%) trotz der jetzt
    // asymmetrischen Range weiterhin exakt bei 12 Uhr liegt (wichtig fuer
    // den "centerOut"-Regelstil, siehe CustomLookAndFeel).
    // Bug-Fix (User-Feedback: "Size default position soll wieder auf 12
    // Uhr"): NormalisableRange::convertTo0to1() rechnet proportion^skew
    // (nicht proportion^(1/skew), wie fuer die Ableitung oben angenommen
    // wurde) - der Skew-Wert war dadurch genau der Kehrwert dessen, was
    // noetig ist, um den Default auf 0.5 (=12 Uhr) zu bringen. Fuer
    // Range 50..200, Default 100 (linearer Anteil 1/3) muss der Skew 0.6309
    // sein (1/3)^0.6309 = 0.5), nicht 1.585 (= 1/0.6309, das war die
    // Verwechslung). Range bleibt unveraendert, wie gewuenscht.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_SIDE_WIDTH, 1 }, "Width",
        juce::NormalisableRange<float> (50.0f, 200.0f, 0.1f, 0.6309f), 100.0f, "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_SIDE_BOOST, 1 }, "Boost",
        juce::NormalisableRange<float> (0.0f, 6.0f, 0.01f), 0.0f, "dB"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_MOVEMENT, 1 }, "Flow",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, "%"));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ID_SPEED, 1 }, "Speed Sync Rate",
        juce::StringArray { "1/16", "1/8", "1/4", "1/2", "1 Bar", "2 Bars", "4 Bars", "8 Bars" },
        speedDefaultIndex));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_SPEED_RATE, 1 }, "Speed",
        juce::NormalisableRange<float> (0.02f, 8.0f, 0.001f, 0.35f), 0.25f, "Hz"));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_SPEED_SYNC, 1 }, "Sync", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_PULSE, 1 }, "Pulse", false));

    // Section-Bypass-Schalter: unabhaengig von den Reglerwerten, damit man
    // eine Sektion schnell A/B-vergleichen kann, ohne die Einstellungen zu verlieren.
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_DRIFT_ON, 1 }, "Drift On", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_POL_ON, 1 }, "Polarity On", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_WIDTHBOOST_ON, 1 }, "Width/Boost On", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_FLOW_ON, 1 }, "Flow On", true));

    // Mod-Icons: einfaches An/Aus, kein eigener Depth-Regler (siehe Chat).
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_TIMEWARP_MOD, 1 }, "Timewarp Mod", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_DIMENSION_MOD, 1 }, "Dimension Mod", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_HYPERDRIVE_MOD, 1 }, "Hyperdrive Mod", false));

    // Tiefe-Regler je Mod-Sektion, Default 50% (siehe modDepthCurve()).
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_TIMEWARP_DEPTH, 1 }, "Timewarp Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, "%"));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_DIMENSION_DEPTH, 1 }, "Dimension Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, "%"));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_HYPERDRIVE_DEPTH, 1 }, "Hyperdrive Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, "%"));

    // Galaxy-Mod (Gravity + Orbit) und Position-Mod (Offset/Width/Distance/
    // Elevate) - dasselbe Prinzip wie Timewarp/Dimension/Hyperdrive.
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_GALAXY_MOD, 1 }, "Galaxy Mod", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_GALAXY_DEPTH, 1 }, "Galaxy Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, "%"));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_POSITION_MOD, 1 }, "Position Mod", false));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_POSITION_DEPTH, 1 }, "Position Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, "%"));

    // --- Position (neue Sektion) ---------------------------------------------
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_POS_ON, 1 }, "Position On", true));

    // Max-Ausschlag nochmal halbiert (User-Feedback: "nur noch bis +-50% des
    // aktuellen Max Wertes") - war zuvor bereits von +-100% auf +-50%
    // reduziert, dann nochmal auf +-50% DAVON, also +-25%. Danach (User-
    // Feedback: "Tilt 10% mehr Range als jetzt") um weitere 10% auf +-27.5%
    // erweitert. Bleibt symmetrisch um 0%, daher weiterhin kein Skew noetig
    // fuer die 12-Uhr-Mitte.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_POS_OFFSET, 1 }, "Offset",
        juce::NormalisableRange<float> (-27.5f, 27.5f, 0.1f), 0.0f, "%"));

    // "Width" (Position-Sektion): Max-Wert etwas kleiner gemacht (User-
    // Feedback: "Max Width Wert ein bisschen kleiner machen") - Obergrenze
    // von 180% auf 150% reduziert, Minus-Seite (verschmaelern) unveraendert
    // bei 50%. Bei dieser Range liegt der Default (100%) linear GENAU in
    // der Mitte ((100-50)/(150-50) = 0.5) - dadurch wird kein Skew-Faktor
    // mehr gebraucht, um ihn bei 12 Uhr zu zeigen (vorher, bei 50..180, war
    // dafuer noch Skew 0.7254 noetig, siehe Aenderungsverlauf).
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_POS_WIDTH, 1 }, "Position Width",
        juce::NormalisableRange<float> (50.0f, 150.0f, 0.1f), 100.0f, "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_POS_DISTANCE, 1 }, "Distance",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_POS_ELEVATE, 1 }, "Elevate",
        // Nur noch nach oben (User): bipolar ueberschnitt sich mit Distance -
        // beide bedienen dieselbe Wahrnehmungsachse, und Smart haette die
        // Haelfte aller Ergebnisse nach hinten geschoben. Mit Null am linken
        // Anschlag greift dagegen die vorhandene "meistens wenig"-Logik.
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, "%"));

    // DEPTH - ein bipolarer Regler statt Distance UND Elevate. Beide bedienen
    // dieselbe Wahrnehmungsachse (naeher/weiter), also gehoeren sie auf einen
    // Regler: links wird es ferner und dunkler, rechts naeher und offener.
    // Mitte = unbearbeitet.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_DEPTH, 1 }, "Depth",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f, "%"));

    // ===== PRISM =====
    // Default: aus, und Bereich ueber das gesamte Spektrum. Beides zusammen
    // stellt sicher, dass bestehende Presets exakt gleich klingen.
    // Default: AN, mit unterer Grenze bei 200 Hz (User-Wunsch: "standardmaessig
    // soll unterhalb von 200 Hz ein Roll-off stattfinden").
    // Das ist normalerweise schlechte Praxis - ein Filter, der immer mitlaeuft,
    // ohne dass man ihn sieht, sorgt fuer "warum klingt das nicht wie erwartet".
    // HIER ist es unbedenklich, und zwar aus genau einem Grund: das Band ist
    // sichtbar. Die Leiste im Footer zeigt jederzeit, wo die Verbreiterung
    // wirkt, und man zieht sie mit einer Geste herunter. Ein sichtbarer
    // Startwert ist etwas anderes als ein verstecktes Dauerfilter.
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_PRISM_ON, 1 }, "Prism", true));
    // Filter-Bypass: aus = die Sektion arbeitet nur innerhalb des Filters
    // (Standard), an = sie ignoriert ihn und wirkt ueber das ganze Spektrum.
    // Kostet in beiden Faellen KEINE zusaetzliche Latenz.
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_PRISM_GALAXY, 1 }, "Galaxy Focus Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_PRISM_DIM, 1 }, "Dimension Focus Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_PRISM_VIS, 1 }, "Vision Focus Bypass", false));
    // Drei Stufen statt eines Reglers: eine Neigung ist entweder da oder nicht,
    // und ein Regler laedt nur zum Uebertreiben ein (User).
    // Standardmaessig AN: der ehrliche Vergleich soll der Normalfall sein,
    // nicht die Ausnahme, die man erst suchen muss.
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_AUTO_GAIN, 1 }, "Auto Gain", true));
    // Bass-Guard: unterhalb 120 Hz bleibt das Material unangetastet.
    // In Galaxy als Maske INNERHALB der FFT (linearphasig, summentreu),
    // in Dimension als Biquad auf dem Side-Anteil. Abschaltbar, weil es
    // mit der neuen Aufloesung (4096 statt 1024) womoeglich gar nicht
    // mehr noetig ist - das laesst sich nur im Vergleich hoeren.
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_BASS_GUARD, 1 }, "Bass Guard", true));
    // HORIZON - die obere Grenze des Extraktionsbandes (Bertoms LPF).
    // Oberhalb davon wird NICHT in Mitte und Seiten zerlegt, das Material
    // bleibt unveraendert in L/R. Zusammen mit dem Bass Guard (untere
    // Grenze, 120 Hz) ist das genau das Band, das Leapwing CenterOne im
    // Manual beschreibt. Als Maske innerhalb der FFT: linearphasig und
    // summentreu - bei Orbit in der Mitte aendert Horizon deshalb GAR
    // nichts, es verschiebt nur, was Orbit ueberhaupt anfassen kann.
    // Voller Ausschlag = aus.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        // AIR (war HORIZON), UMGEDREHT (User): aufdrehen heisst "mehr",
        // nicht "Filter runter". 0 % = aus, 100 % = Bandgrenze bei 500 Hz.
        // Dazwischen logarithmisch: 20 kHz * 0.025^(Wert/100).
        juce::ParameterID { ID_LCR_HORIZON, 1 }, "Regain",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int) -> juce::String
            {
                if (v < 0.5f) return "Off";
                const float hz = 20000.0f * std::pow (0.025f, v * 0.01f);
                if (hz >= 1000.0f) return juce::String (hz / 1000.0f, hz >= 10000.0f ? 1 : 2) + " kHz";
                return juce::String ((int) std::round (hz)) + " Hz";
            })));
    // Skew 0.25 -> logarithmisches Regelgefuehl ueber den Hoerbereich, sonst
    // liegt die halbe Reglerstrecke oberhalb von 10 kHz.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_PRISM_LO, 1 }, "Prism Low",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.25f), 200.0f, "Hz"));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_PRISM_HI, 1 }, "Prism High",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.25f), 20000.0f, "Hz"));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_MONO_CHECK, 1 }, "Mono Check", false));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_MONO_DRY, 1 }, "Mono Dry Compare", false));

    // Globaler Mod-Bypass, siehe ID_GLOBAL_MOD_BYPASS-Kommentar im Header.
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_GLOBAL_MOD_BYPASS, 1 }, "Global Mod Bypass", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_LIFE, 1 }, "Life",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, "%"));

    // Ganz simpler Ausgangs-Trim, allerletzte Stufe der Kette.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_VOL_TRIM, 1 }, "Vol Trim",
        juce::NormalisableRange<float> (-6.0f, 6.0f, 0.01f), 0.0f, "dB"));

    // Balance, allerletzte Stufe nach dem Vol-Trim (Runde 74).
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_OUT_PAN, 1 }, "Pan",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f, "%"));

    // Globaler Mix, siehe ID_MIX-Kommentar im Header.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_MIX, 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, "%"));

    // --- RAY (Stereo-Phaser, siehe Header) ------------------------------------
    // Standard AUS: ein Phaser ist ein hoerbarer Charakter-Eingriff und
    // gehoert nicht in den neutralen Startzustand des Plugins.
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_RAY_ON, 1 }, "Ray On", true));
    // Vier Stufen inkl. "Off" (User: "raye 4 click - off, section soll on
    // bleiben"): Stufe 0 laesst das Signal unveraendert durch, die Sektion
    // selbst bleibt dabei eingeschaltet. Default "Light".
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ID_RAY_STRENGTH, 1 }, "Ray Strength",
        juce::StringArray { "Off", "Light", "Medium", "Strong" }, 1));
    // Rate mit Skew, damit der langsame Bereich (0,05-0,5 Hz), in dem ein
    // Phaser auf Stimmen am schoensten ist, den Grossteil des Reglerwegs
    // bekommt.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ID_RAY_RATE, 1 }, "Ray Speed",
        // Bereich verkleinert (User: "ray speed geht viel zu schnell -> 1/2
        // bar ist was ich als Maximum noch gut empfinde"): 1 Hz ist bei
        // 120 BPM genau ein halber Takt. Default 0,15 Hz = alle knapp 7 s.
        juce::NormalisableRange<float> (0.02f, 1.0f, 0.001f, 0.5f), 0.15f, "Hz"));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_RAY_PAIR, 1 }, "Ray Pair", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ID_RAY_FAST, 1 }, "Ray Fast", false));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ID_SOLO_SECTION, 1 }, "Solo",
        juce::StringArray { "None", "Galaxy", "Timewarp", "Polarity", "Dimension", "Hyperdrive", "Position", "Ray" },
        SOLO_NONE));

    return { params.begin(), params.end() };
}

void LCRMSAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    delayL.prepare (sampleRate, 25.0f);
    delayR.prepare (sampleRate, 25.0f);
    bendL.prepare (sampleRate);
    bendR.prepare (sampleRate);
    lastBendForRatio = -1.0e9f;
    lcrExtractor.prepare (sampleRate);

    // Bug-Fix (User-Feedback: "Latenz wird nicht korrekt uebermittelt. Ist
    // nicht mehr in Sync.") - lastReportedLatency wurde bisher nur auf -1
    // (Sentinel) zurueckgesetzt, der eigentliche setLatencySamples()-Aufruf
    // geschah AUSSCHLIESSLICH reaktiv im ersten processBlock()-Durchlauf.
    // Viele Hosts fragen die Latenz aber bereits direkt nach
    // prepareToPlay() ab, um ihre PDC-Kompensation VOR dem ersten
    // verarbeiteten Block korrekt einzurichten - kam zu diesem Zeitpunkt
    // noch der alte/gar kein Wert zurueck (0), rutschte die gesamte Session
    // dauerhaft aus dem Sync, sobald Galaxy von Anfang an aktiv war (was
    // seit dem "Activate Galaxy on startup"-Default-Fix jetzt haeufiger
    // vorkommt: der Parameter-Default selbst steht schon beim Start auf
    // 1.0, nicht erst reaktiv Millisekunden spaeter). Jetzt wird die
    // tatsaechlich benoetigte Latenz sofort HIER gemeldet, synchron
    // waehrend prepareToPlay(), bevor der Host je einen Block anfordert.
    {
        const bool galaxyActiveAtPrepare = pGalaxyActivate->load() > 0.5f;
        const int initialLatency = galaxyActiveAtPrepare ? lcrExtractor.getLatencySamples() : 0;
        setLatencySamples (initialLatency);
        lastReportedLatency = initialLatency;
    }
    autoPanPhase = 0.0;
    pulseSmoothState = 0.0f;
    // User-Feedback: "Pulse geschmeidiger machen. Nochmal smoothen. Aber
    // trotzdem deutlich unterscheiden zu einer Sinuskurve" - Zeitkonstante
    // von ~6ms auf ~20ms angehoben (deutlich rundere Flanken, weicheres
    // Antippen des Pan-Extrems), aber bewusst noch klar unter einer echten
    // Sinus-Charakteristik: die One-Pole-Kurve naehert sich dem Zielwert
    // (+1/-1) immer noch exponentiell/asymmetrisch an (schnell zu Beginn,
    // dann abflachend), waehrend ein Sinus symmetrisch und ohne "Ecken" im
    // Zielwechsel verlaeuft - der Unterschied bleibt also hoerbar.
    pulseSmoothCoeff = std::exp (-1.0f / (0.020f * (float) sampleRate)); // ~20ms

    const int latSize = juce::jmax (1, lcrExtractor.getLatencySamples());
    bypassDelayL.assign ((size_t) latSize, 0.0f);
    bypassDelayR.assign ((size_t) latSize, 0.0f);
    bypassWritePos = 0;
    bypassDryL.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);
    bypassDryR.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);
    bypassBlend.reset (sampleRate, 0.025);   // ~25 ms Ueberblendung
    bypassBlend.setCurrentAndTargetValue (isBypassedNow() ? 1.0f : 0.0f);

    // Auto Gain: K-Gewichtung aufsetzen und Regelung zuruecksetzen. Die
    // ersten 0,5 s laufen mit kurzer Zeitkonstante, damit ein Offline-Bounce
    // nicht mit einer hoerbaren Einschwingphase beginnt.
    updateHighpassCoeffs (bassGuardCoeffs, sampleRate, 120.0f);
    bassGuardDim = {};

    updateHighpassCoeffs  (kwHpCoeffs,    sampleRate, 60.0f);
    updateHighShelfCoeffs (kwShelfCoeffs, sampleRate, 1500.0f, 4.0f);
    kwInHpL = {}; kwInShelfL = {}; kwInHpR = {}; kwInShelfR = {};
    kwOutHpL = {}; kwOutShelfL = {}; kwOutHpR = {}; kwOutShelfR = {};
    autoGainInDelay.assign ((size_t) juce::jmax (1, lcrExtractor.getLatencySamples()), 0.0f);
    autoGainInPos = 0;
    autoGainInSq = autoGainOutSq = 0.0;
    autoGainTarget = autoGainApplied = 1.0f;
    autoGainParamSum = -1.0e9f;                          // erzwingt ein Messfenster
    autoGainMeasureSamples = (int) (sampleRate * 2.0);   // beim Laden einmal einpegeln
    autoGainDb.store (0.0f, std::memory_order_relaxed);

    lcrDryDelayL.assign ((size_t) latSize, 0.0f);
    lcrDryDelayR.assign ((size_t) latSize, 0.0f);
    lcrDryWritePos = 0;
    lcrWetGain.reset (sampleRate, 0.03);
    lcrWetGain.setCurrentAndTargetValue (
        (pGalaxyActivate->load() > 0.5f && pLcrEnabled->load() > 0.5f) ? 1.0f : 0.0f);
    galaxyEnginePaused = false;
    galaxyWarmupRemaining = 0;

    balanceOnGain.reset (sampleRate, 0.03);
    balanceOnGain.setCurrentAndTargetValue (pTimewarpBalance->load() > 0.5f ? 1.0f : 0.0f);

    chaosDuckGain.reset (sampleRate, 0.01);
    chaosDuckGain.setCurrentAndTargetValue (1.0f);
    chaosDuckHoldSamplesRemaining = 0;

    // Regler-Glaettung: ~20ms Rampe, danach exakt beim aktuellen Parameter-
    // wert starten (kein Einschwingen/Fade-In beim Laden eines Presets).
    const double knobRampSeconds = 0.02;
    sensSmoothed.reset (sampleRate, knobRampSeconds);
    sensSmoothed.setCurrentAndTargetValue (pLcrSens->load() * 0.01f);
    blendSmoothed.reset (sampleRate, knobRampSeconds);
    blendSmoothed.setCurrentAndTargetValue (pLcrBlend->load() * 0.01f);
    widthSmoothed.reset (sampleRate, knobRampSeconds);
    widthSmoothed.setCurrentAndTargetValue (pSideWidth->load() * 0.01f);
    boostSmoothed.reset (sampleRate, knobRampSeconds);
    boostSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pSideBoost->load()));
    movementSmoothed.reset (sampleRate, knobRampSeconds);
    movementSmoothed.setCurrentAndTargetValue (pMovement->load() * 0.01f);
    bendSmoothed.reset (sampleRate, knobRampSeconds);
    for (auto* sv : { &pxMixSmoothed, &pxGainSmoothed, &pxTiltLSmoothed, &pxTiltRSmoothed,
                      &pxPanLSmoothed, &pxPanRSmoothed })
        sv->reset (sampleRate, knobRampSeconds);
    pxMixSmoothed.setCurrentAndTargetValue (0.0f);
    pxGainSmoothed.setCurrentAndTargetValue (1.0f);
    pxPanLSmoothed.setCurrentAndTargetValue (1.0f);
    pxPanRSmoothed.setCurrentAndTargetValue (1.0f);
    pxTiltLSmoothed.setCurrentAndTargetValue (1.0f);
    pxTiltRSmoothed.setCurrentAndTargetValue (1.0f);
    bendSmoothed.setCurrentAndTargetValue (pBend->load());
    offsetSmoothed.reset (sampleRate, knobRampSeconds);
    offsetSmoothed.setCurrentAndTargetValue (pPosOffset->load() * 0.01f);
    posWidthSmoothed.reset (sampleRate, knobRampSeconds);
    posWidthSmoothed.setCurrentAndTargetValue (pPosWidth->load() * 0.01f);
    distanceSmoothed.reset (sampleRate, knobRampSeconds);
    distanceSmoothed.setCurrentAndTargetValue (pPosDistance->load() * 0.01f);
    elevateSmoothed.reset (sampleRate, knobRampSeconds);
    elevateSmoothed.setCurrentAndTargetValue (pPosElevate->load() * 0.01f);
    volTrimSmoothed.reset (sampleRate, knobRampSeconds);
    volTrimSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pVolTrim->load()));
    {
        const float pn = juce::jlimit (-1.0f, 1.0f, pOutPan->load() / 100.0f);
        outPanLGain.reset (sampleRate, knobRampSeconds);
        outPanRGain.reset (sampleRate, knobRampSeconds);
        outPanLGain.setCurrentAndTargetValue (pn > 0.0f ? 1.0f - pn : 1.0f);
        outPanRGain.setCurrentAndTargetValue (pn < 0.0f ? 1.0f + pn : 1.0f);
    }
    mixSmoothed.reset (sampleRate, knobRampSeconds);
    mixSmoothed.setCurrentAndTargetValue (pMix->load() * 0.01f);

    // Section-On/Off-Gains: etwas laengere Rampe (~30ms), damit auch ein
    // hart geklickter Power-Button garantiert klickfrei ein-/ausblendet.
    const double onOffRampSeconds = 0.03;
    driftOnGain.reset (sampleRate, onOffRampSeconds);
    driftOnGain.setCurrentAndTargetValue (pDriftOn->load() > 0.5f ? 1.0f : 0.0f);
    polOnGain.reset (sampleRate, onOffRampSeconds);
    polOnGain.setCurrentAndTargetValue (pPolOn->load() > 0.5f ? 1.0f : 0.0f);
    polLFlip.reset (sampleRate, onOffRampSeconds);
    polLFlip.setCurrentAndTargetValue (pPolL->load() > 0.5f ? 1.0f : 0.0f);
    polRFlip.reset (sampleRate, onOffRampSeconds);
    polRFlip.setCurrentAndTargetValue (pPolR->load() > 0.5f ? 1.0f : 0.0f);
    {
        const int polPos = juce::jlimit (0, 3, (int) std::round (pPolPos->load()));
        for (int k = 0; k < 4; ++k)
        {
            polSlotGain[k].reset (sampleRate, onOffRampSeconds);
            polSlotGain[k].setCurrentAndTargetValue (k == polPos ? 1.0f : 0.0f);
        }
    }
    widthBoostOnGain.reset (sampleRate, onOffRampSeconds);
    widthBoostOnGain.setCurrentAndTargetValue (pWidthBoostOn->load() > 0.5f ? 1.0f : 0.0f);
    flowOnGain.reset (sampleRate, onOffRampSeconds);
    flowOnGain.setCurrentAndTargetValue (pFlowOn->load() > 0.5f ? 1.0f : 0.0f);
    posOnGain.reset (sampleRate, onOffRampSeconds);
    posOnGain.setCurrentAndTargetValue (pPosOn->load() > 0.5f ? 1.0f : 0.0f);
    rayOnGain.reset (sampleRate, onOffRampSeconds);
    rayOnGain.setCurrentAndTargetValue (pRayOn->load() > 0.5f ? 1.0f : 0.0f);
    rayDepthSmoothed.reset (sampleRate, knobRampSeconds);
   #if SPACEX_RAYE_UI == 1
    rayDepthSmoothed.setCurrentAndTargetValue (juce::jlimit (0.0f, 1.0f, pRayAmount->load() * 0.01f));
   #else
    rayDepthSmoothed.setCurrentAndTargetValue (rayStrengthToDepth (pRayStrength->load()));
   #endif
    spacex::sinTable();   // Tabelle hier anlegen, nicht im Audio-Thread
    rayCoefValid = false; rayCoefCountdown = 0;
    offsetPanCachePos = -9.0f;
    rayLifeSmoothed.reset (sampleRate, knobRampSeconds);
    rayLifeSmoothed.setCurrentAndTargetValue (juce::jlimit (0.0f, 1.0f, pLife->load() * 0.01f));
    for (int k = 0; k < kRayStages; ++k) { rayApL[k] = 0.0f; rayApR[k] = 0.0f; }
    rayFbL = rayFbR = 0.0f;
    rayPhase = 0.0;
    monoCheckGain.reset (sampleRate, onOffRampSeconds);
    monoCheckGain.setCurrentAndTargetValue (pMonoCheck->load() > 0.5f ? 1.0f : 0.0f);
    monoDryBlend.reset (sampleRate, onOffRampSeconds);
    monoDryBlend.setCurrentAndTargetValue (pMonoDry->load() > 0.5f ? 1.0f : 0.0f);

    elevateStateL.z1 = elevateStateL.z2 = 0.0f;
    elevateStateR.z1 = elevateStateR.z2 = 0.0f;
    updatePeakingCoeffs (elevateCoeffs, sampleRate, 6500.0f, 0.0f, 0.7f);
    distanceLpfL = distanceLpfR = 0.0f;
    correlationSmooth = 0.0f;
    sideEmphasisSmooth = 0.0f;

    timewarpModPhase = 0.0;
    dimensionModPhase = 0.0;
    hyperdriveModPhase = 0.0;
    galaxyModPhase = 0.0;
    positionModPhase = 0.0;
}

bool LCRMSAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void LCRMSAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    // Input-Pegelanzeige (User-Wunsch: "Links und rechts von Volume ein
    // kleines Input und Output Meter Pegelanzeige") - reiner Anzeige-Peak,
    // VOR jeder Bearbeitung gemessen, beeinflusst die eigentliche
    // Verarbeitung nicht. Das GUI-Meter macht sein eigenes Attack/Decay,
    // hier wird nur der rohe Block-Peak abgelegt.
    const float inputPeakForGuard = buffer.getMagnitude (0, numSamples);
    currentInputLevel.store (inputPeakForGuard, std::memory_order_relaxed);

    // "Chaos"-Button: einmal pro Block pruefen, ob von der GUI ein Duck
    // angefordert wurde (siehe chaosTriggerRequested-Kommentar im Header).
    // exchange(false) ist atomar und race-frei, auch wenn der Button
    // theoretisch zwei Mal kurz hintereinander gedrueckt wird.
    if (chaosTriggerRequested.exchange (false))
    {
        chaosDuckGain.reset (currentSampleRate, 0.01); // ca. 10ms schnell runter
        chaosDuckGain.setTargetValue (0.35f);          // ca. -9dB Dip
        chaosDuckHoldSamplesRemaining = (int) (currentSampleRate * 0.06); // 60ms halten
    }

    // Sektions-Solo: exklusiv, ueberschreibt fuer ALLE ANDEREN Sektionen
    // deren eigenen Power-Icon-Status (nur fuer die Verarbeitung - die
    // Power-Icons selbst bleiben unveraendert und zeigen weiter ihren
    // eigentlichen An/Aus-Zustand).
    // Runde 30 hatte Solo aus der Oberflaeche genommen und den Wert hier fest
    // auf NONE gezogen. In Runde 48 kam Solo als Cmd-Klick auf den Sektions-
    // namen zurueck - aber NUR in der Oberflaeche. Die Klammer hier blieb
    // stehen, also sah Solo aus wie Solo, waehrend im Audio alles
    // unveraendert weiterlief (User Runde 70: "Cmd click SOLO macht die
    // section NICHT solo"). Der Parameter wird jetzt wieder gelesen.
    const int soloSection = juce::jlimit (SOLO_NONE, SOLO_MAX,
                                          (int) std::round (pSoloSection->load()));
    const bool soloActive = soloSection != SOLO_NONE;

    // Galaxy ist in ZWEI unabhaengige Schalter aufgeteilt (User-Feedback:
    // "Wenn ich Galaxy mal kurz bypaessen will, kommt wegen Latenz immer
    // ein Interrupt. Activate Galaxy oben global, und dann der on/off
    // Button ganz normal Bypass."):
    // - galaxyActivateRaw (ID_GALAXY_ACTIVATE, globaler "ACTIVATE GALAXY"-
    //   Schalter): schaltet die STFT-Engine strukturell scharf, steuert
    //   AUSSCHLIESSLICH die an den Host gemeldete Latenz. Soll bewusst
    //   selten umgeschaltet werden - unveraendert vom Solo-Status und vom
    //   Logo-Bypass, sonst verschiebt sich die PDC-Kompensation dabei.
    // - lcrBypassOffRaw (ID_LCR_ENABLED, der bisherige Power-Button der
    //   Galaxy-Sektion): jetzt ein reiner Effekt-Bypass OHNE Latenz-
    //   Aenderung - die Engine laeuft bei aktivem galaxyActivateRaw immer
    //   durchgehend weiter ("warm"), nur das Ergebnis wird weich rein-/
    //   rausgeblendet (siehe lcrWetGain unten), genau wie bei den anderen
    //   Sektionen (Drift/Width usw.) - daher jetzt klickfrei UND ohne
    //   Interrupt schaltbar.
    const bool galaxyActivateRaw = pGalaxyActivate->load() > 0.5f;
    const bool lcrBypassOffRaw   = pLcrEnabled->load() > 0.5f;
    const bool lcrWetOn = galaxyActivateRaw && lcrBypassOffRaw
                          && (! soloActive || soloSection == SOLO_GALAXY);
    if (! lcrWetOn)
    {
        lcrWetGain.setTargetValue (0.0f);
        galaxyWarmupRemaining = 0;
        // Erst pausieren, wenn Galaxy wirklich ganz ausgeblendet ist.
        if (galaxyActivateRaw && ! galaxyEnginePaused
            && ! lcrWetGain.isSmoothing() && lcrWetGain.getCurrentValue() <= 0.0f)
            galaxyEnginePaused = true;
    }
    else
    {
        if (galaxyEnginePaused)
        {
            galaxyEnginePaused = false;
            lcrExtractor.reset();   // sauber neu starten statt mit alten Spektren
            galaxyWarmupRemaining = 3 * lcrExtractor.getLatencySamples();
        }
        if (galaxyWarmupRemaining > 0)
        {
            // Warm-up: Engine rechnet schon, hoerbar ist noch das Original.
            lcrWetGain.setTargetValue (0.0f);
            galaxyWarmupRemaining -= numSamples;
        }
        else
        {
            lcrWetGain.setTargetValue (1.0f);
        }
    }

    // Host-Latenz melden: 0 ohne aktivierte Galaxy-Engine, fftSize sobald
    // aktiviert. Haengt NUR an galaxyActivateRaw - weder am Solo-Status
    // noch am neuen Effekt-Bypass noch am manuellen Logo-Bypass, sonst
    // verschiebt sich die PDC-Kompensation beim Umschalten dieser anderen
    // Zustaende.
    const int neededLatency = galaxyActivateRaw ? lcrExtractor.getLatencySamples() : 0;
    if (neededLatency != lastReportedLatency)
    {
        setLatencySamples (neededLatency);
        lastReportedLatency = neededLatency;
    }

    // Manueller Bypass (Klick auf das Logo): komplettes Umgehen der
    // Verarbeitung, aber weiterhin mit korrekter Latenz-Kompensation.
    // ===== BYPASS (Logo/BYP-Klick oder Host-Bypass) - weich statt hart =====
    // Das latenzgleiche Original laeuft IMMER im Ring mit, damit beim
    // Umschalten sofort das passende Signal da ist; Wet und Original werden
    // ueber ~25 ms ueberblendet (User: "bypass plugin ruckelt/knackt").
    const bool wantBypass = isBypassedNow();
    bypassBlend.setTargetValue (wantBypass ? 1.0f : 0.0f);
    if ((int) bypassDryL.size() < numSamples)
    {
        bypassDryL.resize ((size_t) numSamples, 0.0f);
        bypassDryR.resize ((size_t) numSamples, 0.0f);
    }
    {
        const int   latency = juce::jmax (0, lastReportedLatency);
        const int   bufSize = (int) bypassDelayL.size();
        const auto* inL = buffer.getReadPointer (0);
        const auto* inR = buffer.getReadPointer (1);
        for (int i = 0; i < numSamples; ++i)
        {
            const float dl = bufSize > 0 ? bypassDelayL[(size_t) bypassWritePos] : inL[i];
            const float dr = bufSize > 0 ? bypassDelayR[(size_t) bypassWritePos] : inR[i];
            if (bufSize > 0)
            {
                bypassDelayL[(size_t) bypassWritePos] = inL[i];
                bypassDelayR[(size_t) bypassWritePos] = inR[i];
                bypassWritePos = (bypassWritePos + 1) % bufSize;
            }
            bypassDryL[(size_t) i] = latency > 0 ? dl : inL[i];
            bypassDryR[(size_t) i] = latency > 0 ? dr : inR[i];
        }
    }
    if (wantBypass && ! bypassBlend.isSmoothing())
    {
        // Voll im Bypass: Original (latenzgleich) durchreichen, Rest sparen.
        buffer.copyFrom (0, 0, bypassDryL.data(), numSamples);
        buffer.copyFrom (1, 0, bypassDryR.data(), numSamples);
        currentInputLevel.store  (0.0f, std::memory_order_relaxed);
        currentOutputLevel.store (0.0f, std::memory_order_relaxed);
        currentRayLfo.store (0.0f, std::memory_order_relaxed);
        updateVisualMeters (buffer.getReadPointer (0), buffer.getReadPointer (1), numSamples);
        wasFullyBypassed = true;
        return;
    }

    // Erster Block nach dem Bypass: alles Zustandsbehaftete leeren, damit
    // sich nichts Altes entlaedt (siehe clearProcessingState im Header).
    if (wasFullyBypassed)
    {
        wasFullyBypassed = false;
        clearProcessingState();
    }

    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);

    // Polarity: L/R-Flip und Slot (1-4) als geglaettete Ziele, damit weder
    // das Umschalten eines Kanals noch der Slot-Wechsel knackt. Der alte
    // Slot faehrt aus, der neue ein - fuer ein paar Millisekunden sind beide
    // teilaktiv, was hoerbar nur ein winziger Dip ist, kein Klick.
    polLFlip.setTargetValue (pPolL->load() > 0.5f ? 1.0f : 0.0f);
    polRFlip.setTargetValue (pPolR->load() > 0.5f ? 1.0f : 0.0f);
    {
        const int polPos = juce::jlimit (0, 3, (int) std::round (pPolPos->load()));
        for (int k = 0; k < 4; ++k)
            polSlotGain[k].setTargetValue (k == polPos ? 1.0f : 0.0f);
    }

    // Drift: Richtung R (positiv) verzoegert jetzt den LINKEN Kanal, damit
    // der rechte Kanal zuerst ankommt und das Bild tatsaechlich nach R
    // wandert (vorher invertiert - Bugfix Revision 2). 0-80% Reglerweg
    // decken 0-2ms ab (dort ist der Haas-Effekt am staerksten), die
    // restlichen 20% decken 2-20ms ab. Die ChannelDelayLine glaettet das
    // Ziel intern bereits selbst weich nach.
    float driftPct = pDrift->load();

    // --- Mod-LFOs (Timewarp/Dimension/Hyperdrive) --------------------------
    // Sehr langsame, sanfte Modulation (~8s Zyklus) - bei so tiefen Frequenzen
    // reicht ein Update einmal pro Block voellig aus (kein Audio-Rate-Bedarf,
    // spart Trig-Berechnungen, gleiches Muster wie Distance/Elevate-
    // Koeffizienten weiter unten). WICHTIG (User-Korrektur): die Modulation
    // schwingt immer relativ UM DEN AKTUELL EINGESTELLTEN REGLERWERT herum -
    // die hier genannten Tiefen (z.B. 0.2ms bei Drift) sind nur die
    // Amplitude/Staerke der Schwingung, KEIN fester Zielbereich.
    const double modBlockSeconds = (double) numSamples / currentSampleRate;
    auto advanceModPhase = [modBlockSeconds] (double& phase) -> float
    {
        phase += modBlockSeconds / kModLfoPeriodSeconds;
        if (phase >= 1.0) phase -= std::floor (phase);
        return (float) std::sin (phase * juce::MathConstants<double>::twoPi);
    };
    const float timewarpSine   = advanceModPhase (timewarpModPhase);
    const float dimensionSine  = advanceModPhase (dimensionModPhase);
    const float hyperdriveSine = advanceModPhase (hyperdriveModPhase);
    const float galaxySine     = advanceModPhase (galaxyModPhase);
    const float positionSine   = advanceModPhase (positionModPhase);

    // Globaler Mod-Bypass (User-Idee "Globale Buttons"): schaltet alle 3
    // LFO-Modulationen auf einen Schlag stumm, ohne die einzelnen Mod-Icon-
    // Parameter selbst anzufassen - sie bleiben unveraendert an/aus im
    // Hintergrund und wirken sofort wieder, sobald der globale Bypass
    // ausgeschaltet wird.
    const bool globalModBypass = pGlobalModBypass->load() > 0.5f;

    const bool timewarpModOn   = pTimewarpMod->load()  > 0.5f && ! globalModBypass;
    const bool dimensionModOn  = pDimensionMod->load() > 0.5f && ! globalModBypass;
    // Hyperdrive-Mod bleibt jetzt auch bei aktivem Bar-Sync einschaltbar
    // (User-Feedback: "trotzdem an gehen, wirkt sich dann eben nur auf Flow
    // aus") - moduliert dann nur noch Movement/Flow (siehe weiter unten),
    // NICHT mehr Speed, da eine taktsynchrone Rate nicht moduliert werden
    // kann/soll (das bleibt weiterhin hart auf den Nicht-Sync-Zweig
    // beschraenkt, siehe cycleSeconds-Berechnung weiter unten).
    const bool hyperdriveModOn = pHyperdriveMod->load() > 0.5f && ! globalModBypass;
    const bool galaxyModOn     = pGalaxyMod->load()     > 0.5f && ! globalModBypass;
    const bool positionModOn   = pPositionMod->load()   > 0.5f && ! globalModBypass;

    // Tiefe-Regler (0-100%) je Mod-Sektion -> depthFraction (0..1) ueber die
    // gemeinsame Kurve (50% Reglerstellung = 20% der vollen Parameter-Range,
    // siehe modDepthCurve()). EIN Regler moduliert beide (bzw. bei Position
    // alle 4) zugehoerigen Parameter der Sektion gleichzeitig.
    // LIFE skaliert alle Tiefen gemeinsam (Runde 37).
    const float life01 = juce::jlimit (0.0f, 1.0f, pLife->load() * 0.01f);
    const float timewarpDepthFrac   = modDepthCurve (pTimewarpDepth->load()   * 0.01f * life01);
    const float dimensionDepthFrac  = modDepthCurve (pDimensionDepth->load()  * 0.01f * life01);
    const float hyperdriveDepthFrac = modDepthCurve (pHyperdriveDepth->load() * 0.01f * life01);
    const float galaxyDepthFrac     = modDepthCurve (pGalaxyDepth->load()     * 0.01f * life01);
    const float positionDepthFrac   = modDepthCurve (pPositionDepth->load()   * 0.01f * life01);

    // ===== PARALLAX-MODI (Runde 44) =====
    // In allen Builds ausser SpaceXparaCPU kommen Drift/Shift/Tilt/Mix/Pegel
    // aus Modus + Amount. Die Parallax-Modulation bewegt dann nur Amount.
    constexpr bool pxModes = (SPACEX_PARALLAX_UI != 1);
    float pxBend = 0.0f;
    if (pxModes)
    {
        const int pxMode = juce::jlimit (0, kParallaxModes - 1, (int) std::round (pParallaxMode->load()));
        float pxAmt = juce::jlimit (0.0f, 1.0f, pParallaxAmount->load() * 0.01f);
        if (pTimewarpMod->load() > 0.5f && ! globalModBypass && pxAmt > 0.001f && timewarpDepthFrac > 0.001f)
            pxAmt = juce::jlimit (0.0f, 1.0f, pxAmt + timewarpSine * pxAmt * timewarpDepthFrac);
        currentParallaxAmountLive.store (pxAmt * 100.0f, std::memory_order_relaxed);
        const auto pt = evalParallaxMode (pxMode, pxAmt);
        driftPct = pt.driftPct;
        pxBend   = pt.bendCt;
        pxMixSmoothed.setTargetValue (juce::jlimit (0.0f, 1.0f, pt.mixPct * 0.01f));
        pxGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (pt.gainDb));
        {
            // Eigene Balance des Modus (Runde 75) - dieselbe Rechnung wie beim
            // Pan-Regler: die gehaltene Seite bleibt unveraendert.
            const float pn = juce::jlimit (-1.0f, 1.0f, pt.panPct * 0.01f);
            pxPanLSmoothed.setTargetValue (pn > 0.0f ? 1.0f - pn : 1.0f);
            pxPanRSmoothed.setTargetValue (pn < 0.0f ? 1.0f + pn : 1.0f);
        }
        const float panPos = juce::jlimit (-1.0f, 1.0f, pt.tiltPct * 0.01f);
        const float angle  = (panPos + 1.0f) * (juce::MathConstants<float>::pi * 0.25f);
        pxTiltLSmoothed.setTargetValue (std::cos (angle) * juce::MathConstants<float>::sqrt2);
        pxTiltRSmoothed.setTargetValue (std::sin (angle) * juce::MathConstants<float>::sqrt2);
    }

    // Kleine Toleranz, um "Regler steht auf 0/Neutral" robust gegen
    // Rundungsfehler zu erkennen (User-Entscheidung: Modulation bleibt aus,
    // wenn der jeweilige Basis-Regler auf seinem Neutralwert steht - sonst
    // unklar, ob "wirklich aus" oder nur "unhoerbar leise").
    constexpr float kNeutralEps = 0.001f;

    // Verstaerkungsfaktor fuer Regler, deren Modulation sich am Live-Punkt
    // kaum bemerkbar machte (Gravity/Expand/Offset/Width - User-Feedback),
    // waehrend Drift bewusst unveraendert bleiben soll (siehe modDepthCurve()-
    // Kommentar oben). Wirkt nur lokal auf die betroffenen Formeln, NICHT
    // auf die gemeinsame Tiefe-Kurve selbst.
    constexpr float kWeakModBoost = 3.0f;

    // Drift: Modulationstiefe wird DIREKT IN PROZENT definiert (nicht mehr in
    // ms) und erst danach ueber driftPercentToMs() in eine Verzoegerungszeit
    // umgerechnet. Grund (User-Feedback): die Prozent->ms-Kurve ist bewusst
    // zweigeteilt (0-80% -> 0-2ms, 80-100% -> 2-20ms) - eine in ms fest
    // definierte Tiefe ergab je nach Ausgangswert voellig unterschiedlich
    // grosse Prozent-Ausschlaege und einen sichtbaren Knick/Sprung an der
    // 80%-Schwelle. In Prozent moduliert bewegt sich der Punkt auf dem
    // (Prozent-)Regler wieder gleichmaessig - dass sich das in ms je nach
    // Bereich unterschiedlich stark auswirkt, entspricht genau dem, wie der
    // Regler grundsaetzlich funktioniert.
    const float driftAbsPct = std::abs (driftPct);
    float driftAbsPctMod = driftAbsPct;
    if (! pxModes && timewarpModOn && driftAbsPct > kNeutralEps && timewarpDepthFrac > kNeutralEps)
    {
        // Ausschlag jetzt relativ zum eingestellten Wert selbst (max. +-20%
        // davon bei vollem Tiefe-Regler), nicht mehr relativ zur vollen
        // Range - siehe modDepthCurve().
        const float driftModDepthPct = driftAbsPct * timewarpDepthFrac;
        driftAbsPctMod = juce::jlimit (0.0f, 100.0f, driftAbsPct + timewarpSine * driftModDepthPct);
    }
    const float driftMsLMod = driftPct >= 0.0f ? driftPercentToMs (driftAbsPctMod) : 0.0f;
    const float driftMsRMod = driftPct <  0.0f ? driftPercentToMs (driftAbsPctMod) : 0.0f;
    delayL.setDelayMs (driftMsLMod);
    delayR.setDelayMs (driftMsRMod);

    // "Balance": automatische Gain-Kompensation fuer den Haas-Praezedenz-
    // effekt (User-Feedback, siehe ID_TIMEWARP_BALANCE-Kommentar im
    // Header) - hebt die per Delay verzoegerte (dadurch leiser
    // wahrgenommene) Seite proportional zum aktuellen Drift-Betrag an, um
    // die gefuehlte Wanderung zur Gegenseite zurueckzuholen. Reine An/Aus-
    // Option, Betrag ist fest (kein eigener Staerke-Regler). Tatsaechlich
    // angewendet wird das weiter unten pro Sample (siehe balanceOnGain).
   #if SPACEX_PARALLAX_UI != 1
    // Modus-Builds: Balance gehoert zum Modus (User-Presets A/B/C mit Balance).
    balanceOnGain.setTargetValue (parallaxModeBalance ((int) std::round (pParallaxMode->load())) ? 1.0f : 0.0f);
   #else
    balanceOnGain.setTargetValue (pTimewarpBalance->load() > 0.5f ? 1.0f : 0.0f);
   #endif
    // Runde 30, zwei Korrekturen (User: "er ist hoerbar, koennte aber noch
    // mehr centern"):
    //
    // 1) Die Kompensation haengt jetzt an der VERZOEGERUNGSZEIT statt am
    //    Prozentwert des Reglers. Der Regler ist absichtlich krumm skaliert -
    //    0-80 % decken 0-2 ms ab, die letzten 20 % springen auf 20 ms. Die
    //    alte Rechnung (Prozent x 4,5 dB) war dadurch oben voellig aus dem
    //    Tritt: zehnfache Verzoegerung, aber nur 0,9 dB mehr Ausgleich.
    //
    // 2) Der Betrag war im Arbeitsbereich schlicht zu klein. Bei 1 ms kamen
    //    1,8 dB heraus; um den Praezedenzeffekt dort spuerbar zurueckzuholen,
    //    braucht es je nach Material eher 6-10 dB.
    //
    // Oberhalb von 2 ms wird der Ausgleich bewusst EINGEFROREN: dort gewinnt
    // der Praezedenzeffekt so klar, dass mehr Pegel die Seite nur noch lauter
    // macht statt mittiger. Weiter aufdrehen wuerde den Fehler vergroessern.
    // Runde 31: 3,75 war zu viel (User: "ist lauter als die andere Seite").
    // 2,75 liegt zwischen den alten 1,8 dB bei 1 ms und den 3,75, die
    // ueberschossen haben.
    constexpr float kBalanceDbPerMs = 2.75f;   // 1 ms -> 2,75 dB, 2 ms -> 5,5 dB
    constexpr float kBalanceMaxMs   = 2.0f;
    const float balanceMs = juce::jmin (driftPercentToMs (driftAbsPctMod), kBalanceMaxMs);
    const float balanceTargetCompGain = juce::Decibels::decibelsToGain (balanceMs * kBalanceDbPerMs);
    const bool balanceBoostsLeft = driftPct >= 0.0f;
    // Live-Anzeige: jetzt direkt der modulierte Prozentwert, kein Umweg mehr
    // ueber die ms-Kurve noetig.
    currentDriftLivePercent.store (driftPct >= 0.0f ? driftAbsPctMod : -driftAbsPctMod, std::memory_order_relaxed);

    // Ziele fuer alle per-Sample geglaetteten Regler diesen Block setzen -
    // die eigentliche Interpolation passiert unten in der Schleife per
    // getNextValue(), damit auch sehr schnelles Drehen am Regler nie einen
    // hoerbaren Sprung/Zippern erzeugt.
    // Galaxy-Mod: Gravity moduliert um seinen Neutralwert (50%, daher
    // "centerOut" in der GUI), Orbit um seinen Neutralwert (0%) - beide
    // unabhaengig voneinander, gleiches Prinzip wie Timewarp (Drift+Shift).
    // Gravity bewegte sich beim Live-Punkt kaum sichtbar, selbst bei voll
    // aufgedrehtem Tiefe-Regler (User-Feedback) - deutlich verstaerkter
    // Ausschlag (kWeakModBoost), OHNE modDepthCurve() selbst anzufassen
    // (die wird auch von Timewarp/Drift genutzt, das soll bewusst so
    // bleiben wie bisher).
    const float sensRawBase = pLcrSens->load() * 0.01f;
    float sensTarget = sensRawBase;
    if (galaxyModOn && std::abs (sensRawBase - 0.5f) > kNeutralEps && galaxyDepthFrac > kNeutralEps)
    {
        const float sensModDepth = (sensRawBase - 0.5f) * galaxyDepthFrac * kWeakModBoost;
        sensTarget = juce::jlimit (0.0f, 1.0f, sensRawBase + galaxySine * sensModDepth);
    }
    sensSmoothed.setTargetValue (sensTarget);
    currentGravityLivePercent.store (sensTarget * 100.0f, std::memory_order_relaxed);

    const float blendRawBase = pLcrBlend->load() * 0.01f; // 0..1 (siehe Range-Aenderung oben)
    float blendTarget = blendRawBase;
    // Anders als bei den deviation-basierten Reglern (die bei ihrem
    // Neutralwert bewusst gar nicht modulieren) soll Orbit auch GENAU auf
    // Default (0) sichtbar auslenken (User-Feedback: "Sie soll sich relativ
    // von der aktuellen Position von Orbit ... bewegen" - war vorher bei 0
    // komplett eingefroren). Der Ausschlag haengt daher nur vom Tiefe-Regler
    // ab, nicht mehr vom aktuellen Blend-Wert selbst. Seit der Range-
    // Aenderung auf 0..100 (User-Feedback: "Orbit soll gar nicht schmaelern
    // ... nur noch Werte oberhalb von default position") wird auf 0..1 statt
    // -1..1 geclampt - die Modulation kann Orbit also nur noch NACH OBEN
    // auslenken (Richtung "mehr L+R"), nie mehr in die entfallene negative
    // ("nur Center") Richtung.
    if (galaxyModOn && galaxyDepthFrac > kNeutralEps)
    {
        blendTarget = juce::jlimit (0.0f, 1.0f, blendRawBase + galaxySine * galaxyDepthFrac);
    }
    blendSmoothed.setTargetValue (blendTarget);
    currentOrbitLivePercent.store (blendTarget * 100.0f, std::memory_order_relaxed);

    // Dimension ("Breathe"): Expand (Width) und Boost pulsieren GEGENPHASIG
    // umeinander. Beide Ziele haengen UNABHAENGIG voneinander vom jeweiligen
    // Neutralwert ab (Width=100%, Boost=0dB) - steht z.B. nur Boost auf 0dB,
    // moduliert nur Width weiter.
    const float widthRawBase = pSideWidth->load() * 0.01f;
    const float boostDbBase  = pSideBoost->load();
    float widthTarget = widthRawBase;
    float boostDbTarget = boostDbBase;
    if (dimensionModOn)
    {
        if (std::abs (widthRawBase - 1.0f) > kNeutralEps)
        {
            // Ausschlag relativ zum Abstand des eingestellten Werts vom
            // Neutralwert (100%) - max. +-20% dieses Abstands bei vollem
            // Tiefe-Regler, nicht mehr relativ zur vollen Range.
            // Expand war am Live-Punkt kaum sichtbar (User-Feedback) -
            // verstaerkt, Boost bleibt bewusst unveraendert.
            const float widthDeviation = widthRawBase - 1.0f;
            const float widthModDepth = widthDeviation * dimensionDepthFrac * kWeakModBoost;
            // Clamp auf die tatsaechliche Parameter-Range (50%..200%, siehe
            // ID_SIDE_WIDTH), nicht mehr nur auf 0 - sonst koennte die
            // Modulation den Wert unter die neue Minus-Grenze druecken.
            widthTarget = juce::jlimit (0.5f, 2.0f, widthRawBase + dimensionSine * widthModDepth);
        }
        if (std::abs (boostDbBase) > kNeutralEps)
        {
            const float boostModDepthDb = boostDbBase * dimensionDepthFrac;
            boostDbTarget = boostDbBase - dimensionSine * boostModDepthDb; // Gegenphase zu Width
        }
    }
    // Auto-Gain-Kompensation (User-Idee: "Geht sowas wie Auto Gain?" - siehe
    // Feedback "Drift + Boost in Kombi kann sehr laut werden. Beide alleine
    // sind kein Problem aber in Kombi!"): Drift (Haas-Zeitversatz) vergroes-
    // sert die Side-Energie (L-R) vieler Signale von sich aus schon deutlich -
    // ein zusaetzlich positiver Boost multipliziert dann auf ein bereits
    // aufgeblaehtes Side-Signal drauf. Statt eines harten Limiters am
    // Ausgang wird die Boost-Verstaerkung hier automatisch etwas zurueck-
    // genommen, wenn BEIDE Effekte wirklich gleichzeitig aktiv sind (Sektion
    // an, nicht wegsoloed, Drift hoerbar ausgelenkt) - bei nur einem der
    // beiden Effekte oder Boost <=0dB (Abschwaechung statt Anhebung) bleibt
    // der Klang komplett unveraendert.
    const bool driftSectionActiveForComp = pDriftOn->load() > 0.5f && (! soloActive || soloSection == SOLO_TIMEWARP);
    if (driftSectionActiveForComp && boostDbTarget > 0.0f)
    {
        const float driftAmount = juce::jlimit (0.0f, 1.0f, std::abs (driftPct) * 0.01f);
        // Bis zu 40% Ruecknahme der Boost-Anhebung bei vollem Drift-Ausschlag -
        // bewusst sanft, damit es bei kleinen Drift-Werten kaum bemerkbar
        // ist, bei extremen Kombinationen aber spuerbar gegensteuert.
        const float compAmount = driftAmount * 0.4f;
        boostDbTarget *= (1.0f - compAmount);
    }

    widthSmoothed.setTargetValue (widthTarget);
    boostSmoothed.setTargetValue (juce::Decibels::decibelsToGain (boostDbTarget));
    currentExpandLivePercent.store (widthTarget * 100.0f, std::memory_order_relaxed);
    currentBoostLiveDb.store (boostDbTarget, std::memory_order_relaxed);

    // Hyperdrive: moduliert Flow (Movement), aus wenn Movement=0% oder
    // Regler auf 0 steht (Speed-Modulation passiert weiter unten bei der
    // Zyklusdauer-Berechnung, nur wenn Sync aus ist und moduliert immer mit,
    // da Speed keinen Neutralwert hat).
    const float moveRawBase = pMovement->load() * 0.01f;
    float moveTarget = moveRawBase;
    if (hyperdriveModOn && moveRawBase > kNeutralEps && hyperdriveDepthFrac > kNeutralEps)
    {
        const float moveModDepth = moveRawBase * hyperdriveDepthFrac;
        moveTarget = juce::jlimit (0.0f, 1.0f, moveRawBase + hyperdriveSine * moveModDepth);
    }
    movementSmoothed.setTargetValue (moveTarget); // 0..1
    currentMovementLivePercent.store (moveTarget * 100.0f, std::memory_order_relaxed);

    // Timewarp moduliert zusaetzlich Shift/Bend, aus wenn Shift=0ct oder
    // Regler auf 0 steht.
    const float bendRawBase = pxModes ? pxBend : pBend->load();
    float bendTarget = bendRawBase;
    if (! pxModes && timewarpModOn && bendRawBase > kNeutralEps && timewarpDepthFrac > kNeutralEps)
    {
        const float bendModDepth = bendRawBase * timewarpDepthFrac;
        bendTarget = juce::jmax (0.0f, bendRawBase + timewarpSine * bendModDepth);
    }
    currentBendLiveCt.store (bendTarget, std::memory_order_relaxed);
    bendSmoothed.setTargetValue (bendTarget);

    // Position-Mod: EIN Tiefe-Regler beeinflusst alle 4 Regler dieser
    // Sektion gleichzeitig, jeweils relativ um den eigenen Neutralwert
    // (Offset/Elevate: 0%, Width: 100%, Distance: 0% = untere Grenze) -
    // identisches Prinzip wie bei den anderen Mod-Sektionen (User-Feedback:
    // "Einfluss auf alle Regler, gleiches Prinzip wie bei den anderen").
    // In den Modus-Builds sitzt Tilt IN der Parallax-Stufe (siehe dort).
    const float offsetRawBase = pxModes ? 0.0f : pPosOffset->load() * 0.01f; // -1..1
    float offsetTarget = offsetRawBase;
    if (positionModOn && std::abs (offsetRawBase) > kNeutralEps && positionDepthFrac > kNeutralEps)
    {
        // Offset war am Live-Punkt kaum sichtbar (User-Feedback) - verstaerkt.
        const float offsetModDepth = offsetRawBase * positionDepthFrac * kWeakModBoost;
        // Clamp auf die tatsaechliche Parameter-Range (+-27.5%, siehe
        // ID_POS_OFFSET), nicht mehr +-100%.
        offsetTarget = juce::jlimit (-0.275f, 0.275f, offsetRawBase + positionSine * offsetModDepth);
    }
    offsetSmoothed.setTargetValue (offsetTarget);
    currentOffsetLivePercent.store (offsetTarget * 100.0f, std::memory_order_relaxed);

    // Vision-Width ist gestrichen (User: weniger ist mehr). Breite macht
    // Dimension, und zwar in Mid/Side statt im Positionspfad - zwei Regler
    // fuer dieselbe Sache waren einer zu viel. Der Parameter bleibt bestehen,
    // wirkt aber nicht mehr; neutral heisst 1.0.
    juce::ignoreUnused (pPosWidth);
    const float posWidthRawBase = 1.0f;
    float posWidthTarget = posWidthRawBase;
    if (positionModOn && std::abs (posWidthRawBase - 1.0f) > kNeutralEps && positionDepthFrac > kNeutralEps)
    {
        // Width war am Live-Punkt kaum sichtbar (User-Feedback) - verstaerkt,
        // Distance/Elevate bleiben bewusst unveraendert.
        const float posWidthModDepth = (posWidthRawBase - 1.0f) * positionDepthFrac * kWeakModBoost;
        // Clamp auf die tatsaechliche Parameter-Range (50%..150%, siehe
        // ID_POS_WIDTH - Max-Wert per User-Feedback von 180% auf 150%
        // reduziert), nicht mehr nur auf 0.
        posWidthTarget = juce::jlimit (0.5f, 1.5f, posWidthRawBase + positionSine * posWidthModDepth);
    }
    posWidthSmoothed.setTargetValue (posWidthTarget);
    currentPosWidthLivePercent.store (posWidthTarget * 100.0f, std::memory_order_relaxed);

    // Distance und Elevate werden nicht mehr einzeln bedient, sondern aus
    // DEPTH abgeleitet: die Minus-Haelfte schiebt weg und nimmt oben zurueck,
    // die Plus-Haelfte holt heran und oeffnet oben.
    const float depthRaw = pDepth->load();                                  // -100..100
    const float distanceRawBase = juce::jmax (0.0f, -depthRaw) * 0.01f;     // 0..1
    float distanceTarget = distanceRawBase;
    // Runde 49 (User: "Depth wird durch life moduliert - soll das so sein?"):
    // DEPTH sitzt in DIMENSION, also haengt seine Modulation auch am
    // Dimension-Mod-Icon. Vorher lief sie ueber die unsichtbare Position-Mod
    // weiter - das Icon der Sektion konnte sie gar nicht abschalten.
    if (dimensionModOn && distanceRawBase > kNeutralEps && dimensionDepthFrac > kNeutralEps)
    {
        const float distanceModDepth = distanceRawBase * dimensionDepthFrac;
        distanceTarget = juce::jlimit (0.0f, 1.0f, distanceRawBase + dimensionSine * distanceModDepth);
    }
    distanceSmoothed.setTargetValue (distanceTarget);
    currentDistanceLivePercent.store (distanceTarget * 100.0f, std::memory_order_relaxed);

    const float elevateRawBase = juce::jmax (0.0f, depthRaw) * 0.01f;       // 0..1 (siehe DEPTH)
    float elevateTarget = elevateRawBase;
    if (dimensionModOn && std::abs (elevateRawBase) > kNeutralEps && dimensionDepthFrac > kNeutralEps)
    {
        const float elevateModDepth = elevateRawBase * dimensionDepthFrac;
        elevateTarget = juce::jlimit (-1.0f, 1.0f, elevateRawBase + dimensionSine * elevateModDepth);
    }
    elevateSmoothed.setTargetValue (elevateTarget);
    currentElevateLivePercent.store (elevateTarget * 100.0f, std::memory_order_relaxed);
    // Zurueck auf die DEPTH-Achse: negative Haelfte = Distance, positive = Elevate.
    currentDepthLivePercent.store (depthRaw >= 0.0f ? elevateTarget * 100.0f : -distanceTarget * 100.0f,
                                   std::memory_order_relaxed);

    volTrimSmoothed.setTargetValue (juce::Decibels::decibelsToGain (pVolTrim->load()));
    {
        // Balance statt Pan: die Seite, zu der man zieht, bleibt unveraendert,
        // die andere wird abgesenkt. Kein Pegelsprung in der Mitte.
        const float pn = juce::jlimit (-1.0f, 1.0f, pOutPan->load() / 100.0f);
        outPanLGain.setTargetValue (pn > 0.0f ? 1.0f - pn : 1.0f);
        outPanRGain.setTargetValue (pn < 0.0f ? 1.0f + pn : 1.0f);
    }
    mixSmoothed.setTargetValue (juce::jlimit (0.0f, 1.0f, pMix->load() * 0.01f));

    // Section-On/Off als weiche Gains statt harter Verzweigung - vermeidet
    // Knacksen beim Umschalten der Power-Icons. Solo ueberschreibt hier
    // zusaetzlich alle NICHT-soloten Sektionen auf "aus", unabhaengig vom
    // eigenen Power-Icon-Status - dieselbe Glaettung sorgt automatisch
    // dafuer, dass auch das Solo-Umschalten klickfrei ist.
    driftOnGain.setTargetValue      ((pDriftOn->load()      > 0.5f && (! soloActive || soloSection == SOLO_TIMEWARP))   ? 1.0f : 0.0f);
    polOnGain.setTargetValue        ((pPolOn->load()        > 0.5f && (! soloActive || soloSection == SOLO_POLARITY))   ? 1.0f : 0.0f);
    widthBoostOnGain.setTargetValue ((pWidthBoostOn->load() > 0.5f && (! soloActive || soloSection == SOLO_DIMENSION))  ? 1.0f : 0.0f);
    flowOnGain.setTargetValue       ((pFlowOn->load()       > 0.5f && (! soloActive || soloSection == SOLO_HYPERDRIVE)) ? 1.0f : 0.0f);
    posOnGain.setTargetValue        ((pPosOn->load()        > 0.5f && (! soloActive || soloSection == SOLO_POSITION))   ? 1.0f : 0.0f);
    rayOnGain.setTargetValue        ((pRayOn->load()        > 0.5f && (! soloActive || soloSection == SOLO_RAY))        ? 1.0f : 0.0f);
   #if SPACEX_RAYE_UI == 1
    rayDepthSmoothed.setTargetValue (juce::jlimit (0.0f, 1.0f, pRayAmount->load() * 0.01f));   // Amount stufenlos
   #else
    rayDepthSmoothed.setTargetValue (rayStrengthToDepth (pRayStrength->load()));
   #endif
    // LIFE regelt auch RAYE (User, Runde 39): 0 % = alles steht still.
    rayLifeSmoothed.setTargetValue (juce::jlimit (0.0f, 1.0f, pLife->load() * 0.01f));
    // Mono-Check ist ein reines Monitoring-Utility, kein Solo-Ziel.
    monoCheckGain.setTargetValue    (pMonoCheck->load() > 0.5f ? 1.0f : 0.0f);
    // A/B-Dry-Vergleich: nur relevant, waehrend Mono-Check selbst aktiv ist
    // (GUI verhindert das Einschalten ohnehin, hier zusaetzlich abgesichert),
    // sonst hat der Blend keine hoerbare Wirkung.
    monoDryBlend.setTargetValue     (pMonoDry->load() > 0.5f ? 1.0f : 0.0f);

    // Distance/Elevate-Koeffizienten nur einmal pro Block neu berechnen
    // (kein Audio-Rate-Modulationsziel, beides bewusst traege/subtile
    // Regler) - spart Trig-Berechnungen gegenueber Pro-Sample-Update.
    const float distTargetForCoeffs = distanceSmoothed.getTargetValue();
    const float cutoffHz = juce::jmap (distTargetForCoeffs, 0.0f, 1.0f, 20000.0f, 2200.0f);
    const float distanceLpfCoeff = std::exp (-2.0f * juce::MathConstants<float>::pi * cutoffHz / (float) currentSampleRate);

    const float elevateTargetForCoeffs = elevateSmoothed.getTargetValue();
    // 9 kHz war Luft, nicht Hoehe. 6,5 kHz mit breiterem Q rueckt eine Quelle
    // wirklich nach vorne/oben - dort ist das Ohr aber empfindlicher, deshalb
    // gleichzeitig weniger Anhebung (6 -> 4,5 dB), damit es eine Nuance bleibt
    // und kein EQ-Ersatz wird (User).
    updatePeakingCoeffs (elevateCoeffs, currentSampleRate, 6500.0f, elevateTargetForCoeffs * 4.5f, 0.7f);

    // ===== PRISM: Bandgrenzen einmal pro Block =====
    // Das Band wird bewusst nie enger als Faktor 1,5 zugelassen. Ein sehr
    // schmales verbreitertes Band klingt resonant bis telefonartig - das ist
    // als Effekt reizvoll, als versehentliche Einstellung aber nur aergerlich.
    {
        const bool prismOnRaw = pPrismOn->load() > 0.5f;
        float loHz = juce::jlimit (20.0f, 20000.0f, pPrismLo->load());
        float hiHz = juce::jlimit (20.0f, 20000.0f, pPrismHi->load());
        if (hiHz < loHz * 1.5f)
            hiHz = loHz * 1.5f;

        // Umfasst das Band praktisch das ganze Spektrum, bringt der Filterpfad
        // nichts - dann wird er uebersprungen und das Ergebnis ist bitgenau
        // das bisherige.
        const bool coversAll = (loHz <= 25.0f && hiHz >= 19000.0f);
        // Runde 31: Der Focus-Bereich ist raus (User-Entscheidung). Begruendung
    // in seinen Worten: "es veraendert komplett die Balance der Frequenzen.
    // Vocals klingen je nach setting deutlich praesenter, oder basslaestiger.
    // Das veraendert den mix. Das wollen wir mit dem Plugin nicht erreichen."
    // Genau richtig - ein Imager, der nebenbei den Klang verbiegt, ist in
    // einem fertigen Mix gefaehrlich. Was vom Konzept bleibt, ist der
    // Bass-Guard bei 120 Hz: der schuetzt, statt zu faerben.
    //
    // Die Parameter bleiben vorerst bestehen (sie fliegen zusammen mit
    // Vision-Width, Elevate und den Polarity-Slots in einem Durchgang raus),
    // wirken aber nicht mehr.
    juce::ignoreUnused (prismOnRaw, coversAll);
    prismActive = false;
        // Runde 30: die drei Focus-Bypass-Schalter sind aus der Oberflaeche
    // verschwunden (User: "3 Focus Knobs wieder rueckgaengig machen") - drei
    // Schalter fuer ein Routing, das man nicht hoeren kann. Der Focus wirkt
    // jetzt einheitlich auf alle drei Sektionen. Die Parameter bleiben
    // bestehen, werden aber bewusst ignoriert, damit alte Presets kein
    // unsichtbares Sonderverhalten mehr ausloesen koennen.
    prismGalaxyActive = prismActive;
        prismDimActive    = prismActive;   // siehe prismGalaxyActive
        prismVisActive    = prismActive;   // siehe prismGalaxyActive

        if (prismActive)
        {
            updateHighpassCoeffs (prismHpCoeffs, currentSampleRate, loHz);
            updateLowpassCoeffs  (prismLpCoeffs, currentSampleRate, hiHz);
        }
    }

    // Auto-Pan LFO Rate: entweder Host-Sync (Bars) oder freie Hz-Rate.
    const bool speedSync = pSpeedSync->load() > 0.5f;
    // BPM wird jetzt VOR der Sync-Verzweigung gelesen, weil auch RAYE-Pair
    // sie braucht (halber Takt als Obergrenze, siehe unten).
    double bpm = 120.0;
    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
            if (auto hostBpm = pos->getBpm())
                if (*hostBpm > 1.0)
                    bpm = *hostBpm;
    }
    double cycleSeconds;
    if (speedSync)
    {
        const int speedIndex = juce::jlimit (0, 7, (int) std::round (pSpeed->load()));
        const double secondsPerBar = (60.0 / bpm) * 4.0;
        cycleSeconds = juce::jmax (0.001, (double) speedBarValues[(size_t) speedIndex] * secondsPerBar);
    }
    else
    {
        double hz = juce::jmax (0.01, (double) pSpeedRate->load());
        // Hyperdrive-Mod: relative Geschwindigkeitsschwankung um die
        // aktuell eingestellte Speed-Rate (nur moeglich ohne Sync, siehe
        // hyperdriveModOn weiter oben). Speed hat keinen Neutralwert, daher
        // moduliert es immer mit, wenn Hyperdrive-Mod an ist - war schon
        // immer relativ zum eingestellten Wert (Speed war hier bereits das
        // Vorbild fuer die anderen Regler), Tiefe = bis zu +-20% relative
        // Schwankung bei vollem Tiefe-Regler (siehe modDepthCurve()).
        if (hyperdriveModOn)
            hz = juce::jmax (0.01, hz * (1.0 + (double) hyperdriveSine * (double) hyperdriveDepthFrac));
        currentSpeedLiveHz.store ((float) hz, std::memory_order_relaxed);
        cycleSeconds = 1.0 / hz;
    }
    const double phaseInc = (1.0 / cycleSeconds) * (1.0 / currentSampleRate);
    const bool pulseOn = pPulse->load() > 0.5f;
    float lastPanPos = 0.0f;   // fuer die Live-Anzeige im Flow-Regler (Ring + Punkt)

    // ===== RAY (Phaser): Blockkonstanten =====
    // Pair (User-Korrektur: "nicht in Sync sondern immer halb so schnell und
    // maximal 1/2 bar"): der Phaser laeuft dann mit der HALBEN Hyperdrive-
    // Rate, nie schneller als ein halber Takt. Ein Phaser, der im Tempo des
    // Auto-Pans wobbelt, ist zu nervoes - halb so schnell sitzt er darunter
    // wie eine langsame Welle unter einer schnellen. Eigene Phase, also nicht
    // phasenstarr an den Pan-LFO gekoppelt, nur an sein Tempo.
    const bool  rayPair     = pRayPair->load() > 0.5f;
    const double halfBarSeconds = (60.0 / bpm) * 2.0;
   #if SPACEX_RAYE_UI == 1
    // Runde 49 (User: "dann speed weg. noch simpler."): kein Speed-Regler
    // mehr. Die Grundrate ist fest (0.15 Hz - der bisherige Default), das
    // Tempo macht der Charakter ueber rateMul. FAST legt pauschal 30 % drauf.
    const double rayCycle   = rayPair ? juce::jmax (cycleSeconds * 2.0, halfBarSeconds)
                                      : 1.0 / 0.15;
    const RayCharacter& rayChar = rayCharacterFor ((int) std::round (pRayChar->load()));
    const double rayFastMul = (pRayFast->load() > 0.5f) ? 1.3 : 1.0;
    const double rayPhaseInc = (1.0 / rayCycle) / currentSampleRate * (double) rayChar.rateMul * rayFastMul;
   #else
    const double rayCycle   = rayPair ? juce::jmax (cycleSeconds * 2.0, halfBarSeconds)
                                      : 1.0 / (double) juce::jmax (0.01f, pRayRate->load());
    const double rayPhaseInc = (1.0 / rayCycle) / currentSampleRate;
   #endif
    float lastRayLfo = 0.0f;
    // Vorberechnung fuer die Allpass-Koeffizienten: a = (t-1)/(t+1) mit
    // t = tan(pi*f/fs). Statt tan() je Sample und Stufe wird die Sweep-
    // Frequenz einmal je Sample aus dem LFO abgeleitet und daraus EIN
    // Koeffizient je Kanal, den alle vier Stufen teilen (klassische
    // Phaser-Topologie, spart drei Viertel der Rechnung).
    const float rayPiOverFs = juce::MathConstants<float>::pi / (float) currentSampleRate;
    float lastMoveS  = 0.0f;   // letzter geglaetteter Flow-Wert (fuer die Live-Anzeige)

    // ===== AUTO GAIN: Rampe fuer diesen Block =====
    // Angewandt wird der im VORIGEN Block berechnete Wert. Ein Block Versatz
    // in der Regelstrecke ist bei rund einer Sekunde Zeitkonstante belanglos
    // und erspart eine zweite Messschleife.
    // Bewegt sich irgendein Parameter, oeffnet das Messfenster neu. Die
    // Pruefsumme laeuft ueber ALLE Parameter, damit auch Host-Automation und
    // ein Preset-Wechsel erfasst werden, nicht nur Mausbewegungen in der GUI.
    {
        float sum = 0.0f;
        for (auto* p : getParameters())
            sum += p->getValue();
        if (std::abs (sum - autoGainParamSum) > 1.0e-6f)
        {
            autoGainParamSum = sum;
            autoGainMeasureSamples = (int) (currentSampleRate * 1.5);
        }
    }
    const bool  bassGuardOn = pBassGuard->load() > 0.5f;
    // Setzt nur ein Flag, wenn sich wirklich etwas geaendert hat; die
    // Maske wird im naechsten FFT-Frame neu gerechnet.
    {
        // >= 19990 heisst "aus": weit ueber Nyquist schicken, damit auch die
        // weiche Flanke der Maske komplett oberhalb des Hoerbaren liegt.
        const float air = pHorizon->load();   // 0..100 %, siehe AIR
        lcrExtractor.setExtractionRange (bassGuardOn ? 120.0f : 20.0f,
                                         air < 0.5f ? 96000.0f
                                                    : 20000.0f * std::pow (0.025f, air * 0.01f));
    }
    const bool  autoGainOn = pAutoGain->load() > 0.5f;
    const float agFrom = autoGainApplied;
    const float agTo   = autoGainOn ? autoGainTarget : 1.0f;
    const float agStep = (agTo - agFrom) / (float) juce::jmax (1, numSamples);
    float agNow = agFrom;
    double agInAcc = 0.0, agOutAcc = 0.0;

    // Fuer den Korrelationsmesser: einfache Blockweise-Summen statt einer
    // laufenden Glaettung - guenstig und fuer eine reine Anzeige praezise genug.
    double corrSumLR = 0.0, corrSumLL = 0.0, corrSumRR = 0.0;
    // Fuer die audio-reaktiven Sterne: Side- vs. Mid-Energie ueber den Block.
    double sideEnergySum = 0.0, midEnergySum = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        float l = left[i];
        float r = right[i];
        // Rohes Eingangssignal (VOR jeglicher Verarbeitung) fuer den
        // Mono-Dry-A/B-Vergleich (siehe ID_MONO_DRY weiter unten) - muss vor
        // allen weiteren Schritten gesichert werden, da l/r ab hier
        // schrittweise ueberschrieben werden.
        const float dryL = l;
        const float dryR = r;

        // Auto Gain, Messpunkt EIN: beide Kanaele einzeln K-gewichtet, dann
        // die Energien addiert (so macht es der Loudness-Standard). Der Wert
        // geht durch eine Verzoegerungsleitung in Laenge der gemeldeten
        // Latenz, damit er mit dem Ausgang desselben Moments verglichen wird.
        {
            const float kL = kwInShelfL.process (kwInHpL.process (dryL, kwHpCoeffs), kwShelfCoeffs);
            const float kR = kwInShelfR.process (kwInHpR.process (dryR, kwHpCoeffs), kwShelfCoeffs);
            const float e  = kL * kL + kR * kR;
            const int   dn = (int) autoGainInDelay.size();
            if (dn > 0)
            {
                const float delayed = autoGainInDelay[(size_t) autoGainInPos];
                autoGainInDelay[(size_t) autoGainInPos] = e;
                autoGainInPos = (autoGainInPos + 1) % dn;
                // Nur wenn Galaxy wirklich armiert ist, hat der Ausgang eine
                // Verzoegerung - sonst wuerde hier ein 85 ms alter Eingang mit
                // einem aktuellen Ausgang verglichen. Bei 1024 fiel das kaum
                // auf, bei 4096 waere es ein echter Messfehler.
                agInAcc += (double) (galaxyActivateRaw ? delayed : e);
            }
            else
            {
                agInAcc += (double) e;
            }
        }

        // Alle geglaetteten Regler-/Section-Werte fuer dieses Sample holen.
        const float sensS  = sensSmoothed.getNextValue();
        const float blendS = blendSmoothed.getNextValue();
        const float widthS = widthSmoothed.getNextValue();
        const float boostS = boostSmoothed.getNextValue();
        const float moveS  = movementSmoothed.getNextValue();
        const float bendS  = bendSmoothed.getNextValue();
        const float dGain  = driftOnGain.getNextValue();
        const float pGain  = polOnGain.getNextValue();
        const float polLS  = polLFlip.getNextValue();
        const float polRS  = polRFlip.getNextValue();
        const float polSlotS[4] = { polSlotGain[0].getNextValue(), polSlotGain[1].getNextValue(),
                                    polSlotGain[2].getNextValue(), polSlotGain[3].getNextValue() };
        const float wbGain = widthBoostOnGain.getNextValue();
        const float flGain = flowOnGain.getNextValue();
        const float offsetS   = offsetSmoothed.getNextValue();
        const float posWidthS = posWidthSmoothed.getNextValue();
        const float posGain   = posOnGain.getNextValue();
        juce::ignoreUnused (posGain);   // VISION aufgeloest, siehe unten
        const float monoGain  = monoCheckGain.getNextValue();
        const float monoDryS  = monoDryBlend.getNextValue();
        lastMoveS = moveS;

        // LCR-Blend-Gains je Sample aus dem geglaetteten Blend-Wert ableiten:
        // 0 = neutral (Original), +1 = nur L+R (Center raus), -1 = nur
        // Center (L/R raus). Bei 0 gilt exakt gL=gC=gR=1, d.h. lOnly+center=L,
        // rOnly+center=R -> Originalpegel.
        float gL, gC, gR;
        if (blendS >= 0.0f)
        {
            gC = 1.0f - blendS;
            gL = 1.0f;
            gR = 1.0f;
        }
        else
        {
            const float b = -blendS;
            gC = 1.0f;
            gL = 1.0f - b;
            gR = 1.0f - b;
        }

        // Polarity-Slot 1: VOR Galaxy. Ein einseitiger Flip vor der LCR-
        // Extraktion laesst die Engine ein gegenphasiges Bild sehen - der
        // drastischste der vier Slots.
        {
            // Klickfrei (User: "Knackser bei Polarity L/R und 1-4"): L/R-Flip
            // und Slot-Wahl sind geglaettete Gains, kein harter Schalter mehr.
            const float slot = polSlotS[0];
            l *= 1.0f - 2.0f * pGain * polLS * slot;
            r *= 1.0f - 2.0f * pGain * polRS * slot;
        }

        // 1) LCR (optional vorgeschaltet, Reihenschaltung). Die Engine laeuft
        // durchgehend ("warm"), sobald galaxyActivateRaw an ist - auch wenn
        // der Sektions-Bypass oder Solo das Ergebnis gerade stummschaltet -
        // damit sich beim Umschalten NIE die gemeldete Latenz aendert und
        // NIE ein Interrupt/Klick entsteht. Statt hart durchzuschalten wird
        // zwischen dem verarbeiteten ("wet") und einem latenzgleich
        // verzoegerten Original ("dry", ueber lcrDryDelay*) weich
        // ueberblendet (lcrWetGain, siehe oben) - identisches Prinzip wie
        // bei allen anderen Sektionen (Drift/Width/...), nur mit eigener
        // Verzoegerungsleitung, weil das Original sonst nicht mehr zeitlich
        // zum verarbeiteten Signal passen wuerde.
        // MIX-Original: das latenzgleiche, UNBEARBEITETE Signal (vor Galaxy).
        // Bekommt unten dieselbe Drift-Verzoegerung und dieselben Polarity-
        // Flips wie der verarbeitete Pfad, damit MIX wirklich zwischen
        // "Original" und "alles" mischt (User: "Mix-Regler geht nicht mehr"
        // - vorher lag der Abgriff HINTER Galaxy, Galaxy war also nie im Mix).
        float mixDryL = l, mixDryR = r;
        if (galaxyActivateRaw)
        {
            float lOnly = 0.0f, center = 0.0f, rOnly = 0.0f;
            if (! galaxyEnginePaused)
                lcrExtractor.processSample (l, r, sensS, lOnly, center, rOnly);
            const float wetL = lOnly * gL + center * gC;
            const float wetR = rOnly * gR + center * gC;

            // Eigene Namen (lcrDryL/R statt dryL/R) - vermeidet Shadowing der
            // gleichnamigen Variablen ganz oben im Sample-Loop (die dort das
            // ROHE Eingangssignal fuer den Mono-Dry-Vergleich halten, siehe
            // Kommentar dort - unabhaengiger Zweck).
            float lcrDryL = l, lcrDryR = r;
            const int dSize = (int) lcrDryDelayL.size();
            if (dSize > 0)
            {
                lcrDryL = lcrDryDelayL[(size_t) lcrDryWritePos];
                lcrDryR = lcrDryDelayR[(size_t) lcrDryWritePos];
                lcrDryDelayL[(size_t) lcrDryWritePos] = l;
                lcrDryDelayR[(size_t) lcrDryWritePos] = r;
                lcrDryWritePos = (lcrDryWritePos + 1) % dSize;
            }

            const float wg = lcrWetGain.getNextValue();
            float gxL = lcrDryL + (wetL - lcrDryL) * wg;
            float gxR = lcrDryR + (wetR - lcrDryR) * wg;
            // Bass-Guard sitzt jetzt IM Extractor als Maske pro Bin
            // (setExtractionRange weiter oben). Der frueher hier stehende
            // Biquad lag auf der DIFFERENZ zwischen verarbeitetem und
            // trockenem Signal und hat dabei die Phase verbogen - genau
            // die Art Eingriff, die man als Bassresonanz hoert.
            // "Band Limits Galaxy": nur der ANTEIL der Galaxy-Bearbeitung
            // innerhalb des Bandes bleibt stehen. Dieselbe Rechnung wie bei
            // Dimension/Vision (dry + band(wet - dry)), deshalb ist das
            // Ergebnis bei einem Band ueber das ganze Spektrum wieder exakt
            // das alte. Ausserhalb des Bandes steht das Original - der Bass
            // bleibt also unangetastet, statt in Mitte und Seiten zerlegt zu
            // werden.
            if (prismGalaxyActive)
            {
                const float dL = gxL - lcrDryL;
                const float dR = gxR - lcrDryR;
                gxL = lcrDryL + prismGalLpL.process (prismGalHpL.process (dL, prismHpCoeffs), prismLpCoeffs);
                gxR = lcrDryR + prismGalLpR.process (prismGalHpR.process (dR, prismHpCoeffs), prismLpCoeffs);
            }
            l = gxL;
            r = gxR;
            mixDryL = lcrDryL;
            mixDryR = lcrDryR;
        }

        // Polarity-Slot 1: nach LCR, vor Haas/Drift. Statt eines harten
        // Vorzeichenwechsels wird zwischen Original (pGain=0) und
        // invertiert (pGain=1) weich ueberblendet - vermeidet Knacksen
        // beim Umschalten des Polarity-Power-Icons.
        // Polarity-Slot 2: nach Galaxy, vor Dimension (Timewarp dazwischen ist
        // pro Kanal linear und aendert am Flip nichts).
        {
            // Klickfrei (User: "Knackser bei Polarity L/R und 1-4"): L/R-Flip
            // und Slot-Wahl sind geglaettete Gains, kein harter Schalter mehr.
            const float slot = polSlotS[1];
            l *= 1.0f - 2.0f * pGain * polLS * slot;
            r *= 1.0f - 2.0f * pGain * polRS * slot;
            mixDryL *= 1.0f - 2.0f * pGain * polLS * slot;
            mixDryR *= 1.0f - 2.0f * pGain * polRS * slot;
        }

        // 2) Drift / Haas-Effekt + Bend (Micro-Pitch) - eigene Sektion.
        // Delay + Bend laufen IMMER (beide sind bei Ruhewerten von sich aus
        // ein sauberer 1:1-Durchlauf), das Ergebnis wird nur weich mit dem
        // Original ueberblendet - so entsteht beim Ein-/Ausschalten kein
        // Knacksen und der Puffer bleibt "warm".
        // Runde 38 (CPU): Verhaeltnis nur neu rechnen, wenn sich Shift
        // tatsaechlich geaendert hat - gleiches Ergebnis, bei stehendem Regler
        // aber keine zwei Potenzen pro Sample mehr.
        if (bendS != lastBendForRatio)
        {
            lastBendForRatio = bendS;
            bendL.setRatio (std::exp2 (-bendS / 1200.0f));
            bendR.setRatio (std::exp2 ( bendS / 1200.0f));
        }
        {
            const float lDrift = bendL.process (delayL.process (l));
            const float rDrift = bendR.process (delayR.process (r));

            // Balance-Kompensation weich einblenden (siehe oben) - nur die
            // per Haas verzoegerte Seite wird angehoben, die andere bleibt
            // unveraendert.
            const float balanceAmt = balanceOnGain.getNextValue();
            const float balanceGain = 1.0f + (balanceTargetCompGain - 1.0f) * balanceAmt;
            const float lDriftBal = balanceBoostsLeft ? lDrift * balanceGain : lDrift;
            const float rDriftBal = balanceBoostsLeft ? rDrift : rDrift * balanceGain;

           #if SPACEX_PARALLAX_UI != 1
            // Modus-Builds: Tilt auf das Parallax-Signal, dann der eigene Mix
            // gegen das Original und der Pegelausgleich des Wegpunkts -
            // entspricht (1-Mix)*Original + Mix*Tilt(Parallax) wie beim
            // Einstellen mit dem globalen Mix.
            {
                const float pxM  = pxMixSmoothed.getNextValue();
                const float pxG  = pxGainSmoothed.getNextValue();
                const float pxTL = pxTiltLSmoothed.getNextValue();
                const float pxTR = pxTiltRSmoothed.getNextValue();
                const float outL = (l + (lDriftBal * pxTL - l) * pxM) * pxG * pxPanLSmoothed.getNextValue();
                const float outR = (r + (rDriftBal * pxTR - r) * pxM) * pxG * pxPanRSmoothed.getNextValue();
                l = l + (outL - l) * dGain;
                r = r + (outR - r) * dGain;
            }
           #else
            l = l + (lDriftBal - l) * dGain;
            r = r + (rDriftBal - r) * dGain;
           #endif
            // Das MIX-Original bekommt KEINE Drift-Verzoegerung mehr (User:
            // "Timewarp ist wet auch wenn Mix auf 0") - bei 0 % ist es das
            // reine Original. Preis: mit Drift entsteht in Zwischenstellungen
            // ein leichter Kammfilter (Original + verzoegertes Signal).
        }

        // (Frueherer Slot "nach Haas" entfallen - siehe POL_POS_*-Kommentar im
        //  Header: klanglich identisch mit "nach Galaxy".)

        // ===== MIX-Abgriff =====
        // Das "Original" fuer MIX wird HIER abgegriffen - nach Galaxy und
        // Timewarp. Grund: ein Dry/Wet ueber die ganze Kette mischt bei
        // Timewarp das unverzoegerte Signal mit dem verzoegerten und erzeugt
        // einen Kammfilter (Klangfarbe statt Raum). Galaxy hat mit Orbit und
        // Timewarp mit Drift/Shift ohnehin je einen eigenen Mengenregler.
        // MIX regelt also die Menge von Dimension, Hyperdrive, Vision und
        // RAYE - alles Stufen, die sich sauber mischen lassen. Nebeneffekt:
        // keine Latenz-Verzoegerungsleitung mehr noetig, der Abgriff liegt
        // bereits hinter der Galaxy-Latenz.
        const float mixTapL = mixDryL;
        const float mixTapR = mixDryR;

        // 3) Mid/Side: Width + Boost - eigene Sektion. Bei Width=100%/
        // Boost=0dB ist m+s/m-s exakt wieder l/r (Identitaet), daher ist
        // auch hier ein weiches Ueberblenden statt harter Verzweigung
        // moeglich, ohne dass sich am Klang bei Neutralwerten etwas aendert.
        {
            const float m = 0.5f * (l + r);
            const float s = 0.5f * (l - r);
            // PRISM: statt das GESAMTE Side-Signal zu skalieren, wird nur der
            // Anteil innerhalb des Bandes skaliert.
            //   sWet = s + band(s) * (faktor - 1)
            // Bei faktor = 1 ergibt das exakt s (Identitaet bleibt erhalten),
            // und ohne PRISM ist band(s) = s, also s * faktor - also bitgenau
            // das bisherige Verhalten. Ausserhalb des Bandes bleibt das
            // Side-Signal voellig unberuehrt, es entstehen keine Kammfilter.
            const float sFactor = boostS * widthS;
            // Bass-Guard: skaliert wird nur der Anteil OBERHALB von 120 Hz.
            // Darunter bleibt das Side-Signal so, wie es hereinkam.
            // Filter laeuft immer mit (Zustand bleibt warm, kein Knacksen
            // beim Umschalten), benutzt wird er nur wenn eingeschaltet.
            const float sHigh  = bassGuardDim.process (s, bassGuardCoeffs);
            const float sGuard = bassGuardOn ? sHigh : s;
            float sWet;
            if (prismDimActive)
            {
                const float band = prismDimLp.process (prismDimHp.process (sGuard, prismHpCoeffs), prismLpCoeffs);
                sWet = s + band * (sFactor - 1.0f);
            }
            else
            {
                sWet = s + sGuard * (sFactor - 1.0f);
            }
            const float lWet = m + sWet;
            const float rWet = m - sWet;
            l = l + (lWet - l) * wbGain;
            r = r + (rWet - r) * wbGain;
        }

        // Polarity-Slot 3: nach Mid/Side, vor Auto-Pan (Default-Position).
        // Polarity-Slot 3: nach Dimension (M/S), vor Vision.
        {
            // Klickfrei (User: "Knackser bei Polarity L/R und 1-4"): L/R-Flip
            // und Slot-Wahl sind geglaettete Gains, kein harter Schalter mehr.
            const float slot = polSlotS[2];
            l *= 1.0f - 2.0f * pGain * polLS * slot;
            r *= 1.0f - 2.0f * pGain * polRS * slot;
        }

        // 4) Auto-Pan (Flow) - eigene Sektion.
        float lFlow = l, rFlow = r;
        if (moveS > 0.0001f)
        {
            float lfo;
            if (pulseOn)
            {
                // Puls-Modus: Ziel springt hart zwischen +1 (links) und -1
                // (rechts), wird aber ueber ein One-Pole-Filter nachgefuehrt,
                // damit der Wechsel deutlich haerter als Sinus ist, aber
                // nicht zu 100% ein Rechteck (kein Klick/Aliasing).
                const float pulseTarget = (autoPanPhase < 0.5) ? 1.0f : -1.0f;
                pulseSmoothState = pulseTarget + pulseSmoothCoeff * (pulseSmoothState - pulseTarget);
                lfo = pulseSmoothState;
            }
            else
            {
               #if SPACEX_CPU_OPT
                lfo = spacex::fastSinCycles (autoPanPhase);
               #else
                lfo = (float) std::sin (autoPanPhase * juce::MathConstants<double>::twoPi);
               #endif
            }
            const float panPos = juce::jlimit (-1.0f, 1.0f, lfo * moveS);
           #if SPACEX_CPU_OPT
            // Winkel (panPos+1)*pi/4 in Umdrehungen = (panPos+1)/8.
            const double turns = (double) (panPos + 1.0f) * 0.125;
            const float gPanL = spacex::fastCosCycles (turns) * juce::MathConstants<float>::sqrt2;
            const float gPanR = spacex::fastSinCycles (turns) * juce::MathConstants<float>::sqrt2;
           #else
            const float angle = (panPos + 1.0f) * (juce::MathConstants<float>::pi * 0.25f);
            const float gPanL = std::cos (angle) * juce::MathConstants<float>::sqrt2;
            const float gPanR = std::sin (angle) * juce::MathConstants<float>::sqrt2;
           #endif
            lFlow = l * gPanL;
            rFlow = r * gPanR;
            lastPanPos = panPos;
        }
        l = l + (lFlow - l) * flGain;
        r = r + (rFlow - r) * flGain;

        autoPanPhase += phaseInc;
        if (autoPanPhase >= 1.0) autoPanPhase -= 1.0;

        // (Frueherer Slot "nach Auto-Pan" entfallen - identisch mit Slot 3.)

        // 5) Position (Offset/Width/Distance/Elevate) - eigene Sektion, ganz
        // am Ende der Kette, nach Polarity-Slot 4. Alle vier Regler stehen
        // bei ihrem Ruhewert (Offset=0, Width=100%, Distance=0, Elevate=0)
        // exakt auf Identitaet, daher genuegt ein einziger gemeinsamer
        // Dry/Wet-Crossfade fuer das ganze Sektions-Power-Icon.
        {
            float lPos = l, rPos = r;

            // Offset: konstante-Leistung-Pan, identisch zum Auto-Pan-Ansatz.
            {
                const float panPos = juce::jlimit (-1.0f, 1.0f, offsetS);
               #if SPACEX_CPU_OPT
                // Nur neu rechnen, wenn sich Tilt bewegt (exakt gleiches Ergebnis).
                if (panPos != offsetPanCachePos)
                {
                    offsetPanCachePos = panPos;
                    const float angle = (panPos + 1.0f) * (juce::MathConstants<float>::pi * 0.25f);
                    offsetPanCacheL = std::cos (angle) * juce::MathConstants<float>::sqrt2;
                    offsetPanCacheR = std::sin (angle) * juce::MathConstants<float>::sqrt2;
                }
                lPos *= offsetPanCacheL;
                rPos *= offsetPanCacheR;
               #else
                const float angle = (panPos + 1.0f) * (juce::MathConstants<float>::pi * 0.25f);
                const float gPanL = std::cos (angle) * juce::MathConstants<float>::sqrt2;
                const float gPanR = std::sin (angle) * juce::MathConstants<float>::sqrt2;
                lPos *= gPanL;
                rPos *= gPanR;
               #endif
            }

            // TILT gehoert jetzt zu PARALLAX und haengt deshalb an DESSEN
            // Ein/Aus-Schalter, nicht mehr an dem der alten Vision-Sektion.
            // Die Rechenreihenfolge bleibt exakt wie bisher - nur der
            // Ueberblendpunkt wird aufgeteilt, damit Parallax aus wirklich
            // Tilt aus heisst.
            l = l + (lPos - l) * dGain;
            r = r + (rPos - r) * dGain;
            lPos = l; rPos = r;

            // Width: nochmalige, globale Side-Skalierung (unabhaengig von
            // DIMENSION/Expand weiter vorne).
            {
                const float m = 0.5f * (lPos + rPos);
                const float s = 0.5f * (lPos - rPos);
                // Dieselbe Bandbegrenzung wie im Dimension-Block, aber mit
                // EIGENEM Filterzustand - hier liegt ein anderes Side-Signal
                // an (Drift, Polarity und Auto-Pan sind inzwischen passiert).
                float sWet;
                if (prismVisActive)
                {
                    const float band = prismPosLp.process (prismPosHp.process (s, prismHpCoeffs), prismLpCoeffs);
                    sWet = s + band * (posWidthS - 1.0f);
                }
                else
                {
                    sWet = s * posWidthS;
                }
                lPos = m + sWet;
                rPos = m - sWet;
            }

            // Distance: One-Pole-Tiefpass (Naehe/Ferne) + leichte Pegelabsenkung.
            {
                distanceLpfL += (1.0f - distanceLpfCoeff) * (lPos - distanceLpfL);
                distanceLpfR += (1.0f - distanceLpfCoeff) * (rPos - distanceLpfR);
                // Absenkung am Anschlag: frueher 0.3 (-3.1 dB), jetzt 1 dB
                // weniger (-2.1 dB) - der Hoehenabfall traegt den Tiefeneindruck
                // ohnehin, die Pegelabsenkung soll ihn nur stuetzen (User).
                const float distGain = 1.0f - juce::jlimit (0.0f, 1.0f, distanceSmoothed.getNextValue()) * 0.215f;
                lPos = distanceLpfL * distGain;
                rPos = distanceLpfR * distGain;
            }

            // Elevate: rein lineares Peaking-EQ (kein Waveshaping), siehe
            // updatePeakingCoeffs() - positiv = leichte Anhebung ~9kHz
            // ("erhoeht" wirkend), negativ = Absenkung ("geerdet" wirkend).
            {
                lPos = elevateStateL.process (lPos, elevateCoeffs);
                rPos = elevateStateR.process (rPos, elevateCoeffs);
            }

            // Der Rest der alten Vision-Stufe ist DEPTH, und das gehoert zu
            // DIMENSION - also dessen Schalter. posGain wird nicht mehr
            // gebraucht: die Sektion, zu der er gehoerte, gibt es nicht mehr.
            l = l + (lPos - l) * wbGain;
            r = r + (rPos - r) * wbGain;
        }

        // Polarity-Slot 4: nach Vision (M/S-Width), vor RAYE. RAYE selbst ist
        // pro Kanal linear - ein Slot dahinter braechte nichts Neues.
        {
            // Klickfrei (User: "Knackser bei Polarity L/R und 1-4"): L/R-Flip
            // und Slot-Wahl sind geglaettete Gains, kein harter Schalter mehr.
            const float slot = polSlotS[3];
            l *= 1.0f - 2.0f * pGain * polLS * slot;
            r *= 1.0f - 2.0f * pGain * polRS * slot;
        }

        // ===== RAY: Stereo-Phaser =====
        // Sitzt nach Position, also auf dem fertigen Stereobild - der Phaser
        // faerbt das Ergebnis, er baut es nicht. Zero-Latency (reine
        // Allpaesse), Solo-Gating wie ueberall ueber rayGain.
        {
            const float rayGain  = rayOnGain.getNextValue() * rayLifeSmoothed.getNextValue();
            // rayDepthRaw: 0 = Stufe "Off" (kein Effekt), 1/3..1 = leicht..stark.
            // Der Wet-Anteil faehrt zwischen Off und Light weich auf null.
            const float rayDepthRaw = rayDepthSmoothed.getNextValue();
           #if SPACEX_RAYE_UI == 1
            // Runde 48 (User: "die Modi sind zu leise"): Amount wirkt direkt
            // auf die Tiefe (0 = aus, 100 % = volle Tiefe) statt erst ab einem
            // Drittel, und der Anteil geht weiter hoch (siehe mix unten).
            const float offFade  = juce::jlimit (0.0f, 1.0f, rayDepthRaw * 6.0f);
            const float depth01  = juce::jlimit (0.0f, 1.0f, rayDepthRaw);
           #else
            const float offFade  = juce::jlimit (0.0f, 1.0f, rayDepthRaw * 3.0f);
            const float depth01  = juce::jlimit (0.0f, 1.0f, (rayDepthRaw - 1.0f / 3.0f) * 1.5f);
           #endif

            if (rayGain > 0.0001f && offFade > 0.0005f)
            {
                // LFO: entweder eigene Phase oder an Hyperdrive gekoppelt
                // (Pair). Rechter Kanal eine Viertelperiode voraus -> die
                // Kerben kreisen durchs Stereobild statt nur zu wobbeln.
                const double ph = rayPhase;   // Pair aendert nur das Tempo (siehe rayCycle)
               #if SPACEX_RAYE_UI == 1
                const double stereoOff = (double) rayChar.stereoOffset;
               #else
                const double stereoOff = 0.25;
               #endif
               #if SPACEX_CPU_OPT
                const float lfoL = spacex::fastSinCycles (ph);
                lastRayLfo = lfoL;
               #else
                const float lfoL = (float) std::sin (ph * juce::MathConstants<double>::twoPi);
                const float lfoR = (float) std::sin ((ph + stereoOff) * juce::MathConstants<double>::twoPi);
                lastRayLfo = lfoL;
               #endif

                // Staerke skaliert drei Dinge gemeinsam: Sweep-Breite in
                // Oktaven, Rueckkopplung und Wet-Anteil. Nur den Anteil zu
                // aendern haette den Effekt leiser gemacht, nicht staerker.
                // Alle drei Stufen zurueckgenommen (User: "soll eher wie
                // Movement sein - wenn ich einen Phaser will, nehme ich
                // den"): flacherer Sweep, weniger Rueckkopplung, flachere
                // Kerben. Strong liegt jetzt etwa beim alten Light/Medium.
                // Dritter Anlauf (User: "wieder bisschen staerker, aber nicht
                // zu viel"): zwischen der ersten und der zweiten Fassung.
               #if SPACEX_RAYE_UI == 1
                const float sweepOct = (1.3f + depth01 * 1.7f) * rayChar.sweepMul;
               #else
                const float sweepOct = 1.3f + depth01 * 1.7f;      // 1,3 .. 3,0 Oktaven (urspruenglich 1,6 .. 3,8; zuletzt 1,1 .. 2,5)
               #endif
                // Runde 34 (User: "Raye ist auf Minimum schon zu viel - die
                // Haelfte reicht; insgesamt runter skalieren"): Anteil und
                // Rueckkopplung aller Stufen halbiert.
               #if SPACEX_RAYE_UI == 1
                // Deutlich mehr Weg nach oben, damit man die Charaktere hoert.
                const float feedback = (0.04f + depth01 * 0.20f) * rayChar.fbMul;
                const float mix      = (0.10f + depth01 * 0.28f) * rayChar.mixMul;
                const float centreHz = rayChar.centreHz;
               #else
                const float feedback = 0.04f + depth01 * 0.11f;    // 0,04 .. 0,15 (vorher 0,08 .. 0,30)
                const float mix      = 0.10f + depth01 * 0.10f;    // 0,10 .. 0,20 (vorher 0,20 .. 0,40; 0,5 = volle Kerben)
                constexpr float centreHz = 900.0f;
               #endif

                auto coeffFor = [&] (float lfo) -> float
                {
                    const float f = centreHz * std::exp2 (0.5f * sweepOct * lfo);
                    const float t = std::tan (juce::jlimit (30.0f, 0.45f * (float) currentSampleRate, f) * rayPiOverFs);
                    return (t - 1.0f) / (t + 1.0f);
                };
               #if SPACEX_CPU_OPT
                // CPU-Build: exp2/tan nur alle 8 Samples, exakt an den
                // Rasterpunkten, dazwischen linear - bei LFO-Tempi unter
                // 2 Hz liegt der Unterschied weit unter jeder Hoerschwelle.
                if (rayCoefCountdown <= 0)
                {
                    constexpr int kStep = 8;
                    if (! rayCoefValid)
                    {
                        rayAL = coeffFor (lfoL);
                        rayAR = coeffFor (spacex::fastSinCycles (ph + stereoOff));
                        rayCoefValid = true;
                    }
                    const double phNext = ph + rayPhaseInc * (double) kStep;
                    const float tL = coeffFor (spacex::fastSinCycles (phNext));
                    const float tR = coeffFor (spacex::fastSinCycles (phNext + stereoOff));
                    rayAStepL = (tL - rayAL) / (float) kStep;
                    rayAStepR = (tR - rayAR) / (float) kStep;
                    rayCoefCountdown = kStep;
                }
                const float aL = rayAL, aR = rayAR;
                rayAL += rayAStepL;
                rayAR += rayAStepR;
                --rayCoefCountdown;
               #else
                const float aL = coeffFor (lfoL);
                const float aR = coeffFor (lfoR);
               #endif

                // ("Sides"-Modus - Phaser nur auf L-R - wieder entfernt: User
                //  hoerte kaum einen Unterschied.)
                // Allpass 1. Ordnung, transponierte Direktform:
                //   y = a*x + z;  z' = x - a*y
                float xL = l + rayFbL * feedback;
                float xR = r + rayFbR * feedback;
                for (int k = 0; k < kRayStages; ++k)
                {
                    const float yL = aL * xL + rayApL[k]; rayApL[k] = xL - aL * yL; xL = yL;
                    const float yR = aR * xR + rayApR[k]; rayApR[k] = xR - aR * yR; xR = yR;
                }
                // Rueckkopplung leicht begrenzen, damit sie nie aufschaukelt.
                rayFbL = juce::jlimit (-2.0f, 2.0f, xL);
                rayFbR = juce::jlimit (-2.0f, 2.0f, xR);

                const float wetL = l + (xL - l) * mix;
                const float wetR = r + (xR - r) * mix;
                l = l + (wetL - l) * rayGain * offFade;
                r = r + (wetR - r) * rayGain * offFade;
            }
            else
            {
                // Aus: Zustaende leer halten, damit beim Einschalten nichts
                // Altes nachklingt.
                for (int k = 0; k < kRayStages; ++k) { rayApL[k] = 0.0f; rayApR[k] = 0.0f; }
                rayFbL = rayFbR = 0.0f;
                rayCoefValid = false;
                rayCoefCountdown = 0;
            }

            rayPhase += rayPhaseInc;
            if (rayPhase >= 1.0) rayPhase -= 1.0;
        }

        // ===== MIX: Dimension/Hyperdrive/Vision/RAYE gegen den Abgriff =====
        // Siehe mixTap oben. Die Polarity-Slots 3 und 4 liegen HINTER dem
        // Abgriff - sie werden auf den Abgriff uebertragen, sonst wuerde ein
        // geflippter Kanal bei 50 % gegen sich selbst ausloeschen.
        {
            const float slotFlip34L = (1.0f - 2.0f * pGain * polLS * polSlotS[2]) * (1.0f - 2.0f * pGain * polLS * polSlotS[3]);
            const float slotFlip34R = (1.0f - 2.0f * pGain * polRS * polSlotS[2]) * (1.0f - 2.0f * pGain * polRS * polSlotS[3]);
            const float dL = mixTapL * slotFlip34L;
            const float dR = mixTapR * slotFlip34R;
            const float mixS = mixSmoothed.getNextValue();
            l = dL + (l - dL) * mixS;
            r = dR + (r - dR) * mixS;
        }

        // Bug-Fix (User-Feedback: "Mono Check -> Goniometer und Corr Meter
        // sollen davon nicht beeinflusst sein."): Schnappschuss VOR dem
        // Mono-Check-Block (siehe direkt darunter) - dieser Wert (nicht l/r)
        // speist ab jetzt Goniometer und Korrelationsmesser, damit ein
        // aktiver Mono-Check (der ja absichtlich das Stereobild veraendert,
        // um es zu pruefen) die visuellen Anzeigen nicht mit verzerrt/
        // "mono-quetscht". Vol-Trim und Chaos-Duck (beide NACH Mono-Check,
        // siehe unten) sollen aber weiterhin sichtbar bleiben - werden daher
        // gleich auch auf meterL/meterR angewendet.
        float meterL = l;
        float meterR = r;

        // Mono-Check: reines Monitoring-Utility, kein Solo-Ziel, greift immer
        // ganz am Ende (nach Position), damit es wirklich das finale Signal
        // pruefen kann. Der A/B-Dry-Vergleich (monoDryS) blendet dabei
        // zusaetzlich weich zwischen "bearbeitetes Signal" und "rohes
        // Eingangssignal" um - so laesst sich pruefen, wie das Material OHNE
        // das Plugin in Mono klingen wuerde (User-Feedback). Wirkt jetzt NUR
        // noch auf das tatsaechliche Ausgangssignal (l/r), NICHT mehr auf
        // Goniometer/Korrelationsmesser (siehe meterL/meterR oben).
        {
            // Runde 45 (User-Bug "Sprung beim Umschalten"): das Original
            // latenzgleich nehmen (bypassDry) - mit Galaxy lag das rohe
            // Eingangssignal sonst um die FFT-Latenz zu frueh.
            const float srcL = l + (bypassDryL[(size_t) i] - l) * monoDryS;
            const float srcR = r + (bypassDryR[(size_t) i] - r) * monoDryS;
            const float monoSum = 0.5f * (srcL + srcR);
            l = l + (monoSum - l) * monoGain;
            r = r + (monoSum - r) * monoGain;
        }

        // ===== AUTO GAIN =====
        // Sitzt bewusst VOR dem VOL-Trim: VOL soll ein echter Trim bleiben.
        // Laege Auto Gain dahinter, wuerde es jede VOL-Bewegung wieder
        // wegregeln und der Regler waere funktionslos.
        // Gemessen wird das bearbeitete Signal VOR dem Ausgleich - damit ist
        // die Regelung offen und nicht rueckgekoppelt, also vorhersagbar.
        {
            const float kL = kwOutShelfL.process (kwOutHpL.process (l, kwHpCoeffs), kwShelfCoeffs);
            const float kR = kwOutShelfR.process (kwOutHpR.process (r, kwHpCoeffs), kwShelfCoeffs);
            agOutAcc += (double) (kL * kL + kR * kR);
        }
        agNow += agStep;
        l *= agNow;  r *= agNow;
        meterL *= agNow;  meterR *= agNow;

        // VOL-Trim: allerletzte Gain-Stufe der GESAMTEN Kette (nach
        // Mono-Check, "am ende" - User-Bestaetigung), wirkt daher auch auf
        // alles, was Goniometer und Korrelationsmesser anzeigen - deshalb
        // gleichermassen auf meterL/meterR angewendet.
        // Bug-Fix/User-Wunsch: "VOL darf keinen Einfluss haben wenn bei Mono
        // Check Dry aktiviert ist. Falls Dry nicht aktiviert (nur Mono
        // Check), dann muss VOL Einfluss drauf haben." - beim A/B-Dry-
        // Vergleich soll man das ROHE Eingangssignal hoeren, ohne dass der
        // Trim-Regler das verfaelscht; ohne Dry bleibt VOL weiterhin nuetzlich
        // fuers manuelle Gain-Matching gegen Mono-Check. monoDryS (derselbe
        // bereits geglaettete Blend-Wert wie oben) blendet VOL dabei weich
        // auf 1.0 (= kein Effekt) aus, statt hart umzuschalten.
        {
            const float volGain = volTrimSmoothed.getNextValue();
            const float effectiveVolGain = 1.0f + (volGain - 1.0f) * (1.0f - monoDryS);
            l *= effectiveVolGain;
            r *= effectiveVolGain;
            meterL *= effectiveVolGain;
            meterR *= effectiveVolGain;
        }

        // Balance - allerletzte Stufe, nach dem Vol-Trim (Runde 74, User).
        {
            const float pgL = outPanLGain.getNextValue();
            const float pgR = outPanRGain.getNextValue();
            l *= pgL;  r *= pgR;
            meterL *= pgL;  meterR *= pgR;
        }

        // "Chaos"-Duck (siehe chaosTriggerRequested-Kommentar im Header):
        // wirkt nach dem Vol-Trim, also auf ALLES, egal welche Kombination
        // an Zufallswerten gerade Pegel macht. Nach der Haltezeit einmalig
        // auf die weiche Rueckkehr-Rampe umschalten.
        if (chaosDuckHoldSamplesRemaining > 0 && --chaosDuckHoldSamplesRemaining == 0)
        {
            chaosDuckGain.reset (currentSampleRate, 0.35); // ca. 350ms weich zurueck
            chaosDuckGain.setTargetValue (1.0f);
        }
        {
            const float duckGain = chaosDuckGain.getNextValue();
            l *= duckGain;
            r *= duckGain;
            meterL *= duckGain;
            meterR *= duckGain;
        }

        // Weicher Bypass: waehrend der Ueberblendung Richtung Original.
        {
            const float b = bypassBlend.getNextValue();
            if (b > 0.0f)
            {
                l += (bypassDryL[(size_t) i] - l) * b;
                r += (bypassDryR[(size_t) i] - r) * b;
            }
        }

        left[i]  = l;
        right[i] = r;

        corrSumLR += (double) meterL * (double) meterR;
        corrSumLL += (double) meterL * (double) meterL;
        corrSumRR += (double) meterR * (double) meterR;

        // Seiten-Betonung (fuer die audio-reaktiven Sterne im Goniometer-
        // Hintergrund): Energie von Side (L-R) im Verhaeltnis zur
        // Gesamtenergie (Mid+Side), jetzt aus dem Mono-Check-unabhaengigen
        // Meter-Signal berechnet (siehe Bug-Fix-Kommentar oben).
        const float mFinal = 0.5f * (meterL + meterR);
        const float sFinal = 0.5f * (meterL - meterR);
        sideEnergySum += (double) sFinal * (double) sFinal;
        midEnergySum  += (double) mFinal * (double) mFinal;

        // 6) Goniometer liest das Meter-Signal (siehe Bug-Fix-Kommentar oben),
        // NICHT das tatsaechliche, evtl. durch Mono-Check veraenderte
        // Ausgangssignal.
        gonioBuffer.push (meterL, meterR);
    }

    {
        float corr = 0.0f;
        if (corrSumLL > 1.0e-9 && corrSumRR > 1.0e-9)
            corr = (float) (corrSumLR / std::sqrt (corrSumLL * corrSumRR));
        corr = juce::jlimit (-1.0f, 1.0f, corr);
        // Deutlich staerkere Glaettung ueber die Bloecke hinweg (One-Pole,
        // Zeitkonstante ~300ms) - der rohe Pro-Block-Wert zappelt zu stark
        // fuer eine ruhige, gut ablesbare Anzeige (User-Feedback).
        const float blockSeconds = (float) numSamples / (float) currentSampleRate;
        const float corrSmoothCoeff = std::exp (-blockSeconds / 0.3f);
        correlationSmooth = corr + corrSmoothCoeff * (correlationSmooth - corr);
        currentCorrelation.store (correlationSmooth, std::memory_order_relaxed);
    }

    {
        // 0 = reines Center/Mono, 1 = stark seitenbetont (Side-Energie
        // dominiert). +1e-9 verhindert Division durch 0 bei komplett
        // digitaler Stille. Gleiche Glaettung (~300ms Zeitkonstante) wie
        // die Korrelationsanzeige - reine Deko/Anzeige, kein Audio-Einfluss.
        const float total = (float) (sideEnergySum + midEnergySum);
        float emphasis = total > 1.0e-9f ? (float) (sideEnergySum / (double) total) : 0.0f;
        emphasis = juce::jlimit (0.0f, 1.0f, emphasis);
        const float blockSecondsEmph = (float) numSamples / (float) currentSampleRate;
        const float emphSmoothCoeff = std::exp (-blockSecondsEmph / 0.3f);
        sideEmphasisSmooth = emphasis + emphSmoothCoeff * (sideEmphasisSmooth - emphasis);
        currentSideEmphasis.store (sideEmphasisSmooth, std::memory_order_relaxed);
    }

    currentPanPos.store ((lastMoveS > 0.0001f) ? lastPanPos : 0.0f, std::memory_order_relaxed);
    currentRayLfo.store (lastRayLfo, std::memory_order_relaxed);

    // Output-Pegelanzeige: nach kompletter Verarbeitung (inkl. finaler
    // Gain-Stufe) gemessen, siehe currentInputLevel oben im selben Block.
    // (Korrektur: dieser Store lag zwischenzeitlich versehentlich in
    // updateVisualMeters(), wo es kein "buffer" gibt - richtiger Ort ist
    // hier, am tatsaechlichen Ende von processBlock().)
    // ===== AUTO GAIN: Ziel fuer den naechsten Block =====
    {
        autoGainApplied = agTo;
        autoGainDb.store (juce::Decibels::gainToDecibels (agTo, -24.0f), std::memory_order_relaxed);

        const double blockSec = (double) numSamples / juce::jmax (1.0, currentSampleRate);
        const double inMean  = agInAcc  / (double) juce::jmax (1, numSamples);
        const double outMean = agOutAcc / (double) juce::jmax (1, numSamples);

        // Die Energien laufen IMMER mit, damit beim naechsten Messfenster
        // sofort ein brauchbarer Wert dasteht.
        const double aMeas = std::exp (-blockSec / 0.25);
        autoGainInSq  = inMean  + aMeas * (autoGainInSq  - inMean);
        autoGainOutSq = outMean + aMeas * (autoGainOutSq - outMean);

        // Nachgeregelt wird NUR im Messfenster (siehe Header). Danach steht
        // der Wert fest, egal was das Material macht.
        constexpr double kSilence = 1.0e-9;   // ca. -90 dBFS
        if (autoGainMeasureSamples > 0)
        {
            autoGainMeasureSamples -= numSamples;
            if (autoGainInSq > kSilence && autoGainOutSq > kSilence)
            {
                const double ratio = std::sqrt (autoGainInSq / autoGainOutSq);
                // Enger begrenzt als beim ersten Anlauf (User: "12 dB ist viel
                // zu viel"). Mit der Messung je Kanal ist so viel auch gar
                // nicht mehr noetig - der grosse Ausschlag kam vorher vom
                // Fehler, nicht vom Material.
                const float wanted = juce::jlimit (juce::Decibels::decibelsToGain (-6.0f),
                                                   juce::Decibels::decibelsToGain (6.0f),
                                                   (float) ratio);
                const float aGain = (float) std::exp (-blockSec / 0.25);
                autoGainTarget = wanted + aGain * (autoGainTarget - wanted);
            }
        }
    }

    // ===== DEMO-MODUS =====
    // Absichtlich ganz am Ende und VOR der Pegelanzeige: das OUT-Meter soll
    // das Absenken mitmachen, sonst wirkt es wie ein Fehler statt wie eine
    // Ansage. Rampe ueber den Block, damit nichts knackt.
    if (! licensed.load (std::memory_order_relaxed))
    {
        constexpr double kPeriodSec = 50.0;   // Abstand zwischen zwei Absenkungen
        constexpr double kQuietSec  = 3.2;    // wie lange leise
        constexpr double kFadeSec   = 0.35;   // Rampe in beide Richtungen

        const double blockSec = (double) numSamples / juce::jmax (1.0, currentSampleRate);
        demoPhaseSec += blockSec;
        if (demoPhaseSec >= kPeriodSec)
            demoPhaseSec -= kPeriodSec;

        const float target = (demoPhaseSec >= kPeriodSec - kQuietSec) ? 0.0f : 1.0f;
        const float maxStep = (float) (blockSec / kFadeSec);
        const float from = demoGain;
        demoGain = juce::jlimit (0.0f, 1.0f, from + juce::jlimit (-maxStep, maxStep, target - from));
        if (from < 1.0f || demoGain < 1.0f)
            buffer.applyGainRamp (0, numSamples, from, demoGain);
        demoDuck.store (demoGain, std::memory_order_relaxed);
    }
    else
    {
        demoGain = 1.0f;
        demoPhaseSec = 0.0;
        demoDuck.store (1.0f, std::memory_order_relaxed);
    }

    // ===== Sicherheitsnetz gegen den "Knall" (User: knallt nach einer Weile,
    // wenn in der DAW nichts passiert) =====
    // Die genaue Ursache ist beim Lesen des Codes nicht eindeutig zu finden.
    // Deshalb zwei Netze, die jede der ueblichen Ursachen abfangen:
    //  1) Ungueltige Zahlen (NaN/Inf) - ein einziger solcher Wert in einem
    //     rueckgekoppelten Zustand wird zum Vollpegel-Knall. Dann: Block
    //     stumm, alle Zustaende leer. Dazu eine harte Obergrenze bei +12 dBFS,
    //     damit nie etwas Ohren- oder Boxen-Gefaehrliches durchkommt.
    //  2) Nach 2 s echter Stille werden alle DSP-Zustaende einmal geleert.
    //     Dann kann kein alter Rest (Feedback, Filterzustand, Ringpuffer)
    //     spaeter unvermittelt herauskommen. Auto Gain bleibt unberuehrt.
    {
        bool bad = false;
        for (int ch = 0; ch < buffer.getNumChannels() && ! bad; ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
            {
                if (! std::isfinite (d[i])) { bad = true; break; }
                d[i] = juce::jlimit (-4.0f, 4.0f, d[i]);
            }
        }
        if (bad)
        {
            buffer.clear();
            clearProcessingState();
        }

        if (inputPeakForGuard < 1.0e-6f)   // ca. -120 dBFS
        {
            silentSeconds += (double) numSamples / currentSampleRate;
            if (silentSeconds > 2.0 && ! silenceCleared)
            {
                clearDspTails();
                silenceCleared = true;
            }
        }
        else
        {
            silentSeconds = 0.0;
            silenceCleared = false;
        }
    }

    currentOutputLevel.store (buffer.getMagnitude (0, numSamples), std::memory_order_relaxed);

    // Lebenszeichen fuer das Starfield - siehe lastProcessBlockMs im Header.
    lastProcessBlockMs.store (juce::Time::getMillisecondCounter(), std::memory_order_relaxed);

    // ===== TRANSPORT-ZUSTAND FUER DAS STARFIELD =====
    // User-Wunsch: "alles langsam anhalten und langsam starten (fade in/out)
    // wenn DAW Stop/start". Dafuer braucht die GUI eine verlaessliche
    // Information darueber, ob der Host gerade laeuft.
    //
    // lastProcessBlockMs allein reicht dafuer NICHT: die meisten DAWs rufen
    // processBlock() auch im Stop-Zustand weiter auf (fuer Live-Monitoring),
    // das Starfield wuerde also nie anhalten. Deshalb wird hier zusaetzlich
    // der echte Transport-Zustand vom Playhead gelesen.
    //
    // Wichtig ist der Fallback: gibt es gar keinen Playhead (Standalone,
    // manche Plugin-Hosts, Offline-Rendering), liefert getPosition() nichts -
    // dann gilt bewusst "spielt", damit das Feld dort nicht dauerhaft
    // eingefroren bleibt. Lieber laeuft die Animation einmal zu viel als
    // dass sie in einer Umgebung ohne Transport tot wirkt.
    {
        bool playing = true; // Fallback: ohne Playhead-Info immer animieren
        if (auto* playHead = getPlayHead())
            if (auto pos = playHead->getPosition())
                playing = pos->getIsPlaying();

        currentTransportPlaying.store (playing, std::memory_order_relaxed);
    }
}

// Siehe wasFullyBypassed im Header.
void LCRMSAudioProcessor::clearProcessingState() noexcept
{
    clearDspTails();
    // Auto Gain danach neu einpegeln - die alten Energien stammen von vor
    // dem Bypass und passen nicht mehr.
    kwInHpL = {}; kwInShelfL = {}; kwInHpR = {}; kwInShelfR = {};
    kwOutHpL = {}; kwOutShelfL = {}; kwOutHpR = {}; kwOutShelfR = {};
    std::fill (autoGainInDelay.begin(), autoGainInDelay.end(), 0.0f);
    autoGainInPos = 0;
    autoGainMeasureSamples = (int) (currentSampleRate * 1.0);
}

void LCRMSAudioProcessor::clearDspTails() noexcept
{
    lcrExtractor.reset();
    delayL.reset();
    delayR.reset();
    bendL.reset();
    bendR.reset();
    elevateStateL = {}; elevateStateR = {};
    prismGalHpL = {}; prismGalLpL = {}; prismGalHpR = {}; prismGalLpR = {};
    prismDimHp = {}; prismDimLp = {};
    prismPosHp = {}; prismPosLp = {};
    bassGuardDim = {};
    for (int k = 0; k < kRayStages; ++k) { rayApL[k] = 0.0f; rayApR[k] = 0.0f; }
    rayFbL = rayFbR = 0.0f;
    std::fill (lcrDryDelayL.begin(), lcrDryDelayL.end(), 0.0f);
    std::fill (lcrDryDelayR.begin(), lcrDryDelayR.end(), 0.0f);
    lcrDryWritePos = 0;
}

void LCRMSAudioProcessor::passthroughWithLatencyCompensation (juce::AudioBuffer<float>& buffer)
{
    // Pegelanzeigen bei Bypass auf null (User-Wunsch: "Plugin bypass ->
    // Meter muessen aus gehen"). Bewusst HIER und nicht an den beiden
    // Aufrufstellen: diese Funktion ist der gemeinsame Weg fuer BEIDE
    // Bypass-Arten - den Logo-/BYP-Klick (uiBypassed in processBlock) und
    // den Host-Bypass (processBlockBypassed). Eine Stelle, beide Faelle
    // abgedeckt, keine Chance dass eine davon vergessen wird.
    // Der Input-Peak wird ganz am Anfang von processBlock gespeichert, also
    // VOR der Bypass-Pruefung - ohne dieses Zuruecksetzen wuerde das
    // IN-Meter munter weiterlaufen, waehrend das Plugin gar nichts tut.
    currentInputLevel.store  (0.0f, std::memory_order_relaxed);
    currentOutputLevel.store (0.0f, std::memory_order_relaxed);
    currentRayLfo.store (0.0f, std::memory_order_relaxed);

    // Wichtig fuer korrekte Latenzkompensation beim Bypass (Host-Bypass ODER
    // manueller Logo-Klick-Bypass, z.B. in Cubase): Der Host hat zuletzt
    // lastReportedLatency Samples an PDC eingeplant. Ein "echter"
    // Nulllatenz-Bypass wuerde dazu fuehren, dass diese Spur beim Bypassen
    // ploetzlich zu frueh/spaet zu den anderen Spuren liegt. Deshalb reichen
    // wir das Signal um exakt diese Latenz verzoegert durch (eigener
    // Ringpuffer, unabhaengig von den Drift-Delaylines), statt es
    // unveraendert durchzuschleifen.
    const int latency = juce::jmax (0, lastReportedLatency);
    if (latency <= 0 || bypassDelayL.empty())
    {
        // Bug-Fix (User-Feedback: "Auch nicht Mono Check + Bypass" - Gonio/
        // Korrelationsmesser sollen bei Bypass NICHT einfrieren): echtes
        // Nulllatenz-Bypass gibt hier zwar nichts weiter, das Signal im
        // Buffer ist aber bereits das unveraenderte Original - die Meter
        // trotzdem damit fuettern.
        updateVisualMeters (buffer.getReadPointer (0), buffer.getReadPointer (1), buffer.getNumSamples());
        return; // echtes Nulllatenz-Bypass ist hier bereits korrekt
    }

    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);
    const int numSamples = buffer.getNumSamples();
    const int bufSize = (int) bypassDelayL.size();

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = left[i];
        const float inR = right[i];

        left[i]  = bypassDelayL[(size_t) bypassWritePos];
        right[i] = bypassDelayR[(size_t) bypassWritePos];

        bypassDelayL[(size_t) bypassWritePos] = inL;
        bypassDelayR[(size_t) bypassWritePos] = inR;

        bypassWritePos = (bypassWritePos + 1) % bufSize;
    }

    // Bug-Fix (User-Feedback: "Auch nicht Mono Check + Bypass" - Gonio/
    // Korrelationsmesser sollen bei Bypass NICHT einfrieren): mit dem
    // tatsaechlich durchgereichten (verzoegerten) Signal fuettern, damit die
    // Anzeigen auch waehrend Bypass live weiterlaufen.
    updateVisualMeters (left, right, numSamples);
}

// Siehe Kommentar an der Deklaration (PluginProcessor.h): fuellt Goniometer-
// Ringpuffer + Korrelations-/Side-Emphasis-Anzeige aus einem fertigen
// Stereo-Block - genutzt vom Bypass-Pfad, damit die visuellen Anzeigen dort
// nicht einfrieren (User-Feedback: "Auch nicht Mono Check + Bypass").
void LCRMSAudioProcessor::updateVisualMeters (const float* leftBuf, const float* rightBuf, int numSamples)
{
    double corrSumLR = 0.0, corrSumLL = 0.0, corrSumRR = 0.0;
    double sideEnergySum = 0.0, midEnergySum = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        const float l = leftBuf[i];
        const float r = rightBuf[i];

        corrSumLR += (double) l * (double) r;
        corrSumLL += (double) l * (double) l;
        corrSumRR += (double) r * (double) r;

        const float mFinal = 0.5f * (l + r);
        const float sFinal = 0.5f * (l - r);
        sideEnergySum += (double) sFinal * (double) sFinal;
        midEnergySum  += (double) mFinal * (double) mFinal;

        gonioBuffer.push (l, r);
    }

    {
        float corr = 0.0f;
        if (corrSumLL > 1.0e-9 && corrSumRR > 1.0e-9)
            corr = (float) (corrSumLR / std::sqrt (corrSumLL * corrSumRR));
        corr = juce::jlimit (-1.0f, 1.0f, corr);
        const float blockSeconds = (float) numSamples / (float) currentSampleRate;
        const float corrSmoothCoeff = std::exp (-blockSeconds / 0.3f);
        correlationSmooth = corr + corrSmoothCoeff * (correlationSmooth - corr);
        currentCorrelation.store (correlationSmooth, std::memory_order_relaxed);
    }

    {
        const float total = (float) (sideEnergySum + midEnergySum);
        float emphasis = total > 1.0e-9f ? (float) (sideEnergySum / (double) total) : 0.0f;
        emphasis = juce::jlimit (0.0f, 1.0f, emphasis);
        const float blockSecondsEmph = (float) numSamples / (float) currentSampleRate;
        const float emphSmoothCoeff = std::exp (-blockSecondsEmph / 0.3f);
        sideEmphasisSmooth = emphasis + emphSmoothCoeff * (sideEmphasisSmooth - emphasis);
        currentSideEmphasis.store (sideEmphasisSmooth, std::memory_order_relaxed);
    }
}

void LCRMSAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    passthroughWithLatencyCompensation (buffer);
}

juce::AudioProcessorEditor* LCRMSAudioProcessor::createEditor()
{
    return new LCRMSAudioProcessorEditor (*this);
}

void LCRMSAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void LCRMSAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Siehe Kommentar im Konstruktor: markiert, dass ein (echter oder vom
    // Host selbst gecachter) Zustand eingetroffen ist, damit unser
    // verzoegerter "Activate Galaxy als Standard"-Callback sich zurueckhaelt.
    hasReceivedExternalState = true;
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LCRMSAudioProcessor();
}
