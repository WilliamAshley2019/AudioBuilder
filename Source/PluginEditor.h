// ============================================================================
// Audio Workshop - PluginEditor.h
// Unified editor for audio analysis, editing, and processing
// ============================================================================
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class AudioWorkshopEditor : public juce::AudioProcessorEditor,
    private juce::Timer,
    private juce::FileDragAndDropTarget,
    private juce::ComboBox::Listener,
    private juce::Button::Listener,
    private juce::Slider::Listener {
public:
    explicit AudioWorkshopEditor(AudioWorkshopProcessor&);
    ~AudioWorkshopEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

private:
    AudioWorkshopProcessor& processor;

    // ========================================================================
    // DUAL AUDIO SYSTEM UI
    // ========================================================================

    // Source audio (for extraction)
    juce::TextButton loadSourceButton;
    juce::Label sourceInfoLabel;
    juce::Rectangle<int> sourceWaveformBounds;

    // Target audio (for application)
    juce::TextButton loadTargetButton;
    juce::Label targetInfoLabel;
    juce::Rectangle<int> targetWaveformBounds;

    // ========================================================================
    // FEATURE EXTRACTION UI (from AudioDeconstructor)
    // ========================================================================

    juce::ComboBox featureSelector;
    juce::Label featureLabel;

    juce::ComboBox outputSelector;
    juce::Label outputLabel;

    juce::TextButton extractButton;
    juce::TextButton extractAllButton;
    juce::TextButton extractADSREnvelopeButton;

    juce::Slider windowSizeSlider;
    juce::Label windowSizeLabel;

    juce::Slider hopSizeSlider;
    juce::Label hopSizeLabel;

    juce::ToggleButton normalizeToggle;

    // ========================================================================
    // BREAKPOINT EDITOR UI (Unified)
    // ========================================================================

    juce::Rectangle<int> breakpointGraphBounds;
    std::vector<std::pair<float, float>> displayedBreakpoints;
    juce::String currentFeature;
    int currentOutput = 0;

    juce::TextButton reducePointsButton;
    juce::Label breakpointCountLabel;

    // ========================================================================
    // TIME LATTICE UI (from AudioBuilder)
    // ========================================================================

    juce::ComboBox ppqnSelector;
    juce::Label ppqnLabel;

    juce::ComboBox resolutionSelector;
    juce::Label resolutionLabel;

    juce::Slider tempoSlider;
    juce::Label tempoLabel;

    juce::TextButton quantizeToGridButton;
    juce::TextButton snapMarkersButton;

    // ========================================================================
    // EDIT OPERATIONS UI (Advanced)
    // ========================================================================

    juce::ComboBox editOperationSelector;
    juce::Label editOperationLabel;
    juce::TextButton performEditButton;
    juce::TextButton saveBreakpointsButton;
    juce::TextButton loadBreakpointsButton;

    // ========================================================================
    // APPLICATION UI (from AudioBuilder)
    // ========================================================================

    juce::Slider intensitySlider;
    juce::Label intensityLabel;

    juce::ToggleButton smoothingToggle;

    juce::TextButton applyButton;
    juce::TextButton exportBreakpointsButton;
    juce::TextButton exportAudioButton;

    juce::TextButton clearAllButton;

    // ========================================================================
    // STATUS & INFO
    // ========================================================================

    juce::Label statusLabel;
    juce::Label extractionStatusLabel;
    juce::Label applicationStatusLabel;

    std::unique_ptr<juce::FileChooser> fileChooser;

    // ========================================================================
    // MOUSE INTERACTION
    // ========================================================================

    struct DraggedBreakpoint {
        int index = -1;
        juce::Point<float> dragStartPosition;
    };
    DraggedBreakpoint draggedBreakpoint;
    bool isDragging = false;

    // ========================================================================
    // METHODS
    // ========================================================================

    // Timer callback
    void timerCallback() override;

    // File drag and drop
    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray& files, int, int) override;

    // Listeners
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked(juce::Button* button) override;
    void sliderValueChanged(juce::Slider* slider) override;

    // Drawing methods
    void drawSourceWaveform(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawTargetWaveform(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawBreakpoints(juce::Graphics& g, const juce::Rectangle<int>& area);
    void drawGraphBackground(juce::Graphics& g, const juce::Rectangle<int>& area);

    // Mouse interaction helpers
    int findBreakpointAtPosition(juce::Point<float> position, float tolerance = 10.0f);
    juce::Point<float> timeValueToScreen(float time, float value);
    std::pair<float, float> screenToTimeValue(juce::Point<float> screenPos);
    void updateBreakpointFromDrag(juce::Point<float> currentPosition);
    void addBreakpointAtPosition(juce::Point<float> position);
    void removeBreakpointAtPosition(juce::Point<float> position);

    // UI actions
    void loadSourceAudio();
    void loadTargetAudio();
    void loadBreakpointFile();
    void extractSelectedFeature();
    void extractAllFeatures();
    void extractADSRFromAmplitude();
    void updateBreakpointDisplay();
    void updateOutputSelector();
    void quantizeBreakpointsToGrid();
    void applyBreakpointsToTarget();
    void exportCurrentBreakpoints();
    void exportProcessedAudio();
    void performSelectedEdit();
    void clearAll();
    void updateStatus();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioWorkshopEditor)
};