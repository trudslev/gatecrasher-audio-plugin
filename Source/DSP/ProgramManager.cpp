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

    // Named once. It was a literal in two places, which is two chances to rename one and not the
    // other - and a Program written under one spelling is invisible to a scan using the other.
    constexpr const char* programFileExtension = ".gatecrasherprogram";
}

ProgramManager::ProgramManager(juce::AudioProcessorValueTreeState& stateToControl)
    : apvts(stateToControl)
{
    // The identity has to be valid before initialise() runs, exactly as the atomic index it
    // replaced was valid from its in-class initialiser. A host may call getCurrentProgram or
    // getProgramName the moment the processor exists.
    setCurrentId(factoryIdAt(defaultFactoryProgramIndex));
}

ProgramManager::~ProgramManager()
{
    cancelPendingUpdate();
}

void ProgramManager::initialise()
{
    refreshUserProgramList();
    applyFactoryProgram(kFactoryPrograms[(size_t) defaultFactoryProgramIndex]);
    setCurrentId(factoryIdAt(defaultFactoryProgramIndex));
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

//==============================================================================
// Identity. Nothing below addresses a Program by position except the deliberate crossings above.

ProgramId ProgramManager::factoryIdAt(int factoryPosition)
{
    const auto& p = kFactoryPrograms[(size_t) factoryPosition];
    return { ProgramBank::factory, p.slug, p.name };
}

ProgramId ProgramManager::initId()
{
    return { ProgramBank::init, kInitProgram.slug, kInitProgram.name };
}

int ProgramManager::factoryPositionOf(const juce::String& slug)
{
    for (size_t i = 0; i < kFactoryPrograms.size(); ++i)
        if (slug == kFactoryPrograms[i].slug)
            return (int) i;

    return -1;
}

ProgramId ProgramManager::getCurrentProgramId() const
{
    const juce::SpinLock::ScopedLockType lock(currentIdLock);
    return currentId;
}

void ProgramManager::setCurrentId(const ProgramId& id)
{
    const juce::SpinLock::ScopedLockType lock(currentIdLock);
    currentId = id;
}

int ProgramManager::getCurrentFactoryPosition() const
{
    const auto id = getCurrentProgramId();

    if (id.bank == ProgramBank::factory)
        if (const int pos = factoryPositionOf(id.id); pos >= 0)
            return pos;

    return 0;
}

ProgramId ProgramManager::resolve(ProgramBank bank, const juce::String& id,
                                   const juce::String& displayName) const
{
    if (bank == ProgramBank::init && id == kInitProgram.slug)
        return initId();

    if (bank == ProgramBank::factory)
        if (const int pos = factoryPositionOf(id); pos >= 0)
            return factoryIdAt(pos);

    if (bank == ProgramBank::user)
        for (const auto& f : userProgramFiles)
            if (f.getFileNameWithoutExtension() == id)
                return { ProgramBank::user, id, id };

    // **Degrade honestly.** The restored values are correct and stay put; only the name is unknown,
    // so the panel says so rather than landing on whichever Program now occupies some position.
    return { ProgramBank::unresolved, id, displayName.isNotEmpty() ? displayName : id };
}

std::vector<ProgramId> ProgramManager::listPrograms() const
{
    std::vector<ProgramId> out;
    out.reserve(1 + kFactoryPrograms.size() + (size_t) userProgramFiles.size());

    out.push_back(initId());

    for (size_t i = 0; i < kFactoryPrograms.size(); ++i)
        out.push_back(factoryIdAt((int) i));

    for (const auto& f : userProgramFiles)
    {
        const auto stem = f.getFileNameWithoutExtension();
        out.push_back({ ProgramBank::user, stem, stem });
    }

    return out;
}

juce::String ProgramManager::displayLabelFor(const ProgramId& id) const
{
    if (id.bank == ProgramBank::factory)
        if (const int pos = factoryPositionOf(id.id); pos >= 0)
            return juce::String(pos + 1).paddedLeft('0', 2) + " " + id.displayName;

    return id.displayName;
}

juce::File ProgramManager::userProgramFile(const juce::String& stem) const
{
    for (const auto& f : userProgramFiles)
        if (f.getFileNameWithoutExtension() == stem)
            return f;

    return {};
}

void ProgramManager::requestProgramChange(const ProgramId& id)
{
    {
        const juce::SpinLock::ScopedLockType lock(pendingLock);
        pendingProgram = id;
        hasPendingProgram = true;
    }

    triggerAsyncUpdate();
}

void ProgramManager::handleAsyncUpdate()
{
    ProgramId id;

    {
        const juce::SpinLock::ScopedLockType lock(pendingLock);

        if (! hasPendingProgram)
            return;

        id = pendingProgram;
        hasPendingProgram = false;
    }

    applyProgram(id);
}

juce::String ProgramManager::getProgramName(int factoryPosition) const
{
    return juce::isPositiveAndBelow(factoryPosition, kNumFactoryPrograms)
               ? juce::String(kFactoryPrograms[(size_t) factoryPosition].name)
               : juce::String();
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
    // **macOS needs the "Application Support" segment added by hand, and only macOS.** JUCE's
    // userApplicationDataDirectory is `~/Library` there - NOT `~/Library/Application Support` -
    // while it is `%APPDATA%` on Windows and `~/.config` on Linux, both of which are already the
    // right root. JUCE's own PropertiesFile appends the segment the same way, for the same reason.
    //
    // This was got wrong once in exactly the plausible direction: the note here used to claim JUCE
    // resolved the segment for us, and that hard-coding it would be wrong on two platforms out of
    // three. The first half was false, and the second half only argues for the `#if` - it is one
    // platform's extra segment, not a shared literal path. Programs landed directly in
    // `~/Library/<Company>/` for a while, which is not where application data goes on macOS and is
    // not a folder anything else writes into.
    //
    // No migration from the old location - nothing has shipped, so nothing is there to migrate.
    // See Elmer's ProgramManager for why that is a decision rather than an oversight.
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);

   #if JUCE_MAC
    dir = dir.getChildFile("Application Support");
   #endif

    return dir
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

    for (const auto& entry : juce::RangedDirectoryIterator(dir, false, juce::String("*") + programFileExtension))
        userProgramFiles.add(entry.getFile());

    // **The displayed name, case-insensitively.** The STEM, not getFileName() - comparing with the
    // extension attached sorts "AB C" before "AB", because a space (0x20) precedes the dot (0x2E).
    // And compareIgnoreCase, not operator<, which is a codepoint compare that put every lowercase
    // name after every uppercase one. The GUI forces uppercase as you type, which masked it for
    // anything created here; a file hand-renamed on disk or restored from a backup unmasks it.
    std::sort(userProgramFiles.begin(), userProgramFiles.end(),
               [] (const juce::File& a, const juce::File& b)
               {
                   return a.getFileNameWithoutExtension()
                           .compareIgnoreCase(b.getFileNameWithoutExtension()) < 0;
               });
}

void ProgramManager::applyProgram(const ProgramId& id)
{
    if (id.bank == ProgramBank::init)
    {
        applyFactoryProgram(kInitProgram);
    }
    else if (id.bank == ProgramBank::factory)
    {
        const int pos = factoryPositionOf(id.id);

        if (pos < 0)
            return;

        applyFactoryProgram(kFactoryPrograms[(size_t) pos]);
    }
    else if (id.bank == ProgramBank::user)
    {
        const auto file = userProgramFile(id.id);

        if (file == juce::File())
            return;

        std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));
        if (xml == nullptr || ! xml->hasTagName(apvts.state.getType()))
            return;

        // User Programs are a second serialisation path alongside the host session, and they carry
        // the same schema attribute - so they need the same migration. A Program saved before the
        // Algorithm reorder would otherwise load the old index meaning and select a different tank.
        LegacyMigration::remapLegacyAlgorithmIfNeeded(*xml);

        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
    else
    {
        // Unresolved: the values are whatever the session restored and stay exactly as they are.
        // Landing on some other Program would be the silent wrong answer this model exists to
        // prevent - only the identity is recorded, so the panel can say it does not know the name.
        setCurrentId(id);
        captureCleanSnapshot();

        if (onProgramListChanged)
            onProgramListChanged();

        return;
    }

    setCurrentId(id);
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

    juce::File file = dir.getChildFile(juce::File::createLegalFileName(name) + programFileExtension);

    // **SAVE never overwrites**, which BRAND.md states as a guarantee and this used to break: a
    // second save under one name replaced the first Program's contents silently. It matters more
    // now that the filename IS the identity - two Programs cannot share one.
    if (file.existsAsFile())
        file = file.getNonexistentSibling();

    xml->writeTo(file);

    refreshUserProgramList();
    const auto stem = file.getFileNameWithoutExtension();
    setCurrentId({ ProgramBank::user, stem, stem });
    // The just-saved program IS the current parameter state, so this becomes the new clean
    // baseline - SAVE goes back to disabled immediately after storing, until something moves again.
    captureCleanSnapshot();
    if (onProgramListChanged)
        onProgramListChanged();
}

void ProgramManager::deleteUserProgram(const ProgramId& id)
{
    if (id.bank != ProgramBank::user)
        return;

    const auto file = userProgramFile(id.id);

    if (file == juce::File())
        return;

    const bool wasCurrent = getCurrentProgramId() == id;
    file.deleteFile();
    refreshUserProgramList();

    // Deliberately NOT the unresolved state: deleting from the panel is unambiguous intent, so it
    // falls back to the default. Unresolved is for a session naming something that is gone.
    if (wasCurrent)
        requestProgramChange(factoryIdAt(defaultFactoryProgramIndex));
    else if (onProgramListChanged)
        onProgramListChanged();
}

void ProgramManager::setCurrentProgramWithoutApplying(const ProgramId& id)
{
    setCurrentId(id);

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
    {
        const juce::SpinLock::ScopedLockType lock(pendingLock);
        hasPendingProgram = false;
    }

    cancelPendingUpdate();
}
