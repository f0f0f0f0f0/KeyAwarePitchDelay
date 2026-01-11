#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
namespace kapd
{
    juce::StringArray ChoiceLists::keyRoots()
    {
        return { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    }

    juce::StringArray ChoiceLists::scaleTypes()
    {
        return {
            "Major",
            "Natural Minor",
            "Harmonic Minor",
            "Melodic Minor (Asc)",
            "Major Pentatonic",
            "Minor Pentatonic",
            "Chromatic",
            "Custom"
        };
    }

    juce::StringArray ChoiceLists::modes()
    {
        return { "Mode 1: Diatonic Interval", "Mode 2: Scale Tone Sequence" };
    }

    juce::StringArray ChoiceLists::routings()
    {
        return { "Serial (feedback)", "Parallel (phrase)" };
    }

    juce::StringArray ChoiceLists::pitchSources()
    {
        return { "Audio", "MIDI", "Fixed" };
    }

    juce::StringArray ChoiceLists::trackingSources()
    {
        return { "Input", "Feedback/Loop" };
    }

    juce::StringArray ChoiceLists::chordSnapModes()
    {
        return { "Override", "Intersect w/ Scale" };
    }

    juce::StringArray ChoiceLists::delayDivisions()
    {
        // Beats are defined in DSP (see division table). This is only labels.
        return {
            "1/1",
            "1/2",
            "1/2 Dotted",
            "1/2 Triplet",
            "1/4",
            "1/4 Dotted",
            "1/4 Triplet",
            "1/8",
            "1/8 Dotted",
            "1/8 Triplet",
            "1/16",
            "1/16 Dotted",
            "1/16 Triplet",
            "1/32",
            "1/32 Dotted",
            "1/32 Triplet"
        };
    }

    juce::StringArray ChoiceLists::intervalStepChoices()
    {
        // 15 options: -7..+7 (scale-degree steps)
        return {
            "-7 (down 8ve)",
            "-6 (down 7th)",
            "-5 (down 6th)",
            "-4 (down 5th)",
            "-3 (down 4th)",
            "-2 (down 3rd)",
            "-1 (down 2nd)",
            "0 (unison)",
            "+1 (up 2nd)",
            "+2 (up 3rd)",
            "+3 (up 4th)",
            "+4 (up 5th)",
            "+5 (up 6th)",
            "+6 (up 7th)",
            "+7 (up 8ve)"
        };
    }

    juce::StringArray ChoiceLists::toneStepChoices()
    {
        return {
            "1 (Root)",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "10",
            "11",
            "12"
        };
    }

    juce::StringArray ChoiceLists::fixedNoteChoices()
    {
        juce::StringArray notes;
        notes.ensureStorageAllocated(128);

        static const juce::StringArray pcs { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

        for (int midi = 0; midi < 128; ++midi)
        {
            int pc = midi % 12;
            int oct = (midi / 12) - 1; // MIDI octave convention: C4 = 60 => oct 4
            notes.add(pcs[pc] + juce::String(oct) + " (" + juce::String(midi) + ")");
        }
        return notes;
    }
}

//==============================================================================
KeyAwarePitchDelayAudioProcessor::KeyAwarePitchDelayAudioProcessor()
    : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
#endif
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
                      ),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

bool KeyAwarePitchDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
#endif
}

void KeyAwarePitchDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    dsp.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void KeyAwarePitchDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear any output channels that don't have input data
    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Host BPM (best-effort)
    double bpm = lastBpm.load();
    if (auto* ph = getPlayHead())
    {
        juce::AudioPlayHead::CurrentPositionInfo info;
        if (ph->getCurrentPosition (info))
        {
            if (info.bpm > 0.0 && std::isfinite(info.bpm))
                bpm = info.bpm;
        }
    }
    lastBpm.store(bpm);

    kapd::KeyAwarePitchDelayDSP::Parameters p;

    p.sampleRate = getSampleRate();
    p.numChannels = buffer.getNumChannels();
    p.blockSize = buffer.getNumSamples();
    p.bpm = bpm;

    p.keyRoot = (int) apvts.getRawParameterValue(kapd::param::keyRoot)->load();
    p.scaleType = (int) apvts.getRawParameterValue(kapd::param::scaleType)->load();

    // Custom scale mask bits (12 toggles)
    {
        uint16_t bits = 0;
        const char* ids[12] = {
            kapd::param::customScale0, kapd::param::customScale1, kapd::param::customScale2, kapd::param::customScale3,
            kapd::param::customScale4, kapd::param::customScale5, kapd::param::customScale6, kapd::param::customScale7,
            kapd::param::customScale8, kapd::param::customScale9, kapd::param::customScale10, kapd::param::customScale11
        };

        for (int i = 0; i < 12; ++i)
        {
            if (apvts.getRawParameterValue(ids[i])->load() > 0.5f)
                bits |= (uint16_t) (1u << (uint16_t) i);
        }

        p.customScaleMaskBits = bits;
    }
    p.mode = apvts.getRawParameterValue(kapd::param::mode)->load() > 0.5f ? 1 : 0;
    p.routing = apvts.getRawParameterValue(kapd::param::routing)->load() > 0.5f ? 1 : 0;
    p.pitchSource = (int) apvts.getRawParameterValue(kapd::param::pitchSource)->load();
    p.trackingSource = apvts.getRawParameterValue(kapd::param::trackingSource)->load() > 0.5f ? 1 : 0;
    p.fixedMidi = (int) apvts.getRawParameterValue(kapd::param::fixedMidi)->load();

    p.tempoSync = apvts.getRawParameterValue(kapd::param::tempoSync)->load() > 0.5f;
    p.delayDivision = (int) apvts.getRawParameterValue(kapd::param::delayDivision)->load();
    p.delayMs = apvts.getRawParameterValue(kapd::param::delayMs)->load();

    p.feedback = apvts.getRawParameterValue(kapd::param::feedback)->load();
    p.mix = apvts.getRawParameterValue(kapd::param::mix)->load();
    p.outputGainDb = apvts.getRawParameterValue(kapd::param::outputGain)->load();
    p.sequenceLength = (int) apvts.getRawParameterValue(kapd::param::sequenceLength)->load();
    p.smoothingMs = apvts.getRawParameterValue(kapd::param::smoothingMs)->load();

    p.snapToChord = apvts.getRawParameterValue(kapd::param::snapToChord)->load() > 0.5f;
    p.chordSnapMode = (int) apvts.getRawParameterValue(kapd::param::chordSnapMode)->load();
    p.advanceOnTransient = apvts.getRawParameterValue(kapd::param::advanceOnTransient)->load() > 0.5f;
    p.transientSensitivity = apvts.getRawParameterValue(kapd::param::transientSensitivity)->load();

    // Steps
    const float intervalIdx[8] = {
        apvts.getRawParameterValue(kapd::param::intervalStep1)->load(),
        apvts.getRawParameterValue(kapd::param::intervalStep2)->load(),
        apvts.getRawParameterValue(kapd::param::intervalStep3)->load(),
        apvts.getRawParameterValue(kapd::param::intervalStep4)->load(),
        apvts.getRawParameterValue(kapd::param::intervalStep5)->load(),
        apvts.getRawParameterValue(kapd::param::intervalStep6)->load(),
        apvts.getRawParameterValue(kapd::param::intervalStep7)->load(),
        apvts.getRawParameterValue(kapd::param::intervalStep8)->load()
    };

    const float toneIdx[8] = {
        apvts.getRawParameterValue(kapd::param::toneStep1)->load(),
        apvts.getRawParameterValue(kapd::param::toneStep2)->load(),
        apvts.getRawParameterValue(kapd::param::toneStep3)->load(),
        apvts.getRawParameterValue(kapd::param::toneStep4)->load(),
        apvts.getRawParameterValue(kapd::param::toneStep5)->load(),
        apvts.getRawParameterValue(kapd::param::toneStep6)->load(),
        apvts.getRawParameterValue(kapd::param::toneStep7)->load(),
        apvts.getRawParameterValue(kapd::param::toneStep8)->load()
    };

    for (int i = 0; i < 8; ++i)
    {
        p.intervalStepIndex[i] = (int) intervalIdx[i];
        p.toneStepIndex[i] = (int) toneIdx[i];
    }

    // Per-step level/pan
    const float lvl[8] = {
        apvts.getRawParameterValue(kapd::param::stepLevel1)->load(),
        apvts.getRawParameterValue(kapd::param::stepLevel2)->load(),
        apvts.getRawParameterValue(kapd::param::stepLevel3)->load(),
        apvts.getRawParameterValue(kapd::param::stepLevel4)->load(),
        apvts.getRawParameterValue(kapd::param::stepLevel5)->load(),
        apvts.getRawParameterValue(kapd::param::stepLevel6)->load(),
        apvts.getRawParameterValue(kapd::param::stepLevel7)->load(),
        apvts.getRawParameterValue(kapd::param::stepLevel8)->load()
    };

    const float pan[8] = {
        apvts.getRawParameterValue(kapd::param::stepPan1)->load(),
        apvts.getRawParameterValue(kapd::param::stepPan2)->load(),
        apvts.getRawParameterValue(kapd::param::stepPan3)->load(),
        apvts.getRawParameterValue(kapd::param::stepPan4)->load(),
        apvts.getRawParameterValue(kapd::param::stepPan5)->load(),
        apvts.getRawParameterValue(kapd::param::stepPan6)->load(),
        apvts.getRawParameterValue(kapd::param::stepPan7)->load(),
        apvts.getRawParameterValue(kapd::param::stepPan8)->load()
    };

    for (int i = 0; i < 8; ++i)
    {
        p.stepLevel[i] = lvl[i];
        p.stepPan[i] = pan[i];
    }

    // Post-FX chain parameters
    p.saturationDrive = apvts.getRawParameterValue(kapd::param::saturationDrive)->load();
    p.saturationMix = apvts.getRawParameterValue(kapd::param::saturationMix)->load();
    p.diffusionAmount = apvts.getRawParameterValue(kapd::param::diffusionAmount)->load();
    p.diffusionMix = apvts.getRawParameterValue(kapd::param::diffusionMix)->load();
    p.lofiAmount = apvts.getRawParameterValue(kapd::param::lofiAmount)->load();
    p.lofiMix = apvts.getRawParameterValue(kapd::param::lofiMix)->load();
    p.reverbDecay = apvts.getRawParameterValue(kapd::param::reverbDecay)->load();
    p.reverbDamping = apvts.getRawParameterValue(kapd::param::reverbDamping)->load();
    p.reverbMix = apvts.getRawParameterValue(kapd::param::reverbMix)->load();
    p.highpassFreq = apvts.getRawParameterValue(kapd::param::highpassFreq)->load();
    p.lowpassFreq = apvts.getRawParameterValue(kapd::param::lowpassFreq)->load();

    dsp.process(buffer, midiMessages, p);
}

//==============================================================================
void KeyAwarePitchDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void KeyAwarePitchDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout KeyAwarePitchDelayAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using API = juce::AudioParameterInt;
    using APB = juce::AudioParameterBool;
    using APC = juce::AudioParameterChoice;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<APC> (kapd::param::keyRoot, "Key Root", kapd::ChoiceLists::keyRoots(), 0));
    layout.add (std::make_unique<APC> (kapd::param::scaleType, "Scale", kapd::ChoiceLists::scaleTypes(), 0));

    // Custom scale editor (relative semitone offsets from the Key Root)
    // Defaults to Major: 1,2,3,4,5,6,7 = {0,2,4,5,7,9,11}
    layout.add (std::make_unique<APB> (kapd::param::customScale0,  "Custom: 1",  true));
    layout.add (std::make_unique<APB> (kapd::param::customScale1,  "Custom: b2", false));
    layout.add (std::make_unique<APB> (kapd::param::customScale2,  "Custom: 2",  true));
    layout.add (std::make_unique<APB> (kapd::param::customScale3,  "Custom: b3", false));
    layout.add (std::make_unique<APB> (kapd::param::customScale4,  "Custom: 3",  true));
    layout.add (std::make_unique<APB> (kapd::param::customScale5,  "Custom: 4",  true));
    layout.add (std::make_unique<APB> (kapd::param::customScale6,  "Custom: #4", false));
    layout.add (std::make_unique<APB> (kapd::param::customScale7,  "Custom: 5",  true));
    layout.add (std::make_unique<APB> (kapd::param::customScale8,  "Custom: b6", false));
    layout.add (std::make_unique<APB> (kapd::param::customScale9,  "Custom: 6",  true));
    layout.add (std::make_unique<APB> (kapd::param::customScale10, "Custom: b7", false));
    layout.add (std::make_unique<APB> (kapd::param::customScale11, "Custom: 7",  true));

    // Mode: false = Diatonic Interval, true = Scale Tone Sequence
    layout.add (std::make_unique<APB> (kapd::param::mode, "Mode", false));
    // Routing: false = Serial (feedback), true = Parallel (phrase)
    layout.add (std::make_unique<APB> (kapd::param::routing, "Routing", false));

    layout.add (std::make_unique<APC> (kapd::param::pitchSource, "Pitch Source", kapd::ChoiceLists::pitchSources(), 0));
    // Tracking: false = Input, true = Feedback/Loop
    layout.add (std::make_unique<APB> (kapd::param::trackingSource, "Tracking Source", false));
    layout.add (std::make_unique<APC> (kapd::param::fixedMidi, "Fixed Note", kapd::ChoiceLists::fixedNoteChoices(), 60));

    layout.add (std::make_unique<APB> (kapd::param::snapToChord, "Snap to Chord (MIDI)", false));
    layout.add (std::make_unique<APC> (kapd::param::chordSnapMode, "Chord Snap Mode", kapd::ChoiceLists::chordSnapModes(), 0 /* Override (v2 behaviour) */));
    layout.add (std::make_unique<APB> (kapd::param::advanceOnTransient, "Advance on Transient", false));
    layout.add (std::make_unique<APF> (kapd::param::transientSensitivity, "Transient Sensitivity",
                                       juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.6f));

    layout.add (std::make_unique<APB> (kapd::param::tempoSync, "Tempo Sync", true));
    layout.add (std::make_unique<APC> (kapd::param::delayDivision, "Delay Division", kapd::ChoiceLists::delayDivisions(), 4 /* 1/4 */));
    layout.add (std::make_unique<APF> (kapd::param::delayMs, "Delay (ms)", juce::NormalisableRange<float> (1.0f, 2000.0f, 0.01f), 400.0f));

    layout.add (std::make_unique<APF> (kapd::param::feedback, "Feedback", juce::NormalisableRange<float> (0.0f, 0.95f, 0.0001f), 0.35f));
    layout.add (std::make_unique<APF> (kapd::param::mix, "Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f), 0.5f));
    layout.add (std::make_unique<APF> (kapd::param::outputGain, "Output Gain (dB)", juce::NormalisableRange<float> (-24.0f, 6.0f, 0.01f), 0.0f));

    layout.add (std::make_unique<API> (kapd::param::sequenceLength, "Sequence Length", 1, 8, 4));
    layout.add (std::make_unique<APF> (kapd::param::smoothingMs, "Ratio Smoothing (ms)", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 15.0f));

    // Interval steps (Mode 1) - range 0-14 maps to -7..+7 scale degrees
    layout.add (std::make_unique<API> (kapd::param::intervalStep1, "Interval Step 1", 0, 14, 9)); // +2 (3rd up)
    layout.add (std::make_unique<API> (kapd::param::intervalStep2, "Interval Step 2", 0, 14, 9));
    layout.add (std::make_unique<API> (kapd::param::intervalStep3, "Interval Step 3", 0, 14, 9));
    layout.add (std::make_unique<API> (kapd::param::intervalStep4, "Interval Step 4", 0, 14, 9));
    layout.add (std::make_unique<API> (kapd::param::intervalStep5, "Interval Step 5", 0, 14, 7)); // 0 unison
    layout.add (std::make_unique<API> (kapd::param::intervalStep6, "Interval Step 6", 0, 14, 7));
    layout.add (std::make_unique<API> (kapd::param::intervalStep7, "Interval Step 7", 0, 14, 7));
    layout.add (std::make_unique<API> (kapd::param::intervalStep8, "Interval Step 8", 0, 14, 7));

    // Tone steps (Mode 2) - range 0-11 maps to scale degrees 1-12
    layout.add (std::make_unique<API> (kapd::param::toneStep1, "Tone Step 1", 0, 11, 0)); // 1
    layout.add (std::make_unique<API> (kapd::param::toneStep2, "Tone Step 2", 0, 11, 4)); // 5
    layout.add (std::make_unique<API> (kapd::param::toneStep3, "Tone Step 3", 0, 11, 2)); // 3
    layout.add (std::make_unique<API> (kapd::param::toneStep4, "Tone Step 4", 0, 11, 6)); // 7
    layout.add (std::make_unique<API> (kapd::param::toneStep5, "Tone Step 5", 0, 11, 0));
    layout.add (std::make_unique<API> (kapd::param::toneStep6, "Tone Step 6", 0, 11, 4));
    layout.add (std::make_unique<API> (kapd::param::toneStep7, "Tone Step 7", 0, 11, 2));
    layout.add (std::make_unique<API> (kapd::param::toneStep8, "Tone Step 8", 0, 11, 6));

    // Per-step level & pan (applies in both routings)
    const juce::NormalisableRange<float> levelRange (0.0f, 1.0f, 0.0001f);
    const juce::NormalisableRange<float> panRange (-1.0f, 1.0f, 0.0001f);

    layout.add (std::make_unique<APF> (kapd::param::stepLevel1, "Step 1 Level", levelRange, 1.0f));
    layout.add (std::make_unique<APF> (kapd::param::stepLevel2, "Step 2 Level", levelRange, 1.0f));
    layout.add (std::make_unique<APF> (kapd::param::stepLevel3, "Step 3 Level", levelRange, 1.0f));
    layout.add (std::make_unique<APF> (kapd::param::stepLevel4, "Step 4 Level", levelRange, 1.0f));
    layout.add (std::make_unique<APF> (kapd::param::stepLevel5, "Step 5 Level", levelRange, 1.0f));
    layout.add (std::make_unique<APF> (kapd::param::stepLevel6, "Step 6 Level", levelRange, 1.0f));
    layout.add (std::make_unique<APF> (kapd::param::stepLevel7, "Step 7 Level", levelRange, 1.0f));
    layout.add (std::make_unique<APF> (kapd::param::stepLevel8, "Step 8 Level", levelRange, 1.0f));

    layout.add (std::make_unique<APF> (kapd::param::stepPan1, "Step 1 Pan", panRange, 0.0f));
    layout.add (std::make_unique<APF> (kapd::param::stepPan2, "Step 2 Pan", panRange, 0.0f));
    layout.add (std::make_unique<APF> (kapd::param::stepPan3, "Step 3 Pan", panRange, 0.0f));
    layout.add (std::make_unique<APF> (kapd::param::stepPan4, "Step 4 Pan", panRange, 0.0f));
    layout.add (std::make_unique<APF> (kapd::param::stepPan5, "Step 5 Pan", panRange, 0.0f));
    layout.add (std::make_unique<APF> (kapd::param::stepPan6, "Step 6 Pan", panRange, 0.0f));
    layout.add (std::make_unique<APF> (kapd::param::stepPan7, "Step 7 Pan", panRange, 0.0f));
    layout.add (std::make_unique<APF> (kapd::param::stepPan8, "Step 8 Pan", panRange, 0.0f));

    // === POST-FX CHAIN (Mood/Form2 inspired) ===
    const juce::NormalisableRange<float> zeroOneRange (0.0f, 1.0f, 0.001f);

    // Saturation (warm tube-style)
    layout.add (std::make_unique<APF> (kapd::param::saturationDrive, "Saturation Drive", zeroOneRange, 0.0f));
    layout.add (std::make_unique<APF> (kapd::param::saturationMix, "Saturation Mix", zeroOneRange, 0.5f));

    // Diffusion (smear/blur delays)
    layout.add (std::make_unique<APF> (kapd::param::diffusionAmount, "Diffusion Amount", zeroOneRange, 0.0f));
    layout.add (std::make_unique<APF> (kapd::param::diffusionMix, "Diffusion Mix", zeroOneRange, 0.5f));

    // Lo-Fi (bit crush, sample rate reduction, wow/flutter)
    layout.add (std::make_unique<APF> (kapd::param::lofiAmount, "Lo-Fi Amount", zeroOneRange, 0.0f));
    layout.add (std::make_unique<APF> (kapd::param::lofiMix, "Lo-Fi Mix", zeroOneRange, 0.5f));

    // Room reverb
    layout.add (std::make_unique<APF> (kapd::param::reverbDecay, "Reverb Decay", zeroOneRange, 0.5f));
    layout.add (std::make_unique<APF> (kapd::param::reverbDamping, "Reverb Damping", zeroOneRange, 0.5f));
    layout.add (std::make_unique<APF> (kapd::param::reverbMix, "Reverb Mix", zeroOneRange, 0.0f));

    // Filters (HP/LP)
    layout.add (std::make_unique<APF> (kapd::param::highpassFreq, "Highpass Freq",
                                       juce::NormalisableRange<float> (20.0f, 2000.0f, 1.0f, 0.3f), 20.0f));
    layout.add (std::make_unique<APF> (kapd::param::lowpassFreq, "Lowpass Freq",
                                       juce::NormalisableRange<float> (200.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));

    return layout;
}

//==============================================================================
juce::AudioProcessorEditor* KeyAwarePitchDelayAudioProcessor::createEditor()
{
    return new KeyAwarePitchDelayAudioProcessorEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KeyAwarePitchDelayAudioProcessor();
}
