// ============================================================================
// AudioBuilder - PluginEditor.cpp
// ============================================================================
#include "PluginEditor.h"
#include "PluginProcessor.h"

AudioBuilderEditor::AudioBuilderEditor(AudioBuilderProcessor& p)
    : AudioProcessorEditor(&p), processor(p) {

    // Buttons
    loadAudioButton.setButtonText("Load Audio");
    loadAudioButton.addListener(this);
    addAndMakeVisible(loadAudioButton);

    loadBreakpointsButton.setButtonText("Load Breakpoints");
    loadBreakpointsButton.addListener(this);
    addAndMakeVisible(loadBreakpointsButton);

    applyButton.setButtonText("Apply");
    applyButton.addListener(this);
    addAndMakeVisible(applyButton);

    exportButton.setButtonText("Export WAV");
    exportButton.addListener(this);
    addAndMakeVisible(exportButton);

    clearButton.setButtonText("Clear");
    clearButton.addListener(this);
    addAndMakeVisible(clearButton);

    decimateButton.setButtonText("Reduce Points");
    decimateButton.addListener(this);
    addAndMakeVisible(decimateButton);

    quantizeToGridButton.setButtonText("Snap to Grid");
    quantizeToGridButton.addListener(this);
    addAndMakeVisible(quantizeToGridButton);

    // Edit operations
    editOperationLabel.setText("Edit Operation:", juce::dontSendNotification);
    addAndMakeVisible(editOperationLabel);

    editOperationSelector.addItem("Trim", 1);
    editOperationSelector.addItem("Cut", 2);
    editOperationSelector.addItem("Split", 3);
    editOperationSelector.addItem("Nudge Forward", 4);
    editOperationSelector.addItem("Nudge Backward", 5);
    editOperationSelector.addItem("Time Stretch", 6);
    editOperationSelector.addItem("Quantize to Grid", 7);
    editOperationSelector.addItem("Humanize", 8);
    editOperationSelector.addItem("Detect Beats", 9);
    editOperationSelector.addItem("Snap Breakpoints", 10);
    editOperationSelector.setSelectedId(1);
    editOperationSelector.addListener(this);
    addAndMakeVisible(editOperationSelector);

    performEditButton.setButtonText("Perform Edit");
    performEditButton.addListener(this);
    addAndMakeVisible(performEditButton);

    // PPQN selector
    ppqnLabel.setText("PPQN:", juce::dontSendNotification);
    addAndMakeVisible(ppqnLabel);

    ppqnSelector.addItem("24", 1);
    ppqnSelector.addItem("48", 2);
    ppqnSelector.addItem("96", 3);
    ppqnSelector.addItem("192", 4);
    ppqnSelector.addItem("384", 5);
    ppqnSelector.addItem("480", 6);
    ppqnSelector.addItem("960", 7);
    ppqnSelector.setSelectedId(7); // Default 960
    ppqnSelector.addListener(this);
    addAndMakeVisible(ppqnSelector);

    // Resolution selector
    resolutionLabel.setText("Resolution:", juce::dontSendNotification);
    addAndMakeVisible(resolutionLabel);

    resolutionSelector.addItem("7-bit (MIDI)", 1);
    resolutionSelector.addItem("14-bit (NRPN)", 2);
    resolutionSelector.addItem("24-bit (Audio)", 3);
    resolutionSelector.addItem("32-bit (Float)", 4);
    resolutionSelector.setSelectedId(2); // Default 14-bit
    resolutionSelector.addListener(this);
    addAndMakeVisible(resolutionSelector);

    // Output selector
    outputLabel.setText("Output:", juce::dontSendNotification);
    addAndMakeVisible(outputLabel);

    outputSelector.addListener(this);
    addAndMakeVisible(outputSelector);

    // Feature label
    featureLabel.setText("Feature: None", juce::dontSendNotification);
    featureLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    featureLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    addAndMakeVisible(featureLabel);

    // Intensity slider
    intensityLabel.setText("Intensity:", juce::dontSendNotification);
    addAndMakeVisible(intensityLabel);

    intensitySlider.setRange(0.0, 2.0, 0.01);
    intensitySlider.setValue(1.0);
    intensitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 24);
    intensitySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    intensitySlider.addListener(this);
    addAndMakeVisible(intensitySlider);

    // Smoothing toggle
    smoothingToggle.setButtonText("Smoothing");
    smoothingToggle.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(smoothingToggle);

    // Info labels
    infoLabel.setText("Load audio and breakpoint files to begin", juce::dontSendNotification);
    infoLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(infoLabel);

    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);

    setSize(900, 700);
    startTimerHz(30);
}

AudioBuilderEditor::~AudioBuilderEditor() {
    stopTimer();
}

void AudioBuilderEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff1a1a1a));

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawText("Audio Builder", getLocalBounds().removeFromTop(50), juce::Justification::centred);

    // Graph area
    graphBounds = getLocalBounds().withTrimmedTop(180).withHeight(350).reduced(15, 10);
    drawGraphBackground(g, graphBounds);

    if (processor.hasLoadedAudio()) {
        drawWaveform(g, graphBounds);
    }

    if (!displayedBreakpoints.empty()) {
        drawBreakpoints(g, graphBounds);
    }
}

void AudioBuilderEditor::drawGraphBackground(juce::Graphics& g, const juce::Rectangle<int>& area) {
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(area);

    g.setColour(juce::Colour(0xff444444));
    g.drawRect(area, 2);

    // Grid
    g.setColour(juce::Colour(0xff353535));
    for (int i = 0; i <= 4; ++i) {
        float y = area.getY() + (area.getHeight() * i / 4.0f);
        g.drawHorizontalLine(static_cast<int>(y), static_cast<float>(area.getX()),
            static_cast<float>(area.getRight()));
    }

    for (int i = 0; i <= 10; ++i) {
        float x = area.getX() + (area.getWidth() * i / 10.0f);
        g.drawVerticalLine(static_cast<int>(x), static_cast<float>(area.getY()),
            static_cast<float>(area.getBottom()));
    }

    // Center line
    g.setColour(juce::Colour(0xff666666));
    g.drawHorizontalLine(area.getCentreY(), static_cast<float>(area.getX()),
        static_cast<float>(area.getRight()));
}

void AudioBuilderEditor::drawWaveform(juce::Graphics& g, const juce::Rectangle<int>& area) {
    const auto& buffer = processor.getSourceAudio();
    if (buffer.getNumSamples() == 0) return;

    g.setColour(juce::Colours::grey.withAlpha(0.4f));

    const float* data = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();
    int step = juce::jmax(1, numSamples / area.getWidth());

    juce::Path path;
    path.startNewSubPath(area.getX(), area.getCentreY());

    for (int i = 0; i < numSamples; i += step) {
        float x = area.getX() + (i * area.getWidth() / static_cast<float>(numSamples));
        float y = area.getCentreY() - (data[i] * area.getHeight() * 0.4f);
        path.lineTo(x, y);
    }

    g.strokePath(path, juce::PathStrokeType(1.5f));
}

void AudioBuilderEditor::drawBreakpoints(juce::Graphics& g, const juce::Rectangle<int>& area) {
    if (displayedBreakpoints.empty()) return;

    // Calculate bounds
    float maxTime = 0.0f;
    float minValue = 1e10f;
    float maxValue = -1e10f;

    for (const auto& [time, value] : displayedBreakpoints) {
        maxTime = juce::jmax(maxTime, time);
        minValue = juce::jmin(minValue, value);
        maxValue = juce::jmax(maxValue, value);
    }

    if (maxTime <= 0.0f) maxTime = 1.0f;
    float valueRange = maxValue - minValue;
    if (valueRange < 0.001f) {
        valueRange = 1.0f;
        minValue = maxValue - 0.5f;
    }

    // Draw curve
    juce::Path curvePath;
    bool firstPoint = true;

    for (const auto& [time, value] : displayedBreakpoints) {
        float x = area.getX() + (time / maxTime) * area.getWidth();
        float normalizedValue = (value - minValue) / valueRange;
        float y = area.getY() + area.getHeight() * (1.0f - normalizedValue);

        if (firstPoint) {
            curvePath.startNewSubPath(x, y);
            firstPoint = false;
        }
        else {
            curvePath.lineTo(x, y);
        }
    }

    g.setColour(juce::Colours::cyan.withAlpha(0.8f));
    g.strokePath(curvePath, juce::PathStrokeType(2.5f));

    // Draw breakpoint markers
    for (size_t i = 0; i < displayedBreakpoints.size(); ++i) {
        const auto& [time, value] = displayedBreakpoints[i];
        float x = area.getX() + (time / maxTime) * area.getWidth();
        float normalizedValue = (value - minValue) / valueRange;
        float y = area.getY() + area.getHeight() * (1.0f - normalizedValue);

        // Highlight dragged point
        if (i == draggedBreakpoint.index && isDragging) {
            g.setColour(juce::Colours::red);
            g.fillEllipse(x - 8, y - 8, 16, 16);
            g.setColour(juce::Colours::white);
            g.drawEllipse(x - 8, y - 8, 16, 16, 2.0f);
        }
        else {
            g.setColour(juce::Colours::yellow);
            g.fillEllipse(x - 6, y - 6, 12, 12);
            g.setColour(juce::Colours::black);
            g.drawEllipse(x - 6, y - 6, 12, 12, 1.5f);
        }

        // Draw index labels for every 10th point
        if (i % 10 == 0) {
            g.setColour(juce::Colours::white);
            g.setFont(9.0f);
            g.drawText(juce::String(i), static_cast<int>(x - 15), static_cast<int>(y - 25),
                30, 15, juce::Justification::centred);
        }
    }
}

void AudioBuilderEditor::resized() {
    auto area = getLocalBounds();

    // Skip title
    area.removeFromTop(50);

    // Control row 1
    auto controlRow1 = area.removeFromTop(40).reduced(15, 5);
    loadAudioButton.setBounds(controlRow1.removeFromLeft(100));
    controlRow1.removeFromLeft(10);
    loadBreakpointsButton.setBounds(controlRow1.removeFromLeft(150));
    controlRow1.removeFromLeft(10);
    applyButton.setBounds(controlRow1.removeFromLeft(80));
    controlRow1.removeFromLeft(10);
    exportButton.setBounds(controlRow1.removeFromLeft(100));
    controlRow1.removeFromLeft(10);
    clearButton.setBounds(controlRow1.removeFromLeft(70));

    // Control row 2
    auto controlRow2 = area.removeFromTop(40).reduced(15, 5);
    featureLabel.setBounds(controlRow2.removeFromLeft(200));
    controlRow2.removeFromLeft(20);
    outputLabel.setBounds(controlRow2.removeFromLeft(60));
    controlRow2.removeFromLeft(5);
    outputSelector.setBounds(controlRow2.removeFromLeft(150));

    // Control row 3
    auto controlRow3 = area.removeFromTop(40).reduced(15, 5);
    intensityLabel.setBounds(controlRow3.removeFromLeft(70));
    controlRow3.removeFromLeft(5);
    intensitySlider.setBounds(controlRow3.removeFromLeft(250));
    controlRow3.removeFromLeft(15);
    smoothingToggle.setBounds(controlRow3.removeFromLeft(100));
    controlRow3.removeFromLeft(15);
    decimateButton.setBounds(controlRow3.removeFromLeft(120));
    controlRow3.removeFromLeft(10);
    quantizeToGridButton.setBounds(controlRow3.removeFromLeft(110));

    // Control row 4 - Edit operations
    auto controlRow4 = area.removeFromTop(40).reduced(15, 5);
    editOperationLabel.setBounds(controlRow4.removeFromLeft(100));
    controlRow4.removeFromLeft(5);
    editOperationSelector.setBounds(controlRow4.removeFromLeft(150));
    controlRow4.removeFromLeft(10);
    performEditButton.setBounds(controlRow4.removeFromLeft(110));

    // Control row 5 - Grid settings
    auto controlRow5 = area.removeFromTop(40).reduced(15, 5);
    ppqnLabel.setBounds(controlRow5.removeFromLeft(50));
    controlRow5.removeFromLeft(5);
    ppqnSelector.setBounds(controlRow5.removeFromLeft(80));
    controlRow5.removeFromLeft(20);
    resolutionLabel.setBounds(controlRow5.removeFromLeft(80));
    controlRow5.removeFromLeft(5);
    resolutionSelector.setBounds(controlRow5.removeFromLeft(150));

    // Graph area
    graphBounds = area.removeFromTop(350).reduced(15, 10);

    // Status row
    auto statusRow = area.removeFromTop(30).reduced(15, 5);
    infoLabel.setBounds(statusRow.removeFromLeft(500));
    statusLabel.setBounds(statusRow);
}

void AudioBuilderEditor::timerCallback() {
    updateDisplay();

    if (processor.isProcessing()) {
        float progress = processor.getProcessingProgress();
        statusLabel.setText("Processing: " + juce::String(static_cast<int>(progress * 100)) + "%",
            juce::dontSendNotification);
    }

    repaint();
}

bool AudioBuilderEditor::isInterestedInFileDrag(const juce::StringArray& files) {
    for (const auto& file : files) {
        if (file.endsWithIgnoreCase(".wav") || file.endsWithIgnoreCase(".aif") ||
            file.endsWithIgnoreCase(".aiff") || file.endsWithIgnoreCase(".txt")) {
            return true;
        }
    }
    return false;
}

void AudioBuilderEditor::filesDropped(const juce::StringArray& files, int, int) {
    for (const auto& file : files) {
        if (file.endsWithIgnoreCase(".txt")) {
            if (processor.loadBreakpointFile(juce::File(file))) {
                featureLabel.setText("Feature: " + processor.getBreakpointFeatureName(),
                    juce::dontSendNotification);
                updateOutputSelector();
                statusLabel.setText("Loaded breakpoints", juce::dontSendNotification);
            }
        }
        else if (file.endsWithIgnoreCase(".wav") || file.endsWithIgnoreCase(".aif") ||
            file.endsWithIgnoreCase(".aiff")) {
            if (processor.loadAudioFile(juce::File(file))) {
                infoLabel.setText("Audio: " + juce::File(file).getFileName(),
                    juce::dontSendNotification);
                statusLabel.setText("Ready to apply", juce::dontSendNotification);
            }
        }
        repaint();
    }
}

void AudioBuilderEditor::comboBoxChanged(juce::ComboBox* combo) {
    if (combo == &outputSelector) {
        currentOutput = outputSelector.getSelectedId() - 1;
        updateDisplay();
    }
    else if (combo == &ppqnSelector) {
        int ppqnValues[] = { 24, 48, 96, 192, 384, 480, 960 };
        int selectedIndex = ppqnSelector.getSelectedId() - 1;
        if (selectedIndex >= 0 && selectedIndex < 7) {
            processor.setTimeGridPPQN(ppqnValues[selectedIndex]);
            statusLabel.setText("PPQN set to " + juce::String(ppqnValues[selectedIndex]),
                juce::dontSendNotification);
        }
    }
    else if (combo == &resolutionSelector) {
        ValueResolution resolutions[] = {
            ValueResolution::Bit7,
            ValueResolution::Bit14,
            ValueResolution::Bit24,
            ValueResolution::Bit32
        };
        int selectedIndex = resolutionSelector.getSelectedId() - 1;
        if (selectedIndex >= 0 && selectedIndex < 4) {
            processor.setTimeGridResolution(resolutions[selectedIndex]);
            statusLabel.setText("Resolution set to " + resolutionSelector.getText(),
                juce::dontSendNotification);
        }
    }
}

void AudioBuilderEditor::buttonClicked(juce::Button* button) {
    if (button == &loadAudioButton) {
        loadAudioFile();
    }
    else if (button == &loadBreakpointsButton) {
        loadBreakpointFile();
    }
    else if (button == &applyButton) {
        applyBreakpoints();
    }
    else if (button == &exportButton) {
        exportAudio();
    }
    else if (button == &clearButton) {
        clearAll();
    }
    else if (button == &decimateButton) {
        decimateCurrentBreakpoints();
    }
    else if (button == &quantizeToGridButton) {
        quantizeBreakpointsToGrid();
    }
    else if (button == &performEditButton) {
        performSelectedEdit();
    }
}

void AudioBuilderEditor::sliderValueChanged(juce::Slider* slider) {
    if (slider == &intensitySlider) {
        // Intensity parameter is connected to processor.params
    }
}

void AudioBuilderEditor::loadAudioFile() {
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Audio File",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.wav;*.aif;*.aiff"
    );

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode |
        juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser) {
            auto result = chooser.getResult();
            if (result.existsAsFile()) {
                if (processor.loadAudioFile(result)) {
                    infoLabel.setText("Audio: " + result.getFileName(),
                        juce::dontSendNotification);
                    statusLabel.setText("Ready to apply", juce::dontSendNotification);
                    repaint();
                }
            }
        });
}

void AudioBuilderEditor::loadBreakpointFile() {
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Breakpoint File",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.txt"
    );

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode |
        juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser) {
            auto result = chooser.getResult();
            if (result.existsAsFile()) {
                if (processor.loadBreakpointFile(result)) {
                    featureLabel.setText("Feature: " + processor.getBreakpointFeatureName(),
                        juce::dontSendNotification);
                    updateOutputSelector();
                    updateDisplay();
                    statusLabel.setText("Loaded breakpoints: " + result.getFileName(),
                        juce::dontSendNotification);
                }
            }
        });
}

void AudioBuilderEditor::applyBreakpoints() {
    if (!processor.hasLoadedAudio()) {
        statusLabel.setText("Load audio first", juce::dontSendNotification);
        return;
    }
    if (!processor.hasBreakpoints()) {
        statusLabel.setText("Load breakpoints first", juce::dontSendNotification);
        return;
    }

    statusLabel.setText("Applying breakpoints...", juce::dontSendNotification);
    processor.applyBreakpointsToAudio();
    statusLabel.setText("Applied! Ready to export", juce::dontSendNotification);
}

void AudioBuilderEditor::exportAudio() {
    fileChooser = std::make_unique<juce::FileChooser>(
        "Export Processed Audio",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile(processor.getLoadedFileName() + "_processed.wav"),
        "*.wav"
    );

    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode |
        juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser) {
            auto result = chooser.getResult();
            if (result.getFullPathName().isNotEmpty()) {
                processor.exportProcessedAudio(result);
                statusLabel.setText("Exported: " + result.getFileName(),
                    juce::dontSendNotification);
            }
        });
}

void AudioBuilderEditor::clearAll() {
    processor.clearLoadedAudio();
    processor.clearBreakpoints();
    displayedBreakpoints.clear();
    updateOutputSelector();
    featureLabel.setText("Feature: None", juce::dontSendNotification);
    infoLabel.setText("Load audio and breakpoint files to begin", juce::dontSendNotification);
    statusLabel.setText("Ready", juce::dontSendNotification);
    repaint();
}

void AudioBuilderEditor::decimateCurrentBreakpoints() {
    int currentCount = processor.getCurrentBreakpointCount(currentOutput);
    if (currentCount == 0) {
        statusLabel.setText("No breakpoints to reduce", juce::dontSendNotification);
        return;
    }

    int targetPoints = currentCount / 2;
    if (targetPoints < 10) targetPoints = 10;

    processor.decimateBreakpoints(currentOutput, targetPoints);
    updateDisplay();
    statusLabel.setText("Reduced to " + juce::String(targetPoints) + " points",
        juce::dontSendNotification);
}

void AudioBuilderEditor::updateDisplay() {
    displayedBreakpoints.clear();

    auto points = processor.getBreakpointsForDisplay(currentOutput);
    for (const auto& p : points) {
        displayedBreakpoints.push_back({ static_cast<float>(p.first),
                                        static_cast<float>(p.second) });
    }
}

void AudioBuilderEditor::updateOutputSelector() {
    outputSelector.clear();

    auto outputs = processor.getAvailableOutputs();
    for (int i = 0; i < outputs.size(); ++i) {
        outputSelector.addItem(outputs[i], i + 1);
    }

    if (outputs.size() > 0) {
        currentOutput = 0;
        outputSelector.setSelectedId(1, juce::dontSendNotification);
    }

    updateDisplay();
}

int AudioBuilderEditor::findBreakpointAtPosition(juce::Point<float> position, float tolerance) {
    if (displayedBreakpoints.empty()) return -1;

    float maxTime = 0.0f;
    float minValue = 1e10f;
    float maxValue = -1e10f;

    for (const auto& [time, value] : displayedBreakpoints) {
        maxTime = juce::jmax(maxTime, time);
        minValue = juce::jmin(minValue, value);
        maxValue = juce::jmax(maxValue, value);
    }

    if (maxTime <= 0.0f) maxTime = 1.0f;
    float valueRange = maxValue - minValue;
    if (valueRange < 0.001f) valueRange = 1.0f;

    for (size_t i = 0; i < displayedBreakpoints.size(); ++i) {
        const auto& [time, value] = displayedBreakpoints[i];
        float x = graphBounds.getX() + (time / maxTime) * graphBounds.getWidth();
        float normalizedValue = (value - minValue) / valueRange;
        float y = graphBounds.getY() + graphBounds.getHeight() * (1.0f - normalizedValue);

        if (std::abs(x - position.x) <= tolerance && std::abs(y - position.y) <= tolerance) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

juce::Point<float> AudioBuilderEditor::timeValueToScreen(float time, float value) {
    float maxTime = 1.0f;
    float minValue = 1e10f;
    float maxValue = -1e10f;

    for (const auto& [t, v] : displayedBreakpoints) {
        maxTime = juce::jmax(maxTime, t);
        minValue = juce::jmin(minValue, v);
        maxValue = juce::jmax(maxValue, v);
    }

    if (maxTime <= 0.0f) maxTime = 1.0f;
    float valueRange = maxValue - minValue;
    if (valueRange < 0.001f) valueRange = 1.0f;

    float x = graphBounds.getX() + (time / maxTime) * graphBounds.getWidth();
    float normalizedValue = (value - minValue) / valueRange;
    float y = graphBounds.getY() + graphBounds.getHeight() * (1.0f - normalizedValue);

    return { x, y };
}

std::pair<float, float> AudioBuilderEditor::screenToTimeValue(juce::Point<float> screenPos) {
    float maxTime = 1.0f;
    float minValue = 1e10f;
    float maxValue = -1e10f;

    for (const auto& [time, value] : displayedBreakpoints) {
        maxTime = juce::jmax(maxTime, time);
        minValue = juce::jmin(minValue, value);
        maxValue = juce::jmax(maxValue, value);
    }

    if (maxTime <= 0.0f) maxTime = 1.0f;
    float valueRange = maxValue - minValue;
    if (valueRange < 0.001f) valueRange = 1.0f;

    float time = ((screenPos.x - graphBounds.getX()) / graphBounds.getWidth()) * maxTime;
    float normalizedValue = 1.0f - ((screenPos.y - graphBounds.getY()) / graphBounds.getHeight());
    float value = minValue + normalizedValue * valueRange;

    return { juce::jmax(0.0f, time), value };
}

void AudioBuilderEditor::updateBreakpointFromDrag(juce::Point<float> currentPosition) {
    if (draggedBreakpoint.index >= 0) {
        auto [newTime, newValue] = screenToTimeValue(currentPosition);
        processor.updateBreakpoint(currentOutput, draggedBreakpoint.index, newTime, newValue);
        updateDisplay();
    }
}

void AudioBuilderEditor::addBreakpointAtPosition(juce::Point<float> position) {
    if (graphBounds.contains(position.toInt())) {
        auto [time, value] = screenToTimeValue(position);
        processor.addBreakpoint(currentOutput, time, value);
        updateDisplay();
        statusLabel.setText("Added breakpoint at " + juce::String(time, 2) + "s",
            juce::dontSendNotification);
    }
}

void AudioBuilderEditor::removeBreakpointAtPosition(juce::Point<float> position) {
    int index = findBreakpointAtPosition(position);
    if (index >= 0) {
        processor.removeBreakpoint(currentOutput, index);
        updateDisplay();
        statusLabel.setText("Removed breakpoint " + juce::String(index),
            juce::dontSendNotification);
    }
}

void AudioBuilderEditor::mouseDown(const juce::MouseEvent& event) {
    if (graphBounds.contains(event.getPosition())) {
        if (event.mods.isLeftButtonDown()) {
            int index = findBreakpointAtPosition(event.position);
            if (index >= 0) {
                draggedBreakpoint.index = index;
                draggedBreakpoint.dragStartPosition = event.position;
                isDragging = true;
            }
        }
        else if (event.mods.isRightButtonDown()) {
            removeBreakpointAtPosition(event.position);
        }
    }
}

void AudioBuilderEditor::mouseDrag(const juce::MouseEvent& event) {
    if (isDragging && event.mods.isLeftButtonDown()) {
        updateBreakpointFromDrag(event.position);
    }
}

void AudioBuilderEditor::mouseUp(const juce::MouseEvent&) {
    if (isDragging) {
        isDragging = false;
        statusLabel.setText("Breakpoint updated", juce::dontSendNotification);
    }
}

void AudioBuilderEditor::mouseDoubleClick(const juce::MouseEvent& event) {
    if (graphBounds.contains(event.getPosition()) && event.mods.isLeftButtonDown()) {
        addBreakpointAtPosition(event.position);
    }
}

void AudioBuilderEditor::quantizeBreakpointsToGrid() {
    if (!processor.hasBreakpoints()) {
        statusLabel.setText("Load breakpoints first", juce::dontSendNotification);
        return;
    }

    // Use the processor's edit operation system
    std::vector<double> params = { 0.0 }; // Placeholder
    processor.performEditOperation(AudioBuilderProcessor::EditOperation::SnapToGrid, params);

    updateDisplay();
    statusLabel.setText("Breakpoints snapped to " + juce::String(processor.getTimeGridPPQN()) + " PPQN grid",
        juce::dontSendNotification);
}

void AudioBuilderEditor::performSelectedEdit() {
    if (!processor.hasLoadedAudio()) {
        statusLabel.setText("Load audio first", juce::dontSendNotification);
        return;
    }

    int selectedOp = editOperationSelector.getSelectedId();
    AudioBuilderProcessor::EditOperation op = AudioBuilderProcessor::EditOperation::None;
    std::vector<double> params;

    // Map selection to operation
    switch (selectedOp) {
    case 1: // Trim
        op = AudioBuilderProcessor::EditOperation::Trim;
        // Example: trim first and last 1 second
        params = { 1.0, processor.getSourceAudio().getNumSamples() / processor.getLoadedSampleRate() - 1.0 };
        break;

    case 2: // Cut
        op = AudioBuilderProcessor::EditOperation::Cut;
        // Example: cut middle section
        params = { 2.0, 4.0 };
        break;

    case 3: // Split
        op = AudioBuilderProcessor::EditOperation::Split;
        // Example: split at 2-second intervals
        for (double t = 2.0; t < processor.getSourceAudio().getNumSamples() / processor.getLoadedSampleRate(); t += 2.0) {
            params.push_back(t);
        }
        break;

    case 4: // Nudge Forward
        op = AudioBuilderProcessor::EditOperation::Nudge;
        params = { 0.1 }; // 100ms forward
        break;

    case 5: // Nudge Backward
        op = AudioBuilderProcessor::EditOperation::Nudge;
        params = { -0.1 }; // 100ms backward
        break;

    case 6: // Time Stretch
        op = AudioBuilderProcessor::EditOperation::TimeStretch;
        params = { 1.5 }; // 1.5x slower
        break;

    case 7: // Quantize to Grid
        op = AudioBuilderProcessor::EditOperation::Quantize;
        params = { 0.0 }; // Start at beginning
        break;

    case 8: // Humanize
        op = AudioBuilderProcessor::EditOperation::Humanize;
        params = { 0.0 };
        break;

    case 9: // Detect Beats
        op = AudioBuilderProcessor::EditOperation::DetectBeats;
        break;

    case 10: // Snap Breakpoints
        op = AudioBuilderProcessor::EditOperation::SnapToGrid;
        break;
    }

    auto result = processor.performEditOperation(op, params);

    if (result.getNumSamples() > 0) {
        // Update processor's audio with result
        // For now, just show success message
        statusLabel.setText("Edit operation completed: " + editOperationSelector.getText(),
            juce::dontSendNotification);
    }
    else {
        statusLabel.setText("Edit operation had no effect", juce::dontSendNotification);
    }

    repaint();
}