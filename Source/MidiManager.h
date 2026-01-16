#pragma once

#include <JuceHeader.h>

#define MIN_NOTE 36
#define MAX_NOTE 96

#define MIN_LED 60
#define MAX_LED 72


//==============================================================================
/**
    Classe pour gérer les périphériques MIDI et l'envoi/réception de messages MIDI.
*/
class MidiManager : public juce::MidiInputCallback,
                     public juce::ComboBox::Listener
{
public:
    //==============================================================================
    MidiManager();
    ~MidiManager() override;

    //==============================================================================
    // Configuration des composants UI
    void setupComboBoxes (juce::ComboBox* inputComboBox, 
                          juce::ComboBox* outputComboBox,
                          juce::Label* inputLabel,
                          juce::Label* outputLabel);
    
    // Mettre à jour les listes de périphériques disponibles
    void updateDeviceLists();
    
    // Initialiser les périphériques MIDI
    void initializeDevices();
    
    //==============================================================================
    // MidiInputCallback
    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override;
    
    //==============================================================================
    // ComboBox::Listener
    void comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged) override;
    
    void sendByteAsMidiForPrinter(uint8_t b);
    
    //==============================================================================
    // Méthodes pour envoyer des messages MIDI
    void sendNoteOn (int channel, int noteNumber, uint8_t velocity, bool log = true);
    void sendNoteOff (int channel, int noteNumber, uint8_t velocity, bool log = true);
    void sendProgramChange (int channel, int programNumber);
    void sendControlChange (int channel, int controllerNumber, int controllerValue);
    
    //==============================================================================
    // Callback pour les messages MIDI entrants (optionnel)
    std::function<void(juce::MidiInput*, const juce::MidiMessage&)> onMidiMessageReceived;
public:
    int lastNote = 0;
    int lastLed = 0;
private:
    //==============================================================================
    // Méthodes privées
    void initializeMidiInput();
    void initializeMidiOutput();
    
    //==============================================================================
    // Membres
    juce::ComboBox* midiInputComboBox = nullptr;
    juce::ComboBox* midiOutputComboBox = nullptr;
    juce::Label* midiInputLabel = nullptr;
    juce::Label* midiOutputLabel = nullptr;
    
    std::unique_ptr<juce::MidiInput> midiInput;
    std::unique_ptr<juce::MidiOutput> midiOutput;
    
    bool enableLogging = true;

    bool allNotesState[60];
    int oct = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiManager)
};
