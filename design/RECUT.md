# RE-CUT SHEET — GATECRASHER GR-85

**Every row carries delivered *and* target dimensions.** A target dimension read without its
base is how three figures went wrong in this round: a needle height taken from a placement
offset, a plate "already 3×" of a canvas that no longer exists, a sprite "2×" against an old
frame. All three were true ratios with the base left out. This sheet exists so the target never
travels without it — `../MANIFEST.md` has the same rows for the whole suite.

**No re-cuts.** This casting is plateless — its fascia is a 2 px procedural repeat with nothing
that wants baking, so call 6’s per-casting permission applies (GUI-SPEC §10).

| Delivered | State |
|---|---|
| `icons/gatecrasher-icon-{1024,512,256,128,64,32,16}.png` | **done** — seven optical cuts, each drawn at its size, ladder in GUI-SPEC §11 |
| `fonts/TudorVictors.ttf` | **withdrawn — NOT distributable.** © Chequered Ink 2020, All Rights Reserved, read from the file itself; no licence bought and none available that covers redistribution. Replaced by `assets/gatecrasher-wordmark.png` |
| `assets/gatecrasher-wordmark.png` | **done** — wordmark cut at 3×, **699 × 120 raster / 233 × 40 drawn**, transparent ground, face verified loaded at cut time. Standalone cut, not a plate: this casting has none to bake into |
| `fonts/BarlowCondensed-Medium.ttf` | **OWED — not delivered.** §8 asks for weight 500 in five rows; the build draws all five in SemiBold (600) and the rewrite made that substitution **without recording it**. Ruled 2026-08-21: the weight stands and the file ships — the same OFL file already cut for fifth-member. See `fonts/ABSENT.md` |

## §3.2's HOLD ring, re-cut in the prototype (export 10)

**No bitmap and no geometry — a legend correction, and the only one on this casting's rings.**
HOLD's parameter is `NormalisableRange<float>(10.0f, 500.0f)`, linear and **starting at 10 ms**.
§3.2 printed its five numerals on **even fifths**, which is only correct if the range starts at 0,
so the three middle numerals each sat a little clockwise of where the pointer actually reaches
them — about **5.4 px of arc** at the Ø76 numeral radius at the worst mark.

| Printed | Was drawn at | Now drawn at | Was out by |
|---|---|---|---|
| 125 | −67.50° | **−71.63°** | 4.13° |
| 250 | 0.00° | **−2.76°** | 2.76° |
| 375 | +67.50° | **+66.12°** | 1.38° |

**The build wins** — `../shared/HEADER-PART.md` §12, which called for exactly this confirmation
against `NormalisableRange`. The round numerals are kept and the marks moved; the alternative was
changing the parameter to start at 0 ms, and a gate hold of 0 ms is not a usable value. **The
10 ms floor is not being changed**, and that remains a DSP decision rather than a drawing one.

**The four skewed rings named in §12 all passed** — HP's −58.05 / −3.51 / +69.12, LP's −45.05 /
−9.91 / +38.91 and ATTACK's 0.25 skew reproduce to well inside half a degree. **The skewed rings
were right and an unskewed one was not**, which is worth recording because §12's list was built
from the ones that looked risky.

If a wear layer is ever added to the fascia it becomes a plate and call 6 binds at 3×.
