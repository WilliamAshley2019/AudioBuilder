// ============================================================================
// AudioBuilder - PluginProcessor.h
// Applies breakpoint data to modify audio files
// ============================================================================
#pragma once
#include <JuceHeader.h>
#include <map>
#include <vector>

class AudioBuilderProcessor : public juce::AudioProcessor {
public:
    AudioBuilderProcessor();
    ~AudioBuilderProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Audio Builder"; }
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

    // Audio file management
    bool loadAudioFile(const juce::File& file);
    void clearLoadedAudio();
    bool hasLoadedAudio() const { return sourceAudio.getNumSamples() > 0; }
    const juce::AudioBuffer<float>& getSourceAudio() const { return sourceAudio; }
    const juce::AudioBuffer<float>& getProcessedAudio() const { return processedAudio; }
    double getLoadedSampleRate() const { return sourceSampleRate; }
    juce::String getLoadedFileName() const { return sourceFileName; }

    // Breakpoint file management
    bool loadBreakpointFile(const juce::File& file);
    void clearBreakpoints();
    bool hasBreakpoints() const { return !breakpointData.empty(); }
    juce::String getBreakpointFeatureName() const { return currentFeatureName; }
    juce::StringArray getAvailableOutputs() const;

    // Breakpoint access and editing
    std::vector<std::pair<double, double>> getBreakpointsForDisplay(int outputIndex = 0) const;
    void addBreakpoint(int outputIndex, double time, double value);
    void updateBreakpoint(int outputIndex, size_t pointIndex, double time, double value);
    void removeBreakpoint(int outputIndex, size_t pointIndex);
    void sortBreakpoints(int outputIndex);
    int getNumOutputs() const { return static_cast<int>(breakpointData.size()); }

    // Processing
    void applyBreakpointsToAudio();
    void exportProcessedAudio(const juce::File& file);
    bool isProcessing() const { return processing.load(); }
    float getProcessingProgress() const { return processingProgress.load(); }

    // Decimation for performance
    void decimateBreakpoints(int outputIndex, int targetPoints);
    int getCurrentBreakpointCount(int outputIndex) const;

    juce::AudioProcessorValueTreeState params;

private:
    juce::AudioBuffer<float> sourceAudio;
    juce::AudioBuffer<float> processedAudio;
    double sourceSampleRate = 44100.0;
    juce::String sourceFileName;

    // Breakpoint storage: [outputIndex][points]
    std::vector<std::vector<std::pair<double, double>>> breakpointData;
    juce::String currentFeatureName;
    juce::StringArray outputNames;

    std::atomic<bool> processing{ false };
    std::atomic<float> processingProgress{ 0.0f };

    // Processing methods for different feature types
    void applyAmplitudeModification();
    void applyPanningModification();
    void applyADSRModification();

    float interpolateValue(const std::vector<std::pair<double, double>>& points, double time);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioBuilderProcessor)
};