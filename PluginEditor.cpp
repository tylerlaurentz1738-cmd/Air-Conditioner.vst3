#include "PluginEditor.h"

AirConditionerAudioProcessorEditor::AirConditionerAudioProcessorEditor (AirConditionerAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setSize (560, 300);
    startTimerHz (30);
}

int AirConditionerAudioProcessorEditor::getTemp() const { return (int) proc.apvts.getRawParameterValue ("temp")->load(); }
int AirConditionerAudioProcessorEditor::getMode() const { return (int) proc.apvts.getRawParameterValue ("mode")->load(); }

void AirConditionerAudioProcessorEditor::setParam (const juce::String& id, float v)
{
    if (auto* prm = proc.apvts.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (v));
        prm->endChangeGesture();
    }
}

void AirConditionerAudioProcessorEditor::paint (juce::Graphics& g)
{
    using juce::Colour;
    g.fillAll (Colour (0xff1b1d21));

    // AC body
    auto body = getLocalBounds().toFloat().reduced (10.0f);
    g.setColour (Colour (0xffe8eaed));  g.fillRoundedRectangle (body, 18.0f);
    g.setColour (Colour (0xffb9bec5));  g.drawRoundedRectangle (body, 18.0f, 3.0f);

    // Title
    g.setColour (Colour (0xff55606e));
    g.setFont (juce::Font (juce::FontOptions (22.0f, juce::Font::bold)));
    g.drawText ("AIR CONDITIONER", 40, 24, 260, 28, juce::Justification::left);

    // Vents (louvers)
    for (int i = 0; i < 9; ++i)
    {
        g.setColour (Colour (0xffc3c8cf)); g.fillRoundedRectangle (40.0f, 70.0f + i * 20.0f, 250.0f, 10.0f, 5.0f);
        g.setColour (Colour (0xff8d949d)); g.fillRoundedRectangle (40.0f, 76.0f + i * 20.0f, 250.0f, 4.0f, 2.0f);
    }

    // Green digital display
    g.setColour (Colour (0xff050a06)); g.fillRoundedRectangle (display, 8.0f);
    g.setColour (Colour (0xff2a2f2b)); g.drawRoundedRectangle (display, 8.0f, 2.0f);
    auto text = juce::String (getTemp()) + juce::String::charToString (0x00B0) + "F";
    g.setFont (juce::Font (juce::FontOptions (44.0f, juce::Font::bold)));
    g.setColour (Colour (0x4400ff55)); g.drawText (text, display.translated (0, 1), juce::Justification::centred); // glow
    g.setColour (Colour (0xff22ff66)); g.drawText (text, display, juce::Justification::centred);

    // Up / down buttons
    for (auto* r : { &upBtn, &downBtn })
    {
        g.setColour (Colour (0xff3b4350)); g.fillRoundedRectangle (*r, 6.0f);
        juce::Path tri;
        auto c = r->getCentre();
        if (r == &upBtn) tri.addTriangle (c.x - 10, c.y + 6, c.x + 10, c.y + 6, c.x, c.y - 8);
        else             tri.addTriangle (c.x - 10, c.y - 6, c.x + 10, c.y - 6, c.x, c.y + 8);
        g.setColour (Colour (0xff22ff66)); g.fillPath (tri);
    }

    // 3-way mode switch
    g.setColour (Colour (0xff2a2f36)); g.fillRoundedRectangle (sw, 20.0f);
    const char* names[] = { "PURIFY", "DISTORT", "LIMIT" };
    const float segW = sw.getWidth() / 3.0f;
    for (int i = 0; i < 3; ++i)
    {
        auto seg = juce::Rectangle<float> (sw.getX() + i * segW, sw.getY(), segW, sw.getHeight()).reduced (3.0f);
        if (i == getMode()) { g.setColour (Colour (0xff22ff66)); g.fillRoundedRectangle (seg, 17.0f); }
        g.setColour (i == getMode() ? Colour (0xff04140a) : Colour (0xff9aa3af));
        g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        g.drawText (names[i], seg, juce::Justification::centred);
    }

    g.setColour (Colour (0xff6b7480));
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText ("COLDER = MORE EFFECT", 330, 232, 190, 18, juce::Justification::centred);
}

void AirConditionerAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    auto p = e.position;
    if (upBtn.contains (p))        setParam ("temp", (float) juce::jmin (90, getTemp() + 1));
    else if (downBtn.contains (p)) setParam ("temp", (float) juce::jmax (60, getTemp() - 1));
    else if (sw.contains (p))      setParam ("mode", (float) juce::jlimit (0, 2, (int) ((p.x - sw.getX()) / (sw.getWidth() / 3.0f))));
}

void AirConditionerAudioProcessorEditor::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    setParam ("temp", (float) juce::jlimit (60, 90, getTemp() + (w.deltaY > 0 ? 1 : -1)));
}
