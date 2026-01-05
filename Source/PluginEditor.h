// ============================================================================
// AudioBuilder - PluginEditor.h
// ============================================================================
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class AudioBuilderEditor : public juce::AudioProcessorEditor,
    private juce::Timer,
    private juce::FileDragAndDropTarget,
    private juce::ComboBox::Listener,
    private juce::Button::Listener,
    private juce::Slider::Listener {
public:
    explicit AudioBuilderEditor(AudioBuilderProcessor&);
    ~AudioBuilderEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

private:
    AudioBuilderProcessor& processor;

    // UI Components
    juce::TextButton loadAudioButton;
    juce::TextButton loadBreakpointsButton;
    juce::TextButton applyButton;
    juce::TextButton exportButton;
    juce::TextButton clearButton;
    juce::TextButton decimateButton;
    juce::TextButton quantizeToGridButton;

    // Edit operations
    juce::ComboBox editOperationSelector;
    juce::Label editOperationLabel;
    juce::TextButton performEditButton;

    // PPQN and resolution controls
    juce::ComboBox ppqnSelector;
    juce::Label ppqnLabel;
    juce::ComboBox resolutionSelector;
    juce::Label resolutionLabel;

    juce::ComboBox outputSelector;
    juce::Label outputLabel;
    juce::Label featureLabel;
    juce::Label infoLabel;
    juce::Label statusLabel;

    juce::Slider intensitySlider;
    juce::Label intensityLabel;
    juce::ToggleButton smoothingToggle;

    std::unique_ptr<juce::FileChooser> fileChooser;

    // Display state
    juce::Rectangle<int> graphBounds;
    std::vector<std::pair<float, float>> displayedBreakpoints;
    int currentOutput = 0;

    struct DraggedBreakpoint {
        int index = -1;
        juce::Point<float> dragStartPosition;
    };
    DraggedBreakpoint draggedBreakpoint;
    bool isDragging = false;

    // Timer callback
    void timerCallback() override;

    // File drag and drop
    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray& files, int, int) override;

    // Listeners
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked(juce::Button* button) override;
    void sliderValueChanged(juce::Slider* slider) override;

    // Mouse interaction helpers
    int findBreakpointAtPosition(juce::Point<float> position, float tolerance = 10.0f);
    juce::Point<float> timeValueToScreen(float time, float value);
    std::pair<float, float> screenToTimeValue(juce::Point<float> screenPos);
    void updateBreakpointFromDrag(juce::Point<float> currentPosition);
    void addBreakpointAtPosition(juce::Point<float> position);
    void removeBreakpointAtPosition(juce::Point<float> position);

    // UI actions
    void loadAudioFile();
    void loadBreakpointFile();
    void applyBreakpoints();
    void exportAudio();
    void clearAll();
    void decimateCurrentBreakpoints();
    void updateDisplay();
    void updateOutputSelector();

    // Drawing
    void drawGraphBackground(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawWaveform(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawBreakpoints(juce::Graphics& g, const juce::Rectangle<int>& area);

    void quantizeBreakpointsToGrid();
    void performSelectedEdit();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioBuilderEditor)
};