# ABSENT — gatecrasher/fonts/

This directory holds `TudorVictors.ttf` only. Two faces it draws with are not here, for two
different reasons, and the difference matters.

- **Barlow Condensed SemiBold (600)** — **absent by delivery route, not missing.** Every casting
  carries its own copies of the weights it uses; Gatecrasher's SemiBold resolves through the
  shared `designs/fonts` tree used by the prototypes. Declared in `GUI-SPEC.md` §8.

- **Barlow Condensed Medium (500)** — **absent by omission. Owed.** §8 asks for 500 in **five
  rows** (unit, scale numeral, scope legend, scope header data, and the numeral ring at §3.2's
  `r + 8 + 9 + 6 + 6.5`). The build draws all five in **SemiBold (600)**, and the rewrite made
  that substitution **without recording it** — which is the part that should not persist. The
  difference is invisible on the panel, the way a size 20 % small is.

  Ruled 2026-08-21: **the weight stands and the file ships.** Deliver the same OFL
  `BarlowCondensed-Medium.ttf` already cut for Fifth Member into this directory; the build picks
  it up with one line. §8's rows are **not** being changed to 600. Until it lands, the
  substitution must be stated at the call site, as TapeRot's is.

Register: `shared/FONTS.md`. An undeclared substituted face is what that register exists to
prevent — see *What a substituted face costs, stated once*.
