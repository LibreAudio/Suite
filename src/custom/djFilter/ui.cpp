// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioBaseUI.hpp"

#include "Layout.hpp"

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

struct Metrics {
    struct Window {
        static constexpr const uint padding = 0;
        static constexpr const uint margin = 0;
    };

    struct TopBar {
        static constexpr const uint height = 46;
        static constexpr const uint padding = 0;
        static constexpr const uint margin = 0;
        // static constexpr const uint paddingLeft = 7;
        // static constexpr const uint paddingRight = 14;
        struct Cluster {
            static constexpr const uint gap = 60;
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
        : LibreAudioWidget(parent)
    {
        // rescaled(scaleFactor);
    }

protected:
    void onNanoDisplay() final
    {
        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(0.2f, 0.f, 0.f);
        fill();
        fillColor(1.f, 1.f, 1.f);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), "LA");
    }
};

class LibreAudioPluginName : public LibreAudioWidget
{
public:
    LibreAudioPluginName(NanoSubWidget* const parent)
        : LibreAudioWidget(parent)
    {
        // rescaled(scaleFactor);
    }

protected:
    void onNanoDisplay() final
    {
        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(0.2f, 0.1f, 0.f);
        fill();
        fillColor(1.f, 1.f, 1.f);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), "CHORUS");
    }
};

class LibreAudioPresetWidget : public LibreAudioWidget
{
public:
    LibreAudioPresetWidget(NanoSubWidget* const parent)
        : LibreAudioWidget(parent)
    {
        // rescaled(scaleFactor);
    }

protected:
    void onNanoDisplay() final
    {
        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(0.2f, 0.2f, 0.1f);
        fill();
        fillColor(1.f, 1.f, 1.f);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), "Init Preset \\/");
    }
};

class LibreAudioUndoRedoWidget : public LibreAudioWidget
{
public:
    LibreAudioUndoRedoWidget(NanoSubWidget* const parent)
        : LibreAudioWidget(parent)
    {
        // rescaled(scaleFactor);
    }

protected:
    void onNanoDisplay() final
    {
        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(0.1f, 0.2f, 0.2f);
        fill();
        fillColor(1.f, 1.f, 1.f);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), "U / R");
    }
};

class LibreAudioSnapshotsWidget : public LibreAudioWidget
{
public:
    LibreAudioSnapshotsWidget(NanoSubWidget* const parent)
        : LibreAudioWidget(parent)
    {
        // rescaled(scaleFactor);
    }

protected:
    void onNanoDisplay() final
    {
        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(0.2f, 0.2f, 0.2f);
        fill();
        fillColor(1.f, 1.f, 1.f);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), "Snapshots");
    }
};

class LibreAudioEasyExpertWidget : public LibreAudioWidget
{
public:
    LibreAudioEasyExpertWidget(NanoSubWidget* const parent)
        : LibreAudioWidget(parent)
    {
        // rescaled(scaleFactor);
    }

protected:
    void onNanoDisplay() final
    {
        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(0.1f, 0.2f, 0.2f);
        fill();
        fillColor(1.f, 1.f, 1.f);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), "Easy / Expert");
    }
};

class LibreAudioMenuWidget : public LibreAudioWidget
{
public:
    LibreAudioMenuWidget(NanoSubWidget* const parent)
        : LibreAudioWidget(parent)
    {
        // rescaled(scaleFactor);
    }

protected:
    void onNanoDisplay() final
    {
        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(0.2f, 0.2f, 0.2f);
        fill();
        fillColor(1.f, 1.f, 1.f);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), "= (I)");
    }
};

class LibreAudioTopBarCluster : public LibreAudioWidget
{
public:
    LibreAudioTopBarCluster(NanoSubWidget* const parent)
        : LibreAudioWidget(parent)
    {
        fPreset = new LibreAudioPresetWidget(this);
        fUndoRedo = new LibreAudioUndoRedoWidget(this);
        fSnapshots = new LibreAudioSnapshotsWidget(this);
        fEasyExpert = new LibreAudioEasyExpertWidget(this);
        fMenu = new LibreAudioMenuWidget(this);

        fLayout.widgets.push_back({ fPreset, Expanding });
        fLayout.widgets.push_back({ fUndoRedo, Fixed });
        fLayout.widgets.push_back({ fSnapshots, Fixed });
        fLayout.widgets.push_back({ fEasyExpert, Fixed });
        fLayout.widgets.push_back({ fMenu, Fixed });
    }

protected:
    void onNanoDisplay() final
    {
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        NanoSubWidget::onPositionChanged(ev);

        const uint gap = d_roundToUnsignedInt(Metrics::TopBar::Cluster::gap * fScaleFactor);
        fLayout.setAbsolutePos(ev.pos.getX(), ev.pos.getY(), gap, 0);
    }

    void onResize(const ResizeEvent& ev) final
    {
        NanoSubWidget::onResize(ev);

        const uint gap = d_roundToUnsignedInt(Metrics::TopBar::Cluster::gap * fScaleFactor);
        const uint height = getHeight();

        fPreset->setHeight(height);
        fUndoRedo->setSize(40, height);
        fSnapshots->setSize(104, height);
        fEasyExpert->setSize(92, height);
        fMenu->setSize(45, height);

        fLayout.setWidth(getWidth(), gap, 0);
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
    LibreAudioTopBar(NanoTopLevelWidget* const parent)
        : LibreAudioWidget(parent)
    {
        fLogo = new LibreAudioLogo(this);
        fPluginName = new LibreAudioPluginName(this);
        fCluster = new LibreAudioTopBarCluster(this);

        fLayout.widgets.push_back({ fLogo, Fixed });
        fLayout.widgets.push_back({ fPluginName, Expanding });
        fLayout.widgets.push_back({ fCluster, Fixed });

        // rescaled(scaleFactor);
    }

protected:
    void onNanoDisplay() final
    {
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

        const uint width = ev.size.getWidth();
        const uint height = ev.size.getHeight();

        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::margin * fScaleFactor);
        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::padding * fScaleFactor);

        fLogo->setSize(34, height);
        fPluginName->setHeight(height);
        fCluster->setSize(610, height);

        fLayout.setWidth(width, padding, margin);

        fLayout.setAbsolutePos(getAbsoluteX(), getAbsoluteY(), padding, margin);
    }

    // void rescaled(const double scaleFactor) final
    // {
    //     setSize(d_roundToUnsignedInt((Metrics::width - Metrics::padding * 2) * scaleFactor),
    //             d_roundToUnsignedInt(Metrics::topBarHeight * scaleFactor));
    // }

private:
    HorizontalLayout fLayout;
    ScopedPointer<LibreAudioLogo> fLogo;
    ScopedPointer<LibreAudioPluginName> fPluginName;
    ScopedPointer<LibreAudioTopBarCluster> fCluster;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioMainArea : public LibreAudioWidget
{
public:
    LibreAudioMainArea(NanoTopLevelWidget* const parent)
        : LibreAudioWidget(parent)
    {
        // rescaled(scaleFactor);
    }

protected:
    void onNanoDisplay() final
    {
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
    }

    // void rescaled(const double scaleFactor) final
    // {
    //     setWidth(d_roundToUnsignedInt((Metrics::width - Metrics::padding * 2) * scaleFactor));
    // }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioBottomBar : public LibreAudioWidget
{
public:
    LibreAudioBottomBar(NanoTopLevelWidget* const parent)
        : LibreAudioWidget(parent)
    {
        // rescaled(scaleFactor);
    }

protected:
    void onNanoDisplay() final
    {
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

    // void rescaled(const double scaleFactor) final
    // {
    //     setSize(d_roundToUnsignedInt((Metrics::width - Metrics::padding * 2) * scaleFactor),
    //             d_roundToUnsignedInt(Metrics::bottomBarHeight * scaleFactor));
    // }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioUI : public LibreAudioBaseUI
{
public:
    LibreAudioUI()
        : LibreAudioBaseUI()
    {
        loadSharedResources();

        fTopBar = new LibreAudioTopBar(this);
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

    void onNanoDisplay() final
    {
        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(fColors.background);
        fill();
        strokeColor(fColors.outline);
        strokeWidth(fScaleFactor * 2); // x2 for making it centered
        stroke();
    }

    void onResize(const ResizeEvent& ev) final
    {
        UI::onResize(ev);
        adjustSize();
    }

    void uiScaleFactorChanged(const double scaleFactor) final
    {
        fScaleFactor = scaleFactor;
        adjustSize();
    }

private:
    double fScaleFactor = getScaleFactor();

    const struct {
        Color background = Color::fromHTML("#28282d");
        Color outline = Color::fromHTML("#3a3a42");
    } fColors;

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
