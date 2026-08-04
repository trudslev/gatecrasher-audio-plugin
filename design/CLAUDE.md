# Gatecrasher GR-85 — GUI handoff

You are building the JUCE plugin GUI for **Gatecrasher**, an 80s gated-reverb processor. The visual design is approved and final — implement it, don't redesign it.

Read `GATECRASHER-GUI-SPEC.md` first. It has the palette, every control coordinate, the filmstrip contract, and the scope drawing rules.

**Assets**
- `assets/gatecrasher-panel@2x.png` — approved reference, gate closed
- `assets/gatecrasher-panel-gate-open@2x.png` — gate open (lamp lit)
- `assets/knob_large_128px_128f.png` — 128 frames, knurled, −135°→+135°
- `assets/knob_small_128px_128f.png` — 128 frames, plain skirt
- `assets/header-factory-program@3x.png` — header, factory program (DELETE disabled)
- `assets/header-user-program@3x.png` — header, user program (DELETE enabled)
- `assets/header-name-entry@3x.png` — header, SAVE pressed / naming a program
- `assets/scope-hard-release@3x.png` — envelope with SHAPE = HARD
- `assets/scope-soft-release@3x.png` — envelope with SHAPE = SOFT
- Font `TudorVictors` is used for the wordmark only; bake the wordmark to PNG rather than drawing it (§7).

**Live reference**
`reference/Gatecrasher.dc.html` is the working mockup — open it in a browser to see the scope animating, the gate lamp firing, and the SAVE / DELETE / name-entry flow behaving. `reference/support.js` and `assets/TudorVictors.ttf` must sit alongside it (they do in this package). `reference/Knob Render Comparison.dc.html` is the code-drawn vs filmstrip comparison that settled the knob decision.

Read the mockup's source for exact gradients, shadows, and the scope's draw loop — every value in the spec came from it. Note the mockup uses inline CSS on a small custom runtime; treat it as a visual and behavioural reference, not as code to port.

**Decisions already made**
- Knobs are **filmstrips**, not code-drawn. Rendered at 128px so all panel sizes downscale.
- Panel is fixed 960×434 at 1×.
- Tick rings and the wordmark are drawn/blitted separately from the knob strips.

**Program management** (§6) — SAVE / DELETE are stamped-steel utility buttons right of the LCD. DELETE is disabled whenever a factory program is loaded. The FACT / USER tag inside the LCD is the read-only indicator. SAVE switches the LCD into name-entry with a blinking caret and relabels the buttons STORE / CANCEL.

**SHAPE switch** (§5) — HARD / SOFT toggle under the A/H/R knobs, same switch component as KEY SOURCE. It changes the envelope's closing edge, and the scope redraws to show it.

**Non-negotiable**
- Red `#FF2B1C` only on the GATE OPEN lamp and the envelope trace.
- Envelope corners are hard mitres — no curve smoothing, no easing on the plateau edges.
- Lamp lights on the same sample the gate opens.
- No rounded corners on the fascia, LED windows, or switch.
