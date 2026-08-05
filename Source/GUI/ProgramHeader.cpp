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
    headerClusterRect = {Layout::headerCropX, Layout::headerCropY, Layout::headerCropW, Layout::headerCropH};
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

// The name cell is only a menu trigger while idle - during name entry it's the text field being
// typed into, so clicking it must not replace the half-typed name with a program list.
bool ProgramHeader::isProgramMenuAvailableAt(juce::Point<float> position) const
{
    return !namingMode && nameCellRect.contains(position);
}

void ProgramHeader::showProgramMenu()
{
    const int numPrograms = processorRef.getNumPrograms();
    const int currentIndex = processorRef.getCurrentProgram();

    // Item IDs are index + 1 because PopupMenu reserves 0 for "dismissed without choosing".
    juce::PopupMenu menu;
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

    menu.showMenuAsync(juce::PopupMenu::Options()
                           .withTargetComponent(this)
                           .withTargetScreenArea(localAreaToGlobal(nameCellRect.getSmallestIntegerContainer())),
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

    // Clear the baked LCD tag/name text (the static panel background always shows one fixed
    // example program) before drawing the live values on top - inset by 1px so the surrounding
    // LED-window border/divider drawn by the background itself stays intact. The LED window's own
    // background is a flat, untextured colour (section 1: #07090A), so a plain fill is correct
    // here without needing GatecrasherTheme::eraseToBackground's bitmap-sampling approach.
    g.setColour(Colour::ledWindowBg);
    g.fillRect(tagCellRect.reduced(1.0f));
    g.fillRect(nameCellRect.reduced(1.0f));

    const bool showUserTag = namingMode || !displayedIsFactory;
    g.setColour(showUserTag ? Colour::tagUser : Colour::tagFactory);
    g.setFont(monoFont(9.0f));
    g.drawText(showUserTag ? "USER" : "FACT", tagCellRect, juce::Justification::centred, false);

    g.setColour(Colour::headerName);
    g.setFont(monoFont(13.0f));
    if (namingMode)
    {
        // Left-aligned, cleared, with a blinking block caret (1s period, 50% duty - section 6).
        const bool caretOn = (juce::Time::getMillisecondCounter() % 1000) < 500;
        const juce::String text = typedName + (caretOn ? juce::String(juce::CharPointer_UTF8("\xe2\x96\x88"))
                                                         : juce::String());
        g.drawText(text, nameCellRect.reduced(6.0f, 0.0f), juce::Justification::centredLeft, false);
    }
    else
    {
        g.drawText(displayedProgramName, nameCellRect, juce::Justification::centred, false);

        // Small chevron marking the cell as a menu trigger (see this class's header comment on why
        // that control exists at all). Dimmed and tucked into the right edge so it reads as an
        // affordance rather than competing with the program name. The name stays centred in the full
        // cell, matching the artwork, rather than being squeezed left to make room: the widest
        // possible name (maxProgramNameLength at this font size) still ends well clear of here.
        const float chevronW = 7.0f, chevronH = 4.0f;
        const float chevronRight = nameCellRect.getRight() - 7.0f;
        const float chevronTop = nameCellRect.getCentreY() - chevronH * 0.5f;

        juce::Path chevron;
        chevron.startNewSubPath(chevronRight - chevronW, chevronTop);
        chevron.lineTo(chevronRight - chevronW * 0.5f, chevronTop + chevronH);
        chevron.lineTo(chevronRight, chevronTop);

        g.setColour(Colour::headerName.withAlpha(0.55f));
        g.strokePath(chevron, juce::PathStrokeType(1.0f));
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

        drawTrackedText(g, label, labelFont(8.5f), 1.0f, rect, juce::Justification::centred,
                         enabled ? Colour::buttonEnabledLabel : Colour::buttonDisabledLabel);
    };

    const juce::String saveLabel = namingMode ? "STORE" : "SAVE";
    const juce::String deleteLabel = namingMode ? "CANCEL" : "DELETE";
    drawButton(saveButtonRect, saveLabel, isButtonEnabled(HeaderButton::save), pressedButton == HeaderButton::save);
    drawButton(deleteButtonRect, deleteLabel, isButtonEnabled(HeaderButton::deleteOrCancel),
               pressedButton == HeaderButton::deleteOrCancel);
}
