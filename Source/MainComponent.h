#pragma once

#include <JuceHeader.h>
#include "ComponentLogger.h"
#include "Program.h"
#include "CameraCapture.h"
#include "MidiManager.h"

#define FIRST_NOTE 36

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent,
                        public juce::Timer,
                        public juce::ComboBox::Listener,
                        public juce::Slider::Listener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    
    //==============================================================================
    // Keyboard handling
    bool keyPressed (const juce::KeyPress& key) override;
    
    //==============================================================================
    // Méthodes pour envoyer des messages MIDI (déléguées à MidiManager)
    void sendNoteOn (int channel, int noteNumber, uint8 velocity, bool log = true);
    void sendNoteOff (int channel, int noteNumber, uint8 velocity, bool log = true);
    void sendProgramChange (int channel, int programNumber);
    void sendControlChange (int channel, int controllerNumber, int controllerValue);
    
    void updateLoggerVisibility();
private:

    void loadProgram(Program* pgm);
    void loadVideoFile (const juce::URL& videoURL);
    
    // Gestion des messages MIDI entrants (appelée par MidiManager)
    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message);
    
    // ComboBox::Listener (pour le thresholdSlider uniquement)
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;
    
    // Slider::Listener
    void sliderValueChanged (juce::Slider* slider) override;
    
    void scanPrograms();
    
    // Timer pour vérifier la fin de la vidéo
    void timerCallback() override;
    
    void stopAndHideVideo();
    void idle();
    
    void setCaptureMode(bool state);

private:
    //==============================================================================
    // Your private member variables go here...
    juce::VideoComponent videoComponent;
    std::unique_ptr<CameraCapture> capture;
    
    // TextEditor pour afficher les logs
    juce::TextEditor logTextEditor;
    
    // Slider pour contrôler le threshold de la caméra
    juce::Slider thresholdSlider;
    juce::Label thresholdLabel;
    
    // ComboBox pour sélectionner les périphériques MIDI
    juce::ComboBox midiInputComboBox;
    juce::ComboBox midiOutputComboBox;
    juce::Label midiInputLabel;
    juce::Label midiOutputLabel;
    
    // Logger personnalisé
    std::unique_ptr<ComponentLogger> componentLogger;
    
    // Gestionnaire MIDI
    std::unique_ptr<MidiManager> midiManager;
    
    // État de visibilité du logger
    bool isLoggerVisible = false;
    
    int currentVideoIndex = 0;
    
    std::vector<Program> programs;
    
    File idleVideoFile;
    
    bool videoIsloading = false;
    bool captureMode = false;
    
    // Timestamp du dernier Note On traité (pour limiter à 1 par seconde)
    double lastNoteOnTime = 0.0;
    static constexpr double NOTE_ON_THROTTLE_MS = 1000.0;  // 1 seconde
    
    bool ledState = false;
    bool ledStateChanged = false;
    
    int newProgram = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
