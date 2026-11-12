// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/reference.hpp"
#include "ui/containers/frame.hpp"
#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"

#include "ui/bot/delay-ui.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

struct TransparentCentralArea : Reference::Zero {
    static constexpr const uint padding = Reference::Common::margin;
};

struct OpaqueCentralArea : Reference::CentralArea {
    static constexpr const Color backgroundColor { 0x1a, 0x1b, 0x1f };
};

struct OpaqueSmallCentralArea : OpaqueCentralArea {
    static constexpr const Color backgroundColor { 0x1a, 0x1b, 0x1f };
    static constexpr const uint margin = OpaqueCentralArea::margin / 2;
    static constexpr const uint padding = 0;
};

// --------------------------------------------------------------------------------------------------------------------

class DelayExpertPageWidget final : public LabReferenceContainerWidget<TransparentCentralArea, Horizontal>
{
    using R = TransparentCentralArea;
    using BaseWidget = LabReferenceContainerWidget<R, Horizontal>;
    using BaseWidget::BaseWidget;

    struct RowRef : OpaqueCentralArea {
        static constexpr const uint padding = 0;
    };

    struct SmallRowRef : OpaqueSmallCentralArea {
        static constexpr const uint padding = 0;
        static constexpr const uint height = 100;
    };

    class ControlsColumn : public LabReferenceContainerWidget<TransparentCentralArea, Vertical>
    {
        using BaseWidget = LabReferenceContainerWidget<TransparentCentralArea, Vertical>;
        using BaseWidget::BaseWidget;

    protected:
        std::shared_ptr<FrameContainerReferenceWidget<RowRef>> fTop = addWidget<FrameContainerReferenceWidget<RowRef>, Expanding>();
        std::shared_ptr<FrameContainerReferenceWidget<SmallRowRef>> fBottom = addWidget<FrameContainerReferenceWidget<SmallRowRef>>();
    };

    class ControlsColumnLeft : public ControlsColumn,
                               private IdleCallback
    {
    public:
        explicit ControlsColumnLeft(LabWidget* const parent)
            : ControlsColumn(parent)
        {
            fTop->addPillToggle(kFaustParameterMode);
            // fTop->addSpacer();
            fTop->addKnobGroup<SmallestKnobWidget, 2>(kFaustParameterSync);
            // fTop->addSpacer();
            fKnobsDiv = fTop->addKnobGroup<SmallestKnobWidget, 2>(kFaustParameterDiv_l);
            fKnobsTime = fTop->addKnobGroup<SmallestKnobWidget, 2>(kFaustParameterTime_l);
            // fTop->addSpacer();
            fTop->addKnobGroup<SmallestKnobWidget, 2>(kFaustParameterOffset_l);

            fBottom->addText(kFaustParameterDeess_amount, "DYNAMICS");
            fBottom->addKnobGroup<SmallestKnobWidget, 2>(kFaustParameterDeess_amount);

            update(true);

            addIdleCallback(this);
        }

    private:
        std::shared_ptr<KnobGroupWidget<SmallestKnobWidget, 2>> fKnobsDiv;
        std::shared_ptr<KnobGroupWidget<SmallestKnobWidget, 2>> fKnobsTime;
        bool fSync;
        bool fLink;

        void idleCallback() final
        {
            update();
        }

        void update(const bool init = false)
        {
            bool changed = false;
            const bool sync = d_isNotZero(fInterface->getParameterValue(kParametersMainStart + kFaustParameterSync));
            const bool link = d_isNotZero(fInterface->getParameterValue(kParametersMainStart + kFaustParameterLink));

            if (init || fSync != sync)
            {
                changed = true;
                fSync = sync;
                fKnobsDiv->setVisible(sync);
                fKnobsTime->setVisible(!sync);
            }

            if (init || fLink != link)
            {
                changed = true;
                fLink = link;
                fKnobsDiv->updateEnabledById(kParametersMainStart + kFaustParameterDiv_r, !link);
                fKnobsTime->updateEnabledById(kParametersMainStart + kFaustParameterTime_r, !link);
            }

            if (changed && !init)
                updateSize(true);
        }
    };

    class ControlsColumnRight : public ControlsColumn,
                                private IdleCallback
    {
    public:
        explicit ControlsColumnRight(LabWidget* const parent)
            : ControlsColumn(parent)
        {
            fTop->addPillToggle(kFaustParameterPingpong);
            // fTop->addSpacer();
            fKnobsMode = fTop->addKnobGroup<SmallestKnobWidget, 3>(kFaustParameterFeedback);
            // fTop->addSpacer();
            fTop->addKnobGroup<SmallestKnobWidget, 3>(kFaustParameterMod_rate);
            // fTop->addSpacer();
            fTop->addDualSlider<kFaustParameterHp_freq, kFaustParameterLp_freq>();

            fBottom->addText(kFaustParameterWidth, "OUTPUT");
            fBottom->addKnobGroup<SmallestKnobWidget, 2>(kFaustParameterWidth);

            update(true);

            addIdleCallback(this);
        }

    private:
        std::shared_ptr<KnobGroupWidget<SmallestKnobWidget, 3>> fKnobsMode;
        int fMode;

        void idleCallback() final
        {
            update();
        }

        void update(const bool init = false)
        {
            bool changed = false;
            const int mode = d_roundToIntPositive(fInterface->getParameterValue(kParametersMainStart + kFaustParameterPingpong));

            if (init || fMode != mode)
            {
                fMode = mode;
                fKnobsMode->updateEnabledById(kParametersMainStart + kFaustParameterCross, mode == 0);
            }

            if (changed && !init)
                updateSize(true);
        }
    };

    const std::array<std::shared_ptr<LabWidget>, 3> fWidgets {
        addWidget<ControlsColumnLeft, Expanding>(),
        addWidget<DelayP2, Expanding>(),
        addWidget<ControlsColumnRight, Expanding>(),
    };

public:
    void setScopeZoom(DelayScopeZoom* const zoom) noexcept
    {
        static_cast<DelayP2*>(fWidgets[1].get())->setScopeZoom(zoom);
    }

    [[nodiscard]] Point<int> getMiddleAreaAbsolutePos() const noexcept
    {
        return fWidgets[1]->getAbsolutePos();
    }

    [[nodiscard]] Size<uint> getMiddleAreaSize() const noexcept
    {
        return fWidgets[1]->getSize();
    }
};

// --------------------------------------------------------------------------------------------------------------------

using DelayCentralAreaWidget = CentralAreaWidget<EasyPageWidget, DelayExpertPageWidget>;

class DelayMainArea final : public MainAreaContainerWidget<DelayCentralAreaWidget>
{
    using BaseWidget = MainAreaContainerWidget<DelayCentralAreaWidget>;
    using BaseWidget::BaseWidget;

    const std::shared_ptr<DelayCentralAreaWidget> fCentralArea = getCentralAreaWidget();

public:
    void setScopeZoom(DelayScopeZoom* const zoom) noexcept
    {
        // FIXME
        fCentralArea->getExpertWidget()->setScopeZoom(zoom);
    }

    [[nodiscard]] Point<int> getMiddleAreaAbsolutePos(const Page page) const noexcept
    {
        return page == kPageExpert ? fCentralArea->getExpertWidget()->getMiddleAreaAbsolutePos() : fCentralArea->getAbsolutePos();
    }

    [[nodiscard]] Size<uint> getMiddleAreaSize(const Page page) const noexcept
    {
        return page == kPageExpert ? fCentralArea->getExpertWidget()->getMiddleAreaSize() : fCentralArea->getSize();
    }

    [[nodiscard]] float getMiddleAreaBorderRadius(Page) const noexcept
    {
        return fCentralArea->getBorderRadius();
    }
};

// --------------------------------------------------------------------------------------------------------------------

class DelayRootWidget final : public RootWidget<TopBar, DelayMainArea>,
                              private IdleCallback
{
    using BaseWidget = RootWidget<TopBar, DelayMainArea>;

    Page fLastPage = kPageEasy;
    DelayScopeZoom fScopeZoom;

    using ExpertShaderWidget = UI<DelayRootWidget>::BotShaderPluginWidget;
    std::shared_ptr<ExpertShaderWidget> fShaderExpert;

public:
    DelayRootWidget(Window& window, LabUIWidgetInterface* const iface)
        : BaseWidget(window, iface)
    {
        addIdleCallback(this);
    }

    void enableShaders(const std::list<std::shared_ptr<ShaderWidgetInterface>>& shaders)
    {
        // fShaderExpert = reinterpret_cast<const std::shared_ptr<ExpertShaderWidget>&>(shaders.back());

        // one zoom, read by the shader and by the labels drawn over it
        fMainArea->setScopeZoom(&fScopeZoom);

        // fShaderExpert->setCustomUniform("iSpan", [this] {
        //     return fScopeZoom.span(DelayScopeModel(fInterface).level, getApp().getTime());
        // });
        // fShaderExpert->setVisible(getCurrentPage(fInterface) == kPageExpert);

        BaseWidget::enableShaders(shaders);
    }

private:
    void idleCallback() final
    {
        if (const Page page = getCurrentPage(fInterface); fLastPage != page)
        {
            if (fShaderExpert != nullptr)
                fShaderExpert->setVisible(page == kPageExpert);

            fLastPage = page;
            updateSize(false);
        }
    }

    void updateSize(const bool updateChildren) final
    {
        BaseWidget::updateSize(updateChildren);

        const Point<int> pos = fMainArea->getMiddleAreaAbsolutePos(fLastPage);
        const Size<uint> size = fMainArea->getMiddleAreaSize(fLastPage);
        const float borderRadius = fMainArea->getMiddleAreaBorderRadius(fLastPage);

        for (const std::shared_ptr<ShaderWidgetInterface>& sw : fShaders)
        {
            sw->setAbsolutePos(pos);
            sw->setSize(size);
            sw->setBorderRadius(borderRadius);
        }
    }
};

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::UI<LibreAudio::DelayRootWidget>();
}

END_NAMESPACE_DISTRHO
