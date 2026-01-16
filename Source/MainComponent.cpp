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
    
    // Créer le logger personnalisé
    componentLogger = std::make_unique<ComponentLogger> (&logTextEditor);
    juce::Logger::setCurrentLogger (componentLogger.get());

    // Configurer le TextEditor pour les logs
    logTextEditor.setMultiLine (true);
    logTextEditor.setReturnKeyStartsNewLine (true);
    logTextEditor.setReadOnly (true);
    logTextEditor.setCaretVisible (false);
    logTextEditor.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    logTextEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black);
    logTextEditor.setColour (juce::TextEditor::textColourId, juce::Colours::lightgreen);
    
    // Configurer le slider pour le threshold
    thresholdSlider.setRange (0.0, 1.0, 0.01);
    thresholdSlider.setValue (0.5);
    thresholdSlider.addListener (this);
    thresholdSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 20);
    
    thresholdLabel.setText ("Threshold:", juce::dontSendNotification);
    thresholdLabel.attachToComponent (&thresholdSlider, true);
    thresholdLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    
    // Configurer les labels pour les ComboBox
    midiInputLabel.setText ("MIDI Input:", juce::dontSendNotification);
    midiInputLabel.attachToComponent (&midiInputComboBox, true);
    midiInputLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    
    midiOutputLabel.setText ("MIDI Output:", juce::dontSendNotification);
    midiOutputLabel.attachToComponent (&midiOutputComboBox, true);
    midiOutputLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    
    // Configurer les ComboBox
    midiInputComboBox.addListener (this);
    midiOutputComboBox.addListener (this);
    
    capture = std::make_unique<CameraCapture>();
    
    //addAndMakeVisible (videoComponent);
    addAndMakeVisible (logTextEditor);
    addAndMakeVisible (thresholdSlider);
    addAndMakeVisible (thresholdLabel);
    addAndMakeVisible (midiInputComboBox);
    addAndMakeVisible (midiOutputComboBox);
    addAndMakeVisible (midiInputLabel);
    addAndMakeVisible (midiOutputLabel);
    addAndMakeVisible (capture.get());
    
    // Initialiser le threshold avec la valeur par défaut
    capture->setThreshold (thresholdSlider.getValue());
    
    // Activer le focus clavier pour recevoir les événements de touches
    setWantsKeyboardFocus (true);
    

    
    // Configurer le callback pour détecter la fin de la vidéo
    // Utiliser MessageManager::callAsync pour s'assurer que l'appel est thread-safe
    /*videoComponent.onPlaybackStopped = [this]()
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
                    juce::MessageManager::callAsync ([this]()
                    {
                        playNextVideo();
                    }
                }
            }
        });
    };*/
    
    scanPrograms();
    
    // Démarrer le timer pour vérifier périodiquement la fin de la vidéo
    startTimer (50);  // Vérifier toutes les 1000ms
    
    setAudioChannels (0, 2);
    
    // Mettre à jour les listes de périphériques MIDI
    updateMidiDeviceLists();
    
    // Initialiser l'entrée MIDI
    initializeMidiInput();
    
    // Initialiser la sortie MIDI
    initializeMidiOutput();
    
    // Charger automatiquement la première vidéo si disponible
    if (programs.size() > 0)
    {
        //loadProgram(&programs[0]);
        //loadVideoFile (videoUrls[0]);
    } else {
        abort();
    }
    
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (1200, 600);
    
}

MainComponent::~MainComponent()
{
    // Arrêter le timer
    stopTimer();
    
    // Nettoyer les callbacks du VideoComponent pour éviter les appels après destruction
    /*videoComponent.onPlaybackStopped = nullptr;
    videoComponent.onPlaybackStarted = nullptr;*/
    
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

void MainComponent::loadProgram(Program* pgm) {
    auto videoUrl = pgm->getVideoUrl();
    loadVideoFile(videoUrl);
    
    sendProgramChange(16,pgm->getMatrixProgram());
    sendNoteOn(15, pgm->getPrinterNote(), 1.0f);
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
        capture->setBounds(videoBounds);
        
        // Zone pour les contrôles et le logger à droite
        auto rightArea = bounds;
        
        // Réserver de l'espace pour les contrôles en haut
        const int controlHeight = 30;
        const int labelWidth = 100;
        const int spacing = 5;
        
        auto controlsArea = rightArea.removeFromTop (controlHeight * 3 + spacing * 2);
        
        // Positionner le slider threshold en premier
        auto thresholdArea = controlsArea.removeFromTop (controlHeight);
        thresholdLabel.setBounds (thresholdArea.removeFromLeft (labelWidth));
        thresholdSlider.setBounds (thresholdArea);
        
        controlsArea.removeFromTop (spacing);
        
        // Positionner les ComboBox
        auto inputArea = controlsArea.removeFromTop (controlHeight);
        midiInputLabel.setBounds (inputArea.removeFromLeft (labelWidth));
        midiInputComboBox.setBounds (inputArea);
        
        controlsArea.removeFromTop (spacing);
        auto outputArea = controlsArea.removeFromTop (controlHeight);
        midiOutputLabel.setBounds (outputArea.removeFromLeft (labelWidth));
        midiOutputComboBox.setBounds (outputArea);
        
        // Le TextEditor prend le reste de l'espace en bas
        logTextEditor.setBounds (rightArea);
    }
    else
    {
        // Le lecteur vidéo prend toute la taille du composant
        videoComponent.setBounds (bounds);
        capture->setBounds(bounds);
        logTextEditor.setBounds (0, 0, 0, 0);  // Caché
        thresholdSlider.setBounds (0, 0, 0, 0);  // Caché
        thresholdLabel.setBounds (0, 0, 0, 0);  // Caché
        midiInputComboBox.setBounds (0, 0, 0, 0);  // Caché
        midiOutputComboBox.setBounds (0, 0, 0, 0);  // Caché
        midiInputLabel.setBounds (0, 0, 0, 0);  // Caché
        midiOutputLabel.setBounds (0, 0, 0, 0);  // Caché
    }
}

bool MainComponent::keyPressed (const juce::KeyPress& key)
{
    // Toggle la visibilité du logger avec la touche K (insensible à la casse)
    if (key.getTextCharacter() == 'k' || key.getTextCharacter() == 'K')
    {
        isLoggerVisible = !isLoggerVisible;
        logTextEditor.setVisible (isLoggerVisible);
        thresholdSlider.setVisible (isLoggerVisible);
        thresholdLabel.setVisible (isLoggerVisible);
        midiInputComboBox.setVisible (isLoggerVisible);
        midiOutputComboBox.setVisible (isLoggerVisible);
        midiInputLabel.setVisible (isLoggerVisible);
        midiOutputLabel.setVisible (isLoggerVisible);
        resized();  // Recalculer le layout
        return true;  // Consommer l'événement
    }
    if (key.getTextCharacter() == 'd' || key.getTextCharacter() == 'D')
    {
        /*
        int k = 1;
        while(k--) {
            juce::Thread::sleep (20);
            
            sendControlChange(15, 60, 60);
            int len = 192;
            bool state = true;
            
            uint8 chan = 15;
            
            juce::Thread::sleep (20);
            
            int timing = 2;
            
            for (int i = 0; i < len; i++) {
                
                if (state) {
                    sendByteAsMidi(0xff);
                    juce::Thread::sleep (timing);
                    sendByteAsMidi(0xff);
                    juce::Thread::sleep (timing);
                    sendByteAsMidi(0xff);
                    juce::Thread::sleep (timing);
                } else {
                    sendByteAsMidi(0x00);
                    juce::Thread::sleep (timing);
                    sendByteAsMidi(0x00);
                    juce::Thread::sleep (timing);
                    sendByteAsMidi(0x00);
                    juce::Thread::sleep (timing);
                }
                state = !state;
                //juce::Thread::sleep (20);
                
            }
            juce::Thread::sleep (20);
            sendControlChange(15, 60, 60);
        }*/
        
        auto buff = capture->imgBuffer;
        
        for (int yy = 0; yy < 4; yy++) {
            
            juce::Thread::sleep (20);
            sendControlChange(15, 60, 60);
            juce::Thread::sleep (20);
            
            int yBase = yy * 24;
            
            for (int x = 0; x < 192; x++)
            {
                for (int byte = 0; byte < 3; byte++)
                {
                    uint8_t v = 0;

                    for (int bit = 0; bit < 8; bit++)
                    {
                        int bb = (byte * 8 + bit);
                        int y = yBase + bb;

                        if (y < 108 && buff[x][y]) {
                            v |= (1 << (7 - bit));
                        }
                    }

                    sendByteAsMidi(v);
                }
            }
            
            juce::Thread::sleep (20);
            sendControlChange(15, 60, 60);
            juce::Thread::sleep (2000);
        }
        
    }
    
    return false;  // Laisser passer les autres touches
}



void MainComponent::sendByteAsMidi(uint8_t b)
{
    uint8_t note = b & 0x7F;
    uint8_t vel  = (b & 0x80) ? 1 : 2;

    sendNoteOn(15, note, vel, false);
    juce::Thread::sleep (1);
}


void MainComponent::loadVideoFile (const juce::URL& videoURL)
{
    std::cout << (const char*)videoURL.toString(false).toUTF8() << std::endl;
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
    
    
    /*
     static int pgm = 70;
     pgm++;
     if (pgm > 72) {
     pgm = 70;
     }
     sendProgramChange(16,pgm);
     sendNoteOn(16,60, 1.f);*/
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
        /*
        currentVideoIndex++;
        currentVideoIndex = currentVideoIndex % videoUrls.size();
        
        loadVideoFile (videoUrls[currentVideoIndex]);*/
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

void MainComponent::scanPrograms()
{
    programs.clear();
    
    auto docsDir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    auto bisDir = docsDir.getChildFile ("BIS");
    
    if (!bisDir.isDirectory())
    {
        juce::Logger::writeToLog ("Directory ~/Documents/BIS does not exist");
        return;
    }
    
    juce::Array<juce::File> pgmDirs;
    bisDir.findChildFiles (pgmDirs, juce::File::findDirectories, false);

    for (const auto& dir : pgmDirs)
    {
        /*
        auto extension = file.getFileExtension().toLowerCase();
        
        if (extension == ".mov" || extension == ".mp4")
        {
            videoUrls.add (juce::URL (file));
        }*/
        programs.push_back(Program(dir));
    }
}

void MainComponent::updateMidiDeviceLists()
{
    // Mettre à jour la liste des périphériques MIDI Input
    midiInputComboBox.clear();
    midiInputComboBox.addItem ("None", 1);
    
    auto inputDevices = juce::MidiInput::getAvailableDevices();
    for (int i = 0; i < inputDevices.size(); ++i)
    {
        midiInputComboBox.addItem (inputDevices[i].name, i + 2);
    }
    
    // Mettre à jour la liste des périphériques MIDI Output
    midiOutputComboBox.clear();
    midiOutputComboBox.addItem ("None", 1);
    
    auto outputDevices = juce::MidiOutput::getAvailableDevices();
    for (int i = 0; i < outputDevices.size(); ++i)
    {
        midiOutputComboBox.addItem (outputDevices[i].name, i + 2);
    }
    
    // Sélectionner le premier périphérique par défaut (ou "None" si aucun)
    if (inputDevices.size() > 0)
        midiInputComboBox.setSelectedId (2);
    else
        midiInputComboBox.setSelectedId (1);
    
    if (outputDevices.size() > 0)
        midiOutputComboBox.setSelectedId (2);
    else
        midiOutputComboBox.setSelectedId (1);
}

void MainComponent::sliderValueChanged (juce::Slider* slider)
{
    if (slider == &thresholdSlider)
    {
        float thresholdValue = (float)thresholdSlider.getValue();
        capture->setThreshold (thresholdValue);
    }
}

void MainComponent::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &midiInputComboBox)
    {
        // Fermer l'ancien périphérique
        if (midiInput != nullptr)
        {
            midiInput->stop();
            midiInput.reset();
        }
        
        // Ouvrir le nouveau périphérique si sélectionné
        int selectedId = midiInputComboBox.getSelectedId();
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
                    juce::Logger::writeToLog ("MIDI Input opened: " + devices[deviceIndex].name);
                }
            }
        }
        else
        {
            juce::Logger::writeToLog ("MIDI Input closed");
        }
    }
    else if (comboBoxThatHasChanged == &midiOutputComboBox)
    {
        // Fermer l'ancien périphérique
        midiOutput.reset();
        
        // Ouvrir le nouveau périphérique si sélectionné
        int selectedId = midiOutputComboBox.getSelectedId();
        if (selectedId > 1)  // Pas "None"
        {
            auto devices = juce::MidiOutput::getAvailableDevices();
            int deviceIndex = selectedId - 2;
            
            if (deviceIndex >= 0 && deviceIndex < devices.size())
            {
                midiOutput = juce::MidiOutput::openDevice (devices[deviceIndex].identifier);
                if (midiOutput != nullptr)
                {
                    juce::Logger::writeToLog ("MIDI Output opened: " + devices[deviceIndex].name);
                }
                else
                {
                    juce::Logger::writeToLog ("Failed to open MIDI output device: " + devices[deviceIndex].name);
                }
            }
        }
        else
        {
            juce::Logger::writeToLog ("MIDI Output closed");
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
    
    // Ouvrir le périphérique sélectionné dans la ComboBox (ou le premier par défaut)
    int selectedId = midiInputComboBox.getSelectedId();
    int deviceIndex = (selectedId > 1) ? (selectedId - 2) : 0;
    
    if (deviceIndex >= 0 && deviceIndex < midiDevices.size())
    {
        midiInput = juce::MidiInput::openDevice (midiDevices[deviceIndex].identifier, this);
        
        if (midiInput != nullptr)
        {
            midiInput->start();
            juce::Logger::writeToLog ("MIDI Input opened: " + midiDevices[deviceIndex].name);
        }
        else
        {
            juce::Logger::writeToLog ("Failed to open MIDI device: " + midiDevices[deviceIndex].name);
        }
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
    
    // Ouvrir le périphérique sélectionné dans la ComboBox (ou le premier par défaut)
    int selectedId = midiOutputComboBox.getSelectedId();
    int deviceIndex = (selectedId > 1) ? (selectedId - 2) : 0;
    
    if (deviceIndex >= 0 && deviceIndex < midiDevices.size())
    {
        midiOutput = juce::MidiOutput::openDevice (midiDevices[deviceIndex].identifier);
        
        if (midiOutput != nullptr)
        {
            juce::Logger::writeToLog ("MIDI Output opened: " + midiDevices[deviceIndex].name);
        }
        else
        {
            juce::Logger::writeToLog ("Failed to open MIDI output device: " + midiDevices[deviceIndex].name);
        }
    }
}

void MainComponent::sendNoteOn (int channel, int noteNumber, uint8 velocity, bool log)
{
    if (midiOutput != nullptr)
    {
        // Les canaux MIDI sont de 1-16, mais MidiMessage utilise 0-15
        juce::MidiMessage message = juce::MidiMessage::noteOn (channel, noteNumber, velocity);
        midiOutput->sendMessageNow (message);
        if (log) {
            juce::Logger::writeToLog ("MIDI OUT Note On - Channel: " + juce::String (channel) +
                                       ", Note: " + juce::String (noteNumber) +
                                       ", Velocity: " + juce::String (velocity));
        }
        
    }
}

void MainComponent::sendNoteOff (int channel, int noteNumber, uint8 velocity, bool log)
{
    if (midiOutput != nullptr)
    {
        juce::MidiMessage message = juce::MidiMessage::noteOff (channel, noteNumber, velocity);
        midiOutput->sendMessageNow (message);
        if (log) {
            juce::Logger::writeToLog ("MIDI OUT Note Off - Channel: " + juce::String (channel) +
                                       ", Note: " + juce::String (noteNumber) +
                                       ", Velocity: " + juce::String (velocity));
        }
        
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
    if (videoComponent.isVideoOpen())
    {
        if (!videoComponent.isPlaying()) {
            
            juce::MessageManager::callAsync ([this]()
            {
                //playNextVideo();
                std::cout << "end of video" << std::endl;
            });
        }
    }
}
