#include "ProgramManager.h"

#include <nf/UserProgramDirectory.h>
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

ProgramManager::ProgramManager(juce::AudioProcessorValueTreeState& stateToControl,
                               juce::File userDirectoryOverride)
    : apvts(stateToControl),
      store(nf::userProgramDirectory(pluginCompanyName, pluginProductName, userDirectoryOverride),
            programFileExtension,
            maxProgramNameLength)
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
    store.refresh();
    applyFactoryProgram(kFactoryPrograms[(size_t) defaultFactoryProgramIndex]);
    setCurrentId(factoryIdAt(defaultFactoryProgramIndex));
    captureCleanSnapshot();
}

void ProgramManager::captureCleanSnapshot()
{
    cleanSnapshot.capture(apvts.processor);
}

bool ProgramManager::isModifiedFromLoadedProgram() const
{
    // **Every parameter, with no exclusion list.** Gatecrasher has no momentary triggers - unlike
    // TapeRot, which must exclude STOP/FILTER/FAIL or holding one lights SAVE - so the compared set
    // is simply everything.
    //
    // Still compared in normalised 0..1 space at 1e-4, the figure this casting already used: loose
    // enough to absorb the float round-trip through a user Program's XML, far tighter than the
    // smallest movement any control can produce.
    return cleanSnapshot.differsFrom(apvts.processor);
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
        if (store.fileFor(id) != juce::File())
            return { ProgramBank::user, id, id };

    // **Degrade honestly.** The restored values are correct and stay put; only the name is unknown,
    // so the panel says so rather than landing on whichever Program now occupies some position.
    return { ProgramBank::unresolved, id, displayName.isNotEmpty() ? displayName : id };
}

std::vector<ProgramId> ProgramManager::listPrograms() const
{
    std::vector<ProgramId> out;
    out.reserve(1 + kFactoryPrograms.size() + (size_t) store.getFiles().size());

    out.push_back(initId());

    for (size_t i = 0; i < kFactoryPrograms.size(); ++i)
        out.push_back(factoryIdAt((int) i));

    for (const auto& f : store.getFiles())
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


/*  **The critical section is a SWAP now, and it used to be two assignments.**

    A `juce::String` copy is a refcount increment and reads as safe. The ASSIGNMENT is the other
    half: it releases whatever the target held first, and a refcount reaching zero calls `free()`.
    So `pendingProgram = id` and `id = pendingProgram` each did heap work, and both were inside the
    lock — on a path VST3 can deliver **on the audio thread**, since a program change is an
    automatable parameter there.

    **Measured at 0.12 us worst case against a 10,667 us block budget**, so this was never a dropout
    risk and is not sold as one. It is negligible because a refcount release happens to be cheap,
    not because anything guarantees the path stays heap-free — and the next person to add a field to
    `ProgramId` has no reason to think about it.

    The copy and the destruction both move OUT of the lock: `exchangePendingProgram` takes its
    argument by value, so the caller's copy is made in the caller's frame, and returns the previous
    program by value, so its release happens in the caller's frame too. What is left between the
    lock and the unlock is a pointer exchange.

    **Named functions rather than inline blocks because that is what makes it testable.** An
    allocation sentinel is not lock-aware, so a probe around `requestProgramChange` sees the same
    total either way — the change is WHERE the work happens, not whether it happens. Arming the
    sentinel around a function that IS the critical section is the only honest way to assert it. */
ProgramId ProgramManager::exchangePendingProgram (ProgramId incoming)
{
    const juce::SpinLock::ScopedLockType lock (pendingLock);

    std::swap (pendingProgram, incoming);
    hasPendingProgram = true;

    return incoming;   // the PREVIOUS pending program; it is released in the caller's frame
}

bool ProgramManager::takePendingProgram (ProgramId& out)
{
    const juce::SpinLock::ScopedLockType lock (pendingLock);

    if (! hasPendingProgram)
        return false;

    // `out` is empty on entry, so this is a pointer exchange and nothing is released here.
    std::swap (out, pendingProgram);
    hasPendingProgram = false;

    return true;
}

void ProgramManager::requestProgramChange (const ProgramId& id)
{
    // The copy is made HERE, in this frame: copying a ProgramId is two refcount increments, and an
    // increment never frees. The previous pending program comes back and is released here too.
    const ProgramId previous = exchangePendingProgram (id);
    juce::ignoreUnused (previous);

    triggerAsyncUpdate();
}

void ProgramManager::handleAsyncUpdate()
{
    ProgramId id;

    if (! takePendingProgram (id))
        return;

    applyProgram (id);
}

juce::String ProgramManager::getProgramName(int factoryPosition) const
{
    return juce::isPositiveAndBelow(factoryPosition, kNumFactoryPrograms)
               ? juce::String(kFactoryPrograms[(size_t) factoryPosition].name)
               : juce::String();
}

juce::File ProgramManager::getUserProgramDirectory() const
{
    return store.getDirectory();
}

juce::File ProgramManager::getDefaultUserProgramDirectory()
{
    // The per-OS resolution, the "Application Support" segment macOS alone needs, and the reason
    // ~/Library/Audio/Presets is the wrong answer are all in nf/UserProgramDirectory.h now. That
    // reasoning was carried in six near-identical comment blocks, and the one time it was wrong it
    // was wrong in all six at once.
    return nf::userProgramDirectory(pluginCompanyName, pluginProductName);
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
        const auto file = store.fileFor(id.id);

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
    // **What a Program CONTAINS stays here** - the whole APVTS state plus the schema version. Core
    // owns naming, the collision check and the write, and takes finished XML.
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    xml->setAttribute(LegacyMigration::stateSchemaVersionAttribute, LegacyMigration::currentStateSchemaVersion);

    // **The empty-name fallback is `TAKE n` now, not `NEW PROGRAM`.** The suite had five different
    // ones across six castings; TAKE n is the one that is better rather than merely different,
    // since consecutive empty saves give TAKE 3, TAKE 4 instead of leaning on getNonexistentSibling
    // for "NEW PROGRAM (2)". Trimming, upper-casing and the 25-character cap are core's now too -
    // this casting already applied all three here, so only the fallback string changes.
    const auto file = store.save(requestedName, *xml);

    if (file == juce::File())
        return;   // the write failed; the panel keeps naming the Program it was already on

    // **The stem comes off the file core returned, not off the requested name.** A collision takes
    // the next free sibling, so taking it from the request would point the panel at the first file
    // while the values came from the second.
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

    const bool wasCurrent = getCurrentProgramId() == id;

    if (! store.remove(id.id))
        return;

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
