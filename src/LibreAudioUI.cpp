// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioBaseUI.hpp"
#include "LibreAudioParameters.hpp"
#include "LibreAudioStates.hpp"

#include "EventHandlers.hpp"
#include "Layout.hpp"
#include "extra/Time.hpp"

#include <string>
#include <vector>

#include "ui/reference.hpp"
#include "ui/widgets.hpp"

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioTopBar : public LibreAudioContainerSubWidget<LibreAudioReference::TopBar>
{
    std::unique_ptr<LibreAudioTopBarLogoWidget> fLogo = createWidget<LibreAudioTopBarLogoWidget>();
    std::unique_ptr<LibreAudioTopBarNameWidget> fPluginName = createWidget<LibreAudioTopBarNameWidget>();
    std::unique_ptr<LibreAudioWidget> fSpacer = createSpacer();
    std::unique_ptr<LibreAudioButtonGroupWidget> fUndoRedoGroup = createWidget<LibreAudioTopBarUndoRedoGroupWidget>();
    std::unique_ptr<LibreAudioButtonGroupWidget> fSnapshotsGroup = createWidget<LibreAudioTopBarSnapshotsGroupWidget>();
    std::unique_ptr<LibreAudioButtonGroupWidget> fEasyExpertGroup = createWidget<LibreAudioTopBarEasyExpertGroupWidget>();
    std::unique_ptr<LibreAudioButtonGroupWidget> fMenuPowerGroup = createWidget<LibreAudioTopBarMenuPowerGroupWidget>();

public:
    LibreAudioTopBar(LibreAudioTopLevelWidget* const parent)
        : LibreAudioContainerSubWidget(parent)
    {
    }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioMainArea : public LibreAudioContainerSubWidget<LibreAudioReference::MainArea>
{
    std::unique_ptr<LibreAudioMeterWidget> fMetersIn = createWidget<LibreAudioMeterWidget>();
    std::unique_ptr<LibreAudioStageWidget> fStage = createWidget<LibreAudioStageWidget, Expanding>();
    std::unique_ptr<LibreAudioMeterWidget> fMetersOut = createWidget<LibreAudioMeterWidget>();

public:
    LibreAudioMainArea(LibreAudioTopLevelWidget* const parent)
        : LibreAudioContainerSubWidget(parent)
    {
    }

    Point<int> getStageAreaAbsolutePos() const noexcept
    {
        return fStage->getAbsolutePos();
    }

    Size<uint> getStageAreaSize() const noexcept
    {
        return fStage->getSize();
    }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioRootWidget : public LibreAudioContainerRootWidget<LibreAudioReference::Window, kVertical>
{
    using R = LibreAudioReference::Window;

    std::unique_ptr<LibreAudioTopBar> fTopBar;
    std::unique_ptr<LibreAudioMainArea> fMainArea;

public:
    LibreAudioRootWidget(Window& window, LibreAudioUIWidgetInterface* const iface)
        : LibreAudioContainerRootWidget(window, iface)
    {
        createFontFromMemory("Saira Semi Condensed (Regular)",
                             FONTS_SAIRASEMICONDENSED_SEMIBOLD_TTF_DATA,
                             FONTS_SAIRASEMICONDENSED_SEMIBOLD_TTF_LEN,
                             false);
        createFontFromMemory("Saira Semi Condensed (SemiBold)",
                             FONTS_SAIRASEMICONDENSED_SEMIBOLD_TTF_DATA,
                             FONTS_SAIRASEMICONDENSED_SEMIBOLD_TTF_LEN,
                             false);
        fontFace("Saira Semi Condensed (Regular)");

        fTopBar = createWidget<LibreAudioTopBar>();
        fMainArea = createWidget<LibreAudioMainArea, Expanding>();

        // fake a resize after creating all widgets, to move everything into place
        ResizeEvent ev;
        ev.size = getSize();
        LibreAudioRootWidget::onResize(ev);
    }

    Point<int> getStageAreaAbsolutePos() const noexcept
    {
        return fMainArea->getStageAreaAbsolutePos();
    }

    Size<uint> getStageAreaSize() const noexcept
    {
        return fMainArea->getStageAreaSize();
    }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioUI : public LibreAudioBaseUI
{
    using R = LibreAudioReference::Window;

    double fScaleFactor = getScaleFactor();
    std::unique_ptr<LibreAudioBackgroundShaderWidget> fShaderTest { new LibreAudioBackgroundShaderWidget(this) };
    std::unique_ptr<LibreAudioRootWidget> fRoot { new LibreAudioRootWidget(getWindow(), this) };

public:
    LibreAudioUI()
        : LibreAudioBaseUI()
    {
        updateShaderPosition();
    }

    ~LibreAudioUI() override
    {
    }

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // Widget Callbacks

    void onNanoDisplay() override
    {
        const float w = getWidth();
        const float h = getHeight();

        beginPath();
        rect(0, 0, w, h);
        fillPaint(linearGradient(0, 0, 0, h, R::backgroundGradientStart, R::backgroundGradientStop));
        fill();
    }

    void onResize(const ResizeEvent& ev) override
    {
        LibreAudioBaseUI::onResize(ev);
        updateShaderPosition();
    }

    void uiScaleFactorChanged(const double scaleFactor) final
    {
        // fScaleFactor = scaleFactor;
        // TODO
    }

private:
    void updateShaderPosition()
    {
        fShaderTest->setAbsolutePos(fRoot->getStageAreaAbsolutePos());
        fShaderTest->setSize(fRoot->getStageAreaSize());
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibreAudioUI)
};

// --------------------------------------------------------------------------------------------------------------------

UI* createUI()
{
    return new LibreAudioUI();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
