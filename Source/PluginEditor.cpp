#include "PluginEditor.h"

static void initLabel(juce::Label& l, const juce::String& text)
{
    l.setText(text, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centredLeft);
}

static void addChoiceItems(juce::ComboBox& box, const juce::StringArray& items)
{
    box.clear(juce::dontSendNotification);
    for (int i = 0; i < items.size(); ++i)
        box.addItem(items[i], i + 1);
}

static void initKnob(juce::Slider& s)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
}

static void initSmallKnob(juce::Slider& s)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
}

KeyAwarePitchDelayAudioProcessorEditor::KeyAwarePitchDelayAudioProcessorEditor (KeyAwarePitchDelayAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel(&retroLookAndFeel);
    setSize (1100, 780);

    // Populate combos
    addChoiceItems(keyRootBox, kapd::ChoiceLists::keyRoots());
    addChoiceItems(scaleTypeBox, kapd::ChoiceLists::scaleTypes());
    addChoiceItems(pitchSourceBox, kapd::ChoiceLists::pitchSources());
    addChoiceItems(fixedNoteBox, kapd::ChoiceLists::fixedNoteChoices());
    addChoiceItems(chordSnapModeBox, kapd::ChoiceLists::chordSnapModes());
    addChoiceItems(delayDivisionBox, kapd::ChoiceLists::delayDivisions());

    // Toggle buttons with descriptive text
    modeButton.setButtonText("Tone Seq");
    routingButton.setButtonText("Serial");
    trackingSourceButton.setButtonText("Loop");

    routingButton.onClick = [this]() {
        routingButton.setButtonText(routingButton.getToggleState() ? "Parallel" : "Serial");
    };

    delayMsSlider.onDragStart = [this]() {
        if (auto* param = processor.apvts.getParameter(kapd::param::tempoSync))
            param->setValueNotifyingHost(0.0f);
    };

    delayDivisionBox.onChange = [this]() {
        if (auto* param = processor.apvts.getParameter(kapd::param::tempoSync))
            param->setValueNotifyingHost(1.0f);
    };

    // Custom scale buttons
    static const juce::StringArray customLabels { "1", "b2", "2", "b3", "3", "4", "#4", "5", "b6", "6", "b7", "7" };
    for (int i = 0; i < (int) customScaleButtons.size(); ++i)
        customScaleButtons[(size_t) i].setButtonText(customLabels[i]);

    // Interval step sliders
    static const juce::StringArray intervalLabels {
        "-7", "-6", "-5", "-4", "-3", "-2", "-1", "0", "+1", "+2", "+3", "+4", "+5", "+6", "+7"
    };
    for (auto& s : intervalStepSliders)
    {
        s.setSliderStyle(juce::Slider::LinearVertical);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 18);
        s.setRange(0, 14, 1);
        s.textFromValueFunction = [](double v) { return intervalLabels[(int)v]; };
        s.valueFromTextFunction = [](const juce::String& t) { return (double) intervalLabels.indexOf(t); };
        s.updateText();
    }

    // Tone step sliders
    static const juce::StringArray toneLabels { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12" };
    for (auto& s : toneStepSliders)
    {
        s.setSliderStyle(juce::Slider::LinearVertical);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 18);
        s.setRange(0, 11, 1);
        s.textFromValueFunction = [](double v) { return toneLabels[(int)v]; };
        s.valueFromTextFunction = [](const juce::String& t) { return (double) toneLabels.indexOf(t); };
        s.updateText();
    }

    // Main control knobs
    initKnob(delayMsSlider);
    initKnob(feedbackSlider);
    initKnob(mixSlider);
    initKnob(outputGainSlider);
    initKnob(smoothingSlider);
    initKnob(transientSensitivitySlider);

    // Step level/pan sliders
    for (auto& s : stepLevelSliders)
    {
        s.setSliderStyle(juce::Slider::LinearVertical);
        s.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    }

    for (auto& s : stepPanSliders)
    {
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    }

    sequenceLengthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    sequenceLengthSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 18);

    // === POST-FX KNOBS ===
    initSmallKnob(saturationDriveSlider);
    initSmallKnob(saturationMixSlider);
    initSmallKnob(diffusionAmountSlider);
    initSmallKnob(diffusionMixSlider);
    initSmallKnob(lofiAmountSlider);
    initSmallKnob(lofiMixSlider);
    initSmallKnob(reverbDecaySlider);
    initSmallKnob(reverbDampingSlider);
    initSmallKnob(reverbMixSlider);
    initSmallKnob(highpassFreqSlider);
    initSmallKnob(lowpassFreqSlider);

    // Labels
    initLabel(keyRootLabel, "KEY");
    initLabel(scaleTypeLabel, "SCALE");
    initLabel(modeLabel, "MODE");
    initLabel(routingLabel, "ROUTING");
    initLabel(pitchSourceLabel, "PITCH SRC");
    initLabel(trackingLabel, "TRACK");
    initLabel(fixedNoteLabel, "FIXED");
    initLabel(snapToChordLabel, "CHORD");
    initLabel(chordSnapModeLabel, "SNAP");
    initLabel(advanceOnTransientLabel, "TRANS");
    initLabel(transientSensitivityLabel, "SENS");
    initLabel(tempoSyncLabel, "SYNC");
    initLabel(delayDivisionLabel, "DIV");
    initLabel(delayMsLabel, "TIME");
    initLabel(feedbackLabel, "FDBK");
    initLabel(mixLabel, "MIX");
    initLabel(outputGainLabel, "OUT");
    initLabel(sequenceLengthLabel, "STEPS");
    initLabel(smoothingLabel, "SMOOTH");
    initLabel(intervalStepsLabel, "INTERVAL STEPS");
    initLabel(toneStepsLabel, "TONE STEPS");
    initLabel(stepLevelLabel, "LEVEL");
    initLabel(stepPanLabel, "PAN");
    initLabel(customScaleLabel, "CUSTOM SCALE");

    // Post-FX labels
    initLabel(saturationLabel, "SATURATION");
    initLabel(diffusionLabel, "DIFFUSION");
    initLabel(lofiLabel, "LO-FI");
    initLabel(reverbLabel, "REVERB");
    initLabel(filterLabel, "FILTER");
    initLabel(satDriveLabel, "DRV");
    initLabel(satMixLabel, "MIX");
    initLabel(diffAmtLabel, "AMT");
    initLabel(diffMixLabel, "MIX");
    initLabel(lofiAmtLabel, "AMT");
    initLabel(lofiMixLabel, "MIX");
    initLabel(revDecayLabel, "DEC");
    initLabel(revDampLabel, "DMP");
    initLabel(revMixLabel, "MIX");
    initLabel(hpfLabel, "HPF");
    initLabel(lpfLabel, "LPF");

    // Add components
    auto addPair = [this](juce::Label& l, juce::Component& c) {
        addAndMakeVisible(l);
        addAndMakeVisible(c);
    };

    addPair(keyRootLabel, keyRootBox);
    addPair(scaleTypeLabel, scaleTypeBox);
    addPair(modeLabel, modeButton);
    addPair(routingLabel, routingButton);
    addPair(pitchSourceLabel, pitchSourceBox);
    addPair(trackingLabel, trackingSourceButton);
    addPair(fixedNoteLabel, fixedNoteBox);
    addPair(snapToChordLabel, snapToChordButton);
    addPair(chordSnapModeLabel, chordSnapModeBox);
    addPair(tempoSyncLabel, tempoSyncButton);
    addPair(delayDivisionLabel, delayDivisionBox);
    addPair(delayMsLabel, delayMsSlider);
    addPair(advanceOnTransientLabel, advanceOnTransientButton);
    addPair(transientSensitivityLabel, transientSensitivitySlider);
    addPair(feedbackLabel, feedbackSlider);
    addPair(mixLabel, mixSlider);
    addPair(outputGainLabel, outputGainSlider);
    addPair(sequenceLengthLabel, sequenceLengthSlider);
    addPair(smoothingLabel, smoothingSlider);

    addAndMakeVisible(intervalStepsLabel);
    addAndMakeVisible(toneStepsLabel);
    addAndMakeVisible(stepLevelLabel);
    addAndMakeVisible(stepPanLabel);
    addAndMakeVisible(customScaleLabel);

    for (auto& s : intervalStepSliders) addAndMakeVisible(s);
    for (auto& s : toneStepSliders) addAndMakeVisible(s);
    for (auto& s : stepLevelSliders) addAndMakeVisible(s);
    for (auto& s : stepPanSliders) addAndMakeVisible(s);
    for (auto& b : customScaleButtons) addAndMakeVisible(b);

    // Post-FX components
    addAndMakeVisible(saturationLabel);
    addAndMakeVisible(diffusionLabel);
    addAndMakeVisible(lofiLabel);
    addAndMakeVisible(reverbLabel);
    addAndMakeVisible(filterLabel);
    addPair(satDriveLabel, saturationDriveSlider);
    addPair(satMixLabel, saturationMixSlider);
    addPair(diffAmtLabel, diffusionAmountSlider);
    addPair(diffMixLabel, diffusionMixSlider);
    addPair(lofiAmtLabel, lofiAmountSlider);
    addPair(lofiMixLabel, lofiMixSlider);
    addPair(revDecayLabel, reverbDecaySlider);
    addPair(revDampLabel, reverbDampingSlider);
    addPair(revMixLabel, reverbMixSlider);
    addPair(hpfLabel, highpassFreqSlider);
    addPair(lpfLabel, lowpassFreqSlider);

    // Attachments
    auto& vts = processor.apvts;

    keyRootAttach = std::make_unique<APVTS::ComboBoxAttachment>(vts, kapd::param::keyRoot, keyRootBox);
    scaleTypeAttach = std::make_unique<APVTS::ComboBoxAttachment>(vts, kapd::param::scaleType, scaleTypeBox);
    modeAttach = std::make_unique<APVTS::ButtonAttachment>(vts, kapd::param::mode, modeButton);
    routingAttach = std::make_unique<APVTS::ButtonAttachment>(vts, kapd::param::routing, routingButton);

    pitchSourceAttach = std::make_unique<APVTS::ComboBoxAttachment>(vts, kapd::param::pitchSource, pitchSourceBox);
    trackingSourceAttach = std::make_unique<APVTS::ButtonAttachment>(vts, kapd::param::trackingSource, trackingSourceButton);
    fixedNoteAttach = std::make_unique<APVTS::ComboBoxAttachment>(vts, kapd::param::fixedMidi, fixedNoteBox);

    snapToChordAttach = std::make_unique<APVTS::ButtonAttachment>(vts, kapd::param::snapToChord, snapToChordButton);
    chordSnapModeAttach = std::make_unique<APVTS::ComboBoxAttachment>(vts, kapd::param::chordSnapMode, chordSnapModeBox);
    advanceOnTransientAttach = std::make_unique<APVTS::ButtonAttachment>(vts, kapd::param::advanceOnTransient, advanceOnTransientButton);
    transientSensitivityAttach = std::make_unique<APVTS::SliderAttachment>(vts, kapd::param::transientSensitivity, transientSensitivitySlider);

    tempoSyncAttach = std::make_unique<APVTS::ButtonAttachment>(vts, kapd::param::tempoSync, tempoSyncButton);
    delayDivisionAttach = std::make_unique<APVTS::ComboBoxAttachment>(vts, kapd::param::delayDivision, delayDivisionBox);

    delayMsAttach = std::make_unique<APVTS::SliderAttachment>(vts, kapd::param::delayMs, delayMsSlider);
    feedbackAttach = std::make_unique<APVTS::SliderAttachment>(vts, kapd::param::feedback, feedbackSlider);
    mixAttach = std::make_unique<APVTS::SliderAttachment>(vts, kapd::param::mix, mixSlider);
    outputGainAttach = std::make_unique<APVTS::SliderAttachment>(vts, kapd::param::outputGain, outputGainSlider);
    sequenceLengthAttach = std::make_unique<APVTS::SliderAttachment>(vts, kapd::param::sequenceLength, sequenceLengthSlider);
    smoothingAttach = std::make_unique<APVTS::SliderAttachment>(vts, kapd::param::smoothingMs, smoothingSlider);

    const char* intervalIds[kMaxSteps] = {
        kapd::param::intervalStep1, kapd::param::intervalStep2, kapd::param::intervalStep3, kapd::param::intervalStep4,
        kapd::param::intervalStep5, kapd::param::intervalStep6, kapd::param::intervalStep7, kapd::param::intervalStep8
    };

    const char* toneIds[kMaxSteps] = {
        kapd::param::toneStep1, kapd::param::toneStep2, kapd::param::toneStep3, kapd::param::toneStep4,
        kapd::param::toneStep5, kapd::param::toneStep6, kapd::param::toneStep7, kapd::param::toneStep8
    };

    for (int i = 0; i < kMaxSteps; ++i)
    {
        intervalStepAttach[i] = std::make_unique<APVTS::SliderAttachment>(vts, intervalIds[i], intervalStepSliders[i]);
        toneStepAttach[i]     = std::make_unique<APVTS::SliderAttachment>(vts, toneIds[i], toneStepSliders[i]);
    }

    const char* levelIds[kMaxSteps] = {
        kapd::param::stepLevel1, kapd::param::stepLevel2, kapd::param::stepLevel3, kapd::param::stepLevel4,
        kapd::param::stepLevel5, kapd::param::stepLevel6, kapd::param::stepLevel7, kapd::param::stepLevel8
    };

    const char* panIds[kMaxSteps] = {
        kapd::param::stepPan1, kapd::param::stepPan2, kapd::param::stepPan3, kapd::param::stepPan4,
        kapd::param::stepPan5, kapd::param::stepPan6, kapd::param::stepPan7, kapd::param::stepPan8
    };

    for (int i = 0; i < kMaxSteps; ++i)
    {
        stepLevelAttach[i] = std::make_unique<APVTS::SliderAttachment>(vts, levelIds[i], stepLevelSliders[i]);
        stepPanAttach[i]   = std::make_unique<APVTS::SliderAttachment>(vts, panIds[i], stepPanSliders[i]);
    }

    const char* scaleIds[kScaleButtons] = {
        kapd::param::customScale0, kapd::param::customScale1, kapd::param::customScale2, kapd::param::customScale3,
        kapd::param::customScale4, kapd::param::customScale5, kapd::param::customScale6, kapd::param::customScale7,
        kapd::param::customScale8, kapd::param::customScale9, kapd::param::customScale10, kapd::param::customScale11
    };

    for (int i = 0; i < kScaleButtons; ++i)
        customScaleAttach[i] = std::make_unique<APVTS::ButtonAttachment>(vts, scaleIds[i], customScaleButtons[(size_t) i]);

    // Post-FX attachments
    saturationDriveAttach = std::make_unique<APVTS::SliderAttachment>(vts, kapd::param::saturationDrive, saturationDriveSlider);
    saturationMixAttach = std::make_unique<APVTS::SliderAttachment>(vts, kapd::param::saturationMix, saturationMixSlider);
    diffusionAmountAttach = std::make_unique<APVTS::SliderAttachment>(vts, kapd::param::diffusionAmount, diffusionAmountSlider);
    diffusionMixAttach = std::make_unique<APVTS::SliderAttachment>(vts, kapd::param::diffusionMix, diffusionMixSlider);
    lofiAmountAttach = std::make_unique<APVTS::SliderAttachment>(vts, kapd::param::lofiAmount, lofiAmountSlider);
    lofiMixAttach = std::make_unique<APVTS::SliderAttachment>(vts, kapd::param::lofiMix, lofiMixSlider);
    reverbDecayAttach = std::make_unique<APVTS::SliderAttachment>(vts, kapd::param::reverbDecay, reverbDecaySlider);
    reverbDampingAttach = std::make_unique<APVTS::SliderAttachment>(vts, kapd::param::reverbDamping, reverbDampingSlider);
    reverbMixAttach = std::make_unique<APVTS::SliderAttachment>(vts, kapd::param::reverbMix, reverbMixSlider);
    highpassFreqAttach = std::make_unique<APVTS::SliderAttachment>(vts, kapd::param::highpassFreq, highpassFreqSlider);
    lowpassFreqAttach = std::make_unique<APVTS::SliderAttachment>(vts, kapd::param::lowpassFreq, lowpassFreqSlider);

    startTimerHz(10);
    refreshVisibility();
}

KeyAwarePitchDelayAudioProcessorEditor::~KeyAwarePitchDelayAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void KeyAwarePitchDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    const int w = getWidth();
    const int h = getHeight();

    // Industrial palette
    const auto panelDark = juce::Colour(0xFF1A1A1E);
    const auto panelMid = juce::Colour(0xFF2A2A30);
    const auto metalDark = juce::Colour(0xFF4A4A52);
    const auto metalMid = juce::Colour(0xFF6A6A72);
    const auto tealPanel = juce::Colour(0xFF2D4A4A);
    const auto tealAccent = juce::Colour(0xFF4A8888);
    const auto hotPink = juce::Colour(0xFFFF4488);
    const auto chrome = juce::Colour(0xFFB8B8C0);
    const auto rustOrange = juce::Colour(0xFFB86830);

    // === BASE: Dark industrial panel ===
    {
        juce::ColourGradient bg(panelMid, 0, 0, panelDark, (float)w, (float)h, false);
        bg.addColour(0.5, panelDark.brighter(0.05f));
        g.setGradientFill(bg);
        g.fillRect(0, 0, w, h);
    }

    // === BRUSHED METAL TEXTURE ===
    juce::Random rng(777);
    for (int i = 0; i < 400; ++i)
    {
        int sx = rng.nextInt(w);
        int sy = rng.nextInt(h);
        int len = rng.nextInt(80) + 20;
        float alpha = rng.nextFloat() * 0.03f + 0.01f;
        g.setColour(chrome.withAlpha(alpha));
        g.drawLine((float)sx, (float)sy, (float)(sx + len), (float)sy, 0.5f);
    }

    // === WEATHERED SCRATCHES ===
    rng.setSeed(999);
    for (int i = 0; i < 50; ++i)
    {
        int sx = rng.nextInt(w);
        int sy = rng.nextInt(h);
        int ex = sx + rng.nextInt(100) - 50;
        int ey = sy + rng.nextInt(60) - 30;
        g.setColour(juce::Colours::black.withAlpha(rng.nextFloat() * 0.1f + 0.02f));
        g.drawLine((float)sx, (float)sy, (float)ex, (float)ey, rng.nextFloat() * 1.5f + 0.5f);
    }

    // === PANEL SECTIONS ===
    // Main control panel (teal tinted)
    auto mainPanel = juce::Rectangle<int>(8, 35, w - 220, 160);
    {
        juce::ColourGradient panel(tealPanel.darker(0.3f), (float)mainPanel.getX(), (float)mainPanel.getY(),
                                    tealPanel.darker(0.5f), (float)mainPanel.getX(), (float)mainPanel.getBottom(), false);
        g.setGradientFill(panel);
        g.fillRoundedRectangle(mainPanel.toFloat(), 4.0f);
        g.setColour(metalDark);
        g.drawRoundedRectangle(mainPanel.toFloat(), 4.0f, 1.5f);
    }

    // Post-FX panel (right side)
    auto fxPanel = juce::Rectangle<int>(w - 205, 35, 197, h - 45);
    {
        juce::ColourGradient panel(panelMid.brighter(0.1f), (float)fxPanel.getX(), (float)fxPanel.getY(),
                                    panelDark, (float)fxPanel.getX(), (float)fxPanel.getBottom(), false);
        g.setGradientFill(panel);
        g.fillRoundedRectangle(fxPanel.toFloat(), 4.0f);
        g.setColour(metalDark);
        g.drawRoundedRectangle(fxPanel.toFloat(), 4.0f, 1.5f);

        // Section label
        g.setColour(hotPink);
        g.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
        g.drawText("POST-FX", fxPanel.getX() + 8, fxPanel.getY() + 4, 80, 16, juce::Justification::centredLeft);
    }

    // Sequencer panel
    auto seqPanel = juce::Rectangle<int>(8, 200, w - 220, h - 210);
    {
        juce::ColourGradient panel(panelMid, (float)seqPanel.getX(), (float)seqPanel.getY(),
                                    panelDark.brighter(0.05f), (float)seqPanel.getX(), (float)seqPanel.getBottom(), false);
        g.setGradientFill(panel);
        g.fillRoundedRectangle(seqPanel.toFloat(), 4.0f);
        g.setColour(metalDark);
        g.drawRoundedRectangle(seqPanel.toFloat(), 4.0f, 1.5f);
    }

    // === SCREWS / RIVETS ===
    auto drawScrew = [&](int x, int y) {
        g.setColour(metalDark);
        g.fillEllipse((float)x - 5, (float)y - 5, 10.0f, 10.0f);
        g.setColour(metalMid);
        g.fillEllipse((float)x - 4, (float)y - 4, 8.0f, 8.0f);
        g.setColour(panelDark);
        g.drawLine((float)x - 2, (float)y, (float)x + 2, (float)y, 1.5f);
    };

    drawScrew(20, 45);
    drawScrew(w - 225, 45);
    drawScrew(20, h - 20);
    drawScrew(w - 225, h - 20);
    drawScrew(w - 20, 45);
    drawScrew(w - 20, h - 20);

    // === TITLE ===
    // Embossed/engraved look
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.setFont(juce::Font(juce::FontOptions(20.0f).withStyle("Bold")));
    g.drawText("KEYAWARE PITCH DELAY", 13, 9, w - 230, 24, juce::Justification::centredLeft);

    g.setColour(chrome);
    g.drawText("KEYAWARE PITCH DELAY", 12, 8, w - 230, 24, juce::Justification::centredLeft);

    // Accent line under title
    g.setColour(hotPink);
    g.fillRect(12, 30, 200, 2);

    // === VU METER STYLE DECORATION (subtle) ===
    for (int i = 0; i < 12; ++i)
    {
        float barX = (float)(w - 195 + i * 15);
        float barH = 30.0f + std::sin(i * 0.5f) * 15.0f;
        auto barColor = (i > 8) ? rustOrange : (i > 5) ? tealAccent.brighter(0.2f) : tealAccent;
        g.setColour(barColor.withAlpha(0.15f));
        g.fillRect(barX, (float)(h - 30 - barH), 10.0f, barH);
    }

    // Version tag
    g.setColour(chrome.withAlpha(0.4f));
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText("v1.0 AAAPLUGIN", w - 100, h - 18, 90, 14, juce::Justification::centredRight);
}

void KeyAwarePitchDelayAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(28);

    // Right panel for post-FX
    auto fxArea = area.removeFromRight(200);
    fxArea.removeFromTop(8);

    // Main area
    area.removeFromRight(8); // gap

    // === TOP CONTROL ROW ===
    auto controlsRow = area.removeFromTop(55);
    {
        auto r = controlsRow;
        auto placeControl = [](juce::Rectangle<int> area, juce::Label& l, juce::Component& c, int labelH = 14, int controlH = 28) {
            l.setBounds(area.removeFromTop(labelH));
            c.setBounds(area.removeFromTop(controlH));
        };

        placeControl(r.removeFromLeft(50).reduced(2), keyRootLabel, keyRootBox);
        placeControl(r.removeFromLeft(90).reduced(2), scaleTypeLabel, scaleTypeBox);
        placeControl(r.removeFromLeft(60).reduced(2), modeLabel, modeButton);
        placeControl(r.removeFromLeft(55).reduced(2), routingLabel, routingButton);
        placeControl(r.removeFromLeft(40).reduced(2), tempoSyncLabel, tempoSyncButton);
        placeControl(r.removeFromLeft(70).reduced(2), delayDivisionLabel, delayDivisionBox);
        placeControl(r.removeFromLeft(65).reduced(2), pitchSourceLabel, pitchSourceBox);
        placeControl(r.removeFromLeft(45).reduced(2), trackingLabel, trackingSourceButton);
        placeControl(r.removeFromLeft(70).reduced(2), fixedNoteLabel, fixedNoteBox);
        placeControl(r.removeFromLeft(45).reduced(2), snapToChordLabel, snapToChordButton);
        placeControl(r.removeFromLeft(70).reduced(2), chordSnapModeLabel, chordSnapModeBox);
        placeControl(r.removeFromLeft(45).reduced(2), advanceOnTransientLabel, advanceOnTransientButton);
    }

    // === KNOBS ROW ===
    auto knobsRow = area.removeFromTop(110);
    {
        auto r = knobsRow.reduced(0, 4);
        auto knobW = r.getWidth() / 6;
        auto placeKnob = [](juce::Rectangle<int> rr, juce::Label& l, juce::Component& c) {
            l.setBounds(rr.removeFromTop(14));
            c.setBounds(rr);
        };

        placeKnob(r.removeFromLeft(knobW).reduced(4), delayMsLabel, delayMsSlider);
        placeKnob(r.removeFromLeft(knobW).reduced(4), transientSensitivityLabel, transientSensitivitySlider);
        placeKnob(r.removeFromLeft(knobW).reduced(4), feedbackLabel, feedbackSlider);
        placeKnob(r.removeFromLeft(knobW).reduced(4), mixLabel, mixSlider);
        placeKnob(r.removeFromLeft(knobW).reduced(4), outputGainLabel, outputGainSlider);
        placeKnob(r.removeFromLeft(knobW).reduced(4), smoothingLabel, smoothingSlider);
    }

    // === SEQUENCER SECTION ===
    auto stepsArea = area.reduced(0, 4);

    // Sequence length slider
    auto seqRow = stepsArea.removeFromTop(32);
    sequenceLengthLabel.setBounds(seqRow.removeFromLeft(50).reduced(2));
    sequenceLengthSlider.setBounds(seqRow.removeFromLeft(150).reduced(2));

    // Step sliders label
    auto stepsLabelArea = stepsArea.removeFromTop(18);
    intervalStepsLabel.setBounds(stepsLabelArea);
    toneStepsLabel.setBounds(stepsLabelArea);

    // Step sliders
    auto stepSliderRow = stepsArea.removeFromTop(120);
    auto stepW = stepSliderRow.getWidth() / kMaxSteps;

    auto intervalRowCopy = stepSliderRow;
    auto toneRowCopy = stepSliderRow;
    for (int i = 0; i < kMaxSteps; ++i)
    {
        intervalStepSliders[i].setBounds(intervalRowCopy.removeFromLeft(stepW).reduced(4));
        toneStepSliders[i].setBounds(toneRowCopy.removeFromLeft(stepW).reduced(4));
    }

    // Level faders
    stepLevelLabel.setBounds(stepsArea.removeFromTop(16));
    auto levelRow = stepsArea.removeFromTop(90);
    stepW = levelRow.getWidth() / kMaxSteps;
    for (int i = 0; i < kMaxSteps; ++i)
        stepLevelSliders[i].setBounds(levelRow.removeFromLeft(stepW).reduced(6, 2));

    // Pan sliders
    stepPanLabel.setBounds(stepsArea.removeFromTop(16));
    auto panRow = stepsArea.removeFromTop(40);
    stepW = panRow.getWidth() / kMaxSteps;
    for (int i = 0; i < kMaxSteps; ++i)
        stepPanSliders[i].setBounds(panRow.removeFromLeft(stepW).reduced(4, 2));

    // Custom scale
    customScaleLabel.setBounds(stepsArea.removeFromTop(16));
    auto scaleRow = stepsArea.removeFromTop(28);
    auto scW = scaleRow.getWidth() / kScaleButtons;
    for (int i = 0; i < kScaleButtons; ++i)
        customScaleButtons[(size_t) i].setBounds(scaleRow.removeFromLeft(scW).reduced(1));

    // === POST-FX PANEL LAYOUT ===
    fxArea.removeFromTop(20); // title space

    auto placeSmallKnobPair = [](juce::Rectangle<int>& area, juce::Label& title,
                                  juce::Label& l1, juce::Slider& s1,
                                  juce::Label& l2, juce::Slider& s2, int knobSize = 50) {
        auto section = area.removeFromTop(knobSize + 30);
        title.setBounds(section.removeFromTop(14));
        auto knobRow = section;
        auto halfW = knobRow.getWidth() / 2;

        auto k1 = knobRow.removeFromLeft(halfW).reduced(4);
        l1.setBounds(k1.removeFromBottom(12));
        s1.setBounds(k1);

        auto k2 = knobRow.reduced(4);
        l2.setBounds(k2.removeFromBottom(12));
        s2.setBounds(k2);

        area.removeFromTop(4);
    };

    auto placeTripleKnob = [](juce::Rectangle<int>& area, juce::Label& title,
                               juce::Label& l1, juce::Slider& s1,
                               juce::Label& l2, juce::Slider& s2,
                               juce::Label& l3, juce::Slider& s3, int knobSize = 45) {
        auto section = area.removeFromTop(knobSize + 30);
        title.setBounds(section.removeFromTop(14));
        auto knobRow = section;
        auto thirdW = knobRow.getWidth() / 3;

        auto k1 = knobRow.removeFromLeft(thirdW).reduced(2);
        l1.setBounds(k1.removeFromBottom(12));
        s1.setBounds(k1);

        auto k2 = knobRow.removeFromLeft(thirdW).reduced(2);
        l2.setBounds(k2.removeFromBottom(12));
        s2.setBounds(k2);

        auto k3 = knobRow.reduced(2);
        l3.setBounds(k3.removeFromBottom(12));
        s3.setBounds(k3);

        area.removeFromTop(4);
    };

    placeSmallKnobPair(fxArea, saturationLabel, satDriveLabel, saturationDriveSlider, satMixLabel, saturationMixSlider);
    placeSmallKnobPair(fxArea, diffusionLabel, diffAmtLabel, diffusionAmountSlider, diffMixLabel, diffusionMixSlider);
    placeSmallKnobPair(fxArea, lofiLabel, lofiAmtLabel, lofiAmountSlider, lofiMixLabel, lofiMixSlider);
    placeTripleKnob(fxArea, reverbLabel, revDecayLabel, reverbDecaySlider, revDampLabel, reverbDampingSlider, revMixLabel, reverbMixSlider);
    placeSmallKnobPair(fxArea, filterLabel, hpfLabel, highpassFreqSlider, lpfLabel, lowpassFreqSlider);

    refreshVisibility();
}

void KeyAwarePitchDelayAudioProcessorEditor::timerCallback()
{
    refreshVisibility();
}

void KeyAwarePitchDelayAudioProcessorEditor::refreshVisibility()
{
    const bool intervalMode = processor.apvts.getRawParameterValue(kapd::param::mode)->load() < 0.5f;

    intervalStepsLabel.setVisible(intervalMode);
    toneStepsLabel.setVisible(! intervalMode);

    const bool isParallel = processor.apvts.getRawParameterValue(kapd::param::routing)->load() > 0.5f;
    routingButton.setButtonText(isParallel ? "Parallel" : "Serial");

    const int seqLen = (int) processor.apvts.getRawParameterValue(kapd::param::sequenceLength)->load();
    for (int i = 0; i < kMaxSteps; ++i)
    {
        bool shouldShow = (i < seqLen);
        intervalStepSliders[i].setVisible(shouldShow && intervalMode);
        toneStepSliders[i].setVisible(shouldShow && !intervalMode);
        stepLevelSliders[i].setVisible(shouldShow);
        stepPanSliders[i].setVisible(shouldShow);
    }

    const auto pitchSource = (int) processor.apvts.getRawParameterValue(kapd::param::pitchSource)->load();
    const bool fixed = (pitchSource == 2);
    const bool audio = (pitchSource == 0);

    fixedNoteLabel.setEnabled(fixed);
    fixedNoteBox.setEnabled(fixed);
    fixedNoteLabel.setAlpha(fixed ? 1.0f : 0.35f);
    fixedNoteBox.setAlpha(fixed ? 1.0f : 0.35f);

    trackingLabel.setEnabled(audio);
    trackingSourceButton.setEnabled(audio);
    trackingLabel.setAlpha(audio ? 1.0f : 0.35f);
    trackingSourceButton.setAlpha(audio ? 1.0f : 0.35f);

    const bool sync = processor.apvts.getRawParameterValue(kapd::param::tempoSync)->load() > 0.5f;
    delayDivisionLabel.setEnabled(sync);
    delayDivisionBox.setEnabled(sync);
    delayDivisionLabel.setAlpha(sync ? 1.0f : 0.35f);
    delayDivisionBox.setAlpha(sync ? 1.0f : 0.35f);

    delayMsLabel.setEnabled(!sync);
    delayMsSlider.setEnabled(!sync);
    delayMsLabel.setAlpha(!sync ? 1.0f : 0.35f);
    delayMsSlider.setAlpha(!sync ? 1.0f : 0.35f);

    const bool transient = processor.apvts.getRawParameterValue(kapd::param::advanceOnTransient)->load() > 0.5f;
    transientSensitivityLabel.setEnabled(transient);
    transientSensitivitySlider.setEnabled(transient);
    transientSensitivityLabel.setAlpha(transient ? 1.0f : 0.35f);
    transientSensitivitySlider.setAlpha(transient ? 1.0f : 0.35f);

    const bool snap = processor.apvts.getRawParameterValue(kapd::param::snapToChord)->load() > 0.5f;
    chordSnapModeLabel.setEnabled(snap);
    chordSnapModeBox.setEnabled(snap);
    chordSnapModeLabel.setAlpha(snap ? 1.0f : 0.35f);
    chordSnapModeBox.setAlpha(snap ? 1.0f : 0.35f);

    const auto scaleType = (int) processor.apvts.getRawParameterValue(kapd::param::scaleType)->load();
    const bool custom = (scaleType == 7);

    customScaleLabel.setEnabled(custom);
    customScaleLabel.setAlpha(custom ? 1.0f : 0.35f);
    for (auto& b : customScaleButtons)
    {
        b.setEnabled(custom);
        b.setAlpha(custom ? 1.0f : 0.35f);
    }
}
