#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"

class KeyAwarePitchDelayAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                    private juce::Timer
{
public:
    explicit KeyAwarePitchDelayAudioProcessorEditor (KeyAwarePitchDelayAudioProcessor&);
    ~KeyAwarePitchDelayAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using APVTS = juce::AudioProcessorValueTreeState;

    void timerCallback() override;
    void refreshVisibility();

    KeyAwarePitchDelayAudioProcessor& processor;
    RetroLookAndFeel retroLookAndFeel;

    // Top-level controls
    juce::ComboBox keyRootBox, scaleTypeBox;
    juce::ToggleButton modeButton, routingButton;
    juce::ComboBox pitchSourceBox, fixedNoteBox;
    juce::ToggleButton trackingSourceButton;
    juce::ToggleButton snapToChordButton;
    juce::ComboBox chordSnapModeBox;
    juce::ToggleButton advanceOnTransientButton;
    juce::ToggleButton tempoSyncButton;
    juce::ComboBox delayDivisionBox;

    juce::Slider transientSensitivitySlider;

    juce::Slider delayMsSlider;
    juce::Slider feedbackSlider, mixSlider, outputGainSlider, sequenceLengthSlider, smoothingSlider;

    // Sequence steps
    static constexpr int kMaxSteps = 8;
    std::array<juce::Slider, kMaxSteps> intervalStepSliders;
    std::array<juce::Slider, kMaxSteps> toneStepSliders;

    // Per-step level / pan
    std::array<juce::Slider, kMaxSteps> stepLevelSliders;
    std::array<juce::Slider, kMaxSteps> stepPanSliders;

    // Custom scale editor
    static constexpr int kScaleButtons = 12;
    std::array<juce::ToggleButton, kScaleButtons> customScaleButtons;

    // Post-FX controls
    juce::Slider saturationDriveSlider, saturationMixSlider;
    juce::Slider diffusionAmountSlider, diffusionMixSlider;
    juce::Slider lofiAmountSlider, lofiMixSlider;
    juce::Slider reverbDecaySlider, reverbDampingSlider, reverbMixSlider;
    juce::Slider highpassFreqSlider, lowpassFreqSlider;

    // Labels
    juce::Label keyRootLabel, scaleTypeLabel, modeLabel, routingLabel;
    juce::Label pitchSourceLabel, trackingLabel, fixedNoteLabel;
    juce::Label snapToChordLabel, advanceOnTransientLabel, transientSensitivityLabel;
    juce::Label chordSnapModeLabel;
    juce::Label tempoSyncLabel, delayDivisionLabel, delayMsLabel;
    juce::Label feedbackLabel, mixLabel, outputGainLabel, sequenceLengthLabel, smoothingLabel;
    juce::Label intervalStepsLabel, toneStepsLabel;
    juce::Label stepLevelLabel, stepPanLabel;
    juce::Label customScaleLabel;

    // Post-FX labels
    juce::Label saturationLabel, diffusionLabel, lofiLabel, reverbLabel, filterLabel;
    juce::Label satDriveLabel, satMixLabel, diffAmtLabel, diffMixLabel;
    juce::Label lofiAmtLabel, lofiMixLabel;
    juce::Label revDecayLabel, revDampLabel, revMixLabel;
    juce::Label hpfLabel, lpfLabel;

    // Attachments
    std::unique_ptr<APVTS::ComboBoxAttachment> keyRootAttach, scaleTypeAttach;
    std::unique_ptr<APVTS::ButtonAttachment> modeAttach, routingAttach;
    std::unique_ptr<APVTS::ComboBoxAttachment> pitchSourceAttach, fixedNoteAttach;
    std::unique_ptr<APVTS::ButtonAttachment> trackingSourceAttach;
    std::unique_ptr<APVTS::ButtonAttachment> snapToChordAttach, advanceOnTransientAttach;
    std::unique_ptr<APVTS::ComboBoxAttachment> chordSnapModeAttach;
    std::unique_ptr<APVTS::ButtonAttachment> tempoSyncAttach;
    std::unique_ptr<APVTS::ComboBoxAttachment> delayDivisionAttach;
    std::unique_ptr<APVTS::SliderAttachment> delayMsAttach, feedbackAttach, mixAttach, outputGainAttach, sequenceLengthAttach, smoothingAttach;
    std::unique_ptr<APVTS::SliderAttachment> transientSensitivityAttach;

    std::array<std::unique_ptr<APVTS::SliderAttachment>, kMaxSteps> intervalStepAttach;
    std::array<std::unique_ptr<APVTS::SliderAttachment>, kMaxSteps> toneStepAttach;

    std::array<std::unique_ptr<APVTS::SliderAttachment>, kMaxSteps> stepLevelAttach;
    std::array<std::unique_ptr<APVTS::SliderAttachment>, kMaxSteps> stepPanAttach;

    std::array<std::unique_ptr<APVTS::ButtonAttachment>, kScaleButtons> customScaleAttach;

    // Post-FX attachments
    std::unique_ptr<APVTS::SliderAttachment> saturationDriveAttach, saturationMixAttach;
    std::unique_ptr<APVTS::SliderAttachment> diffusionAmountAttach, diffusionMixAttach;
    std::unique_ptr<APVTS::SliderAttachment> lofiAmountAttach, lofiMixAttach;
    std::unique_ptr<APVTS::SliderAttachment> reverbDecayAttach, reverbDampingAttach, reverbMixAttach;
    std::unique_ptr<APVTS::SliderAttachment> highpassFreqAttach, lowpassFreqAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyAwarePitchDelayAudioProcessorEditor)
};
