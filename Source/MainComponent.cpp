#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : videoComponent (false)  // false = pas de contrôles natifs, on gère nous-mêmes
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);
    addAndMakeVisible (videoComponent);
    setAudioChannels (0, 2);
    
    // Initialiser l'entrée MIDI
    initializeMidiInput();
    
    // Charger automatiquement la vidéo Test.mp4 depuis ~/Documents/BIS
    auto docsDir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    auto videoFile = docsDir.getChildFile ("BIS").getChildFile ("Test.mp4");
    loadVideoFile (juce::URL (videoFile));
}

MainComponent::~MainComponent()
{
    // Fermer l'entrée MIDI
    midiInput.reset();
    
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // Le lecteur vidéo gère son propre rendu, pas besoin de dessiner ici
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    
    // Le lecteur vidéo prend toute la taille du composant
    videoComponent.setBounds (getLocalBounds());
}

void MainComponent::loadVideoFile (const juce::URL& videoURL)
{
    // Charger la vidéo de manière asynchrone (fonctionne sur toutes les plateformes)
    videoComponent.loadAsync (videoURL,
                              [this] (const juce::URL&, juce::Result result)
                              {
                                  if (result.wasOk())
                                  {
                                      videoComponent.setAudioVolume (1.0f);
                                      videoComponent.play();
                                  }
                                  else
                                  {
                                      juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                                              "Erreur",
                                                                              "Impossible de charger la vidéo:\n" + result.getErrorMessage());
                                  }
                              });
}

void MainComponent::handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message)
{
    // Ignorer le paramètre source si non utilisé
    juce::ignoreUnused (source);
    
    // Détecter les messages Note On
    if (message.isNoteOn())
    {
        const int noteNumber = message.getNoteNumber();
        const int velocity = message.getVelocity();
        const int channel = message.getChannel();
        
        // Ici vous pouvez ajouter votre code pour réagir aux messages Note On
        // Par exemple : changer de vidéo, contrôler la lecture, etc.
        
        /*juce::Logger::writeToLog ("Note On reçue - Note: " + juce::String (noteNumber) +
                                  ", Vélocité: " + juce::String (velocity) +
                                  ", Canal: " + juce::String (channel));*/
        std::cout << noteNumber << std::endl;
         
         
        
        // Exemple : si vous voulez faire quelque chose avec la vidéo
        // videoComponent.setPlayPosition (noteNumber / 127.0 * videoComponent.getVideoDuration());
    }
}

void MainComponent::initializeMidiInput()
{
    // Obtenir la liste des périphériques MIDI disponibles
    auto midiDevices = juce::MidiInput::getAvailableDevices();
    
    if (midiDevices.isEmpty())
    {
        juce::Logger::writeToLog ("Aucun périphérique MIDI trouvé");
        return;
    }
    
    // Ouvrir le premier périphérique MIDI disponible
    // Vous pouvez modifier cela pour permettre à l'utilisateur de choisir un périphérique spécifique
    midiInput = juce::MidiInput::openDevice (midiDevices[0].identifier, this);
    
    if (midiInput != nullptr)
    {
        midiInput->start();
        juce::Logger::writeToLog ("MIDI Input ouvert : " + midiDevices[0].name);
    }
    else
    {
        juce::Logger::writeToLog ("Impossible d'ouvrir le périphérique MIDI : " + midiDevices[0].name);
    }
}
