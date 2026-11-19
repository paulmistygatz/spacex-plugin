#pragma once
#include "MicropitchProcessor.h"
#include "FairyLook.h"
#include "Goniometer.h"

// Modusfeld wie in SpaceX: Icon oben (Draufsicht aufs Stereobild), Name
// darunter. Klick = naechster Modus, Cmd-Klick = vorheriger.
class MicropitchModeField : public juce::Component, public juce::SettableTooltipClient
{
public:
    int  mode = 0;
    bool off  = false;
    std::function<void (bool back)> onClick;

    void paint (juce::Graphics& g) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { repaint(); }
    void mouseDown  (const juce::MouseEvent&) override { repaint(); }
    void mouseUp (const juce::MouseEvent& e) override
    {
        repaint();
        if (getLocalBounds().contains (e.getPosition()) && onClick)
            onClick (e.mods.isCommandDown());
    }
};

class MicropitchEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MicropitchEditor (MicropitchProcessor&);
    ~MicropitchEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setMeterOpen (bool open);
    void syncFromParams();
    void updateAmountLabel();

    // Design-Masse (Skalierung 1.0). Das Fenster ist frei skalierbar, das
    // Seitenverhaeltnis bleibt fest; gezeichnet wird alles in diesen Massen.
    static constexpr int kH = 240, kMargin = 12, kGonioW = 216, kGap = 4, kCardW = 280;
    static int widthFor (bool open) { return kMargin * 2 + kCardW + (open ? kGonioW + kGap : 0); }
    static constexpr float kMinScale = 0.75f, kMaxScale = 2.5f;
    void applyConstrainer();
    void layoutContent();
    float uiScale = 1.0f;
    bool  applyingSize = false;
    juce::ComponentBoundsConstrainer constrainer;
    juce::Component content;

    MicropitchProcessor& proc;
    fairy::LookAndFeel lnf;

    fairy::Title       title;
    fairy::MeterToggle meterToggle;
    juce::Slider       amountSlider;
    juce::Label        amountLabel;
    MicropitchModeField modeField;
    fairy::ModeDots    modeDots;
    fairy::Goniometer  gonio;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;

    juce::Rectangle<int> cardArea, gonioArea;
    juce::Image plate;      // Platte + Rahmen, in echter Pixelgroesse
    float plateKey[4] { -1, -1, -1, -1 };
    std::unique_ptr<fairy::KnobGlide> glide;
    bool meterOpen = false;
    bool sectionOn = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicropitchEditor)
};
