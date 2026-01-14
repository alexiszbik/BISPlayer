#pragma once

#include <JuceHeader.h>

class CameraCapture : public juce::Component,
                      public juce::CameraDevice::Listener
{
public:
    CameraCapture()
    {
        // Ouvrir la première caméra
        camera.reset(juce::CameraDevice::openDevice(0, 128, 128, 128, 128));

        if (camera)
            camera->addListener(this);

        // Bouton pour prendre une photo
        addAndMakeVisible(takePhotoButton);
        takePhotoButton.setButtonText("Prendre photo");
        takePhotoButton.onClick = [this]() { takePhoto(); };

        setSize(640, 480);
    }

    ~CameraCapture() override
    {
        if (camera)
            camera->removeListener(this);
    }

    void resized() override
    {
        takePhotoButton.setBounds(10, 10, 120, 30);
    }
    
    void imageReceived(const juce::Image& image) override
    {
        auto now = juce::Time::getMillisecondCounter();
        if (now - lastUpdateTime < 500) // 500 ms = 2 fps
            return; // skip cette frame

        lastUpdateTime = now;

        const int tileSize = 16; // taille de la tuile pour le pixel art

        // Image noir et blanc
        juce::Image bwImage(juce::Image::RGB, image.getWidth(), image.getHeight(), false);

        juce::Image::BitmapData src(image, juce::Image::BitmapData::readOnly);
        juce::Image::BitmapData dst(bwImage, juce::Image::BitmapData::writeOnly);

        for (int by = 0; by < image.getHeight(); by += tileSize)
        {
            for (int bx = 0; bx < image.getWidth(); bx += tileSize)
            {
                int sumGrey = 0;
                int count = 0;

                // Calcul de la moyenne du bloc
                for (int y = by; y < by + tileSize && y < image.getHeight(); ++y)
                {
                    for (int x = bx; x < bx + tileSize && x < image.getWidth(); ++x)
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
                juce::Colour tileColour = (avgGrey < 128) ? juce::Colours::black : juce::Colours::white;

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
    std::unique_ptr<juce::CameraDevice> camera;
    juce::Image currentFrame;
    juce::CriticalSection imageLock;

    juce::TextButton takePhotoButton;
    
    uint32 lastUpdateTime = 0;


    void takePhoto()
    {
        const juce::ScopedLock lock(imageLock);

        if (currentFrame.isValid())
        {
            auto file = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                            .getChildFile("photo_NB.png");
            juce::PNGImageFormat png;
            juce::FileOutputStream stream(file);
            png.writeImageToStream(currentFrame, stream);
            DBG("Photo saved to: " << file.getFullPathName());
        }
    }
};
