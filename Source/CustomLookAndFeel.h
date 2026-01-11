#pragma once

#include <JuceHeader.h>

class RetroLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Y2K / Digital Collage palette
    const juce::Colour deepBlack    = juce::Colour(0xFF0A0A0F);
    const juce::Colour darkPurple   = juce::Colour(0xFF1A1025);
    const juce::Colour midPurple    = juce::Colour(0xFF2D1B3D);
    const juce::Colour neonCyan     = juce::Colour(0xFF00FFFF);
    const juce::Colour neonMagenta  = juce::Colour(0xFFFF00FF);
    const juce::Colour acidGreen    = juce::Colour(0xFF39FF14);
    const juce::Colour hotPink      = juce::Colour(0xFFFF1493);
    const juce::Colour chrome       = juce::Colour(0xFFC0C0C0);
    const juce::Colour chromeDark   = juce::Colour(0xFF707080);
    const juce::Colour chromeLight  = juce::Colour(0xFFE8E8F0);

    RetroLookAndFeel()
    {
        setColour(juce::Slider::thumbColourId, neonCyan);
        setColour(juce::Slider::rotarySliderFillColourId, neonMagenta);
        setColour(juce::Slider::rotarySliderOutlineColourId, midPurple);
        setColour(juce::Slider::trackColourId, neonCyan);

        setColour(juce::ComboBox::backgroundColourId, deepBlack);
        setColour(juce::ComboBox::outlineColourId, chromeDark);
        setColour(juce::ComboBox::textColourId, chrome);
        setColour(juce::ComboBox::arrowColourId, neonCyan);

        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xFF15101A));
        setColour(juce::PopupMenu::textColourId, chrome);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, midPurple);
        setColour(juce::PopupMenu::highlightedTextColourId, neonCyan);

        setColour(juce::ToggleButton::textColourId, chrome);
        setColour(juce::ToggleButton::tickColourId, neonCyan);

        setColour(juce::Label::textColourId, chrome.withAlpha(0.9f));

        setColour(juce::TextEditor::backgroundColourId, deepBlack);
        setColour(juce::TextEditor::textColourId, chromeLight);
        setColour(juce::TextEditor::outlineColourId, chromeDark);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        auto radius = (float) juce::jmin(width / 2, height / 2) - 4.0f;
        auto centreX = (float) x + (float) width * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Neon glow behind knob
        {
            juce::ColourGradient glow(neonCyan.withAlpha(0.15f), centreX, centreY,
                                       juce::Colours::transparentBlack, centreX + radius + 25, centreY, true);
            g.setGradientFill(glow);
            g.fillEllipse(rx - 20, ry - 20, rw + 40, rw + 40);
        }

        // Chrome outer ring
        {
            juce::ColourGradient ring(chromeLight, centreX - radius, centreY - radius,
                                       chromeDark, centreX + radius, centreY + radius, false);
            ring.addColour(0.3, chrome);
            ring.addColour(0.7, chromeDark);
            g.setGradientFill(ring);
            g.fillEllipse(rx - 3, ry - 3, rw + 6, rw + 6);
        }

        // Dark inset bevel
        g.setColour(deepBlack);
        g.fillEllipse(rx + 2, ry + 2, rw - 4, rw - 4);

        // Main knob body - chrome gradient
        {
            juce::ColourGradient knob(chromeLight, centreX - radius * 0.5f, centreY - radius * 0.5f,
                                       chromeDark, centreX + radius * 0.3f, centreY + radius * 0.6f, true);
            knob.addColour(0.3, chrome);
            knob.addColour(0.6, chromeDark.brighter(0.1f));
            g.setGradientFill(knob);
            g.fillEllipse(rx + 5, ry + 5, rw - 10, rw - 10);
        }

        // Subtle scanlines on knob
        g.setColour(juce::Colours::black.withAlpha(0.08f));
        for (float sy = ry + 6; sy < ry + rw - 6; sy += 3)
        {
            float dist = std::abs(sy - centreY);
            if (dist < radius - 8)
            {
                float halfWidth = std::sqrt((radius - 8) * (radius - 8) - dist * dist);
                g.drawLine(centreX - halfWidth, sy, centreX + halfWidth, sy, 1.0f);
            }
        }

        // Value arc - neon glow
        {
            juce::Path arcPath;
            arcPath.addCentredArc(centreX, centreY, radius - 5.0f, radius - 5.0f,
                                  0.0f, rotaryStartAngle, angle, true);

            // Outer glow
            g.setColour(neonMagenta.withAlpha(0.35f));
            g.strokePath(arcPath, juce::PathStrokeType(12.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Main arc
            g.setColour(neonMagenta);
            g.strokePath(arcPath, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Inner bright core
            g.setColour(juce::Colours::white.withAlpha(0.5f));
            g.strokePath(arcPath, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Pointer
        {
            juce::Path pointer;
            auto pointerLength = radius * 0.5f;
            pointer.addRoundedRectangle(-2.5f, -radius + 8.0f, 5.0f, pointerLength, 2.0f);
            pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

            // Cyan glow
            g.setColour(neonCyan.withAlpha(0.5f));
            g.strokePath(pointer, juce::PathStrokeType(8.0f));

            // Main pointer - bright cyan
            g.setColour(neonCyan);
            g.fillPath(pointer);

            // White highlight
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.strokePath(pointer, juce::PathStrokeType(1.0f));
        }

        // Center cap - chrome dome
        {
            auto capRadius = radius * 0.2f;
            juce::ColourGradient cap(chromeLight, centreX - capRadius * 0.5f, centreY - capRadius * 0.5f,
                                      chromeDark, centreX + capRadius * 0.4f, centreY + capRadius * 0.5f, true);
            g.setGradientFill(cap);
            g.fillEllipse(centreX - capRadius, centreY - capRadius, capRadius * 2, capRadius * 2);

            // Specular highlight
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            g.fillEllipse(centreX - capRadius * 0.3f, centreY - capRadius * 0.4f, capRadius * 0.4f, capRadius * 0.25f);
        }
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearVertical)
        {
            auto trackWidth = 10.0f;
            auto trackX = (float) x + (float) width * 0.5f - trackWidth * 0.5f;

            // Neon glow behind track
            g.setColour(neonCyan.withAlpha(0.08f));
            g.fillRoundedRectangle(trackX - 6, (float) y - 4, trackWidth + 12, (float) height + 8, 8.0f);

            // Track background - dark bevel
            {
                juce::ColourGradient track(deepBlack, trackX, (float) y,
                                            midPurple.darker(0.5f), trackX + trackWidth, (float) y, false);
                g.setGradientFill(track);
                g.fillRoundedRectangle(trackX, (float) y, trackWidth, (float) height, 5.0f);
            }

            // Chrome border
            g.setColour(chromeDark.withAlpha(0.5f));
            g.drawRoundedRectangle(trackX, (float) y, trackWidth, (float) height, 5.0f, 1.0f);

            // Filled portion - neon cyan
            auto fillHeight = (float) height - sliderPos + (float) y;
            if (fillHeight > 0)
            {
                // Glow
                g.setColour(neonCyan.withAlpha(0.25f));
                g.fillRoundedRectangle(trackX - 4, sliderPos, trackWidth + 8, fillHeight + 3, 5.0f);

                // Fill gradient
                juce::ColourGradient fill(neonCyan, trackX, sliderPos,
                                           neonCyan.darker(0.4f), trackX + trackWidth, sliderPos, false);
                g.setGradientFill(fill);
                g.fillRoundedRectangle(trackX + 1, sliderPos, trackWidth - 2, fillHeight, 4.0f);

                // Bright core
                g.setColour(juce::Colours::white.withAlpha(0.3f));
                g.fillRoundedRectangle(trackX + 3, sliderPos + 2, trackWidth - 6, fillHeight - 4, 3.0f);
            }

            // Thumb - chrome fader cap
            auto thumbY = sliderPos - 14.0f;
            auto thumbWidth = 34.0f;
            auto thumbHeight = 28.0f;
            auto thumbX = (float) x + (float) width * 0.5f - thumbWidth * 0.5f;

            // Glow behind thumb
            g.setColour(neonMagenta.withAlpha(0.2f));
            g.fillRoundedRectangle(thumbX - 4, thumbY - 2, thumbWidth + 8, thumbHeight + 4, 6.0f);

            // Shadow
            g.setColour(juce::Colours::black.withAlpha(0.5f));
            g.fillRoundedRectangle(thumbX + 2, thumbY + 3, thumbWidth, thumbHeight, 4.0f);

            // Chrome body
            {
                juce::ColourGradient thumb(chromeLight, thumbX, thumbY,
                                            chromeDark, thumbX, thumbY + thumbHeight, false);
                thumb.addColour(0.4, chrome);
                thumb.addColour(0.6, chromeDark.brighter(0.1f));
                g.setGradientFill(thumb);
                g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 4.0f);
            }

            // Grip lines - darker
            g.setColour(chromeDark.withAlpha(0.6f));
            for (int i = 0; i < 4; ++i)
            {
                float ly = thumbY + 7 + i * 5;
                g.drawLine(thumbX + 8, ly, thumbX + thumbWidth - 8, ly, 1.5f);
            }

            // Top highlight
            g.setColour(juce::Colours::white.withAlpha(0.3f));
            g.fillRoundedRectangle(thumbX + 3, thumbY + 2, thumbWidth - 6, 4.0f, 2.0f);

            // Neon border
            g.setColour(neonMagenta.withAlpha(0.4f));
            g.drawRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 4.0f, 1.5f);
        }
        else
        {
            // Horizontal slider
            auto trackHeight = 8.0f;
            auto trackY = (float) y + (float) height * 0.5f - trackHeight * 0.5f;

            // Glow
            g.setColour(neonCyan.withAlpha(0.06f));
            g.fillRoundedRectangle((float) x - 4, trackY - 4, (float) width + 8, trackHeight + 8, 6.0f);

            // Track
            g.setColour(deepBlack);
            g.fillRoundedRectangle((float) x, trackY, (float) width, trackHeight, 4.0f);

            g.setColour(chromeDark.withAlpha(0.4f));
            g.drawRoundedRectangle((float) x, trackY, (float) width, trackHeight, 4.0f, 1.0f);

            // Fill
            auto fillWidth = sliderPos - (float) x;
            if (fillWidth > 0)
            {
                g.setColour(neonCyan.withAlpha(0.2f));
                g.fillRoundedRectangle((float) x - 2, trackY - 2, fillWidth + 4, trackHeight + 4, 5.0f);

                juce::ColourGradient fill(neonCyan, (float) x, trackY,
                                           neonCyan.darker(0.3f), (float) x, trackY + trackHeight, false);
                g.setGradientFill(fill);
                g.fillRoundedRectangle((float) x, trackY, fillWidth, trackHeight, 4.0f);
            }

            // Thumb
            auto thumbX = sliderPos - 10.0f;
            auto thumbHeight = 24.0f;
            auto thumbWidth = 20.0f;
            auto thumbY = (float) y + (float) height * 0.5f - thumbHeight * 0.5f;

            g.setColour(neonMagenta.withAlpha(0.15f));
            g.fillRoundedRectangle(thumbX - 3, thumbY - 2, thumbWidth + 6, thumbHeight + 4, 5.0f);

            g.setColour(juce::Colours::black.withAlpha(0.4f));
            g.fillRoundedRectangle(thumbX + 2, thumbY + 2, thumbWidth, thumbHeight, 4.0f);

            {
                juce::ColourGradient thumb(chromeLight, thumbX, thumbY,
                                            chromeDark, thumbX + thumbWidth, thumbY + thumbHeight, false);
                g.setGradientFill(thumb);
                g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 4.0f);
            }

            g.setColour(neonMagenta.withAlpha(0.3f));
            g.drawRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 4.0f, 1.0f);
        }
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        bool isOn = button.getToggleState();

        // Glow when on
        if (isOn)
        {
            g.setColour(neonCyan.withAlpha(0.2f));
            g.fillRoundedRectangle(bounds.expanded(3), 5.0f);
        }

        // Shadow
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillRoundedRectangle(bounds.translated(1, 2), 3.0f);

        // Background - chrome or active neon
        {
            auto bgColour = isOn ? neonCyan.darker(0.3f) : midPurple;
            juce::ColourGradient bg(isOn ? neonCyan.darker(0.1f) : chromeDark.darker(0.3f), bounds.getX(), bounds.getY(),
                                     bgColour, bounds.getX(), bounds.getBottom(), false);
            g.setGradientFill(bg);
            g.fillRoundedRectangle(bounds, 3.0f);
        }

        // Chrome edge highlight
        g.setColour((isOn ? neonCyan : chrome).withAlpha(0.4f));
        g.drawRoundedRectangle(bounds.reduced(1), 2.0f, 1.0f);

        // Border
        g.setColour(isOn ? neonCyan.withAlpha(0.8f) : chromeDark);
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

        // Text
        g.setColour(isOn ? juce::Colours::white : chrome);
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override
    {
        auto bounds = box.getLocalBounds().toFloat().reduced(1.0f);

        // Subtle glow
        g.setColour(neonMagenta.withAlpha(0.06f));
        g.fillRoundedRectangle(bounds.expanded(2), 4.0f);

        // Shadow
        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.fillRoundedRectangle(bounds.translated(1, 1), 3.0f);

        // Background - dark with chrome hint
        {
            juce::ColourGradient bg(midPurple.darker(0.3f), 0, 0, deepBlack, 0, (float) height, false);
            g.setGradientFill(bg);
            g.fillRoundedRectangle(bounds, 3.0f);
        }

        // Border
        g.setColour(chromeDark.withAlpha(0.5f));
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

        // Arrow - neon
        auto arrowZone = juce::Rectangle<float>((float) buttonX, (float) buttonY, (float) buttonW, (float) buttonH);
        juce::Path arrow;
        arrow.addTriangle(arrowZone.getCentreX() - 4, arrowZone.getCentreY() - 2,
                          arrowZone.getCentreX() + 4, arrowZone.getCentreY() - 2,
                          arrowZone.getCentreX(), arrowZone.getCentreY() + 3);

        // Arrow glow
        g.setColour(neonCyan.withAlpha(0.3f));
        g.fillPath(arrow);
        g.setColour(neonCyan);
        juce::Path arrowInner;
        arrowInner.addTriangle(arrowZone.getCentreX() - 3, arrowZone.getCentreY() - 1,
                               arrowZone.getCentreX() + 3, arrowZone.getCentreY() - 1,
                               arrowZone.getCentreX(), arrowZone.getCentreY() + 2);
        g.fillPath(arrowInner);
    }

    void drawLabel(juce::Graphics& g, juce::Label& label) override
    {
        auto font = getLabelFont(label);
        g.setFont(font);

        auto textArea = label.getBorderSize().subtractedFrom(label.getLocalBounds());

        // Subtle glow behind text
        g.setColour(neonCyan.withAlpha(0.15f));
        g.drawText(label.getText(), textArea.translated(0, 1), label.getJustificationType(), false);

        // Shadow
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawText(label.getText(), textArea.translated(1, 1), label.getJustificationType(), false);

        // Main text
        g.setColour(label.findColour(juce::Label::textColourId));
        g.drawText(label.getText(), textArea, label.getJustificationType(), false);
    }

    juce::Font getLabelFont(juce::Label& label) override
    {
        return juce::Font(12.0f, juce::Font::bold);
    }
};
