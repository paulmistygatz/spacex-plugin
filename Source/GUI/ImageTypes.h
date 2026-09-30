#pragma once
#include <JuceHeader.h>

// Runde 189 (User-Bug Windows: Starfield zeigt nur die Streifen, Sonne,
// Planeten, Erde, Funkeln und Punktewolke fehlen).
// JUCE 8 zeichnet unter Windows mit Direct2D; "normale" Images liegen dann
// auf der Grafikkarte. Unsere Ebenen werden aber Pixel fuer Pixel bearbeitet
// (BitmapData, multiplyAllAlphas, clear) und jede Runde neu zusammengesetzt -
// das klappt mit GPU-Bildern nicht zuverlaessig. Auf Windows deshalb reine
// CPU-Bilder (wie auf dem Mac praktisch ohnehin), auf dem Mac bleibt alles,
// wie es ist.
namespace spacexgfx
{
    inline juce::ImageType& layerImageType()
    {
       #if JUCE_WINDOWS
        static juce::SoftwareImageType t;
       #else
        static juce::NativeImageType t;
       #endif
        return t;
    }
    inline juce::Image makeLayer (juce::Image::PixelFormat f, int w, int h, bool clear = true)
    {
        return juce::Image (f, juce::jmax (1, w), juce::jmax (1, h), clear, layerImageType());
    }
    // Aus Datei/Speicher geladene Bilder vor Pixel-Arbeit auf die CPU holen.
    inline juce::Image toLayerType (const juce::Image& img)
    {
       #if JUCE_WINDOWS
        return img.isValid() ? juce::SoftwareImageType().convert (img) : img;
       #else
        return img;
       #endif
    }
}
