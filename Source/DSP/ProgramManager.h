#pragma once

#include "FactoryPrograms.h"

#include <nf/ParameterSnapshot.h>
#include <nf/UserProgramStore.h>
#include <functional>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>

// Owns factory + user program bookkeeping and file I/O: which program is current, the sorted list
// of user program files on disk, and applying a program's saved parameter values to the APVTS.
//
// Async-safe the same way TapeRot's preset switching is - a host can call requestProgramChange
// from a non-message thread (VST3 delivers program-change as an automatable parameter, which can
// arrive during audio-thread automation), so the actual apply is deferred through AsyncUpdater.
// requestProgramChange is safe from any thread; handleAsyncUpdate always runs on the message
// thread.
//
// Save always creates a new named user program and never overwrites an existing one, even the
// currently-loaded one - there is no "overwrite" or "New Program" method anywhere in this
// interface. Starting fresh is just loading any program, tweaking APVTS parameters, and calling
// saveNewUserProgram.
class ProgramManager : private juce::AsyncUpdater
{
public:
    /** @param userDirectoryOverride  where User Programs live. Defaults to the real per-OS
                                      location; a test passes a temporary directory so it never
                                      writes into the user's own Programs folder. */
    explicit ProgramManager(juce::AudioProcessorValueTreeState& stateToControl,
                            juce::File userDirectoryOverride = {});
    ~ProgramManager() override;

    // Call once from PluginProcessor's constructor, after the APVTS/parameters exist.
    void initialise();

    /** The Factory bank's size - what the host is told, and it never changes. See PluginProcessor's
        adapter comment for why that is a conformance requirement rather than a preference. */
    int getNumPrograms() const noexcept { return kNumFactoryPrograms; }

    /** The current Program's identity. Everything on the panel reads this. */
    ProgramId getCurrentProgramId() const;

    /** Factory identity at a bank position, and INIT's. Position is an argument here and nowhere
        else - this is how the host adapter and the dropdown cross the boundary. */
    static ProgramId factoryIdAt(int factoryPosition);
    static ProgramId initId();
    static int factoryPositionOf(const juce::String& slug);

    /** The Factory position of the current Program, or 0 when it is INIT, a User Program or
        unresolved - none of which the host's list contains. */
    int getCurrentFactoryPosition() const;

    /** Resolves an identifier, or returns an `unresolved` ProgramId carrying the name to show. */
    ProgramId resolve(ProgramBank bank, const juce::String& id, const juce::String& displayName) const;

    /** The list the dropdown paints, in display order: INIT, then Factory, then User. */
    std::vector<ProgramId> listPrograms() const;

    /** **What the LCD and the dropdown print - a label, not a key.** The two-digit number is
        computed from the Factory position at paint time; nothing stores it and nothing resolves by
        it. User Programs carry no number - they sort alphabetically, so one would change whenever a
        Program was saved. INIT is unnumbered because it is outside the bank. */
    juce::String displayLabelFor(const ProgramId& id) const;
    // Safe to call from any thread (see class comment) - actual application happens async.
    void requestProgramChange(const ProgramId& id);

    /** Raw, unnumbered - what the HOST's list wants, since a host renders its own numbering. */
    juce::String getProgramName(int factoryPosition) const;



    // Always creates a new file and switches to it - never overwrites. Trimming, upper-casing, the
    // maxProgramNameLength cap and the empty-name fallback are all nf::UserProgramStore::resolveName
    // now, so they apply on every path rather than only where a caller remembered them.
    //
    // The fallback is TAKE n, not GUI-SPEC.md section 6's "NEW PROGRAM": a suite ruling replaced six
    // different strings with the one that is better rather than merely different, since consecutive
    // empty saves give TAKE 3, TAKE 4 instead of leaning on getNonexistentSibling for
    // "NEW PROGRAM (2)". The spec predates the ruling.
    /** Where this instance stores User Programs, and the real per-OS location regardless of it. */
    juce::File getUserProgramDirectory() const;
    static juce::File getDefaultUserProgramDirectory();

    void saveNewUserProgram(const juce::String& requestedName);

    // No-op for factory indices. Falls back to defaultFactoryProgramIndex if the deleted program
    // was the currently loaded one.
    void deleteUserProgram(const ProgramId& id);

    // True once the APVTS parameters differ from the currently-loaded program's own values - i.e.
    // the user has actually turned something and there is a change worth saving. The GUI uses this
    // to disable SAVE on an untouched program (see ProgramHeader), so "Save" always means "save the
    // edits I just made as a new program" rather than "duplicate this program unchanged".
    bool isModifiedFromLoadedProgram() const;

    // Called by PluginProcessor's setStateInformation after apvts.replaceState() restores a saved
    // session - keeps the FACT/USER header tag in sync with whatever program index the session
    // remembers, without re-applying its parameters (they just came from the session state itself).
    /** Restores which Program is showing without touching a parameter - session restore has
        already put the values where they belong. */
    void setCurrentProgramWithoutApplying(const ProgramId& id);

    // Called by PluginProcessor's setStateInformation before restoring a full session: drops any
    // requestProgramChange that arrived just before the restore but hasn't been applied yet (its
    // handleAsyncUpdate dispatch may otherwise land after the restore and silently clobber the
    // just-restored parameter values with a stale program).
    void cancelPendingChange() noexcept;

    // PluginProcessor wires this to updateHostDisplay(...withProgramChanged(true)) - kept as a
    // callback rather than a base-class call so ProgramManager doesn't need to know about
    // juce::AudioProcessor at all.
    std::function<void()> onProgramListChanged;

private:
    void handleAsyncUpdate() override;
    void applyProgram(const ProgramId& id);
    void setCurrentId(const ProgramId& id);
    void applyFactoryProgram(const FactoryProgram& program);
    void captureCleanSnapshot();

    juce::AudioProcessorValueTreeState& apvts;

    // The User bank on disk. Scanning, sorting, naming, the collision check, save and delete are
    // core's; WHAT a Program contains - the whole APVTS state - stays here.
    nf::UserProgramStore store;

    // Guarded rather than atomic: a ProgramId holds two juce::Strings. Contention is near-zero -
    // writes happen on a Program change only - so the spin lock costs nothing and never allocates.
    mutable juce::SpinLock currentIdLock;
    ProgramId currentId;
    // "Nothing pending" is its own flag now rather than a sentinel value, which removes the last
    // reason to reserve magic negative numbers.
    juce::SpinLock pendingLock;
    bool hasPendingProgram = false;
    ProgramId pendingProgram;

    // The baseline isModifiedFromLoadedProgram compares against. Keyed by parameter ID inside
    // rather than held in getParameters() order - see nf/ParameterSnapshot.h for why an index is
    // the wrong key, and for the SpinLock this used to lack. The claim that every writer runs on
    // the message thread was not quite true: setStateInformation carries no thread guarantee.
    nf::ParameterSnapshot cleanSnapshot;

    // Section 6.1: the 252px name cell fits 27 characters at Share Tech Mono 13px/.10em, and a
    // two-digit index plus a space takes three of them - so names cap at 24, exactly the budget.
    // **25, recomputed now that the "NN " prefix is gone from the displayed string.** The 252px
    // name cell less 10px padding each side is 232px, and Share Tech Mono 13px at .10em advances
    // 8.32px, so the cell holds 27; the dirty marker " *" takes 2.
    //
    // The old 24 was computed as 27 - 3 for the index prefix and ignored the marker entirely, so
    // "NN " + 24 + " *" came to 29 in a 27-character cell - an overrun that only appeared once an
    // edited Program had a long name. Dropping the prefix fixes that as well as freeing a character.
    static constexpr int maxProgramNameLength = 25;
};
