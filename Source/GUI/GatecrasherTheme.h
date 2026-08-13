#pragma once

#include <juce_graphics/juce_graphics.h>
#include <BinaryData.h>
#include <array>
#include <cmath>

// Centralises every pixel constant from design/GUI-SPEC.md (palette, coordinates,
// filmstrip contract) in one place, mirroring TapeRotTheme.h's role for TapeRot - components pull
// from GatecrasherTheme::Colour/Layout rather than hardcoding numbers. Unlike TapeRotTheme, most of
// the fascia here is a static bitmap (see GatecrasherPanelBackground) rather than code-drawn, so
// this file's job is narrower: positions/sizes for the *live* pieces layered on top, plus the
// handful of colours those live pieces need to match the baked artwork around them.
#include <nf/ParameterReadout.h>

namespace GatecrasherTheme
{
    namespace Colour
    {
        // Section 2 palette. Every static label is baked into the plate, so the only fascia ink
        // still drawn in code is the EIGHT state-dependent labels of section 0.4 - which is why
        // this list is now two entries rather than a dozen.
        //
        // Both values are judged against the DARKEST point of the fascia band each label is drawn
        // over, where contrast is worst. Rev 5's #7B8287 inactive grey (1.60:1 there) is DELETED -
        // if it turns up again, it is a regression.
        //
        // The figures below are sampled off the shipped plate by tools/check_contrast.py rather
        // than quoted from the spec. They previously read 7.49 and 5.66, which the plate did not
        // produce - and labelSelected was the case those numbers were chosen to protect, so it had
        // been sitting at 6.90 against its own 7:1 bar.
        // contrast: 7.09-8.43:1 vs plate:keySourceLabels,plate:shapeLabels [functional]
        inline const juce::Colour labelSelected{0xFF141619};     // 700 weight, per section 2
        // contrast: 5.21-6.20:1 vs plate:keySourceLabels,plate:shapeLabels [functional exempt: section 2.1 dims the inactive half deliberately - see this repo's CLAUDE.md, "do not fix it"]
        inline const juce::Colour labelUnselected{0xFF2B3034};   // 400 weight


        inline const juce::Colour ledWindowBg{0xFF07090A};
        inline const juce::Colour ledWindowBorder{0xFF363C41};
        // Section 6.2. Rev 6 warmed this from the old #DFE6EA to amber; the glow is drawn with it.
        inline const juce::Colour ledText{0xFFF0E0B0};
        inline const juce::Colour ledGlow{juce::Colour::fromRGBA(240, 216, 150, 89)};   // .35

        // "Gate accent (ONLY colour on panel)" - reserved exclusively for the GATE OPEN lamp and
        // the envelope trace, per spec section 1's explicit rule and BRAND.md's one-accent-colour
        // rule. Never used for any knob/label/meter.
        inline const juce::Colour gateAccent{0xFFFF2B1C};
        inline const juce::Colour lampUnlit{0xFF3A1512};


        // Switch (KEY SOURCE / SHAPE) - recessed track + sliding metal shoe, section 5/7.
        inline const juce::Colour switchTrackBg{0xFF07090A};
        inline const juce::Colour switchTrackBorder{0xFF353B40};
        inline const juce::Colour switchShoeTop{0xFF8D959B};
        inline const juce::Colour switchShoeBottom{0xFF4E545A};

        // Gate envelope scope, section 5.
        inline const juce::Colour scopeBorder{0xFF0A0C0D};
        // Section 5.1: the dark rect runs #0B0F11 at the top to #050708 at the bottom. These were
        // inverted (and the bottom value wrong) through Rev 5.
        inline const juce::Colour scopeBgTop{0xFF0B0F11};
        inline const juce::Colour scopeBgBottom{0xFF050708};
        // The title strip and scale gutter: flat, slightly darker than the plot backing so they
        // read as chrome, with a 1px rule separating each from the plot.
        inline const juce::Colour scopeStrip{0xFF080B0D};
        inline const juce::Colour scopeStripRule{0x2E96B4BE};   // rgba(150,180,190,.18)
        inline const juce::Colour scopeGrid{0x1A96B4BE};       // rgba(150,180,190,.10)
        inline const juce::Colour scopeBaseline{0x3896B4BE};   // rgba(150,180,190,.22)
        inline const juce::Colour scopeInputWaveform{0x4DB2BEC5}; // rgba(178,190,197,.30)
        // **Opaque, and that alone was the fix.** This draws GATE ENV, 0 dB and -60 dB - printed
        // scales, which BRAND.md names as functional text. At rgba(160,178,186,.55) it read
        // 3.40:1; the same colour opaque reads 8.78, so nothing here was ever a colour choice,
        // only a blend. CHORUS-60 deleted this exact rgba value and recorded the measurement that
        // condemned it; the fix never crossed over.
        // contrast: 8.78-9.20:1 vs scopeBgTop,scopeBgBottom [functional]
        inline const juce::Colour scopeAnnotation{0xFFA0B2BA};
        inline const juce::Colour scopeFillTop{0x4DFF2B1C};    // rgba(255,43,28,.30)
        inline const juce::Colour scopeFillBottom{0x05FF2B1C}; // rgba(255,43,28,.02)

        // GATE OPEN lamp, section 5.
        inline const juce::Colour lampOpenCore{0xFFFF2B1C};
        inline const juce::Colour lampOpenMid{0xFFB0140C};
        inline const juce::Colour lampOpenEdge{0xFF6D0B06};

        // Input meter, section 7.
        inline const juce::Colour meterLitSegment{0xFFF4F8FA};
        inline const juce::Colour meterBloom{0x8CE6F2F8}; // rgba(230,242,248,.55)
        inline const juce::Colour meterThresholdMarker{0x80FFFFFF};

        // Program header LED tag/name text: tagFactory, tagUser and headerName are GONE, along
        // with tickMark and programCellDivider. All five were dead. A dead colour is not inert - it
        // reads as a sanctioned choice, so the next person wanting "the header's grey" finds one
        // already named and blessed and reintroduces a value nothing has drawn in months.

        // SAVE/DELETE (STORE/CANCEL) stamped-steel utility buttons, section 6's state table. Drawn
        // live rather than via the header-state bitmaps - see ProgramHeader.cpp's class comment for
        // why the bitmap-crop approach was abandoned in favour of this.
        /** **These two caps are DARK, and they are the only two controls on the panel that are.**

            It follows from the legend being the lamp: on a pale fascia a bright legend has nowhere
            brighter to go, so lit type could not read as lit. They end up reading as a pair with
            the LCD and the two meter windows rather than as an anomaly - five dark apertures
            across one 34px band.

            **There is no disabled face.** Cap, border, inner highlight and drop are identical in
            all five panel states; only which legend is backlit changes. The pale enabled cap
            (#DBE0E3 -> #AAB1B6) and the separate disabled cap (#C2C8CC -> #A8AFB3) are both gone,
            along with the #55595C disabled label - which had itself just been rescued from
            #8B9297 at .55 alpha, measuring 1.21:1, the worst reading in the suite. The right
            answer turned out not to be a better disabled colour but no disabled state at all. */
        inline const juce::Colour buttonCapTop{0xFF23282C};
        inline const juce::Colour buttonCapBottom{0xFF14181B};
        inline const juce::Colour buttonCapBorder{0xFF43494E};
        inline const juce::Colour buttonPressedTop{0xFF12161A};
        inline const juce::Colour buttonPressedBottom{0xFF20252A};

        /** Lit is a neutral bright, never the accent #FF2B1C - that stays reserved for the gate
            and the envelope trace.
            // contrast: 15.90:1 vs buttonCapTop [functional] */
        inline const juce::Colour legendLit{0xFFF4F8FA};
        /** Unlit is printed and readable, not absent: both legends dark has to read as "nothing to
            do here", never as a blank button.
            // contrast: 5.20:1 vs buttonCapTop [state] */
        inline const juce::Colour legendUnlit{0xFF9AA1A6};
    }

    // The knob filmstrips ship in two skirt styles (see design/GUI-SPEC.md's asset list) -
    // which one a given knob uses is part of its identity in the section 3 coordinate table.
    enum class KnobFilmstripSize { large, small };

    namespace Layout
    {
        constexpr float canvasWidth = 960.0f;
        constexpr float canvasHeight = 434.0f;

        // Rotation range for every knob: pointer at 12 o'clock = centre (section 3).
        /** 190px of vertical drag spans the full range, 760 while Shift is held. Suite figures: six
            castings had six drag feels - this one was on JUCE's untouched 250 AND was the only one
            responding to horizontal drag - so the same hand got a different response from each. */
        constexpr int knobDragPixels = 190;
        constexpr int knobFineDragPixels = 760;

        constexpr float knobArcStartDegrees = -135.0f;
        constexpr float knobArcEndDegrees = 135.0f;

        // No tick-ring constants here on purpose. Rev 5 drew the rings itself at even angular
        // spacing (15 degrees large / 21 small); Rev 6 bakes every tick into the plate at its
        // LABELLED value instead (section 0.3), which on the four skewed controls is not evenly
        // spaced at all. Reintroducing a drawn ring would lay even ticks over uneven printed ones.

        // Filmstrip frames are square with transparent margin for the baked cast shadow - draw into
        // the full FRAME box, not just the knob circle. Section 1.3 states the ratio as a contract:
        // the cap is 0.75 of the frame (120px of cap in a 160px frame), so the box is 1.333 x the
        // section-3 diameter, centred on the section-3 centre.
        //
        // This was 1.07 while the strips were 128px frames around the same 120px cap, and at that
        // ratio the frames clipped their own cast shadow - border alpha was still 88 top / 95 bottom
        // / 38 sides, so every knob sat inside a hard-edged dark rectangle instead of a soft shadow.
        // Rev 9 re-rendered both strips at 160px for exactly this. The cap diameters in section 3 did
        // not change and must not be adjusted to compensate; only the transparent margin grew.
        constexpr float knobBoundingBoxBleed = 1.3333333f;

        /** Side of one filmstrip frame, in the strip's own pixels (section 1.3). */
        constexpr int knobFilmstripFramePx = 160;
        constexpr int knobFilmstripFrameCount = 128;

        /** How far past the cap's edge a click still counts as the knob's. The frame box reaches
            .167 of the diameter beyond the cap now, which on THRESHOLD is 10px of transparent
            margin lying over printed scale numerals - so the hit area is taken from the CAP, not
            from the component's bounds. */
        constexpr float knobClickMargin = 3.0f;

        /** A knob's identity: which parameter, where its DIAL CENTRE sits, how big, and which
            filmstrip. Spec section 3.

            This used to also carry the engraved label, its type metrics, and the live readout's
            format and position - all consumed by PanelChrome and PanelReadouts. Both are gone: the
            labels are baked into the plate and the standing readouts were removed outright (section
            0.2, 6.3), so the value now appears only in the LCD while a control is moved. */
        struct KnobSpec
        {
            const char* paramID;
            float cx, cy, diameter;
            KnobFilmstripSize size;
        };

        // Section 3's full 15-knob table. density/decay are deliberately absent - automation-only,
        // no panel control (section 9 / GatecrasherEditorContent's own comment).
        /** Spec section 3, verbatim. Every one of these moved in the Rev 6/7 redesign - the
            previous values were Rev 5's and were out by up to 74px, which put knobs on top of their
            own printed numerals once the plate started carrying the scales. Take them from the
            spec, never from a screenshot. */
        inline constexpr std::array<KnobSpec, 15> knobs{ {
            {"threshold", 102.0f, 170.0f, 62.0f, KnobFilmstripSize::large},
            {"trigHP",     75.0f, 302.0f, 32.0f, KnobFilmstripSize::small},
            {"trigLP",    151.0f, 302.0f, 32.0f, KnobFilmstripSize::small},
            {"attack",    259.0f, 289.0f, 54.0f, KnobFilmstripSize::large},
            {"hold",      361.0f, 289.0f, 54.0f, KnobFilmstripSize::large},
            {"release",   463.0f, 289.0f, 54.0f, KnobFilmstripSize::large},
            {"algorithm", 632.0f, 154.0f, 50.0f, KnobFilmstripSize::large},
            {"size",      580.0f, 239.0f, 44.0f, KnobFilmstripSize::large},
            {"preDelay",  684.0f, 239.0f, 44.0f, KnobFilmstripSize::large},
            {"dampHF",    583.0f, 364.0f, 34.0f, KnobFilmstripSize::small},
            {"dampLF",    681.0f, 364.0f, 34.0f, KnobFilmstripSize::small},
            {"slam",      793.0f, 157.0f, 40.0f, KnobFilmstripSize::large},
            {"width",     881.0f, 157.0f, 40.0f, KnobFilmstripSize::large},
            {"mix",       793.0f, 262.0f, 40.0f, KnobFilmstripSize::large},
            {"trim",      881.0f, 262.0f, 40.0f, KnobFilmstripSize::large},
        } };

        // Gate envelope scope, section 5.
        // Spec section 5.1: THREE nested rectangles, and they must stay distinct. The well is the
        // baked recess; the dark rect is the full painted canvas; the plot region is the only area
        // the trace may occupy. Clipping the trace to the dark rect instead of the plot region is
        // the specific mistake section 5.1 exists to prevent - it lets the trace run under the
        // scale gutter and collide with the level annotations.
        //
        // These were 218/113/344x122 through Rev 5, which is 43px wider than the plate's actual
        // recess - wide enough that the scope's right edge overlapped the algorithm selector's ROOM
        // and AMBI labels once those started being drawn.
        constexpr float scopeWellX = 208.0f, scopeWellY = 118.0f, scopeWellW = 305.0f, scopeWellH = 116.0f;
        constexpr float scopeDarkX = 210.0f, scopeDarkY = 120.0f, scopeDarkW = 301.0f, scopeDarkH = 112.0f;

        // Plot region, local to the dark rect's top-left.
        constexpr float scopePlotLocalX = 0.0f, scopePlotLocalY = 14.0f;
        constexpr float scopePlotW = 267.0f, scopePlotH = 98.0f;

        // The two reserved strips carved out of the dark rect, also local.
        constexpr float scopeTitleStripH = 14.0f;   // 0,0,301x14 - holds GATE ENV
        constexpr float scopeGutterLocalX = 267.0f; // 267,14,34x98 - holds 0 dB and -inf
        constexpr float scopeGutterW = 34.0f;

        constexpr float scopePixelsPerFrame = 2.0f;
        constexpr float scopeGridSpacing = 44.0f;
        constexpr int scopeNumStaticHorizontals = 5;

        // Spec section 5.5: centre (216, 104), diameter 15. This read (224, 95) until Rev 7 - wrong
        // by (+8, -9), and masked for as long as the plate baked an unlit bulb at the correct spot:
        // the dark disc you saw was the ARTWORK's, with the drawn lamp sitting offset on top of it.
        // Rev 7 removed the baked bulb, so the drawn position is now the only one and had to be
        // right. If the lamp ever looks displaced from its GATE OPEN legend, start here.
        constexpr float lampCx = 216.0f, lampCy = 104.0f, lampDiameter = 15.0f;

        //======================================================================================
        /** The eight state-dependent labels of spec section 0.4 - the ONLY text this build draws
            outside the two LCD windows.

            They are absent from the plate: bare fascia sits where they go, and all eight are drawn
            every frame at whichever weight their control is currently in. Rev 6 baked them at their
            defaults and asked for the changed pair to be redrawn, which cannot work - baked pixels
            cannot be un-drawn, so turning a bold baked word dim would have meant painting matched
            fascia over it first. Rev 7 withdrew that; do not reintroduce it.

            **Draw from `x`, never by re-centring on `centreX`.** A 700-weight word is wider than the
            same word at 400, so re-centring would shift it sideways as the control changes. The
            centre is recorded only because the spec quotes it. */
        struct StateLabel
        {
            const char* text;
            float x;          // left edge, spec section 0.4
            float baselineY;
            float centreX;    // rendered default's centre - reference only, do not lay out from it
        };

        /** Index pairs: 0/1 are KEY SOURCE (Internal/Sidechain), 2/3 SHAPE (Hard/Soft), 4-7 the
            algorithm corners in ROOM, PLATE, AMBI, CHMBR order - note that is panel order, NOT the
            parameter's index order, which section 9.1 gives as Ambience, Room, Plate, Chamber. */
        inline constexpr std::array<StateLabel, 8> stateLabels { {
            { "INTERNAL",   64.0f, 416.0f,  85.7f },
            { "SIDECHAIN", 117.0f, 416.0f, 139.7f },
            { "HARD",      333.0f, 422.0f, 345.0f },
            { "SOFT",      367.0f, 422.0f, 377.0f },
            { "ROOM",      557.0f, 127.0f, 568.4f },
            { "PLATE",     680.0f, 127.0f, 693.3f },
            { "AMBI",      557.0f, 185.0f, 567.0f },
            { "CHMBR",     679.0f, 185.0f, 692.8f } } };

        /** Section 2.3: Barlow Condensed 10px, .10em tracking. */
        constexpr float stateLabelCssPx = 10.0f;
        constexpr float stateLabelTrackingEm = 0.10f;

        // Input meter, section 8: x 165, y 139, 14 x 76. This read (147, 133) through Rev 5, whose
        // header layout put the meter 18px further left; drawn there against the Rev 7 plate it
        // painted a second column of segments straight over the printed "-15" scale numeral while
        // the real baked well sat empty beside it. Measured back off the plate to confirm: the well
        // spans x 165.0-180.5, y 139.0-214.5 including its 1px border.
        constexpr float meterX = 165.0f, meterY = 139.0f, meterW = 14.0f, meterH = 76.0f;
        constexpr float meterSegmentH = 4.0f, meterSegmentPitch = 6.0f;
        // Meter/marker dB range: matches the Threshold parameter's own -60..0dB range (section 9)
        // so the threshold marker is meaningful relative to the lit segments.
        constexpr float meterFloorDb = -60.0f, meterCeilingDb = 0.0f;

        // KEY SOURCE / SHAPE switches - identical track geometry (section 5: "reuses the KEY
        // SOURCE switch verbatim"). Caption/label rows are derived around each track with the same
        // internal proportions (spec gives the track/assembly anchors, not sub-pixel caption/label
        // baselines - these are a careful, symmetric interpolation between the two, not lifted
        // directly from a pixel-measured mockup).
        constexpr float switchTrackW = 58.0f, switchTrackH = 22.0f;   // section 7
        constexpr float switchShoeW = 26.0f;

        // The switch's full assembly (caption row / track / label row, stacked) is what actually
        // gets a Component - a plain juce::Slider maps click/drag position against its own full
        // bounds, so sizing the interactive area to just the 56x20 track (with caption/labels
        // painted externally, TapeRot ToggleSwitch-style) isn't an option here: there's no
        // external static painter that can react to live active/inactive label colour the way
        // SectionPanel does for TapeRot. Splitting the assembly height into a 16px caption row and
        // a 20px label row around the 20px track is this codebase's own layout choice (the spec
        // doesn't restate sub-pixel caption/label baselines for either switch).
        constexpr float switchAssemblyPad = 1.0f;
        constexpr float switchCaptionRowH = 16.0f;
        constexpr float switchLabelRowH = 20.0f;
        constexpr float switchAssemblyW = switchTrackW + 2.0f * switchAssemblyPad;
        constexpr float switchAssemblyH = switchCaptionRowH + switchTrackH + switchLabelRowH;

        // Verified by cropping the background directly: the baked option-label row (e.g.
        // "INTERNAL"/"SIDECHAIN") actually spans well beyond the 58px-wide track assembly on both
        // sides - a real hardware label pair next to a narrow physical switch, not confined to the
        // switch's own footprint. ToggleSwitchComponent's erase-then-redraw can only reach pixels
        // within its own Component bounds (JUCE clips child painting there), so the component
        // itself needs this much extra width on each side, with the track/caption re-centred within
        // it, or the erase leaves the baked label's outer portions un-erased - which is exactly the
        // "KEY SOURCE"/"INTERNAL SIDECHAIN" ghosting this constant fixes.
        constexpr float switchLabelOverflowPad = 45.0f;

        // Vertical counterpart to switchLabelOverflowPad: the baked assembly's true position turned
        // out not to line up with a naive reading of either switch's own spec coordinates (see
        // shapeTrackY below), and pixel-cropping the raw asset to find the real position is
        // inherently a few px imprecise - so the component's bounds get this much extra margin top
        // and bottom, purely as a safety margin against that imprecision (the option-label row's own
        // fix no longer depends on it, see ToggleSwitchComponent's paint()).
        constexpr float switchVerticalSafetyPad = 12.0f;

        // Spec section 7, taken directly: KEY SOURCE track x 84, y 379; SHAPE x 332, y 385; both
        // 58 x 22. Every one of these moved in the Rev 6/7 redesign - the previous values (88/371
        // and 362/380 at 56 x 20) were Rev 5's, and the long archaeology that produced them is no
        // longer relevant now that the plate is authoritative and the spec quotes the track rect
        // directly. Take them from section 7, not from a crop of the render.
        constexpr float keySourceTrackX = 84.0f, keySourceTrackY = 379.0f;
        constexpr float shapeTrackX = 332.0f, shapeTrackY = 385.0f;

        // Option-label ("INTERNAL"/"SIDECHAIN", "HARD"/"SOFT") layout, all measured directly off
        // gatecrasher-panel@2x.png by cropping the baked label rows and scaling back to canvas units
        // (KEY SOURCE and SHAPE independently agree on every value below).
        //
        // The two words are laid out as a TIGHT PAIR centred on the track's centre with a fixed gap
        // between them - each word's own rendered width decides where it sits, so the pair's centres
        // land at different offsets per switch (measured: +-25px for INTERNAL/SIDECHAIN, but only
        // +-14px for the much shorter HARD/SOFT). An earlier version instead centred one label in
        // each half of this component's full padded width, which forced a fixed ~74px separation and
        // flung them far wider apart than the artwork on both switches.
        constexpr float switchOptionLabelGapX = 9.0f;
        // Vertical centre of the label text, below the track's bottom edge.
        constexpr float switchOptionLabelCentreBelowTrack = 13.0f;
        // Reference width of the baked "INTERNAL" (8 glyphs, 38px in canvas units) - the live font
        // height is solved from this at runtime via labelFontHeightForTrackedWidth rather than
        // hard-coded, see that function's comment for why a px size read off the spec doesn't work.
        constexpr const char* switchOptionLabelRefText = "INTERNAL";
        constexpr float switchOptionLabelRefWidth = 38.0f;
        constexpr float switchOptionLabelRefCssPx = 9.0f; // the mockup's size for this same label
        constexpr float switchOptionLabelTracking = 0.6f;

        // Caption ("KEY SOURCE" / "SHAPE") sits this far above the track's top edge, measured off
        // the dressed reference render - notably NOT switchCaptionRowH * 0.5, which the earlier
        // caption-row model assumed and which sits ~4px too low.
        constexpr float switchCaptionCentreAboveTrack = 11.75f;

        // Spec section 6, taken directly. The header was rearranged in Rev 6: the strapline and
        // model line moved from beside the wordmark to underneath it, which freed 74px that went
        // into the program window - the name cell grew 197 -> 252 and everything right of it moved.
        //
        // All of these were previously measured off the dressed render rather than read from the
        // spec, because the plate and the spec used to disagree. They no longer do: section 6 and
        // the Rev 7 plate agree, so these are the spec's numbers.
        // Section 6.2's LCD caption. Rev 8 removed it from the plate: it swaps to NAME PROGRAM
        // during name entry, which makes it state-dependent, and a baked copy would have to be
        // painted over to change - the technique section 0.4 withdrew. Bare fascia sits here now.
        // Drawn from the LEFT x, never re-centred, for the same reason the section-0.4 labels are:
        // the two words are different widths (48.5px and 80.9px) and re-centring would slide the
        // caption sideways every time SAVE was pressed.
        /** **The baseline moved up 6.7px with the 34px row**, and drawing at the old 27.75 lands
            the caption inside the LCD's top border. It was right for a 25px window; the taller one
            pushed the caption+window group's centring up the band. */
        constexpr float programCaptionX = 374.75f, programCaptionBaselineY = 21.08f;
        constexpr float programCaptionCssPx = 10.0f, programCaptionTrackingEm = 0.22f;

        /** **The header band: y 29, height 34, shared by all five parts** - the LCD, both Program
            buttons and both meter windows. 34 is BRAND.md's suite figure rather than this panel's:
            the castings are differently-sized units, not scales of one design.

            Gatecrasher had the shallowest band in the suite at 25px, which is why its buttons were
            the one place two 10px legends provably would not fit - 20px of ink before any leading
            or padding, in a 25px button. The band grew rather than the legends shrinking, because
            10px is BRAND.md's floor for functional text and both legends are functional.

            Every figure here is BORDER-BOX, and the plate agrees: its LCD, IN and OUT wells all
            measure y 29..63 and the two button positions are bare fascia. A plate from before
            Rev 15 leaves 5px of stale dark well above the live LCD and 6px below it. */
        constexpr float programWindowX = 374.75f, programWindowY = 29.0f, programWindowW = 330.0f, programWindowH = 34.0f;
        constexpr float programTagCellX = 375.75f, programTagCellY = 30.0f, programTagCellW = 48.28f, programTagCellH = 32.0f;
        constexpr float programNameCellX = 424.03f, programNameCellY = 30.0f, programNameCellW = 253.72f, programNameCellH = 32.0f;
        constexpr float programChevronCellX = 677.75f, programChevronCellY = 30.0f, programChevronCellW = 26.0f, programChevronCellH = 32.0f;

        constexpr float saveButtonX = 710.75f, saveButtonY = 29.0f, saveButtonW = 50.2f, saveButtonH = 34.0f;
        constexpr float deleteButtonX = 765.95f, deleteButtonY = 29.0f, deleteButtonW = 50.2f, deleteButtonH = 34.0f;

        /** The two stacked legends, positioned by BASELINE rather than by box, because that is what
            the spec quotes and what keeps the pair optically even inside a 34px cap. */
        constexpr float legendUpperBaselineY = 41.08f;
        constexpr float legendLowerBaselineY = 53.08f;
        constexpr float legendCssPx = 10.0f;
        constexpr float legendTrackingEm = 0.10f;

        // 34 tall on y 29, like every other part of the band. These were 24 - one pixel shorter
        // than the 25px LCD and buttons beside them, which is the drift the suite audit found.
        constexpr float inWindowX = 825.0f, inWindowY = 29.0f, inWindowW = 44.0f, inWindowH = 34.0f;
        constexpr float outWindowX = 875.0f, outWindowY = 29.0f, outWindowW = 44.0f, outWindowH = 34.0f;

        /** Section 6.1's name-cell budget: **253.72px cell**, 10px padding each side, 233.72
            usable, Share Tech Mono 13px at .10em = 8.32px per character = **28 characters**.
            Program names are capped at 24, which with a two-digit index and a space is 27 - one
            inside the budget. Verified against the real content: the longest factory name with its
            index ("14 ROOM REINFORCEMENT") is 174.7px with 59px spare, and the longest live value
            ("THRESHOLD: -18.5 dB") 158.1px.

            **The budget grew 27 -> 28 with the 34px header row** (the cell went 252 -> 253.72) and
            the cap stayed at 24. That direction is the rule: a budget may grow, a cap may never
            shrink - lowering it would orphan names already saved to disk, which load and then
            cannot be saved back under their own name.

            Do not narrow the window without re-checking those strings. */
        constexpr float programNameCellPadding = 10.0f;
        constexpr int programNameCharBudget = 27;

        // Reference width of the baked "-6.2" readout in canvas units - the mono font's height is
        // solved from this at runtime (see fontHeightForTrackedWidth) rather than hard-coded.
        constexpr const char* meterReadoutRefText = "-6.2";
        constexpr float meterReadoutRefWidth = 30.0f;
        constexpr float meterReadoutRefCssPx = 12.0f; // the mockup's size for these windows

        // ---- Static chrome (PanelChrome) ----------------------------------------------------
        // Column centres and the divider positions come from computing the reference mockup's own
        // flex layout (26px rails, 10px body padding, 9px gaps, columns 160/1/348/1/208/1/113).
        // That computation independently reproduces this spec's stated divider positions
        // ("x ~= 208, 573, 800") and its scope rect ("x 218, w 344"), so it's the layout the
        // artwork was actually built from. Everything else below was measured off the dressed
        // reference render, gatecrasher-panel@2x.png, the same way the switch labels were.
        constexpr float leftColCentreX = 117.0f;
        constexpr float centreColCentreX = 390.0f;
        constexpr float reverbColCentreX = 687.0f;
        constexpr float outputColCentreX = 866.5f;
        constexpr float sectionHeadingCentreY = 109.0f;

        constexpr float subtitleX = 286.0f;
        constexpr float subtitleLine1CentreY = 32.25f;
        constexpr float subtitleLine2CentreY = 44.75f;
        constexpr float headerCaptionCentreY = 23.0f; // PROGRAM / IN / OUT captions share this row

        constexpr float triggerFilterLabelCentreY = 251.0f;
        constexpr float tankDampingLabelCentreY = 326.5f;
        constexpr float versionCentreY = 406.0f;

        // The meter's recessed window frame; InputMeter fills and draws segments inside it.

        // "ENVELOPE   50 ms / DIV", right-aligned to the scope's right edge.
        constexpr float envelopeAnnotationRight = 562.0f;
        constexpr float envelopeAnnotationCentreY = 95.0f;
        constexpr float envelopeAnnotationGap = 15.0f;

        // Algorithm selector's four corner labels, around the 120x78 block centred on the reverb
        // column. Which one is lit tracks the live Algorithm parameter, so PanelReadouts draws
        // these rather than PanelChrome.
        constexpr float algoLabelLeftX = 627.0f, algoLabelRightX = 746.0f;
        constexpr float algoLabelTopCentreY = 133.0f, algoLabelBottomCentreY = 198.5f;

        constexpr int maxProgramNameLength = 25; // mirrors ProgramManager::maxProgramNameLength

        /** **The readout revert lives in core now - `nf::ReadoutFormat::revertMs`, 900 ms.**

            It was 800 here. The suite ran 800 / 900 / 1100 / 1200 under three different constant
            names and two mechanisms, and no spec anywhere justified any of them; 900 is what three
            castings already had. `ProgramHeader::readoutFormat()` is where this panel states its
            readout spelling, and the delay comes with it rather than being a separate number here
            that nothing binds to the others.

            Left as a comment rather than deleted silently, because a reader looking for the old
            constant should find out where it went rather than conclude the revert was removed. */

        // Wordmark, section 8. Baked into the plate as of Rev 6; nothing draws it.
        constexpr float wordmarkX = 38.0f, wordmarkY = 20.0f, wordmarkW = 232.0f, wordmarkH = 40.0f;
        constexpr float wordmarkHeight = 36.0f; // the mockup's font-size for the stencil

        // The baked wordmark PNG's rect: the nameplate block above, padded 16px on every side. The
        // overspray halo spreads up to 8px (one pass offset a further -4px) and the spatter flecks
        // sit outside the text box, so a tight crop to wordmarkX/Y/W/H would clip them.
        constexpr float wordmarkArtX = wordmarkX - 16.0f, wordmarkArtY = wordmarkY - 16.0f;
        constexpr float wordmarkArtW = wordmarkW + 32.0f, wordmarkArtH = wordmarkH + 32.0f;

        // Width generously oversized (was 130, tight against the baked text's own true extent -
        // see GateLamp.cpp's class comment) so the erase-then-redraw this label does is guaranteed
        // to fully cover whatever's baked underneath regardless of exact glyph-width uncertainty;
        // there's clear space before the scope's own "ENVELOPE ..." readout to the right.
    }

    // Angle (degrees, clockwise from 12 o'clock) for a normalised 0..1 value across the knob arc.
    inline float knobAngleForValue01(float value01) noexcept
    {
        return Layout::knobArcStartDegrees
             + value01 * (Layout::knobArcEndDegrees - Layout::knobArcStartDegrees);
    }

    // Unit direction vector for an angle measured clockwise from 12 o'clock.
    inline juce::Point<float> directionForAngleDegrees(float degrees) noexcept
    {
        const float radians = juce::degreesToRadians(degrees);
        return {std::sin(radians), -std::cos(radians)};
    }

    inline juce::Point<float> pointOnCircle(juce::Point<float> centre, float radius, float angleDegrees) noexcept
    {
        return centre + directionForAngleDegrees(angleDegrees) * radius;
    }


    inline float trackedTextWidth(const juce::String& text, const juce::Font& font, float trackingPx)
    {
        float width = 0.0f;
        for (int i = 0; i < text.length(); ++i)
        {
            width += juce::GlyphArrangement::getStringWidth(font, juce::String::charToString(text[i]));
            if (i < text.length() - 1)
                width += trackingPx;
        }
        return width;
    }

    // juce::Font has no absolute-pixel letter-spacing, so this draws glyph-by-glyph to reproduce
    // the spec's tracking values (e.g. ".14-.20em" on control labels) - same technique as
    // TapeRotTheme::drawTrackedText.
    inline void drawTrackedText(juce::Graphics& g, const juce::String& text, const juce::Font& font,
                                 float trackingPx, juce::Rectangle<float> area,
                                 juce::Justification justification, juce::Colour colour)
    {
        g.setFont(font);
        g.setColour(colour);

        const float totalWidth = trackedTextWidth(text, font, trackingPx);
        float x = area.getX();
        if (justification.testFlags(juce::Justification::horizontallyCentred))
            x = area.getCentreX() - totalWidth * 0.5f;
        else if (justification.testFlags(juce::Justification::right))
            x = area.getRight() - totalWidth;

        for (int i = 0; i < text.length(); ++i)
        {
            const auto ch = juce::String::charToString(text[i]);
            const float charWidth = juce::GlyphArrangement::getStringWidth(font, ch);
            g.drawText(ch, juce::Rectangle<float>(x, area.getY(), charWidth + 1.0f, area.getHeight()),
                       juce::Justification::centredLeft, false);
            x += charWidth + trackingPx;
        }
    }

    // Barlow Condensed SemiBold (600, labels) / Bold (700, lamp/group-title text) and Share Tech
    // Mono Regular (numeric/LED readouts), per GUI-SPEC.md section 2. Loaded once per
    // process via function-local statics, same caching pattern as tudorVictorsTypeface() below.
    inline juce::Typeface::Ptr barlowSemiBoldTypeface()
    {
        static const juce::Typeface::Ptr typeface =
            juce::Typeface::createSystemTypefaceFor(BinaryData::BarlowCondensedSemiBold_ttf,
                                                      (size_t) BinaryData::BarlowCondensedSemiBold_ttfSize);
        return typeface;
    }
    inline juce::Typeface::Ptr barlowRegularTypeface()
    {
        static const juce::Typeface::Ptr typeface =
            juce::Typeface::createSystemTypefaceFor(BinaryData::BarlowCondensedRegular_ttf,
                                                      (size_t) BinaryData::BarlowCondensedRegular_ttfSize);
        return typeface;
    }

    inline juce::Typeface::Ptr barlowBoldTypeface()
    {
        static const juce::Typeface::Ptr typeface =
            juce::Typeface::createSystemTypefaceFor(BinaryData::BarlowCondensedBold_ttf,
                                                      (size_t) BinaryData::BarlowCondensedBold_ttfSize);
        return typeface;
    }
    inline juce::Typeface::Ptr shareTechMonoTypeface()
    {
        static const juce::Typeface::Ptr typeface =
            juce::Typeface::createSystemTypefaceFor(BinaryData::ShareTechMonoRegular_ttf,
                                                      (size_t) BinaryData::ShareTechMonoRegular_ttfSize);
        return typeface;
    }

    // Constructing via a 3-arg juce::FontOptions(name, height, styleFlags) - relying on Font's
    // implicit FontOptions conversion - mirrors TapeRotTheme::counterDigitFont's own proven usage
    // for the same JUCE version, rather than guessing at FontOptions' fluent with*() surface. Style
    // flags are left plain/bold for metrics purposes only - the actual embedded typeface (already a
    // specific weight) is what's applied via withTypeface() so the correct weight always renders,
    // even on a system where "bold" synthesis would otherwise be used.
    inline juce::Font labelFont(float heightPx)
    {
        return juce::Font(juce::FontOptions(heightPx).withTypeface(barlowSemiBoldTypeface()));
    }
    inline juce::Font labelFontBold(float heightPx)
    {
        return juce::Font(juce::FontOptions(heightPx).withTypeface(barlowBoldTypeface()));
    }
    // Section 2.3's 400 weight, for the UNSELECTED half of each section-0.4 label pair.
    inline juce::Font labelFontRegular(float heightPx)
    {
        return juce::Font(juce::FontOptions(heightPx).withTypeface(barlowRegularTypeface()));
    }
    inline juce::Font monoFont(float heightPx)
    {
        return juce::Font(juce::FontOptions(heightPx).withTypeface(shareTechMonoTypeface()));
    }
    inline juce::Font monoFontBold(float heightPx)
    {
        // Share Tech Mono ships one weight only - bold is synthesised (JUCE's usual fallback when
        // a typeface has no dedicated bold face), matching how the LCD/lamp text's few bold usages
        // are more about tracking/size than a genuinely heavier stroke.
        return juce::Font(juce::FontOptions(heightPx).withTypeface(shareTechMonoTypeface())).boldened();
    }

    // Solves for the font height at which `text` renders exactly targetWidthPx wide with the given
    // tracking. Used to size live-drawn text against a width measured directly off the baked
    // artwork, instead of hand-picking a px value: juce::Font's height parameter is ascent+descent,
    // NOT the CSS em size the design spec quotes, and the ratio between them is a property of the
    // specific typeface - so reading "9px" off the spec and passing 9.0f renders visibly smaller
    // than the mockup (which is exactly what happened with the switch option labels). Glyph advances
    // scale linearly with font height while the absolute tracking doesn't, so a single probe render
    // inverts the relationship exactly. probeFont must have been built at probeHeight.
    inline float fontHeightForTrackedWidth(const juce::Font& probeFont, float probeHeight,
                                            const juce::String& text, float trackingPx,
                                            float targetWidthPx)
    {
        const float glyphsAtProbe = trackedTextWidth(text, probeFont, 0.0f);
        const float trackingTotal = trackingPx * (float) juce::jmax(0, text.length() - 1);
        if (glyphsAtProbe <= 0.0f)
            return probeHeight;
        return juce::jmax(1.0f, (targetWidthPx - trackingTotal) * probeHeight / glyphsAtProbe);
    }

    inline float labelFontHeightForTrackedWidth(const juce::String& text, float trackingPx,
                                                 float targetWidthPx)
    {
        constexpr float probeHeight = 40.0f;
        return fontHeightForTrackedWidth(labelFont(probeHeight), probeHeight, text, trackingPx, targetWidthPx);
    }

    inline float monoFontHeightForTrackedWidth(const juce::String& text, float trackingPx,
                                                float targetWidthPx)
    {
        constexpr float probeHeight = 40.0f;
        return fontHeightForTrackedWidth(monoFont(probeHeight), probeHeight, text, trackingPx, targetWidthPx);
    }

    // NOTE: there is deliberately no TudorVictors typeface accessor. The wordmark that used it is
    // now a pre-baked PNG (see wordmarkImage() and WordmarkComponent), and the font is neither
    // embedded nor tracked - its licence grants no redistribution right.

    // Binary-data-backed images, decoded once per process via function-local statics (avoids
    // repeated PNG decode on every repaint/instantiation - the knob filmstrips in particular are
    // 128x16384 sheets, too expensive to decode per-component). Centralised here rather than in
    // each component so BinaryData's identifier-mangling of these particular filenames (hyphens and
    // "@" stripped entirely, "." becomes "_" - see TapeRotTheme.h's InterRegular_ttf precedent for
    // the same JUCE version) only needs verifying/fixing in one place if a name doesn't match.
    // The PRINTED PLATE (spec Rev 6, section 1.1). Everything static is baked into it: fascia,
    // grain, rails, screws, header band, dividers, wordmark, every label, every printed scale and
    // tick, the recessed wells and the footer. It carries no knobs, pointers, lamp, LCD glyphs,
    // meter fill, scope contents or buttons - those composite on top in the order given in
    // section 0.5.
    //
    // This replaced the bare chassis in Rev 6, which inverted Rev 5's architecture. Rev 5 drew
    // every label in code; that code is deleted, not adapted, because running both paths
    // double-draws every label at a one-pixel offset. If you are about to draw a string on the
    // fascia, check section 0.2 first - the answer is almost certainly that it is already baked.
    inline const juce::Image& panelBackgroundImage()
    {
        static const juce::Image image = juce::ImageFileFormat::loadFrom(
            BinaryData::gatecrasherpanelplate2x_png, (size_t) BinaryData::gatecrasherpanelplate2x_pngSize);
        return image;
    }

    // Re-blits the panel background bitmap at a given PANEL-LOCAL rect. Live components whose
    // graphics change clear their previous frame this way, and because the fascia is a gradient
    // with a 1px vertical brush grain (amplitude ~14 levels, clearly visible), re-blitting the real
    // pixels stays seamless where a flat fill would show as a smooth patch.
    //
    // **Rev 7 caveat.** The background is a printed plate, so this only genuinely clears regions the
    // plate leaves EMPTY. Erasing a region whose baked content is what you were about to redraw is
    // a no-op - draw a dimmed label over a baked bold one and you get both.
    //
    // That is why the eight state-dependent labels of section 0.4 are absent from the plate: bare
    // fascia sits where they go, the build draws all eight every frame, and this stays a real
    // clear. Rev 6 baked them at their default weights and asked the build to redraw the pair on
    // change, which cannot work in that direction; Rev 7 withdrew it. If a future revision bakes
    // any state-dependent artwork again, this helper is not the answer.
    //
    // localRect/destOrigin are in the CALLING COMPONENT's own local coordinate space; destOrigin is
    // that component's position relative to the panel-local root (its getPosition(), when the
    // component is a direct child of the panel-local root - which every component that calls this
    // is, see GatecrasherEditorContent.cpp) so the correct source region can be computed.
    inline void eraseToBackground(juce::Graphics& g, juce::Rectangle<float> localRect,
                                   juce::Point<int> destOrigin = {})
    {
        const auto panelLocalRect = localRect + destOrigin.toFloat();
        const auto& bg = panelBackgroundImage();
        const float scaleX = (float) bg.getWidth() / Layout::canvasWidth;
        const float scaleY = (float) bg.getHeight() / Layout::canvasHeight;

        const juce::Rectangle<int> srcRect((int) std::round(panelLocalRect.getX() * scaleX),
                                            (int) std::round(panelLocalRect.getY() * scaleY),
                                            (int) std::round(panelLocalRect.getWidth() * scaleX),
                                            (int) std::round(panelLocalRect.getHeight() * scaleY));
        const juce::Rectangle<int> destRect = localRect.getSmallestIntegerContainer();

        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImage(bg, destRect.getX(), destRect.getY(), destRect.getWidth(), destRect.getHeight(),
                    srcRect.getX(), srcRect.getY(), srcRect.getWidth(), srcRect.getHeight());
    }

    inline const juce::Image& knobLargeFilmstrip()
    {
        static const juce::Image image = juce::ImageFileFormat::loadFrom(
            BinaryData::knob_large_160px_128f_png, (size_t) BinaryData::knob_large_160px_128f_pngSize);
        return image;
    }

    inline const juce::Image& knobSmallFilmstrip()
    {
        static const juce::Image image = juce::ImageFileFormat::loadFrom(
            BinaryData::knob_small_160px_128f_png, (size_t) BinaryData::knob_small_160px_128f_pngSize);
        return image;
    }

    // The filmstrip frames aren't fully opaque across the knob body itself, not just their square's
    // transparent corner margin (confirmed by temporarily drawing a bright fill behind one and
    // seeing the whole knob tinted by it, not just its corners) - they were authored to be
    // composited over the panel background's own baked-dark knob artwork, which supplied the
    // "backdrop" the semi-transparent body blends against. KnobFilmstripComponent can't use that
    // baked artwork directly any more (its own baked pointer would bleed through at the wrong angle
    // - see that file's paint() comment), so it fills a plain backdrop first instead; that backdrop
    // has to be dark (matching the knob's own base tone) or the blend reads as washed-out/light
    // rather than a knob. Sampled directly from each filmstrip's own pixels (bottom-centre of frame
    // 0, well clear of that frame's pointer, which sweeps the upper ~270deg arc and never reaches
    // 6 o'clock) rather than a hand-picked hex, so it's an exact match by construction.
    // The design spec and the reference mockup both quote type sizes as CSS px, but juce::Font's
    // height parameter is ascent+descent, which for a given typeface is a fixed multiple of the CSS
    // em size rather than equal to it - passing a spec px value straight to labelFont() renders
    // noticeably small (see fontHeightForTrackedWidth). These convert once, calibrating the ratio
    // off a reference string whose rendered width was measured directly from the dressed reference
    // artwork, so every spec size across the panel scales correctly from one real measurement.
    inline float labelFontHeightForCssPx(float cssPx)
    {
        static const float ratio =
            labelFontHeightForTrackedWidth(Layout::switchOptionLabelRefText,
                                            Layout::switchOptionLabelTracking,
                                            Layout::switchOptionLabelRefWidth)
            / Layout::switchOptionLabelRefCssPx;
        return cssPx * ratio;
    }

    inline float monoFontHeightForCssPx(float cssPx)
    {
        static const float ratio =
            monoFontHeightForTrackedWidth(Layout::meterReadoutRefText, 0.0f, Layout::meterReadoutRefWidth)
            / Layout::meterReadoutRefCssPx;
        return cssPx * ratio;
    }

    // CSS letter-spacing is expressed in em, i.e. relative to the font's own size, so the absolute
    // pixel tracking drawTrackedText wants scales with the size the label is drawn at.
    inline float trackingPxForEm(float em, float cssPx) { return em * cssPx; }

    /** **How this panel spells the LCD parameter readout.**

        A presentation decision, so it lives with the other presentation constants rather than in
        ProgramHeader - and that placement is load-bearing for the test: ProgramHeader.h reaches
        PluginProcessor.h, which needs JucePlugin_* macros that only exist in the plugin target, so
        a test that read the format from there could not link. The test must read the SHIPPING
        format rather than a copy, or it asserts against itself.

        **This panel carried `ValueCase::wordsOnly` until 2026-08-13, and the enum is gone.** The
        ruling is that case belongs at the SOURCE, never at a display site: section 6.3's
        "ALGORITHM: PLATE" is still the target, but PLATE is authored that way in Parameters.h so
        the LCD and the host's automation lane print one string. Re-casing here made this the only
        site that did.

        **The re-authoring that ruling requires has NOT been done in this casting yet** - see the
        suite-level note in ../CLAUDE.md under "Case belongs at the source". Until the `name`
        arguments and the choice strings in Parameters.h are in caps, this panel's readout prints
        them as authored, which is a visible change from what shipped.

        The revert is core's 900 ms, where this panel carried 800. */
    inline nf::ReadoutFormat readoutFormat()
    {
        nf::ReadoutFormat f;
        f.nameCharacterBudget = 24;
        return f;
    }
}
