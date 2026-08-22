# ABSENT — gatecrasher/fonts/

This directory holds **`BarlowCondensed-Medium.ttf`**. Two faces it draws with are not here, for
two different reasons, and the difference is the whole content of this file.

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

- ~~**Barlow Condensed Medium (500)**~~ — **ARRIVED 2026-08-22, export 12. No longer absent.**
  Kept struck through rather than deleted because a closed omission is the evidence that the
  register works; a deleted one looks like it never happened. 680 glyphs, `usWeightClass` 500,
  v1.408, OFL — the same cut TapeRot has.

  §8 asked for 500 in **four rows** — unit, scale numeral, scope legend (`500/600`) and scope
  header data. §3.2's numeral ring at `r + 8 + 9 + 6 + 6.5` is the **same role restated**, not a
  fifth row; this file said five until 2026-08-22, having counted mentions rather than rows. The build drew all four in **SemiBold (600)** and the rewrite made that
  substitution **without recording it** — which was the part that should not persist; the difference
  is invisible on the panel, the way a size 20 % small is. **The prototype never substituted**: it
  has drawn `font-weight: 500` at all four roles throughout, so nothing in the artwork moves now
  that the face is here.

  Ruled 2026-08-21: **the weight stands and the file ships** — and it now has, to both castings.
  §8's rows were never changed to 600; `tools/check_font_sets.py` was the closure condition and
  should be quiet on this face. Deliver the same OFL
  `BarlowCondensed-Medium.ttf` already cut for Fifth Member into this directory; the build picks
  it up with one line. §8's rows are **not** being changed to 600. Until it lands, the
  substitution must be stated at the call site, as TapeRot's is.

Register: `shared/FONTS.md`. An undeclared substituted face is what that register exists to
prevent — see *What a substituted face costs, stated once*.
