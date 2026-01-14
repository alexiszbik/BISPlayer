/*
  ==============================================================================

    Program.h
    Created: 13 Jan 2026 8:40:40am
    Author:  Alexis ZBIK

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ComponentLogger.h"

using namespace juce;

class Program {
public:
    Program(File folder) {
        juce::Array<juce::File> files;
        folder.findChildFiles (files, juce::File::findFiles, false);

        for (const auto& file : files)
        {
            auto extension = file.getFileExtension().toLowerCase();
            
            if (extension == ".mov" || extension == ".mp4") {
                videoUrls.add (juce::URL (file));
            }
            if (extension == ".xml") {
                parseXMLFile(file);
            }
        }
        auto pgmName = folder.getFileName();
        
        if (videoUrls.size() == 0) {
            juce::Logger::writeToLog ("Loading program " + folder.getFileName() + " Error");
            juce::Logger::writeToLog ("No video");
        } else if (printerNote == -1) {
            juce::Logger::writeToLog ("Loading program " + folder.getFileName() + " Error");
            juce::Logger::writeToLog ("No printer program");
        } else if (matrixPgm.size() == 0) {
            juce::Logger::writeToLog ("Loading program " + folder.getFileName() + " Error");
            juce::Logger::writeToLog ("No matrix program");
        } else {
            juce::Logger::writeToLog ("Loading program " + folder.getFileName() + " OK");
        }
    }
    
    URL getVideoUrl() {
        auto rand = juce::Random(Time().getMillisecondCounter());
        int videoIndex = rand.nextInt(videoUrls.size());
        return videoUrls[videoIndex];
    }
    
    void parseXMLFile(const File& file) {
        auto xmlBaseElement = juce::parseXML(file);
        std::cout << xmlBaseElement->toString() << std::endl;

        if (xmlBaseElement->hasTagName ("Program"))
        {
            // now we'll iterate its sub-elements looking for 'giraffe' elements..
            for (auto* e : xmlBaseElement->getChildIterator())
            {
                if (e->hasTagName ("Matrix"))
                {
                    int value = e->getAllSubText().getIntValue();
                    matrixPgm.push_back(value);
                }
                if (e->hasTagName ("Printer"))
                {
                    printerNote = e->getAllSubText().getIntValue();
                }
            }
        }
    }
    
    int getMatrixProgram() {
        auto rand = juce::Random(Time().getMillisecondCounter());
        int pgmIdx = rand.nextInt((int)matrixPgm.size());
        return matrixPgm[pgmIdx];
    }
    
    int getPrinterNote() {
        return printerNote;
    }
private:
    Array<URL> videoUrls;
    int printerNote = -1;
    std::vector<int> matrixPgm = {};
    
};
