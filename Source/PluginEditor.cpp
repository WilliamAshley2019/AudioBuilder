// ============================================================================
// Audio Workshop - PluginEditor.cpp
// ============================================================================
#include "PluginEditor.h"
#include "PluginProcessor.h"

AudioWorkshopEditor::AudioWorkshopEditor(AudioWorkshopProcessor& p)
    : AudioProcessorEditor(&p), processor(p) {

    setSize(900, 800);

    // ========================================================================
    // SOURCE AUDIO SECTION
    // ========================================================================

    loadSourceButton.setButtonText("Load Source Audio");
    loadSourceButton.addListener(this);
    addAndMakeVisible(loadSourceButton);

    sourceInfoLabel.setText("Source: None", juce::dontSendNotification);
    sourceInfoLabel.setColour(juce::Label::textColourId, juce::Colours::lightblue);
    addAndMakeVisible(sourceInfoLabel);

    // ========================================================================
    // TARGET AUDIO SECTION
    // ========================================================================

    loadTargetButton.setButtonText("Load Target Audio");
    loadTargetButton.addListener(this);
    addAndMakeVisible(loadTargetButton);

    targetInfoLabel.setText("Target: None", juce::dontSendNotification);
    targetInfoLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    addAndMakeVisible(targetInfoLabel);

    // ========================================================================
    // FEATURE EXTRACTION CONTROLS
    // ========================================================================

    featureLabel.setText("Feature:", juce::dontSendNotification);
    addAndMakeVisible(featureLabel);

    auto features = processor.getAvailableFeatures();
    for (const auto& feature : features) {
        featureSelector.addItem(feature, featureSelector.getNumItems() + 1);
    }
    featureSelector.setSelectedId(1);
    featureSelector.addListener(this);
    addAndMakeVisible(featureSelector);

    outputLabel.setText("Output:", juce::dontSendNotification);
    addAndMakeVisible(outputLabel);

    outputSelector.addListener(this);
    addAndMakeVisible(outputSelector);

    extractButton.setButtonText("Extract");
    extractButton.addListener(this);
    addAndMakeVisible(extractButton);

    extractAllButton.setButtonText("Extract All");
    extractAllButton.addListener(this);
    addAndMakeVisible(extractAllButton);

    extractADSREnvelopeButton.setButtonText("Extract ADSR");
    extractADSREnvelopeButton.addListener(this);
    addAndMakeVisible(extractADSREnvelopeButton);

    windowSizeLabel.setText("Window:", juce::dontSendNotification);
    addAndMakeVisible(windowSizeLabel);

    windowSizeSlider.setRange(1.0, 100.0, 0.1);
    windowSizeSlider.setValue(15.0);
    windowSizeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    windowSizeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    windowSizeSlider.addListener(this);
    addAndMakeVisible(windowSizeSlider);

    hopSizeLabel.setText("Hop:", juce::dontSendNotification);
    addAndMakeVisible(hopSizeLabel);

    hopSizeSlider.setRange(10.0, 90.0, 1.0);
    hopSizeSlider.setValue(50.0);
    hopSizeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    hopSizeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    hopSizeSlider.addListener(this);
    addAndMakeVisible(hopSizeSlider);

    normalizeToggle.setButtonText("Normalize");
    normalizeToggle.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(normalizeToggle);

    // ========================================================================
    // BREAKPOINT EDITOR CONTROLS
    // ========================================================================

    reducePointsButton.setButtonText("Reduce Points");
    reducePointsButton.addListener(this);
    addAndMakeVisible(reducePointsButton);

    breakpointCountLabel.setText("Points: 0", juce::dontSendNotification);
    addAndMakeVisible(breakpointCountLabel);

    // ========================================================================
    // TIME LATTICE CONTROLS
    // ========================================================================

    ppqnLabel.setText("PPQN:", juce::dontSendNotification);
    addAndMakeVisible(ppqnLabel);

    ppqnSelector.addItem("24", 1);
    ppqnSelector.addItem("48", 2);
    ppqnSelector.addItem("96", 3);
    ppqnSelector.addItem("192", 4);
    ppqnSelector.addItem("384", 5);
    ppqnSelector.addItem("480", 6);
    ppqnSelector.addItem("960", 7);
    ppqnSelector.setSelectedId(7);
    ppqnSelector.addListener(this);
    addAndMakeVisible(ppqnSelector);

    resolutionLabel.setText("Resolution:", juce::dontSendNotification);
    addAndMakeVisible(resolutionLabel);

    resolutionSelector.addItem("7-bit (MIDI)", 1);
    resolutionSelector.addItem("14-bit (NRPN)", 2);
    resolutionSelector.addItem("24-bit (Audio)", 3);
    resolutionSelector.addItem("32-bit (Float)", 4);
    resolutionSelector.setSelectedId(2);
    resolutionSelector.addListener(this);
    addAndMakeVisible(resolutionSelector);

    tempoLabel.setText("Tempo:", juce::dontSendNotification);
    addAndMakeVisible(tempoLabel);

    tempoSlider.setRange(60.0, 200.0, 1.0);
    tempoSlider.setValue(120.0);
    tempoSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    tempoSlider.addListener(this);
    addAndMakeVisible(tempoSlider);

    quantizeToGridButton.setButtonText("Snap to Grid");
    quantizeToGridButton.addListener(this);
    addAndMakeVisible(quantizeToGridButton);

    snapMarkersButton.setButtonText("Snap Markers");
    snapMarkersButton.addListener(this);
    addAndMakeVisible(snapMarkersButton);

    // ========================================================================
    // EDIT OPERATIONS
    // ========================================================================

    editOperationLabel.setText("Edit:", juce::dontSendNotification);
    addAndMakeVisible(editOperationLabel);

    editOperationSelector.addItem("Remove Silence", 1);
    editOperationSelector.addItem("Split by Beats", 2);
    editOperationSelector.addItem("Isolate Transients", 3);
    editOperationSelector.addItem("Time Stretch", 4);
    editOperationSelector.addItem("Quantize Audio", 5);
    editOperationSelector.addItem("Humanize", 6);
    editOperationSelector.setSelectedId(1);
    editOperationSelector.addListener(this);
    addAndMakeVisible(editOperationSelector);

    performEditButton.setButtonText("Perform Edit");
    performEditButton.addListener(this);
    addAndMakeVisible(performEditButton);

    saveBreakpointsButton.setButtonText("Save Breakpoints");
    saveBreakpointsButton.addListener(this);
    addAndMakeVisible(saveBreakpointsButton);

    loadBreakpointsButton.setButtonText("Load Breakpoints");
    loadBreakpointsButton.addListener(this);
    addAndMakeVisible(loadBreakpointsButton);

    // ========================================================================
    // APPLICATION CONTROLS
    // ========================================================================

    intensityLabel.setText("Intensity:", juce::dontSendNotification);
    addAndMakeVisible(intensityLabel);

    intensitySlider.setRange(0.0, 2.0, 0.01);
    intensitySlider.setValue(1.0);
    intensitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    intensitySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    intensitySlider.addListener(this);
    addAndMakeVisible(intensitySlider);

    smoothingToggle.setButtonText("Smoothing");
    smoothingToggle.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(smoothingToggle);

    applyButton.setButtonText("Apply to Target");
    applyButton.addListener(this);
    addAndMakeVisible(applyButton);

    exportBreakpointsButton.setButtonText("Export Breakpoints");
    exportBreakpointsButton.addListener(this);
    addAndMakeVisible(exportBreakpointsButton);

    exportAudioButton.setButtonText("Export Audio");
    exportAudioButton.addListener(this);
    addAndMakeVisible(exportAudioButton);

    clearAllButton.setButtonText("Clear All");
    clearAllButton.addListener(this);
    addAndMakeVisible(clearAllButton);

    // ========================================================================
    // STATUS LABELS
    // ========================================================================

    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(statusLabel);

    extractionStatusLabel.setText("No features extracted", juce::dontSendNotification);
    extractionStatusLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(extractionStatusLabel);

    applicationStatusLabel.setText("No audio processed", juce::dontSendNotification);
    applicationStatusLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(applicationStatusLabel);

    startTimerHz(30);
}

AudioWorkshopEditor::~AudioWorkshopEditor() {
    stopTimer();
}

void AudioWorkshopEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff1a1a1a));

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.drawText("Audio Workshop", getLocalBounds().removeFromTop(40),
        juce::Justification::centred);

    // Draw source waveform
    if (processor.hasSourceAudio()) {
        drawSourceWaveform(g, sourceWaveformBounds);
    }

    // Draw target waveform
    if (processor.hasTargetAudio()) {
        drawTargetWaveform(g, targetWaveformBounds);
    }

    // Draw breakpoint graph
    drawGraphBackground(g, breakpointGraphBounds);
    if (!displayedBreakpoints.empty()) {
        drawBreakpoints(g, breakpointGraphBounds);
    }
}

void AudioWorkshopEditor::resized() {
    auto area = getLocalBounds();
    area.removeFromTop(40); // Remove title area

    // ========================================================================
    // TOP ROW: Dual Audio System
    // ========================================================================
    auto topRow = area.removeFromTop(120);
    auto sourceCol = topRow.removeFromLeft(getWidth() / 2).reduced(5);
    auto targetCol = topRow.reduced(5);

    loadSourceButton.setBounds(sourceCol.removeFromTop(30));
    sourceInfoLabel.setBounds(sourceCol.removeFromTop(25));
    sourceWaveformBounds = sourceCol.reduced(2);

    loadTargetButton.setBounds(targetCol.removeFromTop(30));
    targetInfoLabel.setBounds(targetCol.removeFromTop(25));
    targetWaveformBounds = targetCol.reduced(2);

    // ========================================================================
    // FEATURE EXTRACTION ROW
    // ========================================================================
    auto featureRow = area.removeFromTop(40).reduced(5);
    featureLabel.setBounds(featureRow.removeFromLeft(60));
    featureSelector.setBounds(featureRow.removeFromLeft(150));
    featureRow.removeFromLeft(10);
    outputLabel.setBounds(featureRow.removeFromLeft(60));
    outputSelector.setBounds(featureRow.removeFromLeft(120));
    featureRow.removeFromLeft(10);
    extractButton.setBounds(featureRow.removeFromLeft(80));
    extractAllButton.setBounds(featureRow.removeFromLeft(90));
    extractADSREnvelopeButton.setBounds(featureRow.removeFromLeft(100));

    // ========================================================================
    // EXTRACTION SETTINGS ROW
    // ========================================================================
    auto settingsRow = area.removeFromTop(40).reduced(5);
    windowSizeLabel.setBounds(settingsRow.removeFromLeft(60));
    windowSizeSlider.setBounds(settingsRow.removeFromLeft(120));
    settingsRow.removeFromLeft(10);
    hopSizeLabel.setBounds(settingsRow.removeFromLeft(40));
    hopSizeSlider.setBounds(settingsRow.removeFromLeft(120));
    settingsRow.removeFromLeft(10);
    normalizeToggle.setBounds(settingsRow.removeFromLeft(100));

    // ========================================================================
    // BREAKPOINT GRAPH AREA
    // ========================================================================
    breakpointGraphBounds = area.removeFromTop(250).reduced(10, 5);

    // ========================================================================
    // BREAKPOINT CONTROLS ROW
    // ========================================================================
    auto breakpointRow = area.removeFromTop(35).reduced(5);
    breakpointCountLabel.setBounds(breakpointRow.removeFromLeft(100));
    reducePointsButton.setBounds(breakpointRow.removeFromLeft(120));

    // ========================================================================
    // TIME LATTICE ROW
    // ========================================================================
    auto latticeRow = area.removeFromTop(40).reduced(5);
    ppqnLabel.setBounds(latticeRow.removeFromLeft(50));
    ppqnSelector.setBounds(latticeRow.removeFromLeft(80));
    latticeRow.removeFromLeft(10);
    resolutionLabel.setBounds(latticeRow.removeFromLeft(80));
    resolutionSelector.setBounds(latticeRow.removeFromLeft(120));
    latticeRow.removeFromLeft(10);
    tempoLabel.setBounds(latticeRow.removeFromLeft(50));
    tempoSlider.setBounds(latticeRow.removeFromLeft(100));
    latticeRow.removeFromLeft(10);
    quantizeToGridButton.setBounds(latticeRow.removeFromLeft(110));
    snapMarkersButton.setBounds(latticeRow.removeFromLeft(110));

    // ========================================================================
    // EDIT OPERATIONS ROW
    // ========================================================================
    auto editRow = area.removeFromTop(40).reduced(5);
    editOperationLabel.setBounds(editRow.removeFromLeft(40));
    editOperationSelector.setBounds(editRow.removeFromLeft(150));
    editRow.removeFromLeft(10);
    performEditButton.setBounds(editRow.removeFromLeft(110));
    editRow.removeFromLeft(10);
    saveBreakpointsButton.setBounds(editRow.removeFromLeft(130));
    loadBreakpointsButton.setBounds(editRow.removeFromLeft(130));

    // ========================================================================
    // APPLICATION ROW
    // ========================================================================
    auto appRow = area.removeFromTop(40).reduced(5);
    intensityLabel.setBounds(appRow.removeFromLeft(70));
    intensitySlider.setBounds(appRow.removeFromLeft(200));
    appRow.removeFromLeft(10);
    smoothingToggle.setBounds(appRow.removeFromLeft(100));
    appRow.removeFromLeft(10);
    applyButton.setBounds(appRow.removeFromLeft(120));
    appRow.removeFromLeft(10);
    exportBreakpointsButton.setBounds(appRow.removeFromLeft(140));
    exportAudioButton.setBounds(appRow.removeFromLeft(110));

    // ========================================================================
    // CLEAR ROW
    // ========================================================================
    auto clearRow = area.removeFromTop(35).reduced(5);
    clearAllButton.setBounds(clearRow.removeFromLeft(100));

    // ========================================================================
    // STATUS ROW
    // ========================================================================
    auto statusRow = area.removeFromTop(30).reduced(5);
    statusLabel.setBounds(statusRow.removeFromLeft(300));
    extractionStatusLabel.setBounds(statusRow.removeFromLeft(250));
    applicationStatusLabel.setBounds(statusRow);
}

void AudioWorkshopEditor::timerCallback() {
    updateStatus();
    repaint();
}

// ========================================================================
// DRAWING METHODS
// ========================================================================

void AudioWorkshopEditor::drawSourceWaveform(juce::Graphics& g, const juce::Rectangle<int>& area) {
    const auto& buffer = processor.getSourceAudio();
    if (buffer.getNumSamples() == 0) return;

    g.setColour(juce::Colours::lightblue.withAlpha(0.7f));

    const float* data = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();
    int step = juce::jmax(1, numSamples / area.getWidth());

    juce::Path path;
    path.startNewSubPath(area.getX(), area.getCentreY());

    for (int i = 0; i < numSamples; i += step) {
        float x = area.getX() + (i * area.getWidth() / static_cast<float>(numSamples));
        float y = area.getCentreY() - (data[i] * area.getHeight() * 0.45f);
        path.lineTo(x, y);
    }

    g.strokePath(path, juce::PathStrokeType(1.5f));
}

void AudioWorkshopEditor::drawTargetWaveform(juce::Graphics& g, const juce::Rectangle<int>& area) {
    const auto& buffer = processor.getTargetAudio();
    if (buffer.getNumSamples() == 0) return;

    g.setColour(juce::Colours::lightgreen.withAlpha(0.7f));

    const float* data = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();
    int step = juce::jmax(1, numSamples / area.getWidth());

    juce::Path path;
    path.startNewSubPath(area.getX(), area.getCentreY());

    for (int i = 0; i < numSamples; i += step) {
        float x = area.getX() + (i * area.getWidth() / static_cast<float>(numSamples));
        float y = area.getCentreY() - (data[i] * area.getHeight() * 0.45f);
        path.lineTo(x, y);
    }

    g.strokePath(path, juce::PathStrokeType(1.5f));
}

void AudioWorkshopEditor::drawGraphBackground(juce::Graphics& g, const juce::Rectangle<int>& area) {
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(area);

    g.setColour(juce::Colour(0xff444444));
    g.drawRect(area, 2);

    // Grid lines
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

void AudioWorkshopEditor::drawBreakpoints(juce::Graphics& g, const juce::Rectangle<int>& area) {
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
    }
}

// ========================================================================
// MOUSE INTERACTION
// ========================================================================

void AudioWorkshopEditor::mouseDown(const juce::MouseEvent& event) {
    if (breakpointGraphBounds.contains(event.getPosition())) {
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

void AudioWorkshopEditor::mouseDrag(const juce::MouseEvent& event) {
    if (isDragging && event.mods.isLeftButtonDown()) {
        updateBreakpointFromDrag(event.position);
    }
}

void AudioWorkshopEditor::mouseUp(const juce::MouseEvent&) {
    if (isDragging) {
        isDragging = false;
        statusLabel.setText("Breakpoint updated", juce::dontSendNotification);
    }
}

void AudioWorkshopEditor::mouseDoubleClick(const juce::MouseEvent& event) {
    if (breakpointGraphBounds.contains(event.getPosition()) && event.mods.isLeftButtonDown()) {
        addBreakpointAtPosition(event.position);
    }
}

// ========================================================================
// FILE DRAG AND DROP
// ========================================================================

bool AudioWorkshopEditor::isInterestedInFileDrag(const juce::StringArray& files) {
    for (const auto& file : files) {
        if (file.endsWithIgnoreCase(".wav") || file.endsWithIgnoreCase(".aif") ||
            file.endsWithIgnoreCase(".aiff") || file.endsWithIgnoreCase(".txt")) {
            return true;
        }
    }
    return false;
}

void AudioWorkshopEditor::filesDropped(const juce::StringArray& files, int, int) {
    for (const auto& file : files) {
        if (file.endsWithIgnoreCase(".txt")) {
            if (processor.loadBreakpointFile(juce::File(file))) {
                statusLabel.setText("Loaded breakpoints: " + juce::File(file).getFileName(),
                    juce::dontSendNotification);
                updateOutputSelector();
                updateBreakpointDisplay();
            }
        }
        else if (file.endsWithIgnoreCase(".wav") || file.endsWithIgnoreCase(".aif") ||
            file.endsWithIgnoreCase(".aiff")) {
            // Ask user if this is source or target
            // For simplicity, we'll load as source if none exists, otherwise target
            if (!processor.hasSourceAudio()) {
                if (processor.loadSourceAudio(juce::File(file))) {
                    sourceInfoLabel.setText("Source: " + juce::File(file).getFileName(),
                        juce::dontSendNotification);
                }
            }
            else if (!processor.hasTargetAudio()) {
                if (processor.loadTargetAudio(juce::File(file))) {
                    targetInfoLabel.setText("Target: " + juce::File(file).getFileName(),
                        juce::dontSendNotification);
                }
            }
        }
    }
    repaint();
}

// ========================================================================
// LISTENERS
// ========================================================================

void AudioWorkshopEditor::comboBoxChanged(juce::ComboBox* combo) {
    if (combo == &featureSelector) {
        currentFeature = featureSelector.getText();
        updateOutputSelector();
        updateBreakpointDisplay();
    }
    else if (combo == &outputSelector) {
        currentOutput = outputSelector.getSelectedId() - 1;
        updateBreakpointDisplay();
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

void AudioWorkshopEditor::buttonClicked(juce::Button* button) {
    if (button == &loadSourceButton) {
        loadSourceAudio();
    }
    else if (button == &loadTargetButton) {
        loadTargetAudio();
    }
    else if (button == &loadBreakpointsButton) {
        loadBreakpointFile();
    }
    else if (button == &extractButton) {
        extractSelectedFeature();
    }
    else if (button == &extractAllButton) {
        extractAllFeatures();
    }
    else if (button == &extractADSREnvelopeButton) {
        extractADSRFromAmplitude();
    }
    else if (button == &reducePointsButton) {
        if (!currentFeature.isEmpty()) {
            int currentCount = processor.getCurrentBreakpointCount(currentFeature, currentOutput);
            int targetPoints = currentCount / 2;
            if (targetPoints < 10) targetPoints = 10;

            processor.decimateBreakpoints(currentFeature, currentOutput, targetPoints);
            updateBreakpointDisplay();
            statusLabel.setText("Reduced to " + juce::String(targetPoints) + " points",
                juce::dontSendNotification);
        }
    }
    else if (button == &quantizeToGridButton) {
        quantizeBreakpointsToGrid();
    }
    else if (button == &applyButton) {
        applyBreakpointsToTarget();
    }
    else if (button == &exportBreakpointsButton) {
        exportCurrentBreakpoints();
    }
    else if (button == &exportAudioButton) {
        exportProcessedAudio();
    }
    else if (button == &performEditButton) {
        performSelectedEdit();
    }
    else if (button == &clearAllButton) {
        clearAll();
    }
}

void AudioWorkshopEditor::sliderValueChanged(juce::Slider* slider) {
    // Parameters are automatically updated through AudioProcessorValueTreeState
}

// ========================================================================
// UI ACTION METHODS
// ========================================================================

void AudioWorkshopEditor::loadSourceAudio() {
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Source Audio",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.wav;*.aif;*.aiff;*.mp3;*.flac"
    );

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode |
        juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser) {
            auto result = chooser.getResult();
            if (result.existsAsFile()) {
                if (processor.loadSourceAudio(result)) {
                    sourceInfoLabel.setText("Source: " + result.getFileName(),
                        juce::dontSendNotification);
                    statusLabel.setText("Ready to extract features",
                        juce::dontSendNotification);
                    repaint();
                }
            }
        });
}

void AudioWorkshopEditor::loadTargetAudio() {
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Target Audio",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.wav;*.aif;*.aiff;*.mp3;*.flac"
    );

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode |
        juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser) {
            auto result = chooser.getResult();
            if (result.existsAsFile()) {
                if (processor.loadTargetAudio(result)) {
                    targetInfoLabel.setText("Target: " + result.getFileName(),
                        juce::dontSendNotification);
                    statusLabel.setText("Ready to apply breakpoints",
                        juce::dontSendNotification);
                    repaint();
                }
            }
        });
}

void AudioWorkshopEditor::loadBreakpointFile() {
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
                    statusLabel.setText("Loaded breakpoints: " + result.getFileName(),
                        juce::dontSendNotification);
                    updateOutputSelector();
                    updateBreakpointDisplay();
                }
            }
        });
}

void AudioWorkshopEditor::extractSelectedFeature() {
    if (!processor.hasSourceAudio()) {
        statusLabel.setText("Load source audio first", juce::dontSendNotification);
        return;
    }

    juce::String feature = featureSelector.getText();
    processor.extractFeature(feature, 0);

    currentFeature = feature;
    updateOutputSelector();
    updateBreakpointDisplay();

    statusLabel.setText("Extracted: " + feature, juce::dontSendNotification);
}

void AudioWorkshopEditor::extractAllFeatures() {
    if (!processor.hasSourceAudio()) {
        statusLabel.setText("Load source audio first", juce::dontSendNotification);
        return;
    }

    processor.extractAllFeatures();

    currentFeature = "Amplitude"; // Default to first feature
    updateOutputSelector();
    updateBreakpointDisplay();

    statusLabel.setText("Extracted all features", juce::dontSendNotification);
}

void AudioWorkshopEditor::extractADSRFromAmplitude() {
    if (!processor.hasSourceAudio()) {
        statusLabel.setText("Load source audio first", juce::dontSendNotification);
        return;
    }

    processor.extractADSRFromAmplitude();

    currentFeature = "ADSR Envelope";
    updateOutputSelector();
    updateBreakpointDisplay();

    statusLabel.setText("Extracted ADSR envelope", juce::dontSendNotification);
}

void AudioWorkshopEditor::updateBreakpointDisplay() {
    displayedBreakpoints.clear();

    if (!currentFeature.isEmpty()) {
        auto points = processor.getBreakpointsForDisplay(currentFeature, currentOutput);
        for (const auto& p : points) {
            displayedBreakpoints.push_back({ static_cast<float>(p.first),
                                            static_cast<float>(p.second) });
        }

        breakpointCountLabel.setText("Points: " + juce::String(points.size()),
            juce::dontSendNotification);
    }
    else {
        breakpointCountLabel.setText("Points: 0", juce::dontSendNotification);
    }
}

void AudioWorkshopEditor::updateOutputSelector() {
    outputSelector.clear();

    if (!currentFeature.isEmpty()) {
        int numOutputs = processor.getNumOutputsForFeature(currentFeature);
        for (int i = 0; i < numOutputs; ++i) {
            outputSelector.addItem(processor.getOutputName(currentFeature, i), i + 1);
        }

        if (numOutputs > 0) {
            currentOutput = 0;
            outputSelector.setSelectedId(1, juce::dontSendNotification);
        }
    }

    updateBreakpointDisplay();
}

void AudioWorkshopEditor::quantizeBreakpointsToGrid() {
    if (currentFeature.isEmpty()) {
        statusLabel.setText("Extract a feature first", juce::dontSendNotification);
        return;
    }

    processor.quantizeBreakpointsToGrid(currentFeature, currentOutput);

    updateBreakpointDisplay();
    statusLabel.setText(
        "Quantized to " + juce::String(processor.getTimeGridPPQN()) + " PPQN",
        juce::dontSendNotification
    );
}

void AudioWorkshopEditor::applyBreakpointsToTarget() {
    if (!processor.hasTargetAudio()) {
        statusLabel.setText("Load target audio first", juce::dontSendNotification);
        return;
    }

    if (!processor.hasBreakpoints()) {
        statusLabel.setText("Extract or load breakpoints first", juce::dontSendNotification);
        return;
    }

    statusLabel.setText("Applying breakpoints...", juce::dontSendNotification);
    processor.applyBreakpointsToTarget();
    statusLabel.setText("Applied! Ready to export", juce::dontSendNotification);
}

void AudioWorkshopEditor::exportCurrentBreakpoints() {
    if (currentFeature.isEmpty() || !processor.hasBreakpoints()) {
        statusLabel.setText("No breakpoints to export", juce::dontSendNotification);
        return;
    }

    juce::String defaultName = processor.hasSourceAudio() ?
        processor.getSourceFileName() + "_" + currentFeature + ".txt" :
        currentFeature + "_breakpoints.txt";

    fileChooser = std::make_unique<juce::FileChooser>(
        "Export Breakpoints",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile(defaultName),
        "*.txt"
    );

    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode |
        juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser) {
            auto result = chooser.getResult();
            if (result.getFullPathName().isNotEmpty()) {
                processor.saveBreakpoints(currentFeature, result);
                statusLabel.setText("Exported: " + result.getFileName(),
                    juce::dontSendNotification);
            }
        });
}

void AudioWorkshopEditor::exportProcessedAudio() {
    if (!processor.hasTargetAudio()) {
        statusLabel.setText("No processed audio to export", juce::dontSendNotification);
        return;
    }

    juce::String defaultName = processor.hasTargetAudio() ?
        processor.getTargetFileName() + "_processed.wav" :
        "processed_audio.wav";

    fileChooser = std::make_unique<juce::FileChooser>(
        "Export Processed Audio",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile(defaultName),
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

void AudioWorkshopEditor::performSelectedEdit() {
    if (!processor.hasTargetAudio()) {
        statusLabel.setText("Load target audio first", juce::dontSendNotification);
        return;
    }

    int selectedOp = editOperationSelector.getSelectedId();
    AudioWorkshopProcessor::EditOperation op;
    std::vector<double> params;

    switch (selectedOp) {
    case 1: // Remove Silence
        op = AudioWorkshopProcessor::EditOperation::RemoveSilence;
        params = { -40.0 }; // -40dB threshold
        break;

    case 2: // Split by Beats
        op = AudioWorkshopProcessor::EditOperation::SplitByBeats;
        break;

    case 3: // Isolate Transients
        op = AudioWorkshopProcessor::EditOperation::IsolateTransients;
        params = { 0.5 }; // Sensitivity
        break;

    case 4: // Time Stretch
        op = AudioWorkshopProcessor::EditOperation::TimeStretch;
        params = { 1.5 }; // 1.5x slower
        break;

    case 5: // Quantize Audio
        op = AudioWorkshopProcessor::EditOperation::Quantize;
        params = { 0.0 }; // Start time
        break;

    case 6: // Humanize
        op = AudioWorkshopProcessor::EditOperation::Humanize;
        params = { 0.1 }; // Humanize amount
        break;

    default:
        return;
    }

    auto result = processor.performEditOperation(op, params);

    if (result.getNumSamples() > 0) {
        statusLabel.setText("Edit completed: " + editOperationSelector.getText(),
            juce::dontSendNotification);
    }
    else {
        statusLabel.setText("Edit operation had no effect", juce::dontSendNotification);
    }

    repaint();
}

void AudioWorkshopEditor::clearAll() {
    processor.clearSourceAudio();
    processor.clearTargetAudio();
    processor.clearBreakpoints();
    displayedBreakpoints.clear();

    sourceInfoLabel.setText("Source: None", juce::dontSendNotification);
    targetInfoLabel.setText("Target: None", juce::dontSendNotification);
    breakpointCountLabel.setText("Points: 0", juce::dontSendNotification);
    statusLabel.setText("Ready", juce::dontSendNotification);
    extractionStatusLabel.setText("No features extracted", juce::dontSendNotification);
    applicationStatusLabel.setText("No audio processed", juce::dontSendNotification);

    repaint();
}

void AudioWorkshopEditor::updateStatus() {
    // Update extraction status
    auto extractedFeatures = processor.getExtractedFeatures();
    if (!extractedFeatures.isEmpty()) {
        extractionStatusLabel.setText("Extracted: " + extractedFeatures.joinIntoString(", "),
            juce::dontSendNotification);
    }
    else {
        extractionStatusLabel.setText("No features extracted", juce::dontSendNotification);
    }

    // Update application status
    if (processor.hasTargetAudio()) {
        applicationStatusLabel.setText("Target loaded: " + processor.getTargetFileName(),
            juce::dontSendNotification);
    }
    else {
        applicationStatusLabel.setText("No audio processed", juce::dontSendNotification);
    }
}

// ========================================================================
// MOUSE INTERACTION HELPER METHODS
// ========================================================================

int AudioWorkshopEditor::findBreakpointAtPosition(juce::Point<float> position, float tolerance) {
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
        float x = breakpointGraphBounds.getX() + (time / maxTime) * breakpointGraphBounds.getWidth();
        float normalizedValue = (value - minValue) / valueRange;
        float y = breakpointGraphBounds.getY() + breakpointGraphBounds.getHeight() * (1.0f - normalizedValue);

        if (std::abs(x - position.x) <= tolerance && std::abs(y - position.y) <= tolerance) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

juce::Point<float> AudioWorkshopEditor::timeValueToScreen(float time, float value) {
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

    float x = breakpointGraphBounds.getX() + (time / maxTime) * breakpointGraphBounds.getWidth();
    float normalizedValue = (value - minValue) / valueRange;
    float y = breakpointGraphBounds.getY() + breakpointGraphBounds.getHeight() * (1.0f - normalizedValue);

    return { x, y };
}

std::pair<float, float> AudioWorkshopEditor::screenToTimeValue(juce::Point<float> screenPos) {
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

    float time = ((screenPos.x - breakpointGraphBounds.getX()) / breakpointGraphBounds.getWidth()) * maxTime;
    float normalizedValue = 1.0f - ((screenPos.y - breakpointGraphBounds.getY()) / breakpointGraphBounds.getHeight());
    float value = minValue + normalizedValue * valueRange;

    return { juce::jmax(0.0f, time), value };
}

void AudioWorkshopEditor::updateBreakpointFromDrag(juce::Point<float> currentPosition) {
    if (draggedBreakpoint.index >= 0) {
        auto [newTime, newValue] = screenToTimeValue(currentPosition);
        processor.updateBreakpoint(currentFeature, currentOutput, draggedBreakpoint.index, newTime, newValue);
        updateBreakpointDisplay();
    }
}

void AudioWorkshopEditor::addBreakpointAtPosition(juce::Point<float> position) {
    if (breakpointGraphBounds.contains(position.toInt())) {
        auto [time, value] = screenToTimeValue(position);
        processor.addBreakpoint(currentFeature, currentOutput, time, value);
        updateBreakpointDisplay();
        statusLabel.setText("Added breakpoint at " + juce::String(time, 2) + "s",
            juce::dontSendNotification);
    }
}

void AudioWorkshopEditor::removeBreakpointAtPosition(juce::Point<float> position) {
    int index = findBreakpointAtPosition(position);
    if (index >= 0) {
        processor.removeBreakpoint(currentFeature, currentOutput, index);
        updateBreakpointDisplay();
        statusLabel.setText("Removed breakpoint " + juce::String(index),
            juce::dontSendNotification);
    }
}