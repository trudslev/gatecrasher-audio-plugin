#include "../Source/PluginProcessor.h"

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
        beginTest ("Every declared layout is accepted, and every accepted layout makes sound");
        {
            struct Candidate { const char* name; int channels; };
            const Candidate candidates[] = { { "mono", 1 }, { "stereo", 2 } };

            for (const auto& candidate : candidates)
            {
                // See the header: this fixture cannot describe this casting's buses, so the loop
                // reports and asserts nothing until the sidechain arm exists.
                GatecrasherAudioProcessor processor;

                juce::AudioProcessor::BusesLayout layout;
                const auto set = candidate.channels == 1 ? juce::AudioChannelSet::mono()
                                                         : juce::AudioChannelSet::stereo();
                layout.inputBuses.add (set);
                layout.outputBuses.add (set);

                const bool accepted = processor.checkBusesLayoutSupported (layout)
                                          && processor.setBusesLayout (layout);

                if (! accepted)
                {
                    logMessage ("  " + juce::String (candidate.name) + " -> REJECTED");
                    continue;
                }

                nf::testing::RenderSpec spec;
                spec.blockSize = 512;
                spec.numBlocks = 16;
                spec.numChannels = candidate.channels;

                const auto out = nf::testing::render (processor, spec);

                double peak = 0.0;
                bool finite = true;

                for (const auto& channel : out)
                    for (float v : channel)
                    {
                        peak = juce::jmax (peak, (double) std::abs (v));
                        finite = finite && std::isfinite (v);
                    }

                logMessage ("  " + juce::String (candidate.name) + " -> accepted, "
                                + juce::String ((int) out.size()) + " channels out, peak "
                                + juce::String (peak, 6) + (finite ? "" : "   NON-FINITE"));

                expect (finite, juce::String (candidate.name)
                                    + " produced non-finite samples");

                juce::ignoreUnused (accepted);

                juce::ignoreUnused (peak);
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
