// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/base.hpp"
#include "../_lab/metrics.hpp"

#include "../reference.hpp"

#include "LibreAudioParameters.hpp"
#include "LibreAudioUtils.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<class R>
struct GridMetrics : LabMetrics<R> {
    using LabMetrics<R>::LabMetrics;

    const LabMetrics<typename R::Label> label { *this };
};

// --------------------------------------------------------------------------------------------------------------------

class GridWidget final : public LabReferenceWidget<Reference::Widgets::Grid>
{
    using R = Reference::Widgets::Grid;
    using BaseWidget = LabReferenceWidget<R>;

    static_assert(R::width == 0, "grid must have 0 width, size is adjusted by its parent");
    static_assert(R::height == 0, "grid must have 0 height, size is adjusted by its parent");

public:
    explicit GridWidget(LabWidget* const parent, const FaustParameterIndex index, const float step)
        : BaseWidget(parent),
          fMin(kFaustParameters[index].min),
          fMax(kFaustParameters[index].max),
          fStep(step),
          fUsingLog(kFaustParameters[index].isLogarithmic)
    {
        setId(kParametersMainStart + index);
        setName(kFaustParameters[index].name);
    }

private:
    const float fMin;
    const float fMax;
    const float fStep;
    const bool fUsingLog;

    void onNanoDisplay() final
    {
        const GridMetrics<R> m { this };

        drawReferenceBackground<R>();

        // common settings for grid labels
        fillColor(R::Label::color);
        fontSize(m.label.fontSize);
        fontFace("mono");
        strokeColor(R::color);
        strokeWidth(1.f * m.s);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        textLetterSpacing(m.label.letterSpacing);

       #if LIBREAUDIO_WANT_GRAPH_WAVEFORM
        // horizontal line
        for (uint32_t t = 1; t < kNumSecondsForWaveform; ++t)
        {
            const float x = m.border + (m.width - m.border * 2) * (1.f - static_cast<float>(t) / kNumSecondsForWaveform);

            beginPath();
            moveTo(x, m.border);
            lineTo(x, m.height - m.border);
            stroke();
        }
       #endif

        // vertical lines, including labels
        char label[8];
        for (float v = fMin; v < fMax; v += fStep)
        {
            std::snprintf(label, sizeof(label), "%d", d_roundToInt(v));

            const float y = (1.f - (v - fMin) / (fMax - fMin)) * m.height;
            const float labelEnd = text(m.border + m.margin + m.label.border + m.label.margin, y, label, nullptr);
            const float fadeStart = labelEnd + m.label.border + m.label.margin + m.label.padding;
            const float fadeEnd = fadeStart + m.label.padding * m.label.padding;

            beginPath();
            moveTo(fadeStart, y);
            lineTo(m.width - m.border, y);
            strokePaint(linearGradient(fadeStart, y, fadeEnd, y, R::color.withAlpha(0.f), R::color));
            stroke();
        }

        drawReferenceBorder<R>();
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
