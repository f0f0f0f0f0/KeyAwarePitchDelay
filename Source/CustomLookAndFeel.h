#pragma once

#include <JuceHeader.h>

class RetroLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Industrial / Grungy palette (inspired by Freakshow/Pladask)
    const juce::Colour panelDark     = juce::Colour(0xFF1A1A1E);
    const juce::Colour panelMid      = juce::Colour(0xFF2A2A30);
    const juce::Colour panelLight    = juce::Colour(0xFF3A3A42);
    const juce::Colour metalDark     = juce::Colour(0xFF4A4A52);
    const juce::Colour metalMid      = juce::Colour(0xFF6A6A72);
    const juce::Colour metalLight    = juce::Colour(0xFF9A9AA2);
    const juce::Colour chrome        = juce::Colour(0xFFB8B8C0);
    const juce::Colour chromeLight   = juce::Colour(0xFFD8D8E0);

    // Accent colors - industrial teal/rust/pink
    const juce::Colour tealPanel     = juce::Colour(0xFF2D4A4A);
    const juce::Colour tealAccent    = juce::Colour(0xFF4A8888);
    const juce::Colour rustOrange    = juce::Colour(0xFFB86830);
    const juce::Colour warnRed       = juce::Colour(0xFFCC3333);
    const juce::Colour hotPink       = juce::Colour(0xFFFF4488);
    const juce::Colour indicatorGreen= juce::Colour(0xFF44CC44);
    const juce::Colour indicatorAmber= juce::Colour(0xFFCCAA22);

    RetroLookAndFeel()
    {
        setColour(juce::Slider::thumbColourId, chrome);
        setColour(juce::Slider::rotarySliderFillColourId, tealAccent);
        setColour(juce::Slider::rotarySliderOutlineColourId, panelDark);
        setColour(juce::Slider::trackColourId, tealAccent);

        setColour(juce::ComboBox::backgroundColourId, panelDark);
        setColour(juce::ComboBox::outlineColourId, metalDark);
        setColour(juce::ComboBox::textColourId, chrome);
        setColour(juce::ComboBox::arrowColourId, tealAccent);

        setColour(juce::PopupMenu::backgroundColourId, panelMid);
        setColour(juce::PopupMenu::textColourId, chrome);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, tealPanel);
        setColour(juce::PopupMenu::highlightedTextColourId, chromeLight);

        setColour(juce::ToggleButton::textColourId, chrome);
        setColour(juce::ToggleButton::tickColourId, indicatorGreen);

        setColour(juce::Label::textColourId, chrome.withAlpha(0.9f));

        setColour(juce::TextEditor::backgroundColourId, panelDark);
        setColour(juce::TextEditor::textColourId, chromeLight);
        setColour(juce::TextEditor::outlineColourId, metalDark);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        auto radius = (float) juce::jmin(width / 2, height / 2) - 6.0f;
        auto centreX = (float) x + (float) width * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // === INDUSTRIAL KNOB STYLE ===

        // Outer shadow/bevel
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.fillEllipse(rx - 2, ry + 3, rw + 4, rw + 4);

        // Metal bezel ring
        {
            juce::ColourGradient bezel(metalLight, centreX - radius, centreY - radius,
                                        metalDark, centreX + radius, centreY + radius, false);
            bezel.addColour(0.3, chrome);
            bezel.addColour(0.5, metalMid);
            bezel.addColour(0.7, metalDark);
            g.setGradientFill(bezel);
            g.fillEllipse(rx - 4, ry - 4, rw + 8, rw + 8);
        }

        // Dark inset groove
        g.setColour(panelDark);
        g.fillEllipse(rx, ry, rw, rw);

        // Main knob body - brushed metal with radial texture
        {
            juce::ColourGradient knob(metalLight, centreX - radius * 0.6f, centreY - radius * 0.6f,
                                       metalDark, centreX + radius * 0.4f, centreY + radius * 0.5f, true);
            knob.addColour(0.2, chrome.withAlpha(0.9f));
            knob.addColour(0.5, metalMid);
            knob.addColour(0.8, metalDark.brighter(0.1f));
            g.setGradientFill(knob);
            g.fillEllipse(rx + 3, ry + 3, rw - 6, rw - 6);
        }

        // Brushed metal texture (radial lines)
        g.setColour(juce::Colours::black.withAlpha(0.05f));
        for (int i = 0; i < 36; ++i)
        {
            float a = i * 10.0f * 3.14159f / 180.0f;
            float x1 = centreX + (radius - 15) * std::cos(a);
            float y1 = centreY + (radius - 15) * std::sin(a);
            float x2 = centreX + (radius - 5) * std::cos(a);
            float y2 = centreY + (radius - 5) * std::sin(a);
            g.drawLine(x1, y1, x2, y2, 0.5f);
        }

        // Knurled edge texture
        g.setColour(metalDark.withAlpha(0.4f));
        for (int i = 0; i < 48; ++i)
        {
            float a = i * 7.5f * 3.14159f / 180.0f;
            float ex = centreX + (radius - 1) * std::cos(a);
            float ey = centreY + (radius - 1) * std::sin(a);
            g.fillEllipse(ex - 1.5f, ey - 1.5f, 3.0f, 3.0f);
        }

        // Position indicator notches around the edge
        g.setColour(panelDark);
        for (int i = 0; i <= 10; ++i)
        {
            float notchAngle = rotaryStartAngle + (i / 10.0f) * (rotaryEndAngle - rotaryStartAngle);
            float nx1 = centreX + (radius + 6) * std::cos(notchAngle);
            float ny1 = centreY + (radius + 6) * std::sin(notchAngle);
            float nx2 = centreX + (radius + 10) * std::cos(notchAngle);
            float ny2 = centreY + (radius + 10) * std::sin(notchAngle);
            g.drawLine(nx1, ny1, nx2, ny2, (i == 0 || i == 5 || i == 10) ? 2.0f : 1.0f);
        }

        // Main pointer - thick industrial style
        {
            juce::Path pointer;
            auto pointerLength = radius - 8.0f;
            auto pointerWidth = 4.0f;
            pointer.addRoundedRectangle(-pointerWidth / 2, -radius + 6.0f, pointerWidth, pointerLength, 1.5f);
            pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

            // Pointer shadow
            g.setColour(juce::Colours::black.withAlpha(0.5f));
            g.fillPath(pointer, juce::AffineTransform::translation(1, 1));

            // Pointer body - bright indicator line
            g.setColour(chromeLight);
            g.fillPath(pointer);

            // Hot pink/red tip for visibility
            juce::Path tip;
            tip.addRoundedRectangle(-pointerWidth / 2, -radius + 6.0f, pointerWidth, 8.0f, 1.5f);
            tip.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
            g.setColour(hotPink);
            g.fillPath(tip);
        }

        // Center cap - industrial dome
        {
            auto capRadius = radius * 0.25f;

            // Shadow
            g.setColour(juce::Colours::black.withAlpha(0.4f));
            g.fillEllipse(centreX - capRadius + 1, centreY - capRadius + 2, capRadius * 2, capRadius * 2);

            // Chrome dome
            juce::ColourGradient cap(chromeLight, centreX - capRadius * 0.4f, centreY - capRadius * 0.4f,
                                      metalDark, centreX + capRadius * 0.3f, centreY + capRadius * 0.4f, true);
            g.setGradientFill(cap);
            g.fillEllipse(centreX - capRadius, centreY - capRadius, capRadius * 2, capRadius * 2);

            // Specular highlight
            g.setColour(juce::Colours::white.withAlpha(0.5f));
            g.fillEllipse(centreX - capRadius * 0.4f, centreY - capRadius * 0.5f, capRadius * 0.5f, capRadius * 0.3f);
        }
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearVertical)
        {
            // === INDUSTRIAL FADER ===
            auto trackWidth = 12.0f;
            auto trackX = (float) x + (float) width * 0.5f - trackWidth * 0.5f;

            // Track shadow
            g.setColour(juce::Colours::black.withAlpha(0.5f));
            g.fillRoundedRectangle(trackX + 2, (float) y + 2, trackWidth, (float) height, 3.0f);

            // Track groove - dark metal
            {
                juce::ColourGradient track(panelDark, trackX, (float) y,
                                            panelMid, trackX + trackWidth, (float) y, false);
                g.setGradientFill(track);
                g.fillRoundedRectangle(trackX, (float) y, trackWidth, (float) height, 3.0f);
            }

            // Track border
            g.setColour(metalDark.withAlpha(0.6f));
            g.drawRoundedRectangle(trackX, (float) y, trackWidth, (float) height, 3.0f, 1.0f);

            // Filled portion - teal indicator
            auto fillHeight = (float) height - sliderPos + (float) y;
            if (fillHeight > 0)
            {
                juce::ColourGradient fill(tealAccent, trackX, sliderPos,
                                           tealPanel, trackX + trackWidth, sliderPos, false);
                g.setGradientFill(fill);
                g.fillRoundedRectangle(trackX + 2, sliderPos, trackWidth - 4, fillHeight - 2, 2.0f);
            }

            // Fader thumb - industrial metal cap
            auto thumbY = sliderPos - 16.0f;
            auto thumbWidth = 36.0f;
            auto thumbHeight = 32.0f;
            auto thumbX = (float) x + (float) width * 0.5f - thumbWidth * 0.5f;

            // Shadow
            g.setColour(juce::Colours::black.withAlpha(0.5f));
            g.fillRoundedRectangle(thumbX + 2, thumbY + 3, thumbWidth, thumbHeight, 4.0f);

            // Metal body
            {
                juce::ColourGradient thumb(metalLight, thumbX, thumbY,
                                            metalDark, thumbX, thumbY + thumbHeight, false);
                thumb.addColour(0.15, chrome);
                thumb.addColour(0.5, metalMid);
                thumb.addColour(0.85, metalDark);
                g.setGradientFill(thumb);
                g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 4.0f);
            }

            // Grip ridges
            g.setColour(panelDark.withAlpha(0.5f));
            for (int i = 0; i < 5; ++i)
            {
                float ly = thumbY + 6 + i * 5;
                g.drawLine(thumbX + 6, ly, thumbX + thumbWidth - 6, ly, 1.5f);
            }

            // Top highlight
            g.setColour(chromeLight.withAlpha(0.4f));
            g.fillRoundedRectangle(thumbX + 3, thumbY + 2, thumbWidth - 6, 3.0f, 1.5f);

            // Center indicator line
            g.setColour(hotPink);
            g.fillRect(thumbX + thumbWidth / 2 - 1.5f, thumbY + 4, 3.0f, thumbHeight - 8);
        }
        else
        {
            // === HORIZONTAL SLIDER ===
            auto trackHeight = 10.0f;
            auto trackY = (float) y + (float) height * 0.5f - trackHeight * 0.5f;

            // Track shadow
            g.setColour(juce::Colours::black.withAlpha(0.4f));
            g.fillRoundedRectangle((float) x + 1, trackY + 2, (float) width, trackHeight, 3.0f);

            // Track groove
            g.setColour(panelDark);
            g.fillRoundedRectangle((float) x, trackY, (float) width, trackHeight, 3.0f);
            g.setColour(metalDark.withAlpha(0.5f));
            g.drawRoundedRectangle((float) x, trackY, (float) width, trackHeight, 3.0f, 1.0f);

            // Fill
            auto fillWidth = sliderPos - (float) x;
            if (fillWidth > 0)
            {
                juce::ColourGradient fill(tealAccent, (float) x, trackY,
                                           tealPanel.darker(0.3f), (float) x, trackY + trackHeight, false);
                g.setGradientFill(fill);
                g.fillRoundedRectangle((float) x + 1, trackY + 1, fillWidth - 1, trackHeight - 2, 2.0f);
            }

            // Thumb
            auto thumbX = sliderPos - 12.0f;
            auto thumbHeight = 26.0f;
            auto thumbWidth = 24.0f;
            auto thumbY = (float) y + (float) height * 0.5f - thumbHeight * 0.5f;

            g.setColour(juce::Colours::black.withAlpha(0.4f));
            g.fillRoundedRectangle(thumbX + 2, thumbY + 2, thumbWidth, thumbHeight, 4.0f);

            {
                juce::ColourGradient thumb(metalLight, thumbX, thumbY,
                                            metalDark, thumbX + thumbWidth, thumbY + thumbHeight, false);
                g.setGradientFill(thumb);
                g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 4.0f);
            }

            // Grip lines
            g.setColour(panelDark.withAlpha(0.4f));
            for (int i = 0; i < 3; ++i)
            {
                float lx = thumbX + 6 + i * 5;
                g.drawLine(lx, thumbY + 5, lx, thumbY + thumbHeight - 5, 1.5f);
            }
        }
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        bool isOn = button.getToggleState();

        // Shadow
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillRoundedRectangle(bounds.translated(1, 2), 3.0f);

        // Button body - industrial switch style
        {
            auto bgColour = isOn ? tealPanel : panelMid;
            juce::ColourGradient bg(isOn ? tealAccent.darker(0.2f) : metalDark, bounds.getX(), bounds.getY(),
                                     bgColour, bounds.getX(), bounds.getBottom(), false);
            g.setGradientFill(bg);
            g.fillRoundedRectangle(bounds, 3.0f);
        }

        // Indicator LED
        auto ledSize = 6.0f;
        auto ledX = bounds.getRight() - ledSize - 4;
        auto ledY = bounds.getCentreY() - ledSize / 2;

        // LED glow
        if (isOn)
        {
            g.setColour(indicatorGreen.withAlpha(0.3f));
            g.fillEllipse(ledX - 3, ledY - 3, ledSize + 6, ledSize + 6);
        }

        g.setColour(isOn ? indicatorGreen : panelDark);
        g.fillEllipse(ledX, ledY, ledSize, ledSize);

        // LED highlight
        if (isOn)
        {
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            g.fillEllipse(ledX + 1, ledY + 1, ledSize * 0.4f, ledSize * 0.4f);
        }

        // Border
        g.setColour(metalDark);
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

        // Text
        auto textBounds = bounds.withTrimmedRight(ledSize + 8);
        g.setColour(isOn ? chromeLight : chrome.withAlpha(0.8f));
        g.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
        g.drawText(button.getButtonText(), textBounds, juce::Justification::centred);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override
    {
        auto bounds = box.getLocalBounds().toFloat().reduced(1.0f);

        // Shadow
        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.fillRoundedRectangle(bounds.translated(1, 1), 3.0f);

        // Background - dark metal panel
        {
            juce::ColourGradient bg(panelMid, 0, 0, panelDark, 0, (float) height, false);
            g.setGradientFill(bg);
            g.fillRoundedRectangle(bounds, 3.0f);
        }

        // Border
        g.setColour(metalDark.withAlpha(0.6f));
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

        // Arrow
        auto arrowZone = juce::Rectangle<float>((float) buttonX, (float) buttonY, (float) buttonW, (float) buttonH);
        juce::Path arrow;
        arrow.addTriangle(arrowZone.getCentreX() - 4, arrowZone.getCentreY() - 2,
                          arrowZone.getCentreX() + 4, arrowZone.getCentreY() - 2,
                          arrowZone.getCentreX(), arrowZone.getCentreY() + 3);

        g.setColour(tealAccent);
        g.fillPath(arrow);
    }

    void drawLabel(juce::Graphics& g, juce::Label& label) override
    {
        auto font = getLabelFont(label);
        g.setFont(font);

        auto textArea = label.getBorderSize().subtractedFrom(label.getLocalBounds());

        // Subtle shadow
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.drawText(label.getText(), textArea.translated(1, 1), label.getJustificationType(), false);

        // Main text
        g.setColour(label.findColour(juce::Label::textColourId));
        g.drawText(label.getText(), textArea, label.getJustificationType(), false);
    }

    juce::Font getLabelFont(juce::Label& label) override
    {
        return juce::Font(juce::FontOptions(11.0f).withStyle("Bold"));
    }
};
