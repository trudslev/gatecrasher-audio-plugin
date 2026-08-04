#include "TestUtils.h"
#include "../Source/DSP/GateEnvelopeGenerator.h"
#include <vector>

namespace
{
    std::vector<float> constantTrigger(int numSamples, float level)
    {
        return std::vector<float>((size_t) numSamples, level);
    }
}

class GateEnvelopeGeneratorTests final : public juce::UnitTest
{
public:
    GateEnvelopeGeneratorTests() : juce::UnitTest("GateEnvelopeGenerator", "DSP") {}

    void runTest() override
    {
        const double sampleRate = 48000.0;

        beginTest("isGateOpen() flips true on the exact sample the trigger crosses threshold, not before");
        {
            GateEnvelopeGenerator gate;
            gate.prepare(sampleRate);

            const float thresholdDb = -18.0f;
            const float thresholdLinear = juce::Decibels::decibelsToGain(thresholdDb);

            std::vector<float> trigger(11, thresholdLinear * 0.5f);
            trigger[10] = thresholdLinear * 1.5f;
            std::vector<float> gain(11);

            for (int i = 0; i < 10; ++i)
            {
                gate.process(&trigger[(size_t) i], 1, thresholdDb, 0.5f, 165.0f, 4.0f, GateShape::hard, &gain[(size_t) i]);
                expect(! gate.isGateOpen(), "Gate must stay closed while the trigger is below threshold");
            }

            gate.process(&trigger[10], 1, thresholdDb, 0.5f, 165.0f, 4.0f, GateShape::hard, &gain[10]);
            expect(gate.isGateOpen(), "Gate must open on the exact sample the trigger crosses threshold");
        }

        beginTest("Trigger permanently below threshold: gate never opens");
        {
            GateEnvelopeGenerator gate;
            gate.prepare(sampleRate);

            const int numSamples = 4800;
            auto trigger = constantTrigger(numSamples, 0.0001f);
            std::vector<float> gain((size_t) numSamples);

            gate.process(trigger.data(), numSamples, -18.0f, 0.4f, 165.0f, 4.0f, GateShape::hard, gain.data());

            expect(! gate.isGateOpen(), "Gate should never open when the trigger never crosses threshold");
            for (float g : gain)
                expectWithinAbsoluteError(g, 0.0f, 1.0e-6f);
        }

        beginTest("A continuously-held trigger keeps the gate fully open past nominal Hold");
        {
            GateEnvelopeGenerator gate;
            gate.prepare(sampleRate);

            const int numSamples = 20000; // generous - well past attack + hold at these settings
            auto trigger = constantTrigger(numSamples, 1.0f);
            std::vector<float> gain((size_t) numSamples);

            gate.process(trigger.data(), numSamples, -18.0f, 0.4f, 165.0f, 4.0f, GateShape::hard, gain.data());

            expect(gate.isGateOpen(), "Gate should be open while the trigger stays above threshold");
            float maxGain = 0.0f;
            for (float g : gain)
                maxGain = juce::jmax(maxGain, g);
            expectWithinAbsoluteError(maxGain, 1.0f, 1.0e-3f);

            // Retriggered every sample, so Hold's counter never elapses - the gate must still be
            // fully open at the very last sample, never having reached Release.
            expectWithinAbsoluteError(gain.back(), 1.0f, 1.0e-3f);
        }

        beginTest("Gate closes and isGateOpen() flips false once Hold + Release elapse with no further trigger");
        {
            GateEnvelopeGenerator gate;
            gate.prepare(sampleRate);

            const int triggerSamples = 500;
            const int silenceSamples = 20000;

            auto trigger = constantTrigger(triggerSamples, 1.0f);
            std::vector<float> gain1((size_t) triggerSamples);
            gate.process(trigger.data(), triggerSamples, -18.0f, 0.5f, 5.0f, 4.0f, GateShape::hard, gain1.data());

            auto silence = constantTrigger(silenceSamples, 0.0f);
            std::vector<float> gain2((size_t) silenceSamples);
            gate.process(silence.data(), silenceSamples, -18.0f, 0.5f, 5.0f, 4.0f, GateShape::hard, gain2.data());

            expect(! gate.isGateOpen(), "Gate should have fully closed well after Hold + Release elapse with no trigger");
            expectWithinAbsoluteError(gain2.back(), 0.0f, 1.0e-3f);
        }

        beginTest("Soft shape is still audibly open later into Release than Hard shape, at the same Release setting");
        {
            GateEnvelopeGenerator hardGate;
            GateEnvelopeGenerator softGate;
            hardGate.prepare(sampleRate);
            softGate.prepare(sampleRate);

            // Hold is deliberately short (1ms = 48 samples @48kHz) so the 300-sample silence block
            // below is guaranteed to fully elapse Hold and land partway into Release for both
            // shapes - a longer Hold (as an earlier version of this test used) can leave both gates
            // still sitting in Hold at gain=1.0 for the entire silence block, which trivially "ties"
            // instead of exercising Release at all.
            const float holdMs = 1.0f;
            const float releaseMs = 20.0f;

            const int triggerSamples = 100; // enough to complete Attack and settle into Hold
            auto trigger = constantTrigger(triggerSamples, 1.0f);
            std::vector<float> hardGain1((size_t) triggerSamples), softGain1((size_t) triggerSamples);
            hardGate.process(trigger.data(), triggerSamples, -18.0f, 0.1f, holdMs, releaseMs, GateShape::hard, hardGain1.data());
            softGate.process(trigger.data(), triggerSamples, -18.0f, 0.1f, holdMs, releaseMs, GateShape::soft, softGain1.data());

            const int silenceSamples = 300;
            auto silence = constantTrigger(silenceSamples, 0.0f);
            std::vector<float> hardGain2((size_t) silenceSamples), softGain2((size_t) silenceSamples);
            hardGate.process(silence.data(), silenceSamples, -18.0f, 0.1f, holdMs, releaseMs, GateShape::hard, hardGain2.data());
            softGate.process(silence.data(), silenceSamples, -18.0f, 0.1f, holdMs, releaseMs, GateShape::soft, softGain2.data());

            expect(softGain2.back() > hardGain2.back(),
                   "At the same point in time well into Release, Soft should still be audibly higher than Hard");
        }

        beginTest("Output gain is always finite and within [0, 1] under random trigger input");
        {
            GateEnvelopeGenerator gate;
            gate.prepare(sampleRate);

            juce::Random random(99);
            const int numSamples = 10000;
            std::vector<float> trigger((size_t) numSamples);
            for (auto& v : trigger)
                v = random.nextFloat();
            std::vector<float> gain((size_t) numSamples);

            gate.process(trigger.data(), numSamples, -24.0f, 1.0f, 50.0f, 10.0f, GateShape::soft, gain.data());

            for (float g : gain)
            {
                expect(std::isfinite(g), "Gain must stay finite");
                expect(g >= 0.0f && g <= 1.0f, "Gain must stay within [0, 1]");
            }
        }
    }
};

static GateEnvelopeGeneratorTests gateEnvelopeGeneratorTests;
