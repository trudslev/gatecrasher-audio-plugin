#include "TestUtils.h"
#include "../Source/DSP/FactoryPrograms.h"

// Structural sanity only (mirrors TapeRot's FactoryPresetsTests.cpp) - every entry must reference a
// valid choice index and stay within each parameter's declared range. Tonal/character correctness
// is a by-ear pass, not something this suite checks (see BUILDING.md).
class FactoryProgramsTests final : public juce::UnitTest
{
public:
    FactoryProgramsTests() : juce::UnitTest("FactoryPrograms", "DSP") {}

    void runTest() override
    {
        beginTest("There are exactly 17 factory programs");
        {
            expectEquals(kNumFactoryPrograms, 17);
        }

        beginTest("Every program's choice fields are valid indices");
        {
            for (const auto& p : kFactoryPrograms)
            {
                expect(p.algorithm >= 0 && p.algorithm <= 3, p.name);
                expect(p.shape == 0 || p.shape == 1, p.name);
                expect(p.keySource == 0 || p.keySource == 1, p.name);
            }
        }

        beginTest("Every program's normalised 0-1 fields are within range");
        {
            for (const auto& p : kFactoryPrograms)
            {
                expect(p.size >= 0.0f && p.size <= 1.0f, p.name);
                expect(p.decay >= 0.0f && p.decay <= 1.0f, p.name);
                expect(p.density >= 0.0f && p.density <= 1.0f, p.name);
                expect(p.dampHF >= 0.0f && p.dampHF <= 1.0f, p.name);
                expect(p.dampLF >= 0.0f && p.dampLF <= 1.0f, p.name);
            }
        }

        beginTest("Every program's ranged fields match Parameters.h's declared ranges");
        {
            for (const auto& p : kFactoryPrograms)
            {
                expect(p.thresholdDb >= -60.0f && p.thresholdDb <= 0.0f, p.name);
                expect(p.trigHPHz >= 20.0f && p.trigHPHz <= 2000.0f, p.name);
                expect(p.trigLPHz >= 500.0f && p.trigLPHz <= 20000.0f, p.name);
                expect(p.attackMs >= 0.1f && p.attackMs <= 20.0f, p.name);
                expect(p.holdMs >= 10.0f && p.holdMs <= 500.0f, p.name);
                expect(p.releaseMs >= 1.0f && p.releaseMs <= 200.0f, p.name);
                expect(p.preDelayMs >= 0.0f && p.preDelayMs <= 120.0f, p.name);
                expect(p.slamDb >= 0.0f && p.slamDb <= 12.0f, p.name);
                expect(p.widthPercent >= 0.0f && p.widthPercent <= 200.0f, p.name);
                expect(p.mixPercent >= 0.0f && p.mixPercent <= 100.0f, p.name);
                expect(p.trimDb >= -24.0f && p.trimDb <= 12.0f, p.name);
            }
        }

        beginTest("Program names are unique");
        {
            juce::StringArray names;
            for (const auto& p : kFactoryPrograms)
                names.add(p.name);
            names.sort(false);
            for (int i = 1; i < names.size(); ++i)
                expect(names[i] != names[i - 1], names[i]);
        }

        beginTest("defaultFactoryProgramIndex names \"Air Tomorrow\", and it is Program 01");
        {
            expectEquals(juce::String(kFactoryPrograms[(size_t) defaultFactoryProgramIndex].name),
                         juce::String("Air Tomorrow"));

            // The default has to be FIRST, not merely present. With it second, a fresh instance
            // opened reading "02" with nothing having selected it, and the obvious "back to the
            // start" gesture landed on a different sound.
            expectEquals(defaultFactoryProgramIndex, 0);
        }
    }
};

static FactoryProgramsTests factoryProgramsTests;
