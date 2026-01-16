#include "MidiManager.h"

//==============================================================================
MidiManager::MidiManager()
{
}

MidiManager::~MidiManager()
{
    // Fermer l'entrée MIDI
    if (midiInput != nullptr)
    {
        midiInput->stop();
        midiInput.reset();
    }
    
    // Fermer la sortie MIDI
    midiOutput.reset();
}

//==============================================================================
void MidiManager::setupComboBoxes (juce::ComboBox* inputComboBox,
                                    juce::ComboBox* outputComboBox,
                                    juce::Label* inputLabel,
                                    juce::Label* outputLabel)
{
    midiInputComboBox = inputComboBox;
    midiOutputComboBox = outputComboBox;
    midiInputLabel = inputLabel;
    midiOutputLabel = outputLabel;
    
    if (midiInputComboBox != nullptr)
        midiInputComboBox->addListener (this);
    
    if (midiOutputComboBox != nullptr)
        midiOutputComboBox->addListener (this);
}

void MidiManager::updateDeviceLists()
{
    // Mettre à jour la liste des périphériques MIDI Input
    if (midiInputComboBox != nullptr)
    {
        midiInputComboBox->clear();
        midiInputComboBox->addItem ("None", 1);
        
        auto inputDevices = juce::MidiInput::getAvailableDevices();
        for (int i = 0; i < inputDevices.size(); ++i)
        {
            midiInputComboBox->addItem (inputDevices[i].name, i + 2);
        }
        
        // Sélectionner le premier périphérique par défaut (ou "None" si aucun)
        if (inputDevices.size() > 0)
            midiInputComboBox->setSelectedId (2);
        else
            midiInputComboBox->setSelectedId (1);
    }
    
    // Mettre à jour la liste des périphériques MIDI Output
    if (midiOutputComboBox != nullptr)
    {
        midiOutputComboBox->clear();
        midiOutputComboBox->addItem ("None", 1);
        
        auto outputDevices = juce::MidiOutput::getAvailableDevices();
        for (int i = 0; i < outputDevices.size(); ++i)
        {
            midiOutputComboBox->addItem (outputDevices[i].name, i + 2);
        }
        
        // Sélectionner le premier périphérique par défaut (ou "None" si aucun)
        if (outputDevices.size() > 0)
            midiOutputComboBox->setSelectedId (2);
        else
            midiOutputComboBox->setSelectedId (1);
    }
}

void MidiManager::initializeDevices()
{
    initializeMidiInput();
    initializeMidiOutput();
}

//==============================================================================
void MidiManager::handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message)
{
    // Si un callback est défini, l'appeler
    if (onMidiMessageReceived)
    {
        onMidiMessageReceived (source, message);
    }
}

//==============================================================================
void MidiManager::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == midiInputComboBox)
    {
        // Fermer l'ancien périphérique
        if (midiInput != nullptr)
        {
            midiInput->stop();
            midiInput.reset();
        }
        
        // Ouvrir le nouveau périphérique si sélectionné
        int selectedId = midiInputComboBox->getSelectedId();
        if (selectedId > 1)  // Pas "None"
        {
            auto devices = juce::MidiInput::getAvailableDevices();
            int deviceIndex = selectedId - 2;
            
            if (deviceIndex >= 0 && deviceIndex < devices.size())
            {
                midiInput = juce::MidiInput::openDevice (devices[deviceIndex].identifier, this);
                if (midiInput != nullptr)
                {
                    midiInput->start();
                    if (enableLogging)
                    {
                        juce::Logger::writeToLog ("MIDI Input opened: " + devices[deviceIndex].name);
                    }
                }
            }
        }
        else
        {
            if (enableLogging)
            {
                juce::Logger::writeToLog ("MIDI Input closed");
            }
        }
    }
    else if (comboBoxThatHasChanged == midiOutputComboBox)
    {
        // Fermer l'ancien périphérique
        midiOutput.reset();
        
        // Ouvrir le nouveau périphérique si sélectionné
        int selectedId = midiOutputComboBox->getSelectedId();
        if (selectedId > 1)  // Pas "None"
        {
            auto devices = juce::MidiOutput::getAvailableDevices();
            int deviceIndex = selectedId - 2;
            
            if (deviceIndex >= 0 && deviceIndex < devices.size())
            {
                midiOutput = juce::MidiOutput::openDevice (devices[deviceIndex].identifier);
                if (midiOutput != nullptr)
                {
                    if (enableLogging)
                    {
                        juce::Logger::writeToLog ("MIDI Output opened: " + devices[deviceIndex].name);
                    }
                }
                else
                {
                    if (enableLogging)
                    {
                        juce::Logger::writeToLog ("Failed to open MIDI output device: " + devices[deviceIndex].name);
                    }
                }
            }
        }
        else
        {
            if (enableLogging)
            {
                juce::Logger::writeToLog ("MIDI Output closed");
            }
        }
    }
}

//==============================================================================
void MidiManager::sendNoteOn (int channel, int noteNumber, uint8_t velocity, bool log)
{
    if (midiOutput != nullptr)
    {
        juce::MidiMessage message = juce::MidiMessage::noteOn (channel, noteNumber, velocity);
        midiOutput->sendMessageNow (message);
        
        if (log && enableLogging)
        {
            juce::Logger::writeToLog ("MIDI OUT Note On - Channel: " + juce::String (channel) +
                                       ", Note: " + juce::String (noteNumber) +
                                       ", Velocity: " + juce::String (velocity));
        }
    }
}

void MidiManager::sendNoteOff (int channel, int noteNumber, uint8_t velocity, bool log)
{
    if (midiOutput != nullptr)
    {
        juce::MidiMessage message = juce::MidiMessage::noteOff (channel, noteNumber, velocity);
        midiOutput->sendMessageNow (message);
        
        if (log && enableLogging)
        {
            juce::Logger::writeToLog ("MIDI OUT Note Off - Channel: " + juce::String (channel) +
                                       ", Note: " + juce::String (noteNumber) +
                                       ", Velocity: " + juce::String (velocity));
        }
    }
}

void MidiManager::sendProgramChange (int channel, int programNumber)
{
    if (midiOutput != nullptr)
    {
        juce::MidiMessage message = juce::MidiMessage::programChange (channel, programNumber);
        midiOutput->sendMessageNow (message);
        
        if (enableLogging)
        {
            juce::Logger::writeToLog ("MIDI OUT Program Change - Channel: " + juce::String (channel) + 
                                       ", Program: " + juce::String (programNumber));
        }
    }
}

void MidiManager::sendControlChange (int channel, int controllerNumber, int controllerValue)
{
    if (midiOutput != nullptr)
    {
        juce::MidiMessage message = juce::MidiMessage::controllerEvent (channel, controllerNumber, controllerValue);
        midiOutput->sendMessageNow (message);
        
        if (enableLogging)
        {
            juce::Logger::writeToLog ("MIDI OUT Control Change - Channel: " + juce::String (channel) + 
                                       ", CC: " + juce::String (controllerNumber) + 
                                       ", Value: " + juce::String (controllerValue));
        }
    }
}

void MidiManager::sendByteAsMidiForPrinter (uint8_t b)
{
    uint8_t note = b & 0x7F;
    uint8_t vel  = (b & 0x80) ? 1 : 127;
    //send midi on printer
    sendNoteOn (15, note, vel, false);
    
    oct++;
    
    uint8_t noteForNotes = ((int)note + oct*12) % 60;
    
    if (oct > 5) {
        oct = 0;
    }
    
    bool* state = &allNotesState[noteForNotes];
    *state = !*state;
    
    if(*state) {
        lastNote = noteForNotes;
    }
    
    sendNoteOn (1, (noteForNotes + MIN_NOTE), *state ? 127 : 0, false);
    
    uint8_t noteForLeds = ((int)note + oct*3) % 12;
    
    noteForLeds = noteForLeds + MIN_LED;
    
    sendNoteOn (10, lastLed, 0, false);
    sendNoteOn (10, noteForLeds, 127, false);
    
    lastLed = noteForLeds;
    
    juce::Thread::sleep (1);
}

//==============================================================================
void MidiManager::initializeMidiInput()
{
    // Obtenir la liste des périphériques MIDI disponibles
    auto midiDevices = juce::MidiInput::getAvailableDevices();
    
    if (midiDevices.isEmpty())
    {
        if (enableLogging)
        {
            juce::Logger::writeToLog ("No MIDI input");
        }
        return;
    }
    
    // Ouvrir le périphérique sélectionné dans la ComboBox (ou le premier par défaut)
    if (midiInputComboBox != nullptr)
    {
        int selectedId = midiInputComboBox->getSelectedId();
        int deviceIndex = (selectedId > 1) ? (selectedId - 2) : 0;
        
        if (deviceIndex >= 0 && deviceIndex < midiDevices.size())
        {
            midiInput = juce::MidiInput::openDevice (midiDevices[deviceIndex].identifier, this);
            
            if (midiInput != nullptr)
            {
                midiInput->start();
                if (enableLogging)
                {
                    juce::Logger::writeToLog ("MIDI Input opened: " + midiDevices[deviceIndex].name);
                }
            }
            else
            {
                if (enableLogging)
                {
                    juce::Logger::writeToLog ("Failed to open MIDI device: " + midiDevices[deviceIndex].name);
                }
            }
        }
    }
}

void MidiManager::initializeMidiOutput()
{
    // Obtenir la liste des périphériques MIDI de sortie disponibles
    auto midiDevices = juce::MidiOutput::getAvailableDevices();
    
    if (midiDevices.isEmpty())
    {
        if (enableLogging)
        {
            juce::Logger::writeToLog ("No MIDI output device found");
        }
        return;
    }
    
    // Ouvrir le périphérique sélectionné dans la ComboBox (ou le premier par défaut)
    if (midiOutputComboBox != nullptr)
    {
        int selectedId = midiOutputComboBox->getSelectedId();
        int deviceIndex = (selectedId > 1) ? (selectedId - 2) : 0;
        
        if (deviceIndex >= 0 && deviceIndex < midiDevices.size())
        {
            midiOutput = juce::MidiOutput::openDevice (midiDevices[deviceIndex].identifier);
            
            if (midiOutput != nullptr)
            {
                if (enableLogging)
                {
                    juce::Logger::writeToLog ("MIDI Output opened: " + midiDevices[deviceIndex].name);
                }
            }
            else
            {
                if (enableLogging)
                {
                    juce::Logger::writeToLog ("Failed to open MIDI output device: " + midiDevices[deviceIndex].name);
                }
            }
        }
    }
}
