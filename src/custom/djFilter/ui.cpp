// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioBaseUI.hpp"

#include "Layout.hpp"

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

struct Metrics {
    static constexpr const int x = 0;
    static constexpr const int y = 0;
    static constexpr const uint margin = 0;
    static constexpr const uint bottomBarHeight = 46;

    struct TopBar {
        static constexpr const uint height = 46;
        static constexpr const uint paddingLeft = 7;
        static constexpr const uint paddingRight = 14;
    };
};

class LibreAudioWidget : public NanoSubWidget
{
public:
    LibreAudioWidget(NanoTopLevelWidget* const parent)
        : NanoSubWidget(parent)
    {
    }
/*
protected:
    virtual void rescaled(double scaleFactor) = 0;*/
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioLogo : public NanoSubWidget
{
public:
    LibreAudioLogo(NanoSubWidget* const parent)
        : NanoSubWidget(parent)
    {
        // rescaled(scaleFactor);
    }

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

class LibreAudioPluginName : public NanoSubWidget
{
public:
    LibreAudioPluginName(NanoSubWidget* const parent)
        : NanoSubWidget(parent)
    {
        // rescaled(scaleFactor);
    }

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

class LibreAudioTopBarCluster : public NanoSubWidget
{
public:
    LibreAudioTopBarCluster(NanoSubWidget* const parent)
        : NanoSubWidget(parent)
    {
        // rescaled(scaleFactor);
    }

    void onNanoDisplay() final
    {
        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(0.2f, 0.1f, 0.1f);
        fill();
        fillColor(1.f, 1.f, 1.f);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), "Top-bar cluster goes here");
    }
};

class LibreAudioTopBar : public LibreAudioWidget
{
public:
    LibreAudioTopBar(NanoTopLevelWidget* const parent, const double scaleFactor)
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

    void adjustSize(const uint width, const uint height)
    {
        const double fScaleFactor = 1.0;
        const uint margin = d_roundToUnsignedInt(Metrics::margin * fScaleFactor);
        const uint widgetHeight = height - d_roundToUnsignedInt(Metrics::margin * 2 * fScaleFactor);

        setSize(width, height);

        fLogo->setSize(34, widgetHeight);
        fPluginName->setHeight(widgetHeight);
        fCluster->setSize(610, widgetHeight);

        fLayout.setSize(width, margin);

        fLayout.setAbsolutePos(d_roundToIntPositive(Metrics::x * fScaleFactor),
                               d_roundToIntPositive(Metrics::y * fScaleFactor),
                               margin);
    }

protected:
    void onNanoDisplay() final
    {
        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(0.1f, 0.f, 0.f);
        fill();
        fillColor(1.f, 1.f, 1.f);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), "This is the top-bar");
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

class LibreAudioMainArea : public LibreAudioWidget
{
public:
    LibreAudioMainArea(NanoTopLevelWidget* const parent, const double scaleFactor)
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

    // void rescaled(const double scaleFactor) final
    // {
    //     setWidth(d_roundToUnsignedInt((Metrics::width - Metrics::padding * 2) * scaleFactor));
    // }
};

class LibreAudioBottomBar : public LibreAudioWidget
{
public:
    LibreAudioBottomBar(NanoTopLevelWidget* const parent, const double scaleFactor)
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

        fTopBar = new LibreAudioTopBar(this, fScaleFactor);
        fMainArea = new LibreAudioMainArea(this, fScaleFactor);
        fBottomBar = new LibreAudioBottomBar(this, fScaleFactor);

        fLayout.widgets.push_back({ fTopBar, Fixed });
        fLayout.widgets.push_back({ fMainArea, Expanding });
        fLayout.widgets.push_back({ fBottomBar, Fixed });

        adjustSize(getWidth(), getHeight());
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
        adjustSize(ev.size.getWidth(), ev.size.getHeight());
    }

    void uiScaleFactorChanged(const double scaleFactor) final
    {
        fScaleFactor = scaleFactor;
        adjustSize(getWidth(), getHeight());
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

    void adjustSize(const uint width, const uint height)
    {
        const uint margin = d_roundToUnsignedInt(Metrics::margin * fScaleFactor);
        const uint widgetWidth = width - d_roundToUnsignedInt(Metrics::margin * 2 * fScaleFactor);

        fTopBar->adjustSize(widgetWidth, d_roundToUnsignedInt(Metrics::TopBar::height * fScaleFactor));
        fMainArea->setWidth(widgetWidth);
        fBottomBar->setSize(widgetWidth, d_roundToUnsignedInt(Metrics::bottomBarHeight * fScaleFactor));

        fLayout.setSize(height, margin);

        fLayout.setAbsolutePos(d_roundToIntPositive(Metrics::x * fScaleFactor),
                               d_roundToIntPositive(Metrics::y * fScaleFactor),
                               margin);
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
