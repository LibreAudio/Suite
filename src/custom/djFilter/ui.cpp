// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// FIXME remove this, helper for IDE
#include "config-custom.h"

#include "LibreAudioBaseUI.hpp"
#include "LibreAudioParameters.hpp"
#include "LibreAudioStates.hpp"

#include "Layout.hpp"

// temp stuff
#include "DearImGui.hpp"

#include "las-resources.h"

#include <string>
#include <vector>

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

static const struct Colors {
    Color background = Color::fromHTML("#28282d");
    Color outline = Color::fromHTML("#3a3a42");

    Color ink = Color::fromHTML("#f2f2f4");
    Color ink2 = Color::fromHTML("#b7b9bf");
    Color ink3 = Color::fromHTML("#74767e");
    Color acc = Color::fromHTML("#c5d9ff");

    Color bg0 = Color::fromHTML("#161618");
    Color bg1 = Color::fromHTML("#1c1c20");
    Color bg2 = Color::fromHTML("#26262b");
    Color bg3 = Color::fromHTML("#303036");
    Color line = Color::fromHTML("#2a2a30");
    Color line2 = Color::fromHTML("#3b3b44");

    const Color& get(const bool isEnabled, const bool isChecked, const bool isHover) const
    {
        if (! isEnabled)
            return ink3;
        if (isChecked)
            return ink;
        if (isHover)
            return acc;
        return ink2;
    }

    const Color& get(const ButtonEventHandler* const button) const
    {
        if (! button->isEnabled())
            return ink3;
        if (button->isChecked())
            return ink;
        if (button->isHovered())
            return acc;
        return ink2;
    }
} gColors;

enum WidgetIds {
    kWidgetIdStart = 1000,
    kWidgetUndo,
    kWidgetRedo,
    kWidgetSnapshotCopy,
    kWidgetSnapshotA,
    kWidgetSnapshotB,
    kWidgetSnapshotC,
    kWidgetSnapshotD,
};

struct Metrics {
    static constexpr const uint fontSize = 20;

    struct Window {
        static constexpr const uint padding = 0;
        static constexpr const uint margin = 0;
    };

    struct TopBar {
        static constexpr const uint height = 46;
        static constexpr const uint padding = 10;
        static constexpr const uint margin = 0;
        static constexpr const uint marginLeft = margin + 7;
        static constexpr const uint marginRight = margin + 14;
        struct Logo {
            static constexpr const uint imageSize = 34;
        };
        struct PluginName {
        //     static constexpr const uint size = 34;
        };
        struct Cluster {
            static constexpr const uint width = 610;
            static constexpr const uint padding = 60;
            static constexpr const uint margin = 0;
            struct UndoRedo {
                static constexpr const uint width = 40;
                static constexpr const uint padding = 12;
                static constexpr const uint margin = 0;
                static constexpr const uint imageSize = 14;
            };
            struct Snapshots {
                static constexpr const uint width = 104;
                static constexpr const uint padding = 10;
                static constexpr const uint margin = 0;
                static constexpr const uint imageSize = 14;
            };
            struct EasyExpert {
                static constexpr const uint width = 92;
                static constexpr const uint padding = 10;
            };
            struct Menu {
                static constexpr const uint width = 45;
                static constexpr const uint padding = 9;
            };
        };
    };

    struct BottomBar {
        static constexpr const uint height = 46;
    };
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioWidget : public NanoSubWidget
{
public:
    LibreAudioWidget(NanoTopLevelWidget* const parent)
        : NanoSubWidget(parent),
          fScaleFactor(parent->getScaleFactor())
    {
    }

    LibreAudioWidget(NanoSubWidget* const parent)
        : NanoSubWidget(parent),
          fScaleFactor(parent->getTopLevelWidget()->getScaleFactor())
    {
    }

protected:
    double fScaleFactor;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioLogo : public LibreAudioWidget
{
public:
    LibreAudioLogo(NanoSubWidget* const parent)
        : LibreAudioWidget(parent) {}

protected:
    void onNanoDisplay() final
    {
        const double size = Metrics::TopBar::Logo::imageSize * fScaleFactor;
        const uint width = getWidth();
        const uint height = getHeight();

        beginPath();
        rect(0, 0, width, height);
        fillPaint(imagePattern((width - size) * 0.5, (height - size) * 0.5, size, size, 0.f, fImage, 1.f));
        fill();
    }

private:
    NanoImage fImage { createImageFromMemory(IMAGES_LA_PNG_DATA, IMAGES_LA_PNG_LEN, IMAGE_GENERATE_MIPMAPS) };
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioPluginName : public LibreAudioWidget
{
public:
    LibreAudioPluginName(NanoSubWidget* const parent)
        : LibreAudioWidget(parent)
    {
        if (fName.startsWith("LA "))
        {
            if (char* const name = fName.getAndReleaseBuffer())
            {
                fName = name + 3;
                std::free(name);
            }
        }

        fName.toUpper();
    }

protected:
    void onNanoDisplay() final
    {
        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(gColors.ink2);
        fontSize(Metrics::fontSize * fScaleFactor);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), fName);
    }

private:
    String fName { DISTRHO_PLUGIN_NAME };
};

class LibreAudioPresetWidget : public LibreAudioWidget
{
public:
    LibreAudioPresetWidget(NanoSubWidget* const parent)
        : LibreAudioWidget(parent)
    {
    }

protected:
    void onNanoDisplay() final
    {
        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(gColors.ink2);
        fontSize(Metrics::fontSize * fScaleFactor);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), "Init Preset \\/");
    }
};

// --------------------------------------------------------------------------------------------------------------------

template<uint imageSize>
class LibreAudioImageButton : public LibreAudioWidget,
                              public ButtonEventHandler
{
public:
    LibreAudioImageButton(NanoSubWidget* const parent, const uchar* const data, const uint dataSize)
        : LibreAudioWidget(parent),
          ButtonEventHandler(this),
          fImage(createImageFromMemory(data, dataSize, IMAGE_GENERATE_MIPMAPS)) {}

protected:
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
        const double size = imageSize * fScaleFactor;
        const uint width = getWidth();
        const uint height = getHeight();

        globalTint(gColors.get(this));

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

class LibreAudioTextButton : public LibreAudioWidget,
                             public ButtonEventHandler
{
public:
    LibreAudioTextButton(NanoSubWidget* const parent, const char text[])
        : LibreAudioWidget(parent),
          ButtonEventHandler(this),
          fText(text) {}

protected:
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
        const double size = Metrics::TopBar::Cluster::UndoRedo::imageSize * fScaleFactor;
        const uint width = getWidth();
        const uint height = getHeight();

        beginPath();
        rect(0, 0, width, height);
        fillColor(gColors.get(this));
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
    using ImageButton = LibreAudioImageButton<Metrics::TopBar::Cluster::UndoRedo::imageSize>;

    LibreAudioUndoRedoWidget(NanoSubWidget* const parent, ButtonEventHandler::Callback* const callback)
        : LibreAudioWidget(parent)
    {
        fUndoButton = new ImageButton(this, IMAGES_UNDO_PNG_DATA, IMAGES_UNDO_PNG_LEN);
        fRedoButton = new ImageButton(this, IMAGES_REDO_PNG_DATA, IMAGES_REDO_PNG_LEN);

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
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::Cluster::UndoRedo::imageSize * fScaleFactor);

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
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::Cluster::UndoRedo::imageSize * fScaleFactor);

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
    ScopedPointer<ImageButton> fUndoButton;
    ScopedPointer<ImageButton> fRedoButton;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioSnapshotsWidget : public LibreAudioWidget
{
public:
    using ImageButton = LibreAudioImageButton<Metrics::TopBar::Cluster::UndoRedo::imageSize>;

    LibreAudioSnapshotsWidget(NanoSubWidget* const parent, ButtonEventHandler::Callback* const callback)
        : LibreAudioWidget(parent)
    {
        fButtonCopy = new ImageButton(this, IMAGES_COPY_PNG_DATA, IMAGES_COPY_PNG_LEN);
        fButtonA = new LibreAudioTextButton(this, "A");
        fButtonB = new LibreAudioTextButton(this, "B");
        fButtonC = new LibreAudioTextButton(this, "C");
        fButtonD = new LibreAudioTextButton(this, "D");

        fButtonCopy->setCallback(callback);
        fButtonA->setCallback(callback);
        fButtonB->setCallback(callback);
        fButtonC->setCallback(callback);
        fButtonD->setCallback(callback);

        fButtonCopy->setCheckable(true);
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
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::imageSize * fScaleFactor);

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
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::imageSize * fScaleFactor);

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
    ScopedPointer<ImageButton> fButtonCopy;
    ScopedPointer<LibreAudioTextButton> fButtonA;
    ScopedPointer<LibreAudioTextButton> fButtonB;
    ScopedPointer<LibreAudioTextButton> fButtonC;
    ScopedPointer<LibreAudioTextButton> fButtonD;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioEasyExpertWidget : public LibreAudioWidget
{
public:
    LibreAudioEasyExpertWidget(NanoSubWidget* const parent)
        : LibreAudioWidget(parent)
    {
    }

protected:
    void onNanoDisplay() final
    {
        fontSize(Metrics::fontSize * fScaleFactor);

        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(0.1f, 0.2f, 0.2f);
        fill();
        fillColor(1.f, 1.f, 1.f);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), "Easy / Expert");
    }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioMenuWidget : public LibreAudioWidget
{
public:
    LibreAudioMenuWidget(NanoSubWidget* const parent)
        : LibreAudioWidget(parent)
    {
    }

protected:
    void onNanoDisplay() final
    {
        fontSize(Metrics::fontSize * fScaleFactor);

        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(0.2f, 0.2f, 0.2f);
        fill();
        fillColor(1.f, 1.f, 1.f);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), "= (I)");
    }
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
        fEasyExpert = new LibreAudioEasyExpertWidget(this);
        fMenu = new LibreAudioMenuWidget(this);

        fLayout.widgets.push_back({ fPreset, Expanding });
        fLayout.widgets.push_back({ fUndoRedo, Fixed });
        fLayout.widgets.push_back({ fSnapshots, Fixed });
        fLayout.widgets.push_back({ fEasyExpert, Fixed });
        fLayout.widgets.push_back({ fMenu, Fixed });
    }

    void update(const bool canUndo, const bool canRedo, const bool isCopyingSnapshot, const uint8_t currentSnapshot)
    {
        fUndoRedo->update(canUndo, canRedo);
        fSnapshots->update(isCopyingSnapshot, currentSnapshot);
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

    void update(const bool canUndo, const bool canRedo, const bool isCopyingSnapshot, const uint8_t currentSnapshot)
    {
        fCluster->update(canUndo, canRedo, isCopyingSnapshot, currentSnapshot);
    }

protected:
    void onNanoDisplay() final
    {
        // fontSize(Metrics::fontSize * fScaleFactor);

        // beginPath();
        // rect(0, 0, getWidth(), getHeight());
        // fillColor(0.0f, 0.1f, 0.1f);
        // fill();
        // fillColor(1.f, 1.f, 1.f);
        // textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        // textBox(0.f, getHeight() * 0.5f, getWidth(), "This is the top bar");
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

class LibreAudioQuickParamTest : public ImGuiSubWidget
{
public:
    explicit LibreAudioQuickParamTest(Widget* const parent, LibreAudioBaseUI* const ui)
        : ImGuiSubWidget(parent),
          fUI(ui)
    {
        // caching strings for display
        fParameterLabels.resize(fUI->kParameterCount);
        fParameterRenders.resize(fUI->kParameterCount);

        for (uint32_t i = 0; i < common_input::kFaustParameterCount; ++i)
        {
            const FaustParameter& param = LibreAudioBaseUI::kFaustParametersIn[i];

            std::string& label = fParameterLabels[kParametersInputStart + i];
            label = param.label;
            label += "##";
            label += param.symbol;

            std::string& render = fParameterRenders[kParametersInputStart + i];
            render = param.isInteger ? "%d" : "%.2f";
            if (param.unit != nullptr)
            {
                render += " ";
                render += param.unit;
            }
        }

        for (uint32_t i = kCommonIOParameters; i < common_output::kFaustParameterCount; ++i)
        {
            const FaustParameter& param = LibreAudioBaseUI::kFaustParametersOut[i];

            std::string& label = fParameterLabels[kParametersOutputStart + i - kCommonIOParameters];
            label = param.label;
            label += "##";
            label += param.symbol;

            std::string& render = fParameterRenders[kParametersOutputStart + i - kCommonIOParameters];
            render = param.isInteger ? "%d" : "%.2f";
            if (param.unit != nullptr)
            {
                render += " ";
                render += param.unit;
            }
        }

        for (uint32_t i = 0, size = LibreAudioBaseUI::kFaustParameters.size(); i < size; ++i)
        {
            const FaustParameter& param = LibreAudioBaseUI::kFaustParameters[i];

            std::string& label = fParameterLabels[kParametersMainStart + i];
            label = param.label;
            label += "##";
            label += param.symbol;

            std::string& render = fParameterRenders[kParametersMainStart + i];
            render = param.isInteger ? "%d" : "%.2f";
            if (param.unit != nullptr)
            {
                render += " ";
                render += param.unit;
            }
        }
    }

protected:
    void onImGuiDisplay() final
    {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(getWidth(), getHeight()));

        constexpr int flags = ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoCollapse;

        ImGui::Begin("LibreAudio", nullptr, flags);

        {
            ImGui::SeparatorText("Input");
            ImGui::BeginGroup();

            for (uint32_t i = 0; i < common_input::kFaustParameterCount; ++i)
            {
                const FaustParameter& param = LibreAudioBaseUI::kFaustParametersIn[i];

                if (param.isOutput)
                    continue;

                displaySlider(param, kParametersInputStart + i);
            }

            ImGui::BeginDisabled();

            for (uint32_t i = 0; i < common_input::kFaustParameterCount; ++i)
            {
                const FaustParameter& param = LibreAudioBaseUI::kFaustParametersIn[i];

                if (! param.isOutput)
                    continue;

                displayMeter(param, kParametersInputStart + i);
            }

            ImGui::EndDisabled();

            ImGui::EndGroup();
        }

        {
            ImGui::SeparatorText("Output");
            ImGui::BeginGroup();

            for (uint32_t i = kCommonIOParameters, size = LibreAudioBaseUI::kFaustParametersOut.size(); i < size; ++i)
            {
                const FaustParameter& param = LibreAudioBaseUI::kFaustParametersOut[i];

                if (param.isOutput)
                    continue;

                displaySlider(param, kParametersOutputStart + i - kCommonIOParameters);
            }

            ImGui::BeginDisabled();

            for (uint32_t i = kCommonIOParameters, size = LibreAudioBaseUI::kFaustParametersOut.size(); i < size; ++i)
            {
                const FaustParameter& param = LibreAudioBaseUI::kFaustParametersOut[i];

                if (! param.isOutput)
                    continue;

                displayMeter(param, kParametersOutputStart + i - kCommonIOParameters);
            }

            ImGui::EndDisabled();

            ImGui::EndGroup();
        }

        bool hasOutputs = false;

        {
            ImGui::SeparatorText("Parameters");
            ImGui::BeginGroup();

            for (uint32_t i = 0, size = LibreAudioBaseUI::kFaustParameters.size(); i < size; ++i)
            {
                const FaustParameter& param = LibreAudioBaseUI::kFaustParameters[i];

                if (param.isOutput)
                {
                    hasOutputs = true;
                    continue;
                }

                displaySlider(param, kParametersMainStart + i);
            }

            ImGui::EndGroup();
        }

        if (hasOutputs)
        {
            ImGui::SeparatorText("Meters / Outputs");
            ImGui::BeginGroup();
            ImGui::BeginDisabled();

            for (uint32_t i = 0, size = LibreAudioBaseUI::kFaustParameters.size(); i < size; ++i)
            {
                const FaustParameter& param = LibreAudioBaseUI::kFaustParameters[i];

                if (! param.isOutput)
                    continue;

                displayMeter(param, kParametersMainStart + i);
            }

            ImGui::EndGroup();
            ImGui::EndDisabled();
        }

        ImGui::End();
    }

private:
    std::vector<std::string> fParameterLabels;
    std::vector<std::string> fParameterRenders;
    LibreAudioBaseUI* const fUI;

    void displaySlider(const FaustParameter& param, const uint32_t index)
    {
        float value = fUI->fParameterValuesRef[index];
        bool modified;

        if (param.isBoolean)
        {
            bool bvalue = value > (param.max - param.min) * 0.5f;
            modified = ImGui::Checkbox(fParameterLabels[index].c_str(), &bvalue);

            if (modified)
                value = bvalue ? param.max : param.min;
        }
        else
        {
            modified = ImGui::SliderFloat(fParameterLabels[index].c_str(),
                                          &value,
                                          param.min,
                                          param.max,
                                          fParameterRenders[index].c_str(),
                                          param.isLogarithmic ? ImGuiSliderFlags_Logarithmic : 0x0);
        }

        if (ImGui::IsItemActivated())
            fUI->parameterControlPressed(index);

        if (modified)
            fUI->parameterControlModified(index, value);

        if (ImGui::IsItemDeactivated())
            fUI->parameterControlReleased(index);
    }

    void displayMeter(const FaustParameter& param, const uint32_t index)
    {
        float value = fUI->fParameterValuesRef[index];
        ImGui::SliderFloat(fParameterLabels[index].c_str(),
                           &value,
                           param.min,
                           param.max,
                           fParameterRenders[index].c_str(),
                           ImGuiSliderFlags_NoInput | (param.isLogarithmic ? ImGuiSliderFlags_Logarithmic : 0x0));
    }
};

class LibreAudioMainArea : public LibreAudioWidget
{
public:
    LibreAudioMainArea(NanoTopLevelWidget* const parent, LibreAudioBaseUI* const ui)
        : LibreAudioWidget(parent)
    {
        fTest = new LibreAudioQuickParamTest(parent, ui);
    }

protected:
    void onNanoDisplay() final
    {
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        LibreAudioWidget::onPositionChanged(ev);

        fTest->setAbsolutePos(ev.pos);
    }

    void onResize(const ResizeEvent& ev) final
    {
        LibreAudioWidget::onResize(ev);

        fTest->setSize(ev.size);
    }

private:
    ScopedPointer<LibreAudioQuickParamTest> fTest;
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

        fTopBar->update(canUndo(), canRedo(), isCopyingSnapshot(), getCurrentSnapshot());
    }

    void uiScaleFactorChanged(const double scaleFactor) final
    {
        fScaleFactor = scaleFactor;
        adjustSize();
    }

private:
    double fScaleFactor = getScaleFactor();

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
