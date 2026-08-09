# GATECRASHER GR-85 — GUI Implementation Spec

**Revision 9.** Panel: **960 × 434 px** at 1× (fixed aspect).
All coordinates are panel-local, origin = top-left of the 960 × 434 panel.

This revision is a **re-export of what changed**, not a new package. Read §0 first — it changes
the shape of the implementation, and Rev 5 got it wrong.

---

# 0. Baked vs. runtime — read this first

## 0.1 Correction to Rev 5

**Rev 5 had the build drawing every label in code.** It listed hex values, type sizes, letter
spacing and per-element x/y for section headings, control labels and switch labels, and it shipped
a *bare chassis* plate (`-bare@2x`) with no text on it, which only makes sense if the build renders
the text itself. That was the wrong call and it is superseded here — but note the wording: **it is
superseded, not merely updated.** If any of the Rev 5 label-drawing code has already been written,
delete it rather than adapting it. Running both paths will double-draw every label at a one-pixel
offset.

The new default is: **all static panel furniture is baked into a single bitmap.** The build
composites moving parts on top of it and draws no text except inside the two LCD windows.

The reason is the printed scales. Their numerals sit on irregular radii at irregular angles (§4),
and four knobs are power-law skewed, so re-deriving those positions in code is both fiddly and
silently driftable — a rounding difference of half a degree puts `800 Hz` visibly off its tick with
nothing to catch it. Baking makes the exported bitmap the single source of truth.

## 0.2 Text manifest

Every text element on the panel. **B** = baked into `gatecrasher-panel-plate`. **R** = drawn by the
build at runtime.

| Element | | Notes |
|---|---|---|
| Wordmark GATECRASHER | **B** | Sprayed stencil, per-letter jitter — never reproducible in code |
| Strapline GATED AMBIENCE PROCESSOR | **B** | Now sits under the wordmark (§6) |
| Model line MODEL GR‑85 · STEREO | **B** | |
| Section headings INPUT / TRIGGER, REVERB TANK, OUTPUT | **B** | |
| GATE OPEN legend | **B** | Permanent legend, never dims — the lamp carries state |
| Scope header ENVELOPE, 50 ms / DIV | **B** | |
| Scope legend GATE ENV, 0 dB, -∞ | **R** | Drawn with the scope, in its reserved strips (§5.1) |
| Control labels THRESHOLD, HP, LP, ATTACK, HOLD, RELEASE, SIZE, PRE-DLY, HF, LF, SLAM, WIDTH, MIX, TRIM | **B** | |
| Group labels TRIGGER FILTER, KEY SOURCE, SHAPE, TANK DAMPING | **B** | |
| Printed scale numerals (all 14 knobs) | **B** | The whole point of baking |
| Unit markings dB, Hz, ms, % | **B** | |
| Algorithm labels ROOM, PLATE, AMBI, CHMBR | **R** | State-dependent — absent from the plate (§0.4) |
| Switch labels INTERNAL, SIDECHAIN, HARD, SOFT | **R** | State-dependent — absent from the plate (§0.4) |
| LCD caption PROGRAM / NAME PROGRAM | **R** | State-dependent — absent from the plate (§6.2) |
| LCD captions IN, OUT | **B** | |
| LCD tag FACT / USER | **R** | |
| LCD program name / live value | **R** | |
| LCD chevron ˅ | **B** | Static affordance, not a button |
| IN / OUT meter values | **R** | |
| SAVE / DELETE button labels | **R** | Face and label both change with state |
| Footer v1.0 | **B** | |

**No other text exists on the panel.** In particular there are no standing value readouts under the
knobs — see §5.

**The sorting rule is "does this change?", not "is this a label?".** Everything baked is
state-independent. The printed scales must be baked because their numerals sit on irregular radii at
irregular angles and re-deriving them in code is exactly the drift baking exists to prevent. The
eight state-dependent labels are the opposite case — six short centred strings whose weight and
colour follow a control — and they are drawn live. Apply that test, not the category, to anything
added later.

## 0.3 Tick ring manifest

| Ring | | Notes |
|---|---|---|
| Printed scale ticks, all 14 knobs | **B** | At labelled values, not even angles (§4) |
| 270° sweep arc, all 14 knobs | **B** | Hairline, radius Ø/2+6 |
| Algorithm selector 4-detent ring | **B** | The only ring that *is* a detent ring |
| Knob pointer / index line | **R** | Part of the filmstrip frame, not a separate draw |

## 0.4 The eight state-dependent labels

Switch labels and algorithm labels are **absent from the plate** — bare fascia sits where they go.
The build draws all eight, every frame, at both states.

Earlier revisions baked them at their default weights and had the build redraw the pair on change.
That does not work: baked pixels cannot be un-drawn, so turning a bold baked word dim would mean
painting fascia over it first — six hard-coded donor offsets tied to one export, covering artwork
that exists only to be covered, failing silently as a smear the moment a label moves. Leaving them
off removes the mechanism entirely.

| Label | x | baseline y | centre x |
|---|---|---|---|
| INTERNAL | 64 | 416 | 85.7 |
| SIDECHAIN | 117 | 416 | 139.7 |
| HARD | 333 | 422 | 345.0 |
| SOFT | 367 | 422 | 377.0 |
| ROOM | 557 | 127 | 568.4 |
| PLATE | 680 | 127 | 693.3 |
| AMBI | 557 | 185 | 567.0 |
| CHMBR | 679 | 185 | 692.8 |

Barlow Condensed 10 px, tracking .10em. Selected **700** `#16191C`; unselected **400** `#2B3034`.
The widths above are the rendered defaults — draw from the left x, not by re-centring, so a weight
change doesn't shift the word. The §2.1 dimming is deliberate; do not "fix" it to match the
selected colour.

These eight are the only text the build draws outside the LCD region. One more live string sits
just above it — the program caption, which swaps to `NAME PROGRAM` in name entry; its geometry is
in §6.2 and it follows the same draw-from-the-left rule.

## 0.5 Composite order

```
1  gatecrasher-panel-plate@Nx.png        full-panel blit at 0,0
2  switch shoes            × 2           §7
3  switch + algorithm labels             §0.4, all eight, every frame
4  knob filmstrip frames   × 14          §3, alpha-blended
5  input meter lit segments              §8
6  gate envelope scope                   §5, dark rect at 210,120; trace clipped to plot region
7  GATE OPEN lamp                        §5
8  LCD caption, tag, name/value, IN, OUT §6
9  SAVE / DELETE buttons                 §6
```

---

# 1. Assets

## 1.1 Re-exported this revision

| File | Size | Placement | What it is |
|---|---|---|---|
| `gatecrasher-panel-plate@1x.png` | 960 × 434 | 0,0 | **The build's background.** All static furniture: fascia, grain, rails, screws, header band, dividers, wordmark, every label **except the eight in §0.4 and the LCD caption (§6.2)**, every printed scale and tick, all recessed wells, footer. No knobs, pointers, lamp, LCD glyphs, meter fill, scope contents or buttons. |
| `gatecrasher-panel-plate@2x.png` | 1920 × 868 | 0,0 | Same, retina |
| `gatecrasher-panel@1x.png` | 960 × 434 | — | Reference only: default state, fully dressed, gate closed |
| `gatecrasher-panel@2x.png` | 1920 × 868 | — | Same, retina |
| `gatecrasher-panel-gate-open@1x.png` | 960 × 434 | — | Reference only: lamp lit, trace at plateau |
| `gatecrasher-panel-gate-open@2x.png` | 1920 × 868 | — | Same, retina |

The three `-panel` / `-gate-open` files are **visual targets for QA**, not build inputs. Composite
your own output and diff against them.

**Also removed from the plate this revision: the GATE OPEN lamp.** Earlier plates baked a flat
`#3A1512` disc at (216, 104), Ø 15 — the unlit fill. The build already draws the whole lamp (unlit
fill, inset highlight, open gradient, glow) at composite step 7, so the baked copy was a second
draw of a live element and is gone. Nothing sits under the lamp now but fascia; §5.5 is corrected
to match.

**Also removed from the plate this revision: the LCD caption `PROGRAM`.** It changes to
`NAME PROGRAM` during name entry (§6.4), which makes it state-dependent, and a baked copy would
have to be painted over to change — the technique §0.4 withdrew. The build now draws it every
frame; bare fascia sits at the caption slot. Geometry in §6.2.

**Both knob filmstrips were re-rendered this revision** — new files, new frame size, same 128 frames
and same frame 0 = −135° ordering. See §1.3 for the ratio change and why. The knurled rim on the
large strip is correct and intended; earlier dressed renders showed a smooth cap because the capture
that produced them dropped the rim layer, not because the design changed.

**When a revision drops a baked element the build also draws, expect a coordinate bug to surface.**
A baked copy at the spec position hides a drawn copy at the wrong position — the screen looks right
because the artwork is right, and the code's offset only appears when the artwork goes away. That
is what happened with the Rev 7 lamp (drawn 8 px right and 9 px high of §5.5) and with TapeRot's
knob centres before that. **This revision's removal to re-check is the LCD caption at (374, 27.75).** Any future removal of this kind gets called out in this
section by name, so the build knows which coordinates to re-check first.

Every knob angle in those renders is derived from the §9 defaults as
`n = ((default − min)/(max − min)) ^ skew`, so a build that initialises from the parameter table
lands on the same pointer positions. If a diff shows a pointer off by a few degrees, the parameter
table is the contract — trust §9 over the bitmap and tell us, because it means the render drifted.

## 1.2 Unchanged — do not replace

| File | Why it's unchanged |
|---|---|
| `gatecrasher-panel-bare@2x.png` | Chassis-only plate. Superseded as a build input by `-plate`, but kept: useful for mocking new layouts |
| `icon/gatecrasher-icon-{1024,512,256,128,64,32,16}.png` | Icon did not change |
| `TudorVictors.ttf` | Wordmark face. **Not a runtime font** — needed only to re-bake the wordmark (§2.4) |
| Scope rendering | Same algorithm, same colours, same HARD/SOFT curves |
| Switch shoe / button geometry | Same sizes and gradients as Rev 5 |

## 1.3 Knob filmstrips

`knob_large_160px_128f.png` — 160 × 20480, 128 frames, knurled skirt.
`knob_small_160px_128f.png` — 160 × 20480, 128 frames, plain skirt.

**Cap-to-frame ratio: the cap is 0.75 of the frame** — 120 px of cap in a 160 px frame, cap centred.
The build's single constant is therefore **frame box = 1.333 × Ø** (was 1.07). For the 62 px
THRESHOLD cap the frame draws into 82.7 px centred on the same centre; §3's diameters are unchanged
and unchanged is the point — only the transparent margin around them grew.

**Why it changed.** The 128 px frames clipped their own cast shadow: alpha at the frame border was
still 88 top / 95 bottom / 38 sides, so the shadow was ~36 % opaque where the image ended and the
panel showed a hard-edged dark rectangle around every knob. The shadow needs .145 of the cap
diameter below, .105 each side and .016 above; a 0.75 ratio affords .167 all round, so it now fades
to zero (border alpha ≤ 1) inside the frame at every one of the 128 angles. **Do not re-crop these
strips.**

Frame 0 = −135°, frame 127 = +135°, linear in **rotation**, not in parameter value — the skew lives
in the parameter→normalised mapping, not in the strip. Both strips carry a real alpha channel
(fully transparent corners, partially transparent along the anti-aliased rim and the baked drop
shadow, which now fades to zero inside the frame): blit over the plate with normal alpha blending and add no shadow of your own.

```cpp
const int frame = juce::jlimit (0, 127, (int) std::round (sliderPos * 127.0f));
// bounds is the FRAME box: 1.333 x the §3 diameter, centred on the §3 centre
g.drawImage (strip,
             bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
             0, frame * 160, 160, 160);
```

Use `Graphics::setImageResamplingQuality (highResamplingQuality)`.

---

# 2. Palette and contrast

The fascia is a vertical gradient — `#C6CCD0` top → `#B0B7BB` mid → `#A0A7AB` bottom — so text
lower on the panel has the worst background. Every value below is chosen against **the bottom**.

| Role | Value | vs top | vs mid | vs bottom |
|---|---|---|---|---|
| **Functional** — section headings, control labels, group labels, scale numerals, units, LCD captions, selected switch/algorithm labels, footer | `#16191C` | 11.27 | 8.99 | **7.57** |
| **Unselected** switch + algorithm labels | `#2B3034` | 8.22 | 6.56 | **5.52** |
| **Flavour** — strapline, model line | `#34383C` | 7.29 | 5.82 | **4.89** |
| Disabled button label | `#262B2F` | — | — | **6.06** (vs button face `#A3AAAE`) |

Rev 5's `#7B8287` inactive grey (1.60 at the bottom) is **deleted**. Nothing on this panel uses it.

**Two rules follow:**

1. **Nothing functional below 10 px.** Every label, numeral and unit is 10 px. Only the strapline,
   model line, scope legend and footer sit at 9 px, and none carry operating information.
2. **Hierarchy through size and weight, never opacity.** No element on this panel is drawn at
   reduced alpha to express state. Disabled buttons press their face *into* the panel and darken it;
   the label stays readable at 6.06:1.

## 2.1 The one deliberate exception

Unselected switch labels (`SIDECHAIN`, `SOFT`) and unselected algorithm labels are `#2B3034` —
**5.52:1, below the 7:1 functional bar and deliberately so.** The dimming is redundant encoding
alongside the shoe position and the weight change, so the state reads at a glance rather than by
comparing two identical-looking words.

Be aware this is a genuine trade-off, not a free win: 7:1 against `#A0A7AB` needs `#191D20`, which
is 0.5 of a contrast ratio away from the selected colour and visually indistinguishable from it.
You cannot have both "visibly dimmer" and "≥7:1" on this gradient. This spec chooses dimmer, holds
the floor at the 4.5:1 flavour bar with margin, and leans on the shoe as the primary indicator.

## 2.2 Surfaces

| Surface | Value |
|---|---|
| Fascia base | `#C6CCD0` → `#B0B7BB` @35 % → `#A0A7AB` |
| Brush grain | repeating vertical 1 px `rgba(255,255,255,.10)` / 1 px `rgba(0,0,0,.022)` |
| Header band | `#D2D8DB` → `#B6BDC1`, bottom border `#71787C` |
| Rack ears (L) | `#CED4D7` → `#AEB5B9` @40 % → `#929A9E` (mirrored R) |
| LCD window | bg `#07090A`, border `#363C41`, inset `rgba(0,0,0,.9)` |
| LCD text | `#F0E0B0`, glow `rgba(240,216,150,.35)` |
| Scale ticks | `#16191C`, 2 × 8–9 px |
| Sweep arc | `rgba(24,30,34,.42)`, hairline, 270° from −135° |
| Section divider | 1 px `rgba(0,0,0,.32)` (fades at ends) + 1 px `rgba(255,255,255,.55)` highlight right |
| **Gate accent (only colour on the panel)** | `#FF2B1C` |
| Lamp unlit | `#3A1512` |

**Red is reserved for gate state.** The GATE OPEN lamp and the envelope trace are the only red
elements. Never tint a knob, label or meter red.

## 2.3 Type

| Role | Face | Size | Weight | Tracking |
|---|---|---|---|---|
| Section heading | Barlow Condensed | 10 px | 700 | .26em |
| Control label | Barlow Condensed | 10 px | 700 | .14–.18em |
| Group label | Barlow Condensed | 10 px | 600 | .20em |
| Scale numeral | Barlow Condensed | 10 px | 500 | normal |
| Unit | Barlow Condensed | 10 px | 400 | .06em |
| Switch / algorithm label | Barlow Condensed | 10 px | 700 sel / 400 unsel | .10em |
| LCD caption | Barlow Condensed | 10 px | 600 | .18–.22em |
| Strapline, model line, footer | Barlow Condensed | 9 px | 400 | .19–.26em |
| LCD program / value | Share Tech Mono | 13 px | — | .10em |
| LCD IN / OUT | Share Tech Mono | 12 px | — | normal |
| Scope header / legend | Share Tech Mono | 9 px | — | .10em |

Only the two Share Tech Mono LCD roles are drawn at runtime. Everything else is baked; the metrics
are given so the §0.4 label redraw matches.

## 2.4 Fonts to embed

Two faces are drawn at runtime and **must be embedded in the binary**:

| Family | Weights / styles used | Drawn for |
|---|---|---|
| **Barlow Condensed** | 700 (Bold), 600 (SemiBold), 400 (Regular) | The §0.4 state-dependent label redraw — switch and algorithm labels |
| **Share Tech Mono** | 400 (Regular) | LCD program name, live values, IN / OUT |

Both are SIL Open Font License 1.1. Ship them in `assets/fonts/` — see the README there.

**`TudorVictors.ttf` is not a runtime font.** It is the wordmark face and the wordmark is baked; it
travels with this package only so the wordmark can be re-rendered if the header ever changes. Do
not embed it.

---

# 3. Knobs

Rotation range for every knob: **−135° → +135°** (270° sweep), pointer at 12 o'clock = centre.

Diameters below are the **cap**, not the frame. Draw each strip into a box of 1.333 × Ø centred on
(cx, cy) — see §1.3.

| Control | cx | cy | Ø | Strip |
|---|---|---|---|---|
| THRESHOLD | 102 | 170 | 62 | large |
| Trigger HP | 75 | 302 | 32 | small |
| Trigger LP | 151 | 302 | 32 | small |
| ATTACK | 259 | 289 | 54 | large |
| HOLD | 361 | 289 | 54 | large |
| RELEASE | 463 | 289 | 54 | large |
| Algorithm selector | 632 | 154 | 50 | large, **4 detents** |
| SIZE | 580 | 239 | 44 | large |
| PRE-DELAY | 684 | 239 | 44 | large |
| DAMP HF | 583 | 364 | 34 | small |
| DAMP LF | 681 | 364 | 34 | small |
| SLAM | 793 | 157 | 40 | large |
| STEREO WIDTH | 881 | 157 | 40 | large |
| MIX | 793 | 262 | 40 | large |
| OUTPUT TRIM | 881 | 262 | 40 | large |

Scale ring geometry, per knob wrapper of `Ø + 2·pad`, measured from the wrapper centre:

| Ø | pad | wrapper | ticks (radius) | numerals |
|---|---|---|---|---|
| 62 | 26 | 114 | Ø/2+2 → Ø/2+10 | Ø/2+22 |
| 54 | 22 | 98 | Ø/2+2 → Ø/2+10 | Ø/2+18 |
| 44 | 22 | 88 | ” | ” |
| 40 | 22 | 84 | ” | ” |
| 34 | 22 | 78 | ” | ” |
| 32 | 22 | 76 | ” | ” |

Numerals are upright — they never rotate with the knob. The unit prints centred in the 90° gap at
the bottom of the arc, **not** appended to the control name: `THRESHOLD` with `dB` beneath it,
never `THRESHOLD · dB`.

---

# 4. Printed scales

## 4.1 Marks

Units print in the scale area. Size, Damping HF and Damping LF are normalised and carry **no unit
at all**.

| Control | Marks | Unit | Spacing |
|---|---|---|---|
| Threshold | −60 / −45 / −30 / −15 / 0 | dB | linear |
| Trigger HP | 20 / 50 / 200 / 800 / 2000 | Hz | **skewed** |
| Trigger LP | 500 / 1k / 2k / 5k / 20k | Hz | **skewed** |
| Attack | 0.1 / 0.5 / 2 / 5 / 20 | ms | **skewed** |
| Hold | 10 / 125 / 250 / 375 / 500 | ms | linear |
| Release | 1 / 5 / 20 / 60 / 200 | ms | **skewed** |
| Size | 0 / 0.25 / 0.5 / 0.75 / 1.0 | — | linear |
| Pre-Delay | 0 / 30 / 60 / 90 / 120 | ms | linear |
| Damping HF | 0 / 0.25 / 0.5 / 0.75 / 1.0 | — | linear |
| Damping LF | 0 / 0.25 / 0.5 / 0.75 / 1.0 | — | linear |
| Slam | 0 / 3 / 6 / 9 / 12 | dB | linear |
| Stereo Width | 0 / 50 / 100 / 150 / 200 | % | linear |
| Mix | 0 / 25 / 50 / 75 / 100 | % | linear |
| Output Trim | −24 / −12 / 0 / +12 | dB | linear (4 marks) |

## 4.2 Skew

Four controls are power-law skewed:

```
fraction = ((v − min) / (max − min)) ^ skew
```

| Control | skew | | | | | |
|---|---|---|---|---|---|---|
| **Trigger HP** | 0.30 | 20 @ 0 % | 50 @ 28.5 % | 200 @ 48.7 % | 800 @ 75.6 % | 2000 @ 100 % |
| **Trigger LP** | 0.30 | 500 @ 0 % | 1k @ 33.3 % | 2k @ 46.3 % | 5k @ 64.4 % | 20k @ 100 % |
| **Attack** | 0.25 | 0.1 @ 0 % | 0.5 @ 37.7 % | 2 @ 55.6 % | 5 @ 70.4 % | 20 @ 100 % |
| **Release** | 0.30 | 1 @ 0 % | 5 @ 31.0 % | 20 @ 49.4 % | 60 @ 69.4 % | 200 @ 100 % |

Fractions are 0 % = −135°, 100 % = +135°. The printed marks were placed from this curve and the
build's parameter mapping uses the same one, **so the LCD value and the printed mark agree by
construction.** Round-tripping the skews above through the published fractions matches to within
0.13° of arc — about a fifth of a pixel at the tick radius.

**No interpolation tables anywhere.** The LCD reads the parameter value directly. Earlier revisions
of this spec called for piecewise interpolation between adjacent marks; that was wrong and is
withdrawn. It came from testing `lo·(hi/lo)^n` — a constant-ratio geometric curve — which really
does miss the marks (20 Hz high at `50`), but which is a different family from the power law on
normalised position used here. Do not build a lookup table for these four controls.

## 4.3 Tick angles

`angle = −135 + fraction × 270`. Baked; listed so the build can verify against a figure.

| Control | Tick angles (°) |
|---|---|
| Threshold, Hold, Size, Pre-Delay, Damp HF, Damp LF, Slam, Width, Mix | −135.00 · −67.50 · 0.00 · +67.50 · +135.00 |
| Output Trim | −135.00 · −45.00 · +45.00 · +135.00 |
| **Trigger HP** | −135.00 · −58.05 · −3.51 · +69.12 · +135.00 |
| **Trigger LP** | −135.00 · −45.09 · −9.99 · +38.88 · +135.00 |
| **Attack** | −135.00 · −33.21 · +15.12 · +55.08 · +135.00 |
| **Release** | −135.00 · −51.30 · −1.62 · +52.38 · +135.00 |
| Algorithm selector (detents) | −135.00 · −45.00 · +45.00 · +135.00 |

**There are no minor ticks.** Every tick on the panel sits at a labelled value. If minor ticks are
added later they must land on real values too — never at even angular subdivisions between majors.

## 4.4 Ticks are not detents

**No continuous parameter on this panel is quantised.** Every knob passes freely through its range,
and the printed marks must not imply otherwise. Do not add snapping, magnetic centres or
intermediate detents to any knob.

Genuinely stepped controls, and the only ones:

- **Algorithm selector** — 4 positions at −135° / −45° / +45° / +135°, sweeping
  **AMBI → ROOM → PLATE → CHMBR**, so the stops fall at the ends of the arc rather than mid-travel.
  Default **PLATE** (+45°) — confirmed lit in the default render.
- **KEY SOURCE** and **SHAPE** — two-position switches (§7).

---

# 5. Gate envelope scope

## 5.1 Three rectangles, not one

The scope has **three** nested rectangles and the build must keep them distinct. Clamping the trace
to the dark rect instead of to the plot region is the one mistake that reintroduces the collision
this section exists to prevent.

| Rectangle | Panel coords | Local coords | What it is |
|---|---|---|---|
| **Well** | x 208, y 118, 305 × 116 | — | Recess incl. 1 px `#0A0C0D` border + 1 px inner pad |
| **Dark rect** | x 210, y 120, 301 × 112 | 0, 0, 301 × 112 | The full painted canvas. Background `#0B0F11` → `#050708` vertical |
| **Plot region** | x 210, y 134, **267 × 98** | **0, 14, 267 × 98** | **The only area the trace may occupy.** Clip to this. |

All local coordinates below are relative to the dark rect's top-left.

The two reserved strips carved out of the dark rect:

| Strip | Local coords | Holds |
|---|---|---|
| Title strip | 0, 0, 301 × 14 | `GATE ENV` |
| Scale gutter | 267, 14, 34 × 98 | `0 dB`, `-∞` |

Both are filled `#080B0D` (flat, slightly darker than the plot backing so they read as chrome) and
separated by a 1 px `rgba(150,180,190,.18)` rule: a horizontal rule at local y 14 across the full
width, and a vertical rule at local x 267 from y 14 to the bottom.

**Clip the trace, the fill, the underlay and the grid to the plot region** before drawing any of
them. Do not rely on the data happening not to reach the gutter.

## 5.2 Plot range

Within the plot region:

| | Local y |
|---|---|
| Ceiling (0 dB, gate fully open) | **22** |
| Baseline (−∞, gate fully closed) | **102** |

The 8 px above the ceiling and 10 px below the baseline are stroke headroom for the 2 px trace and
its glow — **not** a label margin. The trace reaches y 22 and y 102 exactly, because a gate
legitimately goes fully open and fully closed. The separation from the labels is **horizontal**: the
trace stops at local x 267 and the labels live to the right of it.

## 5.3 Level annotations

`0 dB` and `-∞` sit **inside the gutter**, centre-aligned on local x 286, vertically centred on the
level each one marks — `0 dB` at y 22, `-∞` at y 102. Share Tech Mono 9 px
`rgba(170,188,196,.62)`.

A 4 px tick in `rgba(170,188,196,.45)` runs from local x 268 to x 272 at each of those two rows,
tying the label to its level across the separator rule.

`GATE ENV` sits in the title strip, left-aligned at local x 8, baseline y 10. Same face and colour.
It is now structurally protected: the trace cannot enter the title strip at any programme density,
whereas previously it was clear only because the first pulse happened to rise to its right.

## 5.4 Drawing

- **Scrolls right-to-left**, 2 px/frame at 60 fps, from local x 267 leftward. Grid verticals every
  44 px scroll with the signal; 5 horizontals static, spanning the plot region only. Grid
  `rgba(150,180,190,.10)`, baseline `rgba(150,180,190,.22)`.
- **Envelope trace**: 2 px `#FF2B1C`, **mitre joins, no smoothing** — corners must be hard. Glow:
  10 px shadow, same red. Fill beneath: `rgba(255,43,28,.30)` → `rgba(255,43,28,.02)`.
- Attack / Hold / Release map directly to drawn column counts, so the shape is the parameter state.
  The column buffer is **267** samples — the plot width, not the rect width.
- **Input underlay**: 1 px vertical strokes `rgba(178,190,197,.30)`, exponential decay per trigger.
  Grey, never red.
- Leading-edge highlight: `rgba(255,255,255,.03)`, 2 px, at local x 265–267, plot region only.

## 5.5 Shape and lamp

**SHAPE** (track x 332, y 385, 58 × 22): **HARD** (default) linear release over ~3 columns — a
vertical cliff; **SOFT** `pow(1 − t, 2.2)` over ~16 columns. The scope redraws on throw; the
difference between the two edges is the whole point of the control.

**GATE OPEN lamp** — centre **(216, 104)**, Ø 15.

| State | Fill | Glow |
|---|---|---|
| Closed | `#3A1512` | inset `rgba(255,255,255,.14)` only |
| Open | radial `#FF2B1C` → `#B0140C` @70 % → `#6D0B06` | `0 0 12px 3px rgba(255,43,28,.55)`, `0 0 30px 8px rgba(255,43,28,.22)` |

The lamp is drawn entirely by the build; nothing for it is baked. Centre (216, 104), Ø 15. The
lamp must light on the
**same sample** the gate opens — no fade-in. Release may decay over ~40 ms. The `GATE OPEN` label
never dims: it is a legend, not an indicator.

---

# 6. Header

**Changed this revision.** The strapline and model line moved from beside the wordmark to
underneath it, freeing 74 px that went into the program window.

| Element | x | y | w | h |
|---|---|---|---|---|
| Wordmark | 41 | 7 | 231 | 41 |
| Strapline + model line (stacked, 2 px gap) | 41 | 50 | 132 | 20 |
| Program window (outer) | 374 | 34 | 332 | 25 |
| &nbsp;&nbsp;FACT / USER tag cell | 375 | 35 | 48 | 23 |
| &nbsp;&nbsp;name cell | 423 | 35 | 252 | 23 |
| &nbsp;&nbsp;chevron cell | 675 | 35 | 30 | 23 |
| SAVE | 712 | 34 | 50 | 25 |
| DELETE | 767 | 34 | 50 | 25 |
| IN window | 825 | 34 | 44 | 24 |
| OUT window | 875 | 34 | 44 | 24 |

The header is now **full**: the gap between the wordmark block and the program window is exactly
14 px, matching every other header gap. There is no slack left for a wider LCD.

## 6.1 Name cell budget

| | |
|---|---|
| Cell width | 252 px |
| Padding | 10 px each side |
| **Usable** | **232 px** |
| Font | Share Tech Mono 13 px, letter-spacing 1.3 px (.10em) |
| Advance per character | **8.32 px** |
| **Budget** | **27 characters** |

Longest expected strings, measured:

| String | Width | Fits |
|---|---|---|
| `14 ROOM REINFORCEMENT` | 174.7 px | yes |
| `THRESHOLD: -18.5 dB` | 158.1 px | yes |
| `ALGORITHM: AMBIENCE` | 158.1 px | yes |
| `STEREO WIDTH: 200 %` | 158.1 px | yes |

All clear with ≥57 px spare. **Program names are capped at 24 characters** on entry, which with the
two-digit index and space is 27 — exactly the budget.

For the record, this was the reason the header was rearranged. At the previous 178 px cell the
budget was 18 characters and `14 ROOM REINFORCEMENT` overflowed by 17 px; `THRESHOLD: -18.5 dB`
overflowed by 0.4 px. Do not narrow the window back without re-checking these four strings.

## 6.2 Program window

**The caption is drawn by the build, not baked** — it reads `PROGRAM` normally and `NAME PROGRAM`
in name entry (§6.4), so it is state-dependent by the §0.2 test. Bare fascia sits at its slot.

| | Left x | Baseline y | Face | Size | Weight | Tracking | Colour |
|---|---|---|---|---|---|---|---|
| `PROGRAM` / `NAME PROGRAM` | 374 | 27.75 | Barlow Condensed | 10 px | 600 | .22em (2.2 px) | `#16191C` |

Left-aligned with the program window below it (both start at x 374). **Draw from the left x** — do
not re-centre on the string, or the caption will shift sideways when the word changes. Rendered
widths are 48.5 px (`PROGRAM`) and 80.9 px (`NAME PROGRAM`), including the trailing letter-space;
the window is 332 px, so neither crowds anything.

Three cells in one recessed LCD:

- **Tag cell** — `FACT` / `USER`, Share Tech Mono 13 px `#F0E0B0`, same face, size and colour as the
  name; 1 px `#2A3035` rule divides it.
- **Name cell** — Share Tech Mono 13 px `#F0E0B0`, centred, no wrap.
- **Chevron cell** — 30 px, no divider. A thin `˅` (6 × 6 box, 1.5 px right + bottom border, rotated
  45°) in `#F0E0B0` with a soft glow. Baked. It marks the window as a selector; it is not a button
  of its own — clicking anywhere in the window opens the program list.

## 6.3 Live values

**The panel has no standing value readouts.** Confirmed: there is no number anywhere on the fascia
outside the two LCD windows and the printed scales. The old per-knob readouts are gone.

While a control is being moved, the name cell shows `NAME: value unit` — `THRESHOLD: -18.5 dB`,
`HOLD: 165 ms`, `TRIG LP: 6.3 kHz`, `ALGORITHM: PLATE` — reverting to the program name ~800 ms
after the gesture ends. This is the **only** place a live number appears.

Values must come from the §4.2 piecewise tables for the four skewed controls.

IN / OUT windows: same treatment, 12 px.

## 6.4 SAVE / DELETE

50 × 25, Barlow Condensed 600 / 10 px / .10em, `text-indent: .1em`.

| State | Face | Border | Label | Shadow |
|---|---|---|---|---|
| Enabled | `#DBE0E3` → `#AAB1B6` | `#6D7478` | `#16191C` | inset `0 1px 0 rgba(255,255,255,.75)`, drop `0 1px 2px rgba(0,0,0,.35)` |
| Pressed | `#A9B0B5` → `#C9D0D4` | `#6D7478` | `#16191C` | inset `0 2px 4px rgba(0,0,0,.45)` |
| Disabled | `#B4BBBF` → `#A3AAAE` | `#8D9498` | `#262B2F` | inset `0 2px 3px rgba(0,0,0,.28)` |

Behaviour:

- **No edits since load** — `SAVE` disabled. Nothing to store.
- **Any parameter changed** — the program is *dirty*: `SAVE` enables and the name gains a trailing
  ` *`. Clears on store, on delete, and on loading another program.
- **Factory program** — tag `FACT`, `DELETE` disabled (read-only).
- **User program** — tag `USER`, `DELETE` enabled; removes it and falls back to the factory program.
- **SAVE → name entry** — caption `PROGRAM` → `NAME PROGRAM` (drawn live, §6.2); tag switches to `USER`; name cell
  left-aligns, clears, shows a blinking block caret (`█`, 1 s steps, 50 % duty); buttons relabel
  `STORE` / `CANCEL`, both enabled. Typing uppercases, 24-char cap. Enter stores, Esc cancels.
  Empty name falls back to `NEW PROGRAM`.

---

# 7. Switches

**KEY SOURCE** — track x 84, y 379, 58 × 22. **SHAPE** — track x 332, y 385, 58 × 22.

Identical construction: recessed `#07090A` track, 1 px `#353B40` border, 26 px sliding shoe
`#8D959B` → `#4E545A` at `left: 1px` (position A) or `left: 29px` (position B). Track is baked; the
shoe is drawn at runtime.

Labels beneath, 10 px / .10em: selected 700 `#16191C`, unselected 400 `#2B3034`. **Not on the
plate** — the build draws both, always (§0.4).

Algorithm labels behave the same way: all four drawn live, `PLATE` bold at the default.

---

# 8. Other elements

**Input meter** — x 165, y 139, 14 × 76. Well and unlit ledger baked; lit segments runtime.
Unlit `#20262A` in 4 px segments on a 6 px pitch anchored to the **bottom**; lit `#F4F8FA` on the
identical pitch with a 3 px `rgba(230,242,248,.55)` bloom, stacking upward. Threshold marker 1 px
white @50 % full width. Fast attack, slow release (~0.12 coefficient).

**Rack ears** — 26 px full-height columns both sides, one Ø11 screw boss top and bottom, inset
10 px, radial `#9BA2A8` → `#4A5055` @60 % → `#23272A`. Baked.

**Wordmark** — baked, and it must stay baked. TudorVictors with per-letter rotation (±1.9°) and
vertical drift (±1.1 px), uneven per-letter opacity (.80–.99), a soft speckle mask inside each
glyph, a two-stage overspray halo (8 px blur @30 % and 2.8 px blur @34 %) and five spatter flecks.
Ink `#14171A`. Ship `TudorVictors.ttf` only if you intend to re-bake.

---

# 9. Parameters

| ID | Name | Range | Default | Skew |
|---|---|---|---|---|
| `threshold` | Threshold | −60 → 0 dB | −18.5 | linear |
| `trigHP` | Trigger HP | 20 Hz → 2 kHz | 180 Hz | skew 0.30 |
| `trigLP` | Trigger LP | 500 Hz → 20 kHz | 6.3 kHz | skew 0.30 |
| `keySource` | Key Source | Internal / Sidechain | Internal | choice |
| `attack` | Attack | 0.1 → 20 ms | 0.4 | skew 0.25 |
| `hold` | Hold | 10 → 500 ms | 165 | linear |
| `release` | Release | 1 → 200 ms | 4 | skew 0.30 |
| `shape` | Shape | Hard / Soft | Hard | choice — release curve, §5 |
| `algorithm` | Algorithm | Ambience / Room / Plate / Chamber | **Plate** | choice, 4 detents — **see below** |
| `size` | Size | 0 → 1 | 0.72 | linear |
| `preDelay` | Pre-Delay | 0 → 120 ms | 18 | linear |
| `density` | Density | 0 → 1 | 0.6 | linear — **no knob; parameter only** |
| `dampHF` | Damping HF | 0 → 1 | 0.55 | linear |
| `dampLF` | Damping LF | 0 → 1 | 0.35 | linear |
| `slam` | Slam | 0 → +12 dB | +7 | linear — drive into the tank |
| `width` | Stereo Width | 0 → 200 % | 128 % | linear |
| `mix` | Mix | 0 → 100 % | 64 % | linear |
| `trim` | Output Trim | −24 → +12 dB | −1.4 | linear |

Factory programs: Intruder, Air Tomorrow, Cannon, Tom Thunder, Kick Chuff, Drum Bus Gate, Vocal
Chop, Radio Announcer, Synth Stab, Arp Gate, Neon Pad Swell, Power Chord Gate, Solo Ambience, Room
Reinforcement, Wall of Sound, Stutter Gate, Detonator.

## 9.1 Algorithm index order

The selector's order follows the panel, not alphabetical or historical order:

| Index | Value | Angle | Printed label |
|---|---|---|---|
| 0 | Ambience | −135° | AMBI (lower-left) |
| 1 | Room | −45° | ROOM (upper-left) |
| **2** | **Plate** | **+45°** | **PLATE** (upper-right) — **default**, baked bold |
| 3 | Chamber | +135° | CHMBR (lower-right) |

The indices are part of the contract. A choice list ordered Room · Plate · Chamber · Ambience puts
all four positions on the wrong detent and renders the default where the plate prints ROOM — and
because the index is what gets serialised, correcting it afterwards requires a session migration.

---

# 10. What matters most

1. **Composite, don't redraw.** The plate is the source of truth for every label, numeral and tick.
   The only text the build draws is inside the LCD windows, plus the eight state-dependent labels
   in §0.4 — which are absent from the plate, so there is nothing to cover.
2. The scope is the product. Hard mitred corners, instant slam-open, real red glow.
3. The lamp fires on the same sample as the gate.
4. Red appears nowhere else.
5. No knob is quantised. Ticks are a scale, not a set of detents.
6. **Index order is part of the contract, not just the labels** (§9). Anywhere a control's on-panel
   order is load-bearing — here, the Algorithm selector — the enum indices are specified, because
   getting them wrong silently mislabels every position and then needs a session migration to fix.
7. **Clip the scope trace to the plot region (§5.1), not to the dark rectangle.** The level labels
   are separated from the trace horizontally, never vertically — the trace must still reach 0 dB
   and −∞.
7. Sharp corners everywhere — no rounded rectangles on fascia, windows or switches.
