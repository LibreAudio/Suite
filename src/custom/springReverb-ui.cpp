// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// Copyright (C) 2026 Klaus Scheuermann <klaus@libreaudio.org>
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
static constexpr const uint kNumberBoxHeight = 44;

static constexpr const Color& getAccentColor(const uint accentcolor)
{
    switch (accentcolor)
    {
    default:
    case 1: return Reference::Colors::acc1;
    case 2: return Reference::Colors::acc2;
    case 3: return Reference::Colors::acc3;
    case 4: return Reference::Colors::acc4;
    case 5: return Reference::Colors::acc5;
    case 6: return Reference::Colors::acc6;
    }
}

// number box using the accent color declared in the DSP
template<FaustParameterIndex index>
using SpringNumberBoxPair = ParameterWidgetPair<
    index,
    NumberBoxWidget<getAccentColor(kFaustParameters[index].accentcolor), kNumberBoxHeight>
>;

// --------------------------------------------------------------------------------------------------------------------

namespace widgets {
    using Predelay = SpringNumberBoxPair<kFaustParameterPredelay>;
    using Tone = SpringNumberBoxPair<kFaustParameterTone>;

    using Dwell = SpringNumberBoxPair<kFaustParameterDwell>;
    using Tension = SpringNumberBoxPair<kFaustParameterTension>;

    using SpringC = SpringNumberBoxPair<kFaustParameterCenter>;
    using Crossfeed = SpringNumberBoxPair<kFaustParameterCrosslr>;

    using Duck = SpringNumberBoxPair<kFaustParameterDuck_amount>;
    using DeEss = SpringNumberBoxPair<kFaustParameterDeess_amount>;

    using Width = SpringNumberBoxPair<kFaustParameterStereo_width>;
    using Mix = SpringNumberBoxPair<kFaustParameterMix>;
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

struct SpringLabeledGroupReference : Reference::Zero {
    static constexpr const Color color { Reference::Colors::ink3, 0.6f };
    static constexpr const float fontSize = 10.f;
    static constexpr const float letterSpacing = 1.5f;
    static constexpr const uint labelHeight = 18;
    static constexpr const uint height = SpringGroupReference::height + labelHeight;
};

// A group with the DSP bracket of its first parameter centered above it.
template <typename First, typename... Rest>
class SpringLabeledGroupWidget final : public StaticBottomAlignedContainerWidget<SpringLabeledGroupReference,
                                                                                 SpringGroupWidget<First, Rest...>>
{
    using R = SpringLabeledGroupReference;
    using BaseWidget = StaticBottomAlignedContainerWidget<R, SpringGroupWidget<First, Rest...>>;

public:
    explicit SpringLabeledGroupWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

private:
    void onNanoDisplay() final
    {
        const float s = this->fScaleFactor;

        this->fillColor(R::color);
        this->fontFace("regular");
        this->fontSize(R::fontSize * s);
        this->textLetterSpacing(R::letterSpacing * s);
        this->textAlign(BaseWidget::ALIGN_CENTER | BaseWidget::ALIGN_MIDDLE);
        this->text(this->getWidth() * 0.5f, R::labelHeight * 0.5f * s, kFaustParameters[First::kIndex].bracket, nullptr);
    }
};

// --------------------------------------------------------------------------------------------------------------------

struct SpringGroupsContainerReference : Reference::Zero {
    static constexpr const uint height = SpringLabeledGroupReference::height + (border + margin) * 2;
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
        return { addWidget<SpringLabeledGroupWidget<Args...>, Fixed>(), sizeof...(Args) };
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

// Three horizontal rainbow springs, L on top, C in the middle, R at the bottom, drawn as helices seen edge on.
// Each is clamped at both ends and shakes in its transverse modes:
//  - input level: how far it shakes, scaled by that tank's gain, which also sets how bright it is drawn
//  - Tension: faster shaking, more and shallower coils, a faster travelling coil wave
//  - Dwell: more high modes alive at once, more weight on the slow level envelope
// The modes are dispersive, mode m sits slightly above m * f1, so the shapes never repeat exactly.
//
// A turn of the coil is stroked as pieces, each with a horizontal gradient for its color and front-to-back fade.
// The wire splits each piece in two to follow the fade closely, the glow only covers the pieces on the front.
class SpringCoilsWidget final : public LabReferenceWidget<Reference::Zero>
{
    using BaseWidget = LabReferenceWidget<Reference::Zero>;

    static constexpr const float kTau = 6.28318530718f;
    static constexpr const float kPi = 3.14159265359f;

    static constexpr const uint kNumModes = 4;
    static constexpr const uint kPiecesPerTurn = 4;
    static constexpr const uint kPointsPerPiece = 7;
    static constexpr const uint kMiddlePoint = kPointsPerPiece / 2;

    static constexpr const float kYCenter = 0.36f;      // group of three, as a fraction of the stage height
    static constexpr const float kYSpace = 0.2f;        // distance between springs, same unit
    static constexpr const float kSwayMax = 0.075f;     // peak centreline travel, same unit
    static constexpr const float kSwayIdle = 0.05f;     // part of that left with no input
    static constexpr const float kDimMin = 0.05f;       // brightness of a tank turned fully down
    static constexpr const float kEnds = 0.1f;          // width fraction over which the coil reaches full depth
    static constexpr const float kThickness = 1.5f;

    // outermost first, together they approximate a gaussian falloff around the wire
    static constexpr const float kGlowWidths[] = { 5.f, 3.f };
    static constexpr const float kGlowAlphas[] = { 0.14f, 0.4f };

    static constexpr const float kLevelSilenceDb = -70.f;
    static constexpr const float kLevelFloorDb = -40.f;
    static constexpr const float kLevelCeilDb = 0.f;
    static constexpr const float kLevelSlowSeconds = 5.f;
    static constexpr const float kLevelFastSeconds = 1.5f;
    static constexpr const float kLevelTimeAttackSeconds = 1.f;
    static constexpr const float kLevelTimeReleaseSeconds = 2.f;
    static constexpr const float kFrameSeconds = 0.016f;

    static constexpr const Color kRainbow[] = {
        Reference::Colors::acc2,
        Reference::Colors::acc3,
        Reference::Colors::acc4,
        Reference::Colors::acc5,
        Reference::Colors::acc1,
        Reference::Colors::acc6,
        { 0xff, 0xdc, 0xf5 },
    };

    struct Tank {
        float y;
        float detune;
        float seed;
    };

    const LabWidget* const fStage;
    const double fStartTime = getTime();
    double fLastTime = 0.0;

    ExponentialValueSmoother fLevelSlow;
    ExponentialValueSmoother fLevelFast;
    float fLevelSlowHeld = 0.f;
    float fLevelSlowTime = 0.f;

public:
    explicit SpringCoilsWidget(LabWidget* const parent)
        : BaseWidget(parent),
          fStage(parent)
    {
        for (ExponentialValueSmoother* const level : { &fLevelSlow, &fLevelFast })
        {
            level->setSampleRate(1.f / kFrameSeconds);
            level->setTargetValue(kLevelSilenceDb);
            level->clearToTargetValue();
        }

        fLevelSlow.setTimeConstant(kLevelSlowSeconds);
        fLevelFast.setTimeConstant(kLevelFastSeconds);
    }

private:
    static float normalizeLevel(const float db) noexcept
    {
        return std::clamp((db - kLevelFloorDb) / (kLevelCeilDb - kLevelFloorDb), 0.f, 1.f);
    }

    static float smoothstep(const float edge, const float x) noexcept
    {
        const float t = std::clamp(x / edge, 0.f, 1.f);
        return t * t * (3.f - 2.f * t);
    }

    static Color rainbow(const float u, const float alpha) noexcept
    {
        const float x = std::clamp(u, 0.f, 1.f) * (std::size(kRainbow) - 1);
        const uint i = std::min<uint>(x, std::size(kRainbow) - 2);
        const float f = x - i;
        const Color& a = kRainbow[i];
        const Color& b = kRainbow[i + 1];

        return {
            a.red + (b.red - a.red) * f,
            a.green + (b.green - a.green) * f,
            a.blue + (b.blue - a.blue) * f,
            alpha,
        };
    }

    void onNanoDisplay() final
    {
        const double time = getTime() - fStartTime;
        const float frameSeconds = std::clamp<double>(time - fLastTime, 0.0, 0.1);
        fLastTime = time;

        const float peakDb = std::max({
            kLevelSilenceDb,
            fInterface->getParameterValue(kParametersInputStart + common_input::kFaustParameterInput_peak_l),
            fInterface->getParameterValue(kParametersInputStart + common_input::kFaustParameterInput_peak_r),
        });

        fLevelSlow.setTargetValue(peakDb);
        fLevelFast.setTargetValue(peakDb);

        const float slow = normalizeLevel(fLevelSlow.next());
        const float fast = normalizeLevel(fLevelFast.next());

        // the coil wave rate follows the integrated level, so a level change does not move the coils already on screen
        const float levelSeconds = slow >= fLevelSlowHeld ? kLevelTimeAttackSeconds : kLevelTimeReleaseSeconds;
        fLevelSlowHeld += (slow - fLevelSlowHeld) * (1.f - std::exp(-frameSeconds / levelSeconds));
        fLevelSlowTime += fLevelSlowHeld * frameSeconds;

        const float dwell = fInterface->getParameterValue(kParametersMainStart + kFaustParameterDwell) * 0.01f;
        const float tension = fInterface->getParameterValue(kParametersMainStart + kFaustParameterTension) * 0.01f;
        const float centerDb = fInterface->getParameterValue(kParametersMainStart + kFaustParameterCenter);

        // dwell moves the motion from the fast envelope to the slow one, the difference is the whip of a transient
        const float sustain = fast + (slow - fast) * (0.25f + 0.6f * dwell);
        const float drive = std::min(sustain + 0.7f * std::max(fast - slow, 0.f), 1.f);

        lineJoin(BEVEL);

        // top spring nearest the viewer
        drawTank({ kYCenter + kYSpace, 1.13f, 0.73f }, 0.f, time, drive, dwell, tension);
        drawTank({ kYCenter, 0.86f, 0.37f }, centerDb, time, drive, dwell, tension);
        drawTank({ kYCenter - kYSpace, 1.f, 0.f }, 0.f, time, drive, dwell, tension);
    }

    void drawTank(const Tank& tank,
                  const float db,
                  const double time,
                  const float drive,
                  const float dwell,
                  const float tension)
    {
        const float width = getWidth();
        const float height = fStage->getHeight();

        const float f1 = (0.55f + 1.05f * tension) * tank.detune;
        const float lush = 2.2f - 1.4f * dwell;
        const float coils = 16.f + 14.f * tension;
        const float depth = (0.026f - 0.01f * tension) * height;
        const float travel = 0.06f * time + (0.1f + 0.14f * tension) * fLevelSlowTime + tank.seed;

        // amplitude follows the square root of the power gain, so a tank at -20 dB still visibly stirs
        const float gain = std::pow(10.f, db / 40.f);
        const float amp = kSwayMax * (kSwayIdle + (1.f - kSwayIdle) * drive) * gain * height;
        const float dim = kDimMin + (1.f - kDimMin) * gain;

        // quadratic sum, the plain one would leave the spring almost straight whenever the modes cancel
        float modes[kNumModes];
        float norm = 0.f;

        for (uint i = 0; i < kNumModes; ++i)
        {
            const float m = i + 1;
            const float w = std::pow(m, -lush);
            const double rate = f1 * m * std::sqrt(1.f + (0.01f + 0.03f * tension) * m * m);
            modes[i] = w * std::sin(kTau * std::fmod(rate * time + tank.seed * m, 1.0));
            norm += w * w;
        }

        for (float& mode : modes)
            mode *= amp / std::sqrt(norm);

        const float pieceTurns = 1.f / kPiecesPerTurn;
        const int firstPiece = std::floor(-travel / pieceTurns);
        const int lastPiece = std::ceil((coils - travel) / pieceTurns);

        // pieces end on turn fractions, so the brightest and darkest points of the coil fall on piece boundaries
        for (int piece = firstPiece; piece < lastPiece; ++piece)
        {
            const float u1 = std::max((piece * pieceTurns + travel) / coils, 0.f);
            const float u2 = std::min(((piece + 1) * pieceTurns + travel) / coils, 1.f);

            if (u2 <= u1)
                continue;

            const float uMid = (u1 + u2) * 0.5f;
            const float front1 = 0.5f + 0.5f * std::cos(kTau * (coils * u1 - travel));
            const float frontMid = 0.5f + 0.5f * std::cos(kTau * (coils * uMid - travel));
            const float front2 = 0.5f + 0.5f * std::cos(kTau * (coils * u2 - travel));

            float xs[kPointsPerPiece];
            float ys[kPointsPerPiece];

            for (uint p = 0; p < kPointsPerPiece; ++p)
            {
                const float u = u1 + (u2 - u1) * p / (kPointsPerPiece - 1);
                const float ends = smoothstep(kEnds, u) * smoothstep(kEnds, 1.f - u);

                float y = tank.y * height + depth * ends * std::sin(kTau * (coils * u - travel));

                for (uint i = 0; i < kNumModes; ++i)
                    y += modes[i] * std::sin(kPi * (i + 1) * u);

                xs[p] = u * width;
                ys[p] = y;
            }

            const auto strokePoints = [&](const uint first,
                                          const uint last,
                                          const Color& color1,
                                          const Color& color2,
                                          const float strokeW)
            {
                beginPath();
                moveTo(xs[first], ys[first]);
                for (uint p = first + 1; p <= last; ++p)
                    lineTo(xs[p], ys[p]);
                strokePaint(linearGradient(xs[first], 0, xs[last], 0, color1, color2));
                strokeWidth(strokeW * fScaleFactor);
                stroke();
            };

            if (std::max(front1, front2) > 0.5f)
            {
                for (uint layer = 0; layer < std::size(kGlowWidths); ++layer)
                {
                    const float alpha = kGlowAlphas[layer] * dim;
                    strokePoints(0,
                                 kPointsPerPiece - 1,
                                 rainbow(u1, alpha * front1 * front1),
                                 rainbow(u2, alpha * front2 * front2),
                                 kGlowWidths[layer]);
                }
            }

            // the wire is slightly thinner on the far side of each turn, which makes a plain sine read as a coil
            strokePoints(0,
                         kMiddlePoint,
                         rainbow(u1, dim * front1),
                         rainbow(uMid, dim * frontMid),
                         kThickness * (0.72f + 0.17f * (front1 + frontMid)));
            strokePoints(kMiddlePoint,
                         kPointsPerPiece - 1,
                         rainbow(uMid, dim * frontMid),
                         rainbow(u2, dim * front2),
                         kThickness * (0.72f + 0.17f * (frontMid + front2)));
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

class SpringStageWidget final : public StageWidget<EasyStageWidget, SpringExpertPageWidget>
{
    using BaseWidget = StageWidget<EasyStageWidget, SpringExpertPageWidget>;

    // an empty dB range leaves only the frequency lines
    const std::shared_ptr<LabWidget> fGrid { addGraphWidget<GridWidget>(0.f, 0.f, 1.f, "", 0.5f) };
    const std::shared_ptr<LabWidget> fCoils { addGraphWidget<SpringCoilsWidget>() };

public:
    explicit SpringStageWidget(LabWidget* const parent)
        : BaseWidget(parent) {}

    Size<uint> getGraphSize() const noexcept final
    {
        const LabMetrics<R> m { this };

        const float gh = m.height - (R::border + R::margin * 2 +
            (fLastPage == kPageExpert
                ? SpringLabeledGroupReference::height
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

public:
    SpringReverbUI()
    {
        const std::list<std::shared_ptr<ShaderWidgetInterface>> shaders {
            std::make_shared<ShaderBackgroundWidget>(this, this),
            std::make_shared<BotShaderWidget<SHADER_CLOUDSTARFIELD>>(this, this),
            makeShaderAnalyzer(),
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
