#include "MainComponent.h"

#include <random>

int randomBetween(int x, int y) {
    static std::mt19937 rng{std::random_device{}()};
    if (x > y) std::swap(x, y);
    std::uniform_int_distribution<int> dist(x, y);
    return dist(rng);
}

//==============================================================================
MainComponent::MainComponent()
    : videoComponent (false)  // false = pas de contrôles natifs, on gère nous-mêmes
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (1200, 600);
    
    // Configurer le TextEditor pour les logs
    logTextEditor.setMultiLine (true);
    logTextEditor.setReturnKeyStartsNewLine (true);
    logTextEditor.setReadOnly (true);
    logTextEditor.setCaretVisible (false);
    logTextEditor.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    logTextEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);
    logTextEditor.setColour (juce::TextEditor::textColourId, juce::Colours::lightgreen);
    
    addAndMakeVisible (videoComponent);
    addAndMakeVisible (logTextEditor);
    
    // Activer le focus clavier pour recevoir les événements de touches
    setWantsKeyboardFocus (true);
    
    // Créer le logger personnalisé
    componentLogger = std::make_unique<ComponentLogger> (&logTextEditor);
    juce::Logger::setCurrentLogger (componentLogger.get());
    
    // Configurer le callback pour détecter la fin de la vidéo
    // Utiliser MessageManager::callAsync pour s'assurer que l'appel est thread-safe
    videoComponent.onPlaybackStopped = [this]()
    {
        juce::MessageManager::callAsync ([this]()
        {
            // Vérifier si la vidéo est arrivée à la fin
            if (videoComponent.isVideoOpen())
            {
                auto duration = videoComponent.getVideoDuration();
                auto position = videoComponent.getPlayPosition();
                
                // Si on est proche de la fin (à moins de 0.1 seconde), passer à la suivante
                if (duration > 0 && position >= duration - 0.1)
                {
                    playNextVideo();
                }
            }
        });
    };
    
    // Démarrer le timer pour vérifier périodiquement la fin de la vidéo
    startTimer (100);  // Vérifier toutes les 100ms
    
    setAudioChannels (0, 2);
    
    // Initialiser l'entrée MIDI
    initializeMidiInput();
    
    // Initialiser la sortie MIDI
    initializeMidiOutput();
    
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
    // Arrêter le timer
    stopTimer();
    
    // Nettoyer les callbacks du VideoComponent pour éviter les appels après destruction
    videoComponent.onPlaybackStopped = nullptr;
    videoComponent.onPlaybackStarted = nullptr;
    
    // Désenregistrer le logger avant de le détruire
    juce::Logger::setCurrentLogger (nullptr);
    
    // Fermer l'entrée MIDI
    midiInput.reset();
    
    // Fermer la sortie MIDI
    midiOutput.reset();
    
    // Fermer le logger
    componentLogger.reset();
    
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
    // Fond noir
    g.fillAll (juce::Colours::black);
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    
    auto bounds = getLocalBounds();
    
    if (isLoggerVisible)
    {
        // Diviser l'espace : vidéo à gauche, log à droite
        auto videoBounds = bounds.removeFromLeft (bounds.getWidth() * 2 / 3);
        videoComponent.setBounds (videoBounds);
        
        // Le TextEditor prend le reste de l'espace à droite
        logTextEditor.setBounds (bounds);
    }
    else
    {
        // Le lecteur vidéo prend toute la taille du composant
        videoComponent.setBounds (bounds);
        logTextEditor.setBounds (0, 0, 0, 0);  // Caché
    }
}

bool MainComponent::keyPressed (const juce::KeyPress& key)
{
    // Toggle la visibilité du logger avec la touche K (insensible à la casse)
    if (key.getTextCharacter() == 'k' || key.getTextCharacter() == 'K')
    {
        isLoggerVisible = !isLoggerVisible;
        logTextEditor.setVisible (isLoggerVisible);
        resized();  // Recalculer le layout
        return true;  // Consommer l'événement
    }
    
    return false;  // Laisser passer les autres touches
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
                                      juce::Logger::writeToLog ("Failed to load video: " + result.getErrorMessage());
                                  }
                              });
    
    
    sendProgramChange(15, randomBetween(71, 73));
}

void MainComponent::handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message)
{
    juce::String sourceName = "Unknown";
    if (source != nullptr)
    {
        auto deviceInfo = source->getDeviceInfo();
        sourceName = deviceInfo.name.isNotEmpty() ? deviceInfo.name : "Unknown";
    }
    juce::String logMessage;
    
    // Logger tous les types de messages MIDI entrants
    if (message.isNoteOn())
    {
        const int noteNumber = message.getNoteNumber();
        const int velocity = message.getVelocity();
        const int channel = message.getChannel();
        
        logMessage = "MIDI IN [" + sourceName + "] Note On - Channel: " + juce::String (channel) + 
                     ", Note: " + juce::String (noteNumber) + 
                     ", Velocity: " + juce::String (velocity);
        
        juce::Logger::writeToLog (logMessage);
        
        currentVideoIndex++;
        currentVideoIndex = currentVideoIndex % videoUrls.size();
        
        loadVideoFile (videoUrls[currentVideoIndex]);
    }
    else if (message.isNoteOff())
    {
        const int noteNumber = message.getNoteNumber();
        const int velocity = message.getVelocity();
        const int channel = message.getChannel();
        
        logMessage = "MIDI IN [" + sourceName + "] Note Off - Channel: " + juce::String (channel) + 
                     ", Note: " + juce::String (noteNumber) + 
                     ", Velocity: " + juce::String (velocity);
        
        juce::Logger::writeToLog (logMessage);
    }
    else if (message.isProgramChange())
    {
        const int programNumber = message.getProgramChangeNumber();
        const int channel = message.getChannel();
        
        logMessage = "MIDI IN [" + sourceName + "] Program Change - Channel: " + juce::String (channel) + 
                     ", Program: " + juce::String (programNumber);
        
        juce::Logger::writeToLog (logMessage);
    }
    else if (message.isController())
    {
        const int controllerNumber = message.getControllerNumber();
        const int controllerValue = message.getControllerValue();
        const int channel = message.getChannel();
        
        logMessage = "MIDI IN [" + sourceName + "] Control Change - Channel: " + juce::String (channel) + 
                     ", CC: " + juce::String (controllerNumber) + 
                     ", Value: " + juce::String (controllerValue);
        
        juce::Logger::writeToLog (logMessage);
    }
}

void MainComponent::scanVideoFiles()
{
    videoUrls.clear();
    
    auto docsDir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    auto bisDir = docsDir.getChildFile ("BIS");
    
    if (! bisDir.isDirectory())
    {
        juce::Logger::writeToLog ("Directory ~/Documents/BIS does not exist");
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
        juce::Logger::writeToLog ("No MIDI input");
        return;
    }
    
    // Ouvrir le premier périphérique MIDI disponible
    // Vous pouvez modifier cela pour permettre à l'utilisateur de choisir un périphérique spécifique
    midiInput = juce::MidiInput::openDevice (midiDevices[0].identifier, this);
    
    if (midiInput != nullptr)
    {
        midiInput->start();
        juce::Logger::writeToLog ("MIDI Input opened: " + midiDevices[0].name);
    }
    else
    {
        juce::Logger::writeToLog ("Failed to open MIDI device: " + midiDevices[0].name);
    }
}

void MainComponent::initializeMidiOutput()
{
    // Obtenir la liste des périphériques MIDI de sortie disponibles
    auto midiDevices = juce::MidiOutput::getAvailableDevices();
    
    if (midiDevices.isEmpty())
    {
        juce::Logger::writeToLog ("No MIDI output device found");
        return;
    }
    
    // Ouvrir le premier périphérique MIDI de sortie disponible
    // Vous pouvez modifier cela pour permettre à l'utilisateur de choisir un périphérique spécifique
    midiOutput = juce::MidiOutput::openDevice (midiDevices[0].identifier);
    
    if (midiOutput != nullptr)
    {
        juce::Logger::writeToLog ("MIDI Output opened: " + midiDevices[0].name);
    }
    else
    {
        juce::Logger::writeToLog ("Failed to open MIDI output device: " + midiDevices[0].name);
    }
}

void MainComponent::sendNoteOn (int channel, int noteNumber, float velocity)
{
    if (midiOutput != nullptr)
    {
        // Les canaux MIDI sont de 1-16, mais MidiMessage utilise 0-15
        juce::MidiMessage message = juce::MidiMessage::noteOn (channel, noteNumber, velocity);
        midiOutput->sendMessageNow (message);
        
        juce::Logger::writeToLog ("MIDI OUT Note On - Channel: " + juce::String (channel) + 
                                   ", Note: " + juce::String (noteNumber) + 
                                   ", Velocity: " + juce::String ((int)(velocity * 127)));
    }
}

void MainComponent::sendNoteOff (int channel, int noteNumber, float velocity)
{
    if (midiOutput != nullptr)
    {
        juce::MidiMessage message = juce::MidiMessage::noteOff (channel, noteNumber, velocity);
        midiOutput->sendMessageNow (message);
        
        juce::Logger::writeToLog ("MIDI OUT Note Off - Channel: " + juce::String (channel) + 
                                   ", Note: " + juce::String (noteNumber) + 
                                   ", Velocity: " + juce::String ((int)(velocity * 127)));
    }
}

void MainComponent::sendProgramChange (int channel, int programNumber)
{
    if (midiOutput != nullptr)
    {
        // Les canaux MIDI sont de 1-16, mais MidiMessage utilise 0-15
        // Les numéros de programme sont de 0-127
        juce::MidiMessage message = juce::MidiMessage::programChange (channel, programNumber);
        midiOutput->sendMessageNow (message);
        
        juce::Logger::writeToLog ("MIDI OUT Program Change - Channel: " + juce::String (channel) + 
                                   ", Program: " + juce::String (programNumber));
    }
}

void MainComponent::sendControlChange (int channel, int controllerNumber, int controllerValue)
{
    if (midiOutput != nullptr)
    {
        // Les canaux MIDI sont de 1-16, mais MidiMessage utilise 0-15
        // Les numéros de contrôleur et valeurs sont de 0-127
        juce::MidiMessage message = juce::MidiMessage::controllerEvent (channel, controllerNumber, controllerValue);
        midiOutput->sendMessageNow (message);
        
        juce::Logger::writeToLog ("MIDI OUT Control Change - Channel: " + juce::String (channel) + 
                                   ", CC: " + juce::String (controllerNumber) + 
                                   ", Value: " + juce::String (controllerValue));
    }
}

void MainComponent::timerCallback()
{
    // Vérifier périodiquement si la vidéo est arrivée à la fin
    if (videoComponent.isVideoOpen() && videoComponent.isPlaying())
    {
        auto duration = videoComponent.getVideoDuration();
        auto position = videoComponent.getPlayPosition();
        
        // Si on est arrivé à la fin (ou très proche), passer à la vidéo suivante
        if (duration > 0 && position >= duration - 0.05)
        {
            playNextVideo();
        }
    }
}

void MainComponent::playNextVideo()
{
    if (videoUrls.size() == 0)
        return;
    
    // Passer à la vidéo suivante
    currentVideoIndex++;
    currentVideoIndex = currentVideoIndex % videoUrls.size();
    
    // Charger la vidéo suivante
    loadVideoFile (videoUrls[currentVideoIndex]);
    
}
