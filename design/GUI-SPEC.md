# GATECRASHER — GUI SPEC

Model **GR-85**, gated ambience processor. Neon Foundry casting, harmonisation round.
This file is authoritative for the build and supersedes Rev 15 wherever the two disagree.

**Read `shared/HEADER-PART.md` first.** Everything from the top of the panel to y 120 is
the shared part and is *not* restated here: the block, the band, the LCD's cell, budget
and cap, the Program buttons' construction and state matrix, the meter wells. This file
covers the body, the casting's own materials, and the places where Gatecrasher's
material choices meet the shared geometry.

**Asset format: vector / code-drawn.** No plate, no filmstrips, no bitmap of any panel
element. The fascia is a 2 px brushed-steel repeat, every label, tick, numeral, knob,
lamp segment and scope trace is drawn at runtime. One binary ships: `fonts/TudorVictors.ttf`
for the wordmark.

---

## 1 · Canvas

| Figure | Value |
|---|---|
| Canvas | **1340 × 700** at 100 % |
| Rack rails | 16 px each edge, four Ø11 screws at 20.5 / 668.5 |
| Header block | 16, 16, 1308 × 104 — shared part |
| Body origin | y **120**, flush to the block's bottom edge |
| Column dividers | 1 px `rgba(255,255,255,.62)` at x 260, 700, 1010, y 136 → 660 |

**+380 px of width came in with call 1** (960 → 1340, +40 %) and it is what pays for
call 3: the knobs grew from seven diameters to two, and the sections still fit at
216 px of label width per column.

Fascia `repeating-linear-gradient(90deg, #c3c8cc 0 2px, #bdc2c7 2px 4px)`. Rails
`linear-gradient(90deg, #8e959a, #b4babe 45%, #9aa1a6)` and its mirror.

---

## 2 · Sections

| Section | Heading x, y | Contents |
|---|---|---|
| INPUT / TRIGGER | 30, 150 | THRESHOLD Ø76, IN segment ladder |
| TRIGGER FILTER | 30, 386 | HP, LP — Ø56 |
| KEY SOURCE | 30, 578 | two-state shoe, INTERNAL / SIDECHAIN |
| GATE ENVELOPE | 372, 384 | ATTACK, HOLD, RELEASE — Ø76 |
| SHAPE | 372, 600 | two-state shoe, HARD / SOFT |
| REVERB TANK | 747, 150 | mode selector Ø76 + four corner labels, SIZE, PRE-DLY |
| TANK DAMPING | 747, 496 | HF, LF — Ø56 |
| OUTPUT | 1059, 150 | SLAM, WIDTH, MIX, TRIM — Ø56 |

Section headings: Barlow Condensed 600, **12 px / line box 15 / .28 em**, `#16191c`,
centred on a 216 px column.

---

## 3 · Knobs — two classes

Call 3 leaves Gatecrasher two diameters. It had seven (62 · 54 · 50 · 44 · 40 · 34 · 32)
and **kept none of them**: the whole set maps onto **Ø76 primary** and **Ø56 standard**.
Gatecrasher takes no Ø104 — it has no MODEL control, and the REVERB TANK selector is a
detented switch rather than the control the unit is described by.

| Class | Ø | Numerals | Cell | Pointer |
|---|---|---|---|---|
| Primary | 76 | up to **five** | 148 registration box | 3 × 31, `#e6eaec` |
| Standard | 56 | **three**, minors hold the rest | 128 ring in a **148 box** | 3 × 21, `#e6eaec` |

Body `radial-gradient(circle at 38% 26%, #565c61, #2b3034 46%, #14181b)`,
`inset 0 1px 0 rgba(255,255,255,.18)`, `inset 0 -3px 8px rgba(0,0,0,.65)`,
`0 3px 6px rgba(0,0,0,.38)`. Sweep arc: a 270° conic wedge at `rgba(22,25,28,.34)`,
masked to a 1.4 px ring at r + 6.

**Ticks.** Major **2 × 9** at every numeralled position, minor **1.5 × 5** at every
unnumeralled one, ink `#16191c`, both starting 2 px outside the sweep arc so the
hairline does not bridge into a tick and lengthen it visually. **Every marked position
carries a tick** — what a standard-class knob drops is the numeral, not the mark.

**Numeral ring** at `r + 8 + 9 + 6 + 6.5`, Barlow Condensed 500 11 px / 13 / .04 em.
Numerals are placed by rotation fraction and counter-rotated to upright.

### 3.1 Registration — the 10 px this round found

Mixed-class rows put the unit and label on a **Ø76 registration box for every class**:
`dy = (76 − Ø) / 2`, so `unitTop = Ø + 20 + dy` and `labelTop = Ø + 34 + dy`. The label
registers on the box (one line per row) and the ring registers on itself (pivots on one Y).

**This panel was drawn before that was understood and both mixed rows were out.** Its
cells were top-aligned, so pivots sat **12 px** apart and labels **22 px** apart — the 12
against the wrapper difference of 10 is what identified it as the box case rather than a
shift. Corrected: the top band registers on y **262** (THRESHOLD, SLAM, WIDTH — labels on
one line at 374 rendered), the envelope band on y **478** (HP, LP, ATTACK, HOLD, RELEASE).
The band moved to 478 rather than 490 because dropping the Ø56 pair onto the trio's Y put
their label line 1 px off the KEY SOURCE heading; at 478 there is 13 px of clearance.

The REVERB TANK selector at (855, 250) is **exempt and stays where it is**: it carries
corner labels, not a label on the baseline, so the rule does not reach it.

### 3.2 Mark lists

Angle = `−135 + 270 f`. Numerals in **bold**; every other row is a minor tick at a real value.

| Knob | Ø | Marks (angle · printed) |
|---|---|---|
| THRESHOLD | 76 | −135 **−60** · −67.5 **−45** · 0 **−30** · +67.5 **−15** · +135 **0** — dB |
| ATTACK | 76 | −135 **0.1** · −33.34 **0.5** · +15.09 **2** · +55.19 **5** · +135 **20** — ms |
| HOLD | 76 | −135 **10** · −67.5 **125** · 0 **250** · +67.5 **375** · +135 **500** — ms |
| RELEASE | 76 | −135 **1** · −51.38 **5** · −1.54 **20** · +52.48 **60** · +135 **200** — ms |
| HP | 56 | −135 **20** · −58.05 · −3.51 **200** · +69.12 · +135 **2000** — Hz |
| LP | 56 | −135 **500** · −45.05 · −9.91 **2k** · +38.91 · +135 **20k** — Hz |
| SIZE | 56 | −135 **0** · −67.5 · 0 **0.5** · +67.5 · +135 **1.0** |
| PRE-DLY | 56 | −135 **0** · −67.5 · 0 **60** · +67.5 · +135 **120** — ms |
| HF · LF | 56 | −135 **0** · −67.5 · 0 **0.5** · +67.5 · +135 **1.0** |
| SLAM | 56 | −135 **0** · −67.5 · 0 **6** · +67.5 · +135 **12** — dB |
| WIDTH | 56 | −135 **0** · −67.5 · 0 **100** · +67.5 · +135 **200** — % |
| MIX | 56 | −135 **0** · −67.5 · 0 **50** · +67.5 · +135 **100** — % |
| TRIM | 56 | −135 **−24** · −45 · +45 **0** · +135 **+12** — dB |

HP, LP, ATTACK, RELEASE are skewed; **their angles are the contract and must not be
derived by even spacing.** The units print inside the arc's bottom gap, never as a suffix
on the control name. `−` is U+2212 throughout, and TRIM's `+12` keeps its leading plus.

### 3.3 REVERB TANK selector

Ø76, four detents, pointer angle `−45 + index × 90`, ticks 2 × 9 centred on each detent.
Corner labels Barlow Condensed 600 10 px / 13 / .18 em `#16191c`, clickable:
ROOM (index 0, top-left, −45°) · PLATE (1, top-right, +45°) · CHMBR (2, bottom-right,
+135°) · AMBI (3, bottom-left, +225°). **The ordering is deliberate** — the two small
spaces sit left, the two large right. Map indices to the DSP enum as listed.

---

## 4 · IN segment ladder

**Original to this casting and not added by this round.** 13 segments, 22 × 90 at
(196, 214), 1 px gaps, radius 1.

| State | Fill | Ring |
|---|---|---|
| Lit | `linear-gradient(#f2f5f6, #d2d8db)` | `inset 0 0 0 1px rgba(0,0,0,.35)` |
| Unlit | `linear-gradient(#20252a, #12161a)` | `inset 0 0 0 1px rgba(0,0,0,.55)` |

Lit against unlit measures **16.59:1** — the ladder reads as a level, not as a colour
change. Caption `IN` at (186, 310), 10 px / 13 / .28 em. Live peak, drawn at runtime.

---

## 5 · Gate envelope scope

Well 400 × 184 at (284, 176), `#0a0c0e`, `inset 0 0 0 1px #6f767b`,
`inset 0 2px 8px rgba(0,0,0,.9)`. Grid: 1 px `rgba(255,255,255,.07)` at 50 px
horizontally and 46 px vertically.

**Plot region 358 wide, legend gutter 42.** The 1 px `rgba(255,255,255,.10)` rule at
x 358 is the split, and the trace is clamped to the plot region — `0 dB` and `−∞` live
in the gutter at the right, so a full-height envelope cannot reach them. Time axis
50 ms / DIV. Trace `#ff2b1c`, 3 px, `drop-shadow(0 0 6px rgba(255,43,28,.55))`, fill
`rgba(255,43,28,.10)`. Legends Barlow Condensed 10 px / 13, `#9aa1a6`, **7.48:1** on glass.

Gate closed draws the baseline only (`7,168 354,168`) — the trace does not disappear.

---

## 6 · Two-state shoes

Two on this panel, both the shared §4B part: **128 × 32, two 64 halves**, radius 3,
`inset 0 0 0 1px #7c8286`, `0 1px 2px rgba(0,0,0,.3)`. Engaged half
`linear-gradient(#e8ecee, #c0c6ca)` with `inset 0 1px 0 rgba(255,255,255,.9)`;
disengaged half `linear-gradient(#1d2226, #0e1113)`.

| Shoe | At | Positions |
|---|---|---|
| KEY SOURCE | 74, 598 | INTERNAL · SIDECHAIN |
| SHAPE | 416, 620 | HARD · SOFT |

**Legends are printed once, under their own half, and never re-inked or moved** — 64 px
wide, centred, 10 px / 13 / .16 em, `#16191c` on fascia. The shoe indicates state; it does
not relabel the control.

---

## 7 · Palette and measured contrast

Computed in one pass from this panel's own hexes against each ground **by name**, with the
worst case quoted where a ground is a gradient. Functional floor 7:1, flavour 4.5:1,
state floor 3:1. Re-measure rather than transcribe; if a figure here disagrees with
yours, yours wins and tell us.

### On fascia (worst case `#bdc2c7`) and the header block (worst case `#bcc2c7`)

| Ink | Role | Ratio | Class |
|---|---|---|---|
| `#16191c` | section headings, control labels, units, scale numerals, shoe legends, meter caption, header captions, GATE OPEN | **9.84** fascia · **9.82** block | functional |
| `#1b1e21` | wordmark | **9.31** block | functional |
| `#2b3034` | scope header data (`ENVELOPE 50 ms / DIV`) | **7.43** fascia | functional |
| `#2b2f33` | model line | **7.50** block | functional |
| `#34383c` | version stamp | **6.59** fascia | flavour |

**The model line was `#34383c` and measured 6.57 against the block's dark end — under the
functional floor.** It is now `#2b2f33`, 7.50 — the hex the six-material header strip already carried for this role, which the body had not inherited. The version stamp keeps `#34383c`: it is flavour
text and clears 4.5:1 by two stops.

### On LCD glass (`#0d0f11 → #08090b`)

| Ink | Role | Ratio |
|---|---|---|
| `#e8c96a` | program name, bank tag, live readout, meter values, chevron | **11.88** light end · **12.33** dark end |

### On the scope (`#0a0c0e`)

| Ink | Role | Ratio |
|---|---|---|
| `#9aa1a6` | `GATE ENV`, `0 dB`, `−∞` | **7.48** |
| `#ff2b1c` | envelope trace | 5.24 — **graphic, no text floor applies** |

### On the Program cap (`#23282c → #14181b`)

| Ink | State | Ratio |
|---|---|---|
| `#f4f8fa` | lit | **13.93** light end · **16.71** dark end |
| `#9aa1a6` | idle | **5.68** light end · **6.82** dark end |

Both clear the 3:1 state floor at both ends. There is **no disabled face**: cap, ring and
highlight are identical in all five states and only the legend's illumination changes.

### Accent

**One accent: `#ff2b1c`.** The GATE OPEN lamp, its glow, and the envelope trace. It
appears nowhere else — not on knobs, labels, shoes, the ladder or the LCD.

---

## 8 · State matrices

### 8.1 Program legends — shared part, restated because the build reads this file

| Panel state | SAVE | STORE | DELETE | CANCEL |
|---|---|---|---|---|
| Factory Program, unmodified | idle | idle | idle | idle |
| Factory Program, edited | **lit** | idle | idle | idle |
| User Program, unmodified | idle | idle | **lit** | idle |
| User Program, edited | **lit** | idle | **lit** | idle |
| Naming a Program | idle | **lit** | idle | **lit** |

Lit `#f4f8fa` + 7 px bloom; idle `#9AA1A6`, flat. Weight is 600 in every cell — ink weight
never stands in for illumination.

### 8.2 GATE OPEN lamp

| State | Lens | Glow |
|---|---|---|
| Open | `radial-gradient(circle at 40% 32%, #ff5a4a, #ff2b1c 35%, #b0140c 70%, #6d0b06)` | `0 0 12px 3px rgba(255,43,28,.55)`, `0 0 30px 8px rgba(255,43,28,.22)` |
| Closed | `radial-gradient(circle at 40% 32%, #5a221c, #3a1512 60%, #240d0a)` | none; `inset 0 -2px 4px rgba(0,0,0,.6)` |

**Light stops at the lens edge** — no halo on the fascia, and the unlit lens stays a dark
red lens rather than going grey. Ø15 in a 31 px well at (272, 140).

### 8.3 The two shoes

| Shoe | State | Left half | Right half | Legends |
|---|---|---|---|---|
| KEY SOURCE | INTERNAL | light | dark | both printed, unchanged |
| KEY SOURCE | SIDECHAIN | dark | light | both printed, unchanged |
| SHAPE | HARD | light | dark | both printed, unchanged |
| SHAPE | SOFT | dark | light | both printed, unchanged |

Four cells, one construction: the half that is engaged is light. Nothing else on the
control changes in any state.

### 8.4 Scope

| Gate | Trace |
|---|---|
| Open | envelope polyline, three cycles across 358 px |
| Closed | baseline only, full plot width |

---

## 9 · Type

Every size is a **CSS px em size with a pinned line box**, stated as a pair (call 4). No
line-height inherits font metrics anywhere on this panel.

| Role | Face | Size / line box | Tracking | Ink |
|---|---|---|---|---|
| Wordmark | TudorVictors 36 px | 36 / 38 | .02 em | `#1b1e21` |
| Function descriptor | Barlow Condensed 600 | 14 / 17 | .26 em | `#16191c` |
| Model line | Share Tech Mono | 11 / 14 | .20 em | `#2b2f33` |
| Section heading | Barlow Condensed 600 | 12 / 15 | .28 em | `#16191c` |
| Control label | Barlow Condensed 600 | 12 / 15 | .18 em | `#16191c` |
| Unit | Barlow Condensed 500 | 10 / 13 | .16 em | `#16191c` |
| Scale numeral | Barlow Condensed 500 | 11 / 13 | .04 em | `#16191c` |
| Corner label, shoe legend, meter caption | Barlow Condensed 600 | 10 / 13 | .18 / .16 / .28 em | `#16191c` |
| Scope legend | Barlow Condensed 500/600 | 10 / 13 | .14–.20 em | `#9aa1a6` |
| LCD / meter value | Share Tech Mono | 17 / 22 | .10 em | `#e8c96a` |
| Program legend | Barlow Condensed 600 | 11 / 13 | .12 em | see 8.1 |
| Version stamp | Share Tech Mono | 10 / 13 | .18 em | `#34383c` |

**The wordmark is the one per-casting face** and is outside call 7 — it is the nameplate
metaphor, a spray stencil set in TudorVictors with per-letter rotation and vertical
jitter, baked into no bitmap and drawn as eleven rotated spans. Panel lettering is
Barlow Condensed; **numerals and the model line stay in Share Tech Mono**, which is this
casting's own mono as well as the shared LCD face.

---

## 10 · Conformance — calls this casting already satisfied

Named rather than left silent, so a call appearing in neither this section nor §12 is a
gap by construction.

| Call | State |
|---|---|
| **2** — one LCD face, Share Tech Mono 17 / .10 em, 641 cell, budget 49, cap 47 | **already conformed.** Gatecrasher was on Share Tech Mono; only its cap moved, and it moved up (27 → 47). No name is orphaned. |
| **4** — every size a CSS px em with a pinned line box | **already conformed.** Every figure in §9 was already a pair; nothing resolved from font metrics. |
| **5** — knobs code-drawn and cached, no filmstrips | **already conformed** in artwork: the ring, ticks, numerals and pointer were always drawn from rotation fractions. The sheets the call retires were never in this casting's bundle; `setBufferedToImage` is the build's to add. |
| **6** — plates export at 3× | **checked, and it stays plateless.** The fascia is a 2 px procedural repeat with no texture that wants baking, so call 6's per-casting permission applies and nothing exports at 3×. If a wear layer is ever added it becomes a plate and the call binds. |
| **7** — one panel typeface, Barlow Condensed | **already conformed** for all panel lettering. The wordmark is the nameplate metaphor and outside the call; scale numerals and the model line stay in the casting's own mono per call 7's split. |
| **§4B** — two-state shoe, 128 × 32 in two 64 halves, legends printed once | **already conformed.** Both shoes were built to the part; neither re-inks a legend. |
| **Lamps** — light stops at the lens edge, unlit lens stays in its own hue | **already conformed** on the GATE OPEN lamp. |

---

## 11 · Product icon

`icons/gatecrasher-icon-1024.png` and `-256.png`, PNG with alpha, transparent rounded-square
corners so JUCE/macOS masking does not double-round them. Source of truth is the icon
Design Component; re-render at any multiple from there.

**Seven optical cuts, each drawn at its size — not one image scaled.** 1024 · 512 · 256 · 128 ·
64 · 32 · 16, delivered in `handoff/gatecrasher/icons/`.

The mark is **one gate envelope** — rising edge, plateau, release, floor — in the accent on a
dark chassis with an 0.18 corner radius. *(An earlier draft of this section described it as a
three-cycle trace with rail screws. It has never been either; that was the scope’s description
attached to the icon.)*

**What drops as the box shrinks, in order:**

| Size | Grid hairlines | Plateau shadow | Bloom | Brushed texture | Floor run | Stroke |
|---|---|---|---|---|---|---|
| 1024 · 512 | yes | yes | full | yes | yes | .054 |
| 256 | yes | yes | 0.9 | yes | yes | .056 |
| 128 | **dropped** | yes | 0.8 | yes | yes | .060 |
| 64 | — | **dropped** | 0.6 | yes | yes | .068 |
| 32 | — | — | **dropped** | **dropped** | yes | .084 |
| 16 | — | — | — | — | **dropped** | .115 |

**Stroke weight rises as the box falls** — .054 of the box at 1024 against .115 at 16 — because
a mark scaled linearly thins into the chassis before the shapes stop reading.

**16 was cut twice.** The first cut kept the rising edge and plateau alone, on the reasoning that
the edge *is* the mark; drawn, it read as a red corner rather than a gate. **The release fall
stays and the floor run goes instead:** the shape has to say *envelope* before it says *edge*,
and it is the fall that makes it an envelope rather than a bracket. Geometry at 16 is also
opened up — plateau to .60, fall to .855 — so the diagonal has pixels to exist in.

---

## 12 · Changelog and outstanding

### This round

0. **Seven icon optical cuts drawn** at 1024 / 512 / 256 / 128 / 64 / 32 / 16, each cut at its
   size with a stated simplification ladder (§11). The 16 was re-cut after the first version
   read as a corner rather than a gate.
0b. **A sentence in §11 was never true, and is recorded as that rather than as a correction.**
   It described the mark as a three-cycle trace with rail screws — the *scope’s* description,
   attached to the icon. The icon has always been one gate envelope. No change history contains
   the moment it went wrong, because there is no such moment; only reading it against the thing
   it describes finds it, and it would have had someone cutting from the wrong drawing.

1. **Canvas 960 → 1340** (call 1). Sections re-spaced to 216 px columns with dividers at
   260 / 700 / 1010.
2. **Seven knob diameters → two** (call 3): 62 · 54 · 50 · 44 · 40 · 34 · 32 → **Ø76 · Ø56**.
   Every knob grew.
3. **Standard-class numeral counts cut to three** (call 3), with the demoted values kept as
   minors at their real angles. Primary-class rings keep five.
4. **Both mixed rows re-registered** (§3.1): pivots on one Y per band, labels on one line,
   the envelope band moved to y 478 for the KEY SOURCE clearance.
5. **Header replaced by the shared part** — 1308 × 104 block, 641 LCD at x 357, buttons
   62 / 70, wells 64, row ending 1302. Its own LCD cap rose 27 → 47.
6. **LCD chevron re-drawn** as the shared 14 × 8 stroked path (`M1 1.6 L7 6.4 L13 1.6`,
   1.6 px, round caps), replacing a 9 × 9 rotated box. The box measured 84.5° against the
   path's 77° and was never a deliberate choice on any casting.
7. **Model line `#34383c` → `#2b2f33`**, 6.57 → 7.50 on the block's dark end, reconciled with the six-material strip.

### Outstanding

- Wire the IN ladder and both meter wells to real peak metering — the render shows sample
  values (`−6.2` / `−1.4`).
- Confirm the four skewed rings (HP, LP, ATTACK, RELEASE) against the build's
  `NormalisableRange` before anything is treated as final. Where they disagree, the build
  wins and the artwork is re-cut.
- **`shared/HEADER-PART.md` revision 3 is pending three build answers** — the meter's
  display clamp, its number format at both ends, and the sign convention. Nothing on this
  panel changes either way: the wells stay 64 and hold five characters.
