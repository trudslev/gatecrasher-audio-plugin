#pragma once

#include <array>

// Flat POD table of the 17 factory programs, one field per APVTS parameter. Plain int indices
// (not the DSP-layer enums) for algorithm/shape/keySource, mirroring TapeRot's own
// FactoryPresets.h - keeps this header decoupled from the DSP classes; ProgramManager maps the
// indices onto the actual APVTS choice parameters when applying a program.
//
// Values here are a first, structurally-plausible pass matching each program's intended function
// (see the plan / prompts/PROMPTS.md) - not yet tuned by ear against the real DSP. Same status as
// TapeRot's factory presets before their own by-ear pass: audition each, adjust, done.
struct FactoryProgram
{
    const char* name;

    // Reverb section
    int algorithm;      // 0=Room, 1=Plate, 2=Chamber, 3=Ambience
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
    { "Intruder",           0,    0.55f, 5.0f,    0.50f, 0.55f,  0.45f,  0.30f,  -24.0f, 0.3f,   90.0f,  3.0f,   0, 0,     100.0f, 8000.0f, 2.0f,  110.0f, 70.0f, 0.0f },
    { "Air Tomorrow",       0,    0.68f, 8.0f,    0.62f, 0.60f,  0.50f,  0.32f,  -26.0f, 0.3f,   220.0f, 5.0f,   0, 0,     90.0f,  8000.0f, 6.5f,  120.0f, 78.0f, 0.0f },
    { "Cannon",             0,    0.95f, 10.0f,   0.85f, 0.70f,  0.35f,  0.20f,  -30.0f, 0.2f,   260.0f, 8.0f,   0, 0,     60.0f,  10000.0f, 9.0f,  140.0f, 85.0f, -1.0f },
    { "Tom Thunder",        0,    0.70f, 6.0f,    0.65f, 0.55f,  0.50f,  0.25f,  -28.0f, 0.5f,   150.0f, 6.0f,   0, 0,     45.0f,  900.0f,  5.0f,  115.0f, 75.0f, 0.0f },
    { "Kick Chuff",         3,    0.30f, 0.0f,    0.30f, 0.40f,  0.70f,  0.10f,  -22.0f, 0.2f,   40.0f,  15.0f,  1, 0,     30.0f,  500.0f,  3.0f,  100.0f, 35.0f, 0.0f },
    { "Drum Bus Gate",      0,    0.75f, 4.0f,    0.60f, 0.60f,  0.45f,  0.30f,  -20.0f, 0.3f,   180.0f, 10.0f,  0, 1,     80.0f,  12000.0f, 4.0f,  120.0f, 80.0f, 0.0f },
    { "Vocal Chop",         1,    0.50f, 3.0f,    0.50f, 0.65f,  0.40f,  0.35f,  -26.0f, 0.15f,  60.0f,  4.0f,   0, 0,     150.0f, 6000.0f, 3.0f,  110.0f, 60.0f, 0.0f },
    { "Radio Announcer",    3,    0.25f, 0.0f,    0.30f, 0.40f,  0.55f,  0.40f,  -32.0f, 0.4f,   100.0f, 20.0f,  1, 0,     120.0f, 5000.0f, 1.0f,  100.0f, 30.0f, 0.0f },
    { "Synth Stab",         1,    0.55f, 2.0f,    0.55f, 0.92f,  0.40f,  0.30f,  -24.0f, 0.1f,   80.0f,  5.0f,   0, 0,     100.0f, 9000.0f, 5.0f,  130.0f, 65.0f, 0.0f },
    { "Arp Gate",           2,    0.40f, 0.0f,    0.40f, 0.50f,  0.50f,  0.35f,  -18.0f, 0.1f,   15.0f,  2.0f,   0, 0,     200.0f, 6000.0f, 2.0f,  120.0f, 55.0f, 0.0f },
    { "Neon Pad Swell",     2,    0.80f, 90.0f,   0.70f, 0.65f,  0.45f,  0.30f,  -28.0f, 2.0f,   300.0f, 60.0f,  1, 0,     100.0f, 6000.0f, 3.0f,  150.0f, 70.0f, 0.0f },
    { "Power Chord Gate",   0,    0.50f, 2.0f,    0.45f, 0.50f,  0.50f,  0.35f,  -22.0f, 0.2f,   70.0f,  8.0f,   0, 1,     150.0f, 4000.0f, 4.0f,  110.0f, 55.0f, 0.0f },
    { "Solo Ambience",      2,    0.60f, 15.0f,   0.55f, 0.55f,  0.50f,  0.30f,  -30.0f, 0.6f,   130.0f, 25.0f,  1, 0,     90.0f,  7000.0f, 1.5f,  125.0f, 45.0f, 0.0f },
    { "Room Reinforcement", 0,    0.45f, 5.0f,    0.40f, 0.50f,  0.50f,  0.30f,  -34.0f, 0.5f,   400.0f, 180.0f, 1, 0,     80.0f,  8000.0f, 0.5f,  105.0f, 20.0f, 0.0f },
    { "Wall of Sound",      0,    1.00f, 20.0f,   0.95f, 1.00f,  0.20f,  0.10f,  -18.0f, 0.1f,   450.0f, 40.0f,  0, 0,     40.0f,  15000.0f, 11.0f, 180.0f, 95.0f, -3.0f },
    { "Stutter Gate",       1,    0.35f, 0.0f,    0.35f, 0.50f,  0.50f,  0.35f,  -16.0f, 0.1f,   10.0f,  1.0f,   0, 0,     200.0f, 5000.0f, 2.0f,  115.0f, 60.0f, 0.0f },
    { "Detonator",          0,    1.00f, 120.0f,  1.00f, 1.00f,  1.00f,  1.00f,  -12.0f, 0.1f,   500.0f, 200.0f, 1, 0,     20.0f,  20000.0f, 12.0f, 200.0f, 100.0f, -6.0f },
} };

inline constexpr int kNumFactoryPrograms = (int) kFactoryPrograms.size();

// Loaded on first launch / when no saved session state exists yet - "Air Tomorrow" is the fuller,
// more immediately impressive version of the plugin's namesake sound, same reasoning as TapeRot
// defaulting to "Warm Cassette" rather than a more extreme preset.
inline constexpr int defaultFactoryProgramIndex = 1;
