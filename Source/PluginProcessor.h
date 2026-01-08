// ============================================================================
// Audio Workshop - PluginProcessor.h
// Unified audio analysis, editing, and processing tool
// Combines AudioDeconstructor + AudioBuilder + Time Lattice
// ============================================================================
#pragma once
#include <JuceHeader.h>
#include "AudioTimeLattice.h"
#include "FeatureExtractors.h"
#include <map>
#include <vector>

class AudioWorkshopProcessor : public juce::AudioProcessor {
public:
    AudioWorkshopProcessor();
    ~AudioWorkshopProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Audio Workshop"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    // ========================================================================
    // AUDIO FILE MANAGEMENT (Dual Audio System)
    // ========================================================================

    // Source audio (for feature extraction)
    bool loadSourceAudio(const juce::File& file);
    void clearSourceAudio();
    bool hasSourceAudio() const { return sourceAudio.getNumSamples() > 0; }
    const juce::AudioBuffer<float>& getSourceAudio() const { return sourceAudio; }
    double getSourceSampleRate() const { return sourceSampleRate; }
    juce::String getSourceFileName() const { return sourceFileName; }

    // Target audio (for breakpoint application)
    bool loadTargetAudio(const juce::File& file);
    void clearTargetAudio();
    bool hasTargetAudio() const { return targetAudio.getNumSamples() > 0; }
    const juce::AudioBuffer<float>& getTargetAudio() const { return targetAudio; }
    const juce::AudioBuffer<float>& getProcessedAudio() const { return processedAudio; }
    double getTargetSampleRate() const { return targetSampleRate; }
    juce::String getTargetFileName() const { return targetFileName; }

    // ========================================================================
    // FEATURE EXTRACTION (from AudioDeconstructor)
    // ========================================================================

    void extractFeature(const juce::String& featureName, int channel = 0);
    void extractAllFeatures();
    void extractADSRFromAmplitude();

    bool isFeatureExtracted(const juce::String& featureName) const;
    juce::StringArray getExtractedFeatures() const;
    juce::StringArray getAvailableFeatures() const;

    juce::Colour getFeatureColour(const juce::String& featureName) const;
    int getNumOutputsForFeature(const juce::String& featureName) const;
    juce::String getOutputName(const juce::String& featureName, int outputIndex) const;

    // ========================================================================
    // BREAKPOINT MANAGEMENT (Unified System)
    // ========================================================================

    // Access breakpoints
    std::vector<std::pair<double, double>> getBreakpointsForDisplay(
        const juce::String& featureName, int outputIndex = 0) const;

    // Edit breakpoints
    void addBreakpoint(const juce::String& featureName, int outputIndex,
        double time, double value);
    void updateBreakpoint(const juce::String& featureName, int outputIndex,
        size_t pointIndex, double time, double value);
    void removeBreakpoint(const juce::String& featureName, int outputIndex,
        size_t pointIndex);
    void sortBreakpoints(const juce::String& featureName, int outputIndex);

    // Decimation for performance
    void decimateBreakpoints(const juce::String& featureName, int outputIndex,
        int targetPoints);
    int getCurrentBreakpointCount(const juce::String& featureName, int outputIndex) const;

    // File I/O for breakpoints
    bool loadBreakpointFile(const juce::File& file);
    void saveBreakpoints(const juce::String& featureName, const juce::File& file);
    void saveAllBreakpoints(const juce::File& directory);
    void clearBreakpoints();
    bool hasBreakpoints() const;

    // ========================================================================
    // TIME LATTICE & QUANTIZATION (from AudioBuilder)
    // ========================================================================

    void initializeTimeLattice();
    void setTimeGridPPQN(int ppqn);
    int getTimeGridPPQN() const { return currentPPQN; }
    void setTimeGridResolution(ValueResolution resolution);
    ValueResolution getTimeGridResolution() const { return currentResolution; }

    // Quantize extracted breakpoints to musical grid
    void quantizeBreakpointsToGrid(const juce::String& featureName, int outputIndex);

    // ========================================================================
    // AUDIO PROCESSING & APPLICATION
    // ========================================================================

    void applyBreakpointsToTarget();
    void exportProcessedAudio(const juce::File& file);
    bool isProcessing() const { return processing.load(); }
    float getProcessingProgress() const { return processingProgress.load(); }

    // ========================================================================
    // AUDIO EDITING OPERATIONS (Using Time Lattice + Analysis)
    // ========================================================================

    enum class EditOperation {
        None,
        Trim,
        Cut,
        Split,
        Nudge,
        TimeStretch,
        Quantize,
        Humanize,
        DetectBeats,
        SnapToGrid,
        Crossfade,
        RemoveSilence,     // NEW: Use amplitude analysis
        IsolateTransients, // NEW: Use transient detection
        SplitByBeats      // NEW: Use beat detection
    };

    juce::AudioBuffer<float> performEditOperation(EditOperation op,
        const std::vector<double>& params);

    // Advanced editing using analysis
    juce::AudioBuffer<float> removeSilence(const juce::AudioBuffer<float>& input,
        double threshold = -40.0); // dB
    std::vector<juce::AudioBuffer<float>> splitByBeats(const juce::AudioBuffer<float>& input);
    juce::AudioBuffer<float> isolateTransients(const juce::AudioBuffer<float>& input,
        double sensitivity = 0.5);

    juce::AudioProcessorValueTreeState params;
    std::unique_ptr<AudioTimeLattice> timeLattice;

private:
    // Audio buffers (dual system)
    juce::AudioBuffer<float> sourceAudio;      // For extraction
    juce::AudioBuffer<float> targetAudio;      // For application
    juce::AudioBuffer<float> processedAudio;   // Result

    double sourceSampleRate = 44100.0;
    double targetSampleRate = 44100.0;
    juce::String sourceFileName;
    juce::String targetFileName;

    // Feature extraction system
    std::map<juce::String, std::unique_ptr<FeatureExtractor>> extractors;
    std::map<juce::String, std::vector<std::vector<std::pair<double, double>>>> featureBreakpoints;

    std::atomic<bool> isAnalyzing{ false };
    std::atomic<float> analysisProgress{ 0.0f };

    // Time grid state
    int currentPPQN = 960;
    ValueResolution currentResolution = ValueResolution::Bit14;

    // Processing state
    std::atomic<bool> processing{ false };
    std::atomic<float> processingProgress{ 0.0f };

    // Helper methods
    void initializeExtractors();
    float interpolateValue(const std::vector<std::pair<double, double>>& points, double time);

    // Processing methods
    void applyAmplitudeModification();
    void applyPanningModification();
    void applyADSRModification();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioWorkshopProcessor)
};