#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>

namespace kapd
{
    //==============================
    // Utility
    inline float semitonesToRatio(float semitones)
    {
        return std::pow(2.0f, semitones / 12.0f);
    }

    //==============================
    // Scale + diatonic mapping helpers
    class ScaleTable
    {
    public:
        enum ScaleType
        {
            Major = 0,
            NaturalMinor,
            HarmonicMinor,
            MelodicMinor,
            MajorPent,
            MinorPent,
            Chromatic,
            Custom
        };

        void set(int newRootPc, int newScaleType, uint16_t newCustomMaskBits = 0)
        {
            rootPc = juce::jlimit(0, 11, newRootPc);
            scaleType = juce::jlimit(0, (int) Custom, newScaleType);
            customMaskBits = newCustomMaskBits;
            rebuild();
        }

        int getDegreeCount() const { return (int) scaleOffsets.size(); }

        const std::array<bool, 12>& getPitchClasses() const { return allowedPitchClasses; }

        // Quantize a MIDI note to the nearest scale tone
        int quantizeMidi(int midiNote) const
        {
            midiNote = juce::jlimit(0, 127, midiNote);
            if (scaleMidi.empty())
                return midiNote;

            auto it = std::lower_bound(scaleMidi.begin(), scaleMidi.end(), midiNote);
            if (it == scaleMidi.begin())
                return *it;
            if (it == scaleMidi.end())
                return scaleMidi.back();

            auto above = *it;
            auto below = *(it - 1);

            if (std::abs(above - midiNote) < std::abs(midiNote - below))
                return above;

            return below;
        }

        // Shift by scale degrees (e.g., +2 = up a third in the scale)
        // Returns the SEMITONE difference (not the new MIDI note)
        int shiftByDegreesGetSemitones(int midiNote, int degreeShift) const
        {
            if (scaleMidi.empty() || degreeShift == 0)
                return 0;

            midiNote = quantizeMidi(midiNote);
            auto it = std::lower_bound(scaleMidi.begin(), scaleMidi.end(), midiNote);
            if (it == scaleMidi.end())
                return 0;

            int idx = (int) std::distance(scaleMidi.begin(), it);
            int newIdx = juce::jlimit(0, (int) scaleMidi.size() - 1, idx + degreeShift);
            int targetMidi = scaleMidi[(size_t) newIdx];

            return targetMidi - midiNote;
        }

        // Get the MIDI note for a specific scale degree (1-based: 1=root, 2=2nd, etc.)
        // Returns the semitone difference from refMidi
        int getDegreeTargetSemitones(int degreeIndex, int refMidi) const
        {
            refMidi = juce::jlimit(0, 127, refMidi);

            if (scaleOffsets.empty())
                return 0;

            int degCount = (int) scaleOffsets.size();
            int di = juce::jlimit(1, degCount, degreeIndex) - 1;

            // Target pitch class
            int targetPc = (rootPc + scaleOffsets[(size_t) di]) % 12;
            int refPc = refMidi % 12;
            int refOctave = refMidi / 12;

            // Find the target note closest to refMidi
            int candidate = refOctave * 12 + targetPc;
            int best = candidate;
            int bestDist = std::abs(best - refMidi);

            for (int k : { -1, 1 })
            {
                int c = (refOctave + k) * 12 + targetPc;
                c = juce::jlimit(0, 127, c);
                int dist = std::abs(c - refMidi);
                if (dist < bestDist)
                {
                    bestDist = dist;
                    best = c;
                }
            }

            return best - refMidi;
        }

    private:
        int rootPc = 0;
        int scaleType = 0;
        uint16_t customMaskBits = 0;
        std::vector<int> scaleOffsets;
        std::vector<int> scaleMidi;
        std::array<bool, 12> allowedPitchClasses {};

        void rebuild()
        {
            scaleOffsets.clear();

            switch (scaleType)
            {
                case Major:        scaleOffsets = { 0,2,4,5,7,9,11 }; break;
                case NaturalMinor: scaleOffsets = { 0,2,3,5,7,8,10 }; break;
                case HarmonicMinor:scaleOffsets = { 0,2,3,5,7,8,11 }; break;
                case MelodicMinor: scaleOffsets = { 0,2,3,5,7,9,11 }; break;
                case MajorPent:    scaleOffsets = { 0,2,4,7,9 }; break;
                case MinorPent:    scaleOffsets = { 0,3,5,7,10 }; break;
                case Chromatic:    scaleOffsets = { 0,1,2,3,4,5,6,7,8,9,10,11 }; break;
                case Custom:
                {
                    uint16_t bits = (uint16_t) (customMaskBits | 0x0001u);
                    for (int i = 0; i < 12; ++i)
                        if ((bits & (uint16_t) (1u << (uint16_t) i)) != 0)
                            scaleOffsets.push_back(i);

                    if (scaleOffsets.empty())
                        scaleOffsets = { 0 };
                    break;
                }
                default: scaleOffsets = { 0,2,4,5,7,9,11 }; break;
            }

            allowedPitchClasses.fill(false);
            for (auto s : scaleOffsets)
                allowedPitchClasses[(size_t) ((rootPc + (s % 12) + 12) % 12)] = true;

            scaleMidi.clear();
            scaleMidi.reserve(128);

            for (int m = 0; m < 128; ++m)
            {
                int pc = (m % 12);
                if (allowedPitchClasses[(size_t) pc])
                    scaleMidi.push_back(m);
            }
        }
    };

    //==============================
    // Pitched Delay Tap - combines delay reading with varispeed pitch shifting
    // This is the core "tape-style" pitched delay: read from buffer at variable rate
    class PitchedDelayTap
    {
    public:
        void prepare(float sr)
        {
            sampleRate = sr;
            readPos = 0.0f;
            currentRatio = 1.0f;
            targetRatio = 1.0f;
            smoothedRatio = 1.0f;
            active = false;
            fadeGain = 0.0f;
        }

        void reset()
        {
            readPos = 0.0f;
            currentRatio = 1.0f;
            targetRatio = 1.0f;
            smoothedRatio = 1.0f;
            active = false;
            fadeGain = 0.0f;
        }

        void setRatio(float ratio)
        {
            targetRatio = juce::jlimit(0.25f, 4.0f, ratio);
        }

        // Start a new tap from a specific position in the delay buffer
        void trigger(float startPos)
        {
            readPos = startPos;
            smoothedRatio = targetRatio;
            currentRatio = targetRatio;
            active = true;
            fadeGain = 0.0f; // will fade in
        }

        // Read from an external buffer with varispeed playback
        // Returns the pitched sample, handles its own fade in/out
        float process(const std::vector<float>& buffer, int writePos)
        {
            if (!active)
                return 0.0f;

            int bufSize = (int) buffer.size();
            if (bufSize == 0)
                return 0.0f;

            // Smooth ratio changes
            smoothedRatio += (targetRatio - smoothedRatio) * 0.001f;
            currentRatio = smoothedRatio;

            // Fade in smoothly
            if (fadeGain < 1.0f)
                fadeGain = std::min(1.0f, fadeGain + 0.0005f);

            // Read with cubic interpolation
            float output = readCubic(buffer, readPos);

            // Advance read position at variable rate
            readPos += currentRatio;

            // Wrap read position
            while (readPos >= (float) bufSize) readPos -= (float) bufSize;
            while (readPos < 0.0f) readPos += (float) bufSize;

            // Check if we're catching up to write position (running out of audio)
            float dist = (float) writePos - readPos;
            if (dist < 0) dist += (float) bufSize;

            // If too close to write head, fade out and deactivate
            if (dist < 256.0f || dist > (float) bufSize - 256.0f)
            {
                fadeGain *= 0.99f;
                if (fadeGain < 0.001f)
                    active = false;
            }

            return output * fadeGain;
        }

        bool isActive() const { return active; }
        float getReadPos() const { return readPos; }

    private:
        float sampleRate = 44100.0f;
        float readPos = 0.0f;
        float currentRatio = 1.0f;
        float targetRatio = 1.0f;
        float smoothedRatio = 1.0f;
        bool active = false;
        float fadeGain = 0.0f;

        float readCubic(const std::vector<float>& buf, float pos) const
        {
            int bufSize = (int) buf.size();
            while (pos >= (float) bufSize) pos -= (float) bufSize;
            while (pos < 0.0f) pos += (float) bufSize;

            int i1 = (int) pos;
            int i0 = (i1 - 1 + bufSize) % bufSize;
            int i2 = (i1 + 1) % bufSize;
            int i3 = (i1 + 2) % bufSize;

            float frac = pos - (float) i1;

            float y0 = buf[(size_t) i0];
            float y1 = buf[(size_t) i1];
            float y2 = buf[(size_t) i2];
            float y3 = buf[(size_t) i3];

            // Catmull-Rom spline
            float c0 = y1;
            float c1 = 0.5f * (y2 - y0);
            float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
            float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

            return ((c3 * frac + c2) * frac + c1) * frac + c0;
        }
    };

    //==============================
    // Main DSP class
    class KeyAwarePitchDelayDSP
    {
    public:
        struct Parameters
        {
            double sampleRate = 44100.0;
            int numChannels = 2;
            int blockSize = 512;
            double bpm = 120.0;

            int keyRoot = 0;        // 0=C, 1=C#, ... 11=B
            int scaleType = 0;      // ScaleTable::ScaleType
            uint16_t customScaleMaskBits = 0;

            int mode = 0;           // 0 = interval (degree shift), 1 = tone sequence (target degree)
            int routing = 0;        // 0 = serial, 1 = parallel
            int pitchSource = 0;    // 0 = fixed at root, 1 = midi, 2 = fixed note
            int trackingSource = 0; // unused for now
            int fixedMidi = 60;

            bool snapToChord = false;
            int chordSnapMode = 0;
            bool advanceOnTransient = false;
            float transientSensitivity = 0.6f;

            bool tempoSync = true;
            int delayDivision = 4;
            float delayMs = 400.0f;

            float feedback = 0.35f;
            float mix = 0.5f;
            float outputGainDb = 0.0f;

            int sequenceLength = 4;
            float smoothingMs = 15.0f;

            // Interval mode: index 0-14 maps to -7 to +7 scale degrees
            int intervalStepIndex[8] = { 7,7,7,7,7,7,7,7 };
            // Tone mode: index 0-11 maps to scale degrees 1-12 (with octave wrapping)
            int toneStepIndex[8] = { 0,4,2,6,0,4,2,6 };

            float stepLevel[8] = { 1,1,1,1,1,1,1,1 };
            float stepPan[8]   = { 0,0,0,0,0,0,0,0 };
        };

        void prepare(double newSampleRate, int maxBlockSize, int numChannels)
        {
            sampleRate = (float) newSampleRate;
            channels = std::max(1, numChannels);

            // Allocate delay buffers - 10 seconds max
            int bufSize = (int) (10.0 * sampleRate);
            delayBufferL.assign((size_t) bufSize, 0.0f);
            delayBufferR.assign((size_t) bufSize, 0.0f);
            writePos = 0;

            // Prepare taps
            for (int i = 0; i < kMaxSteps; ++i)
            {
                tapsL[i].prepare(sampleRate);
                tapsR[i].prepare(sampleRate);
            }

            scale.set(0, 0, 0);
            reset();
        }

        void reset()
        {
            std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
            std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
            writePos = 0;

            for (int i = 0; i < kMaxSteps; ++i)
            {
                tapsL[i].reset();
                tapsR[i].reset();
            }

            sequencePhase = 0;
            samplesToNextTrigger = 0;
            lastMidiNote = 60;
        }

        void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, const Parameters& p)
        {
            const int numSamples = buffer.getNumSamples();
            const int numCh = buffer.getNumChannels();
            const int bufSize = (int) delayBufferL.size();

            // Update scale if needed
            if (p.keyRoot != cachedRoot || p.scaleType != cachedScaleType || p.customScaleMaskBits != cachedCustomMask)
            {
                scale.set(p.keyRoot, p.scaleType, p.customScaleMaskBits);
                cachedRoot = p.keyRoot;
                cachedScaleType = p.scaleType;
                cachedCustomMask = p.customScaleMaskBits;
            }

            // MIDI tracking for pitch source
            for (const auto meta : midi)
            {
                const auto msg = meta.getMessage();
                if (msg.isNoteOn())
                    lastMidiNote = msg.getNoteNumber();
            }

            // Reference MIDI note for pitch calculations
            int refMidi = 60;
            if (p.pitchSource == 0) // Fixed at scale root
                refMidi = 60 + p.keyRoot;
            else if (p.pitchSource == 1) // MIDI
                refMidi = lastMidiNote;
            else if (p.pitchSource == 2) // Fixed note
                refMidi = p.fixedMidi;

            // Calculate delay time in samples
            int delaySamples = computeDelaySamples(p);
            int seqLen = juce::jlimit(1, kMaxSteps, p.sequenceLength);

            // Calculate pitch ratios for each step
            std::array<float, kMaxSteps> stepRatios;
            for (int i = 0; i < seqLen; ++i)
            {
                int semitones = computeSemitonesForStep(p, refMidi, i);
                stepRatios[i] = semitonesToRatio((float) semitones);
            }

            // Update tap ratios
            for (int i = 0; i < seqLen; ++i)
            {
                tapsL[i].setRatio(stepRatios[i]);
                tapsR[i].setRatio(stepRatios[i]);
            }

            // Process audio
            const float mix = juce::jlimit(0.0f, 1.0f, p.mix);
            const float fb = juce::jlimit(0.0f, 0.95f, p.feedback);
            const float gain = juce::Decibels::decibelsToGain(p.outputGainDb);

            if (samplesToNextTrigger <= 0)
                samplesToNextTrigger = delaySamples;

            for (int s = 0; s < numSamples; ++s)
            {
                // Get input
                float inL = buffer.getSample(0, s);
                float inR = (numCh > 1) ? buffer.getSample(1, s) : inL;

                // Check if it's time to trigger next tap
                samplesToNextTrigger--;
                if (samplesToNextTrigger <= 0)
                {
                    samplesToNextTrigger = delaySamples;

                    // Trigger the current step's tap
                    int stepIdx = sequencePhase % seqLen;

                    // Calculate read start position (where the delayed audio begins)
                    float startPos = (float) writePos - (float) delaySamples;
                    if (startPos < 0) startPos += (float) bufSize;

                    tapsL[stepIdx].trigger(startPos);
                    tapsR[stepIdx].trigger(startPos);

                    // Advance sequence
                    sequencePhase = (sequencePhase + 1) % seqLen;
                }

                // Sum output from all active taps
                float wetL = 0.0f;
                float wetR = 0.0f;

                for (int i = 0; i < seqLen; ++i)
                {
                    float tapL = tapsL[i].process(delayBufferL, writePos);
                    float tapR = tapsR[i].process(delayBufferR, writePos);

                    float level = p.stepLevel[i];
                    float pan = p.stepPan[i];
                    float panL = std::cos((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
                    float panR = std::sin((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);

                    wetL += tapL * level * panL;
                    wetR += tapR * level * panR;
                }

                // Write to delay buffer (input + feedback)
                delayBufferL[(size_t) writePos] = inL + wetL * fb;
                delayBufferR[(size_t) writePos] = inR + wetR * fb;

                writePos = (writePos + 1) % bufSize;

                // Mix and output
                float outL = inL * (1.0f - mix) + wetL * mix;
                float outR = inR * (1.0f - mix) + wetR * mix;

                buffer.setSample(0, s, outL * gain);
                if (numCh > 1)
                    buffer.setSample(1, s, outR * gain);
            }
        }

    private:
        static constexpr int kMaxSteps = 8;

        float sampleRate = 44100.0f;
        int channels = 2;

        std::vector<float> delayBufferL;
        std::vector<float> delayBufferR;
        int writePos = 0;

        PitchedDelayTap tapsL[kMaxSteps];
        PitchedDelayTap tapsR[kMaxSteps];

        ScaleTable scale;
        int cachedRoot = -1;
        int cachedScaleType = -1;
        uint16_t cachedCustomMask = 0;

        int sequencePhase = 0;
        int samplesToNextTrigger = 0;
        int lastMidiNote = 60;

        int computeDelaySamples(const Parameters& p) const
        {
            double bpm = juce::jlimit(20.0, 300.0, p.bpm);
            double seconds = 0.0;

            if (p.tempoSync)
            {
                static const std::array<double, 16> beats = {
                    4.0, 2.0, 3.0, 4.0/3.0,      // 1/1, 1/2, 1/2d, 1/2t
                    1.0, 1.5, 2.0/3.0,           // 1/4, 1/4d, 1/4t
                    0.5, 0.75, 1.0/3.0,          // 1/8, 1/8d, 1/8t
                    0.25, 0.375, 1.0/6.0,        // 1/16, 1/16d, 1/16t
                    0.125, 0.1875, 1.0/12.0      // 1/32, 1/32d, 1/32t
                };

                int idx = juce::jlimit(0, 15, p.delayDivision);
                seconds = (60.0 / bpm) * beats[(size_t) idx];
            }
            else
            {
                seconds = juce::jlimit(0.001, 5.0, (double) p.delayMs / 1000.0);
            }

            return juce::jlimit(1, (int) delayBufferL.size() - 1, (int) std::round(seconds * sampleRate));
        }

        int computeSemitonesForStep(const Parameters& p, int refMidi, int stepIdx) const
        {
            stepIdx = juce::jlimit(0, kMaxSteps - 1, stepIdx);

            if (p.mode == 0) // Interval mode - shift by scale degrees
            {
                // intervalStepIndex: 0-14 maps to -7 to +7 degrees
                int degreeShift = p.intervalStepIndex[stepIdx] - 7;
                return scale.shiftByDegreesGetSemitones(refMidi, degreeShift);
            }
            else // Tone mode - target specific scale degree
            {
                // toneStepIndex: 0-11 maps to degrees 1-12 (octave wrap)
                int degree = (p.toneStepIndex[stepIdx] % scale.getDegreeCount()) + 1;
                return scale.getDegreeTargetSemitones(degree, refMidi);
            }
        }
    };
}
