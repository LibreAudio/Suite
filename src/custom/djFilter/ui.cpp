// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioBaseUI.hpp"

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
} gColors;

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
                static constexpr const uint padding = 12;
                static constexpr const uint margin = 0;
                static constexpr const uint imageSize = 14;
            };
            struct Snapshots {
                static constexpr const uint padding = 10;
            };
            struct EasyExpert {
                static constexpr const uint padding = 10;
            };
            struct Menu {
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

class LibreAudioUndoRedoButton : public LibreAudioWidget,
                                 public ButtonEventHandler
{
public:
    LibreAudioUndoRedoButton(NanoSubWidget* const parent, const uchar* const data, const uint dataSize)
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
        const double size = Metrics::TopBar::Cluster::UndoRedo::imageSize * fScaleFactor;
        const uint width = getWidth();
        const uint height = getHeight();

        globalTint(isEnabled() ? (getState() & kButtonStateHover ? gColors.acc : gColors.ink2) : gColors.ink3);

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

class LibreAudioUndoRedoWidget : public LibreAudioWidget
{
public:
    LibreAudioUndoRedoWidget(NanoSubWidget* const parent, ButtonEventHandler::Callback* const callback)
        : LibreAudioWidget(parent)
    {
        fUndoButton = new LibreAudioUndoRedoButton(this, IMAGES_UNDO_PNG_DATA, IMAGES_UNDO_PNG_LEN);
        fRedoButton = new LibreAudioUndoRedoButton(this, IMAGES_REDO_PNG_DATA, IMAGES_REDO_PNG_LEN);

        fUndoButton->setCallback(callback);
        fRedoButton->setCallback(callback);

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
    ScopedPointer<LibreAudioUndoRedoButton> fUndoButton;
    ScopedPointer<LibreAudioUndoRedoButton> fRedoButton;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioSnapshotsWidget : public LibreAudioWidget
{
public:
    LibreAudioSnapshotsWidget(NanoSubWidget* const parent)
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
        textBox(0.f, getHeight() * 0.5f, getWidth(), "Snapshots");
    }
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
        fSnapshots = new LibreAudioSnapshotsWidget(this);
        fEasyExpert = new LibreAudioEasyExpertWidget(this);
        fMenu = new LibreAudioMenuWidget(this);

        fLayout.widgets.push_back({ fPreset, Expanding });
        fLayout.widgets.push_back({ fUndoRedo, Fixed });
        fLayout.widgets.push_back({ fSnapshots, Fixed });
        fLayout.widgets.push_back({ fEasyExpert, Fixed });
        fLayout.widgets.push_back({ fMenu, Fixed });
    }

    void update(const bool canUndo, const bool canRedo)
    {
        fUndoRedo->update(canUndo, canRedo);
    }

protected:
    void onNanoDisplay() final
    {
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        NanoSubWidget::onPositionChanged(ev);

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::margin * fScaleFactor);
        fLayout.setAbsolutePos(ev.pos.getX(), ev.pos.getY(), padding, margin);
    }

    void onResize(const ResizeEvent& ev) final
    {
        NanoSubWidget::onResize(ev);

        const uint height = ev.size.getHeight();

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::margin * fScaleFactor);

        fPreset->setHeight(height);
        fUndoRedo->setSize(40, height);
        fSnapshots->setSize(104, height);
        fEasyExpert->setSize(92, height);
        fMenu->setSize(45, height);

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

    void update(const bool canUndo, const bool canRedo)
    {
        fCluster->update(canUndo, canRedo);
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
        NanoSubWidget::onPositionChanged(ev);

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
    LibreAudioQuickParamTest(SubWidget* const parent)
        : ImGuiSubWidget(parent)
    {
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
            ImGui::SeparatorText("Undo / Redo");
            ImGui::BeginGroup();

            ImGui::EndGroup();
        }

        ImGui::End();
    }

private:
    std::vector<std::string> fParameterLabels;
    std::vector<std::string> fParameterRenders;

    void displayMeter(const FaustParameter& param, uint32_t index);
    void displaySlider(const FaustParameter& param, uint32_t index);
};

class LibreAudioMainArea : public LibreAudioWidget
{
public:
    LibreAudioMainArea(NanoTopLevelWidget* const parent)
        : LibreAudioWidget(parent)
    {
        fTest = new LibreAudioQuickParamTest(this);
        fTest->setSize(getSize());
    }

protected:
    void onNanoDisplay() final
    {
        fontSize(26.f * fScaleFactor);

        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(0.f, 0.1f, 0.f);
        fill();
        fillColor(1.f, 1.f, 1.f);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), "This is the main area");
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
        fMainArea = new LibreAudioMainArea(this);
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
        d_stdout("buttonClicked %p %d", widget, widget->getId());
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

        fTopBar->update(canUndo(), canRedo());
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
