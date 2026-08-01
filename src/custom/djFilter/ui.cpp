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

#include "las-resources.h"

#include <string>
#include <vector>

#include "ui/colors.hpp"
#include "ui/metrics.hpp"
#include "ui/widgets.hpp"

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

using LibreAudioLogo = LibreAudioImageWidget<IMAGES_LA_PNG_DATA, IMAGES_LA_PNG_LEN>;

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

class LibreAudioTopBar : public LibreAudioWidget
{
public:
    LibreAudioTopBar(NanoTopLevelWidget* const parent)
        : LibreAudioWidget(parent)
    {
        setHeight(d_roundToUnsignedInt(Metrics::TopBar::height * fScaleFactor));

        fLogo = new LibreAudioLogo(this);
        fPluginName = new LibreAudioPluginName(this);
        fSpacer = new LibreAudioWidget(this);

        fLayout.widgets.push_back({ fLogo, Fixed });
        fLayout.widgets.push_back({ fPluginName, Fixed });
        fLayout.widgets.push_back({ fSpacer, Expanding });
    }

protected:
    void onNanoDisplay() final
    {
        fillColor(1.f, 1.f, 1.f);
        fontSize(26.f * fScaleFactor);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), "This is the top bar");

        if constexpr (Metrics::TopBar::border != 0)
        {
            strokeColor(Colors::border);
            strokeWidth(Metrics::TopBar::border * 2 * fScaleFactor);
            stroke();
        }
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        LibreAudioWidget::onPositionChanged(ev);

        const uint border = d_roundToUnsignedInt(Metrics::TopBar::border * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::margin * fScaleFactor);
        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::padding * fScaleFactor);
        fLayout.setAbsolutePos(ev.pos.getX(), ev.pos.getY(), padding, border + margin);
    }

    void onResize(const ResizeEvent& ev) final
    {
        LibreAudioWidget::onResize(ev);

        const uint border = d_roundToUnsignedInt(Metrics::TopBar::border * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::margin * fScaleFactor);
        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::padding * fScaleFactor);
        fLayout.align(getAbsoluteX(), getAbsoluteY(), getWidth(), getHeight(), padding, border + margin);
    }

private:
    HorizontalLayout fLayout;
    ScopedPointer<LibreAudioLogo> fLogo;
    ScopedPointer<LibreAudioPluginName> fPluginName;
    ScopedPointer<LibreAudioWidget> fSpacer;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioMeters : public LibreAudioWidget
{
public:
    LibreAudioMeters(NanoSubWidget* const parent)
        : LibreAudioWidget(parent)
    {
        setWidth(26 * fScaleFactor);
    }

protected:
    void onNanoDisplay() final
    {
        beginPath();
        roundedRect(0, 0, getWidth(), getHeight(), 7 * fScaleFactor);
        fillColor(Colors::accGlow);
        fill();
    }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioMainArea : public LibreAudioWidget
{
public:
    LibreAudioMainArea(NanoTopLevelWidget* const parent)
        : LibreAudioWidget(parent)
    {
        fMetersIn = new LibreAudioMeters(this);
        fSpacer = new LibreAudioWidget(this);
        fMetersOut = new LibreAudioMeters(this);

        fLayout.widgets.push_back({ fMetersIn, Fixed });
        fLayout.widgets.push_back({ fSpacer, Expanding });
        fLayout.widgets.push_back({ fMetersOut, Fixed });
    }

protected:
    void onNanoDisplay() final
    {
        fontSize(26.f * fScaleFactor);
        fillColor(1.f, 1.f, 1.f);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), "This is the main area");

        if constexpr (Metrics::MainArea::border != 0)
        {
            strokeColor(Colors::border);
            strokeWidth(Metrics::MainArea::border * 2 * fScaleFactor);
            stroke();
        }
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        LibreAudioWidget::onPositionChanged(ev);

        const uint border = d_roundToUnsignedInt(Metrics::MainArea::border * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::MainArea::margin * fScaleFactor);
        const uint padding = d_roundToUnsignedInt(Metrics::MainArea::padding * fScaleFactor);
        fLayout.setAbsolutePos(ev.pos.getX(), ev.pos.getY(), padding, border + margin);
    }

    void onResize(const ResizeEvent& ev) final
    {
        LibreAudioWidget::onResize(ev);

        const uint border = d_roundToUnsignedInt(Metrics::MainArea::border * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::MainArea::margin * fScaleFactor);
        const uint padding = d_roundToUnsignedInt(Metrics::MainArea::padding * fScaleFactor);
        fLayout.align(getAbsoluteX(), getAbsoluteY(), getWidth(), getHeight(), padding, border + margin);
    }

private:
    HorizontalLayout fLayout;
    ScopedPointer<LibreAudioMeters> fMetersIn;
    ScopedPointer<LibreAudioWidget> fSpacer;
    ScopedPointer<LibreAudioMeters> fMetersOut;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioUI : public LibreAudioBaseUI,
                     private ButtonEventHandler::Callback
{
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

        fTopBar = new LibreAudioTopBar(this);
        fMainArea = new LibreAudioMainArea(this);

        fLayout.widgets.push_back({ fTopBar, Fixed });
        fLayout.widgets.push_back({ fMainArea, Expanding });

        adjustSize();
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

    void onNanoDisplay() final
    {
        const float w = getWidth();
        const float h = getHeight();

        beginPath();
        rect(0, 0, w, h);
        fillPaint(linearGradient(0, 0, 0, h, Colors::backgroundGradientStart, Colors::backgroundGradientStop));
        fill();

        if constexpr (Metrics::Window::border != 0)
        {
            strokeColor(Colors::border);
            strokeWidth(Metrics::Window::border * 2 * fScaleFactor);
            stroke();
        }
    }

    void onResize(const ResizeEvent& ev) final
    {
        UI::onResize(ev);
        adjustSize();
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
        adjustSize();
    }

private:
    double fScaleFactor = getScaleFactor();
    bool fExpertMode = false;

    VerticalLayout fLayout;
    ScopedPointer<LibreAudioTopBar> fTopBar;
    ScopedPointer<LibreAudioMainArea> fMainArea;

    void adjustSize()
    {
        const uint border = d_roundToUnsignedInt(Metrics::Window::border * fScaleFactor);
        const uint padding = d_roundToUnsignedInt(Metrics::Window::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::Window::margin * fScaleFactor);
        fLayout.align(0, 0, getWidth(), getHeight(), padding, border + margin);
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
