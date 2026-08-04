#include "ProgramHeader.h"
#include "GatecrasherTheme.h"

ProgramHeader::ProgramHeader(GatecrasherAudioProcessor& processor) : processorRef(processor)
{
    using namespace GatecrasherTheme;

    saveButtonRect = {Layout::saveButtonX, Layout::saveButtonY, Layout::saveButtonW, Layout::saveButtonH};
    deleteButtonRect = {Layout::deleteButtonX, Layout::deleteButtonY, Layout::deleteButtonW, Layout::deleteButtonH};
    headerClusterRect = {Layout::headerCropX, Layout::headerCropY, Layout::headerCropW, Layout::headerCropH};
    tagCellRect = {Layout::programTagCellX, Layout::programTagCellY, Layout::programTagCellW, Layout::programTagCellH};
    nameCellRect = {Layout::programNameCellX, Layout::programNameCellY, Layout::programNameCellW, Layout::programNameCellH};

    displayedProgramIndex = processorRef.getCurrentProgram();
    displayedProgramName = processorRef.getProgramName(displayedProgramIndex);
    displayedIsFactory = processorRef.isFactoryProgram(displayedProgramIndex);

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
    if (namingMode)
        repaint(); // caret blink needs a steady repaint even when nothing else changed
}

void ProgramHeader::refreshDisplayFromProcessor()
{
    const int index = processorRef.getCurrentProgram();
    if (index != displayedProgramIndex)
    {
        displayedProgramIndex = index;
        displayedProgramName = processorRef.getProgramName(index);
        displayedIsFactory = processorRef.isFactoryProgram(index);
        repaint();
    }
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
        return true; // SAVE is always enabled
    if (button == HeaderButton::deleteOrCancel)
        return !displayedIsFactory; // DELETE disabled for read-only factory programs
    return false;
}

void ProgramHeader::mouseDown(const juce::MouseEvent& e)
{
    const auto candidate = buttonAt(e.position);
    pressedButton = isButtonEnabled(candidate) ? candidate : HeaderButton::none;
    if (pressedButton != HeaderButton::none)
        repaint();
}

void ProgramHeader::mouseUp(const juce::MouseEvent& e)
{
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

    const auto& sourceImage = namingMode ? headerNameEntryImage()
                                          : (displayedIsFactory ? headerFactoryImage() : headerUserImage());

    // The header bitmaps are full-width renders of the whole header band (wordmark included) -
    // only the "program cluster" sub-rect is blitted here; WordmarkComponent owns the wordmark
    // itself so the two never double-paint the same pixels (see Layout::headerCropX's comment).
    const juce::Rectangle<int> destRect = headerClusterRect.getSmallestIntegerContainer();
    const juce::Rectangle<int> srcRect((int) std::round(Layout::headerCropX * Layout::headerAssetSrcScale),
                                        (int) std::round(Layout::headerCropY * Layout::headerAssetSrcScale),
                                        (int) std::round(Layout::headerCropW * Layout::headerAssetSrcScale),
                                        (int) std::round(Layout::headerCropH * Layout::headerAssetSrcScale));

    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImage(sourceImage, destRect.getX(), destRect.getY(), destRect.getWidth(), destRect.getHeight(),
                srcRect.getX(), srcRect.getY(), srcRect.getWidth(), srcRect.getHeight());

    // Clear the baked placeholder tag/name text (the bitmap always shows the mockup's example
    // state) before drawing the live values on top - inset by 1px so the surrounding LED-window
    // border/divider drawn by the bitmap itself stays intact.
    g.setColour(Colour::ledWindowBg);
    g.fillRect(tagCellRect.reduced(1.0f));
    g.fillRect(nameCellRect.reduced(1.0f));

    // TODO(design): Share Tech Mono not yet in design/assets/ - default monospaced placeholder.
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
    }

    // SAVE/DELETE (STORE/CANCEL while naming) have no dedicated "pressed" bitmap state, so press
    // feedback is a simple translucent overlay rather than a swapped asset.
    if (pressedButton != HeaderButton::none)
    {
        g.setColour(juce::Colours::black.withAlpha(0.22f));
        g.fillRect(pressedButton == HeaderButton::save ? saveButtonRect : deleteButtonRect);
    }
}
