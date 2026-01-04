#pragma once

#include <JuceHeader.h>
#include "ComponentLogger.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent,
                        public juce::MidiInputCallback
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
    // MidiInputCallback
    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override;
    
    //==============================================================================
    // Méthodes pour envoyer des messages MIDI
    void sendNoteOn (int channel, int noteNumber, float velocity);
    void sendNoteOff (int channel, int noteNumber, float velocity = 0.0f);
    void sendProgramChange (int channel, int programNumber);
    void sendControlChange (int channel, int controllerNumber, int controllerValue);

private:
    //==============================================================================
    // Your private member variables go here...
    juce::VideoComponent videoComponent;
    
    // TextEditor pour afficher les logs
    juce::TextEditor logTextEditor;
    
    // Logger personnalisé
    std::unique_ptr<ComponentLogger> componentLogger;
    
    // Entrée MIDI
    std::unique_ptr<juce::MidiInput> midiInput;
    
    // Sortie MIDI
    std::unique_ptr<juce::MidiOutput> midiOutput;
    
    // Tableau des URLs des fichiers vidéo
    juce::Array<juce::URL> videoUrls;
    
    // État de visibilité du logger
    bool isLoggerVisible = true;
    
    // Méthode pour charger une vidéo depuis une URL
    void loadVideoFile (const juce::URL& videoURL);
    
    // Méthode pour initialiser l'entrée MIDI
    void initializeMidiInput();
    
    // Méthode pour initialiser la sortie MIDI
    void initializeMidiOutput();
    
    // Méthode pour scanner le dossier BIS et remplir videoUrls
    void scanVideoFiles();
    
    int currentVideoIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
