#include "../Source/PluginProcessor.h"
#include "../Source/Parameters.h"

#include <nf/testing/ProcessorHarness.h>

#include <juce_audio_processors/juce_audio_processors.h>

/**
    Category 4 — channel configurations.

    ## The known case, named before the set is run

    **Chorus-60 and TapeRot declare stereo only; the other four accept mono as well.** So a mono
    request must be REJECTED by exactly those two and accepted by the other four. If the instrument
    reports every layout supported everywhere, it is not reading the layout at all — which is the
    failure mode a "supports everything" result would otherwise sail through, and it is the same
    shape as a check that can only ever pass.

    ## What is asserted, and what is only reported

    Asserted: the accept/reject set matches what the casting declares, and every ACCEPTED layout
    produces finite, non-silent output rather than crashing or going dead. **Silence out of an
    accepted layout is the interesting failure** — a plugin that accepts mono and then produces
    nothing on it is broken in a way no stereo test sees.

    Reported only: TapeRot generates deliberately, so its non-silence proves less than the others'.

    ## THIS CASTING IS NOT MEASURED BY THE FIXTURE BELOW, and the run said so

    Gatecrasher declares mono, stereo AND a sidechain bus. The fixture builds a layout with ONE
    input bus, so it does not describe this processor's bus arrangement at all — and both arms came
    back REJECTED, mono and stereo alike, contradicting the casting's own declaration.

    **A result that contradicts a declaration is the instrument reporting on itself.** Read as a
    finding it would say Gatecrasher supports no layout whatsoever, which is false and would have
    been believed: every other casting's row is consistent, so nothing else in the table flags it.

    What the fixture needs is the sidechain bus added and exercised PRESENT and ABSENT, which is
    what the plan asks for and what nothing here does yet. Until then this casting's row is **not
    measured** rather than failing — the two are different claims and only one of them is true.
*/
class ChannelLayoutTests final : public juce::UnitTest
{
public:
    ChannelLayoutTests() : juce::UnitTest ("Channel layouts", "DSP") {}

    void runTest() override
    {
        beginTest ("Two input buses — main, and the sidechain present and absent");
        {
            // **The previous fixture built ONE input bus and this casting has two**, so
            // `getChannelSet (true, 1)` never described anything and every layout came back
            // rejected — contradicting the casting's own declaration, which is what said the
            // instrument was at fault rather than the code.
            //
            // What is actually declared (PluginProcessor.cpp:79-89): main input and output must
            // both be STEREO, and the sidechain may be disabled, mono or stereo.
            //
            // KNOWN CASES, named before the run and taken from that declaration rather than
            // invented: a MONO main must be rejected, and all three sidechain states must be
            // accepted. If mono main is accepted the fixture is not reading the layout; if a
            // sidechain state is rejected the declaration and the code disagree.
            struct Case { const char* label; juce::AudioChannelSet main; juce::AudioChannelSet side; bool expected; };

            const Case cases[] = {
                { "main stereo, sidechain disabled", juce::AudioChannelSet::stereo(), juce::AudioChannelSet::disabled(), true  },
                { "main stereo, sidechain mono",     juce::AudioChannelSet::stereo(), juce::AudioChannelSet::mono(),     true  },
                { "main stereo, sidechain stereo",   juce::AudioChannelSet::stereo(), juce::AudioChannelSet::stereo(),   true  },
                { "main MONO   (must be refused)",   juce::AudioChannelSet::mono(),   juce::AudioChannelSet::disabled(), false },
            };

            for (const auto& c : cases)
            {
                GatecrasherAudioProcessor processor;

                juce::AudioProcessor::BusesLayout layout;
                layout.inputBuses.add (c.main);
                layout.inputBuses.add (c.side);
                layout.outputBuses.add (juce::AudioChannelSet::stereo());

                const bool accepted = processor.checkBusesLayoutSupported (layout)
                                          && processor.setBusesLayout (layout);

                juce::String note;
                double peak = 0.0;
                bool finite = true;

                if (accepted)
                {
                    nf::testing::RenderSpec spec;
                    spec.blockSize = 512;
                    spec.numBlocks = 16;
                    spec.numChannels = processor.getTotalNumInputChannels();

                    const auto out = nf::testing::render (processor, spec);

                    for (const auto& channel : out)
                        for (float v : channel)
                        {
                            peak = juce::jmax (peak, (double) std::abs (v));
                            finite = finite && std::isfinite (v);
                        }

                    note = ", " + juce::String (spec.numChannels) + " ch in, peak "
                             + juce::String (peak, 6) + (finite ? "" : "   NON-FINITE");
                }

                logMessage ("  " + juce::String (c.label).paddedRight (' ', 34)
                                + (accepted ? "accepted" : "REJECTED") + note);

                expect (accepted == c.expected,
                        juce::String (c.label) + " came back "
                            + (accepted ? "accepted" : "rejected")
                            + " against its own declaration at PluginProcessor.cpp:79-89");

                if (accepted)
                    expect (finite, juce::String (c.label) + " produced non-finite samples");
            }

            // **The gate with nothing to listen to is REPORTED, not asserted.** With KEY SOURCE set
            // to SIDECHAIN and no sidechain connected, an externally-keyed gate has no key and
            // reads as permanently closed. That is documented suite behaviour rather than a defect,
            // and asserting either way would be asserting a design decision.
            {
                GatecrasherAudioProcessor processor;

                juce::AudioProcessor::BusesLayout layout;
                layout.inputBuses.add (juce::AudioChannelSet::stereo());
                layout.inputBuses.add (juce::AudioChannelSet::disabled());
                layout.outputBuses.add (juce::AudioChannelSet::stereo());

                if (processor.checkBusesLayoutSupported (layout) && processor.setBusesLayout (layout))
                {
                    if (auto* key = processor.apvts.getParameter (ParamIDs::keySource))
                        key->setValueNotifyingHost (1.0f);        // SIDECHAIN

                    nf::testing::RenderSpec spec;
                    spec.blockSize = 512;
                    spec.numBlocks = 16;
                    spec.numChannels = 2;

                    double peak = 0.0;
                    for (const auto& channel : nf::testing::render (processor, spec))
                        for (float v : channel)
                            peak = juce::jmax (peak, (double) std::abs (v));

                    logMessage ("  KEY SOURCE = SIDECHAIN, bus DISABLED   -> peak "
                                    + juce::String (peak, 6)
                                    + (peak < 1.0e-6 ? "   (gate closed)" : "   (falls back, passes signal)"));
                }
            }

            // **Two different situations that root CLAUDE.md's note runs together.** It says an
            // externally-keyed gate "reads as permanently closed" when the sidechain has nothing to
            // listen to — written about the standalone with no microphone permission, where the bus
            // EXISTS and delivers zeros. That is not the same as the bus being disabled, where the
            // plugin can see there is no sidechain at all and fall back.
            //
            // The arm above is bus-disabled. This one is bus-enabled-and-silent, which is the case
            // the note actually describes. Measuring both is what separates "falls back sensibly"
            // from "gate closed", and only one of them is the documented behaviour.
            {
                GatecrasherAudioProcessor processor;

                juce::AudioProcessor::BusesLayout layout;
                layout.inputBuses.add (juce::AudioChannelSet::stereo());
                layout.inputBuses.add (juce::AudioChannelSet::stereo());
                layout.outputBuses.add (juce::AudioChannelSet::stereo());

                if (processor.checkBusesLayoutSupported (layout) && processor.setBusesLayout (layout))
                {
                    if (auto* key = processor.apvts.getParameter (ParamIDs::keySource))
                        key->setValueNotifyingHost (1.0f);

                    // **MIX fully wet, so the residual cannot be the dry path.** This is a gated
                    // REVERB: the gate acts on the wet signal and MIX blends dry back in, so at any
                    // mix below 100% a perfectly closed gate still passes dry — and a residual would
                    // be correct rather than a leak. Fully wet is the only configuration in which
                    // "the gate is closed" and "the output is silent" are the same statement.
                    if (auto* mix = processor.apvts.getParameter (ParamIDs::mix))
                        mix->setValueNotifyingHost (1.0f);

                    nf::testing::RenderSpec spec;
                    spec.blockSize = 512;
                    spec.numBlocks = 16;
                    spec.numChannels = 4;

                    // Main gets signal; the sidechain pair is held at silence, which is exactly
                    // what an ungranted microphone delivers.
                    spec.fillInput = [] (juce::AudioBuffer<float>& buffer, int blockIndex)
                    {
                        buffer.clear();
                        juce::Random r (5150 + blockIndex);

                        for (int ch = 0; ch < juce::jmin (2, buffer.getNumChannels()); ++ch)
                            for (int i = 0; i < buffer.getNumSamples(); ++i)
                                buffer.setSample (ch, i, r.nextFloat() * 2.0f - 1.0f);
                    };

                    // **Peak over the whole render cannot tell "leaks throughout" from "opens
                    // once and then closes".** A gate whose envelope starts open produces one
                    // transient and then silence, and a peak reports that identically to a gate
                    // that never closes. So the last quarter is measured separately.
                    const auto out = nf::testing::render (processor, spec);

                    double peak = 0.0, tailPeak = 0.0;
                    const size_t tailFrom = out[0].size() * 3 / 4;

                    for (const auto& channel : out)
                        for (size_t i = 0; i < channel.size(); ++i)
                        {
                            const double v = std::abs ((double) channel[i]);
                            peak = juce::jmax (peak, v);

                            if (i >= tailFrom)
                                tailPeak = juce::jmax (tailPeak, v);
                        }

                    logMessage ("  KEY SOURCE = SIDECHAIN, bus SILENT     -> peak "
                                    + juce::String (peak, 6) + ", last-quarter peak "
                                    + juce::String (tailPeak, 6)
                                    + (tailPeak < 1.0e-6
                                           ? "   (opens once, then closed — the documented case)"
                                           : "   (STILL PASSING at " + juce::String (20.0 * std::log10 (tailPeak), 1)
                                                 + " dBFS)"));
                }
            }
        }

        beginTest ("Lifecycle — double prepare, rate change, reset, state round trip");
        {
            GatecrasherAudioProcessor processor;

            nf::testing::RenderSpec spec;
            spec.blockSize = 512;
            spec.numBlocks = 16;

            const auto report = nf::testing::exerciseLifecycle (processor, spec);

            logMessage ("  " + report.describe());

            // **`tailEnergyAfterReset` is REPORTED, never asserted, and the plan says why**: what
            // survives a reset that should not is the finding, and core cannot tell a reverb tail
            // (a defect) from a Program selection (correct) apart. The casting has to read it.
            expect (report.sampleRateChangeHandled,
                    "a mid-session sample-rate change was not handled: " + report.describe());

            expect (report.stateRoundTripMismatch.isEmpty(),
                    "a state round trip did not come back identical: " + report.stateRoundTripMismatch);
        }
    }
};

static ChannelLayoutTests channelLayoutTests;
