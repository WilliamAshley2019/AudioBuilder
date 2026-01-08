// ============================================================================
// Audio Workshop - PluginProcessor.cpp
// ============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"

AudioWorkshopProcessor::AudioWorkshopProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , params(*this, nullptr, "PARAMS", {
    // Extraction parameters (from AudioDeconstructor)
    std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"windowSize", 1},
        "Window Size (ms)",
        juce::NormalisableRange<float>(1.0f, 100.0f, 0.1f),
        15.0f
    ),
    std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"hopSize", 1},
        "Hop Size (%)",
        juce::NormalisableRange<float>(10.0f, 90.0f, 1.0f),
        50.0f
    ),
    std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"normalize", 1},
        "Normalize Output",
        true
    ),
        // Application parameters (from AudioBuilder)
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
        )
        })
{
    initializeExtractors();
    initializeTimeLattice();
}

AudioWorkshopProcessor::~AudioWorkshopProcessor() {}

bool AudioWorkshopProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void AudioWorkshopProcessor::prepareToPlay(double sampleRate, int) {
    if (timeLattice) {
        timeLattice->setSampleRate(sampleRate);
    }
}

void AudioWorkshopProcessor::releaseResources() {}

void AudioWorkshopProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    // Pass-through - this is an offline editor
}

// ============================================================================
// AUDIO FILE MANAGEMENT
// ============================================================================

bool AudioWorkshopProcessor::loadSourceAudio(const juce::File& file) {
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

        return true;
    }
    return false;
}

void AudioWorkshopProcessor::clearSourceAudio() {
    sourceAudio.setSize(0, 0);
    sourceFileName = "";
}

bool AudioWorkshopProcessor::loadTargetAudio(const juce::File& file) {
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader != nullptr) {
        targetSampleRate = reader->sampleRate;
        int numChannels = static_cast<int>(reader->numChannels);
        int numSamples = static_cast<int>(reader->lengthInSamples);

        targetAudio.setSize(numChannels, numSamples);
        reader->read(&targetAudio, 0, numSamples, 0, true, true);
        targetFileName = file.getFileNameWithoutExtension();

        processedAudio.makeCopyOf(targetAudio);
        return true;
    }
    return false;
}

void AudioWorkshopProcessor::clearTargetAudio() {
    targetAudio.setSize(0, 0);
    processedAudio.setSize(0, 0);
    targetFileName = "";
}

// ============================================================================
// FEATURE EXTRACTION
// ============================================================================

void AudioWorkshopProcessor::initializeExtractors() {
    extractors["Amplitude"] = FeatureExtractorFactory::createExtractor("Amplitude");
    extractors["Panning"] = FeatureExtractorFactory::createExtractor("Panning");
    extractors["Spectral"] = FeatureExtractorFactory::createExtractor("Spectral");
    extractors["Pitch"] = FeatureExtractorFactory::createExtractor("Pitch");
    extractors["Transients"] = FeatureExtractorFactory::createExtractor("Transients");
    extractors["ADSR Envelope"] = FeatureExtractorFactory::createExtractor("ADSR Envelope");
}

void AudioWorkshopProcessor::extractFeature(const juce::String& featureName, int channel) {
    auto it = extractors.find(featureName);
    if (it == extractors.end() || !hasSourceAudio()) return;

    isAnalyzing = true;
    auto& extractor = it->second;

    // Update settings from parameters
    extractor->settings.windowSizeMs = params.getRawParameterValue("windowSize")->load();
    extractor->settings.hopSizePct = params.getRawParameterValue("hopSize")->load();
    extractor->settings.normalizeOutput = params.getRawParameterValue("normalize")->load() > 0.5f;

    int channelToUse = juce::jlimit(0, sourceAudio.getNumChannels() - 1,
        channel < 0 ? 0 : channel);

    auto results = extractor->extract(sourceAudio, sourceSampleRate, channelToUse);
    featureBreakpoints[featureName] = results;

    isAnalyzing = false;
}

void AudioWorkshopProcessor::extractAllFeatures() {
    for (const auto& [featureName, extractor] : extractors) {
        extractFeature(featureName, 0);
    }
}

void AudioWorkshopProcessor::extractADSRFromAmplitude() {
    if (!hasSourceAudio()) return;

    if (!isFeatureExtracted("Amplitude")) {
        extractFeature("Amplitude", 0);
    }

    auto amplitudeBreakpoints = getBreakpointsForDisplay("Amplitude", 0);
    if (amplitudeBreakpoints.empty()) return;

    auto adsrExtractor = FeatureExtractorFactory::createExtractor("ADSR Envelope");
    if (!adsrExtractor) return;

    auto adsrExtractorPtr = dynamic_cast<ADSREnvelopeExtractor*>(adsrExtractor.get());
    if (adsrExtractorPtr) {
        auto adsrResults = adsrExtractorPtr->extractFromAmplitude(amplitudeBreakpoints, sourceSampleRate);
        featureBreakpoints["ADSR Envelope"] = adsrResults;
        extractors["ADSR Envelope"] = std::move(adsrExtractor);
    }
}

bool AudioWorkshopProcessor::isFeatureExtracted(const juce::String& featureName) const {
    return featureBreakpoints.find(featureName) != featureBreakpoints.end();
}

juce::StringArray AudioWorkshopProcessor::getExtractedFeatures() const {
    juce::StringArray features;
    for (const auto& [name, _] : featureBreakpoints) {
        features.add(name);
    }
    return features;
}

juce::StringArray AudioWorkshopProcessor::getAvailableFeatures() const {
    juce::StringArray features;
    for (const auto& [name, _] : extractors) {
        features.add(name);
    }
    return features;
}

juce::Colour AudioWorkshopProcessor::getFeatureColour(const juce::String& featureName) const {
    auto it = extractors.find(featureName);
    return it != extractors.end() ? it->second->getColor() : juce::Colours::white;
}

int AudioWorkshopProcessor::getNumOutputsForFeature(const juce::String& featureName) const {
    auto it = extractors.find(featureName);
    return it != extractors.end() ? it->second->getNumOutputs() : 0;
}

juce::String AudioWorkshopProcessor::getOutputName(const juce::String& featureName,
    int outputIndex) const {
    auto it = extractors.find(featureName);
    return it != extractors.end() ? it->second->getOutputName(outputIndex) : "";
}

// ============================================================================
// BREAKPOINT MANAGEMENT
// ============================================================================

std::vector<std::pair<double, double>> AudioWorkshopProcessor::getBreakpointsForDisplay(
    const juce::String& featureName, int outputIndex) const {

    auto it = featureBreakpoints.find(featureName);
    if (it != featureBreakpoints.end() && outputIndex < it->second.size()) {
        return it->second[outputIndex];
    }
    return {};
}

void AudioWorkshopProcessor::addBreakpoint(const juce::String& featureName,
    int outputIndex, double time, double value) {

    auto it = featureBreakpoints.find(featureName);
    if (it != featureBreakpoints.end() && outputIndex < it->second.size()) {
        it->second[outputIndex].emplace_back(time, value);
        sortBreakpoints(featureName, outputIndex);
    }
}

void AudioWorkshopProcessor::updateBreakpoint(const juce::String& featureName,
    int outputIndex, size_t pointIndex, double time, double value) {

    auto it = featureBreakpoints.find(featureName);
    if (it != featureBreakpoints.end() && outputIndex < it->second.size()) {
        auto& points = it->second[outputIndex];
        if (pointIndex < points.size()) {
            points[pointIndex] = { juce::jmax(0.0, time), value };
            sortBreakpoints(featureName, outputIndex);
        }
    }
}

void AudioWorkshopProcessor::removeBreakpoint(const juce::String& featureName,
    int outputIndex, size_t pointIndex) {

    auto it = featureBreakpoints.find(featureName);
    if (it != featureBreakpoints.end() && outputIndex < it->second.size()) {
        auto& points = it->second[outputIndex];
        if (pointIndex < points.size()) {
            points.erase(points.begin() + pointIndex);
        }
    }
}

void AudioWorkshopProcessor::sortBreakpoints(const juce::String& featureName, int outputIndex) {
    auto it = featureBreakpoints.find(featureName);
    if (it != featureBreakpoints.end() && outputIndex < it->second.size()) {
        std::sort(it->second[outputIndex].begin(), it->second[outputIndex].end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
    }
}

void AudioWorkshopProcessor::decimateBreakpoints(const juce::String& featureName,
    int outputIndex, int targetPoints) {

    auto it = featureBreakpoints.find(featureName);
    if (it == featureBreakpoints.end() || outputIndex >= it->second.size()) return;

    auto& points = it->second[outputIndex];
    if (points.size() <= targetPoints) return;

    std::vector<std::pair<double, double>> decimated;
    decimated.reserve(targetPoints);

    int step = points.size() / targetPoints;
    for (size_t i = 0; i < points.size(); i += step) {
        if (decimated.size() < targetPoints) {
            decimated.push_back(points[i]);
        }
    }

    if (!points.empty() && decimated.back().first != points.back().first) {
        decimated.push_back(points.back());
    }

    points = decimated;
}

int AudioWorkshopProcessor::getCurrentBreakpointCount(const juce::String& featureName,
    int outputIndex) const {

    auto it = featureBreakpoints.find(featureName);
    if (it != featureBreakpoints.end() && outputIndex < it->second.size()) {
        return static_cast<int>(it->second[outputIndex].size());
    }
    return 0;
}

// ============================================================================
// BREAKPOINT FILE I/O
// ============================================================================

bool AudioWorkshopProcessor::loadBreakpointFile(const juce::File& file) {
    juce::FileInputStream stream(file);
    if (!stream.openedOk()) return false;

    juce::String content = stream.readEntireStreamAsString();
    auto lines = juce::StringArray::fromLines(content);

    juce::String currentFeatureName;
    std::vector<std::pair<double, double>> currentOutput;
    juce::String currentOutputName;

    for (const auto& line : lines) {
        if (line.startsWith("# Feature:")) {
            currentFeatureName = line.fromFirstOccurrenceOf("# Feature:", false, false).trim();
            continue;
        }

        if (line.startsWith("#") && !line.startsWith("# Feature:") &&
            !line.startsWith("# Source:") && !line.startsWith("# Sample Rate:") &&
            !line.startsWith("# Generated:") && !line.startsWith("# Format:") &&
            !line.startsWith("# Audio")) {

            if (!currentOutput.empty() && !currentFeatureName.isEmpty()) {
                if (featureBreakpoints.find(currentFeatureName) == featureBreakpoints.end()) {
                    auto extractorIt = extractors.find(currentFeatureName);
                    if (extractorIt != extractors.end()) {
                        int numOutputs = extractorIt->second->getNumOutputs();
                        featureBreakpoints[currentFeatureName] =
                            std::vector<std::vector<std::pair<double, double>>>(numOutputs);
                    }
                }

                if (featureBreakpoints.find(currentFeatureName) != featureBreakpoints.end()) {
                    auto& outputs = featureBreakpoints[currentFeatureName];
                    outputs.push_back(currentOutput);
                }

                currentOutput.clear();
            }

            currentOutputName = line.fromFirstOccurrenceOf("#", false, false).trim();
            continue;
        }

        if (line.startsWith("#") || line.trim().isEmpty()) continue;

        auto tokens = juce::StringArray::fromTokens(line, "\t ", "");
        if (tokens.size() >= 2) {
            double time = tokens[0].getDoubleValue();
            double value = tokens[1].getDoubleValue();
            currentOutput.emplace_back(time, value);
        }
    }

    if (!currentOutput.empty() && !currentFeatureName.isEmpty()) {
        if (featureBreakpoints.find(currentFeatureName) == featureBreakpoints.end()) {
            auto extractorIt = extractors.find(currentFeatureName);
            if (extractorIt != extractors.end()) {
                int numOutputs = extractorIt->second->getNumOutputs();
                featureBreakpoints[currentFeatureName] =
                    std::vector<std::vector<std::pair<double, double>>>(numOutputs);
            }
        }

        if (featureBreakpoints.find(currentFeatureName) != featureBreakpoints.end()) {
            featureBreakpoints[currentFeatureName].push_back(currentOutput);
        }
    }

    return !featureBreakpoints.empty();
}

void AudioWorkshopProcessor::saveBreakpoints(const juce::String& featureName,
    const juce::File& file) {

    auto it = featureBreakpoints.find(featureName);
    if (it == featureBreakpoints.end()) return;

    juce::FileOutputStream stream(file);
    if (stream.openedOk()) {
        stream.writeText("# Audio Workshop Breakpoint File\n", false, false, "\n");
        stream.writeText("# Feature: " + featureName + "\n", false, false, "\n");
        stream.writeText("# Source: " + sourceFileName + "\n", false, false, "\n");
        stream.writeText("# Sample Rate: " + juce::String(sourceSampleRate) + " Hz\n",
            false, false, "\n");
        stream.writeText("# Generated: " + juce::Time::getCurrentTime().toString(true, true) +
            "\n", false, false, "\n");
        stream.writeText("# Format: time(seconds) value\n\n", false, false, "\n");

        const auto& outputs = it->second;
        for (size_t i = 0; i < outputs.size(); ++i) {
            auto extractorIt = extractors.find(featureName);
            juce::String outputName = extractorIt != extractors.end() ?
                extractorIt->second->getOutputName(static_cast<int>(i)) :
                "Output " + juce::String(i + 1);

            stream.writeText("# " + outputName + "\n", false, false, "\n");

            for (const auto& [time, value] : outputs[i]) {
                stream.writeText(juce::String(time, 6) + "\t" +
                    juce::String(value, 6) + "\n", false, false, "\n");
            }
            stream.writeText("\n", false, false, "\n");
        }
    }
}

void AudioWorkshopProcessor::saveAllBreakpoints(const juce::File& directory) {
    for (const auto& [featureName, _] : featureBreakpoints) {
        juce::File file = directory.getChildFile(sourceFileName + "_" +
            featureName + ".txt");
        saveBreakpoints(featureName, file);
    }
}

void AudioWorkshopProcessor::clearBreakpoints() {
    featureBreakpoints.clear();
}

bool AudioWorkshopProcessor::hasBreakpoints() const {
    return !featureBreakpoints.empty();
}

// ============================================================================
// TIME LATTICE & QUANTIZATION
// ============================================================================

void AudioWorkshopProcessor::initializeTimeLattice() {
    timeLattice = std::make_unique<AudioTimeLattice>(currentPPQN, sourceSampleRate);
    timeLattice->setTempo(120.0);
}

void AudioWorkshopProcessor::setTimeGridPPQN(int ppqn) {
    currentPPQN = ppqn;
    if (timeLattice) {
        timeLattice->setPPQN(ppqn);
    }
}

void AudioWorkshopProcessor::setTimeGridResolution(ValueResolution resolution) {
    currentResolution = resolution;
}

void AudioWorkshopProcessor::quantizeBreakpointsToGrid(const juce::String& featureName,
    int outputIndex) {

    auto it = featureBreakpoints.find(featureName);
    if (it == featureBreakpoints.end() || outputIndex >= it->second.size()) return;

    if (timeLattice) {
        auto& points = it->second[outputIndex];
        auto quantized = timeLattice->quantizeBreakpoints(points, currentResolution, true);
        points = quantized;
    }
}

// ============================================================================
// AUDIO PROCESSING & APPLICATION
// ============================================================================

float AudioWorkshopProcessor::interpolateValue(const std::vector<std::pair<double, double>>& points,
    double time) {

    if (points.empty()) return 0.0f;
    if (points.size() == 1) return static_cast<float>(points[0].second);
    if (time <= points.front().first) return static_cast<float>(points.front().second);
    if (time >= points.back().first) return static_cast<float>(points.back().second);

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

void AudioWorkshopProcessor::applyBreakpointsToTarget() {
    if (!hasTargetAudio() || !hasBreakpoints()) return;

    processing = true;
    processingProgress = 0.0f;

    processedAudio.makeCopyOf(targetAudio);

    // Determine which feature to apply (use first extracted feature)
    auto features = getExtractedFeatures();
    if (features.isEmpty()) {
        processing = false;
        return;
    }

    juce::String featureName = features[0];

    if (featureName.containsIgnoreCase("Amplitude") ||
        featureName.containsIgnoreCase("ADSR")) {
        applyAmplitudeModification();
    }
    else if (featureName.containsIgnoreCase("Panning") ||
        featureName.containsIgnoreCase("Pan")) {
        applyPanningModification();
    }
    else {
        applyAmplitudeModification();
    }

    processing = false;
    processingProgress = 1.0f;
}

void AudioWorkshopProcessor::applyAmplitudeModification() {
    auto features = getExtractedFeatures();
    if (features.isEmpty()) return;

    auto it = featureBreakpoints.find(features[0]);
    if (it == featureBreakpoints.end() || it->second.empty()) return;

    const auto& envelope = it->second[0];
    float intensity = params.getRawParameterValue("intensity")->load();

    int numChannels = processedAudio.getNumChannels();
    int numSamples = processedAudio.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch) {
        float* channelData = processedAudio.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i) {
            double time = i / targetSampleRate;
            float envelopeValue = interpolateValue(envelope, time);

            float gain = 1.0f + (envelopeValue - 1.0f) * intensity;
            channelData[i] *= gain;

            if (i % 10000 == 0) {
                processingProgress = static_cast<float>(i) / numSamples;
            }
        }
    }
}

void AudioWorkshopProcessor::applyPanningModification() {
    auto features = getExtractedFeatures();
    if (features.isEmpty() || processedAudio.getNumChannels() < 2) return;

    auto it = featureBreakpoints.find(features[0]);
    if (it == featureBreakpoints.end() || it->second.empty()) return;

    const auto& panCurve = it->second[0];
    float intensity = params.getRawParameterValue("intensity")->load();

    float* leftData = processedAudio.getWritePointer(0);
    float* rightData = processedAudio.getWritePointer(1);
    int numSamples = processedAudio.getNumSamples();

    for (int i = 0; i < numSamples; ++i) {
        double time = i / targetSampleRate;
        float panValue = interpolateValue(panCurve, time);

        panValue *= intensity;
        panValue = juce::jlimit(-1.0f, 1.0f, panValue);

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

void AudioWorkshopProcessor::applyADSRModification() {
    applyAmplitudeModification();
}

void AudioWorkshopProcessor::exportProcessedAudio(const juce::File& file) {
    if (processedAudio.getNumSamples() == 0) return;

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::FileOutputStream> fileStream(file.createOutputStream());

    if (fileStream) {
        if (auto writer = wavFormat.createWriterFor(fileStream.release(),
            targetSampleRate,
            processedAudio.getNumChannels(),
            24,
            juce::StringPairArray(),
            0)) {
            writer->writeFromAudioSampleBuffer(processedAudio, 0, processedAudio.getNumSamples());
            delete writer;
        }
    }
}

// ============================================================================
// ADVANCED EDITING OPERATIONS
// ============================================================================

juce::AudioBuffer<float> AudioWorkshopProcessor::performEditOperation(EditOperation op,
    const std::vector<double>& params) {

    juce::AudioBuffer<float> result;

    if (!timeLattice) return result;

    switch (op) {
    case EditOperation::RemoveSilence:
        if (hasTargetAudio()) {
            result = removeSilence(targetAudio, params.empty() ? -40.0 : params[0]);
        }
        break;

    case EditOperation::SplitByBeats: {
        auto segments = splitByBeats(targetAudio);
        if (!segments.empty()) {
            result = segments[0];
        }
        break;
    }

    case EditOperation::IsolateTransients:
        if (hasTargetAudio()) {
            result = isolateTransients(targetAudio, params.empty() ? 0.5 : params[0]);
        }
        break;

    default:
        // Use time lattice operations for other cases
        if (hasTargetAudio()) {
            result = targetAudio;
        }
        break;
    }

    return result;
}

juce::AudioBuffer<float> AudioWorkshopProcessor::removeSilence(const juce::AudioBuffer<float>& input,
    double thresholddB) {

    // Use amplitude extractor to find silence
    AmplitudeExtractor ampExtractor;
    auto results = ampExtractor.extract(input, targetSampleRate, 0);

    if (results.empty() || results[0].empty()) return input;

    float threshold = std::pow(10.0f, static_cast<float>(thresholddB) / 20.0f);

    // Find non-silent regions
    std::vector<std::pair<int, int>> nonSilentRegions;
    int regionStart = -1;

    for (const auto& [time, value] : results[0]) {
        int sample = static_cast<int>(time * targetSampleRate);

        if (value > threshold && regionStart < 0) {
            regionStart = sample;
        }
        else if (value <= threshold && regionStart >= 0) {
            nonSilentRegions.push_back({ regionStart, sample });
            regionStart = -1;
        }
    }

    if (regionStart >= 0) {
        nonSilentRegions.push_back({ regionStart, input.getNumSamples() });
    }

    // Concatenate non-silent regions
    int totalSamples = 0;
    for (const auto& [start, end] : nonSilentRegions) {
        totalSamples += (end - start);
    }

    juce::AudioBuffer<float> output(input.getNumChannels(), totalSamples);
    int writePos = 0;

    for (const auto& [start, end] : nonSilentRegions) {
        int length = end - start;
        for (int ch = 0; ch < input.getNumChannels(); ++ch) {
            output.copyFrom(ch, writePos, input, ch, start, length);
        }
        writePos += length;
    }

    return output;
}

std::vector<juce::AudioBuffer<float>> AudioWorkshopProcessor::splitByBeats(
    const juce::AudioBuffer<float>& input) {

    if (!timeLattice) return {};

    auto beats = timeLattice->detectBeats(input);

    std::vector<double> splitTimes;
    for (const auto& beat : beats) {
        splitTimes.push_back(beat.timeInSeconds);
    }

    return timeLattice->split(input, splitTimes);
}

juce::AudioBuffer<float> AudioWorkshopProcessor::isolateTransients(
    const juce::AudioBuffer<float>& input, double sensitivity) {

    auto transients = timeLattice->detectTransients(input, sensitivity);

    // Create output with only transient regions
    juce::AudioBuffer<float> output = input;
    output.clear();

    float windowMs = 50.0f; // 50ms around each transient
    int windowSamples = static_cast<int>(windowMs * 0.001 * targetSampleRate);

    for (double transientTime : transients) {
        int centerSample = static_cast<int>(transientTime * targetSampleRate);
        int start = juce::jmax(0, centerSample - windowSamples / 2);
        int end = juce::jmin(input.getNumSamples(), centerSample + windowSamples / 2);

        for (int ch = 0; ch < input.getNumChannels(); ++ch) {
            output.copyFrom(ch, start, input, ch, start, end - start);
        }
    }

    return output;
}

// ============================================================================
// STATE MANAGEMENT
// ============================================================================

void AudioWorkshopProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = params.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AudioWorkshopProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr && xmlState->hasTagName(params.state.getType())) {
        params.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessorEditor* AudioWorkshopProcessor::createEditor() {
    return new AudioWorkshopEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new AudioWorkshopProcessor();
}