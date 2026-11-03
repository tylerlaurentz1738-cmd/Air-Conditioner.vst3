#include "PluginProcessor.h"
#include "PluginEditor.h"

using Coeffs = juce::dsp::IIR::Coefficients<float>;

juce::AudioProcessorValueTreeState::ParameterLayout AirConditionerAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    // Temperature 60-90. COLDER = STRONGER effect.
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "temp", 1 }, "Temperature", 60, 90, 72));
    // The switch: Purify (studio mic) / Distort / Limit
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "mode", 1 }, "Mode",
                                                              juce::StringArray { "Purify", "Distort", "Limit" }, 0));
    return layout;
}

AirConditionerAudioProcessor::AirConditionerAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool AirConditionerAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void AirConditionerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock,
                                  (juce::uint32) getTotalNumOutputChannels() };
    hpf.prepare (spec); mud.prepare (spec); presence.prepare (spec); air.prepare (spec);
    gate.prepare (spec); comp.prepare (spec); limiter.prepare (spec);

    gate.setRatio (3.0f);   gate.setAttack (2.0f);  gate.setRelease (150.0f);
    comp.setAttack (8.0f);  comp.setRelease (100.0f);
    limiter.setRelease (80.0f);
}

void AirConditionerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const float temp = apvts.getRawParameterValue ("temp")->load();
    const int mode   = (int) apvts.getRawParameterValue ("mode")->load();
    const float amt  = juce::jlimit (0.0f, 1.0f, (90.0f - temp) / 30.0f); // 0 = hot/off, 1 = ice cold/max

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    if (mode == 0) // ---------- PURIFY: make the mic sound "studio" ----------
    {
        *hpf.state      = *Coeffs::makeHighPass (sr, 90.0f);                                        // kill rumble/plosive thump
        *mud.state      = *Coeffs::makePeakFilter (sr, 300.0f, 1.0f,  juce::Decibels::decibelsToGain (-3.5f * amt)); // remove boxy mud
        *presence.state = *Coeffs::makePeakFilter (sr, 4000.0f, 0.8f, juce::Decibels::decibelsToGain (4.0f * amt));  // clarity
        *air.state      = *Coeffs::makeHighShelf (sr, 11000.0f, 0.7f, juce::Decibels::decibelsToGain (5.0f * amt));  // expensive "air"

        gate.setThreshold (-70.0f + 25.0f * amt);   // hush background noise
        comp.setThreshold (-18.0f);
        comp.setRatio (2.0f + 2.0f * amt);          // smooth, even level

        hpf.process (ctx); mud.process (ctx); presence.process (ctx); air.process (ctx);
        gate.process (ctx); comp.process (ctx);
    }
    else if (mode == 1) // ---------- DISTORT ----------
    {
        const float drive = 1.0f + 19.0f * amt;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                d[i] = 0.8f * std::tanh (d[i] * drive) / std::tanh (drive);
        }
    }
    else // ---------- LIMIT ----------
    {
        buffer.applyGain (juce::Decibels::decibelsToGain (12.0f * amt)); // push harder into the ceiling
        limiter.setThreshold (-1.0f);
        limiter.process (ctx);
    }
}

juce::AudioProcessorEditor* AirConditionerAudioProcessor::createEditor()
{
    return new AirConditionerAudioProcessorEditor (*this);
}

void AirConditionerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, destData);
}

void AirConditionerAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AirConditionerAudioProcessor(); }
