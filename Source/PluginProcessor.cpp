#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <nf/BlockChunking.h>
#include <cmath>

GatecrasherAudioProcessor::GatecrasherAudioProcessor()
    : AudioProcessor(BusesProperties()
                          .withInput("Input", juce::AudioChannelSet::stereo(), true)
                          .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
                          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createGatecrasherParameterLayout()),
      programManager(apvts)
{
    thresholdParam = apvts.getRawParameterValue(ParamIDs::threshold);
    trigHPParam = apvts.getRawParameterValue(ParamIDs::trigHP);
    trigLPParam = apvts.getRawParameterValue(ParamIDs::trigLP);
    keySourceParam = apvts.getRawParameterValue(ParamIDs::keySource);
    attackParam = apvts.getRawParameterValue(ParamIDs::attack);
    holdParam = apvts.getRawParameterValue(ParamIDs::hold);
    releaseParam = apvts.getRawParameterValue(ParamIDs::release);
    shapeParam = apvts.getRawParameterValue(ParamIDs::shape);
    algorithmParam = apvts.getRawParameterValue(ParamIDs::algorithm);
    sizeParam = apvts.getRawParameterValue(ParamIDs::size);
    preDelayParam = apvts.getRawParameterValue(ParamIDs::preDelay);
    decayParam = apvts.getRawParameterValue(ParamIDs::decay);
    densityParam = apvts.getRawParameterValue(ParamIDs::density);
    dampHFParam = apvts.getRawParameterValue(ParamIDs::dampHF);
    dampLFParam = apvts.getRawParameterValue(ParamIDs::dampLF);
    slamParam = apvts.getRawParameterValue(ParamIDs::slam);
    widthParam = apvts.getRawParameterValue(ParamIDs::width);
    mixParam = apvts.getRawParameterValue(ParamIDs::mix);
    trimParam = apvts.getRawParameterValue(ParamIDs::trim);

    // updateHostDisplay is an AudioProcessor member ProgramManager can't call directly (it
    // deliberately doesn't know about juce::AudioProcessor at all - see ProgramManager.h) - wired
    // here instead.
    programManager.onProgramListChanged = [this]
    {
        updateHostDisplay(juce::AudioProcessorListener::ChangeDetails().withProgramChanged(true));
    };

    // Construction is single-threaded with no host/automation attached yet, so applying the
    // default program synchronously here (rather than through ProgramManager's async
    // requestProgramChange path) is safe.
    programManager.initialise();
}

void GatecrasherAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec{ sampleRate, (juce::uint32) samplesPerBlock,
                                  (juce::uint32) getMainBusNumOutputChannels() };

    triggerDetector.prepare(spec);
    gateEnvelopeGenerator.prepare(sampleRate);

    const int maxPreDelaySamples = (int) std::ceil(0.001 * 120.0 * sampleRate) + 4;
    preDelayLine.setMaximumDelayInSamples(maxPreDelaySamples);
    preDelayLine.prepare(spec);
    preDelayLine.reset();

    /*  **Prepared knowing which tank is selected, rather than discovering it on the first block.**
        `currentAlgorithm` was constructed to `plate` while the default Program selects ROOM, so
        every instance crossfaded on its first block — 0.041, from a state nobody chose. The map is
        the same one processBlock uses; duplicating it would be a second place to get the
        choice-index-to-tank correspondence wrong. */
    reverbEngine.prepare(spec, tankForAlgorithmChoice((int) algorithmParam->load()));
    dampingStage.prepare(spec);
    slamSaturation.prepare(spec);
    stereoWidthStage.prepare(spec);
    outputMixStage.prepare(spec);

    dryBuffer.setSize((int) spec.numChannels, samplesPerBlock, false, false, true);
    triggerLevelScratch.reserve((size_t) samplesPerBlock);
    gateGainScratch.reserve((size_t) samplesPerBlock);

    inputMeterLevel.store(0.0f, std::memory_order_relaxed);
    outputMeterLevel.store(0.0f, std::memory_order_relaxed);
    triggerLevelDisplay.store(0.0f, std::memory_order_relaxed);
}

//==============================================================================
/** A host's reset - a transport locate, a buffer clear - propagated to the DSP.

    **JUCE's base implementation is a no-op, and none of the six castings overrode it**, so until
    stage 1c a host asking every plugin in the session to clear itself was answered by nothing
    anywhere. Measured tails surviving a reset: Gatecrasher 0.679, Chorus-60 0.429, Reflect-84 0.111.

    Routed to the same per-stage `reset()` calls `prepareToPlay` already makes, and deliberately NOT
    to `prepareToPlay` itself: re-preparing would also re-run whatever a prepare re-arms, and this
    suite has a measured example of that being audible.
*/
void GatecrasherAudioProcessor::reset()
{
    triggerDetector.reset();
    gateEnvelopeGenerator.reset();
    preDelayLine.reset();
    reverbEngine.reset();
    dampingStage.reset();
    slamSaturation.reset();
    stereoWidthStage.reset();
    outputMixStage.reset();
}

void GatecrasherAudioProcessor::releaseResources()
{
}

bool GatecrasherAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()
        || layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    const auto sidechainSet = layouts.getChannelSet(true, 1);
    return sidechainSet == juce::AudioChannelSet::disabled()
        || sidechainSet == juce::AudioChannelSet::mono()
        || sidechainSet == juce::AudioChannelSet::stereo();
}

void GatecrasherAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // **The over-delivery policy.** dryBuffer.setSize and the two scratch resizes below all grow
    // when a host sends more samples than it declared, which is a heap allocation on the audio
    // thread. Chunking removes all three at once: no span is ever longer than the prepared size, so
    // there is nothing left to grow.
    //
    // ScopedNoDenormals stays OUTSIDE - it is scoped, so once per call is correct and cheaper than
    // once per span.
    //
    // **The bus extraction moves INSIDE**, because each span needs its own view of both buses.
    // getBusBuffer computes a channel offset and a length, and a span has the same channel count
    // with its own pointers, so asking it per span is exactly right - asking it once outside would
    // hand every span the whole block's length.
    //
    // The body below is otherwise unchanged and deliberately not re-indented, so the diff shows the
    // wrapper rather than every line of the DSP.
    nf::processInChunks(buffer, getBlockSize(), [&](juce::AudioBuffer<float>& span)
    {
    auto mainIO = getBusBuffer(span, true, 0);
    const int numSamples = mainIO.getNumSamples();
    const int numChannels = mainIO.getNumChannels();

    const bool sidechainConnected = getBus(true, 1) != nullptr && getBus(true, 1)->isEnabled();
    auto sidechainBuffer = sidechainConnected ? getBusBuffer(span, true, 1) : juce::AudioBuffer<float>();

    dryBuffer.setSize(numChannels, numSamples, false, false, true);
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom(ch, 0, mainIO, ch, 0, numSamples);

    // Trigger detection (reads main input or sidechain, per Key Source - never touches the signal)
    const int keySource = (int) keySourceParam->load();
    triggerLevelScratch.resize((size_t) numSamples);
    triggerDetector.process(mainIO, sidechainConnected ? &sidechainBuffer : nullptr,
                             keySource == 1, trigHPParam->load(), trigLPParam->load(),
                             triggerLevelScratch.data(), numSamples);
    if (numSamples > 0)
        triggerLevelDisplay.store(triggerLevelScratch.back(), std::memory_order_relaxed);

    // Gate envelope, computed in parallel with the reverb tank below - applied as a multiply after
    // the tank/damping stages (see class comment in GateEnvelopeGenerator.h and the plan's signal
    // flow: the gate chops the wet tail, it doesn't gate the tank's input).
    const int shapeIndex = (int) shapeParam->load();
    const GateShape shape = shapeIndex == 1 ? GateShape::soft : GateShape::hard;
    gateGainScratch.resize((size_t) numSamples);
    gateEnvelopeGenerator.process(triggerLevelScratch.data(), numSamples,
                                   thresholdParam->load(), attackParam->load(),
                                   holdParam->load(), releaseParam->load(), shape,
                                   gateGainScratch.data());

    // Pre-delay sits ahead of the tank, on the wet path only - the dry tap above was already
    // captured pre-delay, so the Mix blend still compares against the true dry signal.
    preDelayLine.setDelay((float) (0.001 * preDelayParam->load() * getSampleRate()));
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = mainIO.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            preDelayLine.pushSample(ch, data[i]);
            data[i] = preDelayLine.popSample(ch);
        }
    }

    // The parameter's choice order is the PANEL's (Ambience, Room, Plate, Chamber - spec section
    // 9.1); ReverbAlgorithm's is the DSP's own and is unrelated. Map explicitly rather than casting
    // the index: a static_cast happens to compile and would silently run the wrong tank.
    reverbEngine.process(mainIO, tankForAlgorithmChoice((int) algorithmParam->load()),
                          sizeParam->load(), decayParam->load(), densityParam->load());

    dampingStage.process(mainIO, dampHFParam->load(), dampLFParam->load());

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = mainIO.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
            data[i] *= gateGainScratch[(size_t) i];
    }

    slamSaturation.process(mainIO, slamParam->load());
    stereoWidthStage.process(mainIO, widthParam->load());
    outputMixStage.process(mainIO, dryBuffer, mixParam->load(), trimParam->load());

    // Input meter ballistics: fast attack, slow release (~0.12 release coefficient, per
    // GUI-SPEC.md section 7), tracked off the dry input - deliberately independent of
    // TriggerDetector's own much faster ballistics, which exist to make accurate gate decisions,
    // not to look good on a meter.
    float peak = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* data = dryBuffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            peak = juce::jmax(peak, std::abs(data[i]));
    }
    const float current = inputMeterLevel.load(std::memory_order_relaxed);
    const float coeff = peak > current ? 0.5f : 0.12f;
    inputMeterLevel.store(current + coeff * (peak - current), std::memory_order_relaxed);

    // Same ballistics, read off the final post-mix buffer instead of the dry input - this is what
    // the header's live "OUT" LED (ProgramHeader::paint) reads, alongside "IN" reading
    // getInputMeterLevel() above, so the two numeric readouts and the INPUT/TRIGGER segment meter
    // all agree on what "the current level" means rather than drifting out of sync with each other.
    float outPeak = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* data = mainIO.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            outPeak = juce::jmax(outPeak, std::abs(data[i]));
    }
    const float currentOut = outputMeterLevel.load(std::memory_order_relaxed);
    const float coeffOut = outPeak > currentOut ? 0.5f : 0.12f;
    outputMeterLevel.store(currentOut + coeffOut * (outPeak - currentOut), std::memory_order_relaxed);
    });
}

juce::AudioProcessorEditor* GatecrasherAudioProcessor::createEditor()
{
    return new GatecrasherAudioProcessorEditor(*this);
}

void GatecrasherAudioProcessor::setCurrentProgram(int index)
{
    if (! juce::isPositiveAndBelow(index, programManager.getNumPrograms()))
        return;

    // The stale-replay guard, disarmed by this call whether or not it is honoured. A replay carries
    // the position we last reported, so a matching index right after a restore is ignored; anything
    // else, and every later call, applies normally.
    if (userEdits.consumeRestore() && index == getCurrentProgram())
        return;

    programManager.requestProgramChange(ProgramManager::factoryIdAt(index));
}

void GatecrasherAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    xml->setAttribute(LegacyMigration::stateSchemaVersionAttribute, LegacyMigration::currentStateSchemaVersion);
    // Sticky display metadata only - restored clamped/defaulted below, never re-validated against
    // the session's actual knob values (a session saved after tweaking a loaded program still
    // remembers which program it was tweaked from, even though it was never itself Saved).
    // **The bank, the identifier, and the full parameter state.** The values are what makes the
    // session sound right; the identity only decides what the panel CALLS them - so whatever
    // happens to the bank between versions, a session restores its sound and at worst loses a name.
    const auto id = programManager.getCurrentProgramId();
    xml->setAttribute(LegacyMigration::programBankAttribute, LegacyMigration::bankAttributeValue(id.bank));
    xml->setAttribute(LegacyMigration::programIdAttribute, id.id);
    xml->setAttribute(LegacyMigration::programNameAttribute, id.displayName);
    copyXmlToBinary(*xml, destData);
}

void GatecrasherAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes)); xml != nullptr)
        if (xml->hasTagName(apvts.state.getType()))
        {
            programManager.cancelPendingChange();

            // Sessions written before the Algorithm reorder store the old index meaning - remap
            // before the state goes in, not after, or the wrong tank is briefly live.
            LegacyMigration::remapLegacyAlgorithmIfNeeded(*xml);

            apvts.replaceState(juce::ValueTree::fromXml(*xml));

            const int savedSchema =
                xml->getIntAttribute(LegacyMigration::stateSchemaVersionAttribute, 1);
            ProgramId restored;

            if (savedSchema >= LegacyMigration::identitySchemaVersion)
            {
                restored = programManager.resolve(
                    LegacyMigration::bankFromAttribute(
                        xml->getStringAttribute(LegacyMigration::programBankAttribute)),
                    xml->getStringAttribute(LegacyMigration::programIdAttribute),
                    xml->getStringAttribute(LegacyMigration::programNameAttribute));
            }
            else
            {
                // Older sessions stored a position. Map it through the CURRENT bank - correct here
                // because nothing has shipped and the bank has not moved.
                const int savedIndex =
                    xml->getIntAttribute("gatecrasherCurrentProgramIndex", defaultFactoryProgramIndex);

                if (savedIndex == -1)
                    restored = ProgramManager::initId();
                else if (juce::isPositiveAndBelow(savedIndex, kNumFactoryPrograms))
                    restored = ProgramManager::factoryIdAt(savedIndex);
                else
                    restored = ProgramManager::factoryIdAt(defaultFactoryProgramIndex);
            }

            programManager.setCurrentProgramWithoutApplying(restored);

            // **Armed AFTER replaceState**, or the restore's own parameter writes would look like
            // activity and disarm it immediately.
            userEdits.armRestore();
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GatecrasherAudioProcessor();
}
