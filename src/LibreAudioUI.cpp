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
    std::unique_ptr<LibreAudioTopBarLogoWidget> fLogo = addWidget<LibreAudioTopBarLogoWidget>();
    std::unique_ptr<LibreAudioTopBarNameWidget> fPluginName = addWidget<LibreAudioTopBarNameWidget>();
    std::unique_ptr<LibreAudioWidget> fSpacer = addSpacer();
    std::unique_ptr<LibreAudioButtonGroupWidget> fUndoRedoGroup = addWidget<LibreAudioTopBarUndoRedoGroupWidget>();
    std::unique_ptr<LibreAudioButtonGroupWidget> fSnapshotsGroup = addWidget<LibreAudioTopBarSnapshotsGroupWidget>();
    std::unique_ptr<LibreAudioButtonGroupWidget> fEasyExpertGroup = addWidget<LibreAudioTopBarEasyExpertGroupWidget>();
    std::unique_ptr<LibreAudioButtonGroupWidget> fMenuPowerGroup = addWidget<LibreAudioTopBarMenuPowerGroupWidget>();

public:
    LibreAudioTopBar(LibreAudioTopLevelWidget* const parent)
        : LibreAudioContainerSubWidget(parent)
    {
    }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioMainArea : public LibreAudioContainerSubWidget<LibreAudioReference::MainArea>
{
    std::unique_ptr<LibreAudioWidget> fMetersIn = addWidget<LibreAudioMeterWidget<Input>>();
    std::unique_ptr<LibreAudioStageWidget> fStage = addWidget<LibreAudioStageWidget, Expanding>();
    std::unique_ptr<LibreAudioWidget> fMetersOut = addWidget<LibreAudioMeterWidget<Output>>();

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

        fTopBar = addWidget<LibreAudioTopBar>();
        fMainArea = addWidget<LibreAudioMainArea, Expanding>();

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

// SHADERS_LIBREAUDIO_LINE_FRAG_DATA,
// SHADERS_SHADERTOY_SQUARES_FRAG_DATA,
// SHADERS_SHADERTOY_STARRY_SKY_FRAG_DATA,
// SHADERS_LIBREAUDIO_LINE_FRAG_LEN,
// SHADERS_SHADERTOY_SQUARES_FRAG_LEN,
// SHADERS_SHADERTOY_STARRY_SKY_FRAG_LEN,

using S1 = LibreAudioBackgroundShaderWidget<SHADERS_SHADERTOY_STARRY_SKY_FRAG_DATA, SHADERS_SHADERTOY_STARRY_SKY_FRAG_LEN>;
using S2 = LibreAudioBackgroundShaderWidget<SHADERS_LIBREAUDIO_LINE_FRAG_DATA, SHADERS_LIBREAUDIO_LINE_FRAG_LEN>;

class LibreAudioUI : public LibreAudioBaseUI
{
    using R = LibreAudioReference::Window;

    double fScaleFactor = getScaleFactor();
    std::unique_ptr<S1> fShaderBackground;
    std::unique_ptr<S2> fShaderLine;
    std::unique_ptr<LibreAudioRootWidget> fRoot { new LibreAudioRootWidget(getWindow(), this) };

public:
    LibreAudioUI()
        : LibreAudioBaseUI()
    {
        // fShaderBackground.reset(new S1(this, this));
        // fShaderLine.reset(new S2(this, this));
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

    void uiIdle() final
    {
        // FIXME
        updateShaderPosition();
    }

    void uiScaleFactorChanged(const double scaleFactor) final
    {
        fScaleFactor = scaleFactor;
        // fShaderTest->setBorderRadius(LibreAudioReference::Stage::borderRadius * fScaleFactor);
        // TODO
    }

private:
    void updateShaderPosition()
    {
        if (S1* const sw = fShaderBackground.get())
        {
            sw->setAbsolutePos(fRoot->getStageAreaAbsolutePos());
            sw->setSize(fRoot->getStageAreaSize());
            sw->setBorderRadius(LibreAudioReference::Stage::borderRadius * fScaleFactor);
        }
        if (S2* const sw = fShaderLine.get())
        {
            sw->setAbsolutePos(fRoot->getStageAreaAbsolutePos());
            sw->setSize(fRoot->getStageAreaSize());
            sw->setBorderRadius(LibreAudioReference::Stage::borderRadius * fScaleFactor);
        }
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
