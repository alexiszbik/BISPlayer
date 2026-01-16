#include "CameraCapture.h"

//==============================================================================
CameraCapture::CameraCapture()
{
    auto devices = juce::CameraDevice::getAvailableDevices();
    for (auto& d : devices) {
        juce::Logger::writeToLog (d);
    }
    // Ouvrir la première caméra
    camera.reset(juce::CameraDevice::openDevice(0));

    if (camera)
        camera->addListener(this);

    // Bouton pour prendre une photo
    addAndMakeVisible(takePhotoButton);
    takePhotoButton.setButtonText("Prendre photo");
    takePhotoButton.onClick = [this]() { startCountdown(); };

    setSize(640, 480);
}

CameraCapture::~CameraCapture()
{
    stopTimer();
    if (camera)
        camera->removeListener(this);
}

//==============================================================================
void CameraCapture::resized()
{
    takePhotoButton.setBounds(10, 10, 120, 30);
}

void CameraCapture::setThreshold(float value)
{
    threshold = (int)(value * 255.f);
}

//==============================================================================
void CameraCapture::imageReceived(const juce::Image& image)
{
    auto now = juce::Time::getMillisecondCounter();
    if (now - lastUpdateTime < 500) // 500 ms = 2 fps
        return; // skip cette frame

    lastUpdateTime = now;

    // Image noir et blanc
    juce::Image bwImage(juce::Image::RGB, image.getWidth(), image.getHeight(), false);

    juce::Image::BitmapData src(image, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dst(bwImage, juce::Image::BitmapData::writeOnly);

    for (int by = 0; by < image.getHeight(); by += tileSize)
    {
        for (int bx = 0; bx < image.getWidth(); bx += tileSize)
        {
            juce::Colour tileColour = processBlockToColour(src, bx, by, image.getWidth(), image.getHeight()) ? juce::Colours::black : juce::Colours::white;

            // Remplissage du bloc
            for (int y = by; y < by + tileSize && y < image.getHeight(); ++y)
            {
                for (int x = bx; x < bx + tileSize && x < image.getWidth(); ++x)
                {
                    dst.setPixelColour(x, y, tileColour);
                }
            }
        }
    }

    {
        const juce::ScopedLock lock(imageLock);
        currentFrame = bwImage;
    }

    juce::MessageManager::callAsync([this]() { repaint(); });
}

void CameraCapture::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    const juce::ScopedLock lock(imageLock);
    if (currentFrame.isValid())
        g.drawImage(currentFrame, getLocalBounds().toFloat());
    
    // Afficher le décompte si actif
    if (countdownValue > 0)
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Fond semi-transparent pour assombrir l'image
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillAll();
        
        // Grand cercle avec le chiffre
        float circleSize = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.3f;
        auto circleBounds = juce::Rectangle<float>(circleSize, circleSize)
                              .withCentre(bounds.getCentre());
        
        // Cercle blanc épais
        g.setColour(juce::Colours::white);
        g.fillEllipse(circleBounds);
        
        // Bordure noire
        g.setColour(juce::Colours::black);
        g.drawEllipse(circleBounds, 5.0f);
        
        // Chiffre au centre
        g.setColour(juce::Colours::black);
        g.setFont(juce::Font(circleSize * 0.6f, juce::Font::bold));
        g.drawText(juce::String(countdownValue), circleBounds, juce::Justification::centred);
    }
}

//==============================================================================
bool CameraCapture::processBlockToColour(const juce::Image::BitmapData& src,
                                          int blockX, int blockY,
                                          int imageWidth, int imageHeight)
{
    int sumGrey = 0;
    int count = 0;

    // Calcul de la moyenne du bloc
    for (int y = blockY; y < blockY + tileSize && y < imageHeight; ++y)
    {
        for (int x = blockX; x < blockX + tileSize && x < imageWidth; ++x)
        {
            auto c = src.getPixelColour(x, y);
            uint8_t grey = static_cast<uint8_t>(0.299f * c.getRed()
                                         + 0.587f * c.getGreen()
                                         + 0.114f * c.getBlue());
            sumGrey += grey;
            ++count;
        }
    }

    // Seuil pour noir/blanc
    uint8_t avgGrey = static_cast<uint8_t>(sumGrey / count);
    return (avgGrey < threshold);
}

void CameraCapture::startCountdown()
{
    if (isCountingDown)
        return; // Déjà en cours de décompte
    
    isCountingDown = true;
    countdownValue = 3;
    takePhotoButton.setEnabled(false);
    
    // Démarrer le timer pour le décompte (1000ms = 1 seconde)
    startTimer(1000);
    
    // Afficher immédiatement le "3"
    repaint();
}

void CameraCapture::timerCallback()
{
    countdownValue--;
    
    if (countdownValue > 0)
    {
        // Afficher le prochain chiffre
        repaint();
    }
    else
    {
        // Le décompte est terminé, prendre la photo
        stopTimer();
        isCountingDown = false;
        takePhotoButton.setEnabled(true);
        takePhoto();
        repaint(); // Enlever l'affichage du décompte
    }
}

void CameraCapture::takePhoto()
{
    //La photo devrait faire 192 par 108
    
    const juce::ScopedLock lock(imageLock);

    if (!currentFrame.isValid())
        return;

    int w = currentFrame.getWidth();
    int h = currentFrame.getHeight();

    // Taille de l'image pixelisée réelle
    int pixelW = w / tileSize;
    int pixelH = h / tileSize;
    
    std::cout << pixelH << std::endl;
    std::cout << pixelW << std::endl;

    juce::Image pixelImage(juce::Image::RGB, pixelW, pixelH, false);
    juce::Image::BitmapData src(currentFrame, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dst(pixelImage, juce::Image::BitmapData::writeOnly);

    // Calcul de la couleur moyenne pour chaque bloc
    for (int by = 0; by < pixelH; ++by)
    {
        for (int bx = 0; bx < pixelW; ++bx)
        {
            int blockX = bx * tileSize;
            int blockY = by * tileSize;
            bool pixRes = processBlockToColour(src, blockX, blockY, w, h);
            juce::Colour tileColour = pixRes ? juce::Colours::black : juce::Colours::white;
            dst.setPixelColour(bx, by, tileColour);
  
            imgBuffer[bx][by] = pixRes;
        }
    }

    
    for (int x = 0; x < 320; x++)
    {
        for (int y = 0; y < 192; y++)
        {
            if (y < 180) {
                imgBufferForPrint[y][x] = imgBuffer[x][y];
            } else {
                imgBufferForPrint[y][x] = 0;
            }
            
        }
    }
    
    // Sauvegarde
    auto file = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                    .getChildFile("photo_pixelArt.png");
    if (file.exists() ) {
        file.deleteFile();
    }
    juce::PNGImageFormat png;
    juce::FileOutputStream stream(file);
    png.writeImageToStream(pixelImage, stream);
    DBG("Photo saved to: " << file.getFullPathName());
}
