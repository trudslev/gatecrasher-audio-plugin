#include "TestUtils.h"
#include "../Source/Parameters.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace
{
    // Minimal throwaway AudioProcessor just to host an APVTS in tests, avoiding any dependency on
    // the real plugin target's JucePlugin_* macros (which PluginProcessor.cpp requires and which
    // aren't available in the plain console-app Tests target).
    class DummyProcessor final : public juce::AudioProcessor
    {
    public:
        const juce::String getName() const override { return "Dummy"; }
        void prepareToPlay(double, int) override {}
        void releaseResources() override {}
        void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
        double getTailLengthSeconds() const override { return 0.0; }
        bool acceptsMidi() const override { return false; }
        bool producesMidi() const override { return false; }
        juce::AudioProcessorEditor* createEditor() override { return nullptr; }
        bool hasEditor() const override { return false; }
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram(int) override {}
        const juce::String getProgramName(int) override { return {}; }
        void changeProgramName(int, const juce::String&) override {}
        void getStateInformation(juce::MemoryBlock&) override {}
        void setStateInformation(const void*, int) override {}
    };
}

class ParametersStateTests final : public juce::UnitTest
{
public:
    ParametersStateTests() : juce::UnitTest("ParametersState", "DSP") {}

    void runTest() override
    {
        beginTest("Defaults match GATECRASHER-GUI-SPEC.md section 9");
        {
            DummyProcessor proc;
            juce::AudioProcessorValueTreeState apvts(proc, nullptr, "PARAMETERS", createGatecrasherParameterLayout());

            expectWithinAbsoluteError((float) *apvts.getRawParameterValue(ParamIDs::threshold), -18.5f, 0.01f);
            expectWithinAbsoluteError((float) *apvts.getRawParameterValue(ParamIDs::trigHP), 180.0f, 0.01f);
            expectWithinAbsoluteError((float) *apvts.getRawParameterValue(ParamIDs::trigLP), 6300.0f, 0.01f);
            expectEquals((int) *apvts.getRawParameterValue(ParamIDs::keySource), 0); // Internal
            expectWithinAbsoluteError((float) *apvts.getRawParameterValue(ParamIDs::attack), 0.4f, 0.01f);
            expectWithinAbsoluteError((float) *apvts.getRawParameterValue(ParamIDs::hold), 165.0f, 0.01f);
            expectWithinAbsoluteError((float) *apvts.getRawParameterValue(ParamIDs::release), 4.0f, 0.01f);
            expectEquals((int) *apvts.getRawParameterValue(ParamIDs::shape), 0); // Hard
            expectEquals((int) *apvts.getRawParameterValue(ParamIDs::algorithm), 2); // Plate, spec 9.1
            expectWithinAbsoluteError((float) *apvts.getRawParameterValue(ParamIDs::size), 0.72f, 0.01f);
            expectWithinAbsoluteError((float) *apvts.getRawParameterValue(ParamIDs::preDelay), 18.0f, 0.01f);
            expectWithinAbsoluteError((float) *apvts.getRawParameterValue(ParamIDs::decay), 0.6f, 0.01f);
            expectWithinAbsoluteError((float) *apvts.getRawParameterValue(ParamIDs::density), 0.6f, 0.01f);
            expectWithinAbsoluteError((float) *apvts.getRawParameterValue(ParamIDs::dampHF), 0.55f, 0.01f);
            expectWithinAbsoluteError((float) *apvts.getRawParameterValue(ParamIDs::dampLF), 0.35f, 0.01f);
            expectWithinAbsoluteError((float) *apvts.getRawParameterValue(ParamIDs::slam), 7.0f, 0.01f);
            expectWithinAbsoluteError((float) *apvts.getRawParameterValue(ParamIDs::width), 128.0f, 0.01f);
            expectWithinAbsoluteError((float) *apvts.getRawParameterValue(ParamIDs::mix), 64.0f, 0.01f);
            expectWithinAbsoluteError((float) *apvts.getRawParameterValue(ParamIDs::trim), -1.4f, 0.01f);
        }

        beginTest("Full round trip through state XML preserves values, including skewed parameters");
        {
            DummyProcessor procA;
            juce::AudioProcessorValueTreeState apvtsA(procA, nullptr, "PARAMETERS", createGatecrasherParameterLayout());

            *dynamic_cast<juce::AudioParameterFloat*>(apvtsA.getParameter(ParamIDs::threshold)) = -6.0f;
            *dynamic_cast<juce::AudioParameterFloat*>(apvtsA.getParameter(ParamIDs::attack)) = 12.5f;
            *dynamic_cast<juce::AudioParameterChoice*>(apvtsA.getParameter(ParamIDs::algorithm)) = 0; // Ambience
            *dynamic_cast<juce::AudioParameterChoice*>(apvtsA.getParameter(ParamIDs::shape)) = 1; // Soft
            *dynamic_cast<juce::AudioParameterChoice*>(apvtsA.getParameter(ParamIDs::keySource)) = 1; // Sidechain
            *dynamic_cast<juce::AudioParameterFloat*>(apvtsA.getParameter(ParamIDs::slam)) = 11.0f;

            auto state = apvtsA.copyState();
            std::unique_ptr<juce::XmlElement> xml(state.createXml());

            DummyProcessor procB;
            juce::AudioProcessorValueTreeState apvtsB(procB, nullptr, "PARAMETERS", createGatecrasherParameterLayout());
            apvtsB.replaceState(juce::ValueTree::fromXml(*xml));

            expectWithinAbsoluteError((float) *apvtsB.getRawParameterValue(ParamIDs::threshold), -6.0f, 0.01f);
            expectWithinAbsoluteError((float) *apvtsB.getRawParameterValue(ParamIDs::attack), 12.5f, 0.01f);
            expectEquals((int) *apvtsB.getRawParameterValue(ParamIDs::algorithm), 0);
            expectEquals((int) *apvtsB.getRawParameterValue(ParamIDs::shape), 1);
            expectEquals((int) *apvtsB.getRawParameterValue(ParamIDs::keySource), 1);
            expectWithinAbsoluteError((float) *apvtsB.getRawParameterValue(ParamIDs::slam), 11.0f, 0.01f);
        }

        beginTest("Missing parameters in an old saved session fall back to defaults without crashing");
        {
            DummyProcessor procOld;
            juce::AudioProcessorValueTreeState apvtsOld(procOld, nullptr, "PARAMETERS", createGatecrasherParameterLayout());
            *dynamic_cast<juce::AudioParameterFloat*>(apvtsOld.getParameter(ParamIDs::threshold)) = -6.0f;

            auto state = apvtsOld.copyState();
            std::unique_ptr<juce::XmlElement> xml(state.createXml());

            const juce::StringArray idsToStrip{ ParamIDs::shape, ParamIDs::slam, ParamIDs::width };
            for (auto& id : idsToStrip)
                for (int i = xml->getNumChildElements(); --i >= 0;)
                {
                    auto* child = xml->getChildElement(i);
                    if (child->getStringAttribute("id") == id)
                        xml->removeChildElement(child, true);
                }

            DummyProcessor procNew;
            juce::AudioProcessorValueTreeState apvtsNew(procNew, nullptr, "PARAMETERS", createGatecrasherParameterLayout());
            apvtsNew.replaceState(juce::ValueTree::fromXml(*xml));

            expectWithinAbsoluteError((float) *apvtsNew.getRawParameterValue(ParamIDs::threshold), -6.0f, 0.01f);
            expectEquals((int) *apvtsNew.getRawParameterValue(ParamIDs::shape), 0);
            expectWithinAbsoluteError((float) *apvtsNew.getRawParameterValue(ParamIDs::slam), 7.0f, 0.01f);
            expectWithinAbsoluteError((float) *apvtsNew.getRawParameterValue(ParamIDs::width), 128.0f, 0.01f);
        }

        beginTest("A pre-reorder session has its Algorithm remapped to the panel order");
        {
            // v1 stored the choice as Room/Plate/Chamber/Ambience; v2 is the panel's own order
            // (spec 9.1). The index is what gets serialised, so a v1 session naming "Plate" holds a
            // 1, which in v2 means Room - it must come back as 2.
            //
            // Ids and the attribute name are spelled as LITERALS here on purpose: written through
            // the constants this would rename itself alongside the code and assert nothing.
            const auto v1 = R"(<PARAMETERS gatecrasherStateSchemaVersion="1">
  <PARAM id="algorithm" value="1.0"/>
  <PARAM id="mix" value="64.0"/>
</PARAMETERS>)";

            std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(v1));
            expect(xml != nullptr);

            LegacyMigration::remapLegacyAlgorithmIfNeeded(*xml);

            int found = -1;
            for (int i = 0; i < xml->getNumChildElements(); ++i)
                if (xml->getChildElement(i)->getStringAttribute("id") == "algorithm")
                    found = (int) xml->getChildElement(i)->getDoubleAttribute("value");

            expectEquals(found, 2, "v1 Plate (1) must become v2 Plate (2)");
        }

        beginTest("Every v1 Algorithm index maps to the same named tank in v2");
        {
            // old order -> new order, by NAME rather than by index, which is the property that
            // actually matters: a session must sound the same after the reorder.
            const std::array<std::pair<int, int>, 4> cases {{ {0, 1}, {1, 2}, {2, 3}, {3, 0} }};

            for (const auto& [oldIndex, expected] : cases)
            {
                const auto text = juce::String(R"(<PARAMETERS gatecrasherStateSchemaVersion="1">
  <PARAM id="algorithm" value=")") + juce::String(oldIndex) + R"("/>
</PARAMETERS>)";
                std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(text));
                LegacyMigration::remapLegacyAlgorithmIfNeeded(*xml);
                expectEquals((int) xml->getChildElement(0)->getDoubleAttribute("value"), expected);
            }
        }

        beginTest("A current-schema session is left untouched");
        {
            const auto v2 = R"(<PARAMETERS gatecrasherStateSchemaVersion="2">
  <PARAM id="algorithm" value="2.0"/>
</PARAMETERS>)";
            std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(v2));
            LegacyMigration::remapLegacyAlgorithmIfNeeded(*xml);
            expectEquals((int) xml->getChildElement(0)->getDoubleAttribute("value"), 2,
                         "remapping twice would silently rotate the algorithm on every load");
        }
    }
};

static ParametersStateTests parametersStateTests;
