// ============================================================================
// AudioTimeLattice.h
// Universal time-grid system for audio editing and quantization
// Reusable across multiple audio plugins
// ============================================================================
#pragma once
#include <JuceHeader.h>
#include <vector>
#include <map>
#include <string>

// ============================================================================
// Time Domain Enumerations
// ============================================================================

enum class TimeDomain {
    AudioSamples,      // Absolute sample count
    Seconds,           // Real-time seconds
    MusicalTicks,      // PPQN-based musical time
    BarsBeatsTicks,    // Human-readable musical (bar.beat.tick)
    SMPTEFrames        // Video/film frames
};

enum class QuantizeMode {
    Nearest,           // Round to nearest grid point
    Floor,             // Round down
    Ceil               // Round up
};

enum class ValueResolution {
    Bit7 = 7,          // MIDI CC (128 steps)
    Bit14 = 14,        // MIDI NRPN (16384 steps)
    Bit24 = 24,        // Audio (16.7M steps)
    Bit32 = 32         // Float precision
};

// ============================================================================
// Musical Time Structure
// ============================================================================

struct MusicalTime {
    int bars = 1;          // 1-based
    int beats = 1;         // 1-based
    int ticks = 0;         // 0-based
    double remainder = 0;  // Sub-tick precision

    std::string toString() const;
    static MusicalTime fromString(const std::string& str);
};

// ============================================================================
// Tempo Event (for tempo maps)
// ============================================================================

struct TempoEvent {
    double timeInSeconds = 0.0;
    double bpm = 120.0;
    int upperTimeSig = 4;      // Numerator (beats per bar)
    int lowerTimeSig = 4;      // Denominator (beat unit)
};

// ============================================================================
// Audio Edit Marker (for editing operations)
// ============================================================================

struct AudioMarker {
    double timeInSeconds;
    std::string label;
    juce::Colour color;
    int id;
};

// ============================================================================
// Main Time Lattice System
// ============================================================================

class AudioTimeLattice {
public:
    AudioTimeLattice(int ppqn = 960, double sampleRate = 48000.0);
    ~AudioTimeLattice() = default;

    // ========================================================================
    // Configuration
    // ========================================================================

    void setPPQN(int ppqn);
    int getPPQN() const { return ppqn; }

    void setSampleRate(double rate);
    double getSampleRate() const { return sampleRate; }

    void setTempo(double bpm, double timeInSeconds = 0.0);
    void addTempoChange(const TempoEvent& tempo);
    void clearTempoMap();

    // ========================================================================
    // Time Domain Conversions
    // ========================================================================

    double convert(double value, TimeDomain from, TimeDomain to);
    double toSeconds(double value, TimeDomain domain);
    double fromSeconds(double seconds, TimeDomain domain);

    // Specific conversions
    MusicalTime secondsToMusical(double seconds);
    double musicalToSeconds(const MusicalTime& mt);
    int secondsToSamples(double seconds);
    double samplesToSeconds(int samples);

    // ========================================================================
    // Grid Generation
    // ========================================================================

    std::vector<double> generatePPQNGrid(double startSeconds, double endSeconds);
    std::vector<MusicalTime> generateMusicalGrid(double startSeconds, double endSeconds);
    std::vector<double> generateBeatGrid(double startSeconds, double endSeconds);
    std::vector<double> generateBarGrid(double startSeconds, double endSeconds);

    // ========================================================================
    // Quantization
    // ========================================================================

    double quantizeToGrid(double timeInSeconds, QuantizeMode mode = QuantizeMode::Nearest);
    double quantizeToBeat(double timeInSeconds, QuantizeMode mode = QuantizeMode::Nearest);
    double quantizeToBar(double timeInSeconds, QuantizeMode mode = QuantizeMode::Nearest);

    // Value quantization
    double quantizeValue(double value, ValueResolution resolution);

    // Batch quantization
    std::vector<std::pair<double, double>> quantizeBreakpoints(
        const std::vector<std::pair<double, double>>& input,
        ValueResolution resolution,
        bool simplify = true);

    // ========================================================================
    // Audio Editing Operations
    // ========================================================================

    // Basic operations
    juce::AudioBuffer<float> trim(const juce::AudioBuffer<float>& input,
        double startTime, double endTime);

    juce::AudioBuffer<float> cut(const juce::AudioBuffer<float>& input,
        double startTime, double endTime);

    std::vector<juce::AudioBuffer<float>> split(const juce::AudioBuffer<float>& input,
        const std::vector<double>& splitTimes);

    juce::AudioBuffer<float> merge(const std::vector<juce::AudioBuffer<float>>& clips,
        const std::vector<double>& positions);

    juce::AudioBuffer<float> nudge(const juce::AudioBuffer<float>& input,
        double nudgeAmount, bool fillWithSilence = true);

    // Advanced operations
    juce::AudioBuffer<float> timeStretch(const juce::AudioBuffer<float>& input,
        double stretchFactor);

    juce::AudioBuffer<float> quantizeAudio(const juce::AudioBuffer<float>& input,
        double audioStartTime,
        double quantizeStrength = 1.0);

    juce::AudioBuffer<float> humanize(const juce::AudioBuffer<float>& input,
        double audioStartTime,
        double humanizeAmount = 0.1);

    juce::AudioBuffer<float> grooveQuantize(const juce::AudioBuffer<float>& input,
        double audioStartTime,
        const std::vector<double>& grooveTemplate);

    // Transient/beat detection
    std::vector<double> detectTransients(const juce::AudioBuffer<float>& input,
        double threshold = 0.5);

    std::vector<AudioMarker> detectBeats(const juce::AudioBuffer<float>& input);

    // Loop operations
    std::pair<double, double> findBestLoopPoints(const juce::AudioBuffer<float>& input,
        double approximateStart,
        double approximateEnd);

    juce::AudioBuffer<float> createLoop(const juce::AudioBuffer<float>& input,
        double startTime, double endTime,
        int numRepeats);

    // Crossfade
    juce::AudioBuffer<float> crossfade(const juce::AudioBuffer<float>& clip1,
        const juce::AudioBuffer<float>& clip2,
        double crossfadeDuration);

    // Warp (time-stretching with markers)
    juce::AudioBuffer<float> warpToGrid(const juce::AudioBuffer<float>& input,
        const std::vector<double>& detectedBeats);

    // ========================================================================
    // Marker Management
    // ========================================================================

    void addMarker(double timeInSeconds, const std::string& label,
        juce::Colour color = juce::Colours::yellow);
    void removeMarker(int id);
    void clearMarkers();
    std::vector<AudioMarker> getMarkers() const { return markers; }
    AudioMarker* findNearestMarker(double timeInSeconds, double tolerance = 0.1);

    // ========================================================================
    // Grid Snapping Helpers
    // ========================================================================

    double snapToGrid(double timeInSeconds, double gridSpacing);
    bool isOnGrid(double timeInSeconds, double tolerance = 0.001);
    double getNextGridPoint(double timeInSeconds);
    double getPreviousGridPoint(double timeInSeconds);

    // ========================================================================
    // Export/Import
    // ========================================================================

    std::string exportToMIDI(const std::vector<std::pair<double, double>>& breakpoints,
        int ccNumber = 10, int channel = 0);

    std::string exportMarkersToJSON();
    void importMarkersFromJSON(const std::string& json);

    // ========================================================================
    // Utility
    // ========================================================================

    double getTickDuration() const;
    double getBeatDuration(double atTime = 0.0) const;
    double getBarDuration(double atTime = 0.0) const;
    TempoEvent getTempoAt(double timeInSeconds) const;

private:
    // Core settings
    int ppqn;
    double sampleRate;
    std::vector<TempoEvent> tempoMap;
    std::vector<AudioMarker> markers;
    int nextMarkerId = 1;

    // Helper methods
    TempoEvent getCurrentTempo(double timeInSeconds = 0.0) const;
    int musicalToTotalTicks(const MusicalTime& mt) const;
    MusicalTime totalTicksToMusical(int totalTicks) const;

    // Audio processing helpers
    void applyFade(juce::AudioBuffer<float>& buffer, bool fadeIn,
        int startSample, int numSamples);

    std::vector<int> findZeroCrossings(const juce::AudioBuffer<float>& buffer,
        int startSample, int endSample);

    int findNearestZeroCrossing(const juce::AudioBuffer<float>& buffer,
        int targetSample);

    // Quantization helpers
    double calculatePerceptualThreshold(ValueResolution resolution);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioTimeLattice)
};