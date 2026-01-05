I've started to try to add "snapping" via creation of a lower needs timeing grid.
# AudioTimeLattice - Usage Guide

## Overview

The **AudioTimeLattice** system provides a unified framework for time-based audio editing operations with musical timing awareness. It bridges the gap between sample-accurate audio processing and musical time (bars, beats, ticks).

## Core Concept

Modern DAWs operate at sample-rate precision (48kHz = 20.8μs resolution), but musical time is measured in beats and bars. The lattice system creates a **musical grid** that audio can be snapped to, edited on, and quantized against.

---

## Key Features

### 1. Multiple Time Domains
- **AudioSamples**: Raw sample counts (e.g., 48000 samples = 1 second at 48kHz)
- **Seconds**: Real-time seconds
- **MusicalTicks**: PPQN-based resolution (e.g., 960 ticks per quarter note)
- **BarsBeatsTicks**: Human-readable musical time (001:01:000)
- **SMPTEFrames**: Video/film synchronization (30fps, 29.97fps, etc.)

### 2. PPQN Resolution Options
| PPQN | Tick Duration @120BPM | Use Case |
|------|---------------------|----------|
| **24** | 20.8ms | MIDI sync, basic sequencing |
| **48** | 10.4ms | Better swing feel |
| **96** | 5.2ms | Good for most music |
| **192** | 2.6ms | Fine timing |
| **384** | 1.3ms | Professional |
| **480** | 1.04ms | Pro Tools standard |
| **960** | 0.52ms | **Recommended** - inaudible error |

### 3. Value Resolution Options
| Bits | Steps | Range | Use Case |
|------|-------|-------|----------|
| **7-bit** | 128 | MIDI CC | Basic automation |
| **14-bit** | 16,384 | MIDI NRPN | **Recommended** for smooth curves |
| **24-bit** | 16.7M | Audio precision | Scientific accuracy |
| **32-bit** | 4.2B | Float precision | Internal processing |

---

## Integration in AudioBuilder

### Setup (in PluginProcessor)

```cpp
// Initialize in constructor
AudioBuilderProcessor::AudioBuilderProcessor() {
    timeLattice = std::make_unique<AudioTimeLattice>(960, 48000.0);
    timeLattice->setTempo(120.0);
}

// Update sample rate
void AudioBuilderProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    if (timeLattice) {
        timeLattice->setSampleRate(sampleRate);
    }
}
```

### UI Controls

**PPQN Selector**: Choose grid resolution (24-960)
**Resolution Selector**: Choose value quantization (7-bit to 32-bit)
**Edit Operation Dropdown**: Select from 10+ audio operations
**Snap to Grid Button**: Quantize breakpoints to musical grid

---

## Audio Editing Operations

### 1. **Trim** - Remove edges
```cpp
// Trim audio from 1s to 5s
auto trimmed = timeLattice->trim(audioBuffer, 1.0, 5.0);
```

### 2. **Cut** - Remove middle section
```cpp
// Cut from 2s to 4s (removes that section)
auto cutResult = timeLattice->cut(audioBuffer, 2.0, 4.0);
```

### 3. **Split** - Divide into segments
```cpp
// Split at 2s and 4s into 3 segments
std::vector<double> splitTimes = {2.0, 4.0};
auto segments = timeLattice->split(audioBuffer, splitTimes);
```

### 4. **Nudge** - Shift timing
```cpp
// Nudge forward by 100ms
auto nudged = timeLattice->nudge(audioBuffer, 0.1, true);

// Nudge backward by 50ms
auto nudged = timeLattice->nudge(audioBuffer, -0.05, true);
```

### 5. **Quantize to Grid** - Snap transients to beat
```cpp
// Quantize audio events to nearest PPQN grid point
auto quantized = timeLattice->quantizeAudio(audioBuffer, 0.0, 1.0);
```

### 6. **Humanize** - Add natural timing variation
```cpp
// Add subtle random timing variations (±10% of tick duration)
auto humanized = timeLattice->humanize(audioBuffer, 0.0, 0.1);
```

### 7. **Detect Beats** - Find transients
```cpp
// Detect beats/transients in audio
auto beats = timeLattice->detectBeats(audioBuffer);

// Access detected times
for (const auto& beat : beats) {
    DBG("Beat at: " << beat.timeInSeconds << "s");
}
```

### 8. **Time Stretch** - Change duration without pitch
```cpp
// Stretch to 1.5x duration (slower)
auto stretched = timeLattice->timeStretch(audioBuffer, 1.5);

// Compress to 0.75x duration (faster)
auto compressed = timeLattice->timeStretch(audioBuffer, 0.75);
```

### 9. **Crossfade** - Blend two clips
```cpp
// Crossfade over 100ms
auto blended = timeLattice->crossfade(clip1, clip2, 0.1);
```

### 10. **Snap Breakpoints** - Quantize automation
```cpp
// Quantize breakpoints to musical grid
auto quantized = timeLattice->quantizeBreakpoints(
    breakpoints, 
    ValueResolution::Bit14, 
    true  // simplify
);
```

---

## Time Domain Conversion Examples

### Musical Time to Seconds
```cpp
MusicalTime mt{1, 1, 0, 0};  // Bar 1, beat 1, tick 0
double seconds = timeLattice->musicalToSeconds(mt);
// Result: 0.0s (start of bar 1)
```

### Seconds to Musical Time
```cpp
double seconds = 2.5;
MusicalTime mt = timeLattice->secondsToMusical(seconds);
// At 120 BPM, 4/4: Result might be Bar 1, Beat 4, Tick 480
```

### Generate PPQN Grid
```cpp
// Get all grid points between 0s and 5s
auto gridPoints = timeLattice->generatePPQNGrid(0.0, 5.0);

// Use for snapping UI elements or quantizing events
for (double gridTime : gridPoints) {
    // Draw grid line at gridTime
}
```

---

## Practical Use Cases

### Use Case 1: Drum Loop Slice Marker Creation
```cpp
// 1. Detect beats in drum loop
auto beats = timeLattice->detectBeats(drumLoop);

// 2. Snap beat positions to 16th note grid
for (auto& beat : beats) {
    beat.timeInSeconds = timeLattice->quantizeToGrid(beat.timeInSeconds);
}

// 3. Add markers for export
for (const auto& beat : beats) {
    timeLattice->addMarker(beat.timeInSeconds, "Slice", juce::Colours::yellow);
}

// 4. Export markers as JSON for DAW import
std::string json = timeLattice->exportMarkersToJSON();
```

### Use Case 2: Quantize Breakpoint Automation
```cpp
// Load amplitude envelope breakpoints
std::vector<std::pair<double, double>> breakpoints = 
    processor.getBreakpointsForDisplay(0);

// Quantize to 960 PPQN with 14-bit value resolution
auto quantized = timeLattice->quantizeBreakpoints(
    breakpoints,
    ValueResolution::Bit14,
    true  // simplify redundant points
);

// Result: ~99% fewer points, completely inaudible difference
// Original: 5000 points at 0.000014 precision
// Quantized: ~50 points at 0.000122 precision
```

### Use Case 3: Tempo-Synced Audio Editing
```cpp
// Set project tempo
timeLattice->setTempo(128.0);  // 128 BPM

// Trim to exact bar boundaries
double barLength = timeLattice->getBarDuration();
auto trimmed = timeLattice->trim(audio, 0.0, barLength * 4);  // 4 bars

// Split into beats
std::vector<double> beatTimes;
for (int i = 1; i <= 16; ++i) {  // 16 beats
    beatTimes.push_back(timeLattice->getBeatDuration() * i);
}
auto beatSegments = timeLattice->split(trimmed, beatTimes);
```

---

## Reusing in Other Plugins

The `AudioTimeLattice.h` and `AudioTimeLattice.cpp` files are **fully standalone** and can be dropped into any JUCE plugin project.

### Integration Steps

1. **Copy files to your project**
```
YourPlugin/
├── Source/
│   ├── AudioTimeLattice.h
│   ├── AudioTimeLattice.cpp
│   ├── PluginProcessor.h
│   └── PluginProcessor.cpp
```

2. **Include in processor header**
```cpp
#include "AudioTimeLattice.h"

class YourProcessor : public juce::AudioProcessor {
private:
    std::unique_ptr<AudioTimeLattice> timeLattice;
};
```

3. **Initialize in constructor**
```cpp
YourProcessor::YourProcessor() {
    timeLattice = std::make_unique<AudioTimeLattice>(960, 48000.0);
}
```

4. **Use in your operations**
```cpp
void YourProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer&) {
    // Quantize incoming audio to beat grid
    if (shouldQuantize) {
        buffer = timeLattice->quantizeAudio(buffer, currentTime, 1.0);
    }
}
```

---

## Performance Considerations

### Memory Usage
- **Breakpoint quantization**: Reduces data by ~99%
  - Before: 5000 points × 16 bytes = 80 KB
  - After: 50 points × 16 bytes = 800 bytes

### CPU Usage
- **Grid generation**: O(n) where n = duration × PPQN
  - At 960 PPQN, 120 BPM: ~1920 points/second
  - For 10s audio: ~19,200 points (negligible memory)

### Recommended Settings for DAW Compatibility
```cpp
// Smooth automation curves
timeLattice->setPPQN(960);
processor.setTimeGridResolution(ValueResolution::Bit14);

// Aggressive simplification
auto quantized = timeLattice->quantizeBreakpoints(
    breakpoints,
    ValueResolution::Bit14,
    true  // Enable simplification
);
```

**Result**: FL Studio, Ableton, Logic will handle the automation smoothly with no performance issues.

---

## Advanced: Custom Tempo Maps

```cpp
// Add tempo changes
timeLattice->addTempoChange({0.0, 120.0, 4, 4});    // 120 BPM at start
timeLattice->addTempoChange({10.0, 140.0, 4, 4});   // 140 BPM at 10s
timeLattice->addTempoChange({20.0, 100.0, 3, 4});   // 100 BPM, 3/4 time at 20s

// Grid will automatically adapt to tempo changes
auto grid = timeLattice->generatePPQNGrid(0.0, 30.0);
```

---

## Troubleshooting

### "Breakpoints sound stepped/robotic"
- **Solution**: Increase value resolution from 7-bit to 14-bit
- Check PPQN isn't too low (use 480+ for smooth curves)

### "DAW lags when importing automation"
- **Solution**: Click "Reduce Points" or enable simplification
- Target <200 breakpoints for complex automation

### "Audio clicks after quantization"
- **Solution**: Enable zero-crossing detection (coming in v2)
- Reduce quantize strength parameter

### "Tempo changes not working"
- **Solution**: Call `addTempoChange()` for each tempo event
- Ensure times are in ascending order

---

## API Reference Summary

### Configuration
```cpp
void setPPQN(int ppqn);                          // Set grid resolution
void setSampleRate(double rate);                  // Set sample rate
void setTempo(double bpm, double time = 0.0);    // Set single tempo
void addTempoChange(const TempoEvent& tempo);    // Add tempo event
```

### Time Conversion
```cpp
double convert(double value, TimeDomain from, TimeDomain to);
MusicalTime secondsToMusical(double seconds);
double musicalToSeconds(const MusicalTime& mt);
```

### Grid Generation
```cpp
std::vector<double> generatePPQNGrid(double start, double end);
std::vector<double> generateBeatGrid(double start, double end);
std::vector<double> generateBarGrid(double start, double end);
```

### Quantization
```cpp
double quantizeToGrid(double time, QuantizeMode mode);
double quantizeValue(double value, ValueResolution resolution);
std::vector<std::pair<double, double>> quantizeBreakpoints(...);
```

### Editing Operations
```cpp
AudioBuffer<float> trim(const AudioBuffer<float>&, double start, double end);
AudioBuffer<float> cut(const AudioBuffer<float>&, double start, double end);
std::vector<AudioBuffer<float>> split(const AudioBuffer<float>&, const std::vector<double>& times);
AudioBuffer<float> nudge(const AudioBuffer<float>&, double amount, bool fill);
AudioBuffer<float> timeStretch(const AudioBuffer<float>&, double factor);
AudioBuffer<float> quantizeAudio(const AudioBuffer<float>&, double startTime, double strength);
AudioBuffer<float> humanize(const AudioBuffer<float>&, double startTime, double amount);
AudioBuffer<float> crossfade(const AudioBuffer<float>&, const AudioBuffer<float>&, double duration);
```

### Analysis
```cpp
std::vector<double> detectTransients(const AudioBuffer<float>&, double threshold);
std::vector<AudioMarker> detectBeats(const AudioBuffer<float>&);
```

---


 
