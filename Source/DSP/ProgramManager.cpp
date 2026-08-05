#include "ProgramManager.h"
#include "../Parameters.h"
#include <algorithm>

namespace
{
    // Deliberately not JucePlugin_Manufacturer/JucePlugin_Name: those macros only exist in the
    // real plugin target's JUCE-generated headers, and ProgramManager.cpp is also compiled
    // directly into the Tests console-app target (see Tests/CMakeLists.txt) so
    // ProgramManagerTests.cpp can exercise it without linking the whole plugin - unlike TapeRot,
    // whose equivalent preset logic lives inline on PluginProcessor and is never compiled into its
    // Tests target. Matches CMakeLists.txt's COMPANY_NAME/PRODUCT_NAME - keep in sync if those change.
    constexpr const char* pluginCompanyName = "Tanis";
    constexpr const char* pluginProductName = "Gatecrasher";
}

ProgramManager::ProgramManager(juce::AudioProcessorValueTreeState& stateToControl)
    : apvts(stateToControl)
{
}

ProgramManager::~ProgramManager()
{
    cancelPendingUpdate();
}

void ProgramManager::initialise()
{
    refreshUserProgramList();
    applyFactoryProgram(kFactoryPrograms[(size_t) defaultFactoryProgramIndex]);
    currentProgramIndex.store(defaultFactoryProgramIndex, std::memory_order_relaxed);
    captureCleanSnapshot();
}

void ProgramManager::captureCleanSnapshot()
{
    const auto& params = apvts.processor.getParameters();
    cleanSnapshot.clear();
    cleanSnapshot.reserve((size_t) params.size());
    for (const auto* param : params)
        cleanSnapshot.push_back(param->getValue());
}

bool ProgramManager::isModifiedFromLoadedProgram() const
{
    const auto& params = apvts.processor.getParameters();
    if (cleanSnapshot.size() != (size_t) params.size())
        return false;

    // Compared in normalised 0..1 space, so one epsilon is meaningful for every parameter
    // regardless of its real-world range. Loose enough to absorb the float round-trip through a
    // user program's XML (which is why a just-loaded user program doesn't read as modified),
    // far tighter than the smallest movement any control can actually produce.
    for (int i = 0; i < params.size(); ++i)
        if (std::abs(params[i]->getValue() - cleanSnapshot[(size_t) i]) > 1.0e-4f)
            return true;

    return false;
}

int ProgramManager::getNumPrograms() const noexcept
{
    return kNumFactoryPrograms + userProgramFiles.size();
}

void ProgramManager::requestProgramChange(int index)
{
    if (index < 0 || index >= getNumPrograms())
        return;
    pendingProgramIndex.store(index, std::memory_order_relaxed);
    triggerAsyncUpdate();
}

void ProgramManager::handleAsyncUpdate()
{
    const int index = pendingProgramIndex.exchange(-1, std::memory_order_relaxed);
    if (index >= 0)
        applyProgramByIndex(index);
}

juce::String ProgramManager::getProgramName(int index) const
{
    if (isFactoryProgram(index))
        return kFactoryPrograms[(size_t) index].name;

    const int userIndex = index - kNumFactoryPrograms;
    if (userIndex >= 0 && userIndex < userProgramFiles.size())
        return userProgramFiles.getReference(userIndex).getFileNameWithoutExtension();
    return {};
}

juce::File ProgramManager::getUserProgramDirectory()
{
   #if JUCE_WINDOWS || JUCE_LINUX
    // Windows: %APPDATA%\<Manufacturer>\<Plugin>\Programs. Linux: ~/.config/<Manufacturer>/<Plugin>/Programs
    // (JUCE's userApplicationDataDirectory resolves to the right per-OS location on each).
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile(pluginCompanyName)
        .getChildFile(pluginProductName)
        .getChildFile("Programs");
   #else
    // "Presets" here is Apple/AU's own special-location folder name that Logic and other hosts
    // scan (~/Library/Audio/Presets/<Manufacturer>/<Plugin>/) - not a lapse into TapeRot's
    // "Preset" terminology, just the OS convention this path has to match to be discoverable.
    return juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile("Library")
        .getChildFile("Audio")
        .getChildFile("Presets")
        .getChildFile(pluginCompanyName)
        .getChildFile(pluginProductName);
   #endif
}

void ProgramManager::refreshUserProgramList()
{
    userProgramFiles.clear();
    const auto dir = getUserProgramDirectory();
    if (! dir.isDirectory())
        return;

    for (const auto& entry : juce::RangedDirectoryIterator(dir, false, "*.gatecrasherprogram"))
        userProgramFiles.add(entry.getFile());

    std::sort(userProgramFiles.begin(), userProgramFiles.end(),
               [] (const juce::File& a, const juce::File& b) { return a.getFileName() < b.getFileName(); });
}

void ProgramManager::applyProgramByIndex(int index)
{
    if (isFactoryProgram(index))
    {
        applyFactoryProgram(kFactoryPrograms[(size_t) index]);
    }
    else
    {
        const int userIndex = index - kNumFactoryPrograms;
        if (userIndex < 0 || userIndex >= userProgramFiles.size())
            return;

        std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(userProgramFiles.getReference(userIndex)));
        if (xml == nullptr || ! xml->hasTagName(apvts.state.getType()))
            return;

        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }

    currentProgramIndex.store(index, std::memory_order_relaxed);
    captureCleanSnapshot();
    if (onProgramListChanged)
        onProgramListChanged();
}

void ProgramManager::applyFactoryProgram(const FactoryProgram& program)
{
    *dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(ParamIDs::algorithm)) = program.algorithm;
    *dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::size)) = program.size;
    *dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::preDelay)) = program.preDelayMs;
    *dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::decay)) = program.decay;
    *dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::density)) = program.density;
    *dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::dampHF)) = program.dampHF;
    *dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::dampLF)) = program.dampLF;
    *dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::threshold)) = program.thresholdDb;
    *dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::attack)) = program.attackMs;
    *dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::hold)) = program.holdMs;
    *dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::release)) = program.releaseMs;
    *dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(ParamIDs::shape)) = program.shape;
    *dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(ParamIDs::keySource)) = program.keySource;
    *dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::trigHP)) = program.trigHPHz;
    *dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::trigLP)) = program.trigLPHz;
    *dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::slam)) = program.slamDb;
    *dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::width)) = program.widthPercent;
    *dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::mix)) = program.mixPercent;
    *dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(ParamIDs::trim)) = program.trimDb;
}

void ProgramManager::saveNewUserProgram(const juce::String& requestedName)
{
    juce::String name = requestedName.trim().toUpperCase();
    if (name.isEmpty())
        name = "NEW PROGRAM";
    if (name.length() > maxProgramNameLength)
        name = name.substring(0, maxProgramNameLength);

    const auto dir = getUserProgramDirectory();
    if (! dir.isDirectory())
        dir.createDirectory();

    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    xml->setAttribute(LegacyMigration::stateSchemaVersionAttribute, LegacyMigration::currentStateSchemaVersion);

    const juce::File file = dir.getChildFile(juce::File::createLegalFileName(name) + ".gatecrasherprogram");
    xml->writeTo(file);

    refreshUserProgramList();
    const int newIndex = kNumFactoryPrograms + userProgramFiles.indexOf(file);
    currentProgramIndex.store(newIndex, std::memory_order_relaxed);
    // The just-saved program IS the current parameter state, so this becomes the new clean
    // baseline - SAVE goes back to disabled immediately after storing, until something moves again.
    captureCleanSnapshot();
    if (onProgramListChanged)
        onProgramListChanged();
}

void ProgramManager::deleteUserProgram(int index)
{
    if (isFactoryProgram(index))
        return;

    const int userIndex = index - kNumFactoryPrograms;
    if (userIndex < 0 || userIndex >= userProgramFiles.size())
        return;

    const bool wasCurrent = currentProgramIndex.load(std::memory_order_relaxed) == index;
    userProgramFiles.getReference(userIndex).deleteFile();
    refreshUserProgramList();

    if (wasCurrent)
        requestProgramChange(defaultFactoryProgramIndex);
    else if (onProgramListChanged)
        onProgramListChanged();
}

void ProgramManager::setCurrentProgramIndexWithoutApplying(int index) noexcept
{
    currentProgramIndex.store(index, std::memory_order_relaxed);

    // Treats the restored session as its own clean baseline rather than diffing it against the
    // remembered program's stored values. Those values aren't loaded here by design (the caller
    // restores parameters from the session itself), and re-reading them - parsing a user program
    // file back off disk - to answer "were there unsaved edits when this session was saved?" isn't
    // worth it for a button's enablement: the consequence of the simplification is only that SAVE
    // starts out disabled after reopening a session that had unsaved edits, and it enables again
    // the moment any control moves.
    captureCleanSnapshot();
}

void ProgramManager::cancelPendingChange() noexcept
{
    pendingProgramIndex.store(-1, std::memory_order_relaxed);
    cancelPendingUpdate();
}
