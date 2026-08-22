# ABSENT — gatecrasher/fonts/

This directory holds no faces. Three are involved and the three reasons differ, which is the
whole content of this file.

- **Tudor Victors (the wordmark)** — **ABSENT BY LICENSING, and it must stay absent.** The
  delivered `.ttf` states its own terms in its `name` table: *"Typeface by Chequered Ink. © 2020.
  All Rights Reserved."* No Chequered Ink licence was bought, and the available licences grant use
  of the face to make things rather than the right to redistribute the file — so embedding it in a
  distributed binary is not covered **at either price**. Removed from this directory and from the
  prototype's `@font-face` in export 11.

  The letterforms ship as artwork instead, TapeRot's treatment: **`../assets/gatecrasher-wordmark.png`**,
  cut at 3× (699 × 120 raster, **233 × 40 drawn**) with the face checked loaded at cut time so the
  glyphs are the real ones. §2's "one binary ships: `fonts/TudorVictors.ttf`" is now wrong in both
  halves — no font ships and a PNG does.

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
