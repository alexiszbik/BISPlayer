#pragma once

#include <JuceHeader.h>

class CameraCapture : public juce::Component,
                      public juce::CameraDevice::Listener
{
public:
    CameraCapture()
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
        takePhotoButton.onClick = [this]() { takePhoto(); };

        setSize(640, 480);
    }
    
    bool imgBuffer[192][108];

    ~CameraCapture() override
    {
        if (camera)
            camera->removeListener(this);
    }

    void resized() override
    {
        takePhotoButton.setBounds(10, 10, 120, 30);
    }
    
    void setThreshold(float value) {
        threshold = (int)(value * 255.f);
    }
    
    void imageReceived(const juce::Image& image) override
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

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);

        const juce::ScopedLock lock(imageLock);
        if (currentFrame.isValid())
            g.drawImage(currentFrame, getLocalBounds().toFloat());
    }

private:
    static constexpr int tileSize = 10; // taille de la tuile pour le pixel art
    
    std::unique_ptr<juce::CameraDevice> camera;
    juce::Image currentFrame;
    juce::CriticalSection imageLock;

    juce::TextButton takePhotoButton;
    
    uint32 lastUpdateTime = 0;
    
    int threshold = 127;

    // Calcule la couleur (noir ou blanc) pour un bloc donné dans une image
    bool processBlockToColour(const juce::Image::BitmapData& src,
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
                uint8 grey = static_cast<uint8>(0.299f * c.getRed()
                                             + 0.587f * c.getGreen()
                                             + 0.114f * c.getBlue());
                sumGrey += grey;
                ++count;
            }
        }

        // Seuil pour noir/blanc
        uint8 avgGrey = static_cast<uint8>(sumGrey / count);
        return (avgGrey < threshold);
    }
    
    

    void takePhoto()
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

    

};
