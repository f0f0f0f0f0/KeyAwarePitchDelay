# KeyAware Pitch Delay (JUCE)

A tempo-synced delay that repitches repeats **diatonically** (key-aware) instead of using fixed semitone intervals.

## Features

- Tempo-synced delay time (musical divisions) **or** ms
- Key & scale selection (Major/Minor + a few extras)
- Two pitch modes:
  - **Mode 1 (Interval / diatonic degrees):** each repeat shifts by a *scale-degree interval* (e.g. +2 degrees = “3rd up”), staying in key
  - **Mode 2 (Scale tone sequence):** each repeat snaps to specified scale degrees (e.g. 1, 5, 3, 7)
- Two routing modes:
  - **Serial:** pitch shifter in the feedback loop (classic “harmonizer delay” feel; indefinite repeats)
  - **Parallel (phrase loop):** multi-tap phrase of length `N * baseDelay`, with feedback from the last tap so the *pattern repeats without overlapping*
- Pitch source options:
  - **Audio** (built-in monophonic pitch detector)
  - **MIDI** (use last played MIDI note as reference)
  - **Fixed** (user-selected reference note)
- Wet/Dry mix, Feedback, Output gain, Ratio smoothing
- Per-step **Level** and **Pan** (classic multitap feel)
- **Custom scale editor** (12 semitone toggles) in addition to presets
- **Advance on transient** option (sequence phase advances when an input onset is detected)
- **Snap repeats to chord** mode (feed MIDI chords to constrain the repeat notes)
  - Chord Snap Mode: **Override** (allow chord tones outside the scale) or **Intersect w/ Scale**

## Build (macOS)

### 1) Get JUCE

Either:

- Clone JUCE into this folder as `./JUCE`

or

- Pass `-DJUCE_DIR=/path/to/JUCE` to CMake

### 2) Configure + build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Outputs will include:

- AU (`.component`)
- VST3 (`.vst3`)
- Standalone app

## Known limitations

- The audio pitch detector is **best on monophonic** sources (vocals, bass, leads).
- The pitch shifter is a lightweight granular shifter; extreme shifts can create artifacts (often still musically usable for delay effects).

