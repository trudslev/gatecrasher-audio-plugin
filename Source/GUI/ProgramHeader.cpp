#include "ProgramHeader.h"
#include "GatecrasherTheme.h"

namespace
{
    // The IN/OUT windows' text. Below the meter's own floor the numeric value stops being
    // meaningful (and -60.0 reads as a real level rather than "nothing"), so it collapses to -INF,
    // matching the scope's own -inf annotation. Positive values carry an explicit + so a hot output
    // is unambiguous next to the negative values these windows normally show.
    juce::String formatMeterReadout(float linearLevel)
    {
        using namespace GatecrasherTheme;

        const float db = juce::Decibels::gainToDecibels(juce::jmax(0.0f, linearLevel),
                                                         Layout::meterFloorDb);
        if (db <= Layout::meterFloorDb + 0.05f)
            return "-INF";

        return (db > 0.0f ? "+" : "") + juce::String(db, 1);
    }
}

ProgramHeader::ProgramHeader(GatecrasherAudioProcessor& processor) : processorRef(processor)
{
    using namespace GatecrasherTheme;

    saveButtonRect = {Layout::saveButtonX, Layout::saveButtonY, Layout::saveButtonW, Layout::saveButtonH};
    deleteButtonRect = {Layout::deleteButtonX, Layout::deleteButtonY, Layout::deleteButtonW, Layout::deleteButtonH};
    // The cluster is what hitTest claims: the program window plus both buttons plus the two meter
    // windows, taken from section 6's own coordinates rather than from a crop of the render.
    programWindowRect = {Layout::programWindowX, Layout::programWindowY,
                          Layout::programWindowW, Layout::programWindowH};
    headerClusterRect = programWindowRect
                            .getUnion({Layout::saveButtonX, Layout::saveButtonY,
                                        Layout::saveButtonW, Layout::saveButtonH})
                            .getUnion({Layout::deleteButtonX, Layout::deleteButtonY,
                                        Layout::deleteButtonW, Layout::deleteButtonH});
    tagCellRect = {Layout::programTagCellX, Layout::programTagCellY, Layout::programTagCellW, Layout::programTagCellH};
    nameCellRect = {Layout::programNameCellX, Layout::programNameCellY, Layout::programNameCellW, Layout::programNameCellH};
    inWindowRect = {Layout::inWindowX, Layout::inWindowY, Layout::inWindowW, Layout::inWindowH};
    outWindowRect = {Layout::outWindowX, Layout::outWindowY, Layout::outWindowW, Layout::outWindowH};

    displayedProgramIndex = processorRef.getCurrentProgram();
    displayedProgramName = processorRef.getProgramName(displayedProgramIndex);
    displayedIsFactory = processorRef.isFactoryProgram(displayedProgramIndex);
    displayedIsModified = processorRef.isCurrentProgramModified();
    displayedInText = formatMeterReadout(processorRef.getInputMeterLevel());
    displayedOutText = formatMeterReadout(processorRef.getOutputMeterLevel());

    setWantsKeyboardFocus(true);
    startTimerHz(20);
}

ProgramHeader::~ProgramHeader()
{
    stopTimer();
}

bool ProgramHeader::hitTest(int x, int y)
{
    return headerClusterRect.contains((float) x, (float) y);
}

void ProgramHeader::timerCallback()
{
    // Live-value takeover reverts on its own clock rather than a second timer.
    if (revertAtMs != 0 && juce::Time::getMillisecondCounter() >= revertAtMs)
    {
        revertAtMs = 0;
        editingParamID.clear();
        repaint(nameCellRect.getSmallestIntegerContainer());
    }

    refreshDisplayFromProcessor();
    refreshMeterReadoutsFromProcessor();
    if (namingMode)
        repaint(); // caret blink needs a steady repaint even when nothing else changed
}

void ProgramHeader::refreshMeterReadoutsFromProcessor()
{
    const auto inText = formatMeterReadout(processorRef.getInputMeterLevel());
    const auto outText = formatMeterReadout(processorRef.getOutputMeterLevel());

    if (inText == displayedInText && outText == displayedOutText)
        return;

    displayedInText = inText;
    displayedOutText = outText;

    // Only the two windows, not the whole component: this fires far more often than any other
    // change here, and this component spans the entire canvas (see the class comment), so a plain
    // repaint() would redraw the whole header cluster several times a second for two digits.
    repaint(inWindowRect.getSmallestIntegerContainer()
                .getUnion(outWindowRect.getSmallestIntegerContainer()));
}

void ProgramHeader::showParameter(const juce::String& paramID)
{
    if (namingMode)
        return;    // the glass belongs to the name field until it commits or cancels

    editingParamID = paramID;
    revertAtMs = 0;
    repaint(nameCellRect.getSmallestIntegerContainer());
}

void ProgramHeader::releaseParameter()
{
    if (editingParamID.isNotEmpty())
        revertAtMs = juce::Time::getMillisecondCounter()
                         + (juce::uint32) GatecrasherTheme::Layout::lcdRevertMs;
}

juce::String ProgramHeader::numberedProgramName() const
{
    // Section 6.1 sizes the cell around "14 ROOM REINFORCEMENT" and "the two-digit index and space"
    // on top of the 24-character name cap - so the index is part of what the cell shows, 1-based and
    // zero-padded to two digits. Uppercase throughout: the entry path already uppercases what is
    // typed (section 6.4), and the factory names are stored in title case, so without this the two
    // banks would read in different cases through the same window.
    return juce::String(displayedProgramIndex + 1).paddedLeft('0', 2)
           + " " + displayedProgramName.toUpperCase();
}

juce::String ProgramHeader::liveValueText() const
{
    auto* param = processorRef.apvts.getParameter(editingParamID);
    if (param == nullptr)
        return {};

    // Straight through the parameter's own getText, so the number and its unit match what the host
    // shows for the same control - there is no second formatting convention to keep in step. The
    // four skewed knobs need no special handling: their printed ticks were placed from the same
    // power-law curve the parameter uses (section 4.2), so the value always agrees with the mark
    // the pointer is sitting on.
    const auto name = param->getName(24).toUpperCase();
    const auto unit = param->getLabel();
    auto value = param->getText(param->getValue(), 0);

    // Section 6.3's examples set the case: "THRESHOLD: -18.5 dB" and "TRIG LP: 6.3 kHz" keep their
    // units as written, while "ALGORITHM: PLATE" is capitalised. The difference is a unit - the
    // float parameters all carry one (or bake it into the text, as the two frequencies do), the
    // choice parameters do not - so a unitless value is a word and gets the name's treatment.
    if (unit.isEmpty() && ! value.containsAnyOf("0123456789"))
        value = value.toUpperCase();

    return name + ": " + value + (unit.isEmpty() ? juce::String() : " " + unit);
}

void ProgramHeader::refreshDisplayFromProcessor()
{
    bool changed = false;

    const int index = processorRef.getCurrentProgram();
    if (index != displayedProgramIndex)
    {
        displayedProgramIndex = index;
        displayedProgramName = processorRef.getProgramName(index);
        displayedIsFactory = processorRef.isFactoryProgram(index);
        changed = true;
    }

    // Naming mode shows STORE, which must stay enabled regardless of whether anything was modified
    // (its own value is what's being stored) - so the modified flag isn't tracked while naming, and
    // isButtonEnabled short-circuits on namingMode before ever reading it.
    if (! namingMode)
    {
        const bool modified = processorRef.isCurrentProgramModified();
        if (modified != displayedIsModified)
        {
            displayedIsModified = modified;
            changed = true;
        }
    }

    if (changed)
        repaint();
}

ProgramHeader::HeaderButton ProgramHeader::buttonAt(juce::Point<float> position) const
{
    if (saveButtonRect.contains(position))
        return HeaderButton::save;
    if (deleteButtonRect.contains(position))
        return HeaderButton::deleteOrCancel;
    return HeaderButton::none;
}

bool ProgramHeader::isButtonEnabled(HeaderButton button) const
{
    if (namingMode)
        return button == HeaderButton::save || button == HeaderButton::deleteOrCancel; // STORE/CANCEL, both enabled

    if (button == HeaderButton::save)
        return displayedIsModified; // nothing changed since the program loaded = nothing to save
    if (button == HeaderButton::deleteOrCancel)
        return !displayedIsFactory; // DELETE disabled for read-only factory programs
    return false;
}

// Section 6.2: "clicking anywhere in the window opens the program list" - the whole recessed LCD,
// not just the name cell. The baked chevron marks it as a selector but is not a button of its own.
//
// Only while idle: during name entry the cell is the text field being typed into, so a click must
// not replace a half-typed name with a program list.
bool ProgramHeader::isProgramMenuAvailableAt(juce::Point<float> position) const
{
    return !namingMode && programWindowRect.contains(position);
}

void ProgramHeader::showProgramMenu()
{
    const int numPrograms = processorRef.getNumPrograms();
    const int currentIndex = processorRef.getCurrentProgram();

    // Item IDs are index + 1 because PopupMenu reserves 0 for "dismissed without choosing".
    juce::PopupMenu menu;
    menu.setLookAndFeel(&menuLookAndFeel);
    bool hasUserPrograms = false;

    menu.addSectionHeader("Factory");
    for (int i = 0; i < numPrograms; ++i)
    {
        if (processorRef.isFactoryProgram(i))
            menu.addItem(i + 1, processorRef.getProgramName(i), true, i == currentIndex);
        else
            hasUserPrograms = true;
    }

    // Second pass rather than one loop building two menus: user programs always sort after the
    // factory bank by index (see ProgramManager), so this keeps the menu in index order without
    // needing an intermediate submenu.
    if (hasUserPrograms)
    {
        menu.addSeparator();
        menu.addSectionHeader("User");
        for (int i = 0; i < numPrograms; ++i)
            if (!processorRef.isFactoryProgram(i))
                menu.addItem(i + 1, processorRef.getProgramName(i), true, i == currentIndex);
    }

    // Anchored to, and at least as wide as, the whole program window rather than the name cell -
    // section 6.2 makes the entire window the affordance, so the list should drop from its full
    // width. localAreaToGlobal keeps this right on a scaled or moved editor.
    const auto glassOnScreen = localAreaToGlobal(programWindowRect.getSmallestIntegerContainer());
    const auto window = programWindowRect.getSmallestIntegerContainer();

    auto options = juce::PopupMenu::Options()
                       .withTargetComponent(this)
                       .withTargetScreenArea(glassOnScreen)
                       .withMaximumNumColumns(1);

    if (menuParent != nullptr)
    {
        // The list is laid out INSIDE menuHost rather than as its own desktop window. JUCE fits a
        // menu to its parent area, so an area running from the window's bottom edge to the panel's
        // gives both guarantees at once: the top cannot move and the height cannot exceed the
        // panel. A bank too long to fit scrolls. See ../../CLAUDE.md, "The Program dropdown".
        //
        // Anchor to a 1px strip on the window's bottom EDGE, not the window. With a parent, JUCE
        // first does constrainedWithin(parentArea), which slides the whole 25px window down into
        // the host before measuring and opens the list 25px too low. 1px and not zero: a
        // zero-height rectangle is isEmpty(), which drops the list out of align-to-rectangle into
        // the sideways placement meant for submenus.
        const juce::Rectangle<int> anchor { window.getX(), menuAnchorY() - 1, window.getWidth(), 1 };

        options = options.withTargetScreenArea(localAreaToGlobal(anchor))
                         .withParentComponent(menuParent)
                         .withMinimumWidth(window.getWidth());
    }
    else
    {
        options = options.withMinimumWidth(glassOnScreen.getWidth());
    }

    menu.showMenuAsync(options,
                       [safeThis = juce::Component::SafePointer<ProgramHeader>(this)](int result)
                       {
                           if (safeThis == nullptr || result == 0)
                               return;

                           // Goes through ProgramManager's async apply path (see its AsyncUpdater) -
                           // the 20Hz timerCallback picks the change up and repaints, so there's
                           // deliberately no forced refresh here.
                           safeThis->processorRef.setCurrentProgram(result - 1);
                       });
}

void ProgramHeader::mouseMove(const juce::MouseEvent& e)
{
    // Position-dependent, so it can't be a one-off setMouseCursor in the constructor: the whole
    // component spans the canvas, and only this one cell is clickable.
    setMouseCursor(isProgramMenuAvailableAt(e.position) ? juce::MouseCursor::PointingHandCursor
                                                        : juce::MouseCursor::NormalCursor);
}

void ProgramHeader::mouseDown(const juce::MouseEvent& e)
{
    pressedNameCell = isProgramMenuAvailableAt(e.position);

    const auto candidate = buttonAt(e.position);
    pressedButton = isButtonEnabled(candidate) ? candidate : HeaderButton::none;
    if (pressedButton != HeaderButton::none)
        repaint();
}

void ProgramHeader::mouseUp(const juce::MouseEvent& e)
{
    if (pressedNameCell)
    {
        pressedNameCell = false;
        if (isProgramMenuAvailableAt(e.position))
        {
            showProgramMenu();
            return;
        }
    }

    const auto released = buttonAt(e.position);
    if (released != HeaderButton::none && released == pressedButton)
    {
        if (released == HeaderButton::save)
        {
            if (namingMode)
                commitStore();
            else
                enterNamingMode();
        }
        else // deleteOrCancel
        {
            if (namingMode)
                cancelNaming();
            else if (!displayedIsFactory)
                processorRef.deleteUserProgram(displayedProgramIndex);
        }
    }

    if (pressedButton != HeaderButton::none)
    {
        pressedButton = HeaderButton::none;
        repaint();
    }
}

void ProgramHeader::enterNamingMode()
{
    namingMode = true;
    typedName.clear();
    grabKeyboardFocus();
    repaint();
}

void ProgramHeader::commitStore()
{
    // Empty name -> ProgramManager's own "NEW PROGRAM" fallback handles it (section 6) - not
    // duplicated here.
    processorRef.saveNewUserProgram(typedName.trim());

    namingMode = false;
    typedName.clear();
    giveAwayKeyboardFocus();

    // saveNewUserProgram applies synchronously (see ProgramManager::saveNewUserProgram), so the
    // new program is already current by the time this returns.
    refreshDisplayFromProcessor();
    repaint();
}

void ProgramHeader::cancelNaming()
{
    // Must NOT touch APVTS parameters - the user's tweaked-but-unsaved knob values survive a
    // Cancel (section 6). displayedProgramIndex/Name/IsFactory were never written to while naming,
    // so simply leaving naming mode reverts the display to whatever was loaded before SAVE.
    namingMode = false;
    typedName.clear();
    giveAwayKeyboardFocus();
    repaint();
}

bool ProgramHeader::keyPressed(const juce::KeyPress& key)
{
    if (!namingMode)
        return false;

    if (key.isKeyCode(juce::KeyPress::returnKey))
    {
        commitStore();
        return true;
    }
    if (key.isKeyCode(juce::KeyPress::escapeKey))
    {
        cancelNaming();
        return true;
    }
    if (key.isKeyCode(juce::KeyPress::backspaceKey))
    {
        if (typedName.isNotEmpty())
            typedName = typedName.dropLastCharacters(1);
        repaint();
        return true;
    }

    const juce::juce_wchar character = key.getTextCharacter();
    if (character >= 32 && character != 127
        && typedName.length() < GatecrasherTheme::Layout::maxProgramNameLength)
    {
        typedName += juce::String::charToString(character).toUpperCase();
        repaint();
        return true;
    }

    return false;
}

void ProgramHeader::paint(juce::Graphics& g)
{
    using namespace GatecrasherTheme;

    // Section 6.2's caption, drawn rather than baked since Rev 8 - PROGRAM normally, NAME PROGRAM
    // while naming. Barlow Condensed 600 at 10 CSS px, .22em, in the functional ink. No explicit
    // erase: this component is not opaque, so JUCE repaints the plate underneath first, and the
    // longer word's tail therefore clears itself when naming ends.
    {
        const auto captionFont = labelFont(labelFontHeightForCssPx(Layout::programCaptionCssPx));
        const juce::Rectangle<float> captionRect(
            Layout::programCaptionX,
            Layout::programCaptionBaselineY - captionFont.getAscent(),
            200.0f, captionFont.getAscent() + captionFont.getDescent());

        drawTrackedText(g, namingMode ? "NAME PROGRAM" : "PROGRAM", captionFont,
                         trackingPxForEm(Layout::programCaptionTrackingEm, Layout::programCaptionCssPx),
                         captionRect, juce::Justification::left, Colour::labelSelected);
    }

    // Clear the two cells before drawing. The plate leaves the LCD windows empty (section 0.2:
    // the tag, name and live value are all R), so this is clearing the PREVIOUS FRAME, not baked
    // artwork. Inset 1px to leave the window's own baked border and divider intact. The glass is a
    // flat untextured #07090A, so a plain fill is right here - no bitmap sampling needed.
    g.setColour(Colour::ledWindowBg);
    g.fillRect(tagCellRect.reduced(1.0f));
    g.fillRect(nameCellRect.reduced(1.0f));

    // Section 6.2: the tag is "same face, size and colour as the name" - one 13px Share Tech Mono
    // in #F0E0B0, not the two greys Rev 5 used to distinguish FACT from USER. The word itself
    // carries the distinction.
    // Section 6.1 puts both cells at Share Tech Mono 13px with 1.3px (.10em) letter-spacing, and it
    // is that tracking the 8.32px-per-character budget is built on - drawn without it the name sits
    // visibly tighter than the plate's own printed type and the budget stops meaning anything.
    const auto lcdFont = monoFont(monoFontHeightForCssPx(13.0f));
    const float lcdTracking = trackingPxForEm(0.10f, 13.0f);

    const bool showUserTag = namingMode || !displayedIsFactory;
    drawTrackedText(g, showUserTag ? "USER" : "FACT", lcdFont, lcdTracking, tagCellRect,
                     juce::Justification::centred, Colour::ledText);

    if (namingMode)
    {
        // Left-aligned, cleared, with a blinking block caret (1s period, 50% duty - section 6).
        const bool caretOn = (juce::Time::getMillisecondCounter() % 1000) < 500;
        const juce::String text = typedName + (caretOn ? juce::String(juce::CharPointer_UTF8("\xe2\x96\x88"))
                                                         : juce::String());
        drawTrackedText(g, text, lcdFont, lcdTracking, nameCellRect.reduced(6.0f, 0.0f),
                         juce::Justification::left, Colour::ledText);
    }
    else
    {
        // Section 6.4: a modified program gains a trailing " *", cleared on store, on delete and
        // on loading another program - all three of which reset displayedIsModified via
        // refreshDisplayFromProcessor, so nothing extra is needed to clear it here.
        const auto shown = editingParamID.isNotEmpty()
                               ? liveValueText()
                               : numberedProgramName() + (displayedIsModified ? " *" : "");
        drawTrackedText(g, shown, lcdFont, lcdTracking, nameCellRect,
                         juce::Justification::centred, Colour::ledText);
    }

    // IN / OUT level windows. Same erase-then-draw as the tag/name cells above (and the same flat
    // fill, the window interior being an untextured #07090A): the static background bakes in one
    // frozen reading from whenever the reference render was captured, which otherwise just sits
    // there looking like a live meter that never moves.
    g.setColour(Colour::ledWindowBg);
    g.fillRect(inWindowRect.reduced(1.0f));
    g.fillRect(outWindowRect.reduced(1.0f));

    static const float meterReadoutHeight =
        monoFontHeightForTrackedWidth(Layout::meterReadoutRefText, 0.0f, Layout::meterReadoutRefWidth);

    g.setColour(Colour::ledText);
    g.setFont(monoFont(meterReadoutHeight));
    g.drawText(displayedInText, inWindowRect, juce::Justification::centred, false);
    g.drawText(displayedOutText, outWindowRect, juce::Justification::centred, false);

    // SAVE/DELETE (STORE/CANCEL while naming): drawn live (gradient fill + border + label per
    // section 6's exact state table) rather than composited from the header-state bitmaps - see
    // this file's class comment in the header for why that bitmap-crop approach was abandoned.
    // Erase each button's baked default-state pixels back to the real background first (the main
    // panel background's own baked buttons are correct only for the specific factory program that
    // reference screenshot happened to be captured showing), then draw the button's actual state.
    auto drawButton = [&] (juce::Rectangle<float> rect, const juce::String& label, bool enabled, bool pressed)
    {
        eraseToBackground(g, rect, getPosition());

        const auto top = pressed ? Colour::buttonPressedTop
                                  : (enabled ? Colour::buttonEnabledTop : Colour::buttonDisabledTop);
        const auto bottom = pressed ? Colour::buttonPressedBottom
                                     : (enabled ? Colour::buttonEnabledBottom : Colour::buttonDisabledBottom);
        juce::ColourGradient fill(top, rect.getX(), rect.getY(), bottom, rect.getX(), rect.getBottom(), false);
        g.setGradientFill(fill);
        g.fillRect(rect);

        g.setColour(enabled ? Colour::buttonEnabledBorder : Colour::buttonDisabledBorder);
        g.drawRect(rect, 1.0f);

        // Section 6.4: Barlow Condensed 600 at 10 CSS px, .10em. Through the CSS-px converter rather
        // than the pre-converted number that used to sit here, so a recalibration of the ratio moves
        // this with every other label instead of leaving it behind.
        drawTrackedText(g, label, labelFont(labelFontHeightForCssPx(10.0f)),
                         trackingPxForEm(0.10f, 10.0f), rect, juce::Justification::centred,
                         enabled ? Colour::buttonEnabledLabel : Colour::buttonDisabledLabel);
    };

    const juce::String saveLabel = namingMode ? "STORE" : "SAVE";
    const juce::String deleteLabel = namingMode ? "CANCEL" : "DELETE";
    drawButton(saveButtonRect, saveLabel, isButtonEnabled(HeaderButton::save), pressedButton == HeaderButton::save);
    drawButton(deleteButtonRect, deleteLabel, isButtonEnabled(HeaderButton::deleteOrCancel),
               pressedButton == HeaderButton::deleteOrCancel);
}
