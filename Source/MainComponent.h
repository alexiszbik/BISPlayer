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
    bool isLoggerVisible = true;
    
    int currentVideoIndex = 0;
    
    std::vector<Program> programs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
