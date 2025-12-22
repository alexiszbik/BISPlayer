#pragma once

#include <JuceHeader.h>

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
    // MidiInputCallback
    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override;

private:
    //==============================================================================
    // Your private member variables go here...
    juce::VideoComponent videoComponent;
    
    // Entrée MIDI
    std::unique_ptr<juce::MidiInput> midiInput;
    
    // Méthode pour charger une vidéo depuis une URL
    void loadVideoFile (const juce::URL& videoURL);
    
    // Méthode pour initialiser l'entrée MIDI
    void initializeMidiInput();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
