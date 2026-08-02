// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// FIXME remove this, helper for IDE
#include "config-custom.h"

#include "LibreAudioBaseUI.hpp"
#include "LibreAudioParameters.hpp"
#include "LibreAudioStates.hpp"

#include "EventHandlers.hpp"
#include "Layout.hpp"
#include "extra/Time.hpp"

// temp stuff
// #include "DearImGui.hpp"

#include <string>
#include <vector>

#include "ui/reference.hpp"
#include "ui/widgets.hpp"

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioTopBar : public LibreAudioContainer<LibreAudioReference::TopBar>
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
        : LibreAudioContainer(parent)
    {
    }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioMainArea : public LibreAudioContainer<LibreAudioReference::MainArea>
{
    std::unique_ptr<LibreAudioMeter> fMetersIn = createWidget<LibreAudioMeter>();
    std::unique_ptr<LibreAudioStage> fSpacer = createWidget<LibreAudioStage, Expanding>();
    std::unique_ptr<LibreAudioMeter> fMetersOut = createWidget<LibreAudioMeter>();

public:
    LibreAudioMainArea(LibreAudioTopLevelWidget* const parent)
        : LibreAudioContainer(parent)
    {
    }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioUI : public LibreAudioBaseUI,
                     private ButtonEventHandler::Callback
{
    using R = LibreAudioReference::Window;

public:
    LibreAudioUI()
        : LibreAudioBaseUI()
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

        // force initial resize after creating all widgets
        ResizeEvent ev;
        ev.size = getSize();
        LibreAudioUI::onResize(ev);
    }

    ~LibreAudioUI() override
    {
    }

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // Widget Callbacks

    void buttonClicked(SubWidget* const widget, int) final
    {
        switch (widget->getId())
        {
        // case kWidgetUndo:
        //     undo();
        //     break;
        // case kWidgetRedo:
        //     redo();
        //     break;
        // case kWidgetSnapshotCopy:
        //     // if (static_cast<LibreAudioSnapshotsWidget::ImageButton*>(widget)->isChecked())
        //     snapshotButtonClicked(kSnapshotButtonCopy);
        //     break;
        // case kWidgetSnapshotA:
        //     if (static_cast<LibreAudioTextButton*>(widget)->isChecked())
        //         snapshotButtonClicked(kSnapshotButtonA);
        //     break;
        // case kWidgetSnapshotB:
        //     if (static_cast<LibreAudioTextButton*>(widget)->isChecked())
        //         snapshotButtonClicked(kSnapshotButtonB);
        //     break;
        // case kWidgetSnapshotC:
        //     if (static_cast<LibreAudioTextButton*>(widget)->isChecked())
        //         snapshotButtonClicked(kSnapshotButtonC);
        //     break;
        // case kWidgetSnapshotD:
        //     if (static_cast<LibreAudioTextButton*>(widget)->isChecked())
        //         snapshotButtonClicked(kSnapshotButtonD);
        //     break;
        // case kWidgetEasy:
        //     fExpertMode = false;
        //     break;
        // case kWidgetExpert:
        //     fExpertMode = true;
        //     break;
        // case kWidgetMenu:
        //     break;
        // case kWidgetPower:
        //     parameterControlPressed(kCommonParameterBypass);
        //     parameterControlModified(kCommonParameterBypass,
        //                              static_cast<LibreAudioPowerButton*>(widget)->isChecked() ? 1.f : 0.f);
        //     parameterControlReleased(kCommonParameterBypass);
        //     break;
        }
    }

    void uiIdle() final
    {
        LibreAudioBaseUI::uiIdle();

        // fMainArea->update(fParameterValuesRef[kParametersMainStart + djFilter::kFaustParameterKnob]);
        //
        // fTopBar->update(canUndo(),
        //                 canRedo(),
        //                 isCopyingSnapshot(),
        //                 getCurrentSnapshot(),
        //                 fExpertMode,
        //                 d_isNotZero(fParameterValuesRef[kCommonParameterBypass]));
    }

    void uiScaleFactorChanged(const double scaleFactor) final
    {
        fScaleFactor = scaleFactor;
        // TODO
    }

private:
    double fScaleFactor = getScaleFactor();
    // bool fExpertMode = false;

    std::unique_ptr<LibreAudioTopBar> fTopBar;
    std::unique_ptr<LibreAudioMainArea> fMainArea;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibreAudioUI)
};

// --------------------------------------------------------------------------------------------------------------------

UI* createUI()
{
    return new LibreAudioUI();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
