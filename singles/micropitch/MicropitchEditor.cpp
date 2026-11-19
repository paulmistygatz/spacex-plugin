#include "MicropitchEditor.h"

namespace
{
    const char* const kModeNames[micropitch::kModes] = { "VELVET", "HALO", "ILLUSION", "DOUBLE" };
    // Texte wie in SpaceX.
    const char* const kModeTips[micropitch::kModes]  = {
        "Velvet: the gentlest - soft and close, the centre barely moves",
        "Halo: a soft ring around the sound - stays centred, holds up on a full mix",
        "Illusion: sounds wider than it is, but stays tight",
        "Double: the widest of the four, with the strongest character" };
}

// ===== Modusfeld (Zeichnung aus CustomLookAndFeel, pxDiagram) =====
void MicropitchModeField::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto icoCol = off ? fairy::labelOff() : fairy::frameRaye;
    auto db = bounds.withHeight (bounds.getHeight() * 0.66f);
    const float sc = juce::jmin (3.8f, db.getHeight() / 13.0f);
    const float cx = db.getCentreX(), cy = db.getCentreY();

    if (! off)
    {
        const bool down = isMouseButtonDown(), over = isMouseOver();
        const float hot = down ? 1.9f : over ? 1.55f : 1.0f;
        const float room = juce::jmin (juce::jmin (cy - bounds.getY(), bounds.getBottom() - cy), bounds.getWidth() * 0.5f);
        const float gr = juce::jmin (11.0f * sc * (hot > 1.0f ? 1.10f : 1.0f), room);
        juce::ColourGradient grad (icoCol.withAlpha (juce::jmin (0.65f, 0.26f * hot)), cx, cy,
                                   icoCol.withAlpha (0.0f), cx + gr, cy, true);
        grad.addColour (0.45, icoCol.withAlpha (juce::jmin (0.40f, 0.13f * hot)));
        g.setGradientFill (grad);
        g.fillEllipse (cx - gr, cy - gr, gr * 2.0f, gr * 2.0f);
    }

    const float a = off ? 1.0f : 0.85f;
    g.setColour (icoCol.withAlpha (a));
    const int diag = juce::jlimit (0, 3, mode);
    // Draufsicht: Mitte = Zentrum, aussen = Seiten.
    if (diag == 0 || diag == 1)
    {
        const float r = (diag == 1 ? 5.4f : 3.4f) * sc;
        if (diag == 0) g.setColour (icoCol.withAlpha (a * 0.50f));
        g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 1.1f * sc);
        g.setColour (icoCol.withAlpha (a));
        g.fillEllipse (cx - 1.3f * sc, cy - 1.3f * sc, 2.6f * sc, 2.6f * sc);
    }
    else
    {
        const float spread[4] = { 2.6f, 0.0f, 5.0f, 6.6f };
        const float sp = spread[diag] * sc;
        const float h  = (diag == 3 ? 7.0f : 5.4f) * sc;
        const float w  = 2.2f * sc;
        g.fillRoundedRectangle (cx - sp - w * 0.5f, cy - h * 0.5f, w, h, w * 0.5f);
        g.fillRoundedRectangle (cx + sp - w * 0.5f, cy - h * 0.5f, w, h, w * 0.5f);
        g.setColour (icoCol.withAlpha (a * (diag == 3 ? 0.30f : 1.0f)));
        g.fillEllipse (cx - 1.2f * sc, cy - 1.2f * sc, 2.4f * sc, 2.4f * sc);
    }

    g.setColour (off ? fairy::labelOff() : fairy::frameRaye);
    g.setFont (fairy::modeFont (getHeight()));
    g.drawText (kModeNames[diag], getLocalBounds().withTrimmedTop (juce::roundToInt ((float) getHeight() * 0.64f)),
                juce::Justification::centred, false);
}

// ===== Editor =====
MicropitchEditor::MicropitchEditor (MicropitchProcessor& p)
    : AudioProcessorEditor (&p), proc (p), gonio (p.meterFifo)
{
    setLookAndFeel (&lnf);

    title.text = "MICROPITCH";
    title.setTooltip ("Micropitch: makes a mono sound wide with tiny time and pitch offsets. Click to switch it on or off.");
    title.onClick = [this]
    {
        if (auto* prm = proc.apvts.getParameter (MicropitchProcessor::ID_ON))
            prm->setValueNotifyingHost (prm->getValue() > 0.5f ? 0.0f : 1.0f);
        syncFromParams();
    };
    addAndMakeVisible (title);

    meterToggle.onClick = [this] { setMeterOpen (! meterOpen); };
    addAndMakeVisible (meterToggle);

    amountSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    amountSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    amountSlider.setTooltip ("Amount: how much of the selected style is applied");
    amountSlider.onValueChange = [this] { updateAmountLabel(); };
    amountSlider.onDragStart   = [this] { updateAmountLabel(); };
    amountSlider.onDragEnd     = [this] { updateAmountLabel(); };
    addAndMakeVisible (amountSlider);
    amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        proc.apvts, MicropitchProcessor::ID_AMOUNT, amountSlider);
    amountSlider.setDoubleClickReturnValue (true, 0.0);

    amountLabel.setJustificationType (juce::Justification::centred);
    amountLabel.setFont (fairy::paramFont());
    amountLabel.setBorderSize (juce::BorderSize<int> (1, 0, 1, 0));
    amountLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (amountLabel);

    modeField.onClick = [this] (bool back)
    {
        if (auto* prm = proc.apvts.getParameter (MicropitchProcessor::ID_MODE))
        {
            const int cur  = juce::jlimit (0, micropitch::kModes - 1, (int) std::round (prm->convertFrom0to1 (prm->getValue())));
            const int next = (cur + (back ? micropitch::kModes - 1 : 1)) % micropitch::kModes;
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) next));
        }
        syncFromParams();
    };
    addAndMakeVisible (modeField);

    modeDots.count = micropitch::kModes;
    modeDots.setTooltip ("Click a dot to pick a style directly");
    modeDots.onPick = [this] (int i)
    {
        if (auto* prm = proc.apvts.getParameter (MicropitchProcessor::ID_MODE))
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) i));
        syncFromParams();
    };
    addAndMakeVisible (modeDots);

    addChildComponent (gonio);

    meterOpen = proc.isMeterOpen();
    setSize (widthFor (meterOpen), kH);
    setMeterOpen (meterOpen);
    syncFromParams();
    updateAmountLabel();
    startTimerHz (15);
}

MicropitchEditor::~MicropitchEditor()
{
    stopTimer();
    proc.meterActive.store (false);
    gonio.setActive (false);
    setLookAndFeel (nullptr);
}

void MicropitchEditor::setMeterOpen (bool open)
{
    meterOpen = open;
    proc.setMeterOpen (open);
    meterToggle.open = open;
    meterToggle.setTooltip (open ? "Hide the goniometer" : "Show the goniometer");
    meterToggle.repaint();
    gonio.setVisible (open);
    gonio.setActive (open);
    proc.meterActive.store (open);
    if (getWidth() != widthFor (open))
        setSize (widthFor (open), kH);
    else
        resized();
    repaint();
}

void MicropitchEditor::syncFromParams()
{
    const bool on = proc.apvts.getRawParameterValue (MicropitchProcessor::ID_ON)->load() > 0.5f;
    const int mode = juce::jlimit (0, micropitch::kModes - 1,
                                   (int) std::round (proc.apvts.getRawParameterValue (MicropitchProcessor::ID_MODE)->load()));
    if (on != sectionOn || title.on != on)
    {
        sectionOn = on;
        title.on = on;
        title.repaint();
        amountSlider.getProperties().set ("sectionOff", ! on);
        amountSlider.repaint();
        amountLabel.setColour (juce::Label::textColourId, on ? fairy::labelOn : fairy::labelOff());
        repaint (cardArea);
    }
    if (modeField.mode != mode || modeField.off != ! on)
    {
        modeField.mode = mode;
        modeField.off = ! on;
        modeField.setTooltip (juce::String (kModeTips[mode]) + ". Click for the next one, Cmd-click to go back.");
        modeField.repaint();
    }
    if (modeDots.index != mode || modeDots.off != ! on)
    {
        modeDots.index = mode;
        modeDots.off = ! on;
        modeDots.repaint();
    }
}

void MicropitchEditor::updateAmountLabel()
{
    // Beim Drehen zeigt die Beschriftung den Wert.
    const bool showValue = amountSlider.isMouseButtonDown();
    amountLabel.setText (showValue ? juce::String (juce::roundToInt (amountSlider.getValue())) + " %" : juce::String ("Amount"),
                         juce::dontSendNotification);
}

void MicropitchEditor::timerCallback()
{
    syncFromParams();
    // Neues Projekt / Undo im Host kann den Meter-Zustand aendern.
    if (proc.isMeterOpen() != meterOpen)
        setMeterOpen (proc.isMeterOpen());
}

void MicropitchEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0c0e15));
    if (plate.getWidth() != getWidth() || plate.getHeight() != getHeight())
        plate = fairy::makePlate (getWidth(), getHeight(), 0.0f);
    g.drawImageAt (plate, 0, 0);

    if (meterOpen)
        fairy::drawSectionFrame (g, gonioArea, true);
    fairy::drawSectionFrame (g, cardArea, sectionOn);
}

void MicropitchEditor::resized()
{
    gonioArea = { kMargin, kMargin, kGonioW, kH - 2 * kMargin };
    cardArea  = { meterOpen ? kMargin + kGonioW + kGap : kMargin, kMargin, kCardW, kH - 2 * kMargin };

    gonio.setBounds (gonioArea.reduced (10));

    auto inner = cardArea.reduced (10);
    auto header = inner.removeFromTop (24);
    meterToggle.setBounds (header.removeFromRight (24).withSizeKeepingCentre (22, 22));
    title.setBounds (header.withTrimmedLeft (6));

    // Regler links, Modusfeld rechts - wie in SpaceX.
    constexpr int knobD = 96, labelH = 14;
    const int blockY = inner.getY() + (inner.getHeight() - (knobD + labelH)) / 2;
    auto left  = inner.removeFromLeft (inner.getWidth() / 2);
    auto right = inner;
    amountSlider.setBounds (left.getCentreX() - knobD / 2, blockY, knobD, knobD);
    amountLabel.setBounds (left.getX(), blockY + knobD, left.getWidth(), labelH);

    const int dotsCy = amountLabel.getBounds().getCentreY();
    modeDots.setBounds (right.getCentreX() - 30, dotsCy - 6, 60, 12);
    constexpr int fieldW = 110, fieldH = 86;
    modeField.setBounds (right.getCentreX() - fieldW / 2, dotsCy - 8 - fieldH, fieldW, fieldH);
}
