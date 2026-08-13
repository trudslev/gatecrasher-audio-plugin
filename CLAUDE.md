# CLAUDE.md

This file provides guidance to Claude Code when working with code in this repository.

Gatecrasher is its own independent repo and does not depend on `../taperot/` at runtime - it's a
sibling casting under the shared [Neon Foundry](../BRAND.md) umbrella, read here purely as a
structural reference (JUCE/CMake setup, APVTS conventions, DSP folder organization, Tests/
approach, BUILDING.md shape). Read `../BRAND.md` first for the cross-plugin design system (naming,
"Program" not "Preset", the one-accent-color-per-plugin rule, component grammar), then this file
for Gatecrasher's own conventions and status. `design/GUI-SPEC.md` remains the authoritative source
for exact GUI pixel/asset detail - this file summarizes and points at it rather than duplicating its
content. It is one document now: the bundle used to carry a second `design/CLAUDE.md` summarising
it, and that has been dropped, because a bundled copy of a repo-owned filename reads as authority
while being a snapshot of whatever the designers held when they cut the zip.

## Commands

Gatecrasher builds on macOS (AU + VST3 + Standalone), Windows (VST3 + Standalone), and Linux
(VST3 + Standalone) — AU is Apple-only. JUCE 8.0.14 is fetched automatically via CMake
`FetchContent` on any platform, no local checkout needed.

Configure once — macOS: `cmake -B build -G Xcode`. Windows:
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
Key Source switch's component verbatim - see `design/GUI-SPEC.md` section 5.)

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

**The LCD parameter takeover is `nf::describeParameter`**, reverting **900 ms** after release —
`nf::ReadoutFormat::revertMs`, where this panel carried 800. This panel set
`ValueCase::wordsOnly` until 2026-08-13, when the designers ruled that **case belongs at the source,
never at a display site**. Section 6.3's `ALGORITHM: PLATE` is still the target, but PLATE is
authored that way in `Parameters.h` so the LCD and the host's automation lane print one string.
`ValueCase` is deleted from core, and its line was removed from `GatecrasherTheme.h` to restore the
build. **That deletion is a compile fix, not the ruling: the caps re-authoring in `Parameters.h` is
still outstanding here**, so the readout currently prints names and choice values as authored. See
the root `../CLAUDE.md` under "Case belongs at the source".

`GatecrasherTheme::readoutFormat()` holds that, **not `ProgramHeader`**, and the placement is
load-bearing: `ProgramHeader.h` reaches `PluginProcessor.h`, which needs `JucePlugin_*` macros that
exist only in the plugin target, so a test reading the format from there cannot link. The test must
read the shipping format rather than a copy, or it asserts against itself.

`ReadoutConformanceTests` sweeps every parameter across its range and fails the build on a value
that would print badly. It confirms the `fixed(0)` question the suite audit left open: the
whole-percent formatter uses `roundToInt`, and nothing in the layout prints at JUCE's seven-place
default.

**The bank on disk, the dirty flag and Program identity all come from `neon-foundry-core`**, pinned
at `v1.0.0` and declared *after* `FetchContent_MakeAvailable(JUCE)` — core links `juce::juce_core`
and refuses to fetch its own, and two JUCE trees in one build link two `juce_core` builds into one
binary. It is linked into both `Gatecrasher` and `GatecrasherTests`, because `ProgramManager.cpp` is
compiled into both.

`nf::UserProgramStore` owns scanning, sort-by-stem, naming, the collision check, save and delete;
`nf::ParameterSnapshot` owns the dirty baseline; `nf::ProgramId` owns identity. **What a Program
*contains* stays here** — the whole APVTS state plus the schema version. Core owns files and names;
this repo owns meaning.

Two things changed with the move. The empty-name fallback is **`TAKE n`**, not `NEW PROGRAM` — six
castings had five different fallbacks, and consecutive empty saves now give `TAKE 3`, `TAKE 4`
rather than leaning on `getNonexistentSibling` for `NEW PROGRAM (2)`. And the dirty baseline is
**keyed by parameter ID and guarded by a `SpinLock`**, where it was a positional
`std::vector<float>` with neither. The old comment claimed every writer ran on the message thread;
`setStateInformation` carries no such guarantee from JUCE, and the GUI polls the flag while it runs.
TapeRot and Elmer had both spotted that and guarded their own copies — this casting had not.

### GUI (`Source/GUI/`)

**Deliberate divergence from TapeRot**: TapeRot's GUI is fully vector/code-drawn (`SectionPanel`,
`TapeRotLookAndFeel`, etc. - see `../taperot/CLAUDE.md`'s GUI section). Gatecrasher's is
asset-based, because pre-rendered bitmap sculpting reads as more authentically "real 80s hardware"
than modern vector rendering for this particular fascia.

Since Rev 6 the background is `design/assets/gatecrasher-panel-plate@2x.png`: a **fully printed
plate** carrying every static element - fascia, grain, rails, screws, header band, dividers,
wordmark, every label, every printed scale, numeral and tick, all recessed wells, and the footer.
Only nine things are drawn at runtime, and spec section 0.1 lists them in draw order:

1. Switch shoes (`ToggleSwitchComponent`)
2. The eight state-dependent labels (`StateLabels`)
3. Knob filmstrip frames (`KnobFilmstripComponent`) - the whole 160 px frame, not just the cap
4. Input meter **lit segments only** (`InputMeter`) - the well and unlit ledger are baked
5. The gate-envelope scope's contents (`GateScope`)
6. The GATE OPEN lamp (`GateLamp`)
7. LCD caption, tag, name/live value, IN, OUT (`ProgramHeader`)
8. SAVE / DELETE (`ProgramHeader`)

The LCD caption joined that list in Rev 8, for the same reason as the section-0.4 labels: it reads
`PROGRAM` normally and `NAME PROGRAM` during name entry, so a baked copy would have to be painted
over to change. Bare fascia sits at its slot and section 6.2 carries its geometry.

**Do not reintroduce a drawing layer for anything else.** Rev 5 drew 20+ static strings, all 14 knob
labels and its own tick rings; that code is deleted, not adapted, and the spec is explicit that
running both paths double-draws every label at a one-pixel offset. The tick rings in particular were
*wrong*, not merely redundant: they were spaced at even angles (15 deg / 21 deg), and Rev 6 bakes
every tick at its **labelled value**, which on the four skewed controls is not evenly spaced.

The one exception is section 0.4's eight state-dependent labels - `INTERNAL`/`SIDECHAIN`,
`HARD`/`SOFT` and the four algorithm corners - which are drawn because their weight and colour
follow a control. Rev 6 tried baking them at their defaults and asking the build to redraw only the
changed pair; that cannot work, because baked pixels cannot be un-drawn. Rev 7 removed them from the
plate, so bare fascia sits where they go and `StateLabels` paints all eight every frame. Their
dimming is 5.52:1 against a 7:1 bar and is documented as deliberate in section 2.1 - **do not
"fix" it.**

**Knob frame box vs cap.** Section 1.3 states the cap-to-frame ratio as a contract: the cap is 0.75
of the frame (120 px of cap in a 160 px frame), so `knobBoundingBoxBleed` is **1.333** and each strip
is blitted into a box that size, centred on section 3's centre. Section 3's diameters are the **cap**
and must never be adjusted to compensate for the margin. Rev 8 and earlier shipped 128 px frames
around the same 120 px cap, a ratio of 1.07, and at that ratio the frames clipped their own cast
shadow - border alpha was still 88 top / 95 bottom / 38 sides, so every knob sat inside a hard-edged
dark rectangle. Rev 9 re-rendered both strips at 160 px purely to give the shadow room; the fix was
in the asset, and there is nothing to compensate for in code.

Because that box now reaches .167 of the diameter past the cap - 10 px of transparent margin lying
over printed scale numerals on THRESHOLD - `KnobFilmstripComponent::hitTest` claims the **cap**
rather than the component's bounds. Without it, clicking the printed `-60` would grab the knob.

`GatecrasherTheme::eraseToBackground` re-blits the plate to clear a live element's previous frame.
Against the printed plate that is a genuine clear, and it is what `InputMeter`, `GateLamp`,
`ToggleSwitchComponent` and `ProgramHeader`'s buttons use. It was NOT one under Rev 5's briefly-used
*dressed* background, where re-blitting restored a baked copy of the very thing being redrawn - if
you find a comment or a flat-fill workaround that assumes that, it predates Rev 6.

Type sizes are quoted by the spec as CSS px, which is **not** the same number as a `juce::Font`
height (that is ascent+descent, a typeface-specific multiple of the em size).
`GatecrasherTheme::labelFontHeightForCssPx` / `monoFontHeightForCssPx` convert, calibrating the ratio
off a reference string whose rendered width was measured from the artwork. Pass a spec px value
straight to `labelFont()` and it renders visibly small. Letter-spacing is likewise absolute pixels
that `juce::Font` has no setting for - `drawTrackedText` draws glyph-by-glyph to reproduce it, and
the LCD's own 8.32px-per-character budget (section 6.1) only holds with the .10em tracking applied.

Fixed reference canvas is 960x434 (not TapeRot's 960x400) - within `../BRAND.md`'s allowance for a
denser control set to scale proportionally rather than match the reference ratio exactly.

**Program header.** `ProgramHeader` spans the whole canvas and narrows its `hitTest` to the program
window plus SAVE/DELETE; a canvas-sized component that intercepted everything would swallow the
knobs' clicks. Clicking anywhere in the program *window* - not just the name cell - opens the
dropdown (section 6.2: the baked chevron is an affordance, not a button). The menu is dressed by
`GatecrasherMenuLookAndFeel` as an extension of the LCD glass and opens at the window's own width.
The name cell shows `NN NAME` with a 1-based two-digit index, uppercased, plus a trailing ` *` while
the program is dirty.

While a control is being dragged the same cell shows `NAME: value unit` instead, reverting ~800 ms
after the gesture (section 6.3) - there is no drag popup and no standing readout anywhere on the
fascia. `GatecrasherEditorContent` guards those calls on the knob's **own** drag state:
a `SliderAttachment` also fires when a program is applied and on every host automation step, and
without the guard the display latches onto whichever parameter was written last and flickers for the
length of a song.

That live text comes straight from each parameter's `getText`, so the LCD and the host always agree.
It only reads properly because `Parameters.h` gives every float parameter an explicit
`withStringFromValueFunction`: `withLabel()` feeds `getLabel()` alone, and a parameter left at
JUCE's default renders `-16.3999977` rather than `-16.4`. Add a float parameter without a formatter
and it will show up seven decimals wide in the LCD, in the automation lane and in any generic editor.

**The header band is 34px at y 29, and Gatecrasher is where that figure was hardest to reach.** The
LCD, both Program buttons and both meter windows share one height and one baseline. This casting had
the shallowest band in the suite at 25px, which made it the one place two 10px legends provably
would not fit — 20px of ink before any leading or padding, inside a 25px button. The band grew
rather than the legends shrinking, because 10px is BRAND.md's floor for functional text and both
legends are functional.

Rev 15 re-exported the plate for it: the LCD, IN and OUT wells run **y 29..63** where they used to
run 34..57, and only the header band (y 0..77) was re-rendered, so nothing below it could have
drifted. **A plate from before Rev 15 leaves 5px of stale dark well above the live LCD and 6px
below** — measured and confirmed against the shipped asset, along with the button positions now
being bare fascia.

**The two Program caps are dark, and they are the only two controls on this panel that are.** It
follows from the legend being the lamp: on a pale fascia a bright legend has nowhere brighter to go,
so lit type could not read as lit. They read as a pair with the LCD and the meter windows — five
dark apertures across one band.

**There is no disabled face and no relabelling.** Cap, border, highlight and drop are identical in
all five panel states; only which of the two printed legends is backlit changes. The pale enabled
cap, the separate disabled cap and the `#55595C` disabled label are all gone — and that label had
itself only just been rescued from `#8B9297` at .55 alpha, which measured **1.21:1**, the worst
reading in the suite. The right answer turned out not to be a better disabled colour but no
disabled state at all.

`design/GUI-SPEC.md` is the authoritative pixel spec and is meant to be implemented
as-is, not redesigned. When a coordinate here and a coordinate there disagree, the spec wins - and measure the artwork to confirm
before assuming either. Two long-lived bugs came from not doing that: the lamp sat at (224, 95)
instead of section 5.5's (216, 104), masked for months by a baked bulb at the correct spot, and the
input meter kept Rev 5's (147, 133) instead of section 8's (165, 139), painting a second column of
segments straight over the printed scale numeral while the real well sat empty beside it.

### Build system

`CMakeLists.txt` fetches JUCE via `FetchContent` (pinned to `8.0.14`, matching TapeRot so the two
sibling plugins don't drift on JUCE version) and defines one `juce_add_plugin(Gatecrasher ...)`
target, with `FORMATS`/copy-dir branching on `if(APPLE)` the same way TapeRot's does.
`PLUGIN_MANUFACTURER_CODE` (`Nfdy`), `PLUGIN_CODE` (`Gr85`, referencing the design spec's own
"GATECRASHER GR-85" model tagline), `BUNDLE_ID` (`com.neonfoundry.gatecrasher`) and `COMPANY_NAME`
("Neon Foundry") are settled, not placeholders - the vendor identity was unified across the
suite before any versioned release, and changing it again breaks saved projects in both AU
and VST3 (JUCE derives the VST3 class ID from the manufacturer and plugin codes together).
`Tests/` is a separate `juce_add_console_app` target that compiles the DSP `.cpp` files directly
(not linked against the plugin target) plus its own JUCE-`UnitTest` files - new DSP `.cpp` files
need to be added to both `target_sources(Gatecrasher ...)` here and
`target_sources(GatecrasherTests ...)` in `Tests/CMakeLists.txt` to be covered by tests.

Gatecrasher declares an optional stereo sidechain input bus (for `Trigger Source: Sidechain`) in
addition to the required stereo main input/output - `isBusesLayoutSupported` also accepts the
sidechain bus disabled or mono. This is the one structural difference from TapeRot's bus setup.

`juce_add_binary_data(GatecrasherBinaryData SOURCES ...)` embeds the printed plate, the two knob
filmstrips, and four fonts: **all three** Barlow Condensed weights and Share Tech Mono. The three
weights are one more than `design/assets/fonts/README.md` implies - it offers SemiBold (600) only for
a group caption, but section 6.4 also puts the SAVE/DELETE labels at 600 and those are drawn live.
Bold (700) and Regular (400) are the selected/unselected halves of the section-0.4 labels.

It deliberately excludes every *dressed* render - `gatecrasher-panel@2x.png`, its gate-open
counterpart, the three header-state bitmaps, and the two SHAPE-switch scope references
(`scope-hard/soft-release@3x.png`). Those are pixel-matching acceptance targets to check the live
rendering against, not runtime assets; the header-state bitmaps in particular were briefly used at
runtime and turned out to be uncalibrated against the panel's own coordinate frame (see
`ProgramHeader.h`). `gatecrasher-panel-bare@2x.png` is likewise no longer a build input, and
`TudorVictors.ttf` ships only so the wordmark can be re-baked - it is not a runtime face.

Note for asset drops: `design/assets/` is **added to, not replaced**. A wholesale replacement has
already once deleted all 19 tracked font files (breaking the build outright, since BinaryData
couldn't resolve them) and overwritten this file with a copy of the `design/CLAUDE.md` the bundles
carried at the time; both were recovered from git. Bundles no longer ship that file — the second
half of that failure cannot recur — but the font half still can, and did again on 2026-08-11:
**four of the six bundles now ship no font binaries at all**, so extracting one *as* `design/`
deletes the build's type without a word.

### The Program list's group caption

**Sized from its own type plus padding, never derived from the row height.** The construction is
`nf::captionHeight (font, topPadding, bottomPadding)` — 3px above and 4px below, the suite's adopted
default — and it comes out **18px** here, from a nominal 11px built from a JUCE height rather than through `withPointHeight`.

**The construction is the rule, not the number.** Writing 18 as a literal would break silently at
the first change of font, size or font construction, which is a change nobody would think to check a
caption against. It is also how this caption came to inherit JUCE's `rowHeight + rowHeight / 2` in
the first place — a caption half again *taller* than a row, which is a menu convention rather than a
panel one.

**The 18 is not a divergence to correct.** Predicting 19 for all four castings and measuring 18
here is what surfaced the type-scale finding: this casting owns a `monoFontHeightForCssPx`
converter and the menu type bypasses it, so the same nominal constant renders smaller. That is a
question about the whole type scale, recorded in the root `CLAUDE.md`, not about captions.

## Status

- **DSP**: every stage has real, functioning processing - no stubs. `auval` and `pluginval
  --strictness-level 8` both pass on AU and VST3. The reverb tanks' comb/allpass
  delay-length tables and feedback ranges, and the 17 factory programs' parameter values, are a
  structurally-plausible first pass rather than a tuned one (see `BUILDING.md`'s DSP tuning note
  and `CombAllpassNetwork.h`'s class comment) - the same status TapeRot's own factory presets had
  before their by-ear pass. Build, load, listen, adjust.
- **GUI**: conformant to Rev 7 of the spec and verified against it - the composite was captured from
  the Standalone and diffed against `gatecrasher-panel@2x.png`, and every region the build paints
  over the plate was accounted for against section 0.1's list (no double-draws, nothing straying onto
  baked furniture). The wordmark is baked now, so the old live-drawn `WordmarkComponent` deviation is
  closed, and Rev 8 un-baked the LCD caption so the section-6.4 caption swap works.
  Rev 9 closed both items PROMPT #4 raised: the filmstrips were re-rendered at 160 px so the shadow
  fades to zero (border alpha now <= 1 at every frame) instead of being clipped, and the knurled rim
  is confirmed intended.
- **The dressed renders are known-stale in one respect.** `gatecrasher-panel@2x.png` and its
  gate-open counterpart show a *smooth* large knob because the capture that produced them dropped the
  rim layer; the shipping filmstrip's knurled skirt is correct (section 1.2). Do not chase that
  difference when diffing a composite against them.
- **Not yet verified**: the gate-open composite (`gatecrasher-panel-gate-open@2x.png`) needs the gate
  actually open, which means audio through the Standalone - it has not been diffed. Everything else
  in the spec's QA list has.
- **Decay and Density** are intentionally automation-only parameters with no panel control - this
  is a settled design decision (Density always was; a gap around Shape having no control was
  separately resolved by adding the SHAPE switch, not by adding one for Decay), not an outstanding
  item.
