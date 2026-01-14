#pragma once

#include <JuceHeader.h>

class CameraCapture : public Component
{
public:
    CameraCapture()
    {
        // Ouvrir la première caméra
        camera.reset(juce::CameraDevice::openDevice(0));

        if (camera)
        {
            // Créer un viewer (prévisualisation)
            viewer = camera->createViewerComponent();
            addAndMakeVisible(viewer);

            // Bouton pour prendre une photo
            addAndMakeVisible(takePhotoButton);
            takePhotoButton.setButtonText("Prendre photo");
            takePhotoButton.onClick = [this]() { takePhoto(); };
        }

        setSize(640, 480);
    }

    ~CameraCapture() override
    {
        camera = nullptr; // détruit proprement
    }

    void resized() override
    {
        if (viewer) viewer->setBounds(getLocalBounds());
        takePhotoButton.setBounds(10, 10, 120, 30);
    }

private:
    std::unique_ptr<juce::CameraDevice> camera;
    juce::Component* viewer = nullptr;
    juce::TextButton takePhotoButton;

    void takePhoto()
    {
        if (camera)
        {
            camera->takeStillPicture([this](const juce::Image& image)
            {
                // Sauvegarde la photo sur le bureau
                auto file = juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                                .getChildFile("photo.png");
                juce::PNGImageFormat png;
                juce::FileOutputStream stream(file);
                png.writeImageToStream(image, stream);

                DBG("Photo saved to: " << file.getFullPathName());
            });
        }
    }
};
