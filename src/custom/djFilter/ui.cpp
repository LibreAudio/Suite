// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// FIXME remove this, helper for IDE
#include "config-custom.h"

#include "LibreAudioBaseUI.hpp"
#include "LibreAudioParameters.hpp"
#include "LibreAudioStates.hpp"

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

static constexpr const char kStringInitPreset[] = "Init Preset \\/";
static constexpr const char kStringShortName[] = DISTRHO_PLUGIN_SHORTNAME;

using LibreAudioLogo = LibreAudioImageWidget<IMAGES_LA_PNG_DATA, IMAGES_LA_PNG_LEN>;
using LibreAudioPluginName = LibreAudioTextWidget<kStringShortName, true>;
using LibreAudioPresetWidget = LibreAudioTextWidget<kStringInitPreset>;

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

class LibreAudioImageButton : public LibreAudioWidget,
                              public ButtonEventHandler
{
public:
    LibreAudioImageButton(NanoSubWidget* const parent, const uchar* const data, const uint dataSize)
        : LibreAudioWidget(parent),
          ButtonEventHandler(this),
          fImage(createImageFromMemory(data, dataSize, IMAGE_GENERATE_MIPMAPS)) {}

protected:
    virtual const Color& getColor() const
    {
        return gColors.get(this);
    }

    bool onMouse(const Widget::MouseEvent& ev) final
    {
        if (mouseEvent(ev))
            return true;
        return LibreAudioWidget::onMouse(ev);
    }

    bool onMotion(const Widget::MotionEvent& ev) final
    {
        if (motionEvent(ev))
            return true;
        return LibreAudioWidget::onMotion(ev);
    }

    void onNanoDisplay() final
    {
        const double size = Metrics::TopBar::smallImageSize * fScaleFactor;
        const uint width = getWidth();
        const uint height = getHeight();

        globalTint(getColor());

        beginPath();
        rect(0, 0, width, height);
        fillPaint(imagePattern((width - size) * 0.5, (height - size) * 0.5, size, size, 0.f, fImage, 1.f));
        fill();
    }

    void stateChanged(const State state, const State oldState) final
    {
        if ((state & kButtonStateHover) != 0)
        {
            if ((oldState & kButtonStateHover) == 0)
                getWindow().setCursor(kMouseCursorHand);
        }
        else
        {
            if ((oldState & kButtonStateHover) != 0)
                getWindow().setCursor(kMouseCursorArrow);
        }
    }

private:
    NanoImage fImage;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioImageCheckBox : public LibreAudioWidget,
                                public ButtonEventHandler
{
public:
    LibreAudioImageCheckBox(NanoSubWidget* const parent,
                            const uchar* const dataOn,
                            const uint dataOnSize,
                            const uchar* const dataOff,
                            const uint dataOffSize)
        : LibreAudioWidget(parent),
          ButtonEventHandler(this),
          fImageOn(createImageFromMemory(dataOn, dataOnSize, IMAGE_GENERATE_MIPMAPS)),
          fImageOff(createImageFromMemory(dataOff, dataOffSize, IMAGE_GENERATE_MIPMAPS))
    {
        setCheckable(true);
    }

protected:
    virtual const Color& getColor() const
    {
        return gColors.get(this);
    }

    bool onMouse(const Widget::MouseEvent& ev) final
    {
        if (mouseEvent(ev))
            return true;
        return LibreAudioWidget::onMouse(ev);
    }

    bool onMotion(const Widget::MotionEvent& ev) final
    {
        if (motionEvent(ev))
            return true;
        return LibreAudioWidget::onMotion(ev);
    }

    void onNanoDisplay() final
    {
        const double size = Metrics::TopBar::smallImageSize * fScaleFactor;
        const uint width = getWidth();
        const uint height = getHeight();

        globalTint(getColor());

        beginPath();
        rect(0, 0, width, height);
        fillPaint(imagePattern((width - size) * 0.5,
                               (height - size) * 0.5,
                               size,
                               size,
                               0.f,
                               isChecked() ? fImageOn : fImageOff,
                               1.f));
        fill();
    }

    void stateChanged(const State state, const State oldState) final
    {
        if ((state & kButtonStateHover) != 0)
        {
            if ((oldState & kButtonStateHover) == 0)
                getWindow().setCursor(kMouseCursorHand);
        }
        else
        {
            if ((oldState & kButtonStateHover) != 0)
                getWindow().setCursor(kMouseCursorArrow);
        }
    }

private:
    NanoImage fImageOn;
    NanoImage fImageOff;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioTextButton : public LibreAudioWidget,
                             public ButtonEventHandler
{
public:
    LibreAudioTextButton(NanoSubWidget* const parent, const char text[])
        : LibreAudioWidget(parent),
          ButtonEventHandler(this),
          fText(text) {}

protected:
    virtual const Color& getColor() const
    {
        return gColors.get(this);
    }

    bool onMouse(const Widget::MouseEvent& ev) final
    {
        if (mouseEvent(ev))
            return true;
        return LibreAudioWidget::onMouse(ev);
    }

    bool onMotion(const Widget::MotionEvent& ev) final
    {
        if (motionEvent(ev))
            return true;
        return LibreAudioWidget::onMotion(ev);
    }

    void onNanoDisplay() final
    {
        const uint width = getWidth();
        const uint height = getHeight();

        beginPath();
        rect(0, 0, width, height);
        fillColor(getColor());
        fontSize(Metrics::fontSize * fScaleFactor);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), fText);
    }

    void stateChanged(const State state, const State oldState) final
    {
        if ((state & kButtonStateHover) != 0)
        {
            if ((oldState & kButtonStateHover) == 0)
                getWindow().setCursor(kMouseCursorHand);
        }
        else
        {
            if ((oldState & kButtonStateHover) != 0)
                getWindow().setCursor(kMouseCursorArrow);
        }
    }

private:
    const char* const fText;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioUndoRedoWidget : public LibreAudioWidget
{
public:
    LibreAudioUndoRedoWidget(NanoSubWidget* const parent, ButtonEventHandler::Callback* const callback)
        : LibreAudioWidget(parent)
    {
        fUndoButton = new LibreAudioImageButton(this, IMAGES_UNDO_PNG_DATA, IMAGES_UNDO_PNG_LEN);
        fRedoButton = new LibreAudioImageButton(this, IMAGES_REDO_PNG_DATA, IMAGES_REDO_PNG_LEN);

        fUndoButton->setCallback(callback);
        fRedoButton->setCallback(callback);

        fUndoButton->setId(kWidgetUndo);
        fRedoButton->setId(kWidgetRedo);

        fLayout.widgets.push_back({ fUndoButton, Fixed });
        fLayout.widgets.push_back({ fRedoButton, Fixed });
    }

    void update(const bool canUndo, const bool canRedo)
    {
        fUndoButton->setEnabled(canUndo);
        fRedoButton->setEnabled(canRedo);
    }

protected:
    void onNanoDisplay() final
    {
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        LibreAudioWidget::onPositionChanged(ev);

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::UndoRedo::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::UndoRedo::margin * fScaleFactor);
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::smallImageSize * fScaleFactor);

        fLayout.setAbsolutePos(ev.pos.getX(),
                               ev.pos.getY() + (getHeight() - buttonSize) / 2,
                               padding,
                               margin);
    }

    void onResize(const ResizeEvent& ev) final
    {
        LibreAudioWidget::onResize(ev);

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::UndoRedo::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::UndoRedo::margin * fScaleFactor);
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::smallImageSize * fScaleFactor);

        fUndoButton->setSize(buttonSize, buttonSize);
        fRedoButton->setSize(buttonSize, buttonSize);

        fLayout.setWidth(ev.size.getWidth(), padding, margin);
        fLayout.setAbsolutePos(getAbsoluteX(),
                               getAbsoluteY() + (ev.size.getHeight() - buttonSize) / 2,
                               padding,
                               margin);
    }

private:
    HorizontalLayout fLayout;
    ScopedPointer<LibreAudioImageButton> fUndoButton;
    ScopedPointer<LibreAudioImageButton> fRedoButton;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioSnapshotsWidget : public LibreAudioWidget
{
public:
    LibreAudioSnapshotsWidget(NanoSubWidget* const parent, ButtonEventHandler::Callback* const callback)
        : LibreAudioWidget(parent)
    {
        fButtonCopy = new LibreAudioImageCheckBox(this,
                                                  IMAGES_X_PNG_DATA,
                                                  IMAGES_X_PNG_LEN,
                                                  IMAGES_COPY_PNG_DATA,
                                                  IMAGES_COPY_PNG_LEN);
        fButtonA = new LibreAudioTextButton(this, "A");
        fButtonB = new LibreAudioTextButton(this, "B");
        fButtonC = new LibreAudioTextButton(this, "C");
        fButtonD = new LibreAudioTextButton(this, "D");

        fButtonCopy->setCallback(callback);
        fButtonA->setCallback(callback);
        fButtonB->setCallback(callback);
        fButtonC->setCallback(callback);
        fButtonD->setCallback(callback);

        fButtonA->setCheckable(true);
        fButtonB->setCheckable(true);
        fButtonC->setCheckable(true);
        fButtonD->setCheckable(true);

        fButtonCopy->setId(kWidgetSnapshotCopy);
        fButtonA->setId(kWidgetSnapshotA);
        fButtonB->setId(kWidgetSnapshotB);
        fButtonC->setId(kWidgetSnapshotC);
        fButtonD->setId(kWidgetSnapshotD);

        fLayout.widgets.push_back({ fButtonCopy, Fixed });
        fLayout.widgets.push_back({ fButtonA, Fixed });
        fLayout.widgets.push_back({ fButtonB, Fixed });
        fLayout.widgets.push_back({ fButtonC, Fixed });
        fLayout.widgets.push_back({ fButtonD, Fixed });
    }

    void update(const bool isCopyingSnapshot, const uint8_t currentSnapshot)
    {
        fButtonCopy->setChecked(isCopyingSnapshot, false);
        fButtonA->setChecked(currentSnapshot == 0, false);
        fButtonB->setChecked(currentSnapshot == 1, false);
        fButtonC->setChecked(currentSnapshot == 2, false);
        fButtonD->setChecked(currentSnapshot == 3, false);
    }

protected:
    void onNanoDisplay() final
    {
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        LibreAudioWidget::onPositionChanged(ev);

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::margin * fScaleFactor);
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::smallImageSize * fScaleFactor);

        fLayout.setAbsolutePos(ev.pos.getX(),
                               ev.pos.getY() + (getHeight() - buttonSize) / 2,
                               padding,
                               margin);
    }

    void onResize(const ResizeEvent& ev) final
    {
        LibreAudioWidget::onResize(ev);

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::margin * fScaleFactor);
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::smallImageSize * fScaleFactor);

        fButtonCopy->setSize(buttonSize, buttonSize);
        fButtonA->setSize(buttonSize, buttonSize);
        fButtonB->setSize(buttonSize, buttonSize);
        fButtonC->setSize(buttonSize, buttonSize);
        fButtonD->setSize(buttonSize, buttonSize);

        fLayout.setWidth(ev.size.getWidth(), padding, margin);
        fLayout.setAbsolutePos(getAbsoluteX(),
                               getAbsoluteY() + (ev.size.getHeight() - buttonSize) / 2,
                               padding,
                               margin);
    }

private:
    HorizontalLayout fLayout;
    ScopedPointer<LibreAudioImageCheckBox> fButtonCopy;
    ScopedPointer<LibreAudioTextButton> fButtonA;
    ScopedPointer<LibreAudioTextButton> fButtonB;
    ScopedPointer<LibreAudioTextButton> fButtonC;
    ScopedPointer<LibreAudioTextButton> fButtonD;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioEasyExpertWidget : public LibreAudioWidget
{
public:
    LibreAudioEasyExpertWidget(NanoSubWidget* const parent, ButtonEventHandler::Callback* const callback)
        : LibreAudioWidget(parent)
    {
        fButtonEasy = new LibreAudioTextButton(this, "EASY");
        fButtonExpert = new LibreAudioTextButton(this, "EXPERT");

        fButtonEasy->setCallback(callback);
        fButtonExpert->setCallback(callback);

        fButtonEasy->setCheckable(true);
        fButtonExpert->setCheckable(true);

        fButtonEasy->setId(kWidgetEasy);
        fButtonExpert->setId(kWidgetExpert);

        fLayout.widgets.push_back({ fButtonEasy, Fixed });
        fLayout.widgets.push_back({ fButtonExpert, Fixed });
    }

    void update(const bool expertMode)
    {
        fButtonEasy->setChecked(!expertMode, false);
        fButtonExpert->setChecked(expertMode, false);
    }

protected:
    void onNanoDisplay() final
    {
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        LibreAudioWidget::onPositionChanged(ev);

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::EasyExpert::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::EasyExpert::margin * fScaleFactor);
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::smallImageSize * fScaleFactor);

        fLayout.setAbsolutePos(ev.pos.getX(),
                               ev.pos.getY() + (getHeight() - buttonSize) / 2,
                               padding,
                               margin);
    }

    void onResize(const ResizeEvent& ev) final
    {
        LibreAudioWidget::onResize(ev);

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::EasyExpert::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::EasyExpert::margin * fScaleFactor);
        const uint buttonWidth = d_roundToUnsignedInt(
            (Metrics::TopBar::Cluster::EasyExpert::width / 2 - Metrics::TopBar::Cluster::EasyExpert::margin - Metrics::TopBar::Cluster::EasyExpert::padding / 2)
            * fScaleFactor);
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::smallImageSize * fScaleFactor);

        fButtonEasy->setSize(buttonWidth, buttonSize);
        fButtonExpert->setSize(buttonWidth, buttonSize);

        fLayout.setWidth(ev.size.getWidth(), padding, margin);
        fLayout.setAbsolutePos(getAbsoluteX(),
                               getAbsoluteY() + (ev.size.getHeight() - buttonSize) / 2,
                               padding,
                               margin);
    }

private:
    HorizontalLayout fLayout;
    ScopedPointer<LibreAudioTextButton> fButtonEasy;
    ScopedPointer<LibreAudioTextButton> fButtonExpert;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioPowerButton : public LibreAudioImageButton
{
public:
    LibreAudioPowerButton(NanoSubWidget* const parent)
        : LibreAudioImageButton(parent, IMAGES_POWER_PNG_DATA, IMAGES_POWER_PNG_LEN)
    {
    }

protected:
    const Color& getColor() const override
    {
        if (! isEnabled())
            return gColors.ink3;
        if (isChecked())
            return fOffColor;
        // if (isHovered())
        //     return gColors.acc;
        return gColors.ink2;
    }

private:
    const Color fOffColor = Color::fromHTML("#ff5d5d");
};

class LibreAudioMenuWidget : public LibreAudioWidget
{
public:
    LibreAudioMenuWidget(NanoSubWidget* const parent, ButtonEventHandler::Callback* const callback)
        : LibreAudioWidget(parent)
    {
        fButtonMenu = new LibreAudioImageButton(this, IMAGES_MENU_PNG_DATA, IMAGES_MENU_PNG_LEN);
        fButtonPower = new LibreAudioPowerButton(this);

        fButtonMenu->setCallback(callback);
        fButtonPower->setCallback(callback);

        fButtonPower->setCheckable(true);

        fButtonMenu->setId(kWidgetMenu);
        fButtonPower->setId(kWidgetPower);

        fLayout.widgets.push_back({ fButtonMenu, Fixed });
        fLayout.widgets.push_back({ fButtonPower, Fixed });
    }

    void update(const bool enabled)
    {
        fButtonPower->setChecked(enabled, false);
    }

protected:
    void onNanoDisplay() final
    {
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        LibreAudioWidget::onPositionChanged(ev);

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::margin * fScaleFactor);
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::smallImageSize * fScaleFactor);

        fLayout.setAbsolutePos(ev.pos.getX(),
                               ev.pos.getY() + (getHeight() - buttonSize) / 2,
                               padding,
                               margin);
    }

    void onResize(const ResizeEvent& ev) final
    {
        LibreAudioWidget::onResize(ev);

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::margin * fScaleFactor);
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::smallImageSize * fScaleFactor);

        fButtonMenu->setSize(buttonSize, buttonSize);
        fButtonPower->setSize(buttonSize, buttonSize);

        fLayout.setWidth(ev.size.getWidth(), padding, margin);
        fLayout.setAbsolutePos(getAbsoluteX(),
                               getAbsoluteY() + (ev.size.getHeight() - buttonSize) / 2,
                               padding,
                               margin);
    }

private:
    HorizontalLayout fLayout;
    ScopedPointer<LibreAudioImageButton> fButtonMenu;
    ScopedPointer<LibreAudioPowerButton> fButtonPower;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioTopBarCluster : public LibreAudioWidget
{
public:
    LibreAudioTopBarCluster(NanoSubWidget* const parent, ButtonEventHandler::Callback* const callback)
        : LibreAudioWidget(parent)
    {
        fPreset = new LibreAudioPresetWidget(this);
        fUndoRedo = new LibreAudioUndoRedoWidget(this, callback);
        fSnapshots = new LibreAudioSnapshotsWidget(this, callback);
        fEasyExpert = new LibreAudioEasyExpertWidget(this, callback);
        fMenu = new LibreAudioMenuWidget(this, callback);

        fLayout.widgets.push_back({ fPreset, Expanding });
        fLayout.widgets.push_back({ fUndoRedo, Fixed });
        fLayout.widgets.push_back({ fSnapshots, Fixed });
        fLayout.widgets.push_back({ fEasyExpert, Fixed });
        fLayout.widgets.push_back({ fMenu, Fixed });
    }

    void update(const bool canUndo,
                const bool canRedo,
                const bool isCopyingSnapshot,
                const uint8_t currentSnapshot,
                const bool expertMode,
                const bool enabled)
    {
        fUndoRedo->update(canUndo, canRedo);
        fSnapshots->update(isCopyingSnapshot, currentSnapshot);
        fEasyExpert->update(expertMode);
        fMenu->update(enabled);
    }

protected:
    void onNanoDisplay() final
    {
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        LibreAudioWidget::onPositionChanged(ev);

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::margin * fScaleFactor);
        fLayout.setAbsolutePos(ev.pos.getX(), ev.pos.getY(), padding, margin);
    }

    void onResize(const ResizeEvent& ev) final
    {
        LibreAudioWidget::onResize(ev);

        const uint height = ev.size.getHeight();

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::margin * fScaleFactor);

        fPreset->setHeight(height);
        fUndoRedo->setSize(d_roundToUnsignedInt(Metrics::TopBar::Cluster::UndoRedo::width * fScaleFactor), height);
        fSnapshots->setSize(d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::width * fScaleFactor), height);
        fEasyExpert->setSize(d_roundToUnsignedInt(Metrics::TopBar::Cluster::EasyExpert::width * fScaleFactor), height);
        fMenu->setSize(d_roundToUnsignedInt(Metrics::TopBar::Cluster::Menu::width * fScaleFactor), height);

        fLayout.setWidth(ev.size.getWidth(), padding, margin);
        fLayout.setAbsolutePos(getAbsoluteX(), getAbsoluteY(), padding, margin);
    }

private:
    HorizontalLayout fLayout;
    ScopedPointer<LibreAudioPresetWidget> fPreset;
    ScopedPointer<LibreAudioUndoRedoWidget> fUndoRedo;
    ScopedPointer<LibreAudioSnapshotsWidget> fSnapshots;
    ScopedPointer<LibreAudioEasyExpertWidget> fEasyExpert;
    ScopedPointer<LibreAudioMenuWidget> fMenu;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioTopBar : public LibreAudioWidget
{
public:
    LibreAudioTopBar(NanoTopLevelWidget* const parent, ButtonEventHandler::Callback* const callback)
        : LibreAudioWidget(parent)
    {
        fLogo = new LibreAudioLogo(this);
        fPluginName = new LibreAudioPluginName(this);
        fCluster = new LibreAudioTopBarCluster(this, callback);

        fLayout.widgets.push_back({ fLogo, Fixed });
        fLayout.widgets.push_back({ fPluginName, Expanding });
        fLayout.widgets.push_back({ fCluster, Fixed });
    }

    void update(const bool canUndo,
                const bool canRedo,
                const bool isCopyingSnapshot,
                const uint8_t currentSnapshot,
                const bool expertMode,
                const bool enabled)
    {
        fCluster->update(canUndo, canRedo, isCopyingSnapshot, currentSnapshot, expertMode, enabled);
    }

protected:
    void onNanoDisplay() final
    {
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        LibreAudioWidget::onPositionChanged(ev);

        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::margin * fScaleFactor);
        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::padding * fScaleFactor);
        fLayout.setAbsolutePos(ev.pos.getX(), ev.pos.getY(), padding, margin);
    }

    void onResize(const ResizeEvent& ev) final
    {
        LibreAudioWidget::onResize(ev);

        const uint height = ev.size.getHeight();

        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::margin * fScaleFactor);
        const uint marginLeft = d_roundToUnsignedInt(Metrics::TopBar::marginLeft * fScaleFactor);
        const uint marginRight = d_roundToUnsignedInt(Metrics::TopBar::marginRight * fScaleFactor);
        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::padding * fScaleFactor);

        fLogo->setSize(d_roundToUnsignedInt(Metrics::TopBar::Logo::imageSize * fScaleFactor), height);
        fPluginName->setHeight(height);
        fCluster->setSize(d_roundToUnsignedInt(Metrics::TopBar::Cluster::width * fScaleFactor), height);

        fLayout.setWidth(ev.size.getWidth() + margin * 2 - marginLeft - marginRight, padding, margin);

        fLayout.setAbsolutePos(getAbsoluteX() + marginLeft - margin, getAbsoluteY(), padding, margin);
    }

private:
    HorizontalLayout fLayout;
    ScopedPointer<LibreAudioLogo> fLogo;
    ScopedPointer<LibreAudioPluginName> fPluginName;
    ScopedPointer<LibreAudioTopBarCluster> fCluster;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioMainArea : public LibreAudioWidget,
                           private IdleCallback
{
public:
    LibreAudioMainArea(NanoTopLevelWidget* const parent, LibreAudioBaseUI* const ui)
        : LibreAudioWidget(parent)
    {
        colors[0] = Color::fromHTML("#ffbfcb");
        colors[1] = Color::fromHTML("#ffdfad");
        colors[2] = Color::fromHTML("#d2fdd3");
        colors[3] = Color::fromHTML("#bef1ff");
        colors[4] = Color::fromHTML("#c3d9ff");
        colors[5] = Color::fromHTML("#dac1f3");
        colors[6] = Color::fromHTML("#ffdcf5");

        getWindow().addIdleCallback(this);
    }

protected:
    struct Pt { float x; float y; };
    static constexpr int kStopWidth = 50;
    static constexpr int kNumStops = 6;
    std::array<Pt, kStopWidth * kNumStops + 1> pts;
    std::array<Color, kNumStops + 1> colors;

    static constexpr int WIN = 2;
    static constexpr int dbMax = 18;
    static constexpr int dbMin = -42;
    static constexpr int rw = 868;
    static constexpr int rh = 200;

    void tracePts(float hz, float amp, float ph)
    {
        for (int i = 0; i < std::size(pts); i++) {
            float t = (float)i / (std::size(pts) - 1);
            float y = /*baseY(t)*/ getHeight() * 0.5f - (amp * std::sin(2.f * M_PI * (t * hz * WIN - ph))) * fScaleFactor;
            // y = std::max(-40.f, std::min(h + 40, y));
            pts[i] = { t * getWidth(), y };
        }
    }

    float h1 = 0.f;
    float ph1 = 0.f;
    float last = 0.f;

    void idleCallback() final
    {
        int t = d_gettime_ms();
        float phase = t * 0.0001f;
        float dt = phase - last;
        if (!(dt >= 0) || dt > 0.1) dt = 0.016;
        last = phase;

        const float k = std::min(1.f, dt * 7.f);

        h1 += (3.0f - h1) * k;
        ph1 += h1 * dt;
        repaint();
    }

    void onNanoDisplay() final
    {
        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(0.f, 0.f, 0.f);
        fill();

        tracePts(h1, 100, ph1);

        strokeWidth(3.f * fScaleFactor);

        beginPath();
        moveTo(pts[0].x, pts[0].y);

        for (int i = 1, step = 0; i < kStopWidth * kNumStops; ++i)
        {
            if ((i % kStopWidth) == 0)
            {
                lineTo(pts[i].x, pts[i].y);
                strokePaint(linearGradient(pts[step * kStopWidth].x,
                                            pts[step * kStopWidth].y,
                                            pts[(step + 1) * kStopWidth].x,
                                            pts[(step + 1) * kStopWidth].y,
                                            colors[step],
                                            colors[step + 1]));
                stroke();
                ++step;

                beginPath();
                moveTo(pts[i].x, pts[i].y);
            }
            else
            {
                lineTo(pts[i].x, pts[i].y);
            }
        }

        lineTo(pts[kStopWidth * kNumStops].x, pts[kStopWidth * kNumStops].y);
        strokePaint(linearGradient(pts[kStopWidth * (kNumStops - 1)].x,
                                   pts[kStopWidth * (kNumStops - 1)].y,
                                   pts[kStopWidth * kNumStops].x,
                                   pts[kStopWidth * kNumStops].y,
                                   colors[(kNumStops - 1)],
                                   colors[kNumStops]));
        stroke();
    }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioBottomBar : public LibreAudioWidget
{
public:
    LibreAudioBottomBar(NanoTopLevelWidget* const parent)
        : LibreAudioWidget(parent)
    {
    }

protected:
    void onNanoDisplay() final
    {
        fontSize(26.f * fScaleFactor);

        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(0.f, 0.f, 0.1f);
        fill();
        fillColor(1.f, 1.f, 1.f);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), "This is the bottom-bar");
    }

    void onResize(const ResizeEvent& ev) final
    {
        LibreAudioWidget::onResize(ev);
    }
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

        fTopBar = new LibreAudioTopBar(this, this);
        fMainArea = new LibreAudioMainArea(this, this);
        fBottomBar = new LibreAudioBottomBar(this);

        fLayout.widgets.push_back({ fTopBar, Fixed });
        fLayout.widgets.push_back({ fMainArea, Expanding });
        fLayout.widgets.push_back({ fBottomBar, Fixed });

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
        case kWidgetUndo:
            undo();
            break;
        case kWidgetRedo:
            redo();
            break;
        case kWidgetSnapshotCopy:
            // if (static_cast<LibreAudioSnapshotsWidget::ImageButton*>(widget)->isChecked())
            snapshotButtonClicked(kSnapshotButtonCopy);
            break;
        case kWidgetSnapshotA:
            if (static_cast<LibreAudioTextButton*>(widget)->isChecked())
                snapshotButtonClicked(kSnapshotButtonA);
            break;
        case kWidgetSnapshotB:
            if (static_cast<LibreAudioTextButton*>(widget)->isChecked())
                snapshotButtonClicked(kSnapshotButtonB);
            break;
        case kWidgetSnapshotC:
            if (static_cast<LibreAudioTextButton*>(widget)->isChecked())
                snapshotButtonClicked(kSnapshotButtonC);
            break;
        case kWidgetSnapshotD:
            if (static_cast<LibreAudioTextButton*>(widget)->isChecked())
                snapshotButtonClicked(kSnapshotButtonD);
            break;
        case kWidgetEasy:
            fExpertMode = false;
            break;
        case kWidgetExpert:
            fExpertMode = true;
            break;
        case kWidgetMenu:
            break;
        case kWidgetPower:
            parameterControlPressed(kCommonParameterBypass);
            parameterControlModified(kCommonParameterBypass,
                                     static_cast<LibreAudioPowerButton*>(widget)->isChecked() ? 1.f : 0.f);
            parameterControlReleased(kCommonParameterBypass);
            break;
        }
    }

    void onNanoDisplay() final
    {
        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(gColors.background);
        fill();
        strokeColor(gColors.outline);
        strokeWidth(fScaleFactor * 2); // x2 for making it centered
        stroke();
    }

    void onResize(const ResizeEvent& ev) final
    {
        UI::onResize(ev);
        adjustSize();
    }

    void uiIdle() final
    {
        LibreAudioBaseUI::uiIdle();

        fTopBar->update(canUndo(),
                        canRedo(),
                        isCopyingSnapshot(),
                        getCurrentSnapshot(),
                        fExpertMode,
                        d_isNotZero(fParameterValuesRef[kCommonParameterBypass]));
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
    ScopedPointer<LibreAudioBottomBar> fBottomBar;

    void adjustSize()
    {
        const uint width = getWidth();
        const uint height = getHeight();
        const uint padding = d_roundToUnsignedInt(Metrics::Window::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::Window::margin * fScaleFactor);

        fTopBar->setSize(width, d_roundToUnsignedInt(Metrics::TopBar::height * fScaleFactor));
        fMainArea->setWidth(width);
        fBottomBar->setSize(width, d_roundToUnsignedInt(Metrics::BottomBar::height * fScaleFactor));

        fLayout.setHeight(height, padding, margin);

        fLayout.setAbsolutePos(0, 0, padding, margin);
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
