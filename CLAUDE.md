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

## RESUME POINT — the header has a measured baseline, and it is what a panel move fails against

**Verified `578fb10` on 2026-08-17: this casting's header draws exactly where its own constants say.**
Not read — captured from the Release standalone and measured off the pixels.

Why it is written here rather than left in a session report: the value of a baseline is entirely in
being read by whoever moves the panel next, and a figure that lives somewhere the mover does not
open is worth nothing.

**What was measured**, band at y 29, height 34, canvas 960 x 434:

| Element | Constants | Measured |
|---|---|---|
| LCD | `programWindowX` 374.75, W 330 -> 374.75..704.75 | 376.0 .. 703.0 |
| SAVE | `saveButtonX` 710.75, W 50.2 -> 710.75..760.95 | 711.0 .. 758.5 |
| DELETE | `deleteButtonX` 765.95 -> 765.95..816.15 | 766.0 .. 814.0 |
| IN well | `inWindowX` 825, W 44 -> 825..869 | 825.0 .. 866.5 |
| OUT well | `outWindowX` 875, W 44 -> 875..919 | 875.0 .. 916.5 |

**What this is NOT.** This casting references `nf::HeaderGeometry` **nowhere**, so it is on its own
canvas and its own layout, and none of the figures above is expected to match the shared part. The
baseline says *internally consistent*, not *conformant*.

**The defect it exists to catch** was found in Chorus-60 on 2026-08-17: that casting's header pass
aliased its LCD to the shared part and left SAVE, DELETE and both meter wells as literals from the
previous canvas — **29 px right and 29 px down** — and nothing could see it, because the plate baked
those faces and the only symptom was text centred inside a box nobody drew. It surfaced the moment
the material had to be painted from those rects.

**AND A BAND FIGURE IS NOT ONLY A RECTANGLE — this failed here on 2026-08-19, in the edit this
paragraph was written to govern.** The canvas went to 1340 × 700 and every rect was aliased in one
edit exactly as instructed. The band's **type** was not: `PROGRAM` stayed at y 21 against a band that
now starts at 61, and both button legends drew **20 px above their caps**, onto bare fascia, in an
ink meant for a dark face. The caption's tracking was also the scope legend's .22 rather than §7's
.24.

**The diff showed a coherent edit and the capture showed the legends in mid-air.** That is this
section's own sentence — *a rect that moves and a rect that does not are indistinguishable in a diff
and obvious in a measurement* — arriving one layer below where it expected to, on the type positioned
**from** the band rather than on the band. The baselines are offsets from `nf::HeaderGeometry::bandY`
now, so what is written down is the property (12.08 and 24.08 into a 34 px cap keeps the pair
optically even) and the absolute is derived.

**Third time a capture has caught what a diff could not, and this casting has far more of it ahead
than any of the five.** The other two were Chorus-60's header literals 29 px out behind a plate that
baked their faces, and its nameplate compared against a prototype rendering in a fallback. All three
share a shape: **the edit was internally consistent and the panel was wrong**, so nothing that reads
source can see it. Gatecrasher is going from one plate blit to **280 drawn objects** — every label,
tick, numeral, divider, rail and the wordmark — which is the largest surface in the suite for a
consistent-looking edit to be wrong on.

**So capture after every step of the rewrite, not at the end of it.** `tools/capture_panel.py`,
Release bundle named explicitly, and kill any running instance first — it will otherwise reuse one
and measure a build you did not just compile, which it did once during this pass.

**So when this casting moves: alias every band figure in one edit, then re-measure against the table
above.** A rect that moves and a rect that does not are indistinguishable in a diff and obvious in a
measurement. And note that **a literal which happens to agree with core is indistinguishable from an
alias by reading** — Reflect-84 held four such literals, one of them 2 px off §4's shared descriptor
anchor, in the casting whose editor had been declared conformant.

---

## THE PANEL REWRITE'S ENUMERATION — 280 objects, DERIVED, and the list is a command

**This casting goes from one plate blit to an entirely code-drawn panel**, so every label, tick,
numeral, divider, rail, well and the wordmark stops arriving as pixels and becomes a draw call.
`GatecrasherPanelBackground::paint` is one `drawImage` today; §0 of the spec says *"no plate, no
filmstrips, no bitmap of any panel element"*, and one binary ships (`fonts/TudorVictors.ttf`).

**The list is not written down here, and that is the point.** Root `CLAUDE.md` records three
hand-authored enumerations that each read as complete and each could not contain a whole category —
HEADER-PART §10's dependant table (six design artefacts, no builds), the merge check's reverse arm
(phrased over the merged tree, so a file that never arrived was unreachable), and **Chorus-60's plate
enumeration, eleven rows, every one of them ink, thirteen rows of material missing.** None was a
failure of care. The boundary is the room the author was standing in, and Chorus-60 had far less
baked than this casting does.

So the enumeration is regenerated rather than maintained:

    python3 ../tools/enumerate_prototype.py "design/Gatecrasher GR-85 Panel.dc.html" --canvas 1340x700

**The census at bundle 3, and it is what the rewrite owes:**

| | |
|---|---|
| **280 objects to draw** | 156 material + 126 ink, **2 of them both** |
| Material | 73 knob ticks · 15 sweep arcs · 15 bodies · 15 pointers · 13 ladder segments · 4 screws · 4 shoe halves · 3 dividers · 2 rails · 2 button caps · 2 meter wells · 2 shoe frames · header block · LCD well · lamp lens · scope well · scope grid · scope plot rule |
| Ink | 120 strings · 4 vector (LCD chevron, scope trace) · the 2 meter values, which are also material |
| Type roles | **18**, against §9's **thirteen** rows — the collapsing is deliberate where it remains |

**The 2 that are both are the finding in miniature.** The first classifier asked `"ink" if text else
"material"` and dropped **both meter wells** out of the material list entirely — they are a well that
holds a value, and a branch can only file them once. That is the same construction as root
`CLAUDE.md`'s *"Wordmark — stays baked, it is the CHORUS badge"* row, true about the badge and false
about the nameplate. Classify on two independent flags, never on a branch.

**Three things the derivation found that reading §9 would not — ALL THREE ANSWERED in export 6, the
same day they were raised.** Kept with their answers because what the enumeration was able to see is
the reusable part:

1. **The meter value carried no tracking in the prototype** where §9 paired it with the LCD at
   `.10 em`, and `HEADER-PART.md` §7 — which governs the header — stated the face and the size and
   was **silent on the tracking**. Two documents disagreeing and the one that governs not mentioning
   it. **§7 now states `17 px / line box 22 / .10 em`** and rules explicitly that *"a prototype
   drawing the wells with tracking unset differs from the part and the part governs."* Build it at
   `.10 em`, and read the figure from §7 rather than from §9 — the wells are the shared part's.
2. **`ENVELOPE 50 ms / DIV` had no row in §9 at all.** It draws at 10 / 13, **.22 em**, `#2b3034` —
   outside the *"Scope legend · .14–.20 em · `#9aa1a6`"* row in both tracking and ink, so a role of
   its own rather than that role at a variant setting. §9 carries it now and no pixel moved for it.
3. **Four codepoints above ASCII, and one of them was invisible.** U+2212 MINUS (×8), U+00B7 MIDDLE
   DOT, U+221E INFINITY — and **U+3000 IDEOGRAPHIC SPACE**, inside that same scope-header string.
   The first three stand and every one must be built from its codepoint: `juce::String`'s
   `const char*` constructor decodes **Latin-1, not UTF-8**. **The U+3000 was a stray paste and is
   now a normal space**, on a better argument than the ask made — *an invisible character that
   changes metrics has no place in a source anyone transcribes from, and a gap worth setting is set
   in the tracking or the box.*

   **It is the one no person enumerates**, and it is why the tool has a whitespace arm separate from
   its glyph arm: a list of special characters is a list of things you can see. U+00A0, U+2009,
   U+200B and U+FEFF have the identical property — they transcribe to `' '` and the result looks
   correct. Swept across all six panels; this was the only one in the suite, and there is none now.

**What the current build draws at runtime, measured rather than recalled**, by probing the old
960 × 434 prototype (`design/reference/Gatecrasher.dc.html`) against the plate's own pixels — every
string whose box holds no ink is one the code supplies: the LCD's bank tag and name, the `PROGRAM`
caption, both meter values, and **the eight state labels** (INTERNAL · SIDECHAIN · HARD · SOFT ·
ROOM · PLATE · AMBI · CHMBR). Everything else on the panel is baked.

**`StateLabels` is deleted by this round rather than ported.** It indicates state by *font weight* —
700 selected against 400 unselected — and §6 and §8.3 both forbid exactly that: *"legends are printed
once, under their own half, and never re-inked or moved"*, and §8.1's *"weight is 600 in every cell —
ink weight never stands in for illumination."* The eight become static printed ink and the shoe and
the pointer carry the state. Its 8 labels are inside the 120 above.

**The control set does not change: 15 knobs before and 15 after**, `Layout::knobs` against the
prototype's `knobSpec()`. What changes is every diameter (seven classes → **Ø76 / Ø56**), every
position, and that the ring is drawn instead of printed.

### Probing a prototype: the trap that makes a fallback look like a finding

**A prototype whose `@font-face` points somewhere the render cannot reach falls back in silence**,
and the resulting layout is confident, precise and wrong. `design/reference/Gatecrasher.dc.html`
sources its wordmark from `uploads/TudorVictors-lgxzD.ttf`, which no delivered bundle carries.
Probed without it, the eleven letter spans lay out on Barlow's metrics and the **final `R` lands
clear of the plate's ink** — which reads exactly like a missing glyph, and was written down as one
before it was checked. The plate's wordmark spans canvas x 41.0 → 268.5 and reads GATECRASHER.

The check is the one this repo uses everywhere: **render twice, once with the font source removed,
and show that boxes move.** The current panel prototype moves **11 of 388** boxes without `fonts/`,
which is what proves TudorVictors resolved. The tool does this arm on every run and refuses to let
the two states read alike — *no local face declared* and *declared and did not resolve* are different
claims, and only the second invalidates a measurement.

---

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
build.

**The caps re-authoring in `Parameters.h` IS done here.** This paragraph said it was still
outstanding, and that was stale rather than wrong when written: the deletion of `ValueCase` really
was a compile fix on the day, and the re-authoring landed afterwards without the sentence being
revisited. Every parameter name is authored in caps, `Tests/ReadoutConformanceTests.cpp:127` asserts
it off `getName()` rather than off the literal, and this casting additionally asserts the unitless,
digit-free values. The suite is green, so the assertion is live rather than merely present.

Corrected 2026-08-17 while scoping the harmonisation rewrite, and recorded in place rather than
deleted: this is the suite's most expensive documented failure — a plugin's `CLAUDE.md` outliving
the code it describes, which gets trusted and reproduces the original wrong conclusion. See the root
`../CLAUDE.md` under "Case belongs at the source".

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
