#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Logger personnalisé qui écrit dans un TextEditor
*/
class ComponentLogger  : public juce::Logger
{
public:
    //==============================================================================
    ComponentLogger (juce::TextEditor* textEditorToUse)
        : logTextEditor (textEditorToUse)
    {
    }
    
    //==============================================================================
    void logMessage (const juce::String& message) override
    {
        if (logTextEditor != nullptr)
        {
            // Ajouter le message au texte (thread-safe via MessageManager)
            juce::MessageManager::callAsync ([this, message]()
            {
                auto timestamp = juce::Time::getCurrentTime().formatted ("%H:%M:%S");
                auto logLine = "[" + timestamp + "] " + message + "\n";
                
                logTextEditor->moveCaretToEnd();
                logTextEditor->insertTextAtCaret (logLine);
                
                // Faire défiler vers le bas pour voir les nouveaux messages
                logTextEditor->moveCaretToEnd();
                
                // Limiter le nombre de lignes si nécessaire
                trimLogIfNeeded();
            });
        }
    }

private:
    //==============================================================================
    juce::TextEditor* logTextEditor;
    static constexpr int maxLogLines = 1000;
    
    void trimLogIfNeeded()
    {
        if (logTextEditor == nullptr)
            return;
            
        auto text = logTextEditor->getText();
        auto lines = juce::StringArray::fromLines (text);
        
        if (lines.size() > maxLogLines)
        {
            // Garder seulement les dernières maxLogLines lignes
            lines.removeRange (0, lines.size() - maxLogLines);
            logTextEditor->setText (lines.joinIntoString ("\n"));
        }
    }
};

