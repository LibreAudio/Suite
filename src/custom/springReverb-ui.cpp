// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/container.hpp"
#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"
#include "ui/widgets/grid.hpp"
#include "ui/widgets/number-box.hpp"

#include "LibreAudioParameters.hpp"

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

static constexpr const uint kCommonMargin = 8;
static constexpr const uint kCommonPadding = 6;
static constexpr const uint kNumberBoxHeight = 50;

template<const Color& kAccentColor>
using SpringNumberBoxWidget = NumberBoxWidget<kAccentColor, kNumberBoxHeight>;

// --------------------------------------------------------------------------------------------------------------------

namespace widgets {
    using Predelay = ParameterWidgetPair<kFaustParameterPredelay, SpringNumberBoxWidget<Reference::Colors::acc5>>;
    using Tone = ParameterWidgetPair<kFaustParameterTone, SpringNumberBoxWidget<Reference::Colors::acc3>>;

    using Dwell = ParameterWidgetPair<kFaustParameterDwell, SpringNumberBoxWidget<Reference::Colors::acc1>>;
    using Tension = ParameterWidgetPair<kFaustParameterTension, SpringNumberBoxWidget<Reference::Colors::acc2>>;

    using SpringC = ParameterWidgetPair<kFaustParameterCenter, SpringNumberBoxWidget<Reference::Colors::acc4>>;
    using Crossfeed = ParameterWidgetPair<kFaustParameterCrosslr, SpringNumberBoxWidget<Reference::Colors::acc5>>;

    using Duck = ParameterWidgetPair<kFaustParameterDuck_amount, SpringNumberBoxWidget<Reference::Colors::acc4>>;
    using DeEss = ParameterWidgetPair<kFaustParameterDeess_amount, SpringNumberBoxWidget<Reference::Colors::acc2>>;

    using Width = ParameterWidgetPair<kFaustParameterStereo_width, SpringNumberBoxWidget<Reference::Colors::acc5>>;
    using Mix = ParameterWidgetPair<kFaustParameterMix, SpringNumberBoxWidget<Reference::Colors::acc6>>;
}

// --------------------------------------------------------------------------------------------------------------------

struct SpringGroupReference : Reference::Zero {
    static constexpr const Color backgroundColor { 0x30, 0x32, 0x3c, 0.7f };
    static constexpr const Color borderColor = backgroundColor;
    static constexpr const Color borderHighlightColor { 0x40, 0x42, 0x4c, 0.8f };
    static constexpr const Color borderShadowColor { 0, 0, 0, 0.12f };
    static constexpr const uint border = 1;
    static constexpr const uint borderRadius = 8;
    static constexpr const uint margin = kCommonMargin;
    static constexpr const uint padding = kCommonPadding;
    static constexpr const uint height = kNumberBoxHeight + (border + margin) * 2;

    static constexpr const Color shadowColor { 0, 0, 0, 0.6f };
    static constexpr const Point<int> shadowOffset = { 0, 5 };
    static constexpr const Size<int> shadowSize = { -12, -12 };
    static constexpr const uint shadowFeather = 14;
};

template <typename... Args>
using SpringGroupWidget = StaticParameterContainerWidget<SpringGroupReference, Horizontal, Expanding, Args...>;

// --------------------------------------------------------------------------------------------------------------------

struct SpringGroupsContainerReference : Reference::Zero {
    static constexpr const uint height = SpringGroupReference::height + (border + margin) * 2;
};

// Sizes each group by its number of boxes, so that all boxes get the same width.
// The gaps between groups are spacers, which also absorb the rounding remainder.
class SpringGroupsContainerWidget final : public ReferenceContainerWidget<SpringGroupsContainerReference, Horizontal>
{
    using BaseWidget = ReferenceContainerWidget<SpringGroupsContainerReference, Horizontal>;

    struct Item {
        std::shared_ptr<LabWidget> widget;
        uint numBoxes;
    };

    const std::array<Item, 7> fItems {
        addGroup<widgets::Predelay, widgets::Tone>(),
        addGap(),
        addGroup<widgets::Dwell, widgets::Tension, widgets::SpringC, widgets::Crossfeed>(),
        addGap(),
        addGroup<widgets::Duck, widgets::DeEss>(),
        addGap(),
        addGroup<widgets::Width, widgets::Mix>(),
    };

public:
    explicit SpringGroupsContainerWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

private:
    template <typename... Args>
    Item addGroup()
    {
        return { addWidget<SpringGroupWidget<Args...>, Fixed>(), sizeof...(Args) };
    }

    Item addGap()
    {
        return { addSpacer(), 0 };
    }

    void updateSize(const bool updateChildren) final
    {
        using G = SpringGroupReference;

        const uint frame = d_roundToUnsignedInt((G::border + G::margin) * 2 * fScaleFactor);
        const uint padding = d_roundToUnsignedInt(G::padding * fScaleFactor);

        uint spaceUsed = 0;
        uint numBoxes = 0;

        for (const Item& item : fItems)
        {
            spaceUsed += item.numBoxes != 0 ? frame + padding * (item.numBoxes - 1) : padding;
            numBoxes += item.numBoxes;
        }

        const uint width = getWidth();
        const uint boxWidth = width > spaceUsed ? (width - spaceUsed) / numBoxes : 0;

        for (const Item& item : fItems)
        {
            if (item.numBoxes != 0)
                item.widget->setWidth(frame + padding * (item.numBoxes - 1) + boxWidth * item.numBoxes);
        }

        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

struct SpringExpertPageReference : Reference::Zero {
    static constexpr const uint margin = 12;
    static constexpr const uint padding = 8;
};

using SpringExpertPageWidget = StaticBottomAlignedContainerWidget<
    SpringExpertPageReference,
    SpringGroupsContainerWidget
>;

// --------------------------------------------------------------------------------------------------------------------

class SpringStageWidget final : public StageWidget<EasyStageWidget, SpringExpertPageWidget>
{
    using BaseWidget = StageWidget<EasyStageWidget, SpringExpertPageWidget>;

    // an empty dB range leaves only the frequency lines
    const std::shared_ptr<LabWidget> fGrid { addGraphWidget<GridWidget>(0.f, 0.f, 1.f) };

public:
    explicit SpringStageWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

    Size<uint> getGraphSize() const noexcept final
    {
        const LabMetrics<R> m { this };

        const float gh = m.height - (R::border + R::margin * 2 +
            (fLastPage == kPageExpert
                ? SpringGroupReference::height
                : Reference::Widgets::CardGroup::height + R::margin * 0.5f
            )) * fScaleFactor;

        return { getWidth(), d_roundToUnsignedInt(gh) };
    }
};

// --------------------------------------------------------------------------------------------------------------------

using SpringMainArea = MainAreaContainerWidget<SpringStageWidget>;
using SpringRootWidget = RootWidget<TopBar, SpringMainArea>;

// --------------------------------------------------------------------------------------------------------------------

class SpringReverbUI : public LibreAudioBaseUI
{
    using ShaderAnalyzerW = BotShaderWidget<SHADERS_ANALYSER_FFT_FRAG_DATA, SHADERS_ANALYSER_FFT_FRAG_LEN, LibreAudioAnalyzerIPC::kNumBins>;
    using ShaderSpringsW = BotShaderWidget<SHADERS_CURVE_SPRING_REVERB_FRAG_DATA, SHADERS_CURVE_SPRING_REVERB_FRAG_LEN>;

public:
    SpringReverbUI()
    {
        const std::list<std::shared_ptr<ShaderWidgetInterface>> shaders {
            std::make_shared<ShaderBackgroundWidget>(this, this),
            std::make_shared<BotShaderWidget<SHADER_CLOUDSTARFIELD>>(this, this),
            makeShaderAnalyzer(),
            std::make_shared<ShaderSpringsW>(this, this),
        };

        createRootWidget<SpringRootWidget>();

        static_cast<SpringRootWidget*>(fRootWidget.get())->enableShaders(shaders);
    }

protected:
    std::shared_ptr<ShaderAnalyzerW> fShaderAnalyzer;

    void audioGraphReceived(const float values[LibreAudioAnalyzerIPC::kNumBins]) final
    {
        fShaderAnalyzer->replace(values);
    }

private:
    std::shared_ptr<ShaderAnalyzerW> makeShaderAnalyzer()
    {
        fShaderAnalyzer.reset(new ShaderAnalyzerW(this, this));
        return fShaderAnalyzer;
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::SpringReverbUI();
}

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
