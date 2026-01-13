/*
  ==============================================================================

    Program.h
    Created: 13 Jan 2026 8:40:40am
    Author:  Alexis ZBIK

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
using namespace juce;

class Program {
public:
    Program(File folder) {
        juce::Array<juce::File> files;
        folder.findChildFiles (files, juce::File::findFiles, false);

        for (const auto& file : files)
        {
            auto extension = file.getFileExtension().toLowerCase();
            
            if (extension == ".mov" || extension == ".mp4")
            {
                videoUrls.add (juce::URL (file));
            }
        }
    }
    
    URL getVideoUrl() {
        auto rand = juce::Random(Time().getMillisecondCounter());
        int videoIndex = rand.nextInt(videoUrls.size());
        return videoUrls[videoIndex];
    }
private:
    
    Array<URL> videoUrls;
};
