# Gatecrasher GR-85 — GUI handoff

**Rev 7.** You are building the JUCE plugin GUI for **Gatecrasher**, an 80s gated-reverb
processor. The visual design is approved and final — implement it, don't redesign it.

Read `GATECRASHER-GUI-SPEC.md` first, and read **§0 before anything else**. It decides the shape of
the whole implementation.

## The one thing to get right

**Almost nothing on this panel is drawn in code.** All static furniture — every label, every printed
scale numeral, every tick, the wordmark, the recessed wells — is baked into
`assets/gatecrasher-panel-plate@2x.png`. Blit that as the background and composite moving parts on
top of it.

**If you started from Rev 5, delete the label-drawing code.** Rev 5 told you to draw the text
yourself and shipped a bare chassis to draw it onto. That is superseded. Adapting the old code
rather than deleting it will double-draw every label at a one-pixel offset.

The build draws exactly nine things (§0.5): knob filmstrip frames, switch shoes, the eight
state-dependent labels, the input meter fill, the scope, the gate lamp, the LCD text, and the two
buttons. That is the complete list.

The sorting rule behind the plate is **"does this change?"**, not "is this a label?". Printed scales
are baked because their numerals sit on irregular radii at irregular angles. The switch and
algorithm labels are drawn live because their weight follows a control — and baked pixels cannot be
un-drawn.

## Assets

**Build inputs**
- `assets/gatecrasher-panel-plate@1x.png` / `@2x.png` — the background. Blit at 0,0.
- `assets/knob_large_128px_128f.png` — 128 frames, knurled, −135°→+135°
- `assets/knob_small_128px_128f.png` — 128 frames, plain skirt
- `assets/icon/` — plugin icon, 1024 down to 16 px

**QA targets** — composite your own output and diff against these
- `assets/gatecrasher-panel@1x.png` / `@2x.png` — default state, gate closed
- `assets/gatecrasher-panel-gate-open@1x.png` / `@2x.png` — gate open, lamp lit
- `assets/header-factory-program@3x.png` — factory program, clean: SAVE **and** DELETE disabled
- `assets/header-edited@3x.png` — same program edited: SAVE enabled, name shows trailing ` *`
- `assets/header-name-entry@3x.png` — SAVE pressed: caret, STORE / CANCEL
- `assets/header-user-program@3x.png` — stored: tag USER, DELETE enabled
- `assets/scope-hard-release@3x.png` / `scope-soft-release@3x.png` — SHAPE = HARD vs SOFT

**Runtime fonts** — embed both, see `assets/fonts/README.md`
- **Barlow Condensed** 700 / 600 / 400 — the eight state-dependent labels (§0.4)
- **Share Tech Mono** 400 — LCD name, live values, IN / OUT

**Reference only**
- `assets/gatecrasher-panel-bare@2x.png` — chassis with no furniture. Superseded as a build input;
  kept for mocking layout changes.
- `assets/TudorVictors.ttf` — wordmark face. **Not a runtime font**; the wordmark is baked. Ships
  only so it can be re-rendered if the header layout changes.

## Live reference

`reference/Gatecrasher.dc.html` is the working mockup — open it in a browser to see the scope
animating, the lamp firing, knobs dragging with live LCD readout, and the SAVE / DELETE /
name-entry flow. `reference/support.js` and `assets/TudorVictors.ttf` must sit alongside it (they do
in this package).

Every value in the spec came from this file. Read its source for exact gradients and the scope draw
loop, but treat it as a visual and behavioural reference, **not** as code to port — it uses inline
CSS on a small custom runtime.

Also included: `reference/Panel Background.dc.html` (regenerates the bare chassis),
`reference/Gatecrasher Icon Master.dc.html` (regenerates the icon), and
`reference/Knob Render Comparison.dc.html` (the code-drawn vs filmstrip comparison that settled the
knob decision).

## Decisions already made

- Knobs are **filmstrips**, not code-drawn. Rendered at 128 px so all panel sizes downscale.
- Panel is fixed 960 × 434 at 1×.
- Printed scales replace per-knob value readouts. **The panel carries no live numbers** outside the
  two LCD windows.
- Algorithm sweeps **AMBI → ROOM → PLATE → CHMBR** across −135° → +135°, so the stops land at the
  ends of the arc. Index order is **0 = Ambience, 1 = Room, 2 = Plate, 3 = Chamber**, default 2
  (§9.1). The indices are serialised, so they are part of the contract — not just the labels.

## Four traps

1. **Clip the scope trace to the plot region, not the dark rectangle** (§5.1). The scope has three
   nested rectangles. The level labels are separated from the trace horizontally, in a reserved
   right-hand gutter — the trace must still reach 0 dB and −∞ vertically. Clamping to the wrong rect
   puts the trace back through the labels.

2. **Four knobs are power-law skewed** — `fraction = ((v − min)/(max − min)) ^ skew`, with skew 0.30
   (Trigger HP, Trigger LP, Release) and 0.25 (Attack). The printed marks were placed from this
   curve and your parameter mapping uses the same one, so the LCD and the tick agree by
   construction. **No lookup tables** — the LCD reads the parameter value directly.

3. **No knob is quantised.** Ticks are a scale, not detents. Only the Algorithm selector, KEY SOURCE
   and SHAPE are genuinely stepped. Do not add snapping or magnetic centres to anything else.

4. **The eight state-dependent labels are absent from the plate** (§0.4) — `INTERNAL` / `SIDECHAIN`,
   `HARD` / `SOFT`, and the four algorithm corners. Bare fascia sits where they go; draw all eight
   every frame from the position table in §0.4. They are the only text outside the LCDs the build
   touches.

## Program management (§6)

SAVE / DELETE are stamped-steel utility buttons right of the LCD.

- **SAVE is disabled until something is edited.** Any knob, switch or algorithm change marks the
  program dirty, enables SAVE, and appends ` *` to the name.
- **DELETE is disabled whenever a factory program is loaded** — factory programs are read-only.
- The FACT / USER tag inside the LCD is the read-only indicator.
- SAVE switches the LCD into name-entry with a blinking caret and relabels the buttons
  STORE / CANCEL. Names cap at 24 characters.
- While a control is moved the name cell shows `NAME: value unit`, reverting after ~800 ms. This is
  the only place a live number appears.

The name cell is 252 px wide — a 27-character budget at 13 px (§6.1). Every expected string clears
with room to spare, but the margin exists because the header was rearranged to create it. Don't
narrow the window without re-checking the four strings in that table.

## Non-negotiable

- Red `#FF2B1C` only on the GATE OPEN lamp and the envelope trace.
- Envelope corners are hard mitres — no curve smoothing, no easing on the plateau edges.
- Lamp lights on the same sample the gate opens.
- Nothing functional below 10 px; hierarchy through size and weight, never opacity.
- No rounded corners on the fascia, LCD windows, or switches.
