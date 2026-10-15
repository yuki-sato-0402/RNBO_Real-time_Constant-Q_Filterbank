#pragma once

#include <JuceHeader.h>
#include <vector>
#include <cmath>

class SpectrogramDisplay : public juce::AnimatedAppComponent
{
public:
    SpectrogramDisplay(const float* dataPtr)
        : ringBuffer(dataPtr)
    {
        visualEnergy.resize(88, 0.0f);
        
        // Optimize: Use a fixed-size low-resolution canvas for the waterfall.
        // This dramatically reduces CPU usage for image scrolling and drawing,
        // while the GPU handles the high-quality scaling and smoothing.
        const int imgWidth = 260; // 52 white keys * 5 pixels per key
        const int imgHeight = 150; // 150 history slices
        spectrogramImage = juce::Image(juce::Image::ARGB, imgWidth, imgHeight, true);
        spectrogramImage.clear(spectrogramImage.getBounds(), juce::Colour(10, 13, 20));

        // Precompute key layout in the low-resolution image coordinates
        float imgWhiteKeyWidth = static_cast<float>(imgWidth) / 52.0f; // Exactly 5.0f
        float imgBlackKeyWidth = imgWhiteKeyWidth * 0.62f; // 3.1f

        imgKeyVisuals.resize(88);
        for (size_t i = 0; i < 88; ++i)
        {
            int noteIdx = static_cast<int>(i);
            int w = getWhiteKeyIndex(noteIdx);
            imgKeyVisuals[i].isBlack = (w == -1);
            if (!imgKeyVisuals[i].isBlack)
            {
                imgKeyVisuals[i].x = static_cast<float>(w) * imgWhiteKeyWidth;
                imgKeyVisuals[i].w = imgWhiteKeyWidth;
            }
            else
            {
                int leftW = getBlackKeyLeftWhiteIndex(noteIdx);
                imgKeyVisuals[i].x = static_cast<float>(leftW + 1) * imgWhiteKeyWidth - (imgBlackKeyWidth * 0.5f);
                imgKeyVisuals[i].w = imgBlackKeyWidth;
            }
        }

        setFramesPerSecond(60);
    }

    ~SpectrogramDisplay() override = default;

    // Helper to map 88 note indices (0-87) to 52 white key indices (0-51)
    // Returns -1 if the note is a black key.
    static int getWhiteKeyIndex(int noteIndex)
    {
        if (noteIndex == 0) return 0;  // A0
        if (noteIndex == 1) return -1; // Bb0 (black)
        if (noteIndex == 2) return 1;  // B0

        // Notes 3 to 86 correspond to C1 to B7 (7 full octaves)
        int rel = (noteIndex - 3) % 12;
        int oct = (noteIndex - 3) / 12;

        switch (rel)
        {
            case 0:  return 2 + oct * 7 + 0; // C
            case 1:  return -1;             // C#
            case 2:  return 2 + oct * 7 + 1; // D
            case 3:  return -1;             // D#
            case 4:  return 2 + oct * 7 + 2; // E
            case 5:  return 2 + oct * 7 + 3; // F
            case 6:  return -1;             // F#
            case 7:  return 2 + oct * 7 + 4; // G
            case 8:  return -1;             // G#
            case 9:  return 2 + oct * 7 + 5; // A
            case 10: return -1;            // A#
            case 11: return 2 + oct * 7 + 6; // B
            default: return -1;
        }
    }

    // Helper to find the left white key index (0-51) for a given black key (0-87)
    // Returns -1 if the note is a white key.
    static int getBlackKeyLeftWhiteIndex(int noteIndex)
    {
        if (noteIndex == 1) return 0; // Bb0 sits after A0 (w = 0)
        if (noteIndex < 3 || noteIndex > 86) return -1;

        int rel = (noteIndex - 3) % 12;
        int oct = (noteIndex - 3) / 12;

        switch (rel)
        {
            case 1:  return 2 + oct * 7 + 0; // Db sits after C
            case 3:  return 2 + oct * 7 + 1; // Eb sits after D
            case 6:  return 2 + oct * 7 + 3; // Gb sits after F
            case 8:  return 2 + oct * 7 + 4; // Ab sits after G
            case 10: return 2 + oct * 7 + 5; // Bb sits after A
            default: return -1;
        }
    }

    // Premium color mapper for energy (0.0 to 1.0)
    static juce::Colour getSpectrogramColor(float energy)
    {
        // Apply visual curve to expand dynamic range for low-level signals
        energy = std::sqrt(juce::jlimit(0.0f, 1.0f, energy));

        if (energy < 0.05f)
        {
            // Near silence - deep charcoal navy blue background color
            float t = energy / 0.05f;
            return juce::Colour(10, 13, 20).interpolatedWith(juce::Colour(45, 15, 85), t);
        }
        else if (energy < 0.35f)
        {
            // Low level - deep indigo/purple to vivid violet-pink
            float t = (energy - 0.05f) / 0.30f;
            return juce::Colour(45, 15, 85).interpolatedWith(juce::Colour(220, 10, 110), t);
        }
        else if (energy < 0.65f)
        {
            // Mid level - pinkish magenta to glowing orange-red
            float t = (energy - 0.35f) / 0.30f;
            return juce::Colour(220, 10, 110).interpolatedWith(juce::Colour(255, 110, 0), t);
        }
        else if (energy < 0.90f)
        {
            // High level - orange-red to bright cyan/yellow glow
            float t = (energy - 0.65f) / 0.25f;
            return juce::Colour(255, 110, 0).interpolatedWith(juce::Colour(0, 240, 240), t);
        }
        else
        {
            // Peak - bright cyan/blue to pure hot white
            float t = (energy - 0.90f) / 0.10f;
            return juce::Colour(0, 240, 240).interpolatedWith(juce::Colours::white, t);
        }
    }

    struct KeyVisual
    {
        int noteIndex;
        bool isBlack;
        juce::Rectangle<float> rect;
    };

    struct ImageKeyVisual
    {
        float x;
        float w;
        bool isBlack;
    };

    void update() override
    {
        if (ringBuffer == nullptr)
            return;

        const int imgW = spectrogramImage.getWidth();
        const int imgH = spectrogramImage.getHeight();

        // 1. Fetch latest raw values from processor buffer and apply decay for keyboard visual response
        std::vector<float> latestEnergy(88);
        for (size_t i = 0; i < 88; ++i)
        {
            float raw = ringBuffer[i];
            latestEnergy[i] = raw;
            // Key glow decay: responsive attack, smooth exponential release
            visualEnergy[i] = juce::jmax(raw, visualEnergy[i] * 0.90f);
        }

        // 2. Scroll the low-resolution spectrogram image upwards (extremely cheap!)
        {
            juce::Graphics imgG(spectrogramImage);
            imgG.drawImage(spectrogramImage,
                           0, 0, imgW, imgH - scrollSpeed,
                           0, scrollSpeed, imgW, imgH - scrollSpeed);

            // Clear the new bottom slice with the background color
            imgG.setColour(juce::Colour(10, 13, 20));
            imgG.fillRect(0, imgH - scrollSpeed, imgW, scrollSpeed);

            // Draw new spectrogram row (white keys first, then black keys on top)
            for (size_t i = 0; i < 88; ++i)
            {
                if (!imgKeyVisuals[i].isBlack)
                {
                    float energy = latestEnergy[i];
                    imgG.setColour(getSpectrogramColor(energy));
                    imgG.fillRect(imgKeyVisuals[i].x, static_cast<float>(imgH - scrollSpeed), imgKeyVisuals[i].w, static_cast<float>(scrollSpeed));
                }
            }
            for (size_t i = 0; i < 88; ++i)
            {
                if (imgKeyVisuals[i].isBlack)
                {
                    float energy = latestEnergy[i];
                    imgG.setColour(getSpectrogramColor(energy));
                    imgG.fillRect(imgKeyVisuals[i].x, static_cast<float>(imgH - scrollSpeed), imgKeyVisuals[i].w, static_cast<float>(scrollSpeed));
                }
            }
        }

        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Fill base background (very sleek deep space/dark slate)
        g.fillAll(juce::Colour(8, 10, 15));

        float keyboardHeight = keyVisuals.empty() ? 80.0f : keyVisuals[0].rect.getHeight();
        float spectrogramHeight = bounds.getHeight() - keyboardHeight;

        // 1. Draw the scrolling spectrogram image (stretched by GPU)
        if (spectrogramImage.isValid())
        {
            g.drawImageWithin(spectrogramImage, 
                              0, 0, static_cast<int>(bounds.getWidth()), static_cast<int>(spectrogramHeight),
                              juce::RectanglePlacement::stretchToFit);
        }

        // 2. Draw vertical octave lines as a technical grid overlay
        for (size_t i = 3; i < 88; ++i)
        {
            if ((i - 3) % 12 == 0) // C note boundaries
            {
                float x = keyVisuals[i].rect.getX();
                // Subtly colored glowing grid line
                g.setColour(juce::Colour(255, 255, 255).withAlpha(0.06f));
                g.drawVerticalLine(juce::roundToInt(x), 0.0f, spectrogramHeight);
            }
        }

        // 3. Draw a separator line between spectrogram and keyboard
        g.setColour(juce::Colour(40, 42, 50));
        g.drawHorizontalLine(juce::roundToInt(spectrogramHeight), 0.0f, bounds.getWidth());

        // 4. Draw the 88-key piano keyboard
        // Draw white keys first
        for (const auto& kv : keyVisuals)
        {
            if (!kv.isBlack)
            {
                float energy = visualEnergy[static_cast<size_t>(kv.noteIndex)];
                
                // Normal white key color is a soft clean off-white
                juce::Colour normalColor = juce::Colour(240, 240, 245);
                
                // Glowing active color
                juce::Colour activeColor = getSpectrogramColor(energy);
                
                // Interpolate key color based on energy
                juce::Colour drawColor = normalColor.interpolatedWith(activeColor, juce::jlimit(0.0f, 0.85f, energy * 2.0f));
                
                g.setColour(drawColor);
                g.fillRect(kv.rect);
                
                // Soft elegant border
                g.setColour(juce::Colour(50, 52, 60));
                g.drawRect(kv.rect, 1.0f);

                // Draw note labels for C keys to help navigation
                if (kv.noteIndex >= 3 && (kv.noteIndex - 3) % 12 == 0)
                {
                    int octave = (kv.noteIndex - 3) / 12 + 1;
                    juce::String label = "C" + juce::String(octave);
                    g.setColour(juce::Colour(90, 95, 105));
                    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
                    juce::Rectangle<float> labelArea(kv.rect.getX(), kv.rect.getBottom() - 15.0f, kv.rect.getWidth(), 12.0f);
                    g.drawText(label, labelArea, juce::Justification::centred);
                }
                else if (kv.noteIndex == 0)
                {
                    g.setColour(juce::Colour(90, 95, 105));
                    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
                    juce::Rectangle<float> labelArea(kv.rect.getX(), kv.rect.getBottom() - 15.0f, kv.rect.getWidth(), 12.0f);
                    g.drawText("A0", labelArea, juce::Justification::centred);
                }
            }
        }

        // Draw black keys on top
        for (const auto& kv : keyVisuals)
        {
            if (kv.isBlack)
            {
                float energy = visualEnergy[static_cast<size_t>(kv.noteIndex)];
                
                // Rich charcoal/black key color
                juce::Colour normalColor = juce::Colour(22, 24, 30);
                
                // Glowing active color
                juce::Colour activeColor = getSpectrogramColor(energy);
                
                // Interpolate key color
                juce::Colour drawColor = normalColor.interpolatedWith(activeColor, juce::jlimit(0.0f, 0.85f, energy * 2.0f));
                
                g.setColour(drawColor);
                g.fillRect(kv.rect);
                
                // Sleek outline
                g.setColour(juce::Colour(60, 62, 70));
                g.drawRect(kv.rect, 1.0f);
            }
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds().toFloat();
        float width = bounds.getWidth();
        float height = bounds.getHeight();
        
        // Keyboard height takes 22% of UI or stays within 55px and 120px
        float keyboardHeight = height * 0.22f;
        keyboardHeight = juce::jlimit(55.0f, 120.0f, keyboardHeight);
        
        float whiteKeyWidth = width / 52.0f;
        float blackKeyWidth = whiteKeyWidth * 0.62f;
        float blackKeyHeight = keyboardHeight * 0.62f;
        
        keyVisuals.clear();
        
        for (size_t i = 0; i < 88; ++i)
        {
            int noteIdx = static_cast<int>(i);
            int w = getWhiteKeyIndex(noteIdx);
            if (w != -1)
            {
                // White key
                KeyVisual kv;
                kv.noteIndex = noteIdx;
                kv.isBlack = false;
                kv.rect = juce::Rectangle<float>(
                    static_cast<float>(w) * whiteKeyWidth, 
                    height - keyboardHeight, 
                    whiteKeyWidth, 
                    keyboardHeight
                );
                keyVisuals.push_back(kv);
            }
            else
            {
                // Black key
                int leftW = getBlackKeyLeftWhiteIndex(noteIdx);
                KeyVisual kv;
                kv.noteIndex = noteIdx;
                kv.isBlack = true;
                kv.rect = juce::Rectangle<float>(
                    static_cast<float>(leftW + 1) * whiteKeyWidth - (blackKeyWidth * 0.5f), 
                    height - keyboardHeight, 
                    blackKeyWidth, 
                    blackKeyHeight
                );
                keyVisuals.push_back(kv);
            }
        }
    }

private:
    const float* ringBuffer = nullptr;
    std::vector<float> visualEnergy;
    std::vector<KeyVisual> keyVisuals;
    std::vector<ImageKeyVisual> imgKeyVisuals;
    juce::Image spectrogramImage;
    const int scrollSpeed = 1; // Scroll 1 pixel per frame (smoother!)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrogramDisplay)
};
