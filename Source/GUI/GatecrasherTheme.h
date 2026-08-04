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

        struct KnobSpec
        {
            const char* paramID;
            float cx, cy, diameter;
            KnobFilmstripSize size;
            bool isAlgorithmSelector = false; // 4 fixed decorative ticks instead of the swept ring
        };

        // Section 3's full 15-knob table. density/decay are deliberately absent - automation-only,
        // no panel control (section 9 / GatecrasherEditorContent's own comment).
        inline constexpr std::array<KnobSpec, 15> knobs{ {
            {"threshold", 103.0f, 162.0f, 62.0f, KnobFilmstripSize::large},
            {"trigHP",     89.0f, 284.0f, 38.0f, KnobFilmstripSize::small},
            {"trigLP",    145.0f, 284.0f, 38.0f, KnobFilmstripSize::small},
            {"attack",    300.0f, 284.0f, 56.0f, KnobFilmstripSize::large},
            {"hold",      390.0f, 284.0f, 56.0f, KnobFilmstripSize::large},
            {"release",   480.0f, 284.0f, 56.0f, KnobFilmstripSize::large},
            {"algorithm", 686.0f, 165.0f, 50.0f, KnobFilmstripSize::large, true},
            {"size",      652.0f, 241.0f, 44.0f, KnobFilmstripSize::large},
            {"preDelay",  722.0f, 241.0f, 44.0f, KnobFilmstripSize::large},
            {"dampHF",    657.0f, 357.0f, 34.0f, KnobFilmstripSize::small},
            {"dampLF",    717.0f, 357.0f, 34.0f, KnobFilmstripSize::small},
            {"slam",      841.0f, 151.0f, 40.0f, KnobFilmstripSize::large},
            {"width",     893.0f, 151.0f, 40.0f, KnobFilmstripSize::large},
            {"mix",       841.0f, 237.0f, 40.0f, KnobFilmstripSize::large},
            {"trim",      893.0f, 237.0f, 40.0f, KnobFilmstripSize::large},
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
        // a 20px label row around the 20px track reproduces section 5's explicit SHAPE assembly
        // (centre 361,364, size 58x56) exactly when applied around its track position - the same
        // split is then applied around KEY SOURCE's own explicit track position (section 7) for a
        // symmetric, matching assembly, since section 7 doesn't restate separate caption/label
        // sub-coordinates for it beyond "identical to the SHAPE switch".
        constexpr float switchAssemblyPad = 1.0f;
        constexpr float switchCaptionRowH = 16.0f;
        constexpr float switchLabelRowH = 20.0f;
        constexpr float switchAssemblyW = switchTrackW + 2.0f * switchAssemblyPad;
        constexpr float switchAssemblyH = switchCaptionRowH + switchTrackH + switchLabelRowH;

        constexpr float shapeTrackX = 333.0f, shapeTrackY = 352.0f;
        constexpr float keySourceTrackX = 88.0f, keySourceTrackY = 371.0f;

        // Program header (section 6). The three header-state bitmaps are full-width renders of the
        // whole header band (wordmark included, confirmed by inspecting the assets) - ProgramHeader
        // only ever blits the "program cluster" sub-rect below (PROGRAM caption through the IN/OUT
        // windows), leaving the wordmark to WordmarkComponent so the two never double-paint the
        // same pixels. This crop rect is a generous, carefully-derived bounding box around section
        // 6's coordinate table (which itself only covers x>=480), not a pixel-measured exact crop -
        // safe because the surrounding fascia is pixel-identical to the static panel background in
        // every direction, so a slightly loose crop still blends seamlessly.
        constexpr float headerAssetSrcScale = 3.0f; // the bitmaps are shipped @3x
        constexpr float headerCropX = 460.0f, headerCropY = 14.0f, headerCropW = 446.0f, headerCropH = 54.0f;

        constexpr float programWindowX = 480.0f, programWindowY = 33.0f, programWindowW = 238.0f, programWindowH = 25.0f;
        constexpr float programTagCellX = 481.0f, programTagCellY = 34.0f, programTagCellW = 39.0f, programTagCellH = 23.0f;
        constexpr float programNameCellX = 521.0f, programNameCellY = 34.0f, programNameCellW = 197.0f, programNameCellH = 23.0f;

        constexpr float saveButtonX = 724.0f, saveButtonY = 33.0f, saveButtonW = 44.0f, saveButtonH = 25.0f;
        constexpr float deleteButtonX = 773.0f, deleteButtonY = 33.0f, deleteButtonW = 44.0f, deleteButtonH = 25.0f;

        constexpr int maxProgramNameLength = 22; // mirrors ProgramManager::maxProgramNameLength

        // Wordmark, section 8 - owned separately from ProgramHeader (see headerCrop comment above).
        constexpr float wordmarkX = 38.0f, wordmarkY = 20.0f, wordmarkW = 232.0f, wordmarkH = 40.0f;

        constexpr float gateOpenLabelX = 240.0f, gateOpenLabelY = 87.0f, gateOpenLabelW = 130.0f, gateOpenLabelH = 16.0f;
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

    // TudorVictors (design/assets/TudorVictors.ttf), embedded for the wordmark placeholder only -
    // see WordmarkComponent's TODO(design) comment. Loaded once per process via a function-local
    // static, same caching pattern as TapeRotTheme's sansBoldTypeface()/dymoFont().
    inline juce::Typeface::Ptr tudorVictorsTypeface()
    {
        static const juce::Typeface::Ptr typeface =
            juce::Typeface::createSystemTypefaceFor(BinaryData::TudorVictors_ttf,
                                                      (size_t) BinaryData::TudorVictors_ttfSize);
        return typeface;
    }

    // Binary-data-backed images, decoded once per process via function-local statics (avoids
    // repeated PNG decode on every repaint/instantiation - the knob filmstrips in particular are
    // 128x16384 sheets, too expensive to decode per-component). Centralised here rather than in
    // each component so BinaryData's identifier-mangling of these particular filenames (hyphens and
    // "@" stripped entirely, "." becomes "_" - see TapeRotTheme.h's InterRegular_ttf precedent for
    // the same JUCE version) only needs verifying/fixing in one place if a name doesn't match.
    inline const juce::Image& panelBackgroundImage()
    {
        static const juce::Image image = juce::ImageFileFormat::loadFrom(
            BinaryData::gatecrasherpanel2x_png, (size_t) BinaryData::gatecrasherpanel2x_pngSize);
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

    inline const juce::Image& headerFactoryImage()
    {
        static const juce::Image image = juce::ImageFileFormat::loadFrom(
            BinaryData::headerfactoryprogram3x_png, (size_t) BinaryData::headerfactoryprogram3x_pngSize);
        return image;
    }

    inline const juce::Image& headerUserImage()
    {
        static const juce::Image image = juce::ImageFileFormat::loadFrom(
            BinaryData::headeruserprogram3x_png, (size_t) BinaryData::headeruserprogram3x_pngSize);
        return image;
    }

    inline const juce::Image& headerNameEntryImage()
    {
        static const juce::Image image = juce::ImageFileFormat::loadFrom(
            BinaryData::headernameentry3x_png, (size_t) BinaryData::headernameentry3x_pngSize);
        return image;
    }
}
