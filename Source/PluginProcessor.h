#pragma once

#include <JuceHeader.h>

namespace kapd
{
    // Parameter IDs
    namespace param
    {
        static constexpr const char* keyRoot         = "keyRoot";
        static constexpr const char* scaleType       = "scaleType";

        // Custom scale editor (12 semitone offsets relative to Key Root)
        static constexpr const char* customScale0    = "customScale0";  // 1
        static constexpr const char* customScale1    = "customScale1";  // b2
        static constexpr const char* customScale2    = "customScale2";  // 2
        static constexpr const char* customScale3    = "customScale3";  // b3
        static constexpr const char* customScale4    = "customScale4";  // 3
        static constexpr const char* customScale5    = "customScale5";  // 4
        static constexpr const char* customScale6    = "customScale6";  // #4/b5
        static constexpr const char* customScale7    = "customScale7";  // 5
        static constexpr const char* customScale8    = "customScale8";  // b6
        static constexpr const char* customScale9    = "customScale9";  // 6
        static constexpr const char* customScale10   = "customScale10"; // b7
        static constexpr const char* customScale11   = "customScale11"; // 7
        static constexpr const char* mode            = "mode";
        static constexpr const char* routing         = "routing";
        static constexpr const char* pitchSource     = "pitchSource";
        static constexpr const char* trackingSource  = "trackingSource";
        static constexpr const char* fixedMidi       = "fixedMidi";
        static constexpr const char* tempoSync       = "tempoSync";
        static constexpr const char* delayDivision   = "delayDivision";
        static constexpr const char* delayMs         = "delayMs";
        static constexpr const char* feedback        = "feedback";
        static constexpr const char* mix             = "mix";
        static constexpr const char* outputGain      = "outputGain";
        static constexpr const char* sequenceLength  = "sequenceLength";
        static constexpr const char* smoothingMs     = "smoothingMs";

        static constexpr const char* snapToChord        = "snapToChord";
        static constexpr const char* chordSnapMode      = "chordSnapMode";
        static constexpr const char* advanceOnTransient = "advanceOnTransient";
        static constexpr const char* transientSensitivity = "transientSensitivity";

        // 8 interval steps (Mode 1)
        static constexpr const char* intervalStep1 = "intervalStep1";
        static constexpr const char* intervalStep2 = "intervalStep2";
        static constexpr const char* intervalStep3 = "intervalStep3";
        static constexpr const char* intervalStep4 = "intervalStep4";
        static constexpr const char* intervalStep5 = "intervalStep5";
        static constexpr const char* intervalStep6 = "intervalStep6";
        static constexpr const char* intervalStep7 = "intervalStep7";
        static constexpr const char* intervalStep8 = "intervalStep8";

        // 8 tone steps (Mode 2)
        static constexpr const char* toneStep1 = "toneStep1";
        static constexpr const char* toneStep2 = "toneStep2";
        static constexpr const char* toneStep3 = "toneStep3";
        static constexpr const char* toneStep4 = "toneStep4";
        static constexpr const char* toneStep5 = "toneStep5";
        static constexpr const char* toneStep6 = "toneStep6";
        static constexpr const char* toneStep7 = "toneStep7";
        static constexpr const char* toneStep8 = "toneStep8";

        // Per-step level / pan (applies to both serial steps and parallel taps)
        static constexpr const char* stepLevel1 = "stepLevel1";
        static constexpr const char* stepLevel2 = "stepLevel2";
        static constexpr const char* stepLevel3 = "stepLevel3";
        static constexpr const char* stepLevel4 = "stepLevel4";
        static constexpr const char* stepLevel5 = "stepLevel5";
        static constexpr const char* stepLevel6 = "stepLevel6";
        static constexpr const char* stepLevel7 = "stepLevel7";
        static constexpr const char* stepLevel8 = "stepLevel8";

        static constexpr const char* stepPan1 = "stepPan1";
        static constexpr const char* stepPan2 = "stepPan2";
        static constexpr const char* stepPan3 = "stepPan3";
        static constexpr const char* stepPan4 = "stepPan4";
        static constexpr const char* stepPan5 = "stepPan5";
        static constexpr const char* stepPan6 = "stepPan6";
        static constexpr const char* stepPan7 = "stepPan7";
        static constexpr const char* stepPan8 = "stepPan8";

        // Post-FX chain (Mood/Form2 inspired)
        static constexpr const char* saturationDrive = "saturationDrive";
        static constexpr const char* saturationMix   = "saturationMix";
        static constexpr const char* diffusionAmount = "diffusionAmount";
        static constexpr const char* diffusionMix    = "diffusionMix";
        static constexpr const char* lofiAmount      = "lofiAmount";
        static constexpr const char* lofiMix         = "lofiMix";
        static constexpr const char* reverbDecay     = "reverbDecay";
        static constexpr const char* reverbDamping   = "reverbDamping";
        static constexpr const char* reverbMix       = "reverbMix";
        static constexpr const char* highpassFreq    = "highpassFreq";
        static constexpr const char* lowpassFreq     = "lowpassFreq";
    }

    struct ChoiceLists
    {
        static juce::StringArray keyRoots();
        static juce::StringArray scaleTypes();
        static juce::StringArray modes();
        static juce::StringArray routings();
        static juce::StringArray pitchSources();
        static juce::StringArray trackingSources();
        static juce::StringArray chordSnapModes();
        static juce::StringArray delayDivisions();
        static juce::StringArray intervalStepChoices();
        static juce::StringArray toneStepChoices();
        static juce::StringArray fixedNoteChoices();
    };
}

#include "DSP/KeyAwarePitchDelayDSP.h"

class KeyAwarePitchDelayAudioProcessor final : public juce::AudioProcessor
{
public:
    KeyAwarePitchDelayAudioProcessor();
    ~KeyAwarePitchDelayAudioProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 60.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;

private:
    kapd::KeyAwarePitchDelayDSP dsp;

    // Cached host tempo (updated every block if available)
    std::atomic<double> lastBpm { 120.0 };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyAwarePitchDelayAudioProcessor)
};

