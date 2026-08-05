# CLAUDE.md

This file provides guidance to Claude Code when working with code in this repository.

Gatecrasher is its own independent repo and does not depend on `../taperot/` at runtime - it's a
sibling casting under the shared [Neon Foundry](../BRAND.md) umbrella, read here purely as a
structural reference (JUCE/CMake setup, APVTS conventions, DSP folder organization, Tests/
approach, BUILDING.md shape). Read `../BRAND.md` first for the cross-plugin design system (naming,
"Program" not "Preset", the one-accent-color-per-plugin rule, component grammar), then this file
for Gatecrasher's own conventions and status. `design/GATECRASHER-GUI-SPEC.md` and `design/CLAUDE.md`
remain the authoritative source for exact GUI pixel/asset detail - this file summarizes and points
at them rather than duplicating their content.

## Commands

Gatecrasher builds on macOS (AU + VST3 + Standalone), Windows (VST3 + Standalone), and Linux
(VST3 + Standalone) — AU is Apple-only. JUCE 8.0.14 is fetched automatically via CMake
`FetchContent` on any platform, no local checkout needed.

Configure once — macOS: `cmake -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64`. Windows:
`cmake -B build -A x64`. Linux (single-config generator, so `CMAKE_BUILD_TYPE` must be set here
rather than only at build time): `cmake -B build -DCMAKE_BUILD_TYPE=Release`. Re-run the configure
step whenever `CMakeLists.txt` changes (new sources, new `juce_add_plugin` args) — a plain rebuild
won't pick those up.

Build: `cmake --build build --config Release`. Run the DSP unit tests (JUCE-`UnitTest`-based,
console app target `GatecrasherTests`): `./build/Tests/GatecrasherTests_artefacts/Release/GatecrasherTests`
(macOS/Linux) or the `.exe` equivalent on Windows.

See [BUILDING.md](BUILDING.md) for full per-platform requirements (Xcode/Visual Studio, CMake
3.24+, pluginval) and validation commands (`auval`/pluginval).

## Prompts log

`prompts/PROMPTS.md` holds numbered work-package prompts. Once a prompt has been fully implemented,
mark it `SHIPPED` with the date it shipped (e.g. `PROMPT #1 - SHIPPED 2026-08-03`) instead of
leaving it as a bare, implicitly in-flight `PROMPT #N`.

## Architecture

### Signal chain (fixed order, all in `PluginProcessor::processBlock`)

```
Input ──┬─────────────────────────────────────────► PreDelay ─► ReverbEngine (active tank)
        │                                                                │
        └─► TriggerDetector ─► GateEnvelopeGenerator ─► gain envelope    ▼
            (main in or sidechain, per Key Source)              │  DampingStage
                                                                 │        │
                                                                 └───► × (gate multiply)
                                                                            │
                                                                     SlamSaturation
                                                                            │
                                                                     StereoWidthStage
                                                                            │
Input (dry tap, captured before PreDelay) ─────────────────────────► OutputMixStage ─► Output
```

The reverb tank runs continuously off the (pre-delayed) input; the gate chops the *wet tail*, not
the input - this is the classic gated-reverb topology (an SSL-style gate feeding an AMS-style
non-linear reverb return) that produces the choked-ambience "In The Air Tonight" effect the factory
bank's `Intruder`/`Air Tomorrow` programs are named after. `TriggerDetector` and
`GateEnvelopeGenerator` run in parallel off the dry input (or the sidechain bus, if `Key Source` is
Sidechain and a sidechain is connected) and never touch the wet signal directly - the gate's output
is a per-sample gain array that `PluginProcessor` multiplies into the wet buffer itself, after
`DampingStage` and before `SlamSaturation`.

`Algorithm` selects which of `RoomTank`/`PlateTank`/`ChamberTank`/`AmbienceTank` is active;
`ReverbEngine` owns all four and crossfades briefly (~60ms) across a switch to avoid a click, the
same click-avoidance concern as TapeRot's MODEL switch. Each tank is a classic Schroeder/Moorer
comb+allpass network (see `CombAllpassNetwork.h`) configured with its own delay-length table and
feedback range - Room's the longest/most spacious, Plate's the densest/brightest, Chamber's between
the two, Ambience deliberately never grows a long tail even at maximum Decay.

`Shape` (Hard/Soft) only affects `GateEnvelopeGenerator`'s release-phase curve - attack and hold are
identical either way. Hard is a fast linear fall; Soft both curves the fall (`pow(1-t, 2.2)`) and
extends its effective duration roughly 5x, matching the GUI spec's scope reference renders (~3
scroll-columns for Hard vs. ~16 for Soft at the same Release setting).

`Decay` and `Density` are real APVTS parameters with no panel control - deliberately automation-only
by design, not an oversight. (An earlier version of the approved GUI spec was also missing a
control for `Shape`; that gap has since been resolved with a dedicated SHAPE switch, reusing the
Key Source switch's component verbatim - see `design/GATECRASHER-GUI-SPEC.md` section 5.)

### Parameters

`Source/Parameters.h` is the single source of truth for parameter IDs (`ParamIDs::*`) and the APVTS
layout (`createGatecrasherParameterLayout()`). `PluginProcessor` caches raw atomic pointers to each
parameter in its constructor via `apvts.getRawParameterValue(...)` and reads them fresh every block
in `processBlock` - don't call `getRawParameterValue` per-block, and don't add a parameter without
adding both the layout entry and the cached pointer. New parameters are appended below the existing
list in `Parameters.h`, never inserted above, to keep saved programs' parameter IDs stable.

Every stored parameter snapshot is called a **Program**, never a "Preset" - in the UI label, the
parameter/method naming (`ProgramManager`, `FactoryProgram`, `.gatecrasherprogram`), and any docs -
matching `../BRAND.md`'s terminology rule.

### Program management (`Source/DSP/ProgramManager.*`, `FactoryPrograms.h`)

A dedicated class rather than living inline on `PluginProcessor` the way TapeRot's does it -
justified by the header's three-state UI (idle+factory / idle+user / naming) and the larger,
17-program factory bank. `FactoryPrograms.h` holds `kFactoryPrograms`, a `constexpr` POD array
(indices `[0, kNumFactoryPrograms)`); user programs occupy `[kNumFactoryPrograms, getNumPrograms())`,
one `.gatecrasherprogram` XML file each, sorted alphabetically by filename, in a per-OS directory
(see `ProgramManager::getUserProgramDirectory()`).

Save (`saveNewUserProgram`) **always** creates a new file and switches to it - there is no overwrite
path and no separate "New Program" action anywhere in the interface. Starting fresh is just loading
any program, tweaking APVTS parameters, and calling Save; Cancel (handled entirely at the GUI layer,
see below) only ever backs out of the naming dialog, never touches the APVTS. `deleteUserProgram`
no-ops for a factory index and falls back to `defaultFactoryProgramIndex` if the deleted program was
the one currently loaded. Program switching is async-safe the same way TapeRot's preset switching
is: a host can call `setCurrentProgram` from a non-message thread, so the real application is
deferred through `ProgramManager`'s own `AsyncUpdater`.

### GUI (`Source/GUI/`)

**Deliberate divergence from TapeRot**: TapeRot's GUI is fully vector/code-drawn (`SectionPanel`,
`TapeRotLookAndFeel`, etc. - see `../taperot/CLAUDE.md`'s GUI section). Gatecrasher's is
asset-based for its *sculpted* elements - the chassis and the knobs - because pre-rendered bitmap
sculpting reads as more authentically "real 80s hardware" than modern vector rendering for this
particular fascia. Everything flat is drawn in code.

The background is `design/assets/gatecrasher-panel-bare@2x.png`: a **bare chassis** - fascia
gradient/grain, rack ears, screws, the header band, and the three section dividers, and nothing
else. No controls, labels, nameplate or window frames. Layered on top, in order:

- `PanelChrome` - the static engraved layer: section headings, group labels, every knob label, the
  switch captions, the nameplate subtitle, the PROGRAM/IN/OUT captions, the recessed LED window
  frames, the input meter's frame, the scope's timebase annotation, the version stamp.
- `PanelReadouts` - the live numeric value under each knob, plus the algorithm selector's four
  corner labels (whose lit one follows the Algorithm parameter).
- The controls themselves: 15 knobs (bitmap filmstrips, `KnobFilmstripComponent`, with code-drawn
  tick rings since those don't rotate with the knob), the gate-envelope scope, the GATE OPEN lamp,
  the input meter's segments, the Key Source/Shape switches, `WordmarkComponent`, and
  `ProgramHeader`'s dynamic text and buttons.

This replaced an earlier arrangement that used the **fully dressed** render
(`gatecrasher-panel@2x.png`) as the background, so every live element sat on top of a baked copy of
itself. That produced a whole family of bugs - a frozen second needle behind every knob, ghosted
switch labels, baked meter segments showing through the gaps between the live ones, IN/OUT windows
stuck on a reading that never moved - each needing its own erase-or-cover workaround, several of
which could only mask the symptom (re-blitting the background over a region whose baked content is
exactly what you are about to redraw is a no-op, not an erase). With a bare chassis none of that is
needed: `GatecrasherTheme::eraseToBackground` is a genuine clear again, used only to wipe a live
element's *previous frame*. The dressed renders remain in `design/assets/` as pixel-matching
acceptance targets but are no longer shipped in BinaryData.

Type sizes are quoted by the spec and the reference mockup as CSS px, which is **not** the same
number as a `juce::Font` height (that is ascent+descent, a typeface-specific multiple of the em
size). `GatecrasherTheme::labelFontHeightForCssPx` / `monoFontHeightForCssPx` convert, calibrating
the ratio off a reference string whose rendered width was measured directly from the dressed
artwork - so one real measurement scales every size on the panel. Passing a spec px value straight
to `labelFont()` renders visibly small.

Fixed reference canvas is 960x434 (not TapeRot's 960x400) - within `../BRAND.md`'s allowance for a
denser control set to scale proportionally rather than match the reference ratio exactly.

`design/GATECRASHER-GUI-SPEC.md` is the authoritative pixel spec (palette, every control coordinate,
the filmstrip frame-mapping formula, the scope's exact draw rules including the Shape-driven
release-curve behavior, the program header's state machine) - read it before touching any GUI code.
`design/CLAUDE.md` is Claude Design's handoff note summarizing the same. Both were written before
implementation began and are meant to be implemented as-is, not redesigned.

### Build system

`CMakeLists.txt` fetches JUCE via `FetchContent` (pinned to `8.0.14`, matching TapeRot so the two
sibling plugins don't drift on JUCE version) and defines one `juce_add_plugin(Gatecrasher ...)`
target, with `FORMATS`/copy-dir branching on `if(APPLE)` the same way TapeRot's does.
`PLUGIN_MANUFACTURER_CODE` (`Gcsh`), `PLUGIN_CODE` (`Gr85`, referencing the design spec's own
"GATECRASHER GR-85" model tagline), `BUNDLE_ID`, and `COMPANY_NAME` are placeholders per
BUILDING.md - treat them as effectively permanent once anything is shipped or automated against.
`Tests/` is a separate `juce_add_console_app` target that compiles the DSP `.cpp` files directly
(not linked against the plugin target) plus its own JUCE-`UnitTest` files - new DSP `.cpp` files
need to be added to both `target_sources(Gatecrasher ...)` here and
`target_sources(GatecrasherTests ...)` in `Tests/CMakeLists.txt` to be covered by tests.

Gatecrasher declares an optional stereo sidechain input bus (for `Trigger Source: Sidechain`) in
addition to the required stereo main input/output - `isBusesLayoutSupported` also accepts the
sidechain bus disabled or mono. This is the one structural difference from TapeRot's bus setup.

`juce_add_binary_data(GatecrasherBinaryData SOURCES ...)` embeds the bare panel chassis, the two
knob filmstrips, and the four fonts (Barlow Condensed SemiBold/Bold, Share Tech Mono, TudorVictors).
It deliberately excludes every *dressed* render - `gatecrasher-panel@2x.png`, its gate-open
counterpart, the three header-state bitmaps, and the two SHAPE-switch scope references
(`scope-hard/soft-release@3x.png`). Those are pixel-matching acceptance targets to check the live
rendering against, not runtime assets; the header-state bitmaps in particular were briefly used at
runtime and turned out to be uncalibrated against the panel's own coordinate frame (see
`ProgramHeader.h`).

Note for asset drops: `design/assets/` is **added to, not replaced**. A wholesale replacement has
already once deleted all 19 tracked font files (breaking the build outright, since BinaryData
couldn't resolve them) and overwritten this file with a copy of `design/CLAUDE.md`; both were
recovered from git.

## Status

- **DSP**: every stage has real, functioning processing - no stubs. The reverb tanks' comb/allpass
  delay-length tables and feedback ranges, and the 17 factory programs' parameter values, are a
  structurally-plausible first pass rather than a tuned one (see `BUILDING.md`'s DSP tuning note
  and `CombAllpassNetwork.h`'s class comment) - the same status TapeRot's own factory presets had
  before their by-ear pass. Build, load, listen, adjust.
- **GUI**: implemented against the approved spec using the bare-chassis approach above. Barlow
  Condensed and Share Tech Mono are embedded and in use throughout. One interim deviation remains:
  section 8 calls for the wordmark to be a pre-baked sprayed-stencil PNG, and no such PNG exists in
  `design/assets/`, so `WordmarkComponent` draws it live from the embedded TudorVictors typeface -
  reproducing the mockup's per-letter rotation/drift/opacity and its spatter flecks, and
  approximating the two-stage overspray halo with stacked offset copies, but *not* the per-glyph
  speckle mask (that needs an offscreen render per letter and reads as noise at 36px). Swapping in a
  real PNG when one arrives is a small change, tracked in `prompts/PROMPTS.md`.
- **Decay and Density** are intentionally automation-only parameters with no panel control - this
  is a settled design decision (Density always was; a gap around Shape having no control was
  separately resolved by adding the SHAPE switch, not by adding one for Decay), not an outstanding
  item.
