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
    inline float midiToHz(float midiNote)
    {
        return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
    }

    inline float hzToMidi(float hz)
    {
        if (hz <= 0.0f) return 0.0f;
        return 69.0f + 12.0f * std::log2(hz / 440.0f);
    }

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

        int shiftByDegrees(int midiNote, int degreeShift) const
        {
            if (scaleMidi.empty())
                return midiNote;

            midiNote = quantizeMidi(midiNote);
            auto it = std::lower_bound(scaleMidi.begin(), scaleMidi.end(), midiNote);
            if (it == scaleMidi.end())
                return midiNote;

            int idx = (int) std::distance(scaleMidi.begin(), it);
            int newIdx = juce::jlimit(0, (int) scaleMidi.size() - 1, idx + degreeShift);
            return scaleMidi[(size_t) newIdx];
        }

        // degreeIndex is 1-based (1..7 for diatonic scales)
        int nearestMidiForDegree(int degreeIndex, int nearMidi) const
        {
            nearMidi = juce::jlimit(0, 127, nearMidi);

            if (scaleOffsets.empty())
                return nearMidi;

            int degCount = (int) scaleOffsets.size();
            int di = juce::jlimit(1, degCount, degreeIndex) - 1;

            int targetPc = (rootPc + scaleOffsets[(size_t) di]) % 12;

            int baseOct = nearMidi / 12;
            int candidate = baseOct * 12 + targetPc;

            // choose the closest candidate across octaves
            int best = candidate;
            int bestDist = std::abs(best - nearMidi);

            for (int k : { -1, 1 })
            {
                int c = (baseOct + k) * 12 + targetPc;
                c = juce::jlimit(0, 127, c);
                int dist = std::abs(c - nearMidi);
                if (dist < bestDist)
                {
                    bestDist = dist;
                    best = c;
                }
            }

            return juce::jlimit(0, 127, best);
        }

    private:
        int rootPc = 0;
        int scaleType = 0;
        uint16_t customMaskBits = 0;
        std::vector<int> scaleOffsets; // semitones in octave
        std::vector<int> scaleMidi;    // all scale notes in MIDI 0..127

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
                    // mask bits are semitone offsets relative to root. Always include the root.
                    uint16_t bits = (uint16_t) (customMaskBits | 0x0001u);
                    for (int i = 0; i < 12; ++i)
                        if ((bits & (uint16_t) (1u << (uint16_t) i)) != 0)
                            scaleOffsets.push_back(i);

                    if (scaleOffsets.empty())
                        scaleOffsets = { 0 };
                    break;
                }
                default:           scaleOffsets = { 0,2,4,5,7,9,11 }; break;
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
    // Lightweight YIN pitch detector (monophonic, best-effort)
    class YinPitchDetector
    {
    public:
        void prepare(double newSampleRate, int windowSize = 1024, int hopSize = 256)
        {
            sampleRate = (float) newSampleRate;
            N = std::max(128, windowSize);
            hop = std::max(32, hopSize);

            ring.assign((size_t) N, 0.0f);
            ringWrite = 0;
            hopCounter = 0;

            tauMax = N / 2;
            diff.assign((size_t) (tauMax + 1), 0.0f);
            cmnd.assign((size_t) (tauMax + 1), 0.0f);

            lastPitchHz = 0.0f;
            lastConfidence = 0.0f;
        }

        void reset()
        {
            std::fill(ring.begin(), ring.end(), 0.0f);
            ringWrite = 0;
            hopCounter = 0;
            lastPitchHz = 0.0f;
            lastConfidence = 0.0f;
        }

        void processBlock(const float* mono, int numSamples)
        {
            if (mono == nullptr || numSamples <= 0) return;

            for (int i = 0; i < numSamples; ++i)
            {
                ring[(size_t) ringWrite] = mono[i];
                ringWrite = (ringWrite + 1) % N;

                if (++hopCounter >= hop)
                {
                    hopCounter = 0;
                    compute();
                }
            }
        }

        float getPitchHz() const { return lastPitchHz; }
        float getConfidence() const { return lastConfidence; }

    private:
        float sampleRate = 44100.0f;
        int N = 1024;
        int hop = 256;
        int tauMax = 512;

        std::vector<float> ring;
        int ringWrite = 0;
        int hopCounter = 0;

        std::vector<float> diff;
        std::vector<float> cmnd;

        float lastPitchHz = 0.0f;
        float lastConfidence = 0.0f;

        void compute()
        {
            // Copy ring to contiguous buffer with most recent sample at end.
            // We want x[0]..x[N-1] in time order.
            std::vector<float> x((size_t) N);
            for (int i = 0; i < N; ++i)
            {
                int idx = ringWrite + i;
                if (idx >= N) idx -= N;
                x[(size_t) i] = ring[(size_t) idx];
            }

            // Difference function
            for (int tau = 1; tau <= tauMax; ++tau)
            {
                float sum = 0.0f;
                for (int i = 0; i < N - tau; ++i)
                {
                    float d = x[(size_t) i] - x[(size_t) (i + tau)];
                    sum += d * d;
                }
                diff[(size_t) tau] = sum;
            }

            // Cumulative mean normalized difference (CMND)
            float running = 0.0f;
            cmnd[0] = 1.0f;
            for (int tau = 1; tau <= tauMax; ++tau)
            {
                running += diff[(size_t) tau];
                if (running > 0.0f)
                    cmnd[(size_t) tau] = diff[(size_t) tau] * (float) tau / running;
                else
                    cmnd[(size_t) tau] = 1.0f;
            }

            constexpr float threshold = 0.12f;

            int tauEstimate = 0;
            for (int tau = 2; tau <= tauMax; ++tau)
            {
                if (cmnd[(size_t) tau] < threshold)
                {
                    // pick local minimum
                    while (tau + 1 <= tauMax && cmnd[(size_t) (tau + 1)] < cmnd[(size_t) tau])
                        ++tau;

                    tauEstimate = tau;
                    break;
                }
            }

            if (tauEstimate == 0)
            {
                lastPitchHz = 0.0f;
                lastConfidence = 0.0f;
                return;
            }

            // Parabolic interpolation around tauEstimate
            float betterTau = (float) tauEstimate;
            if (tauEstimate > 1 && tauEstimate < tauMax)
            {
                float s0 = cmnd[(size_t) (tauEstimate - 1)];
                float s1 = cmnd[(size_t) tauEstimate];
                float s2 = cmnd[(size_t) (tauEstimate + 1)];

                float denom = 2.0f * (2.0f * s1 - s2 - s0);
                if (std::abs(denom) > 1.0e-9f)
                    betterTau = (float) tauEstimate + (s2 - s0) / denom;
            }

            float hz = sampleRate / std::max(1.0f, betterTau);
            if (! std::isfinite(hz) || hz < 20.0f || hz > 2000.0f)
            {
                lastPitchHz = 0.0f;
                lastConfidence = 0.0f;
                return;
            }

            lastPitchHz = hz;
            lastConfidence = juce::jlimit(0.0f, 1.0f, 1.0f - cmnd[(size_t) tauEstimate]);
        }
    };

    //==============================
    // Granular pitch shifter: two overlapping grains reading from a circular buffer
    class GranularPitchShifter
    {
    public:
        void prepare(double newSampleRate, int grainSizeSamples = 512)
        {
            sampleRate = (float) newSampleRate;
            grainSize = std::max(64, grainSizeSamples);
            phaseInc = 1.0f / (float) grainSize;

            // Keep buffer comfortably larger than the grain + delay offset
            delayOffset = grainSize * 2;
            bufferSize = juce::nextPowerOfTwo(grainSize * 8);
            buffer.assign((size_t) bufferSize, 0.0f);

            writePos = 0;

            currentRatio = 1.0f;
            targetRatio = 1.0f;
            smoothingCoeff = 1.0f; // immediate by default

            grains[0].phase = 0.0f;
            grains[1].phase = 0.5f;

            resetReadPositions();
        }

        void reset()
        {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
            currentRatio = targetRatio;
            grains[0].phase = 0.0f;
            grains[1].phase = 0.5f;
            resetReadPositions();
        }

        void setTargetRatio(float ratio)
        {
            targetRatio = juce::jlimit(0.25f, 4.0f, ratio);
        }

        void setSmoothingTimeMs(float ms)
        {
            if (ms <= 0.0f)
            {
                smoothingCoeff = 1.0f;
                return;
            }

            float samples = (ms * sampleRate) / 1000.0f;
            samples = std::max(1.0f, samples);

            // standard 1-pole smoothing coefficient
            smoothingCoeff = 1.0f - std::exp(-1.0f / samples);
        }

        float processSample(float in)
        {
            buffer[(size_t) writePos] = in;
            writePos = (writePos + 1) & (bufferSize - 1);

            // Smooth the ratio to avoid zipper noise
            currentRatio += (targetRatio - currentRatio) * smoothingCoeff;

            float out = 0.0f;

            for (auto& g : grains)
            {
                float w = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * g.phase);
                out += w * readInterp(g.readPos);

                g.phase += phaseInc;
                g.readPos = wrapPos(g.readPos + currentRatio);

                if (g.phase >= 1.0f)
                {
                    g.phase -= 1.0f;

                    // Restart grain at a fixed delay behind write head
                    float start = (float) writePos - (float) delayOffset;
                    start = wrapPos(start);

                    g.readPos = wrapPos(start + currentRatio * (float) grainSize * g.phase);
                }
            }

            return out;
        }

    private:
        struct Grain
        {
            float readPos = 0.0f;
            float phase = 0.0f;
        };

        float sampleRate = 44100.0f;
        int grainSize = 512;
        float phaseInc = 1.0f / 512.0f;

        int delayOffset = 1024;
        int bufferSize = 4096;
        std::vector<float> buffer;
        int writePos = 0;

        Grain grains[2];

        float currentRatio = 1.0f;
        float targetRatio = 1.0f;
        float smoothingCoeff = 1.0f;

        void resetReadPositions()
        {
            float start = (float) writePos - (float) delayOffset;
            start = wrapPos(start);

            for (auto& g : grains)
                g.readPos = wrapPos(start + currentRatio * (float) grainSize * g.phase);
        }

        float wrapPos(float p) const
        {
            while (p < 0.0f) p += (float) bufferSize;
            while (p >= (float) bufferSize) p -= (float) bufferSize;
            return p;
        }

        float readInterp(float pos) const
        {
            pos = wrapPos(pos);
            int i0 = (int) pos;
            int i1 = (i0 + 1) & (bufferSize - 1);
            float frac = pos - (float) i0;
            float a = buffer[(size_t) i0];
            float b = buffer[(size_t) i1];
            return a + frac * (b - a);
        }
    };

    //==============================
    // Simple integer-sample delay line (circular buffer)
    class DelayLine
    {
    public:
        void prepare(int newSize)
        {
            size = std::max(1, newSize);
            buffer.assign((size_t) size, 0.0f);
            writePos = 0;
        }

        void reset()
        {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
        }

        inline void write(float s)
        {
            buffer[(size_t) writePos] = s;
            if (++writePos >= size) writePos = 0;
        }

        inline float read(int delaySamples) const
        {
            delaySamples = juce::jlimit(0, size - 1, delaySamples);
            int rp = writePos - delaySamples;
            if (rp < 0) rp += size;
            return buffer[(size_t) rp];
        }

    private:
        int size = 1;
        int writePos = 0;
        std::vector<float> buffer;
    };

    //==============================
    class KeyAwarePitchDelayDSP
    {
    public:
        struct Parameters
        {
            double sampleRate = 44100.0;
            int numChannels = 2;
            int blockSize = 512;
            double bpm = 120.0;

            int keyRoot = 0;
            int scaleType = 0;
            uint16_t customScaleMaskBits = 0; // used when scaleType == ScaleTable::Custom

            int mode = 0;      // 0 interval, 1 tone sequence
            int routing = 0;   // 0 serial, 1 parallel phrase
            int pitchSource = 0;     // 0 audio, 1 midi, 2 fixed
            int trackingSource = 0;  // 0 input, 1 loop (only for audio)
            int fixedMidi = 60;

            bool snapToChord = false;        // MIDI chord defines allowed notes for repeats
            int chordSnapMode = 0;           // 0 Override (allow chord tones outside scale), 1 Intersect with scale
            bool advanceOnTransient = false; // advance sequence phase on detected input transients
            float transientSensitivity = 0.6f; // 0..1

            bool tempoSync = true;
            int delayDivision = 4;
            float delayMs = 400.0f;

            float feedback = 0.35f;
            float mix = 0.5f;
            float outputGainDb = 0.0f;

            int sequenceLength = 4;
            float smoothingMs = 15.0f;

            int intervalStepIndex[8] = { 7,7,7,7,7,7,7,7 }; // choice index 0..14
            int toneStepIndex[8] = { 0,4,2,6,0,4,2,6 };     // choice index 0..11

            float stepLevel[8] = { 1,1,1,1,1,1,1,1 };       // 0..1
            float stepPan[8]   = { 0,0,0,0,0,0,0,0 };       // -1..1
        };

        void prepare(double newSampleRate, int maxBlockSize, int numChannels)
        {
            sampleRate = newSampleRate;
            maxSamples = maxBlockSize;
            channels = std::max(1, numChannels);

            // Transient detector coefficients (time-domain, fast/slow envelope)
            {
                const float sr = (float) juce::jmax(1.0, sampleRate);
                const float tauFast = 0.005f; // 5 ms
                const float tauSlow = 0.050f; // 50 ms
                transientFastCoeff = 1.0f - std::exp(-1.0f / (tauFast * sr));
                transientSlowCoeff = 1.0f - std::exp(-1.0f / (tauSlow * sr));
                transientHoldoffSamples = (int) std::round(0.05f * sr); // 50 ms
            }

            // Delay buffer sizing:
            // For serial routing we only need "baseDelay".
            // For parallel phrase routing the last tap can be (kMaxSteps * baseDelay).
            // Worst-case baseDelay is 1/1 at 20 BPM: 12 seconds, so allocate up to ~96 seconds.
            const double maxDelaySeconds = 12.0 * (double) kMaxSteps;
            const int delayBufferSize = (int) std::ceil(maxDelaySeconds * sampleRate) + 1;

            delayLines.resize((size_t) channels);
            for (auto& dl : delayLines)
                dl.prepare(delayBufferSize);

            serialShifters.resize((size_t) channels);
            for (auto& ps : serialShifters)
                ps.prepare(sampleRate);

            tapShifters.clear();
            tapShifters.resize((size_t) channels);
            for (auto& vec : tapShifters)
            {
                vec.resize(kMaxSteps);
                for (auto& ps : vec)
                    ps.prepare(sampleRate);
            }

            inputAnalysis.assign((size_t) maxSamples, 0.0f);
            loopAnalysis.assign((size_t) maxSamples, 0.0f);

            pitchIn.prepare(sampleRate);
            pitchLoop.prepare(sampleRate);

            scale.set(0, 0, 0);

            reset();
        }

        void reset()
        {
            for (auto& dl : delayLines) dl.reset();
            for (auto& ps : serialShifters) ps.reset();
            for (auto& ch : tapShifters) for (auto& ps : ch) ps.reset();

            pitchIn.reset();
            pitchLoop.reset();

            sequencePhase = 0;
            samplesToNextStep = 0;
            lastBaseDelaySamples = 0;
            lastRefMidi = 60;
            lastMidiNote = 60;
            midiHeld = false;

            midiNotesActive.fill(false);
            chordPitchClasses.fill(false);
            snapPitchClasses.fill(false);
            chordSnapActive = false;

            fastEnv = 0.0f;
            slowEnv = 0.0f;
            transientHoldoff = 0;

            cachedRoot = -1;
            cachedScaleType = -1;
            cachedCustomMaskBits = 0xffffu;
            cachedRouting = -1;
        }

        void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, const Parameters& p)
        {
            const int numSamples = buffer.getNumSamples();
            const int numCh = buffer.getNumChannels();

            // --- MIDI note tracking (Pitch Source + Chord Snap)
            for (const auto meta : midi)
            {
                const auto msg = meta.getMessage();

                if (msg.isNoteOn())
                {
                    const int n = msg.getNoteNumber();
                    if (juce::isPositiveAndBelow(n, 128))
                        midiNotesActive[(size_t) n] = true;
                    lastMidiNote = n;
                }
                else if (msg.isNoteOff())
                {
                    const int n = msg.getNoteNumber();
                    if (juce::isPositiveAndBelow(n, 128))
                        midiNotesActive[(size_t) n] = false;
                }
                else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                {
                    midiNotesActive.fill(false);
                }
            }

            // Determine if any MIDI notes are held and pick a stable reference note
            midiHeld = false;
            int lowestActive = -1;
            for (int n = 0; n < 128; ++n)
            {
                if (midiNotesActive[(size_t) n])
                {
                    midiHeld = true;
                    if (lowestActive < 0)
                        lowestActive = n;
                }
            }

            if (midiHeld)
            {
                const int safeLast = juce::jlimit(0, 127, lastMidiNote);
                if (! midiNotesActive[(size_t) safeLast] && lowestActive >= 0)
                    lastMidiNote = lowestActive;
            }

            chordPitchClasses.fill(false);
            if (midiHeld)
                for (int n = 0; n < 128; ++n)
                    if (midiNotesActive[(size_t) n])
                        chordPitchClasses[(size_t) (n % 12)] = true;

            const bool hasChord = std::any_of(chordPitchClasses.begin(), chordPitchClasses.end(), [](bool b){ return b; });

            // --- Update scale if needed (including custom scale editor mask)
            if (p.keyRoot != cachedRoot || p.scaleType != cachedScaleType || p.customScaleMaskBits != cachedCustomMaskBits)
            {
                scale.set(p.keyRoot, p.scaleType, p.customScaleMaskBits);
                cachedRoot = p.keyRoot;
                cachedScaleType = p.scaleType;
                cachedCustomMaskBits = p.customScaleMaskBits;
            }

            // --- Chord snapping behaviour (Override vs Intersect-with-Scale)
            //
            // - Override: allowed set = chord tones (may include notes outside the current scale)
            // - Intersect: allowed set = chord tones that are ALSO in the current scale
            //
            // If the intersection is empty, chord snapping is effectively disabled (falls back to key/scale behaviour).
            snapPitchClasses.fill(false);
            if (p.snapToChord && hasChord)
            {
                if (p.chordSnapMode == 0)
                {
                    snapPitchClasses = chordPitchClasses;
                }
                else
                {
                    const auto& scalePC = scale.getPitchClasses();
                    for (int pc = 0; pc < 12; ++pc)
                        snapPitchClasses[(size_t) pc] = chordPitchClasses[(size_t) pc] && scalePC[(size_t) pc];
                }
            }

            chordSnapActive = std::any_of(snapPitchClasses.begin(), snapPitchClasses.end(), [](bool b){ return b; });

            // --- Determine reference MIDI note for THIS block (from previous pitch estimate if audio)
            int refMidi = lastRefMidi;

            if (p.pitchSource == 1) // MIDI
            {
                refMidi = lastMidiNote;
            }
            else if (p.pitchSource == 2) // Fixed
            {
                refMidi = p.fixedMidi;
            }
            else // Audio
            {
                float hz = (p.trackingSource == 0) ? pitchIn.getPitchHz() : pitchLoop.getPitchHz();
                float conf = (p.trackingSource == 0) ? pitchIn.getConfidence() : pitchLoop.getConfidence();

                if (hz > 0.0f && conf > 0.05f)
                {
                    refMidi = (int) std::round(hzToMidi(hz));
                }
                // else: keep lastRefMidi
            }

            refMidi = juce::jlimit(0, 127, refMidi);
            refMidi = scale.quantizeMidi(refMidi);
            lastRefMidi = refMidi;

            // --- Compute delay time
            const int baseDelaySamples = computeBaseDelaySamples(p);
            const int seqLen = juce::jlimit(1, kMaxSteps, p.sequenceLength);

            if (sequencePhase < 0 || sequencePhase >= seqLen)
                sequencePhase = 0;

            // Reset the sequence phase when switching routing modes (keeps behaviour predictable)
            if (p.routing != cachedRouting)
            {
                sequencePhase = 0;
                samplesToNextStep = baseDelaySamples;
                cachedRouting = p.routing;
            }

            // Ratio smoothing
            for (auto& ps : serialShifters) ps.setSmoothingTimeMs(p.smoothingMs);
            for (auto& ch : tapShifters) for (auto& ps : ch) ps.setSmoothingTimeMs(p.smoothingMs);

            // --- Serial routing: if delay time changed, reset the step counter to avoid weird boundaries
            if (p.routing == 0 && baseDelaySamples != lastBaseDelaySamples)
            {
                samplesToNextStep = baseDelaySamples;
                sequencePhase = 0;
                lastBaseDelaySamples = baseDelaySamples;

                const float r = computeSerialRatioForStep(p, refMidi, sequencePhase);
                for (auto& ps : serialShifters) ps.setTargetRatio(r);
            }

            // --- Analysis buffers
            if ((int) inputAnalysis.size() < numSamples)
            {
                inputAnalysis.assign((size_t) numSamples, 0.0f);
                loopAnalysis.assign((size_t) numSamples, 0.0f);
            }

            // --- Per-sample processing
            const float mix = juce::jlimit(0.0f, 1.0f, p.mix);
            const float fb  = juce::jlimit(0.0f, 0.95f, p.feedback);
            const float gain = juce::Decibels::decibelsToGain(p.outputGainDb);

            if (p.routing == 0)
            {
                processSerial(buffer, numCh, numSamples, baseDelaySamples, seqLen, p, refMidi, mix, fb, gain);
            }
            else
            {
                processParallel(buffer, numCh, numSamples, baseDelaySamples, seqLen, p, refMidi, mix, fb, gain);
            }

            // --- Feed pitch detectors AFTER processing (for next block's ref pitch)
            pitchIn.processBlock(inputAnalysis.data(), numSamples);
            pitchLoop.processBlock(loopAnalysis.data(), numSamples);
        }

    private:
        static constexpr int kMaxSteps = 8;

        double sampleRate = 44100.0;
        int maxSamples = 512;
        int channels = 2;

        ScaleTable scale;
        int cachedRoot = -1;
        int cachedScaleType = -1;
        uint16_t cachedCustomMaskBits = 0xffffu;
        int cachedRouting = -1;

        std::vector<DelayLine> delayLines;

        std::vector<GranularPitchShifter> serialShifters;
        std::vector<std::vector<GranularPitchShifter>> tapShifters;

        YinPitchDetector pitchIn, pitchLoop;
        std::vector<float> inputAnalysis;
        std::vector<float> loopAnalysis;

        int sequencePhase = 0;
        int samplesToNextStep = 0;
        int lastBaseDelaySamples = 0;

        int lastRefMidi = 60;

        int lastMidiNote = 60;
        bool midiHeld = false;

        // Held MIDI notes (for chord snap)
        std::array<bool, 128> midiNotesActive {};
        std::array<bool, 12> chordPitchClasses {};
        std::array<bool, 12> snapPitchClasses {};
        bool chordSnapActive = false;

        // Transient detector state (simple fast/slow envelope ratio)
        float fastEnv = 0.0f;
        float slowEnv = 0.0f;
        int transientHoldoff = 0;
        int transientHoldoffSamples = 2205; // ~50ms @ 44.1k
        float transientFastCoeff = 0.0f;
        float transientSlowCoeff = 0.0f;

        // --- Helpers
        int computeBaseDelaySamples(const Parameters& p) const
        {
            double bpm = (p.bpm > 0.0 && std::isfinite(p.bpm)) ? p.bpm : 120.0;
            bpm = juce::jlimit(20.0, 300.0, bpm);

            double seconds = 0.0;

            if (p.tempoSync)
            {
                static const std::array<double, 16> beats = {
                    4.0,        // 1/1
                    2.0,        // 1/2
                    3.0,        // 1/2 dotted
                    4.0/3.0,    // 1/2 triplet
                    1.0,        // 1/4
                    1.5,        // 1/4 dotted
                    2.0/3.0,    // 1/4 triplet
                    0.5,        // 1/8
                    0.75,       // 1/8 dotted
                    1.0/3.0,    // 1/8 triplet
                    0.25,       // 1/16
                    0.375,      // 1/16 dotted
                    1.0/6.0,    // 1/16 triplet
                    0.125,      // 1/32
                    0.1875,     // 1/32 dotted
                    1.0/12.0    // 1/32 triplet
                };

                int idx = juce::jlimit(0, (int) beats.size() - 1, p.delayDivision);
                seconds = (60.0 / bpm) * beats[(size_t) idx];
            }
            else
            {
                seconds = juce::jlimit(0.001, 2.0, (double) p.delayMs / 1000.0);
            }

            int samples = (int) std::round(seconds * sampleRate);
            return juce::jlimit(1, 200000, samples);
        }

        int intervalIndexToDegreeShift(int intervalChoiceIndex) const
        {
            // Our interval choices are [-7..+7] mapped to indices [0..14]
            intervalChoiceIndex = juce::jlimit(0, 14, intervalChoiceIndex);
            return intervalChoiceIndex - 7;
        }

        void constantPowerPanGains(float pan, float& gL, float& gR) const
        {
            pan = juce::jlimit(-1.0f, 1.0f, pan);
            const float angle = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi; // 0..pi/2
            gL = std::cos(angle);
            gR = std::sin(angle);
        }

        int snapMidiToChord(int midiNote) const
        {
            midiNote = juce::jlimit(0, 127, midiNote);

            // If no chord tones are present, do nothing.
            bool any = false;
            for (bool b : snapPitchClasses)
                any = any || b;
            if (! any)
                return midiNote;

            int best = midiNote;
            int bestDist = 999;

            // Search within +/- 24 semitones (2 octaves).
            for (int d = -24; d <= 24; ++d)
            {
                const int c = midiNote + d;
                if (c < 0 || c > 127)
                    continue;

                if (snapPitchClasses[(size_t) (c % 12)])
                {
                    const int dist = std::abs(d);
                    if (dist < bestDist || (dist == bestDist && c < best))
                    {
                        bestDist = dist;
                        best = c;
                    }
                }
            }

            return best;
        }

        bool checkTransientAdvance(float inMono, float sensitivity)
        {
            // Fast/slow envelope follower, compare ratio.
            const float absx = std::abs(inMono);
            fastEnv += (absx - fastEnv) * transientFastCoeff;
            slowEnv += (absx - slowEnv) * transientSlowCoeff;

            if (transientHoldoff > 0)
                --transientHoldoff;

            sensitivity = juce::jlimit(0.0f, 1.0f, sensitivity);
            const float threshold = 1.05f + (1.0f - sensitivity) * 1.2f; // higher sens -> lower threshold

            const float ratio = fastEnv / (slowEnv + 1.0e-6f);

            if (transientHoldoff <= 0 && ratio > threshold && absx > 1.0e-4f)
            {
                transientHoldoff = transientHoldoffSamples;
                return true;
            }

            return false;
        }

        float computeSerialRatioForStep(const Parameters& p, int refMidi, int stepIdx)
        {
            stepIdx = juce::jlimit(0, kMaxSteps - 1, stepIdx);

            const int degCount = juce::jmax(1, scale.getDegreeCount());

            if (p.mode == 0) // Mode 1 interval degrees
            {
                int degreeShift = intervalIndexToDegreeShift(p.intervalStepIndex[stepIdx]);
                int targetMidi = scale.shiftByDegrees(refMidi, degreeShift);
                if (chordSnapActive)
                    targetMidi = snapMidiToChord(targetMidi);
                int diff = targetMidi - refMidi;
                return semitonesToRatio((float) diff);
            }
            else // Mode 2 tone sequence (degree targets)
            {
                int deg = juce::jlimit(1, degCount, p.toneStepIndex[stepIdx] + 1);
                int targetMidi = scale.nearestMidiForDegree(deg, refMidi);
                if (chordSnapActive)
                    targetMidi = snapMidiToChord(targetMidi);
                int diff = targetMidi - refMidi;
                return semitonesToRatio((float) diff);
            }
        }

        void computeTapRatios(const Parameters& p, int refMidi, int seqLen, int startPhase, std::array<float, kMaxSteps>& ratios)
        {
            ratios.fill(1.0f);

            if (seqLen <= 0) return;

            startPhase = juce::jlimit(0, seqLen - 1, startPhase);

            const int degCount = juce::jmax(1, scale.getDegreeCount());

            if (p.mode == 0)
            {
                int midi = refMidi;
                for (int i = 0; i < seqLen; ++i)
                {
                    const int stepIdx = (startPhase + i) % seqLen;
                    int degreeShift = intervalIndexToDegreeShift(p.intervalStepIndex[stepIdx]);
                    midi = scale.shiftByDegrees(midi, degreeShift);
                    if (chordSnapActive)
                        midi = snapMidiToChord(midi);
                    int diff = midi - refMidi;
                    ratios[(size_t) i] = semitonesToRatio((float) diff);
                }
            }
            else
            {
                for (int i = 0; i < seqLen; ++i)
                {
                    const int stepIdx = (startPhase + i) % seqLen;
                    int deg = juce::jlimit(1, degCount, p.toneStepIndex[stepIdx] + 1);
                    int targetMidi = scale.nearestMidiForDegree(deg, refMidi);
                    if (chordSnapActive)
                        targetMidi = snapMidiToChord(targetMidi);
                    int diff = targetMidi - refMidi;
                    ratios[(size_t) i] = semitonesToRatio((float) diff);
                }
            }
        }

        void processSerial(juce::AudioBuffer<float>& buffer,
                           int numCh, int numSamples,
                           int baseDelaySamples, int seqLen,
                           const Parameters& p, int refMidi,
                           float mix, float fb, float gain)
        {
            if (samplesToNextStep <= 0)
                samplesToNextStep = baseDelaySamples;

            sequencePhase = juce::jlimit(0, std::max(1, seqLen) - 1, sequencePhase);

            float currentRatio = computeSerialRatioForStep(p, refMidi, sequencePhase);
            float currentLevel = juce::jlimit(0.0f, 1.0f, p.stepLevel[sequencePhase]);
            float currentPan = juce::jlimit(-1.0f, 1.0f, p.stepPan[sequencePhase]);
            float panL = 0.7071067f, panR = 0.7071067f;
            constantPowerPanGains(currentPan, panL, panR);

            for (auto& ps : serialShifters) ps.setTargetRatio(currentRatio);

            for (int s = 0; s < numSamples; ++s)
            {
                // Capture input mono before overwriting buffer
                float inMono = 0.0f;
                for (int ch = 0; ch < numCh; ++ch)
                    inMono += buffer.getSample(ch, s);
                inMono /= (float) std::max(1, numCh);
                inputAnalysis[(size_t) s] = inMono;

                bool advance = false;
                if (p.advanceOnTransient)
                {
                    if (checkTransientAdvance(inMono, p.transientSensitivity))
                        advance = true;
                }
                else
                {
                    if (--samplesToNextStep <= 0)
                    {
                        samplesToNextStep += baseDelaySamples;
                        advance = true;
                    }
                }

                if (advance)
                {
                    sequencePhase = (sequencePhase + 1) % std::max(1, seqLen);
                    currentRatio = computeSerialRatioForStep(p, refMidi, sequencePhase);
                    currentLevel = juce::jlimit(0.0f, 1.0f, p.stepLevel[sequencePhase]);
                    currentPan = juce::jlimit(-1.0f, 1.0f, p.stepPan[sequencePhase]);
                    constantPowerPanGains(currentPan, panL, panR);
                    for (auto& ps : serialShifters) ps.setTargetRatio(currentRatio);
                }

                if (numCh <= 1)
                {
                    const float dry = buffer.getSample(0, s);
                    const float delayed = delayLines[0].read(baseDelaySamples);
                    loopAnalysis[(size_t) s] = delayed;

                    const float pitched = serialShifters[0].processSample(delayed);
                    const float pitchedForFb = pitched * currentLevel;
                    delayLines[0].write(dry + fb * pitchedForFb);

                    const float out = dry * (1.0f - mix) + pitchedForFb * mix;
                    buffer.setSample(0, s, out * gain);
                }
                else
                {
                    const float dryL = buffer.getSample(0, s);
                    const float dryR = buffer.getSample(1, s);

                    const float delayedL = delayLines[0].read(baseDelaySamples);
                    const float delayedR = delayLines[1].read(baseDelaySamples);
                    loopAnalysis[(size_t) s] = 0.5f * (delayedL + delayedR);

                    const float pitchedL = serialShifters[0].processSample(delayedL);
                    const float pitchedR = serialShifters[1].processSample(delayedR);

                    delayLines[0].write(dryL + fb * (pitchedL * currentLevel));
                    delayLines[1].write(dryR + fb * (pitchedR * currentLevel));

                    const float wetMono = 0.5f * (pitchedL + pitchedR) * currentLevel;
                    const float wetL = wetMono * panL;
                    const float wetR = wetMono * panR;

                    const float outL = dryL * (1.0f - mix) + wetL * mix;
                    const float outR = dryR * (1.0f - mix) + wetR * mix;

                    buffer.setSample(0, s, outL * gain);
                    buffer.setSample(1, s, outR * gain);
                }
            }
        }

        void processParallel(juce::AudioBuffer<float>& buffer,
                             int numCh, int numSamples,
                             int baseDelaySamples, int seqLen,
                             const Parameters& p, int refMidi,
                             float mix, float fb, float gain)
        {
            sequencePhase = juce::jlimit(0, std::max(1, seqLen) - 1, sequencePhase);

            std::array<float, kMaxSteps> tapRatios {};
            std::array<float, kMaxSteps> tapLevel {};
            std::array<float, kMaxSteps> tapGainL {};
            std::array<float, kMaxSteps> tapGainR {};

            float wetNorm = 1.0f;

            auto updateTapParams = [&]()
            {
                computeTapRatios(p, refMidi, seqLen, sequencePhase, tapRatios);

                for (int ch = 0; ch < numCh; ++ch)
                    for (int i = 0; i < seqLen; ++i)
                        tapShifters[(size_t) ch][(size_t) i].setTargetRatio(tapRatios[(size_t) i]);

                float sumLevels = 0.0f;
                for (int i = 0; i < seqLen; ++i)
                {
                    const int stepIdx = (sequencePhase + i) % seqLen;
                    const float lvl = juce::jlimit(0.0f, 1.0f, p.stepLevel[stepIdx]);
                    const float pan = juce::jlimit(-1.0f, 1.0f, p.stepPan[stepIdx]);

                    tapLevel[(size_t) i] = lvl;

                    float gL = 0.7071067f, gR = 0.7071067f;
                    constantPowerPanGains(pan, gL, gR);
                    tapGainL[(size_t) i] = lvl * gL;
                    tapGainR[(size_t) i] = lvl * gR;

                    sumLevels += lvl;
                }

                wetNorm = (sumLevels > 1.0e-4f) ? (1.0f / sumLevels) : 1.0f;
            };

            updateTapParams();

            for (int s = 0; s < numSamples; ++s)
            {
                // Capture input mono before overwriting buffer
                float inMono = 0.0f;
                for (int ch = 0; ch < numCh; ++ch)
                    inMono += buffer.getSample(ch, s);
                inMono /= (float) std::max(1, numCh);
                inputAnalysis[(size_t) s] = inMono;

                if (p.advanceOnTransient)
                {
                    if (checkTransientAdvance(inMono, p.transientSensitivity))
                    {
                        sequencePhase = (sequencePhase + 1) % std::max(1, seqLen);
                        updateTapParams();
                    }
                }

                if (numCh <= 1)
                {
                    const float dry = buffer.getSample(0, s);

                    float wet = 0.0f;
                    float lastTapPitchedForFb = 0.0f;
                    float lastTapInput = 0.0f;

                    for (int i = 0; i < seqLen; ++i)
                    {
                        const int dSamp = (i + 1) * baseDelaySamples;
                        const float tapIn = delayLines[0].read(dSamp);
                        const float tapOut = tapShifters[0][(size_t) i].processSample(tapIn);

                        wet += tapOut * tapLevel[(size_t) i];

                        if (i == seqLen - 1)
                        {
                            lastTapPitchedForFb = tapOut * tapLevel[(size_t) i];
                            lastTapInput = tapIn;
                        }
                    }

                    wet *= wetNorm;
                    delayLines[0].write(dry + fb * lastTapPitchedForFb);

                    loopAnalysis[(size_t) s] = lastTapInput;

                    const float out = dry * (1.0f - mix) + wet * mix;
                    buffer.setSample(0, s, out * gain);
                }
                else
                {
                    const float dryL = buffer.getSample(0, s);
                    const float dryR = buffer.getSample(1, s);

                    float wetL = 0.0f;
                    float wetR = 0.0f;
                    float lastTapPitchedForFbL = 0.0f;
                    float lastTapPitchedForFbR = 0.0f;
                    float lastTapInputMono = 0.0f;

                    for (int i = 0; i < seqLen; ++i)
                    {
                        const int dSamp = (i + 1) * baseDelaySamples;
                        const float tapInL = delayLines[0].read(dSamp);
                        const float tapInR = delayLines[1].read(dSamp);

                        const float tapOutL = tapShifters[0][(size_t) i].processSample(tapInL);
                        const float tapOutR = tapShifters[1][(size_t) i].processSample(tapInR);
                        const float tapMono = 0.5f * (tapOutL + tapOutR);

                        wetL += tapMono * tapGainL[(size_t) i];
                        wetR += tapMono * tapGainR[(size_t) i];

                        if (i == seqLen - 1)
                        {
                            const float lvl = tapLevel[(size_t) i];
                            lastTapPitchedForFbL = tapOutL * lvl;
                            lastTapPitchedForFbR = tapOutR * lvl;
                            lastTapInputMono = 0.5f * (tapInL + tapInR);
                        }
                    }

                    wetL *= wetNorm;
                    wetR *= wetNorm;

                    delayLines[0].write(dryL + fb * lastTapPitchedForFbL);
                    delayLines[1].write(dryR + fb * lastTapPitchedForFbR);

                    loopAnalysis[(size_t) s] = lastTapInputMono;

                    const float outL = dryL * (1.0f - mix) + wetL * mix;
                    const float outR = dryR * (1.0f - mix) + wetR * mix;

                    buffer.setSample(0, s, outL * gain);
                    buffer.setSample(1, s, outR * gain);
                }
            }
        }
    };
}
