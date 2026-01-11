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
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
}

KeyAwarePitchDelayAudioProcessorEditor::KeyAwarePitchDelayAudioProcessorEditor (KeyAwarePitchDelayAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel(&retroLookAndFeel);
    setSize (1000, 720);

    // Populate combos
    addChoiceItems(keyRootBox, kapd::ChoiceLists::keyRoots());
    addChoiceItems(scaleTypeBox, kapd::ChoiceLists::scaleTypes());
    addChoiceItems(pitchSourceBox, kapd::ChoiceLists::pitchSources());
    addChoiceItems(fixedNoteBox, kapd::ChoiceLists::fixedNoteChoices());
    addChoiceItems(chordSnapModeBox, kapd::ChoiceLists::chordSnapModes());
    addChoiceItems(delayDivisionBox, kapd::ChoiceLists::delayDivisions());

    // Toggle buttons with descriptive text
    modeButton.setButtonText("Tone Seq");      // OFF=Diatonic Interval, ON=Scale Tone Sequence
    routingButton.setButtonText("Serial");     // Will update dynamically
    trackingSourceButton.setButtonText("Loop"); // OFF=Input, ON=Feedback/Loop

    // Make toggle buttons update their text when clicked
    routingButton.onClick = [this]() {
        routingButton.setButtonText(routingButton.getToggleState() ? "Parallel" : "Serial");
    };

    // Clicking delay ms knob disables sync mode
    delayMsSlider.onDragStart = [this]() {
        if (auto* param = processor.apvts.getParameter(kapd::param::tempoSync))
            param->setValueNotifyingHost(0.0f);
    };

    // Clicking division dropdown enables sync mode
    delayDivisionBox.onChange = [this]() {
        if (auto* param = processor.apvts.getParameter(kapd::param::tempoSync))
            param->setValueNotifyingHost(1.0f);
    };

    // Custom scale buttons (intervals relative to root)
    static const juce::StringArray customLabels { "1", "b2", "2", "b3", "3", "4", "#4", "5", "b6", "6", "b7", "7" };
    for (int i = 0; i < (int) customScaleButtons.size(); ++i)
        customScaleButtons[(size_t) i].setButtonText(customLabels[i]);

    // Interval step sliders (0-14 snapping to integers) with descriptive labels
    static const juce::StringArray intervalLabels {
        "-7 (8ve down)", "-6 (7th down)", "-5 (6th down)", "-4 (5th down)",
        "-3 (4th down)", "-2 (3rd down)", "-1 (2nd down)", "0 (unison)",
        "+1 (2nd up)", "+2 (3rd up)", "+3 (4th up)", "+4 (5th up)",
        "+5 (6th up)", "+6 (7th up)", "+7 (8ve up)"
    };
    for (auto& s : intervalStepSliders)
    {
        s.setSliderStyle(juce::Slider::LinearVertical);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 20);
        s.setRange(0, 14, 1);
        s.textFromValueFunction = [](double v) { return intervalLabels[(int)v]; };
        s.valueFromTextFunction = [](const juce::String& t) {
            return (double) intervalLabels.indexOf(t);
        };
        s.updateText();
    }

    // Tone step sliders (0-11 snapping to integers) with descriptive labels
    static const juce::StringArray toneLabels {
        "1 (Root)", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"
    };
    for (auto& s : toneStepSliders)
    {
        s.setSliderStyle(juce::Slider::LinearVertical);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        s.setRange(0, 11, 1);
        s.textFromValueFunction = [](double v) { return toneLabels[(int)v]; };
        s.valueFromTextFunction = [](const juce::String& t) {
            return (double) toneLabels.indexOf(t);
        };
        s.updateText();
    }

    // Sliders
    initKnob(delayMsSlider);
    initKnob(feedbackSlider);
    initKnob(mixSlider);
    initKnob(outputGainSlider);
    initKnob(smoothingSlider);
    initKnob(transientSensitivitySlider);

    // Step level sliders - mixer style faders
    for (auto& s : stepLevelSliders)
    {
        s.setSliderStyle(juce::Slider::LinearVertical);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 16);
    }

    // Step pan sliders - horizontal for intuitive L/R control
    for (auto& s : stepPanSliders)
    {
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 16);
    }

    sequenceLengthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    sequenceLengthSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 70, 18);

    // Labels
    initLabel(keyRootLabel, "Key");
    initLabel(scaleTypeLabel, "Scale");
    initLabel(modeLabel, "Mode");
    initLabel(routingLabel, "Routing");

    initLabel(pitchSourceLabel, "Pitch Src");
    initLabel(trackingLabel, "Track");
    initLabel(fixedNoteLabel, "Fixed Note");

    initLabel(snapToChordLabel, "Chord");
    initLabel(chordSnapModeLabel, "Snap Mode");
    initLabel(advanceOnTransientLabel, "Transient");
    initLabel(transientSensitivityLabel, "Sens.");

    initLabel(tempoSyncLabel, "Sync");
    initLabel(delayDivisionLabel, "Division");
    initLabel(delayMsLabel, "Delay (ms)");

    initLabel(feedbackLabel, "Feedback");
    initLabel(mixLabel, "Mix");
    initLabel(outputGainLabel, "Output (dB)");
    initLabel(sequenceLengthLabel, "Seq Len");
    initLabel(smoothingLabel, "Smooth (ms)");

    initLabel(intervalStepsLabel, "Interval Steps (Mode 1)");
    initLabel(toneStepsLabel, "Tone Steps (Mode 2)");

    initLabel(stepLevelLabel, "Step Level");
    initLabel(stepPanLabel, "Step Pan");
    initLabel(customScaleLabel, "Custom Scale");

    // Add components
    auto addPair = [this](juce::Label& l, juce::Component& c)
    {
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

    startTimerHz(10);
    refreshVisibility();
}

KeyAwarePitchDelayAudioProcessorEditor::~KeyAwarePitchDelayAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void KeyAwarePitchDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    juce::Random rng;
    const int w = getWidth();
    const int h = getHeight();

    // === PALETTE - Y2K / Digital Collage ===
    const auto deepBlack = juce::Colour(0xFF0A0A0F);
    const auto darkPurple = juce::Colour(0xFF1A1025);
    const auto midPurple = juce::Colour(0xFF2D1B3D);
    const auto neonCyan = juce::Colour(0xFF00FFFF);
    const auto neonMagenta = juce::Colour(0xFFFF00FF);
    const auto acidGreen = juce::Colour(0xFF39FF14);
    const auto hotPink = juce::Colour(0xFFFF1493);
    const auto chrome = juce::Colour(0xFFC0C0C0);
    const auto holoPink = juce::Colour(0xFFFFB6C1);
    const auto holoBlue = juce::Colour(0xFF87CEEB);
    const auto holoGreen = juce::Colour(0xFF98FB98);

    // === BASE: Dark gradient with purple tones ===
    {
        juce::ColourGradient bg(deepBlack, 0, 0, darkPurple, (float)w, (float)h, false);
        bg.addColour(0.3, midPurple.darker(0.3f));
        bg.addColour(0.7, darkPurple);
        g.setGradientFill(bg);
        g.fillRect(bounds);
    }

    // === LAYER 1: Holographic/iridescent sweep ===
    rng.setSeed(42);
    for (int i = 0; i < 5; ++i)
    {
        float startX = rng.nextFloat() * w * 0.3f;
        float startY = rng.nextFloat() * h;
        float endX = startX + w * 0.7f;
        float endY = startY + rng.nextFloat() * 100 - 50;

        juce::ColourGradient holo(holoPink.withAlpha(0.03f), startX, startY,
                                   holoBlue.withAlpha(0.03f), endX, endY, false);
        holo.addColour(0.5, holoGreen.withAlpha(0.02f));
        g.setGradientFill(holo);
        g.fillRect(0, (int)startY - 30, w, 60);
    }

    // === LAYER 2: Scattered "window" frames - Y2K UI aesthetic ===
    rng.setSeed(555);
    for (int i = 0; i < 8; ++i)
    {
        int fx = rng.nextInt(w) - 50;
        int fy = rng.nextInt(h) - 50;
        int fw = rng.nextInt(200) + 80;
        int fh = rng.nextInt(150) + 60;
        float alpha = rng.nextFloat() * 0.06f + 0.02f;

        // Window fill - semi-transparent
        g.setColour(midPurple.withAlpha(alpha));
        g.fillRect(fx, fy, fw, fh);

        // Title bar - chrome gradient
        juce::ColourGradient titleBar(chrome.withAlpha(alpha * 2), (float)fx, (float)fy,
                                       chrome.darker(0.4f).withAlpha(alpha * 2), (float)fx, (float)(fy + 18), false);
        g.setGradientFill(titleBar);
        g.fillRect(fx, fy, fw, 18);

        // Window border - neon accent
        auto borderColor = (i % 3 == 0) ? neonCyan : (i % 3 == 1) ? neonMagenta : acidGreen;
        g.setColour(borderColor.withAlpha(alpha * 1.5f));
        g.drawRect(fx, fy, fw, fh, 1);

        // Window buttons (dots in title bar)
        for (int b = 0; b < 3; ++b)
        {
            g.setColour(chrome.withAlpha(alpha * 3));
            g.fillEllipse((float)(fx + fw - 14 - b * 12), (float)(fy + 5), 8.0f, 8.0f);
        }
    }

    // === LAYER 3: Dithering pattern - checkered noise ===
    rng.setSeed(888);
    for (int y = 0; y < h; y += 2)
    {
        for (int x = 0; x < w; x += 2)
        {
            if ((x + y) % 4 == 0)
            {
                float noise = rng.nextFloat();
                if (noise > 0.7f)
                {
                    g.setColour(juce::Colours::white.withAlpha(0.015f));
                    g.fillRect(x, y, 1, 1);
                }
            }
        }
    }

    // === LAYER 4: CRT Scanlines ===
    for (int y = 0; y < h; y += 3)
    {
        float alpha = 0.08f + std::sin(y * 0.02f) * 0.02f;
        g.setColour(juce::Colours::black.withAlpha(alpha));
        g.fillRect(0, y, w, 1);
    }

    // === LAYER 5: RGB color shift / glitch bands ===
    rng.setSeed(333);
    for (int i = 0; i < 6; ++i)
    {
        int gy = rng.nextInt(h);
        int gh = rng.nextInt(20) + 5;
        int offset = rng.nextInt(8) - 4;

        // Red channel shift
        g.setColour(juce::Colour(0xFFFF0000).withAlpha(0.04f));
        g.fillRect(offset, gy, w, gh);

        // Cyan channel shift (opposite)
        g.setColour(neonCyan.withAlpha(0.03f));
        g.fillRect(-offset, gy + 1, w, gh);
    }

    // === LAYER 6: Chrome/metallic diagonal streaks ===
    rng.setSeed(222);
    for (int i = 0; i < 12; ++i)
    {
        float x1 = rng.nextFloat() * w;
        float y1 = rng.nextFloat() * h;
        float len = rng.nextFloat() * 300 + 100;
        float angle = 0.7f + rng.nextFloat() * 0.3f; // ~40-60 degrees

        juce::ColourGradient streak(chrome.withAlpha(0.0f), x1, y1,
                                     chrome.withAlpha(0.08f), x1 + len * 0.5f, y1 + len * 0.5f, false);
        streak.addColour(0.5, chrome.withAlpha(0.12f));
        streak.addColour(1.0, chrome.withAlpha(0.0f));

        g.setGradientFill(streak);
        g.drawLine(x1, y1, x1 + len * std::cos(angle), y1 + len * std::sin(angle), 2.0f);
    }

    // === LAYER 7: Scattered UI symbols / icons ===
    rng.setSeed(666);
    juce::StringArray symbols = { "+", "x", "o", "*", "//", "[]", "<>", "::", "##" };
    g.setFont(juce::Font(10.0f));
    for (int i = 0; i < 40; ++i)
    {
        int sx = rng.nextInt(w);
        int sy = rng.nextInt(h);
        auto symbolColor = (i % 4 == 0) ? neonCyan : (i % 4 == 1) ? neonMagenta :
                           (i % 4 == 2) ? acidGreen : chrome;
        g.setColour(symbolColor.withAlpha(rng.nextFloat() * 0.15f + 0.05f));
        g.drawText(symbols[rng.nextInt(symbols.size())], sx, sy, 20, 12, juce::Justification::centred);
    }

    // === LAYER 8: Neon glow spots ===
    rng.setSeed(111);
    for (int i = 0; i < 8; ++i)
    {
        float gx = rng.nextFloat() * w;
        float gy = rng.nextFloat() * h;
        float radius = rng.nextFloat() * 100 + 50;
        auto glowColor = (i % 3 == 0) ? neonCyan : (i % 3 == 1) ? neonMagenta : hotPink;

        juce::ColourGradient glow(glowColor.withAlpha(0.06f), gx, gy,
                                   juce::Colours::transparentBlack, gx + radius, gy, true);
        g.setGradientFill(glow);
        g.fillEllipse(gx - radius, gy - radius, radius * 2, radius * 2);
    }

    // === LAYER 9: CD/holographic reflection arc ===
    {
        float cx = w * 0.7f;
        float cy = h * 0.4f;
        float arcRadius = 400.0f;

        for (int a = 0; a < 180; a += 2)
        {
            float angle = a * 3.14159f / 180.0f;
            float x = cx + arcRadius * std::cos(angle);
            float y = cy + arcRadius * std::sin(angle) * 0.3f; // Elliptical

            // Rainbow shift based on angle
            float hue = (float)a / 180.0f;
            auto rainbowColor = juce::Colour::fromHSV(hue, 0.7f, 1.0f, 0.04f);
            g.setColour(rainbowColor);
            g.fillEllipse(x - 3, y - 3, 6, 6);
        }
    }

    // === LAYER 10: Decorative border - layered neon ===
    // Outer glow
    g.setColour(neonMagenta.withAlpha(0.1f));
    g.drawRect(bounds.reduced(2), 3);
    g.setColour(neonCyan.withAlpha(0.08f));
    g.drawRect(bounds.reduced(5), 2);

    // Corner accents
    int cornerSize = 30;
    g.setColour(acidGreen.withAlpha(0.15f));
    // Top-left
    g.drawLine(5, 5, 5 + cornerSize, 5, 2);
    g.drawLine(5, 5, 5, 5 + cornerSize, 2);
    // Top-right
    g.drawLine((float)(w - 5), 5, (float)(w - 5 - cornerSize), 5, 2);
    g.drawLine((float)(w - 5), 5, (float)(w - 5), 5 + cornerSize, 2);
    // Bottom-left
    g.drawLine(5, (float)(h - 5), 5 + cornerSize, (float)(h - 5), 2);
    g.drawLine(5, (float)(h - 5), 5, (float)(h - 5 - cornerSize), 2);
    // Bottom-right
    g.drawLine((float)(w - 5), (float)(h - 5), (float)(w - 5 - cornerSize), (float)(h - 5), 2);
    g.drawLine((float)(w - 5), (float)(h - 5), (float)(w - 5), (float)(h - 5 - cornerSize), 2);

    // === LAYER 11: Film grain overlay ===
    rng.setSeed(999);
    for (int i = 0; i < 3000; ++i)
    {
        int gx = rng.nextInt(w);
        int gy = rng.nextInt(h);
        float alpha = rng.nextFloat() * 0.05f;
        g.setColour(rng.nextBool() ? juce::Colours::white.withAlpha(alpha)
                                    : juce::Colours::black.withAlpha(alpha));
        g.fillRect(gx, gy, 1, 1);
    }

    // === TITLE: Chrome/neon text with glow ===
    // Glow layer
    g.setColour(neonCyan.withAlpha(0.3f));
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.drawText("KEYAWARE PITCH DELAY", 11, 9, w - 20, 22, juce::Justification::centredLeft);

    // Magenta offset (chromatic aberration)
    g.setColour(neonMagenta.withAlpha(0.2f));
    g.drawText("KEYAWARE PITCH DELAY", 13, 11, w - 20, 22, juce::Justification::centredLeft);

    // Main text - chrome
    g.setColour(chrome);
    g.setFont(juce::Font(17.0f, juce::Font::bold));
    g.drawText("KEYAWARE PITCH DELAY", 12, 10, w - 20, 20, juce::Justification::centredLeft);
}

void KeyAwarePitchDelayAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(28);

    // Single row for ALL controls - very compact
    auto controlsRow = area.removeFromTop(52);
    {
        auto r = controlsRow;
        auto placeControl = [](juce::Rectangle<int> area, juce::Label& l, juce::Component& c, int labelH = 14, int controlH = 26)
        {
            l.setBounds(area.removeFromTop(labelH));
            c.setBounds(area.removeFromTop(controlH));
        };

        placeControl(r.removeFromLeft(55).reduced(2), keyRootLabel, keyRootBox);
        placeControl(r.removeFromLeft(100).reduced(2), scaleTypeLabel, scaleTypeBox);
        placeControl(r.removeFromLeft(65).reduced(2), modeLabel, modeButton);
        placeControl(r.removeFromLeft(60).reduced(2), routingLabel, routingButton);
        placeControl(r.removeFromLeft(45).reduced(2), tempoSyncLabel, tempoSyncButton);
        placeControl(r.removeFromLeft(75).reduced(2), delayDivisionLabel, delayDivisionBox);
        placeControl(r.removeFromLeft(75).reduced(2), pitchSourceLabel, pitchSourceBox);
        placeControl(r.removeFromLeft(45).reduced(2), trackingLabel, trackingSourceButton);
        placeControl(r.removeFromLeft(75).reduced(2), fixedNoteLabel, fixedNoteBox);
        placeControl(r.removeFromLeft(45).reduced(2), snapToChordLabel, snapToChordButton);
        placeControl(r.removeFromLeft(80).reduced(2), chordSnapModeLabel, chordSnapModeBox);
        placeControl(r.removeFromLeft(50).reduced(2), advanceOnTransientLabel, advanceOnTransientButton);
    }

    // Knobs row: delay ms, sensitivity, feedback, mix, output, smoothing - LARGE
    auto knobsRow = area.removeFromTop(140);
    {
        auto r = knobsRow.reduced(0, 4);
        auto knobW = r.getWidth() / 6;
        auto placeKnob = [](juce::Rectangle<int> rr, juce::Label& l, juce::Component& c)
        {
            l.setBounds(rr.removeFromTop(18));
            c.setBounds(rr);
        };

        placeKnob(r.removeFromLeft(knobW).reduced(6), delayMsLabel, delayMsSlider);
        placeKnob(r.removeFromLeft(knobW).reduced(6), transientSensitivityLabel, transientSensitivitySlider);
        placeKnob(r.removeFromLeft(knobW).reduced(6), feedbackLabel, feedbackSlider);
        placeKnob(r.removeFromLeft(knobW).reduced(6), mixLabel, mixSlider);
        placeKnob(r.removeFromLeft(knobW).reduced(6), outputGainLabel, outputGainSlider);
        placeKnob(r.removeFromLeft(knobW).reduced(6), smoothingLabel, smoothingSlider);
    }


    // Remaining area: steps + step level/pan + custom scale editor
    auto stepsArea = area.reduced(0, 6);

    // Sequence length slider - right above the step sliders
    auto seqRow = stepsArea.removeFromTop(36);
    sequenceLengthLabel.setBounds(seqRow.removeFromLeft(70).reduced(4));
    sequenceLengthSlider.setBounds(seqRow.removeFromLeft(200).reduced(4));

    // Step sliders - interval and tone share the same space (toggle visibility)
    auto stepsLabelArea = stepsArea.removeFromTop(22);
    intervalStepsLabel.setBounds(stepsLabelArea);
    toneStepsLabel.setBounds(stepsLabelArea);  // Same position, visibility toggled

    auto stepSliderRow = stepsArea.removeFromTop(140);
    auto stepW = stepSliderRow.getWidth() / kMaxSteps;

    // Position both sets of sliders in the same space
    auto intervalRowCopy = stepSliderRow;
    auto toneRowCopy = stepSliderRow;
    for (int i = 0; i < kMaxSteps; ++i)
    {
        intervalStepSliders[i].setBounds(intervalRowCopy.removeFromLeft(stepW).reduced(4));
        toneStepSliders[i].setBounds(toneRowCopy.removeFromLeft(stepW).reduced(4));
    }

    // Mixer-style level faders - tall and prominent
    stepLevelLabel.setBounds(stepsArea.removeFromTop(20));
    auto levelRow = stepsArea.removeFromTop(120);
    stepW = levelRow.getWidth() / kMaxSteps;
    for (int i = 0; i < kMaxSteps; ++i)
        stepLevelSliders[i].setBounds(levelRow.removeFromLeft(stepW).reduced(8, 4));

    // Pan sliders - horizontal, wider for easy L/R control
    stepPanLabel.setBounds(stepsArea.removeFromTop(20));
    auto panRow = stepsArea.removeFromTop(50);
    stepW = panRow.getWidth() / kMaxSteps;
    for (int i = 0; i < kMaxSteps; ++i)
        stepPanSliders[i].setBounds(panRow.removeFromLeft(stepW).reduced(6, 4));

    customScaleLabel.setBounds(stepsArea.removeFromTop(18));
    auto scaleRow = stepsArea.removeFromTop(30);
    auto scW = scaleRow.getWidth() / kScaleButtons;
    for (int i = 0; i < kScaleButtons; ++i)
        customScaleButtons[(size_t) i].setBounds(scaleRow.removeFromLeft(scW).reduced(2));

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

    // Update routing button text based on state
    const bool isParallel = processor.apvts.getRawParameterValue(kapd::param::routing)->load() > 0.5f;
    routingButton.setButtonText(isParallel ? "Parallel" : "Serial");

    // Show/hide step sliders based on sequence length
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
    const bool custom = (scaleType == 7); // see ChoiceLists::scaleTypes() ordering

    customScaleLabel.setEnabled(custom);
    customScaleLabel.setAlpha(custom ? 1.0f : 0.35f);
    for (auto& b : customScaleButtons)
    {
        b.setEnabled(custom);
        b.setAlpha(custom ? 1.0f : 0.35f);
    }
}
