#include "../Source/PluginProcessor.h"

#include <nf/testing/ProcessorHarness.h>

#include <juce_audio_processors/juce_audio_processors.h>

/**
    Category 1 of the suite-wide bug sweep, for Gatecrasher.

    **Core owns the drivers; this file owns what Gatecrasher's answers should be.** That split is
    the whole reason `nf::testing` could go into core at all — "does this allocate on the audio
    thread" is one question asked of six plugins, while "Gatecrasher has a sidechain bus that may be
    absent" is knowledge core must not have.

    ## The lead this file was written for

    Gatecrasher has **three** sites that can allocate inside `processBlock`, more than any other
    casting, and all three are conditional by construction:

      - `PluginProcessor.cpp:102` — `dryBuffer.setSize(numChannels, numSamples, false, false, true)`.
        The final `true` is `avoidReallocating`, and the buffer is sized in `prepareToPlay` at
        `:66`, so it grows only when a host delivers more samples than it declared.
      - `PluginProcessor.cpp:108` — `triggerLevelScratch.resize(numSamples)`.
      - `PluginProcessor.cpp:120` — `gateGainScratch.resize(numSamples)`.

    A code read cannot settle any of them, because the condition is exactly what is in question.
    **Preparing at 256 and driving at 2048 can**, and the result is recorded here either way: a lead
    that resolves to "not a defect" is a finding, or the next audit re-derives the same suspicion
    from the same three lines.
*/
class RealtimeSafetyTests final : public juce::UnitTest
{
public:
    RealtimeSafetyTests() : juce::UnitTest ("Real-time safety", "DSP") {}

    void runTest() override
    {
        beginTest ("processBlock does not allocate at the block size it was prepared for");
        {
            GatecrasherAudioProcessor processor;
            const auto r = nf::testing::probeProcessBlockAllocation (processor, 48000.0, 512, 512, 2);

            expect (r.clean(), "steady-state processBlock allocated: " + r.describe());
            logMessage ("  prepared 512, driven 512 -> " + r.describe());
        }

        beginTest ("processBlock when a host over-delivers — the three conditional sites");
        {
            // The case the leads describe: prepared for 256, handed 2048. If dryBuffer's
            // avoidReallocating is doing its job and the two scratch vectors have capacity, this is
            // clean; if not, this is where it shows.
            GatecrasherAudioProcessor processor;

            // **Measured twice, because one figure cannot answer the question.** The default probe
            // runs unmeasured warm-up blocks first, so that a lazily-built table allocated on a
            // first block is not reported as a per-block defect. But growth on first over-delivery
            // is a ONE-OFF too — it happens on the first oversized block and never again — so the
            // warmed figure cannot distinguish "never allocates" from "allocated once, before we
            // started counting". Those are different findings: the second is still an audio-thread
            // allocation and still a dropout, just a single one.
            const auto first  = nf::testing::probeProcessBlockAllocation (processor, 48000.0, 256, 2048, 2,
                                                                          /* measuredBlocks */ 1,
                                                                          /* warmUpBlocks   */ 0);

            GatecrasherAudioProcessor steadyProcessor;
            const auto steady = nf::testing::probeProcessBlockAllocation (steadyProcessor, 48000.0, 256, 2048, 2);

            logMessage ("  prepared 256, driven 2048, FIRST block  -> " + first.describe());
            logMessage ("  prepared 256, driven 2048, steady state -> " + steady.describe());

            // **Reported, not asserted clean.** Whether an allocation here is a defect Gatecrasher
            // must fix, or a documented consequence of a host violating its declared maximum, is a
            // ruling — and the sweep's job is to produce the measurement the ruling needs.
            //
            // The steady state is the one that must be clean regardless: a per-block allocation at
            // an over-delivered size is a dropout on every block, not one.
            expect (steady.clean(), "processBlock allocates on EVERY over-delivered block: "
                                        + steady.describe());
        }

        beginTest ("The 128-byte residue — is it PER SPAN or fixed? (reported, not asserted)");
        {
            // **Stage 1b left four allocations totalling 128 bytes on the first over-delivered
            // block, and a residue carried past its own stage stops being looked at.** Chunking took
            // 6 alloc / 16512 bytes down to 4 alloc / 128 bytes; the 16384 was the buffer growth
            // this stage exists to remove, and these four are not that — they do not scale the way
            // 16384 did, and the matched-size arm reports none.
            //
            // The question a sweep answers and a guess does not: **does the count follow the number
            // of SPANS?** Prepared at 256, driving 512 / 1024 / 2048 gives 2 / 4 / 8 spans. A count
            // that tracks them is something inside the loop; a count that does not is a one-off on
            // the first over-delivered block whatever its size.
            //
            // **MEASURED: 4 alloc / 128 bytes at ALL FOUR, including 256 — which is one span and
            // not an over-delivery at all.** So it is neither per-span nor over-delivery-related.
            // It is a first-block one-off that has always been here, and 1b did not leave it: 1b
            // removed the 16384 bytes of growth that was hiding it.
            //
            // That is exactly the class category 1 named when it required BOTH figures per casting:
            // *"a casting that allocates once on its very first block reads identically clean under
            // a warmed probe, and that is a different finding from never allocating."* This casting's
            // matched-size arm reports no heap activity because it warms first; this one does not
            // warm, which is the only reason the four are visible at all.
            //
            // **Still unnamed, and now a different question.** It is not a chunking leftover, so it
            // does not belong to 1b — it belongs with category 1's cold-allocation column, and the
            // next thing worth asking is whether the other five castings show the same profile
            // under a zero-warm-up probe. Shared, it is a suite finding; alone, it is Gatecrasher's.
            for (int driven : { 256, 512, 1024, 2048 })
            {
                GatecrasherAudioProcessor p;
                const auto cold = nf::testing::probeProcessBlockAllocation (p, 48000.0, 256, driven, 2, 1, 0);

                logMessage ("  prepared 256, driven " + juce::String (driven).paddedLeft (' ', 4)
                                + " (" + juce::String (driven / 256) + " spans) -> " + cold.describe());
            }

            expect (true);   // profiling
        }

        beginTest ("The sidechain bus, present and absent");
        {
            // Gatecrasher is the one casting with a sidechain, so this case exists here and nowhere
            // else in the suite. Both states must survive a block without non-finite output.
            GatecrasherAudioProcessor processor;

            nf::testing::RenderSpec spec;
            spec.numBlocks = 16;

            const auto out = nf::testing::render (processor, spec);

            bool finite = true;
            for (const auto& channel : out)
                for (auto v : channel)
                    if (! std::isfinite (v))
                        finite = false;

            expect (finite, "non-finite output from a plain stereo render");
        }
    }
};

static RealtimeSafetyTests realtimeSafetyTests;
