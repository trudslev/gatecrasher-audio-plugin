#pragma once

#include <juce_core/juce_core.h>

#include <nf/ProgramId.h>

#include <array>

// Flat POD table of the 17 factory programs, one field per APVTS parameter. Plain int indices
// (not the DSP-layer enums) for algorithm/shape/keySource, mirroring TapeRot's own
// FactoryPresets.h - keeps this header decoupled from the DSP classes; ProgramManager maps the
// indices onto the actual APVTS choice parameters when applying a program.
//
// Values here are a first, structurally-plausible pass matching each program's intended function
// (see the plan / prompts/PROMPTS.md) - not yet tuned by ear against the real DSP. Same status as
// TapeRot's factory presets before their own by-ear pass: audition each, adjust, done.
/** **Program identity comes from core, and these aliases are the whole of the local surface.**

    `nf::ProgramBank` and `nf::ProgramId` say what six identical copies used to say separately:
    INIT is its own bank rather than a magic index, identity is a permanent slug for a Factory
    Program and the filename stem for a User one, and `displayName` is carried for presentation
    only - a factory slug is not presentable in the LCD - and is deliberately outside `operator==`,
    so a corrected typo in the bank cannot make a Program stop equalling itself.

    Aliased rather than used qualified at every call site, because the unqualified names are what
    the panel, the header and the tests already read. */
using ProgramBank = nf::ProgramBank;
using ProgramId   = nf::ProgramId;

struct FactoryProgram
{
    /** **The permanent identity, fixed at creation and never changed again.** `name` is a label the
        designers may revise; `slug` may not be, because it is what a saved session stores and what
        resolves back to this entry. Renaming the Program is free; renaming the slug orphans every
        session that referenced it. */
    const char* slug;

    const char* name;

    // Reverb section
    int algorithm;      // 0=Ambience, 1=Room, 2=Plate, 3=Chamber (panel order, spec 9.1)
    float size;          // 0-1
    float preDelayMs;
    float decay;          // 0-1, automation-only (no panel control)
    float density;         // 0-1, automation-only (no panel control)
    float dampHF;            // 0-1
    float dampLF;              // 0-1

    // Gate section
    float thresholdDb;
    float attackMs;
    float holdMs;
    float releaseMs;
    int shape;            // 0=Hard, 1=Soft
    int keySource;         // 0=Internal, 1=Sidechain
    float trigHPHz;
    float trigLPHz;

    // Output section
    float slamDb;
    float widthPercent;
    float mixPercent;
    float trimDb;
};

inline constexpr std::array<FactoryProgram, 17> kFactoryPrograms{ {
    // name,                algo, size, preDelay, decay, density, dampHF, dampLF, thresh, attack, hold,  release, shape, keySrc, trigHP, trigLP, slam, width, mix, trim
    { "air-tomorrow", "AIR TOMORROW",       1,    0.68f, 8.0f,    0.62f, 0.60f,  0.50f,  0.32f,  -26.0f, 0.3f,   220.0f, 5.0f,   0, 0,     90.0f,  8000.0f, 6.5f,  120.0f, 78.0f, 0.0f },
    { "intruder", "INTRUDER",           1,    0.55f, 5.0f,    0.50f, 0.55f,  0.45f,  0.30f,  -24.0f, 0.3f,   90.0f,  3.0f,   0, 0,     100.0f, 8000.0f, 2.0f,  110.0f, 70.0f, 0.0f },
    { "cannon", "CANNON",             1,    0.95f, 10.0f,   0.85f, 0.70f,  0.35f,  0.20f,  -30.0f, 0.2f,   260.0f, 8.0f,   0, 0,     60.0f,  10000.0f, 9.0f,  140.0f, 85.0f, -1.0f },
    { "tom-thunder", "TOM THUNDER",        1,    0.70f, 6.0f,    0.65f, 0.55f,  0.50f,  0.25f,  -28.0f, 0.5f,   150.0f, 6.0f,   0, 0,     45.0f,  900.0f,  5.0f,  115.0f, 75.0f, 0.0f },
    { "kick-chuff", "KICK CHUFF",         0,    0.30f, 0.0f,    0.30f, 0.40f,  0.70f,  0.10f,  -22.0f, 0.2f,   40.0f,  15.0f,  1, 0,     30.0f,  500.0f,  3.0f,  100.0f, 35.0f, 0.0f },
    { "drum-bus-gate", "DRUM BUS GATE",      1,    0.75f, 4.0f,    0.60f, 0.60f,  0.45f,  0.30f,  -20.0f, 0.3f,   180.0f, 10.0f,  0, 1,     80.0f,  12000.0f, 4.0f,  120.0f, 80.0f, 0.0f },
    { "vocal-chop", "VOCAL CHOP",         2,    0.50f, 3.0f,    0.50f, 0.65f,  0.40f,  0.35f,  -26.0f, 0.15f,  60.0f,  4.0f,   0, 0,     150.0f, 6000.0f, 3.0f,  110.0f, 60.0f, 0.0f },
    { "radio-announcer", "RADIO ANNOUNCER",    0,    0.25f, 0.0f,    0.30f, 0.40f,  0.55f,  0.40f,  -32.0f, 0.4f,   100.0f, 20.0f,  1, 0,     120.0f, 5000.0f, 1.0f,  100.0f, 30.0f, 0.0f },
    { "synth-stab", "SYNTH STAB",         2,    0.55f, 2.0f,    0.55f, 0.92f,  0.40f,  0.30f,  -24.0f, 0.1f,   80.0f,  5.0f,   0, 0,     100.0f, 9000.0f, 5.0f,  130.0f, 65.0f, 0.0f },
    { "arp-gate", "ARP GATE",           3,    0.40f, 0.0f,    0.40f, 0.50f,  0.50f,  0.35f,  -18.0f, 0.1f,   15.0f,  2.0f,   0, 0,     200.0f, 6000.0f, 2.0f,  120.0f, 55.0f, 0.0f },
    { "neon-pad-swell", "NEON PAD SWELL",     3,    0.80f, 90.0f,   0.70f, 0.65f,  0.45f,  0.30f,  -28.0f, 2.0f,   300.0f, 60.0f,  1, 0,     100.0f, 6000.0f, 3.0f,  150.0f, 70.0f, 0.0f },
    { "power-chord-gate", "POWER CHORD GATE",   1,    0.50f, 2.0f,    0.45f, 0.50f,  0.50f,  0.35f,  -22.0f, 0.2f,   70.0f,  8.0f,   0, 1,     150.0f, 4000.0f, 4.0f,  110.0f, 55.0f, 0.0f },
    { "solo-ambience", "SOLO AMBIENCE",      3,    0.60f, 15.0f,   0.55f, 0.55f,  0.50f,  0.30f,  -30.0f, 0.6f,   130.0f, 25.0f,  1, 0,     90.0f,  7000.0f, 1.5f,  125.0f, 45.0f, 0.0f },
    { "room-reinforcement", "ROOM REINFORCEMENT", 1,    0.45f, 5.0f,    0.40f, 0.50f,  0.50f,  0.30f,  -34.0f, 0.5f,   400.0f, 180.0f, 1, 0,     80.0f,  8000.0f, 0.5f,  105.0f, 20.0f, 0.0f },
    { "wall-of-sound", "WALL OF SOUND",      1,    1.00f, 20.0f,   0.95f, 1.00f,  0.20f,  0.10f,  -18.0f, 0.1f,   450.0f, 40.0f,  0, 0,     40.0f,  15000.0f, 11.0f, 180.0f, 95.0f, -3.0f },
    { "stutter-gate", "STUTTER GATE",       2,    0.35f, 0.0f,    0.35f, 0.50f,  0.50f,  0.35f,  -16.0f, 0.1f,   10.0f,  1.0f,   0, 0,     200.0f, 5000.0f, 2.0f,  115.0f, 60.0f, 0.0f },
    { "detonator", "DETONATOR",          1,    1.00f, 120.0f,  1.00f, 1.00f,  1.00f,  1.00f,  -12.0f, 0.1f,   500.0f, 200.0f, 1, 0,     20.0f,  20000.0f, 12.0f, 200.0f, 100.0f, -6.0f },
} };

inline constexpr int kNumFactoryPrograms = (int) kFactoryPrograms.size();

/** INIT's index. **-1, deliberately outside the bank rather than position 0 within it.**

    INIT is the blank canvas you start from, not an authored sound competing with the eleven, so
    numbering it would push Air Tomorrow to 02 and imply a running order it is not part of. Keeping
    it outside also means it never renumbers anything.

    -1 is therefore a meaningful index here, so every "no index" sentinel in this casting has to be
    something else - see ProgramManager's pendingIndex, which is -2. */
inline constexpr int initProgramIndex = -1;

/** The blank canvas: gate and reverb both present and audible in their plainest form, with
    everything that gives Gatecrasher its character at zero.

    Three rules decide every value, and they are not the same rule:
      - **Character and amount go to zero** - Slam at 0 dB. Raise it and you immediately hear what
        it does.
      - **Structure goes to a usable middle, never zero** - Size, Decay and Density at 0.5. A reverb
        at zero decay is not neutral, it is broken; there would be nothing to hear when the gate
        opened.
      - **Anything meaning "not acting" takes whatever value that is** - and for a GATE that is a
        threshold too low ever to close, so **-60 dB, the bottom of the range**. The gate is
        genuinely there and genuinely open; raising the threshold is the first thing that makes it
        act. Trigger HP at its 20 Hz floor and LP at its 20 kHz ceiling are both wide open, so the
        detector hears the whole signal. Width 100 % is the neutral point of a 0-200 range, not a
        setting. Trim 0 dB.

    Hold at its 500 ms maximum and Release at its 200 ms maximum are also "not acting": with the
    threshold on the floor the gate never closes, so the two controls that govern how it closes are
    parked where they interfere least.

    **Mix is 50 %, not 100 %.** Gatecrasher is a wet/dry effect, and the midpoint reads as "nothing
    decided yet" where a value like 35 % would look like a judgement someone made. The two serial
    castings, TapeRot and Elmer, sit at 100 % for the opposite reason. */
inline constexpr FactoryProgram kInitProgram
    { "init", "INIT",               2,    0.50f, 0.0f,    0.50f, 0.50f,  0.00f,  0.00f,  -60.0f, 0.1f,   500.0f, 200.0f, 1, 0,     20.0f,  20000.0f, 0.0f,  100.0f, 50.0f, 0.0f };

// Loaded on first launch / when no saved session state exists yet - "Air Tomorrow" is the fuller,
// more immediately impressive version of the plugin's namesake sound, same reasoning as TapeRot
// defaulting to "Warm Cassette" rather than a more extreme preset.
//
// It sits FIRST in the bank so that the default is also Program 01. It used to be second, which
// meant every fresh instance opened reading "02" with nothing having selected it - the LCD implied
// the user had scrolled somewhere, and the obvious "go back to the start" gesture landed on a
// different sound. Which Program is the default and which is first are separable, but there is no
// reason for them to differ, and one of the two orderings makes the panel explain itself.
inline constexpr int defaultFactoryProgramIndex = 0;
