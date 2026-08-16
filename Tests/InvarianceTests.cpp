#include "../Source/PluginProcessor.h"
#include "../Source/Parameters.h"

#include <nf/testing/ProcessorHarness.h>

#include <juce_audio_processors/juce_audio_processors.h>

/**
    Category 3 — invariance. Does the same audio come out when only the CONTAINER changes?

    ## Why this file leads with premise checks rather than results

    An invariance failure looks like a pass more readily than anything else in this sweep, and it
    also looks like a failure more readily. Reflect-84 produced both in one run: four block-size
    rows all reporting DIFFERS, and every one of them measuring a first-run-only state rather than
    block dependence — because `blockSizeInvariance` compares each size against the FIRST size, so
    its first row is 64 against 64, and that self-comparison differed too.

    So nothing here is believed until the processor is shown to be reproducible against itself, and
    the comparison is shown able to fail. Both are asserted below rather than assumed.
*/
class InvarianceTests final : public juce::UnitTest
{
public:
    InvarianceTests() : juce::UnitTest ("Invariance", "DSP") {}

    void runTest() override
    {
        beginTest ("PREMISE CHECK — reproducible against itself, cold and warmed");
        {
            // Three renders, no parameter writes. A vs B and C vs D separate the two shapes:
            //
            //   A != B, C == D   ->  FIRST-RUN-ONLY state: something is in its constructed
            //                        condition for the first render and its steady one after.
            //   A != B, C != D   ->  ONGOING carry across prepareToPlay.
            //   both exact       ->  reproducible; every result below means what it claims.
            //
            // Reflect-84 came back first-run-only, and the cause was a smoother that never got a
            // setCurrentAndTargetValue — its pre-delay glided up from zero on the first run only.
            GatecrasherAudioProcessor processor;

            nf::testing::RenderSpec spec;
            spec.blockSize = 512;
            spec.numBlocks = 16;

            const auto ab = nf::testing::compareRenders (nf::testing::render (processor, spec),
                                                         nf::testing::render (processor, spec));
            const auto cd = nf::testing::compareRenders (nf::testing::render (processor, spec),
                                                         nf::testing::render (processor, spec));

            logMessage ("  cold   A vs B -> " + ab.describe());
            logMessage ("  warmed C vs D -> " + cd.describe());
            logMessage (juce::String ("  => ") + (ab.sampleExact ? "reproducible from construction"
                                                : cd.sampleExact ? "FIRST-RUN-ONLY state — see the note below"
                                                                 : "ONGOING carry across prepareToPlay"));

            // The warmed comparison is what every driver below depends on. A cold difference is a
            // finding in its own right and is reported rather than asserted, because the drivers
            // warm before measuring; a warmed difference means no invariance result is readable.
            expect (cd.sampleExact,
                    "this processor is not reproducible even warmed, so NO invariance result below "
                    "means anything: " + cd.describe());

            if (! ab.sampleExact)
                logMessage ("  NOTE: a first-run-only difference is itself a finding — an instance's "
                            "first playback differs from every later one. Reported, not asserted.");
        }

        beginTest ("BISECT — what moves Gatecrasher's first-run divergence off sample 2020?");
        {
            // This casting is the one the suite-wide smoother grep explains NOTHING about: zero
            // unguarded SmoothedValue::reset sites, and a first-run divergence that is not at
            // sample 0. ~2020 samples is ~42 ms at 48 kHz — state that takes time to express rather
            // than an initial condition read straight away.
            //
            // ## Candidates, named before measuring
            //
            //   1. The gate envelope reaching a threshold.
            //   2. The trigger detector's filter histories, which reach the output only once they
            //      move the envelope past that threshold.
            //   3. A tank's first wrap, at whichever line is ~42 ms.
            //   4. The switch crossfade — RULED OUT BY SHAPE: Reflect-84's equivalent fires on the
            //      first block, so it would show at sample 0. Recorded so it is not re-derived.
            //
            // ## What this instrument is, and what is known about it
            //
            // The metric is `firstDivergentSample`, which is not new: it produced correct verdicts
            // across five castings in this category, including two defects reproduced by other
            // means. What is new is the DRIVE DESIGN, and the known case for that is this casting's
            // own baseline — cold against warmed is known to differ, and warmed against warmed is
            // known to be exact, so the comparison is known able to both fail and pass before a
            // single row below is read.
            //
            // If the divergence point moves with a parameter, that parameter's path owns it. If it
            // sits at 2020 through every drive, none of the four candidates is right and the
            // divergence is anchored to something none of these touch.
            nf::testing::RenderSpec spec;
            spec.blockSize = 512;
            spec.numBlocks = 16;

            const auto divergenceAt = [&spec] (const char* id, float normalised)
            {
                GatecrasherAudioProcessor cold, warmRef;

                for (auto* p : { &cold, &warmRef })
                    if (id != nullptr)
                        if (auto* param = p->apvts.getParameter (id))
                            param->setValueNotifyingHost (normalised);

                nf::testing::RenderSpec warmSpec;
                warmSpec.blockSize = 512;
                warmSpec.numBlocks = 4;
                nf::testing::render (warmRef, warmSpec);

                return nf::testing::compareRenders (nf::testing::render (cold, spec),
                                                    nf::testing::render (warmRef, spec));
            };

            const auto baseline = divergenceAt (nullptr, 0.0f);
            logMessage ("  baseline (defaults)          -> " + baseline.describe());

            struct Drive { const char* id; const char* why; };
            const Drive drives[] = {
                { ParamIDs::threshold, "candidate 1/2 — the gate's own threshold" },
                { ParamIDs::attack,    "candidate 1 — how fast the envelope gets there" },
                { ParamIDs::trigHP,    "candidate 2 — the detector's filter" },
                { ParamIDs::size,      "candidate 3 — the tank's line lengths" },
                { ParamIDs::preDelay,  "candidate 3 — when the tank is first fed" },
                { ParamIDs::decay,     "candidate 3 — how long the tank holds" },
            };

            for (const auto& d : drives)
                for (float v : { 0.15f, 0.85f })
                {
                    const auto r = divergenceAt (d.id, v);
                    logMessage ("  " + (juce::String (d.id) + " = " + juce::String (v, 2)).paddedRight (' ', 28)
                                    + " -> first at " + juce::String (r.firstDivergentSample)
                                    + ", max |delta| " + juce::String (r.maxAbsDifference, 9)
                                    + "   (" + d.why + ")");
                }


            // **Candidate 4 was ruled out by a shape argument and the argument was WRONG.** It said
            // a switch crossfade "would show at sample 0, so it is not this". But Reflect-84's
            // crossfade showed at sample 351 — its WET ARRIVAL — because a difference introduced
            // inside the tank reaches the output when the wet path does, exactly like every other
            // candidate here. The drives above establish that this divergence sits at wet arrival,
            // which does not exclude the crossfade; it is precisely what the crossfade would do.
            //
            // Two readings of this casting's source have already produced wrong answers in this
            // bisect — DampingStage's resize() (which does call reset(), so it is clean) and this
            // shape argument — so this arm measures rather than reads. ReverbEngine constructs
            // currentAlgorithm = plate, and the parameter's default index is 2, which the
            // StringArray also maps to plate. If those agree no crossfade fires on the first block,
            // and driving the algorithm to each of the four says so directly.
            // **The baseline diverges exactly like index 1 while the parameter declares index 2.**
            // Nine identical digits is not a coincidence, so either the default is not what the
            // layout says or writing a parameter changes something beyond its value. Logged rather
            // than reasoned about.
            {
                GatecrasherAudioProcessor fresh;
                auto* algo = fresh.apvts.getParameter (ParamIDs::algorithm);
                logMessage ("  default algorithm reads -> \"" + algo->getCurrentValueAsText()
                                + "\", normalised " + juce::String (algo->getValue(), 4));

                algo->setValueNotifyingHost (0.67f);
                logMessage ("  after writing 0.67      -> \"" + algo->getCurrentValueAsText()
                                + "\", normalised " + juce::String (algo->getValue(), 4));
            }

            logMessage ("  --- the four algorithms, and the crossfade candidate ---");

            for (float v : { 0.0f, 0.34f, 0.67f, 1.0f })
            {
                const auto r = divergenceAt (ParamIDs::algorithm, v);
                logMessage ("  algorithm = " + juce::String (v, 2) + " -> first at "
                                + juce::String (r.firstDivergentSample) + ", max |delta| "
                                + juce::String (r.maxAbsDifference, 9));
            }

            // Neutralising drives: each removes one stage from the wet path's contribution.
            for (auto* id : { ParamIDs::mix, ParamIDs::slam, ParamIDs::dampHF, ParamIDs::dampLF })
                for (float v : { 0.0f, 1.0f })
                {
                    const auto r = divergenceAt (id, v);
                    logMessage ("  " + (juce::String (id) + " = " + juce::String (v, 2)).paddedRight (' ', 28)
                                    + " -> first at " + juce::String (r.firstDivergentSample)
                                    + ", max |delta| " + juce::String (r.maxAbsDifference, 9));
                }

            expect (! baseline.sampleExact,
                    "the baseline came back exact, so every row above compared two identical things "
                    "and the bisect measured nothing");
        }

        beginTest ("Block size — sample-exact at 64 / 128 / 511 / 2048");
        {
            GatecrasherAudioProcessor processor;
            warm (processor);

            nf::testing::RenderSpec spec;
            spec.blockSize = 512;
            spec.numBlocks = 64;

            const auto results = nf::testing::blockSizeInvariance (processor, spec,
                                                                   { 64, 128, 511, 2048 });

            for (const auto& r : results)
                logMessage ("  " + r.describe());

            // 511 is prime and shares no factor with the others, so it catches any assumption that
            // a block divides evenly into an internal chunk — the failure a 64/128/2048 sweep walks
            // past because all three share factors.
            //
            // The first row is the size compared against itself. It passing is what makes the other
            // three readable; it failing means the run measured non-determinism.
            expect (! results.empty() && results.front().sampleExact,
                    "the self-comparison failed, so the other rows measured non-determinism rather "
                    "than block dependence");

            for (const auto& r : results)
                expect (r.sampleExact,
                        "block-size invariance failed — the same sample stream cut differently "
                        "produced different output: " + r.describe());
        }

        beginTest ("Reproducible across reset() ALONE — the structurally-absent case, ASSERTED");
        {
            /*  **A path nothing in this suite could reach until `nf::testing::renderBlocks` existed.**
                `render` calls `prepareToPlay` on every invocation, so every premise check anywhere is
                a *prepare* check by construction, and *prepare once → render → `reset()` → render*
                could not be expressed at all. A host asks it on every transport locate.

                **This casting has NO generator, which is why its row can be asserted while four
                others wait on a ruling.** The open question — whether a `reset()` owes a rewound
                generator or only a cleared tail — cannot arise here, so what this arm measures is the
                narrower and unambiguous half: does `reset()` return the processor to the same state
                at all. That is a property nobody disputes, and Gatecrasher's tail is the one this
                suite measured at 0.679 before stage 1c cleared it.

                **Structurally absent is not the same claim as measured clean**, and this row is now
                the second: no generator to leave running, AND `reset()` shown to reach a fixed point
                with the reverb driven. Before this arm, only the first was true.

                Driven rather than left at defaults, for the reason every reset row in this sweep has
                had to be: a gate that never opens has no tail to leave behind, and would report clean
                whatever `reset()` did. */
            GatecrasherAudioProcessor processor;

            const auto setP = [&processor] (const juce::String& id, float value)
            {
                if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (processor.apvts.getParameter (id)))
                    p->setValueNotifyingHost (p->getNormalisableRange().convertTo0to1 (value));
            };

            setP (ParamIDs::threshold, -60.0f);   // open, so there is signal to build a tail from
            setP (ParamIDs::decay, 6.0f);
            setP (ParamIDs::mix, 100.0f);

            nf::testing::RenderSpec spec;
            spec.blockSize = 512;
            spec.numBlocks = 16;

            const auto r = nf::testing::reproducibleAcrossReset (processor, spec);
            logMessage ("  " + r.describe());

            expect (r.premiseHeld(),
                    "this processor is not reproducible across prepare, so its reset row means "
                    "nothing: " + r.acrossPrepare.describe());

            expect (r.acrossReset.sampleExact,
                    "reset() did not return this processor to the same state, with the reverb driven "
                    "— and this casting has no generator, so the open seeding ruling cannot explain "
                    "it. Something else survives reset: " + r.acrossReset.describe());
        }

        beginTest ("Offline against real-time");
        {
            GatecrasherAudioProcessor processor;
            warm (processor);

            const auto r = nf::testing::offlineAgainstRealtime (processor, {});

            logMessage ("  " + r.describe());

            // **Confirm setNonRealtime changed something observable**, or a passing comparison is
            // only evidence that the flag was ignored.
            if (! r.nonRealtimeWasHonoured)
                logMessage ("  NOTE: setNonRealtime changed nothing this processor reports, so this "
                            "row is 'no offline path exists' rather than 'the offline path agrees'.");

            expect (r.sampleExact || ! r.comparisonWasMeaningful,
                    "offline differs from real-time. Not a defect on its face — this casting would "
                    "have to intend it: " + r.describe());
        }
    }

private:
    /** One discarded render, so any first-run-only state is spent before a driver measures. */
    static void warm (GatecrasherAudioProcessor& p)
    {
        nf::testing::RenderSpec spec;
        spec.blockSize = 512;
        spec.numBlocks = 4;
        nf::testing::render (p, spec);
    }
};

static InvarianceTests invarianceTests;
