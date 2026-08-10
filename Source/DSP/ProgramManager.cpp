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
    // Tests target.
    //
    // These previously carried literals marked "keep in sync with CMakeLists.txt". They didn't stay
    // in sync - COMPANY_NAME became "Neon Foundry" while this file still said "Tanis", so saved
    // Programs were being written to (and looked for in) the old directory with nothing to signal
    // it. Both targets are now handed the same values by CMake; a missing definition is a hard
    // error rather than a silent fallback, because a silent fallback is precisely the failure.
#if !defined(NF_COMPANY_NAME) || !defined(NF_PRODUCT_NAME)
 #error "NF_COMPANY_NAME and NF_PRODUCT_NAME must be defined by CMake - see CMakeLists.txt."
#endif
    constexpr const char* pluginCompanyName = NF_COMPANY_NAME;
    constexpr const char* pluginProductName = NF_PRODUCT_NAME;
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
    // **Application data on every platform - no macOS special case.** This used to branch, putting
    // macOS Programs under ~/Library/Audio/Presets. That is Apple's location for the AU PRESET
    // FORMAT: .aupreset files the AU system itself scans, reads and writes. Our user Programs are
    // not those - they are application-owned data in our own XML format - so they belong where an
    // application keeps its data, and the AU folder should hold only what AU understands.
    //
    // Discoverability was the old justification, and it does not survive scrutiny: a host scanning
    // that folder is looking for .aupreset, and would not have loaded a .gatecrasherprogram from it.
    //
    // The "Application Support" segment is JUCE's and must never be hard-coded:
    // userApplicationDataDirectory resolves to ~/Library/Application Support on macOS, %APPDATA% on
    // Windows and ~/.config on Linux. A shared literal would be wrong on two of the three.
    //
    // No migration from the old location - nothing has shipped, so nothing is there to migrate.
    // See Elmer's ProgramManager for why that is a decision rather than an oversight.
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile(pluginCompanyName)
        .getChildFile(pluginProductName)
        .getChildFile("Programs");
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

        // User Programs are a second serialisation path alongside the host session, and they carry
        // the same schema attribute - so they need the same migration. A Program saved before the
        // Algorithm reorder would otherwise load the old index meaning and select a different tank.
        LegacyMigration::remapLegacyAlgorithmIfNeeded(*xml);

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
