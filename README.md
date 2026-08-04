# Gatecrasher

An 80s-style gated reverb plugin (AU/VST3/Standalone on macOS; VST3/Standalone on Windows/Linux),
built with JUCE 8. Second casting under the [Neon Foundry](../BRAND.md) umbrella, sibling to
[TapeRot](../taperot/).

Gatecrasher emulates the choked, arena-huge ambience of the accidental gated-reverb drum sound that
defined 80s record production: a reverb tank (Room/Plate/Chamber/Ambience algorithms) runs
continuously off the input, and a fast noise-gate chops its tail off after a set Hold time instead
of letting it decay naturally - the "In The Air Tonight" trick, generalized into a full processor.

## Parameters

| Parameter | Range | Default | Notes |
|---|---|---|---|
| Algorithm | Room / Plate / Chamber / Ambience | Plate | Reverb tank topology |
| Size | 0-1 | 0.72 | Tank delay-network scale |
| Pre-Delay | 0-120ms | 18ms | Gap before the tank's onset |
| Decay | 0-1 | 0.6 | Tank feedback / tail length - automation-only, no panel control |
| Density | 0-1 | 0.6 | Diffusion amount - automation-only, no panel control |
| Damping HF / LF | 0-1 / 0-1 | 0.55 / 0.35 | Post-tank tone shaping |
| Threshold | -60..0 dB | -18.5dB | Gate trigger level |
| Attack | 0.1-20ms | 0.4ms | Gate open ramp |
| Hold | 10-500ms | 165ms | Time held fully open after trigger |
| Release | 1-200ms | 4ms | Gate close ramp (see Shape) |
| Shape | Hard / Soft | Hard | Release curve character - Hard is a near-instant cliff, Soft is a longer curved fall |
| Trigger Source | Internal / Sidechain | Internal | What drives the gate |
| Trigger Filter HP / LP | 20Hz-2kHz / 500Hz-20kHz | 180Hz / 6.3kHz | Shapes the detection signal only, never the audio |
| Slam | 0-12 | 7 | Drive into a saturation stage on the gated tail |
| Stereo Width | 0-200% | 128% | Mid/side width on the wet signal |
| Mix | 0-100% | 64% | Dry/wet |
| Output Trim | -24..+12 dB | -1.4dB | Output level |

## Building

See [BUILDING.md](BUILDING.md) for per-platform build requirements, commands, validation
(auval/pluginval on macOS, pluginval on Windows/Linux), and running the DSP test suite.

## Project layout

```
Source/
  PluginProcessor.*    Audio processor: parameter caching, signal chain, program management wiring
  PluginEditor.*        Editor shell, fixed-aspect-ratio scaling
  Parameters.h          APVTS parameter layout, IDs, and legacy-session migration seam
  DSP/
    TriggerDetector, GateEnvelopeGenerator   Sidechain/internal trigger detection and the gate envelope
    ReverbTank, RoomTank, PlateTank,
    ChamberTank, AmbienceTank, ReverbEngine   One tank class per algorithm, switch-crossfaded
    CombAllpassNetwork                        Shared comb+allpass diffusion network the tanks configure
    DampingStage, SlamSaturation,
    StereoWidthStage, OutputMixStage          Post-tank tone, drive, width, and dry/wet/trim stages
    ProgramManager, FactoryPrograms           Factory (read-only) + user (read-write) program banks
  GUI/
    GatecrasherLookAndFeel/Theme     Layout/colour constants (960x434 reference canvas)
    GatecrasherPanelBackground       Full static fascia bitmap - the deliberate asset-based divergence from TapeRot
    KnobFilmstripComponent           Bitmap sprite-sheet knobs
    GateScope, GateLamp, InputMeter  Live, code-drawn indicators
    ToggleSwitchComponent            Shared Key Source / Shape switch
    ProgramHeader                    Three-bitmap-state program LCD + name-entry flow
    WordmarkComponent                Baked nameplate (interim placeholder pending the real asset)
    GatecrasherEditorContent         Assembles and positions all of the above
Tests/                   JUCE-UnitTest DSP unit tests (see BUILDING.md to run)
design/                  GUI spec, approved reference renders, bitmap/font assets (see design/CLAUDE.md)
prompts/                 Numbered work-package prompts (gitignored, local-only)
```

## Status

See [CLAUDE.md](CLAUDE.md) for full architecture notes and current status, including the DSP
tuning and GUI-asset follow-ups tracked in `prompts/PROMPTS.md`.
