#pragma once

#include <juce_graphics/juce_graphics.h>
#include <BinaryData.h>
#include <array>
#include <cmath>

// Centralises every pixel constant from design/GATECRASHER-GUI-SPEC.md (palette, coordinates,
// filmstrip contract) in one place, mirroring TapeRotTheme.h's role for TapeRot - components pull
// from GatecrasherTheme::Colour/Layout rather than hardcoding numbers. Unlike TapeRotTheme, most of
// the fascia here is a static bitmap (see GatecrasherPanelBackground) rather than code-drawn, so
// this file's job is narrower: positions/sizes for the *live* pieces layered on top, plus the
// handful of colours those live pieces need to match the baked artwork around them.
namespace GatecrasherTheme
{
    namespace Colour
    {
        // Section 1 palette - only the values actually needed by live-drawn components (most of
        // the fascia gradient/grain/rack-ear/divider colours never leave the static background
        // bitmap, so aren't reproduced here).
        inline const juce::Colour controlLabelText{0xFF2B3034};
        inline const juce::Colour inactiveLabel{0xFF7B8287};
        inline const juce::Colour tertiaryGroupLabel{0xFF464C51};

        // Static chrome, all straight from section 1 / the reference mockup's own CSS.
        inline const juce::Colour engravedHeading{0xFF33383D};   // section headings, subtitle line 1
        inline const juce::Colour subtitleSecondary{0xFF4A5055}; // "MODEL GR-85 - STEREO"
        inline const juce::Colour headerCaption{0xFF3A4045};     // PROGRAM / IN / OUT captions
        inline const juce::Colour valueText{0xFF3D4348};         // numeric readouts under knobs
        inline const juce::Colour ahrLabelText{0xFF1E2226};      // ATTACK/HOLD/RELEASE sit darker
        inline const juce::Colour algorithmActive{0xFF15181B};
        inline const juce::Colour algorithmInactive{0xFF474D52};
        inline const juce::Colour versionText{0xFF4C5257};
        inline const juce::Colour meterFrameBorder{0xFF33393E};
        inline const juce::Colour programCellDivider{0xFF2A3035};
        inline const juce::Colour wordmarkInk{0xFF14171A}; // section 8

        inline const juce::Colour ledWindowBg{0xFF07090A};
        inline const juce::Colour ledWindowBorder{0xFF363C41};
        inline const juce::Colour ledText{0xFFDFE6EA};

        // "Gate accent (ONLY colour on panel)" - reserved exclusively for the GATE OPEN lamp and
        // the envelope trace, per spec section 1's explicit rule and BRAND.md's one-accent-colour
        // rule. Never used for any knob/label/meter.
        inline const juce::Colour gateAccent{0xFFFF2B1C};
        inline const juce::Colour lampUnlit{0xFF3A1512};

        inline const juce::Colour tickMark{0xFF3F454A};

        // Switch (KEY SOURCE / SHAPE) - recessed track + sliding metal shoe, section 5/7.
        inline const juce::Colour switchTrackBg{0xFF07090A};
        inline const juce::Colour switchTrackBorder{0xFF353B40};
        inline const juce::Colour switchShoeTop{0xFF8D959B};
        inline const juce::Colour switchShoeBottom{0xFF4E545A};

        // Gate envelope scope, section 5.
        inline const juce::Colour scopeBorder{0xFF0A0C0D};
        inline const juce::Colour scopeBgTop{0xFF06080A};
        inline const juce::Colour scopeBgBottom{0xFF0B0F11};
        inline const juce::Colour scopeGrid{0x1A96B4BE};       // rgba(150,180,190,.10)
        inline const juce::Colour scopeBaseline{0x3896B4BE};   // rgba(150,180,190,.22)
        inline const juce::Colour scopeInputWaveform{0x4DB2BEC5}; // rgba(178,190,197,.30)
        inline const juce::Colour scopeAnnotation{0x8CA0B2BA};    // rgba(160,178,186,.55)
        inline const juce::Colour scopeFillTop{0x4DFF2B1C};    // rgba(255,43,28,.30)
        inline const juce::Colour scopeFillBottom{0x05FF2B1C}; // rgba(255,43,28,.02)

        // GATE OPEN lamp, section 5.
        inline const juce::Colour lampOpenCore{0xFFFF2B1C};
        inline const juce::Colour lampOpenMid{0xFFB0140C};
        inline const juce::Colour lampOpenEdge{0xFF6D0B06};
        inline const juce::Colour gateOpenLabelActive{0xFF141719};
        inline const juce::Colour gateOpenLabelInactive{0xFF43494E};

        // Input meter, section 7.
        inline const juce::Colour meterUnlitSegment{0xFF20262A};
        inline const juce::Colour meterLitSegment{0xFFF4F8FA};
        inline const juce::Colour meterBloom{0x8CE6F2F8}; // rgba(230,242,248,.55)
        inline const juce::Colour meterThresholdMarker{0x80FFFFFF};

        // Program header LED tag/name text, section 6.
        inline const juce::Colour tagFactory{0xFF6F797F};
        inline const juce::Colour tagUser{0xFFCFD7DC};
        inline const juce::Colour headerName{0xFFDFE6EA};

        // SAVE/DELETE (STORE/CANCEL) stamped-steel utility buttons, section 6's state table. Drawn
        // live rather than via the header-state bitmaps - see ProgramHeader.cpp's class comment for
        // why the bitmap-crop approach was abandoned in favour of this.
        inline const juce::Colour buttonEnabledTop{0xFFDBE0E3};
        inline const juce::Colour buttonEnabledBottom{0xFFAAB1B6};
        inline const juce::Colour buttonEnabledBorder{0xFF6D7478};
        inline const juce::Colour buttonEnabledLabel{0xFF22272B};
        inline const juce::Colour buttonPressedTop{0xFFA9B0B5};
        inline const juce::Colour buttonPressedBottom{0xFFC9D0D4};
        inline const juce::Colour buttonDisabledTop{0xFFC2C8CC};
        inline const juce::Colour buttonDisabledBottom{0xFFA8AFB3};
        inline const juce::Colour buttonDisabledBorder{0xFF8D9498};
        inline const juce::Colour buttonDisabledLabel{0x8C8B9297}; // #8B9297 @ .55 opacity, per spec
    }

    // The 128-frame knob filmstrips ship in two skirt styles (see design/CLAUDE.md's asset list) -
    // which one a given knob uses is part of its identity in the section 3 coordinate table.
    enum class KnobFilmstripSize { large, small };

    namespace Layout
    {
        constexpr float canvasWidth = 960.0f;
        constexpr float canvasHeight = 434.0f;

        // Rotation range for every knob: pointer at 12 o'clock = centre (section 3).
        constexpr float knobArcStartDegrees = -135.0f;
        constexpr float knobArcEndDegrees = 135.0f;

        // Regular knobs' decorative tick ring spacing target (section 3: "every 15 for large
        // knobs / 20-22 for small ones"). The actual per-knob spacing is derived from this so the
        // ring always lands exactly on both arc endpoints (see tickCountForSpacing below).
        constexpr float largeKnobTickSpacingDegrees = 15.0f;
        constexpr float smallKnobTickSpacingDegrees = 21.0f;

        // Tick ring sits just outside the knob's own radius (section 3: "from r+2 to r+7").
        constexpr float tickInnerOffset = 2.0f;
        constexpr float tickOuterOffset = 7.0f;

        // Filmstrip frames are square with transparent margin for the baked cast shadow - draw
        // into the full bounding box (diameter + ~7% bleed), not just the knob circle.
        constexpr float knobBoundingBoxBleed = 1.07f;

        // How a knob's numeric readout is formatted. The reference render uses a different
        // convention per control rather than one global rule - "180 Hz" but "6.3 k" once past a
        // kilohertz, "0.4 ms" but "165 ms" once the value is big enough that a decimal is noise,
        // and the OUTPUT column drops units entirely ("+7", "-1.4") where the column is too narrow
        // for them. Deliberately not the parameters' own getText(): that renders a fixed 2 decimal
        // places with no unit, which matches neither the artwork nor each other.
        enum class ValueFormat
        {
            none,       // no readout under this knob
            decibels1,  // "-18.5 dB"
            hertzAuto,  // "180 Hz" / "6.3 k"
            millisAuto, // "0.4 ms" / "165 ms"
            percent0,   // "64%"
            plain2,     // "0.72"
            signedInt,  // "+7"
            signed1     // "-1.4"
        };

        struct KnobSpec
        {
            const char* paramID;
            float cx, cy, diameter;
            KnobFilmstripSize size;
            bool isAlgorithmSelector = false; // 4 fixed decorative ticks instead of the swept ring

            // Engraved label under the knob, and the live numeric readout under that. Both are
            // centred on cx. The Y positions are absolute rather than derived from cy + diameter
            // because the reference artwork's own vertical rhythm isn't a single constant offset -
            // the mockup's flex gaps differ per group (5px on the small/output knobs, 6px on
            // THRESHOLD and A/H/R) and the label sizes differ too, so these were measured off the
            // dressed render per group. labelCssPx/labelTrackingEm mirror the mockup's CSS for that
            // specific label; valueCentreY == 0 means the knob has no readout (HF/LF), and a null
            // label means no engraved label at all (the algorithm selector, which instead gets the
            // four corner labels PanelReadouts draws).
            const char* label = nullptr;
            float labelCssPx = 9.5f;
            float labelTrackingEm = 0.14f;
            float labelCentreY = 0.0f;
            float valueCentreY = 0.0f;
            ValueFormat valueFormat = ValueFormat::none;
        };

        // Section 3's full 15-knob table. density/decay are deliberately absent - automation-only,
        // no panel control (section 9 / GatecrasherEditorContent's own comment).
        inline constexpr std::array<KnobSpec, 15> knobs{ {
            {"threshold", 103.0f, 162.0f, 62.0f, KnobFilmstripSize::large, false, "THRESHOLD", 9.5f, 0.18f, 204.7f, 220.7f, ValueFormat::decibels1},
            {"trigHP",     89.0f, 284.0f, 38.0f, KnobFilmstripSize::small, false, "HP",        9.5f, 0.16f, 314.7f, 330.7f, ValueFormat::hertzAuto},
            {"trigLP",    145.0f, 284.0f, 38.0f, KnobFilmstripSize::small, false, "LP",        9.5f, 0.16f, 314.7f, 330.7f, ValueFormat::hertzAuto},
            {"attack",    300.0f, 284.0f, 56.0f, KnobFilmstripSize::large, false, "ATTACK",   10.0f, 0.20f, 324.4f, 340.8f, ValueFormat::millisAuto},
            {"hold",      390.0f, 284.0f, 56.0f, KnobFilmstripSize::large, false, "HOLD",     10.0f, 0.20f, 324.4f, 340.8f, ValueFormat::millisAuto},
            {"release",   480.0f, 284.0f, 56.0f, KnobFilmstripSize::large, false, "RELEASE",  10.0f, 0.20f, 324.4f, 340.8f, ValueFormat::millisAuto},
            {"algorithm", 686.0f, 165.0f, 50.0f, KnobFilmstripSize::large, true},
            {"size",      652.0f, 241.0f, 44.0f, KnobFilmstripSize::large, false, "SIZE",      9.5f, 0.16f, 274.7f, 290.7f, ValueFormat::plain2},
            {"preDelay",  722.0f, 241.0f, 44.0f, KnobFilmstripSize::large, false, "PRE-DLY",   9.5f, 0.16f, 274.7f, 290.7f, ValueFormat::millisAuto},
            {"dampHF",    657.0f, 357.0f, 34.0f, KnobFilmstripSize::small, false, "HF",        9.5f, 0.14f, 385.7f, 0.0f,   ValueFormat::none},
            {"dampLF",    717.0f, 357.0f, 34.0f, KnobFilmstripSize::small, false, "LF",        9.5f, 0.14f, 385.7f, 0.0f,   ValueFormat::none},
            {"slam",      841.0f, 151.0f, 40.0f, KnobFilmstripSize::large, false, "SLAM",      9.5f, 0.14f, 182.7f, 198.7f, ValueFormat::signedInt},
            {"width",     893.0f, 151.0f, 40.0f, KnobFilmstripSize::large, false, "WIDTH",     9.5f, 0.14f, 182.7f, 198.7f, ValueFormat::percent0},
            {"mix",       841.0f, 237.0f, 40.0f, KnobFilmstripSize::large, false, "MIX",       9.5f, 0.14f, 268.7f, 284.7f, ValueFormat::percent0},
            {"trim",      893.0f, 237.0f, 40.0f, KnobFilmstripSize::large, false, "TRIM",      9.5f, 0.14f, 268.7f, 284.7f, ValueFormat::signed1},
        } };

        // Gate envelope scope, section 5.
        constexpr float scopeX = 218.0f, scopeY = 113.0f, scopeW = 344.0f, scopeH = 122.0f;
        constexpr float scopeInnerInset = 2.0f; // 1px border + 1px inner padding
        constexpr float scopeBaselineInset = 14.0f; // baseline at h - 14
        constexpr float scopeCeilingInset = 14.0f;  // ceiling at y + 14
        constexpr float scopePixelsPerFrame = 2.0f;
        constexpr float scopeGridSpacing = 44.0f;
        constexpr int scopeNumStaticHorizontals = 5;

        constexpr float lampCx = 224.0f, lampCy = 95.0f, lampDiameter = 15.0f;

        // Input meter, section 7.
        constexpr float meterX = 147.0f, meterY = 133.0f, meterW = 14.0f, meterH = 76.0f;
        constexpr float meterSegmentH = 4.0f, meterSegmentPitch = 6.0f;
        // Meter/marker dB range: matches the Threshold parameter's own -60..0dB range (section 9)
        // so the threshold marker is meaningful relative to the lit segments.
        constexpr float meterFloorDb = -60.0f, meterCeilingDb = 0.0f;

        // KEY SOURCE / SHAPE switches - identical track geometry (section 5: "reuses the KEY
        // SOURCE switch verbatim"). Caption/label rows are derived around each track with the same
        // internal proportions (spec gives the track/assembly anchors, not sub-pixel caption/label
        // baselines - these are a careful, symmetric interpolation between the two, not lifted
        // directly from a pixel-measured mockup).
        constexpr float switchTrackW = 56.0f, switchTrackH = 20.0f;
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

        // KEY SOURCE's track position (section 7: "track at x 88, y 371") was given directly and
        // verified pixel-for-pixel against the raw background asset - correct as-is. SHAPE's spec
        // text ("centred beneath the A/H/R knobs at x 361, y 364, 58 x 56") describes the *assembly's*
        // top-left corner (58x56 is the whole caption+track+labels stack, not the 56x20 track), which
        // an earlier pass misread as the assembly's *centre* - shapeTrackX/Y below were back-computed
        // from that wrong reading (giving 333/352) and never verified against the actual asset. Doing
        // that verification (cropping+upscaling narrow strips of gatecrasher-panel@2x.png through the
        // track) found the real baked track top at canvas y=380, not y=352 - a ~28px miss that left
        // this component's whole bounds sitting above where the baked assembly actually is, so it was
        // erasing/redrawing over plain fascia while the real baked switch stayed fully exposed
        // untouched below it (the "duplicated control" bug: two full track+thumb assemblies visible
        // at once). Corrected to match the verified asset position.
        constexpr float shapeTrackX = 362.0f, shapeTrackY = 380.0f;
        constexpr float keySourceTrackX = 88.0f, keySourceTrackY = 371.0f;

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

        // Program header (section 6). The three header-state bitmaps are full-width renders of the
        // whole header band (wordmark included, confirmed by inspecting the assets) - ProgramHeader
        // only ever blits the "program cluster" sub-rect below (PROGRAM caption through the IN/OUT
        // windows), leaving the wordmark to WordmarkComponent so the two never double-paint the
        // same pixels. This crop rect is a generous, carefully-derived bounding box around section
        // 6's coordinate table (which itself only covers x>=480), not a pixel-measured exact crop -
        // safe because the surrounding fascia is pixel-identical to the static panel background in
        // every direction, so a slightly loose crop still blends seamlessly.
        constexpr float headerAssetSrcScale = 3.0f; // the bitmaps are shipped @3x
        // Verified directly against the source bitmap (design/assets/header-factory-program@3x.png):
        // the original headerCropX=460 started mid-glyph through "PROGRAM"'s P, chopping it to
        // "ROGRAM" and - since destRect/srcRect share this same wrong offset - throwing every
        // erase-rect below (tag/name cell, SAVE, DELETE, all independently correct per the spec's
        // own coordinates) out of alignment with where their content actually lands once drawn,
        // which is what produced the "FACT FACT" double-tag and the SAVE button being partly
        // covered by the name cell. 433 was found by cropping the bitmap directly until "PROGRAM"
        // reads cleanly with a few px of left padding; width increased to keep the right edge
        // (which was never reported as clipped) at the same x=906 it was already correctly at.
        constexpr float headerCropX = 433.0f, headerCropY = 14.0f, headerCropW = 473.0f, headerCropH = 54.0f;

        constexpr float programWindowX = 480.0f, programWindowY = 33.0f, programWindowW = 238.0f, programWindowH = 25.0f;
        constexpr float programTagCellX = 481.0f, programTagCellY = 34.0f, programTagCellW = 39.0f, programTagCellH = 23.0f;
        constexpr float programNameCellX = 521.0f, programNameCellY = 34.0f, programNameCellW = 197.0f, programNameCellH = 23.0f;

        constexpr float saveButtonX = 724.0f, saveButtonY = 33.0f, saveButtonW = 44.0f, saveButtonH = 25.0f;
        constexpr float deleteButtonX = 773.0f, deleteButtonY = 33.0f, deleteButtonW = 44.0f, deleteButtonH = 25.0f;

        // Measured off gatecrasher-panel@2x.png (cropping narrow strips through the windows and
        // scaling back to canvas units), not taken from section 6's table: that row reads
        // "822 -> 908 | 33 | ~44 each | 25", but the artwork actually puts OUT's right edge at ~920,
        // a dozen px past where 908 would place it. Same situation as headerCropX above - where the
        // spec's coordinates and the shipped asset disagree, the asset wins, because these rects
        // exist to erase and redraw over exactly what's baked there.
        constexpr float inWindowX = 825.0f, inWindowY = 32.5f, inWindowW = 43.5f, inWindowH = 25.0f;
        constexpr float outWindowX = 876.5f, outWindowY = 32.5f, outWindowW = 43.5f, outWindowH = 25.0f;

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
        constexpr float meterFrameX = 146.0f, meterFrameY = 132.0f, meterFrameW = 16.0f, meterFrameH = 78.0f;
        constexpr float meterCaptionCentreY = 220.0f;

        // "ENVELOPE   50 ms / DIV", right-aligned to the scope's right edge.
        constexpr float envelopeAnnotationRight = 562.0f;
        constexpr float envelopeAnnotationCentreY = 95.0f;
        constexpr float envelopeAnnotationGap = 15.0f;

        // Algorithm selector's four corner labels, around the 120x78 block centred on the reverb
        // column. Which one is lit tracks the live Algorithm parameter, so PanelReadouts draws
        // these rather than PanelChrome.
        constexpr float algoLabelLeftX = 627.0f, algoLabelRightX = 746.0f;
        constexpr float algoLabelTopCentreY = 133.0f, algoLabelBottomCentreY = 198.5f;

        constexpr int maxProgramNameLength = 22; // mirrors ProgramManager::maxProgramNameLength

        // Wordmark, section 8 - owned separately from ProgramHeader (see headerCrop comment above).
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
        constexpr float gateOpenLabelX = 240.0f, gateOpenLabelY = 87.0f, gateOpenLabelW = 190.0f, gateOpenLabelH = 16.0f;
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

    // Number of ticks (inclusive of both arc endpoints) whose even spacing across the full 270
    // sweep comes closest to targetSpacingDegrees, landing exactly on -135 and +135.
    inline int tickCountForSpacing(float targetSpacingDegrees) noexcept
    {
        const float sweep = Layout::knobArcEndDegrees - Layout::knobArcStartDegrees;
        const int intervals = juce::jmax(1, (int) std::round(sweep / targetSpacingDegrees));
        return intervals + 1;
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
    // Mono Regular (numeric/LED readouts), per GATECRASHER-GUI-SPEC.md section 2. Loaded once per
    // process via function-local statics, same caching pattern as tudorVictorsTypeface() below.
    inline juce::Typeface::Ptr barlowSemiBoldTypeface()
    {
        static const juce::Typeface::Ptr typeface =
            juce::Typeface::createSystemTypefaceFor(BinaryData::BarlowCondensedSemiBold_ttf,
                                                      (size_t) BinaryData::BarlowCondensedSemiBold_ttfSize);
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
    // The bare chassis: fascia gradient + brush grain, side rails with screws, the header band and
    // its bottom border, and the three section dividers. Deliberately has NO controls, labels,
    // nameplate or window frames on it - every one of those is drawn live on top (PanelChrome for
    // the static engraved layer, the individual components for anything that moves). The fully
    // dressed renders remain in design/assets/ as visual acceptance references, but are no longer
    // shipped in BinaryData.
    inline const juce::Image& panelBackgroundImage()
    {
        static const juce::Image image = juce::ImageFileFormat::loadFrom(
            BinaryData::gatecrasherpanelbare2x_png, (size_t) BinaryData::gatecrasherpanelbare2x_pngSize);
        return image;
    }

    // Re-blits the panel background bitmap at a given PANEL-LOCAL rect, restoring that patch to the
    // bare chassis artwork underneath. Live components whose text/graphics change need to clear
    // their previous frame before drawing the new one, and the fascia there is a gradient with a
    // 1px vertical brush grain - so re-blitting the real background pixels stays seamless where a
    // flat-colour fill would show as a smooth patch against the grain around it.
    //
    // (Before the bare chassis existed this also had to paper over baked-in labels, which is a very
    // different job: erasing a region whose baked content was exactly what you were about to redraw
    // is a no-op, so several components had to use opaque flat fills instead. Those workarounds are
    // gone - nothing is baked into the background any more, so an erase here always genuinely
    // clears.)
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

    // The baked spray-stencil wordmark, transparent, at 3x - see WordmarkComponent's class comment.
    inline const juce::Image& wordmarkImage()
    {
        static const juce::Image image = juce::ImageFileFormat::loadFrom(
            BinaryData::gatecrasherwordmark3x_png, (size_t) BinaryData::gatecrasherwordmark3x_pngSize);
        return image;
    }

    inline const juce::Image& knobLargeFilmstrip()
    {
        static const juce::Image image = juce::ImageFileFormat::loadFrom(
            BinaryData::knob_large_128px_128f_png, (size_t) BinaryData::knob_large_128px_128f_pngSize);
        return image;
    }

    inline const juce::Image& knobSmallFilmstrip()
    {
        static const juce::Image image = juce::ImageFileFormat::loadFrom(
            BinaryData::knob_small_128px_128f_png, (size_t) BinaryData::knob_small_128px_128f_pngSize);
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
}
