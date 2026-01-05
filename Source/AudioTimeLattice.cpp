// ============================================================================
// AudioTimeLattice.cpp
// Implementation of universal time-grid system
// ============================================================================
#include "AudioTimeLattice.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

// ============================================================================
// MusicalTime Implementation
// ============================================================================

std::string MusicalTime::toString() const {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(3) << bars << ":"
        << std::setfill('0') << std::setw(2) << beats << ":"
        << std::setfill('0') << std::setw(3) << ticks;
    return oss.str();
}

MusicalTime MusicalTime::fromString(const std::string& str) {
    MusicalTime mt;
    std::istringstream iss(str);
    char delim;
    iss >> mt.bars >> delim >> mt.beats >> delim >> mt.ticks;
    return mt;
}

// ============================================================================
// AudioTimeLattice Constructor
// ============================================================================

AudioTimeLattice::AudioTimeLattice(int ppqn, double sampleRate)
    : ppqn(ppqn), sampleRate(sampleRate) {
    // Add default tempo
    tempoMap.push_back({ 0.0, 120.0, 4, 4 });
}

// ============================================================================
// Configuration
// ============================================================================

void AudioTimeLattice::setPPQN(int newPPQN) {
    ppqn = juce::jmax(24, newPPQN); // Minimum 24 PPQN
}

void AudioTimeLattice::setSampleRate(double rate) {
    sampleRate = juce::jmax(1.0, rate);
}

void AudioTimeLattice::setTempo(double bpm, double timeInSeconds) {
    clearTempoMap();
    tempoMap.push_back({ timeInSeconds, bpm, 4, 4 });
}

void AudioTimeLattice::addTempoChange(const TempoEvent& tempo) {
    tempoMap.push_back(tempo);
    std::sort(tempoMap.begin(), tempoMap.end(),
        [](const TempoEvent& a, const TempoEvent& b) {
            return a.timeInSeconds < b.timeInSeconds;
        });
}

void AudioTimeLattice::clearTempoMap() {
    tempoMap.clear();
}

// ============================================================================
// Time Domain Conversions
// ============================================================================

double AudioTimeLattice::convert(double value, TimeDomain from, TimeDomain to) {
    double seconds = toSeconds(value, from);
    return fromSeconds(seconds, to);
}

double AudioTimeLattice::toSeconds(double value, TimeDomain domain) {
    switch (domain) {
    case TimeDomain::AudioSamples:
        return value / sampleRate;

    case TimeDomain::Seconds:
        return value;

    case TimeDomain::MusicalTicks: {
        TempoEvent tempo = getCurrentTempo(0);
        double beatLength = 60.0 / tempo.bpm;
        double tickLength = beatLength / ppqn;
        return value * tickLength;
    }

    case TimeDomain::BarsBeatsTicks: {
        int encoded = static_cast<int>(value);
        int bars = encoded / 1000000;
        int beats = (encoded % 1000000) / 1000;
        int ticks = encoded % 1000;
        MusicalTime mt{ bars, beats, ticks, 0 };
        return musicalToSeconds(mt);
    }

    case TimeDomain::SMPTEFrames:
        return value / 30.0; // 30 fps default
    }
    return 0.0;
}

double AudioTimeLattice::fromSeconds(double seconds, TimeDomain domain) {
    switch (domain) {
    case TimeDomain::AudioSamples:
        return seconds * sampleRate;

    case TimeDomain::Seconds:
        return seconds;

    case TimeDomain::MusicalTicks: {
        TempoEvent tempo = getCurrentTempo(seconds);
        double beatLength = 60.0 / tempo.bpm;
        return seconds / (beatLength / ppqn);
    }

    case TimeDomain::BarsBeatsTicks: {
        MusicalTime mt = secondsToMusical(seconds);
        return mt.bars * 1000000.0 + mt.beats * 1000.0 + mt.ticks;
    }

    case TimeDomain::SMPTEFrames:
        return seconds * 30.0;
    }
    return 0.0;
}

MusicalTime AudioTimeLattice::secondsToMusical(double seconds) {
    TempoEvent tempo = getCurrentTempo(seconds);

    double quarterLength = 60.0 / tempo.bpm;
    double beatLength = quarterLength * 4.0 / tempo.lowerTimeSig;
    double barLength = tempo.upperTimeSig * beatLength;

    // Add epsilon for floating-point precision
    const double eps = 1.0 + std::max(1.0, seconds) *
        std::numeric_limits<double>::epsilon();

    int bars = static_cast<int>(std::floor(seconds * eps / barLength));
    seconds -= bars * barLength;

    int beats = static_cast<int>(std::floor(seconds * eps / beatLength));
    seconds -= beats * beatLength;

    double ticksFractional = seconds * (ppqn / beatLength);
    int ticks = static_cast<int>(ticksFractional);
    double remainder = ticksFractional - ticks;

    return { bars + 1, beats + 1, ticks, remainder };
}

double AudioTimeLattice::musicalToSeconds(const MusicalTime& mt) {
    TempoEvent tempo = getCurrentTempo(0);

    double quarterLength = 60.0 / tempo.bpm;
    double beatLength = quarterLength * 4.0 / tempo.lowerTimeSig;

    double totalBeats = (mt.bars - 1) * tempo.upperTimeSig +
        (mt.beats - 1) +
        (mt.ticks + mt.remainder) / static_cast<double>(ppqn);

    return totalBeats * beatLength;
}

int AudioTimeLattice::secondsToSamples(double seconds) {
    return static_cast<int>(seconds * sampleRate + 0.5);
}

double AudioTimeLattice::samplesToSeconds(int samples) {
    return samples / sampleRate;
}

// ============================================================================
// Grid Generation
// ============================================================================

std::vector<double> AudioTimeLattice::generatePPQNGrid(double startSeconds, double endSeconds) {
    std::vector<double> grid;

    TempoEvent tempo = getCurrentTempo(startSeconds);
    double beatLength = 60.0 / tempo.bpm;
    double tickLength = beatLength / ppqn;

    int startTick = static_cast<int>(startSeconds / tickLength);
    int endTick = static_cast<int>(endSeconds / tickLength);

    for (int tick = startTick; tick <= endTick; ++tick) {
        double time = tick * tickLength;
        if (time >= startSeconds && time <= endSeconds) {
            grid.push_back(time);
        }
    }

    return grid;
}

std::vector<MusicalTime> AudioTimeLattice::generateMusicalGrid(double startSeconds, double endSeconds) {
    std::vector<MusicalTime> grid;
    auto timeGrid = generatePPQNGrid(startSeconds, endSeconds);

    for (double time : timeGrid) {
        grid.push_back(secondsToMusical(time));
    }

    return grid;
}

std::vector<double> AudioTimeLattice::generateBeatGrid(double startSeconds, double endSeconds) {
    std::vector<double> grid;

    TempoEvent tempo = getCurrentTempo(startSeconds);
    double beatLength = 60.0 / tempo.bpm;

    int startBeat = static_cast<int>(startSeconds / beatLength);
    int endBeat = static_cast<int>(endSeconds / beatLength);

    for (int beat = startBeat; beat <= endBeat; ++beat) {
        double time = beat * beatLength;
        if (time >= startSeconds && time <= endSeconds) {
            grid.push_back(time);
        }
    }

    return grid;
}

std::vector<double> AudioTimeLattice::generateBarGrid(double startSeconds, double endSeconds) {
    std::vector<double> grid;

    TempoEvent tempo = getCurrentTempo(startSeconds);
    double beatLength = 60.0 / tempo.bpm;
    double barLength = tempo.upperTimeSig * beatLength;

    int startBar = static_cast<int>(startSeconds / barLength);
    int endBar = static_cast<int>(endSeconds / barLength);

    for (int bar = startBar; bar <= endBar; ++bar) {
        double time = bar * barLength;
        if (time >= startSeconds && time <= endSeconds) {
            grid.push_back(time);
        }
    }

    return grid;
}

// ============================================================================
// Quantization
// ============================================================================

double AudioTimeLattice::quantizeToGrid(double timeInSeconds, QuantizeMode mode) {
    auto grid = generatePPQNGrid(timeInSeconds - 1.0, timeInSeconds + 1.0);
    if (grid.empty()) return timeInSeconds;

    // Find nearest grid point
    double nearestTime = grid[0];
    double minDist = std::abs(timeInSeconds - nearestTime);

    for (double gridTime : grid) {
        double dist = std::abs(timeInSeconds - gridTime);
        if (dist < minDist) {
            minDist = dist;
            nearestTime = gridTime;
        }
    }

    switch (mode) {
    case QuantizeMode::Nearest:
        return nearestTime;
    case QuantizeMode::Floor:
        return (nearestTime <= timeInSeconds) ? nearestTime :
            (grid.size() > 1 ? grid[grid.size() - 2] : nearestTime);
    case QuantizeMode::Ceil:
        return (nearestTime >= timeInSeconds) ? nearestTime :
            (grid.size() > 1 ? grid[1] : nearestTime);
    }

    return nearestTime;
}

double AudioTimeLattice::quantizeToBeat(double timeInSeconds, QuantizeMode mode) {
    TempoEvent tempo = getCurrentTempo(timeInSeconds);
    double beatLength = 60.0 / tempo.bpm;

    int beatNum = static_cast<int>(std::round(timeInSeconds / beatLength));

    switch (mode) {
    case QuantizeMode::Nearest:
        return beatNum * beatLength;
    case QuantizeMode::Floor:
        return std::floor(timeInSeconds / beatLength) * beatLength;
    case QuantizeMode::Ceil:
        return std::ceil(timeInSeconds / beatLength) * beatLength;
    }

    return beatNum * beatLength;
}

double AudioTimeLattice::quantizeToBar(double timeInSeconds, QuantizeMode mode) {
    TempoEvent tempo = getCurrentTempo(timeInSeconds);
    double beatLength = 60.0 / tempo.bpm;
    double barLength = tempo.upperTimeSig * beatLength;

    int barNum = static_cast<int>(std::round(timeInSeconds / barLength));

    switch (mode) {
    case QuantizeMode::Nearest:
        return barNum * barLength;
    case QuantizeMode::Floor:
        return std::floor(timeInSeconds / barLength) * barLength;
    case QuantizeMode::Ceil:
        return std::ceil(timeInSeconds / barLength) * barLength;
    }

    return barNum * barLength;
}

double AudioTimeLattice::quantizeValue(double value, ValueResolution resolution) {
    int bits = static_cast<int>(resolution);
    int steps = (1 << bits) - 1;

    // Normalize -1 to 1 range to 0 to 1
    double normalized = (value + 1.0) * 0.5;
    int quantized = static_cast<int>(normalized * steps + 0.5);

    return (quantized / static_cast<double>(steps)) * 2.0 - 1.0;
}

std::vector<std::pair<double, double>> AudioTimeLattice::quantizeBreakpoints(
    const std::vector<std::pair<double, double>>& input,
    ValueResolution resolution,
    bool simplify) {

    std::vector<std::pair<double, double>> result;
    if (input.empty()) return result;

    double lastTime = -1.0;
    double lastValue = 0.0;
    double threshold = calculatePerceptualThreshold(resolution);

    for (const auto& [time, value] : input) {
        double qTime = quantizeToGrid(time);
        double qValue = quantizeValue(value, resolution);

        // Skip if too close to previous point
        if (simplify && !result.empty()) {
            double timeDiff = std::abs(qTime - lastTime);
            double valueDiff = std::abs(qValue - lastValue);

            if (timeDiff < getTickDuration() * 0.5 && valueDiff < threshold) {
                continue;
            }
        }

        result.push_back({ qTime, qValue });
        lastTime = qTime;
        lastValue = qValue;
    }

    return result;
}

// ============================================================================
// Audio Editing Operations
// ============================================================================

juce::AudioBuffer<float> AudioTimeLattice::trim(const juce::AudioBuffer<float>& input,
    double startTime, double endTime) {
    int startSample = secondsToSamples(startTime);
    int endSample = secondsToSamples(endTime);

    startSample = juce::jlimit(0, input.getNumSamples(), startSample);
    endSample = juce::jlimit(startSample, input.getNumSamples(), endSample);

    int numSamples = endSample - startSample;
    juce::AudioBuffer<float> output(input.getNumChannels(), numSamples);

    for (int ch = 0; ch < input.getNumChannels(); ++ch) {
        output.copyFrom(ch, 0, input, ch, startSample, numSamples);
    }

    return output;
}

juce::AudioBuffer<float> AudioTimeLattice::cut(const juce::AudioBuffer<float>& input,
    double startTime, double endTime) {
    int startSample = secondsToSamples(startTime);
    int endSample = secondsToSamples(endTime);

    startSample = juce::jlimit(0, input.getNumSamples(), startSample);
    endSample = juce::jlimit(startSample, input.getNumSamples(), endSample);

    int cutLength = endSample - startSample;
    int outputLength = input.getNumSamples() - cutLength;

    juce::AudioBuffer<float> output(input.getNumChannels(), outputLength);

    for (int ch = 0; ch < input.getNumChannels(); ++ch) {
        // Copy before cut
        output.copyFrom(ch, 0, input, ch, 0, startSample);
        // Copy after cut
        output.copyFrom(ch, startSample, input, ch, endSample,
            input.getNumSamples() - endSample);
    }

    return output;
}

std::vector<juce::AudioBuffer<float>> AudioTimeLattice::split(
    const juce::AudioBuffer<float>& input,
    const std::vector<double>& splitTimes) {

    std::vector<juce::AudioBuffer<float>> segments;

    std::vector<double> times = { 0.0 };
    times.insert(times.end(), splitTimes.begin(), splitTimes.end());
    times.push_back(samplesToSeconds(input.getNumSamples()));

    for (size_t i = 0; i < times.size() - 1; ++i) {
        segments.push_back(trim(input, times[i], times[i + 1]));
    }

    return segments;
}

juce::AudioBuffer<float> AudioTimeLattice::merge(
    const std::vector<juce::AudioBuffer<float>>& clips,
    const std::vector<double>& positions) {

    if (clips.empty()) return juce::AudioBuffer<float>();
    if (positions.size() != clips.size()) return juce::AudioBuffer<float>();

    // Calculate total length needed
    double totalLength = 0.0;
    for (size_t i = 0; i < clips.size(); ++i) {
        double endTime = positions[i] + samplesToSeconds(clips[i].getNumSamples());
        totalLength = std::max(totalLength, endTime);
    }

    int numChannels = clips[0].getNumChannels();
    juce::AudioBuffer<float> output(numChannels, secondsToSamples(totalLength));
    output.clear();

    // Mix all clips at their positions
    for (size_t i = 0; i < clips.size(); ++i) {
        int startSample = secondsToSamples(positions[i]);
        for (int ch = 0; ch < numChannels; ++ch) {
            output.addFrom(ch, startSample, clips[i], ch, 0, clips[i].getNumSamples());
        }
    }

    return output;
}

juce::AudioBuffer<float> AudioTimeLattice::nudge(const juce::AudioBuffer<float>& input,
    double nudgeAmount, bool fillWithSilence) {
    int nudgeSamples = secondsToSamples(std::abs(nudgeAmount));
    bool nudgeForward = nudgeAmount > 0;

    juce::AudioBuffer<float> output(input.getNumChannels(), input.getNumSamples());
    output.clear();

    if (nudgeForward) {
        for (int ch = 0; ch < input.getNumChannels(); ++ch) {
            output.copyFrom(ch, nudgeSamples, input, ch, 0,
                input.getNumSamples() - nudgeSamples);
        }
    }
    else {
        for (int ch = 0; ch < input.getNumChannels(); ++ch) {
            output.copyFrom(ch, 0, input, ch, nudgeSamples,
                input.getNumSamples() - nudgeSamples);
        }
    }

    return output;
}

juce::AudioBuffer<float> AudioTimeLattice::timeStretch(const juce::AudioBuffer<float>& input,
    double stretchFactor) {
    // Simple time stretching using linear interpolation
    // For production use, consider juce::TimeStretcher or rubberband library

    int outputLength = static_cast<int>(input.getNumSamples() * stretchFactor);
    juce::AudioBuffer<float> output(input.getNumChannels(), outputLength);

    for (int ch = 0; ch < input.getNumChannels(); ++ch) {
        const float* inputData = input.getReadPointer(ch);
        float* outputData = output.getWritePointer(ch);

        for (int i = 0; i < outputLength; ++i) {
            double srcPos = i / stretchFactor;
            int srcIndex = static_cast<int>(srcPos);
            double frac = srcPos - srcIndex;

            if (srcIndex + 1 < input.getNumSamples()) {
                outputData[i] = inputData[srcIndex] * (1.0f - frac) +
                    inputData[srcIndex + 1] * frac;
            }
            else if (srcIndex < input.getNumSamples()) {
                outputData[i] = inputData[srcIndex];
            }
        }
    }

    return output;
}

juce::AudioBuffer<float> AudioTimeLattice::quantizeAudio(const juce::AudioBuffer<float>& input,
    double audioStartTime,
    double quantizeStrength) {
    auto transients = detectTransients(input, 0.5);
    juce::AudioBuffer<float> output = input;

    for (double transientTime : transients) {
        double globalTime = audioStartTime + transientTime;
        double quantizedTime = quantizeToGrid(globalTime);
        double shift = (quantizedTime - globalTime) * quantizeStrength;

        // TODO: Implement actual time-shifting per transient
        // This requires more complex audio manipulation
    }

    return output;
}

juce::AudioBuffer<float> AudioTimeLattice::humanize(const juce::AudioBuffer<float>& input,
    double audioStartTime,
    double humanizeAmount) {
    auto transients = detectTransients(input, 0.5);
    juce::AudioBuffer<float> output = input;

    juce::Random random;
    for (double transientTime : transients) {
        double randomShift = (random.nextFloat() - 0.5) * 2.0 * humanizeAmount * getTickDuration();
        // TODO: Apply random time shift to this transient
    }

    return output;
}

std::vector<double> AudioTimeLattice::detectTransients(const juce::AudioBuffer<float>& input,
    double threshold) {
    std::vector<double> transients;

    int windowSize = 1024;
    int hopSize = 512;
    float previousEnergy = 0.0f;

    for (int start = 0; start < input.getNumSamples() - windowSize; start += hopSize) {
        float energy = 0.0f;

        for (int ch = 0; ch < input.getNumChannels(); ++ch) {
            const float* data = input.getReadPointer(ch);
            for (int i = 0; i < windowSize; ++i) {
                float sample = data[start + i];
                energy += sample * sample;
            }
        }

        energy = std::sqrt(energy / (windowSize * input.getNumChannels()));

        float onsetStrength = std::max(0.0f, energy - previousEnergy);

        if (onsetStrength > threshold) {
            transients.push_back(samplesToSeconds(start));
        }

        previousEnergy = energy * 0.9f; // Decay
    }

    return transients;
}

std::vector<AudioMarker> AudioTimeLattice::detectBeats(const juce::AudioBuffer<float>& input) {
    std::vector<AudioMarker> beats;
    auto transients = detectTransients(input, 0.4);

    int id = 1;
    for (double time : transients) {
        beats.push_back({ time, "Beat " + std::to_string(id), juce::Colours::cyan, id });
        id++;
    }

    return beats;
}

juce::AudioBuffer<float> AudioTimeLattice::crossfade(const juce::AudioBuffer<float>& clip1,
    const juce::AudioBuffer<float>& clip2,
    double crossfadeDuration) {
    int crossfadeSamples = secondsToSamples(crossfadeDuration);
    int outputLength = clip1.getNumSamples() + clip2.getNumSamples() - crossfadeSamples;

    juce::AudioBuffer<float> output(clip1.getNumChannels(), outputLength);
    output.clear();

    // Copy first clip
    for (int ch = 0; ch < clip1.getNumChannels(); ++ch) {
        output.copyFrom(ch, 0, clip1, ch, 0, clip1.getNumSamples());
    }

    // Crossfade region
    int fadeStart = clip1.getNumSamples() - crossfadeSamples;
    for (int i = 0; i < crossfadeSamples; ++i) {
        float ratio = i / static_cast<float>(crossfadeSamples);
        float fadeOut = std::cos(ratio * juce::MathConstants<float>::halfPi);
        float fadeIn = std::sin(ratio * juce::MathConstants<float>::halfPi);

        for (int ch = 0; ch < output.getNumChannels(); ++ch) {
            float* data = output.getWritePointer(ch);
            float sample1 = clip1.getSample(ch, fadeStart + i) * fadeOut;
            float sample2 = clip2.getSample(ch, i) * fadeIn;
            data[fadeStart + i] = sample1 + sample2;
        }
    }

    // Copy rest of second clip
    for (int ch = 0; ch < clip2.getNumChannels(); ++ch) {
        output.copyFrom(ch, clip1.getNumSamples(), clip2, ch, crossfadeSamples,
            clip2.getNumSamples() - crossfadeSamples);
    }

    return output;
}

// ============================================================================
// Marker Management
// ============================================================================

void AudioTimeLattice::addMarker(double timeInSeconds, const std::string& label,
    juce::Colour color) {
    markers.push_back({ timeInSeconds, label, color, nextMarkerId++ });
}

void AudioTimeLattice::removeMarker(int id) {
    markers.erase(std::remove_if(markers.begin(), markers.end(),
        [id](const AudioMarker& m) { return m.id == id; }),
        markers.end());
}

void AudioTimeLattice::clearMarkers() {
    markers.clear();
}

AudioMarker* AudioTimeLattice::findNearestMarker(double timeInSeconds, double tolerance) {
    AudioMarker* nearest = nullptr;
    double minDist = tolerance;

    for (auto& marker : markers) {
        double dist = std::abs(marker.timeInSeconds - timeInSeconds);
        if (dist < minDist) {
            minDist = dist;
            nearest = &marker;
        }
    }

    return nearest;
}

// ============================================================================
// Helper Methods
// ============================================================================

TempoEvent AudioTimeLattice::getCurrentTempo(double timeInSeconds) const {
    if (tempoMap.empty()) return { 0.0, 120.0, 4, 4 };

    for (auto it = tempoMap.rbegin(); it != tempoMap.rend(); ++it) {
        if (timeInSeconds >= it->timeInSeconds) {
            return *it;
        }
    }

    return tempoMap.front();
}

double AudioTimeLattice::getTickDuration() const {
    TempoEvent tempo = getCurrentTempo(0);
    double beatLength = 60.0 / tempo.bpm;
    return beatLength / ppqn;
}

double AudioTimeLattice::getBeatDuration(double atTime) const {
    TempoEvent tempo = getCurrentTempo(atTime);
    return 60.0 / tempo.bpm;
}

double AudioTimeLattice::getBarDuration(double atTime) const {
    TempoEvent tempo = getCurrentTempo(atTime);
    return (60.0 / tempo.bpm) * tempo.upperTimeSig;
}

TempoEvent AudioTimeLattice::getTempoAt(double timeInSeconds) const {
    return getCurrentTempo(timeInSeconds);
}

double AudioTimeLattice::calculatePerceptualThreshold(ValueResolution resolution) {
    // Based on human hearing research
    switch (resolution) {
    case ValueResolution::Bit7:  return 0.02;  // ~2% change
    case ValueResolution::Bit14: return 0.001; // ~0.1% change
    case ValueResolution::Bit24: return 0.0001;
    case ValueResolution::Bit32: return 0.00001;
    }
    return 0.01;
}

double AudioTimeLattice::snapToGrid(double timeInSeconds, double gridSpacing) {
    return std::round(timeInSeconds / gridSpacing) * gridSpacing;
}

bool AudioTimeLattice::isOnGrid(double timeInSeconds, double tolerance) {
    double quantized = quantizeToGrid(timeInSeconds);
    return std::abs(timeInSeconds - quantized) < tolerance;
}

double AudioTimeLattice::getNextGridPoint(double timeInSeconds) {
    return quantizeToGrid(timeInSeconds + getTickDuration(), QuantizeMode::Ceil);
}

double AudioTimeLattice::getPreviousGridPoint(double timeInSeconds) {
    return quantizeToGrid(timeInSeconds - getTickDuration(), QuantizeMode::Floor);
}