#pragma once
#include "JuceHeader.h"
#include "FairyLook.h"
#include "Goniometer.h"
#include "MeterFifo.h"

// Gemeinsames Geruest aller Einzel-Plugins:
//  - Prozessor: Parameter, Zustand, Bus-Layout, Goniometer-Abgriff.
//  - Editor: Fairy-Tale-Platte, eine Sektion (Karte) mit klickbarem Titel
//    (= Ein/Aus) und Meter-Knopf, aufklappbares Goniometer links, frei
//    skalierbar mit festem Seitenverhaeltnis, keine Tooltips.
namespace fairy
{
    class ShellProcessor : public juce::AudioProcessor
    {
    public:
        ShellProcessor (juce::AudioProcessorValueTreeState::ParameterLayout layout, bool allowMonoInput,
                        const char* stateType)
            : AudioProcessor (BusesProperties()
                                .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
              apvts (*this, nullptr, stateType, std::move (layout)),
              monoIn (allowMonoInput) {}

        // Einmal pro Block, nur Stereo. l/r werden in-place bearbeitet.
        virtual void processStereo (float* l, float* r, int numSamples) = 0;

        bool isBusesLayoutSupported (const BusesLayout& layouts) const override
        {
            const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
            return out == juce::AudioChannelSet::stereo()
                && (in == juce::AudioChannelSet::stereo() || (monoIn && in == juce::AudioChannelSet::mono()));
        }

        void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
        {
            juce::ScopedNoDenormals noDenormals;
            const int n = buffer.getNumSamples();
            const int numIn = getTotalNumInputChannels();
            for (int ch = numIn; ch < getTotalNumOutputChannels(); ++ch)
                buffer.clear (ch, 0, n);
            if (buffer.getNumChannels() < 2 || n <= 0)
                return;
            if (numIn == 1)
                buffer.copyFrom (1, 0, buffer, 0, 0, n);   // Mono -> beide Seiten
            auto* l = buffer.getWritePointer (0);
            auto* r = buffer.getWritePointer (1);
            processStereo (l, r, n);
            if (meterActive.load (std::memory_order_relaxed))
                meterFifo.push (l, r, n);
        }
        using AudioProcessor::processBlock;

        bool hasEditor() const override { return true; }
        const juce::String getName() const override { return JucePlugin_Name; }
        bool acceptsMidi() const override { return false; }
        bool producesMidi() const override { return false; }
        bool isMidiEffect() const override { return false; }
        double getTailLengthSeconds() const override { return 0.05; }
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram (int) override {}
        const juce::String getProgramName (int) override { return {}; }
        void changeProgramName (int, const juce::String&) override {}
        void releaseResources() override {}

        void getStateInformation (juce::MemoryBlock& destData) override
        {
            if (auto xml = apvts.copyState().createXml())
                copyXmlToBinary (*xml, destData);
        }
        void setStateInformation (const void* data, int sizeInBytes) override
        {
            if (auto xml = getXmlFromBinary (data, sizeInBytes))
                if (xml->hasTagName (apvts.state.getType()))
                    apvts.replaceState (juce::ValueTree::fromXml (*xml));
        }

        float param (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

        juce::AudioProcessorValueTreeState apvts;
        mistycat::MeterFifo meterFifo;
        std::atomic<bool>   meterActive { false };

        bool  isMeterOpen() const    { return apvts.state.getProperty ("meterOpen", false); }
        void  setMeterOpen (bool b)  { apvts.state.setProperty ("meterOpen", b, nullptr); }
        float uiScale() const        { return (float) apvts.state.getProperty ("uiScale", 1.0f); }
        void  setUiScale (float s)   { apvts.state.setProperty ("uiScale", s, nullptr); }

    private:
        const bool monoIn;
    };

    // ===================================================================
    class ShellEditor : public juce::AudioProcessorEditor, private juce::Timer
    {
    public:
        // Design-Masse (Skalierung 1.0); gezeichnet wird alles in diesen Massen.
        static constexpr int kH = 240, kMargin = 12, kGonioW = 216, kGap = 4;
        static constexpr int kHeaderH = 24, kLabelH = 14;
        static constexpr float kMinScale = 0.75f, kMaxScale = 2.5f;

        ShellEditor (ShellProcessor& p, const juce::String& titleText, const char* onParamId, int cardWidth)
            : AudioProcessorEditor (&p), shell (p), onId (onParamId), cardW (cardWidth), gonio (p.meterFifo), glide (*this)
        {
            setLookAndFeel (&lnf);
            content.setInterceptsMouseClicks (false, true);
            addAndMakeVisible (content);

            title.text = titleText;
            title.onClick = [this]
            {
                if (auto* prm = shell.apvts.getParameter (onId))
                    prm->setValueNotifyingHost (prm->getValue() > 0.5f ? 0.0f : 1.0f);
                syncOnState();
            };
            content.addAndMakeVisible (title);
            meterToggle.onClick = [this] { setMeterOpen (! meterOpen); };
            content.addAndMakeVisible (meterToggle);
            content.addChildComponent (gonio);
        }

        ~ShellEditor() override
        {
            stopTimer();
            shell.meterActive.store (false);
            gonio.setActive (false);
            setLookAndFeel (nullptr);
        }

        void paint (juce::Graphics& g) override
        {
            // Platte + Rahmen einmal pro Groesse in echter Pixelaufloesung -
            // beim Drehen wird davon nur ein Ausschnitt kopiert.
            const float ds = juce::jmax (1.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
            const float key[4] = { (float) getWidth(), (float) getHeight(), ds, (sectionOn ? 1.0f : 0.0f) + (meterOpen ? 2.0f : 0.0f) };
            if (! plate.isValid() || ! std::equal (key, key + 4, plateKey))
            {
                plate = makePlate (juce::roundToInt ((float) getWidth() * ds), juce::roundToInt ((float) getHeight() * ds));
                juce::Graphics pg (plate);
                pg.addTransform (juce::AffineTransform::scale ((float) getWidth() * ds / (float) widthFor (meterOpen),
                                                               (float) getHeight() * ds / (float) kH));
                if (meterOpen)
                    drawSectionFrame (pg, gonioArea, true);
                drawSectionFrame (pg, cardArea, sectionOn);
                std::copy (key, key + 4, plateKey);
            }
            g.drawImage (plate, getLocalBounds().toFloat());
        }

        void refreshNow() { syncOnState(); }

        void resized() override
        {
            const int baseW = widthFor (meterOpen);
            const float s = juce::jlimit (kMinScale, kMaxScale, (float) getHeight() / (float) kH);
            if (! applyingSize && std::abs (s - uiScale) > 0.001f)
            {
                uiScale = s;
                shell.setUiScale (uiScale);
            }
            content.setBounds (0, 0, baseW, kH);
            content.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) baseW, (float) getHeight() / (float) kH));

            gonioArea = { kMargin, kMargin, kGonioW, kH - 2 * kMargin };
            cardArea  = { meterOpen ? kMargin + kGonioW + kGap : kMargin, kMargin, cardW, kH - 2 * kMargin };
            gonio.setBounds (gonioArea.reduced (10));

            auto inner = cardArea.reduced (10);
            auto header = inner.removeFromTop (kHeaderH);
            meterToggle.setBounds (header.removeFromRight (24).withSizeKeepingCentre (22, 22));
            header.removeFromRight (8);
            auto t = header.removeFromLeft (title.preferredWidth() + 6);
            title.setBounds (t.withTrimmedLeft (6));
            layoutCard (inner, header);
        }

    protected:
        // Von der Unterklasse: Regler in content anlegen, dann finishSetup().
        virtual void layoutCard (juce::Rectangle<int> inner, juce::Rectangle<int> headerRest) = 0;
        virtual void syncControls (bool /*sectionOn*/) {}

        void finishSetup()
        {
            uiScale   = juce::jlimit (kMinScale, kMaxScale, shell.uiScale());
            meterOpen = shell.isMeterOpen();
            setResizable (true, true);
            setMeterOpen (meterOpen);
            syncOnState (true);
            startTimerHz (15);
        }

        // Drehregler im SpaceX-Stil. Cmd-Klick = Default (wie in SpaceX).
        // format: Text, den die Beschriftung beim Drehen statt des Namens zeigt.
        void addKnob (juce::Slider& s, juce::Label& l, const juce::String& name, const char* paramId,
                      double defaultValue, std::function<juce::String (double)> format)
        {
            s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            content.addAndMakeVisible (s);
            attachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (shell.apvts, paramId, s));
            s.setDoubleClickReturnValue (true, defaultValue, juce::ModifierKeys::commandModifier);
            addLabel (l, name);
            auto update = [&s, &l, name, format]
            {
                l.setText (s.isMouseButtonDown() && format ? format (s.getValue()) : name, juce::dontSendNotification);
            };
            s.onValueChange = update;
            s.onDragStart   = update;
            s.onDragEnd     = update;
            glide.add (s);
            offControls.push_back (&s);
        }

        void addLabel (juce::Label& l, const juce::String& text)
        {
            l.setText (text, juce::dontSendNotification);
            l.setJustificationType (juce::Justification::centred);
            l.setFont (paramFont());
            l.setMinimumHorizontalScale (0.88f);
            l.setBorderSize (juce::BorderSize<int> (1, 0, 1, 0));
            l.setInterceptsMouseClicks (false, false);
            content.addAndMakeVisible (l);
            labels.push_back (&l);
        }

        // Modusfeld (Icon + Name) wie in SpaceX - kind: "pxDiagram", "rayDiagram", "eqDiagram".
        void addModeField (juce::TextButton& b, ModeDots& dots, const char* kind, int count)
        {
            b.setClickingTogglesState (false);
            b.setWantsKeyboardFocus (false);
            b.getProperties().set ("thinOnFrame", true);
            b.getProperties().set ("modePill", true);
            b.getProperties().set (kind, 0);
            b.getProperties().set ("pillColour", (int) themePalette().frameRaye.getARGB());
            content.addAndMakeVisible (b);
            dots.count = count;
            content.addAndMakeVisible (dots);
            offControls.push_back (&b);
        }

        // Unterklassen rufen das in ihrem Destruktor auf: die Attachments
        // muessen weg sein, bevor deren Regler zerstoert werden.
        void clearAttachments() { attachments.clear(); }

        void addControl (juce::Component& c) { content.addAndMakeVisible (c); offControls.push_back (&c); }

        // Regler mit Beschriftung: Regler oben, Beschriftung direkt darunter.
        static void placeKnob (juce::Rectangle<int> slot, int y, int size, juce::Slider& s, juce::Label& l)
        {
            s.setBounds (slot.getCentreX() - size / 2, y, size, size);
            l.setBounds (slot.getX() - 8, y + size, slot.getWidth() + 16, kLabelH);
        }
        // Modusfeld wie in SpaceX: mittig auf Reglerhoehe, Punkte direkt darunter.
        static void placeModeField (juce::Rectangle<int> slot, int knobCentreY, juce::TextButton& b, ModeDots& dots)
        {
            constexpr int w = 118, h = 80;
            b.setBounds (slot.getCentreX() - w / 2, knobCentreY - h / 2, w, h);
            const int dotsW = dots.count * 10 + 6;
            dots.setBounds (slot.getCentreX() - dotsW / 2, b.getBottom() - 1, dotsW, 12);
        }

        void setParam (const char* id, float plainValue)
        {
            if (auto* prm = shell.apvts.getParameter (id))
                prm->setValueNotifyingHost (prm->convertTo0to1 (plainValue));
        }
        int choice (const char* id) const { return (int) std::round (shell.param (id)); }

        // Sofort nachziehen (z. B. nach einem Klick), sonst 15x pro Sekunde.
        void syncOnState (bool force = false)
        {
            const bool on = shell.param (onId) > 0.5f;
            if (force || on != sectionOn)
            {
                sectionOn = on;
                title.on = on;
                title.repaint();
                for (auto* c : offControls)
                {
                    c->getProperties().set ("sectionOff", ! on);
                    c->repaint();
                }
                for (auto* l : labels)
                    l->setColour (juce::Label::textColourId, on ? labelOn() : labelOff());
                repaint();
            }
            syncControls (on);
        }

        ShellProcessor& shell;
        juce::Component content;
        LookAndFeel lnf;
        bool sectionOn = true;

    private:
        void timerCallback() override
        {
            syncOnState();
            if (shell.isMeterOpen() != meterOpen)
                setMeterOpen (shell.isMeterOpen());
        }

        int widthFor (bool open) const { return kMargin * 2 + cardW + (open ? kGonioW + kGap : 0); }

        void applyConstrainer()
        {
            const double w = widthFor (meterOpen);
            constrainer.setFixedAspectRatio (w / (double) kH);
            constrainer.setSizeLimits (juce::roundToInt (w * kMinScale), juce::roundToInt (kH * kMinScale),
                                       juce::roundToInt (w * kMaxScale), juce::roundToInt (kH * kMaxScale));
            setConstrainer (&constrainer);
        }

        void setMeterOpen (bool open)
        {
            meterOpen = open;
            shell.setMeterOpen (open);
            meterToggle.open = open;
            meterToggle.repaint();
            gonio.setVisible (open);
            gonio.setActive (open);
            shell.meterActive.store (open);
            applyingSize = true;
            applyConstrainer();
            const int w = juce::roundToInt ((float) widthFor (open) * uiScale), h = juce::roundToInt ((float) kH * uiScale);
            if (getWidth() != w || getHeight() != h)
                setSize (w, h);
            applyingSize = false;
            resized();
            repaint();
        }

        const char* onId;
        const int cardW;
        Title title;
        MeterToggle meterToggle;
        Goniometer gonio;
        juce::ComponentBoundsConstrainer constrainer;
        juce::Rectangle<int> cardArea, gonioArea;
        juce::Image plate;
        float plateKey[4] { -1, -1, -1, -1 };
        float uiScale = 1.0f;
        bool  applyingSize = false, meterOpen = false;
        std::vector<juce::Component*> offControls;
        std::vector<juce::Label*> labels;
        std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;

    protected:
        KnobGlide glide;
    };
}
