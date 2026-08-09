# Runtime fonts

Two faces are drawn by the plugin at runtime and must be embedded in the binary. Both are
SIL Open Font License 1.1, so embedding is unrestricted.

| Family | Files needed | Drawn for |
|---|---|---|
| **Barlow Condensed** | `BarlowCondensed-Bold.ttf` (700)<br>`BarlowCondensed-SemiBold.ttf` (600)<br>`BarlowCondensed-Regular.ttf` (400) | The eight state-dependent labels (§0.4) — `INTERNAL` / `SIDECHAIN`, `HARD` / `SOFT`, and the four algorithm corners. These are absent from the plate; the build draws all eight every frame. Selected 700, unselected 400; the 600 weight is only needed if you also redraw a group caption. |
| **Share Tech Mono** | `ShareTechMono-Regular.ttf` (400) | LCD program name, live parameter values, IN / OUT meter values. |

Sources — both are on Google Fonts:

- Barlow Condensed — Jeremy Tribby, https://fonts.google.com/specimen/Barlow+Condensed
- Share Tech Mono — Ralph du Carrois, https://fonts.google.com/specimen/Share+Tech+Mono

Every other typographic element on the panel is baked into
`gatecrasher-panel-plate@2x.png` and needs no font at runtime.

`../TudorVictors.ttf` is **not** a runtime font. It is the wordmark face; the wordmark is baked to
the plate. It ships only so the wordmark can be re-rendered if the header layout ever changes.

## Metrics the redraw must match

From spec §2.3:

| Role | Face | Size | Weight | Tracking |
|---|---|---|---|---|
| Switch / algorithm label | Barlow Condensed | 10 px | 700 selected / 400 unselected | .10em || LCD program / value | Share Tech Mono | 13 px | 400 | .10em (1.3 px) |
| LCD IN / OUT | Share Tech Mono | 12 px | 400 | normal |

Colours: selected label `#16191C`, unselected `#2B3034`, LCD text `#F0E0B0` with a
`rgba(240,216,150,.35)` glow.

The LCD name cell budget in §6.1 (27 characters at 8.32 px advance) assumes Share Tech Mono at
13 px with 1.3 px letter-spacing. Substituting another mono face invalidates that measurement.
