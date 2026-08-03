// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "widgets/button.hpp"
#include "widgets/button-group.hpp"
#include "widgets/knob.hpp"
#include "widgets/knob-group.hpp"
#include "widgets/meter.hpp"
#include "widgets/stage.hpp"
#include "widgets/top-bar-name.hpp"

#include "las-resources.h"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

enum WidgetIds {
    kWidgetIdStart = 1000,
    kWidgetUndo,
    kWidgetRedo,
    kWidgetSnapshotCopy,
    kWidgetSnapshotA,
    kWidgetSnapshotB,
    kWidgetSnapshotC,
    kWidgetSnapshotD,
    kWidgetEasy,
    kWidgetExpert,
    kWidgetMenu,
    kWidgetPower,
};

// --------------------------------------------------------------------------------------------------------------------

using LibreAudioTopBarLogoWidget = LibreAudioImageWidget<IMAGES_LA_PNG_DATA, IMAGES_LA_PNG_LEN>;

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioTopBarUndoRedoGroupWidget : public LibreAudioButtonGroupWidget,
                                            private ButtonEventHandler::Callback,
                                            private IdleCallback
{
    std::unique_ptr<LibreAudioButtonWidget> fUndo = addButton<LibreAudioImageButtonWidget<IMAGES_UNDO_PNG_DATA, IMAGES_UNDO_PNG_LEN>>();
    std::unique_ptr<LibreAudioButtonWidget> fRedo = addButton<LibreAudioImageButtonWidget<IMAGES_REDO_PNG_DATA, IMAGES_REDO_PNG_LEN>>();

public:
    explicit LibreAudioTopBarUndoRedoGroupWidget(LibreAudioWidget* const parent)
        : LibreAudioButtonGroupWidget(parent)
    {
        done(this);

        fUndo->setId(kWidgetUndo);
        fRedo->setId(kWidgetRedo);

        // no undo/redo by default
        fUndo->setEnabled(false);
        fRedo->setEnabled(false);

        getTopLevelWidget()->addIdleCallback(this);
    }

private:
    void buttonClicked(SubWidget* const widget, int) final
    {
        switch (widget->getId())
        {
        case kWidgetUndo:
            fInterface->undo();
            break;
        case kWidgetRedo:
            fInterface->redo();
            break;
        }
    }

    void idleCallback() final
    {
        fUndo->setEnabled(fInterface->canUndo());
        fRedo->setEnabled(fInterface->canRedo());
    }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioTopBarSnapshotsGroupWidget : public LibreAudioButtonGroupWidget,
                                             private ButtonEventHandler::Callback,
                                             private IdleCallback
{
    static constexpr const char kTextA[] = "A";
    static constexpr const char kTextB[] = "B";
    static constexpr const char kTextC[] = "C";
    static constexpr const char kTextD[] = "D";
    std::unique_ptr<LibreAudioButtonWidget> fCopy = addButton<LibreAudioImageButtonWidget<IMAGES_COPY_PNG_DATA, IMAGES_COPY_PNG_LEN>>();
    std::unique_ptr<LibreAudioButtonWidget> fA = addButton<LibreAudioTextButtonWidget<kTextA>>();
    std::unique_ptr<LibreAudioButtonWidget> fB = addButton<LibreAudioTextButtonWidget<kTextB>>();
    std::unique_ptr<LibreAudioButtonWidget> fC = addButton<LibreAudioTextButtonWidget<kTextC>>();
    std::unique_ptr<LibreAudioButtonWidget> fD = addButton<LibreAudioTextButtonWidget<kTextD>>();

public:
    explicit LibreAudioTopBarSnapshotsGroupWidget(LibreAudioWidget* const parent)
        : LibreAudioButtonGroupWidget(parent)
    {
        fA->setWidth(fCopy->getWidth());
        fB->setWidth(fCopy->getWidth());
        fC->setWidth(fCopy->getWidth());
        fD->setWidth(fCopy->getWidth());
        done(this);

        fCopy->setCheckable(true);
        fA->setCheckable(true);
        fB->setCheckable(true);
        fC->setCheckable(true);
        fD->setCheckable(true);

        fCopy->setId(kWidgetSnapshotCopy);
        fA->setId(kWidgetSnapshotA);
        fB->setId(kWidgetSnapshotB);
        fC->setId(kWidgetSnapshotC);
        fD->setId(kWidgetSnapshotD);

        getTopLevelWidget()->addIdleCallback(this);
    }

private:
    void buttonClicked(SubWidget* const widget, int) final
    {
        switch (widget->getId())
        {
        case kWidgetSnapshotCopy:
            fInterface->snapshotButtonClicked(LibreAudioUIWidgetInterface::kSnapshotButtonCopy);
            break;
        case kWidgetSnapshotA:
            fInterface->snapshotButtonClicked(LibreAudioUIWidgetInterface::kSnapshotButtonA);
            break;
        case kWidgetSnapshotB:
            fInterface->snapshotButtonClicked(LibreAudioUIWidgetInterface::kSnapshotButtonB);
            break;
        case kWidgetSnapshotC:
            fInterface->snapshotButtonClicked(LibreAudioUIWidgetInterface::kSnapshotButtonC);
            break;
        case kWidgetSnapshotD:
            fInterface->snapshotButtonClicked(LibreAudioUIWidgetInterface::kSnapshotButtonD);
            break;
        }

        update();
    }

    void idleCallback() final
    {
        update();
    }

    void update()
    {
        fCopy->setChecked(fInterface->isCopyingSnapshot(), false);

        const uint8_t snapshot = fInterface->getCurrentSnapshot();
        fA->setChecked(snapshot == 0, false);
        fB->setChecked(snapshot == 1, false);
        fC->setChecked(snapshot == 2, false);
        fD->setChecked(snapshot == 3, false);
    }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioTopBarEasyExpertGroupWidget : public LibreAudioButtonGroupWidget,
                                              private ButtonEventHandler::Callback
{
    static constexpr const char kTextEasy[] = "Easy";
    static constexpr const char kTextExpert[] = "Expert";
    std::unique_ptr<LibreAudioButtonWidget> fEasy = addButton<LibreAudioTextButtonWidget<kTextEasy>>();
    std::unique_ptr<LibreAudioButtonWidget> fExpert = addButton<LibreAudioTextButtonWidget<kTextExpert>>();

    // easy mode checked by default
    bool fEasyMode = true;

public:
    explicit LibreAudioTopBarEasyExpertGroupWidget(LibreAudioWidget* const parent)
        : LibreAudioButtonGroupWidget(parent)
    {
        done(this);

        fEasy->setCheckable(true);
        fExpert->setCheckable(true);

        fEasy->setChecked(fEasyMode, false);
        fExpert->setChecked(!fEasyMode, false);

        fEasy->setId(kWidgetEasy);
        fExpert->setId(kWidgetExpert);
    }

private:
    void buttonClicked(SubWidget*, int) final
    {
        fEasyMode = !fEasyMode;
        fEasy->setChecked(fEasyMode, false);
        fExpert->setChecked(!fEasyMode, false);

        // fInterface->
    }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioTopBarMenuPowerGroupWidget : public LibreAudioButtonGroupWidget,
                                             private ButtonEventHandler::Callback,
                                             private IdleCallback
{
    std::unique_ptr<LibreAudioButtonWidget> fMenu = addButton<LibreAudioImageButtonWidget<IMAGES_MENU_PNG_DATA, IMAGES_MENU_PNG_LEN>>();
    std::unique_ptr<LibreAudioButtonWidget> fPower = addButton<LibreAudioImageButtonWidget<IMAGES_POWER_PNG_DATA, IMAGES_POWER_PNG_LEN>>();

public:
    explicit LibreAudioTopBarMenuPowerGroupWidget(LibreAudioWidget* const parent)
        : LibreAudioButtonGroupWidget(parent)
    {
        done(this);

        fMenu->setCheckable(true);
        fPower->setCheckable(true);

        fMenu->setId(kWidgetMenu);
        fPower->setId(kWidgetPower);

        getTopLevelWidget()->addIdleCallback(this);
    }

private:
    void buttonClicked(SubWidget* const widget, int) final
    {
        switch (widget->getId())
        {
        case kWidgetMenu:
            // TODO
            break;
        case kWidgetPower:
            fInterface->parameterControlPressed(kCommonParameterBypass);
            fInterface->parameterControlModified(
                kCommonParameterBypass, static_cast<LibreAudioButtonWidget*>(widget)->isChecked() ? 1.f : 0.f);
            fInterface->parameterControlReleased(kCommonParameterBypass);
            break;
        }
    }

    void idleCallback() final
    {
        // NOTE this only triggers updates if the value doesnt match
        fPower->setChecked(d_isNotZero(fInterface->getParameterValue(kCommonParameterBypass)), true);
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
