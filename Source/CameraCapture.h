#pragma once

#include <JuceHeader.h>
#include <stdlib.h>

class CameraCapture : public juce::Component,
                      public juce::CameraDevice::Listener,
                      public juce::Timer
{
public:
    CameraCapture();
    ~CameraCapture() override;

    void resized() override;
    void paint(juce::Graphics& g) override;
    
    void setThreshold(float value);
    void imageReceived(const juce::Image& image) override;
    void timerCallback() override;
    
    // Buffers publics pour l'accès depuis MainComponent
    bool imgBuffer[320][180];
    bool imgBufferForPrint[192][320];

private:
    static constexpr int tileSize = 6; // taille de la tuile pour le pixel art
    
    void startCountdown();
    void takePhoto();
    bool processBlockToColour(const juce::Image::BitmapData& src,
                              int blockX, int blockY,
                              int imageWidth, int imageHeight);
    
    std::unique_ptr<juce::CameraDevice> camera;
    juce::Image currentFrame;
    juce::CriticalSection imageLock;

    juce::TextButton takePhotoButton;
    
    uint32_t lastUpdateTime = 0;
    
    int threshold = 127;
    
    // Variables pour le décompte
    int countdownValue = 0;
    bool isCountingDown = false;
};
