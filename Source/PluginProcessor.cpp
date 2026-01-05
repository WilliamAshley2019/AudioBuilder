// ============================================================================
// AudioBuilder - PluginProcessor.cpp
// ============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"

AudioBuilderProcessor::AudioBuilderProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , params(*this, nullptr, "PARAMS", {
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"intensity", 1},
            "Intensity",
            juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
            1.0f
        ),
        std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"smoothing", 1},
            "Smoothing",
            true
        ),
        std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"ppqn", 1},
            "PPQN Resolution",
            juce::StringArray{"24", "48", "96", "192", "384", "480", "960"},
            6  // Default to 960
        ),
        std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"valueres", 1},
            "Value Resolution",
            juce::StringArray{"7-bit (MIDI CC)", "14-bit (NRPN)", "24-bit (Audio)", "32-bit (Float)"},
            1  // Default to 14-bit
        )
        })
{
    initializeTimeLattice();
}

AudioBuilderProcessor::~AudioBuilderProcessor() {}

bool AudioBuilderProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void AudioBuilderProcessor::prepareToPlay(double, int) {}
void AudioBuilderProcessor::releaseResources() {}

void AudioBuilderProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    // Pass-through - this is an offline editor
}

bool AudioBuilderProcessor::loadAudioFile(const juce::File& file) {
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader != nullptr) {
        sourceSampleRate = reader->sampleRate;
        int numChannels = static_cast<int>(reader->numChannels);
        int numSamples = static_cast<int>(reader->lengthInSamples);

        sourceAudio.setSize(numChannels, numSamples);
        reader->read(&sourceAudio, 0, numSamples, 0, true, true);
        sourceFileName = file.getFileNameWithoutExtension();

        processedAudio.makeCopyOf(sourceAudio);
        return true;
    }
    return false;
}

void AudioBuilderProcessor::clearLoadedAudio() {
    sourceAudio.setSize(0, 0);
    processedAudio.setSize(0, 0);
    sourceFileName = "";
}

bool AudioBuilderProcessor::loadBreakpointFile(const juce::File& file) {
    juce::FileInputStream stream(file);
    if (!stream.openedOk()) return false;

    juce::String content = stream.readEntireStreamAsString();
    auto lines = juce::StringArray::fromLines(content);

    breakpointData.clear();
    outputNames.clear();
    currentFeatureName = "";

    std::vector<std::pair<double, double>> currentOutput;
    juce::String currentOutputName;

    for (const auto& line : lines) {
        // Parse feature name from header
        if (line.startsWith("# Feature:")) {
            currentFeatureName = line.fromFirstOccurrenceOf("# Feature:", false, false).trim();
            continue;
        }

        // Parse output names
        if (line.startsWith("#") && !line.startsWith("# Feature:") &&
            !line.startsWith("# Source:") && !line.startsWith("# Sample Rate:") &&
            !line.startsWith("# Generated:") && !line.startsWith("# Format:") &&
            !line.startsWith("# Audio Deconstructor")) {

            // If we have pending output, save it
            if (!currentOutput.empty()) {
                breakpointData.push_back(currentOutput);
                outputNames.add(currentOutputName);
                currentOutput.clear();
            }

            currentOutputName = line.fromFirstOccurrenceOf("#", false, false).trim();
            continue;
        }

        // Skip other comments and empty lines
        if (line.startsWith("#") || line.trim().isEmpty()) continue;

        // Parse data line
        auto tokens = juce::StringArray::fromTokens(line, "\t ", "");
        if (tokens.size() >= 2) {
            double time = tokens[0].getDoubleValue();
            double value = tokens[1].getDoubleValue();
            currentOutput.emplace_back(time, value);
        }
    }

    // Add final output
    if (!currentOutput.empty()) {
        breakpointData.push_back(currentOutput);
        outputNames.add(currentOutputName);
    }

    return !breakpointData.empty();
}

void AudioBuilderProcessor::clearBreakpoints() {
    breakpointData.clear();
    outputNames.clear();
    currentFeatureName = "";
}

juce::StringArray AudioBuilderProcessor::getAvailableOutputs() const {
    return outputNames;
}

std::vector<std::pair<double, double>> AudioBuilderProcessor::getBreakpointsForDisplay(int outputIndex) const {
    if (outputIndex >= 0 && outputIndex < breakpointData.size()) {
        return breakpointData[outputIndex];
    }
    return {};
}

void AudioBuilderProcessor::addBreakpoint(int outputIndex, double time, double value) {
    if (outputIndex >= 0 && outputIndex < breakpointData.size()) {
        breakpointData[outputIndex].emplace_back(time, value);
        sortBreakpoints(outputIndex);
    }
}

void AudioBuilderProcessor::updateBreakpoint(int outputIndex, size_t pointIndex, double time, double value) {
    if (outputIndex >= 0 && outputIndex < breakpointData.size()) {
        auto& points = breakpointData[outputIndex];
        if (pointIndex < points.size()) {
            points[pointIndex] = { juce::jmax(0.0, time), value };
            sortBreakpoints(outputIndex);
        }
    }
}

void AudioBuilderProcessor::removeBreakpoint(int outputIndex, size_t pointIndex) {
    if (outputIndex >= 0 && outputIndex < breakpointData.size()) {
        auto& points = breakpointData[outputIndex];
        if (pointIndex < points.size()) {
            points.erase(points.begin() + pointIndex);
        }
    }
}

void AudioBuilderProcessor::sortBreakpoints(int outputIndex) {
    if (outputIndex >= 0 && outputIndex < breakpointData.size()) {
        std::sort(breakpointData[outputIndex].begin(), breakpointData[outputIndex].end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
    }
}

void AudioBuilderProcessor::decimateBreakpoints(int outputIndex, int targetPoints) {
    if (outputIndex < 0 || outputIndex >= breakpointData.size()) return;

    auto& points = breakpointData[outputIndex];
    if (points.size() <= targetPoints) return;

    std::vector<std::pair<double, double>> decimated;
    decimated.reserve(targetPoints);

    int step = points.size() / targetPoints;
    for (int i = 0; i < points.size(); i += step) {
        if (decimated.size() < targetPoints) {
            decimated.push_back(points[i]);
        }
    }

    // Always include last point
    if (!points.empty() && decimated.back().first != points.back().first) {
        decimated.push_back(points.back());
    }

    points = decimated;
}

int AudioBuilderProcessor::getCurrentBreakpointCount(int outputIndex) const {
    if (outputIndex >= 0 && outputIndex < breakpointData.size()) {
        return static_cast<int>(breakpointData[outputIndex].size());
    }
    return 0;
}

float AudioBuilderProcessor::interpolateValue(const std::vector<std::pair<double, double>>& points, double time) {
    if (points.empty()) return 0.0f;
    if (points.size() == 1) return static_cast<float>(points[0].second);
    if (time <= points.front().first) return static_cast<float>(points.front().second);
    if (time >= points.back().first) return static_cast<float>(points.back().second);

    // Linear interpolation between points
    for (size_t i = 0; i < points.size() - 1; ++i) {
        if (time >= points[i].first && time <= points[i + 1].first) {
            double t1 = points[i].first;
            double t2 = points[i + 1].first;
            double v1 = points[i].second;
            double v2 = points[i + 1].second;

            double ratio = (time - t1) / (t2 - t1);
            return static_cast<float>(v1 + ratio * (v2 - v1));
        }
    }

    return static_cast<float>(points.back().second);
}

void AudioBuilderProcessor::applyBreakpointsToAudio() {
    if (!hasLoadedAudio() || !hasBreakpoints()) return;

    processing = true;
    processingProgress = 0.0f;

    processedAudio.makeCopyOf(sourceAudio);
    float intensity = params.getRawParameterValue("intensity")->load();

    // Determine processing type based on feature name
    if (currentFeatureName.containsIgnoreCase("Amplitude") ||
        currentFeatureName.containsIgnoreCase("ADSR")) {
        applyAmplitudeModification();
    }
    else if (currentFeatureName.containsIgnoreCase("Panning") ||
        currentFeatureName.containsIgnoreCase("Pan")) {
        applyPanningModification();
    }
    else {
        // Default: apply as amplitude envelope
        applyAmplitudeModification();
    }

    processing = false;
    processingProgress = 1.0f;
}

void AudioBuilderProcessor::applyAmplitudeModification() {
    if (breakpointData.empty()) return;

    const auto& envelope = breakpointData[0]; // Use first output (RMS)
    float intensity = params.getRawParameterValue("intensity")->load();

    int numChannels = processedAudio.getNumChannels();
    int numSamples = processedAudio.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch) {
        float* channelData = processedAudio.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i) {
            double time = i / sourceSampleRate;
            float envelopeValue = interpolateValue(envelope, time);

            // Apply with intensity control (1.0 = full effect)
            float gain = 1.0f + (envelopeValue - 1.0f) * intensity;
            channelData[i] *= gain;

            if (i % 10000 == 0) {
                processingProgress = static_cast<float>(i) / numSamples;
            }
        }
    }
}

void AudioBuilderProcessor::applyPanningModification() {
    if (breakpointData.empty() || processedAudio.getNumChannels() < 2) return;

    const auto& panCurve = breakpointData[0]; // Pan position
    float intensity = params.getRawParameterValue("intensity")->load();

    float* leftData = processedAudio.getWritePointer(0);
    float* rightData = processedAudio.getWritePointer(1);
    int numSamples = processedAudio.getNumSamples();

    for (int i = 0; i < numSamples; ++i) {
        double time = i / sourceSampleRate;
        float panValue = interpolateValue(panCurve, time); // -1 to 1

        // Apply intensity
        panValue *= intensity;
        panValue = juce::jlimit(-1.0f, 1.0f, panValue);

        // Equal power panning
        float angle = (panValue + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        float leftGain = std::cos(angle);
        float rightGain = std::sin(angle);

        leftData[i] *= leftGain;
        rightData[i] *= rightGain;

        if (i % 10000 == 0) {
            processingProgress = static_cast<float>(i) / numSamples;
        }
    }
}

void AudioBuilderProcessor::applyADSRModification() {
    applyAmplitudeModification(); // ADSR uses same logic as amplitude
}

void AudioBuilderProcessor::exportProcessedAudio(const juce::File& file) {
    if (processedAudio.getNumSamples() == 0) return;

    juce::WavAudioFormat wavFormat;

    // Create output stream
    std::unique_ptr<juce::FileOutputStream> fileStream(file.createOutputStream());

    if (fileStream) {
        // Create writer with parameters
        if (auto writer = wavFormat.createWriterFor(fileStream.release(), // Note: release() transfers ownership
            sourceSampleRate,
            processedAudio.getNumChannels(),
            24,
            juce::StringPairArray(), // empty metadata
            0)) {
            writer->writeFromAudioSampleBuffer(processedAudio, 0, processedAudio.getNumSamples());
            delete writer; // Clean up the writer
        }
    }
}

void AudioBuilderProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = params.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AudioBuilderProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr && xmlState->hasTagName(params.state.getType())) {
        params.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessorEditor* AudioBuilderProcessor::createEditor() {
    return new AudioBuilderEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new AudioBuilderProcessor();
}

juce::AudioBuffer<float> AudioBuilderProcessor::performEditOperation(EditOperation op,
    const std::vector<double>& operationParams) {

    // Return empty buffer for now - implement actual operations later
    juce::AudioBuffer<float> result;

    switch (op) {
    case EditOperation::Trim:
    case EditOperation::Cut:
    case EditOperation::Split:
    case EditOperation::Nudge:
    case EditOperation::TimeStretch:
    case EditOperation::Quantize:
    case EditOperation::Humanize:
    case EditOperation::DetectBeats:
    case EditOperation::SnapToGrid:
    case EditOperation::Crossfade:
        // Placeholder - implement these operations
        break;
    case EditOperation::None:
    default:
        break;
    }

    return result;
}

void AudioBuilderProcessor::initializeTimeLattice() {
    timeLattice = std::make_unique<AudioTimeLattice>(currentPPQN, sourceSampleRate);
}

void AudioBuilderProcessor::setTimeGridPPQN(int ppqn) {
    currentPPQN = ppqn;
    if (timeLattice) {
        timeLattice->setPPQN(ppqn);
    }
}

int AudioBuilderProcessor::getTimeGridPPQN() const {
    return currentPPQN;
}

void AudioBuilderProcessor::setTimeGridResolution(ValueResolution resolution) {
    currentResolution = resolution;
}

ValueResolution AudioBuilderProcessor::getTimeGridResolution() const {
    return currentResolution;
}