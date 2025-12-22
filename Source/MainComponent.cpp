#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : videoComponent (false)  // false = pas de contrôles natifs, on gère nous-mêmes
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);

    // Ajouter le composant vidéo comme enfant
    addAndMakeVisible (videoComponent);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
    
    // Charger automatiquement la vidéo Test.mp4
    loadVideoFile();
}

MainComponent::~MainComponent()
{
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

void MainComponent::loadVideoFile()
{
    // Chercher le fichier Test.mp4 dans ~/Documents/BIS
    auto docsDir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    auto videoFile = docsDir.getChildFile ("BIS").getChildFile ("Test.mp4");
    
    // Charger la vidéo de manière asynchrone (fonctionne sur toutes les plateformes)
    if (videoFile.existsAsFile())
    {
        videoComponent.loadAsync (juce::URL (videoFile),
                                  [this] (const juce::URL&, juce::Result result)
                                  {
                                      if (result.wasOk())
                                      {
                                          // Démarrer la lecture automatiquement
                                          videoComponent.play();
                                      }
                                      else
                                      {
                                          juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                                                  "Erreur",
                                                                                  "Impossible de charger la vidéo Test.mp4:\n" + result.getErrorMessage());
                                      }
                                  });
    }
    else
    {
        // Fichier introuvable - afficher un message d'erreur
        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                "Fichier introuvable",
                                                "Le fichier Test.mp4 n'a pas été trouvé dans ~/Documents/BIS/");
    }
}
