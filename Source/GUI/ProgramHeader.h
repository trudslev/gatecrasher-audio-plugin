#pragma once

#include "../PluginProcessor.h"
#include "GatecrasherMenuLookAndFeel.h"
#include "GatecrasherTheme.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

// The program section (GUI-SPEC.md section 6). The static panel background already
// bakes in the LED window frame and the PROGRAM caption at their correct panel-local position, so
// this component only needs to redraw the parts that are genuinely dynamic: the FACT/USER tag, the
// program name (or the in-progress typed name + blinking caret), the SAVE/DELETE (STORE/CANCEL)
// buttons' gradient/label per their enabled/disabled/pressed state, and the IN/OUT numeric LED
// readouts (the panel background only ever bakes in one frozen example reading from whenever the
// reference mockup was captured - these read the processor's live input/output meter levels).
//
// An earlier version instead tried to composite design/assets/header-{factory,user,name-entry}@3x.png
// as a second bitmap layer on top of the main panel background for this whole region. That doesn't
// work: those bitmaps' own internal coordinate system isn't calibrated to the same panel-local
// frame the rest of this codebase's Layout constants assume (verified by cropping the raw asset -
// its content sits about 32px further left than a naive originX=0/scale=3x reading predicts), so
// every rect computed from Layout's *correct*, spec-matching absolute coordinates landed slightly
// off the bitmap's own content, producing doubled/ghosted text ("FACT FACT") rather than a clean
// composite. Redrawing every dynamic element live (erasing its patch of the background first via
// GatecrasherTheme::eraseToBackground, then drawing fresh) sidesteps needing that second bitmap's
// coordinate system to agree with anything, and is the same pattern GateLamp and
// ToggleSwitchComponent use for their own live-recoloured labels.
//
// "Current program" isn't an APVTS parameter, so this polls
// getCurrentProgram()/getProgramName()/isFactoryProgram() on a timer to stay in sync with
// host-driven program changes, same pattern as TapeRot's PresetStrip.
//
// DELIBERATE ADDITION beyond GUI-SPEC.md section 6: clicking the program name cell opens
// a menu of all programs (factory and user, current one ticked). The spec's section 6 defines the
// name cell as a pure readout and its Behaviour subsection covers only idle / SAVE / name-entry /
// DELETE - it specifies no program-browsing control anywhere on the panel, and the reference mockup
// has click handlers for SAVE and DELETE only. That left the 17 factory programs reachable solely
// through a host's own preset menu, i.e. not at all in the Standalone build, and made Gatecrasher
// inconsistent with sibling TapeRot (whose PresetStrip has prev/next arrows). A menu on the existing
// LCD was chosen over adding arrow furniture specifically because it needs no new panel artwork, so
// the static background asset is unaffected, and because it scales to an arbitrary number of user
// programs where stepping through them one at a time would not.
//
// Sized to the full canvas (matching GateScope/GateLamp/InputMeter's convention for pure overlay
// components), with hitTest narrowed to just the header cluster's own bounds so it doesn't swallow
// clicks meant for knobs/switches elsewhere on the panel - see TapeRot's PresetStrip for the same
// pattern, minus the full-canvas OR-of-rects (everything interactive here already sits inside one
// bounding rect).
class ProgramHeader final : public juce::Component, private juce::Timer
{
public:
    explicit ProgramHeader(GatecrasherAudioProcessor& processor);
    ~ProgramHeader() override;

    void paint(juce::Graphics&) override;

    /** Section 6.3: while a control is being moved the name cell shows `NAME: value unit`,
        reverting to the program name ~800ms after the gesture ends. This is the only place a live
        number appears on the panel.

        Call showParameter on drag start / value change and releaseParameter on drag end. The CALLER
        guards on the control's own drag state - a SliderAttachment also fires when a program is
        applied and on every host automation step, and without that guard the display latches onto
        whichever parameter was written last and flickers for the length of a song. */
    void showParameter(const juce::String& paramID);
    void releaseParameter();

    /** The component the Program list is laid out inside. Its bounds become the list's parent area,
        which is what fixes the list's top edge and caps its height - layout, not plumbing. Passing
        nullptr returns the list to being a free desktop window sized to its own content, which for
        a long bank overhangs the panel. See ../../CLAUDE.md, "The Program dropdown".

        This canvas is only 434px tall, the shortest in the suite, so it reaches the cap soonest:
        the list has roughly 359px to work in, about fourteen rows, and scrolls past that. */
    void setMenuParent(juce::Component* parent) noexcept { menuParent = parent; }

    /** The row the list's top edge lands on: the program window's own bottom edge, so the two read
        as one object rather than a bar with a list floating under it. */
    static int menuAnchorY() noexcept
    {
        return (int) std::floor(GatecrasherTheme::Layout::programWindowY
                                + GatecrasherTheme::Layout::programWindowH);
    }

    /** Where menuHost has to start, and it is NOT the anchor: JUCE clamps a menu to
        `jmax(parentArea.getY() + 1, ...)`, so a host beginning exactly at the anchor can only open
        one pixel below it, leaving a hairline of panel between the bar and its list.

        The lead has a floor and a ceiling. Too small and the clamp bites again; too large and the
        list can grow past the panel, because JUCE sizes it to `parentArea.getHeight() - 24` while
        the room actually below the anchor is the window's own height less than that. */
    static int menuHostTop() noexcept { return menuAnchorY() - 8; }
    bool hitTest(int x, int y) override;
    void focusLost (FocusChangeType) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    bool keyPressed(const juce::KeyPress&) override;

private:
    // Dresses the dropdown as an extension of the PROGRAM glass (section 6.2). Owned here so it
    // outlives every menu this component opens.
    GatecrasherMenuLookAndFeel menuLookAndFeel;

    enum class HeaderButton { none, save, deleteOrCancel };

    void timerCallback() override;
    void refreshDisplayFromProcessor();
    juce::String numberedProgramName() const;
    juce::String liveValueText() const;
    void enterNamingMode();
    void commitStore();
    void cancelNaming();
    void showProgramMenu();
    void refreshMeterReadoutsFromProcessor();
    bool isProgramMenuAvailableAt(juce::Point<float>) const;
    HeaderButton buttonAt(juce::Point<float>) const;
    bool isButtonEnabled(HeaderButton) const;

    GatecrasherAudioProcessor& processorRef;

    // No menuOpen flag here, unlike the siblings: this panel's chevron is BAKED into the background
    // bitmap, so there is nothing at runtime to invert while the list is open. Un-baking it is a
    // plate change, not a code one - raised with the designers rather than drawn over the top,
    // which would double-print it.
    juce::Component* menuParent = nullptr;

    // Mirrors whatever program was loaded before SAVE was pressed - CANCEL reverts the display to
    // this without ever touching APVTS (the user's tweaked-but-unsaved knob values must survive a
    // Cancel, per section 6). Never written to while namingMode is true.
    // Live-value takeover (section 6.3). editingParamID empty = showing the program name.
    juce::String editingParamID;
    juce::uint32 revertAtMs = 0;

    /** The Program the panel is currently showing, mirrored so the 20 Hz poll only repaints when
        something actually changed. An identity, not a position - so a bank that changed underneath
        cannot make this name the wrong sound. */
    ProgramId displayedId;

    /** The Programs the open menu was built from, in row order. The callback indexes this rather
        than reconstructing a Program from a number. */
    std::vector<ProgramId> menuRows;

    // Polled alongside the program index rather than queried straight from the processor inside
    // paint()/isButtonEnabled(): it changes on any parameter move, from the GUI or from host
    // automation, so it needs the same repaint-on-change handling the program index gets.
    bool displayedIsModified = false;

    // Cached formatted text rather than the raw levels, so the 20Hz poll only triggers a repaint
    // when the digits would actually differ - the underlying levels change on essentially every
    // block, but at one decimal place most of that never reaches the display.
    juce::String displayedInText, displayedOutText;

    bool namingMode = false;
    juce::String typedName;

    HeaderButton pressedButton = HeaderButton::none;

    // Tracks a press that began on the program-name cell, so releasing outside it (a drag-off)
    // cancels rather than opening the menu - same press/release contract the buttons use.
    bool pressedNameCell = false;

    juce::Rectangle<float> saveButtonRect, deleteButtonRect, headerClusterRect;
    juce::Rectangle<float> programWindowRect, tagCellRect, nameCellRect;
    juce::Rectangle<float> inWindowRect, outWindowRect;
};
