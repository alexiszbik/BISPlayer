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
    
    // Scanner le dossier BIS pour trouver tous les fichiers vidéo
    scanVideoFiles();
    
    // Charger automatiquement la première vidéo si disponible
    if (videoUrls.size() > 0)
    {
        loadVideoFile (videoUrls[0]);
    }
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
    juce::ignoreUnused (source);
    if (message.isNoteOn())
    {
        const int noteNumber = message.getNoteNumber();
        const int velocity = message.getVelocity();
        const int channel = message.getChannel();
        
        std::cout << noteNumber << std::endl;
         
        currentVideoIndex++;
        currentVideoIndex = currentVideoIndex % videoUrls.size();
        
        loadVideoFile (videoUrls[currentVideoIndex]);
    }
}

void MainComponent::scanVideoFiles()
{
    videoUrls.clear();
    
    auto docsDir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    auto bisDir = docsDir.getChildFile ("BIS");
    
    if (! bisDir.isDirectory())
    {
        jassert(false && "Le dossier ~/Documents/BIS n'existe pas");
        return;
    }
    
    juce::Array<juce::File> files;
    bisDir.findChildFiles (files, juce::File::findFiles, false);

    for (const auto& file : files)
    {
        auto extension = file.getFileExtension().toLowerCase();
        
        if (extension == ".mov" || extension == ".mp4")
        {
            videoUrls.add (juce::URL (file));
        }
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
