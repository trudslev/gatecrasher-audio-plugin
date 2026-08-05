# GATECRASHER GR-85 — GUI Implementation Spec

Panel: **960 × 434 px** at 1× (fixed aspect). Reference renders in `assets/`.
All coordinates below are panel-local, origin = top-left of the 960×434 panel.

`assets/gatecrasher-panel-bare@2x.png` — 1920 × 868, the empty chassis: brushed-steel field with
vertical grain, side rails with screws, the 76 px header band, and the three section dividers.
No controls, labels, or nameplate. Use it as the panel background image; everything else is drawn
on top. (`gatecrasher-panel@2x.png` and `-gate-open@2x.png` remain the fully dressed references.)

---

## 1. Palette

| Role | Value |
|---|---|
| Fascia base (vertical gradient, top→bottom) | `#C6CCD0` → `#B0B7BB` @35% → `#A0A7AB` |
| Brush grain overlay | repeating vertical 1px `rgba(255,255,255,.10)` / 1px `rgba(0,0,0,.022)` |
| Header band | `#D2D8DB` → `#B6BDC1`, bottom border `#71787C` |
| Rack ears (L) | `#CED4D7` → `#AEB5B9` @40% → `#929A9E` (mirrored on R) |
| Engraved heading text | `#33383D` |
| Control label text | `#2B3034` |
| Value / secondary text | `#3D4348` |
| Tertiary (group labels) | `#464C51` |
| Inactive label (SIDECHAIN) | `#7B8287` |
| LED window bg | `#07090A`, border `#363C41`, inset shadow `rgba(0,0,0,.9)` |
| LED text | `#DFE6EA`, glow `rgba(200,220,230,.25)` |
| Section divider | 1px `rgba(0,0,0,.32)` (fades at ends) + 1px `rgba(255,255,255,.55)` highlight to its right |
| **Gate accent (ONLY colour on panel)** | `#FF2B1C` |
| Lamp unlit | `#3A1512` |

**Rule: red is reserved for gate state.** The GATE OPEN lamp and the envelope trace are the only red elements. Never tint a knob, label, or meter red.

## 2. Typography

- Labels / headings: **Barlow Condensed** (600 for labels, 700 for lamp text).
- Numeric readouts, LED windows, scope annotations: **Share Tech Mono**.
- Wordmark: **TudorVictors** (ship in BinaryData) — see §7.
- Sizes: section headings 9.5px / .28em tracking · control labels 9.5–10px / .14–.20em · values 9px · LED program 13px / .10em.

## 3. Knob geometry (centres, panel-local)

Rotation range for every knob: **−135° → +135°** (270° sweep), pointer at 12 o'clock = centre.

| Control | cx | cy | Ø | Filmstrip |
|---|---|---|---|---|
| THRESHOLD | 103 | 162 | 62 | large |
| Trigger HP | 89 | 284 | 38 | small |
| Trigger LP | 145 | 284 | 38 | small |
| ATTACK | 300 | 284 | 56 | large |
| HOLD | 390 | 284 | 56 | large |
| RELEASE | 480 | 284 | 56 | large |
| Algorithm selector | 686 | 165 | 50 | large, **4 detents** |
| SIZE | 652 | 241 | 44 | large |
| PRE-DELAY | 722 | 241 | 44 | large |
| DAMP HF | 657 | 357 | 34 | small |
| DAMP LF | 717 | 357 | 34 | small |
| SLAM | 841 | 151 | 40 | large |
| STEREO WIDTH | 893 | 151 | 40 | large |
| MIX | 841 | 237 | 40 | large |
| OUTPUT TRIM | 893 | 237 | 40 | large |

Tick ring: draw in code, **not** part of the filmstrip (it does not rotate). 1px radial ticks in `#3F454A` at 50% alpha, from r+2 to r+7, every 15° for large knobs / 20–22° for small ones. Algorithm selector instead gets 4 ticks at 45°, 135°, 225°, 315° in `#3F454A` @70%.

## 4. Filmstrips

`assets/knob_large_128px_128f.png` — 128 × 16384, 128 frames, knurled skirt.
`assets/knob_small_128px_128f.png` — 128 × 16384, 128 frames, plain skirt.

Frame 0 = −135°, frame 127 = +135°, linear. Frames are square with transparent margin; the cast shadow is baked in, so draw the frame into the knob's **full bounding box** (Ø + ~7% bleed), not the knob circle.

```cpp
const int frame = juce::jlimit(0, 127, (int) std::round(sliderPos * 127.0f));
g.drawImage(strip,
            bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
            0, frame * 128, 128, 128);
```

Rendered at 128px so every panel size (largest is 62px → 124px @2×) downscales rather than upscales. Use `Graphics::setImageResamplingQuality (highResamplingQuality)`.

## 5. Gate envelope scope — the centrepiece

Rect: **x 218, y 113, w 344, h 122** (1px `#0A0C0D` border, 1px inner padding, bg `#06080A` → `#0B0F11` vertical).

Baseline at `h − 14`, ceiling at `y + 14`.

- **Scrolls right-to-left**, 2 px per frame at 60fps. Grid verticals every 44px scroll with the signal; 5 horizontals are static. Grid `rgba(150,180,190,.10)`, baseline `rgba(150,180,190,.22)`.
- **Envelope trace**: 2px `#FF2B1C`, **mitre joins, no smoothing** — the corners must be hard. Glow: 10px shadow in the same red. Fill beneath: vertical `rgba(255,43,28,.30)` → `rgba(255,43,28,.02)`.
- Shape per trigger: attack ramp over ~5 columns → flat plateau at full height for the Hold time → fall governed by the SHAPE switch (below). Attack/Hold/Release knobs must map to these column counts so the drawn shape is the actual parameter state.

### SHAPE switch (release character)

Two-position toggle centred beneath the A/H/R knobs at **x 361, y 364, 58 × 56** (label / track / labels stack). It reuses the KEY SOURCE switch verbatim — recessed 56 × 20 track `#07090A` with a 1px `#353B40` border, 26px sliding metal shoe `#8D959B` → `#4E545A`, shoe at `left: 1px` for the first position and `left: 29px` for the second. `SHAPE` caption above (9px / .20em, `#464C51`); `HARD` / `SOFT` beneath, active `#2B3034`, inactive `#7B8287`.

| Position | Release edge | Scope |
|---|---|---|
| **HARD** (default) | Linear, ~3 columns — a vertical cliff | `assets/scope-hard-release@3x.png` |
| **SOFT** | `pow(1 − t, 2.2)` over ~16 columns | `assets/scope-soft-release@3x.png` |

The scope must redraw with the switch — the difference between the two release edges is the whole point of the control, and it should be visible the instant it's thrown.
- **Input waveform underlay**: 1px vertical strokes, `rgba(178,190,197,.30)`, exponential decay from each trigger. Grey, never red.
- Annotations in Share Tech Mono 9px `rgba(160,178,186,.55)`: `GATE ENV` top-left, `0 dB` / `-∞` right-aligned.

**GATE OPEN lamp** — centre (224, 95), Ø 15, above the scope's left edge.

| State | Fill | Glow |
|---|---|---|
| Closed | `#3A1512` | inset `rgba(255,255,255,.14)` only |
| Open | radial `#FF2B1C` → `#B0140C` @70% → `#6D0B06` | `0 0 12px 3px rgba(255,43,28,.55)`, `0 0 30px 8px rgba(255,43,28,.22)` |

Label `GATE OPEN` switches `#43494E` → `#141719` with the lamp. Lamp must light on the **same sample** the gate opens — no fade-in, no smoothing. Release may decay over ~40ms.

## 6. Program section (header, right side)

Coordinates are panel-local, 1×.

| Element | x | y | w | h |
|---|---|---|---|---|
| Program window (outer) | 480 | 33 | 238 | 25 |
| &nbsp;&nbsp;FACT / USER tag cell | 481 | 34 | 39 | 23 |
| SAVE button | 724 | 33 | 44 | 25 |
| DELETE button | 773 | 33 | 44 | 25 |
| IN / OUT windows | 822 → 908 | 33 | ~44 each | 25 |

`PROGRAM` caption sits above the window, 9px / .24em, `#3A4045`.

### Program window

One recessed LED window split into two cells by a 1px `#2A3035` divider:

- **Tag cell** — Share Tech Mono 9px / .12em. Reads `FACT` in `#6F797F` for factory programs, `USER` in `#CFD7DC` for user programs. This is the at-a-glance read-only indicator.
- **Name cell** — Share Tech Mono 13px / .10em, `#DFE6EA`, centred, no wrap. Min-width 236px on the outer window; let it grow for long user names rather than truncate.

### SAVE / DELETE buttons

Stamped-steel utility buttons, deliberately secondary to the LCD. Same height as the window (25px), width sized to the label with 6px internal side padding, 6px gap to the LCD and 5px between the two. Bottom-aligned with the window so their centre lines up with it.

| State | Face | Border | Label | Shadow |
|---|---|---|---|---|
| Enabled | `#DBE0E3` → `#AAB1B6` | `#6D7478` | `#22272B` | inset `0 1px 0 rgba(255,255,255,.75)`, drop `0 1px 2px rgba(0,0,0,.35)` |
| Pressed | `#A9B0B5` → `#C9D0D4` | `#6D7478` | `#22272B` | inset `0 2px 4px rgba(0,0,0,.45)` |
| Disabled | `#C2C8CC` → `#A8AFB3` | `#8D9498` | `#8B9297` | inset `0 1px 0 rgba(255,255,255,.4)`, opacity .55 |

Labels: Barlow Condensed 600, 8.5px, .12em tracking, plus `text-indent: .12em` so the trailing letter-space doesn't push the label optically left.

### Behaviour

**Idle, factory program loaded** — tag `FACT`. `SAVE` enabled. `DELETE` **disabled** (factory programs are read-only).

**Idle, user program loaded** — tag `USER`. Both enabled. `DELETE` removes the program and falls back to the factory program.

**SAVE pressed → name-entry mode:**
- Caption changes `PROGRAM` → `NAME PROGRAM`
- Tag switches to `USER` immediately (you are naming a user program)
- Name cell left-aligns, clears, and shows a blinking block caret (`█`, 1s steps, 50% duty)
- Buttons relabel `SAVE` → `STORE` and `DELETE` → `CANCEL`; both enabled
- Typing fills the name, uppercased, 22 char cap. Enter = store, Esc = cancel.
- `STORE` commits (empty name falls back to `NEW PROGRAM`); `CANCEL` reverts to the previously loaded program.

Reference renders: `assets/header-factory-program@3x.png`, `assets/header-user-program@3x.png`, `assets/header-name-entry@3x.png`.

## 7. Other elements

**Input meter** — x 147, y 133, 14 × 76. Segmented: unlit ledger `#20262A` in 4px segments on a 6px pitch anchored to the **bottom** edge; lit segments `#F4F8FA` on the identical pitch with a 3px `rgba(230,242,248,.55)` bloom, stacking upward. Threshold marker: 1px white @50% across the full width. Fast attack, slow release (~0.12 coefficient).

**KEY SOURCE switch** — track at x 88, y 371, 56 × 20 (identical to the SHAPE switch). Recessed black track, sliding metal shoe `#8D959B` → `#4E545A` (26px wide, snaps left/right). `INTERNAL` / `SIDECHAIN` beneath; active one `#2B3034`, inactive `#7B8287`.

**Rack ears** — 26px columns full height on both sides, one Ø11 screw boss top and bottom, inset 10px. Screws are radial `#9BA2A8` → `#4A5055` @60% → `#23272A`.

**Section dividers** at x ≈ 208, 573, 800. Vertical padding inside every section column: 16px top and bottom.

## 8. Wordmark

Do **not** render the sprayed lettering in code. Bake it: the treatment is TudorVictors with per-letter rotation (±1.9°) and vertical drift (±1.1px), uneven per-letter opacity (.80–.99), a soft speckle mask inside each glyph, a two-stage overspray halo (8px blur @30% and 2.8px blur @34%), and five spatter flecks. Ship as a transparent PNG at @2× (and @3× if you support it) positioned at x 38, y 20, ~232 × 40 at 1×. Ink colour `#14171A`.

## 9. Parameters

| ID | Name | Range | Default | Skew / notes |
|---|---|---|---|---|
| `threshold` | Threshold | −60 → 0 dB | −18.5 | linear |
| `trigHP` | Trigger HP | 20 Hz → 2 kHz | 180 Hz | log |
| `trigLP` | Trigger LP | 500 Hz → 20 kHz | 6.3 kHz | log |
| `keySource` | Key Source | Internal / Sidechain | Internal | choice |
| `attack` | Attack | 0.1 → 20 ms | 0.4 | log — sub-ms matters |
| `hold` | Hold | 10 → 500 ms | 165 | linear |
| `release` | Release | 1 → 200 ms | 4 | log |
| `shape` | Shape | Hard / Soft | Hard | choice — release curve, see §5 |
| `algorithm` | Algorithm | Room / Plate / Chamber / Ambience | Plate | choice, 4 detents |
| `size` | Size | 0 → 1 | 0.72 | linear |
| `preDelay` | Pre-Delay | 0 → 120 ms | 18 | linear |
| `density` | Density | 0 → 1 | 0.6 | linear — **no knob on panel; expose as a parameter only, or add if you re-spec the layout** |
| `dampHF` | Damping HF | 0 → 1 | 0.55 | linear |
| `dampLF` | Damping LF | 0 → 1 | 0.35 | linear |
| `slam` | Slam | 0 → +12 | +7 | drive into the tank |
| `width` | Stereo Width | 0 → 200% | 128% | linear |
| `mix` | Mix | 0 → 100% | 64% | linear |
| `trim` | Output Trim | −24 → +12 dB | −1.4 | linear |

Factory programs: Intruder, Air Tomorrow, Cannon, Tom Thunder, Kick Chuff, Drum Bus Gate, Vocal Chop, Radio Announcer, Synth Stab, Arp Gate, Neon Pad Swell, Power Chord Gate, Solo Ambience, Room Reinforcement, Wall of Sound, Stutter Gate, Detonator.

## 10. What matters most

1. The scope is the product. Hard mitred corners, instant slam-open, real red glow.
2. The lamp fires on the same sample as the gate.
3. Red appears nowhere else.
4. Sharp corners everywhere — no rounded rectangles on the fascia, windows, or switch.
