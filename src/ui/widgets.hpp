// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "widgets/button.hpp"
#include "widgets/button-group.hpp"
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

class LibreAudioTopBarUndoRedoGroupWidget : public LibreAudioButtonGroupWidget,
                                            private ButtonEventHandler::Callback
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
    }

private:
    void buttonClicked(SubWidget* const widget, int) final
    {
    }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioTopBarSnapshotsGroupWidget : public LibreAudioButtonGroupWidget,
                                             private ButtonEventHandler::Callback
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

    // snapshot A checked by default
    char fCurrentSnapshot = 'A';
    char fLastSnapshot = fCurrentSnapshot;

public:
    explicit LibreAudioTopBarSnapshotsGroupWidget(LibreAudioWidget* const parent)
        : LibreAudioButtonGroupWidget(parent)
    {
        fA->setWidth(fCopy->getWidth());
        fB->setWidth(fCopy->getWidth());
        fC->setWidth(fCopy->getWidth());
        fD->setWidth(fCopy->getWidth());
        done(this);

        fCopy->setId(kWidgetSnapshotCopy);
        fA->setId(kWidgetSnapshotA);
        fB->setId(kWidgetSnapshotB);
        fC->setId(kWidgetSnapshotC);
        fD->setId(kWidgetSnapshotD);

        fA->setChecked(fCurrentSnapshot == 'A', false);
        fB->setChecked(fCurrentSnapshot == 'B', false);
        fC->setChecked(fCurrentSnapshot == 'C', false);
        fD->setChecked(fCurrentSnapshot == 'D', false);
    }

private:
    void buttonClicked(SubWidget* const widget, int) final
    {
        if (widget->getId() == kWidgetSnapshotCopy)
        {
            // TODO some other stuff
            return;
        }

        if (static_cast<LibreAudioButtonWidget*>(widget)->isChecked())
        {
            fLastSnapshot = fCurrentSnapshot;
            fCurrentSnapshot = 'A' + (widget->getId() - kWidgetSnapshotA);
        }
        else
        {
            if (fCurrentSnapshot == fLastSnapshot)
                return;
            fCurrentSnapshot = fLastSnapshot;
        }

        fA->setChecked(fCurrentSnapshot == 'A', false);
        fB->setChecked(fCurrentSnapshot == 'B', false);
        fC->setChecked(fCurrentSnapshot == 'C', false);
        fD->setChecked(fCurrentSnapshot == 'D', false);
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
    }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioTopBarMenuPowerGroupWidget : public LibreAudioButtonGroupWidget,
                                             private ButtonEventHandler::Callback
{
    std::unique_ptr<LibreAudioButtonWidget> fMenu = addButton<LibreAudioImageButtonWidget<IMAGES_MENU_PNG_DATA, IMAGES_MENU_PNG_LEN>>();
    std::unique_ptr<LibreAudioButtonWidget> fPower = addButton<LibreAudioImageButtonWidget<IMAGES_POWER_PNG_DATA, IMAGES_POWER_PNG_LEN>>();

public:
    explicit LibreAudioTopBarMenuPowerGroupWidget(LibreAudioWidget* const parent)
        : LibreAudioButtonGroupWidget(parent)
    {
        done(this);

        fMenu->setId(kWidgetMenu);
        fPower->setId(kWidgetPower);
    }

private:
    void buttonClicked(SubWidget* const widget, int) final
    {
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
