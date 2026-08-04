Convention: once a prompt below has been implemented, mark it SHIPPED with the date, e.g.
"PROMPT #1 - SHIPPED 2026-08-03".

PROMPT #1 - SHIPPED 2026-08-03

Scaffold Gatecrasher, an 80s-style gated reverb, as a new sibling plugin to TapeRot under
neon-foundry/. Study TapeRot's Source/ layout, DSP folder organization (one class per
responsibility), Tests/ setup, and BUILDING.md format, and carry those conventions over without
copying TapeRot's DSP-specific code. One deliberate GUI divergence: asset-based (pre-rendered
bitmap components) rather than TapeRot's fully vector/code-drawn approach, per
design/GATECRASHER-GUI-SPEC.md and design/CLAUDE.md (already authored by Claude Design) - use the
approved gate-closed panel bitmap as one flat static background with only the inherently-live
elements (knobs, scope, lamp, meter, switches, header text) layered on top, reconciled against
BRAND.md's shared component grammar (red reserved for gate state, Program not Preset terminology,
etc).

Delivered: full project scaffold (CMakeLists.txt, .gitignore, README.md, BUILDING.md, CLAUDE.md);
Parameters.h with all 19 parameters; the DSP signal chain (TriggerDetector,
GateEnvelopeGenerator, CombAllpassNetwork + RoomTank/PlateTank/ChamberTank/AmbienceTank +
ReverbEngine, DampingStage, SlamSaturation, StereoWidthStage, OutputMixStage); ProgramManager +
FactoryPrograms.h with all 17 factory programs (save-always-creates-new, never overwrites, no
"New Program" button); PluginProcessor/PluginEditor wiring it all together; the GUI layer
(GatecrasherPanelBackground, KnobFilmstripComponent, GateScope, GateLamp, InputMeter,
ToggleSwitchComponent, ProgramHeader, WordmarkComponent, GatecrasherEditorContent); and a
JUCE-UnitTest DSP test suite.

PROMPT #2

Follow-up work identified during PROMPT #1, not yet done:

1. DSP tuning by ear. The four reverb tanks' comb/allpass delay-length tables and feedback ranges
   (CombAllpassNetwork.h, RoomTank/PlateTank/ChamberTank/AmbienceTank.cpp) and all 17 factory
   programs' parameter values (FactoryPrograms.h) are a structurally-plausible first pass, not a
   tuned one - same status TapeRot's own factory presets had before their by-ear pass. Build, load
   each program, listen, adjust. Pay particular attention to whether Room/Plate/Chamber/Ambience
   actually sound like four distinct algorithm characters rather than the same topology with
   different knob positions, and whether Wall of Sound/Detonator are appropriately
   over-the-top without simply distorting.

2. DONE 2026-08-04. Barlow Condensed (full family) and Share Tech Mono Regular were added to
   design/assets/. Wired the SemiBold (600, labels)/Bold (700, lamp text) weights and Share Tech
   Mono into GatecrasherTheme::labelFont/labelFontBold/monoFont/monoFontBold in place of the
   interim JUCE-default-font placeholders, and embedded them via CMakeLists.txt's
   juce_add_binary_data. Remaining: no LICENSE.txt/OFL notice shipped alongside the fonts yet
   (TapeRot's design/inter/LICENSE.txt is the precedent to match) - both are SIL Open Font License,
   free for commercial use, but the notice file itself is still missing.

3. Bake the real wordmark. GATECRASHER-GUI-SPEC.md section 8 wants a pre-rendered transparent PNG
   (sprayed-stencil treatment: per-letter rotation/drift/opacity variance, speckle mask, two-stage
   overspray halo, spatter flecks) at x38/y20, ~232x40 @1x, shipped @2x (and optionally @3x).
   WordmarkComponent currently draws "GATECRASHER" directly in TudorVictors as an interim
   placeholder - once the real PNG exists in design/assets/, swap it in, add it to CMakeLists.txt's
   juce_add_binary_data SOURCES list, and remove the interim TudorVictors.ttf embed if nothing else
   needs it live.

4. Confirm PLUGIN_MANUFACTURER_CODE (Gcsh), PLUGIN_CODE (Gr85), BUNDLE_ID
   (com.gatecrasher.gatecrasher), and COMPANY_NAME in CMakeLists.txt before any real release -
   currently placeholders, effectively permanent once shipped or automated against.

5. Minor: `auval -v aufx Gr85 Gcsh` passes overall (AU VALIDATION SUCCEEDED) but logs one warning
   on the Trigger HP parameter - "Parameter did not retain default value when set" - likely
   float32 round-trip drift through its skewed NormalisableRange (skew 0.3, same skew convention
   TapeRot's LP/HP use) rather than a real defect, but worth a quick look before release.

6. GatecrasherEditorContent doesn't currently paint live numeric values under each knob (e.g.
   "-18.5 dB" updating as Threshold moves) - the design spec has no per-knob label-rect
   coordinates to build against. Numeric feedback is the standard Slider drag popup for now.
   Worth a design decision on whether/where a persistent readout should live.
