// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/base.hpp"
#include "../_lab/metrics.hpp"

#include "../reference.hpp"

#include "LibreAudioParameters.hpp"
#include "LibreAudioUtils.hpp"

// for testing purposes
// #define LIBREAUDIO_GRID_NO_DOTTED_LINE

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<class R>
struct GridMetrics : LabMetrics<R> {
    using LabMetrics<R>::LabMetrics;

    const LabMetrics<typename R::Label> label { *this };

    const float dotPadding = d_roundToIntPositive(3 * this->s);
    const float dotWidth = d_roundToIntPositive(7 * this->s);
};

// --------------------------------------------------------------------------------------------------------------------

class GridWidget final : public LabReferenceWidget<Reference::Widgets::Grid>
{
    using R = Reference::Widgets::Grid;
    using BaseWidget = LabReferenceWidget<R>;

    static_assert(R::width == 0, "grid must have 0 width, size is adjusted by its parent");
    static_assert(R::height == 0, "grid must have 0 height, size is adjusted by its parent");

public:
    explicit GridWidget(LabWidget* const parent,
                        const float min,
                        const float max,
                        const float step,
                        const char* const unit = "")
        : BaseWidget(parent),
          fMin(min),
          fMax(max),
          fStep(step)
    {
        setUnit(unit);
    }

    explicit GridWidget(LabWidget* const parent, const FaustParameterIndex index, const float step)
        : BaseWidget(parent),
          fMin(kFaustParameters[index].min),
          fMax(kFaustParameters[index].max),
          fStep(step)
    {
        setId(kParametersMainStart + index);
        setName(kFaustParameters[index].name);
        setUnit(kFaustParameters[index].unit);
    }

private:
    const float fMin;
    const float fMax;
    const float fStep;
    String fUnitWithSpacePrefix;

    void setUnit(const char* const unit)
    {
        if (*unit == '\0')
            return;

        fUnitWithSpacePrefix = " ";
        fUnitWithSpacePrefix += unit;
    }

    void onNanoDisplay() final
    {
        const GridMetrics<R> m { this };

        drawReferenceBackground<R>();

        char label[16];

        // common settings for grid labels
        fillColor(R::Label::color);
        fontSize(m.label.fontSize);
        fontFace("mono");
        strokeColor(R::color.withAlpha(0.25f));
        strokeWidth(1.f * m.s);
        textLetterSpacing(m.label.letterSpacing);

        // X lines
       #ifdef LIBREAUDIO_PLUGIN__mbComp5
        {
            const float margin = m.border + m.margin;
            const float yfade = m.fontSize + m.label.padding;
            float y;

            textAlign(ALIGN_CENTER | ALIGN_TOP);

            for (int dec = 10; dec < kFreqMax; dec *= 10)
            {
                for (uint8_t s = 0; s < 9; ++s)
                {
                    const int freq = (s + 1) * dec;
                    if (freq <= kFreqMin || freq >= kFreqMax)
                        continue;

                    const float x = freq2norm(freq) * m.width;
                    y = margin;

                    if ((s == 0 || s == 1 || s == 4) && freq >= 50)
                    {
                        if (freq >= 1000)
                            std::snprintf(label, sizeof(label), "%dk", freq / 1000);
                        else
                            std::snprintf(label, sizeof(label), "%d", freq);

                        text(x, y, label);
                        y += m.label.fontSize;
                    }

                    switch (s)
                    {
                    case 0:
                        strokePaint(linearGradient(x,
                                                   y,
                                                   x,
                                                   y + yfade,
                                                   R::color.withAlpha(0.f),
                                                   R::color.withAlpha(.7f)));
                        break;
                    case 4:
                        strokePaint(linearGradient(x,
                                                   y,
                                                   x,
                                                   y + yfade,
                                                   R::color.withAlpha(0.f),
                                                   R::color.withAlpha(.45f)));
                        break;
                    default:
                        strokePaint(linearGradient(x,
                                                   y,
                                                   x,
                                                   y + yfade,
                                                   R::color.withAlpha(0.f),
                                                   R::color.withAlpha(.22f)));
                        break;
                    }

                    beginPath();
                    moveTo(x, y);
                    lineTo(x, m.height - yfade * 0.25f);
                    stroke();

                    switch (s)
                    {
                    case 0:
                        strokePaint(linearGradient(x,
                                                   m.height - yfade * 0.25f,
                                                   x,
                                                   m.height + yfade * 0.75f,
                                                   R::color.withAlpha(.7f),
                                                   R::color.withAlpha(0.f)));
                        break;
                    case 4:
                        strokePaint(linearGradient(x,
                                                   m.height - yfade * 0.25f,
                                                   x,
                                                   m.height + yfade * 0.75f,
                                                   R::color.withAlpha(.45f),
                                                   R::color.withAlpha(0.f)));
                        break;
                    default:
                        strokePaint(linearGradient(x,
                                                   m.height - yfade * 0.25f,
                                                   x,
                                                   m.height + yfade * 0.75f,
                                                   R::color.withAlpha(.22f),
                                                   R::color.withAlpha(0.f)));
                        break;
                    }

                    beginPath();
                    moveTo(x, m.height - yfade * 0.25f);
                    lineTo(x, m.height + yfade * 0.75f);
                    stroke();
                }
            }
        }
       #elif LIBREAUDIO_WANT_GRAPH_WAVEFORM
        for (uint32_t t = 1; t < kNumSecondsForWaveform; ++t)
        {
            const float x = m.border + (m.width - m.border * 2) * (1.f - static_cast<float>(t) / kNumSecondsForWaveform);

            beginPath();
            moveTo(x, m.border);
            lineTo(x, m.height - m.border);
            stroke();
        }
       #endif

        // Y lines, including labels (left-side first)
        bool isFirst = true;
        for (float v = fMin; v < fMax; v += fStep)
        {
            const bool isZero = d_isZero(v);
            if (isFirst)
                std::snprintf(label,
                              sizeof(label),
                              "%c%d%s",
                              v > 0.f ? '+' : '-',
                              std::abs(d_roundToInt(v)),
                              fUnitWithSpacePrefix.buffer());
            else if (isZero)
                std::strncpy(label, "0", sizeof(label));
            else
                std::snprintf(label, sizeof(label), "%c%d", v > 0.f ? '+' : '-', std::abs(d_roundToInt(v)));
            label[sizeof(label) - 1] = '\0';
            isFirst = false;

            const float margin = m.border + m.margin + m.label.border + m.label.margin;
            const float y = (1.f - (v - fMin) / (fMax - fMin)) * m.height;

            textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
            const float sx = text(margin, y, label) + m.label.border + m.label.margin + m.label.padding;
            const float ex = m.width - sx;

            textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
            text(m.width - margin, y, label);

            strokePaint(linearGradient(margin + (sx - margin) * 0.5f,
                                       y,
                                       sx + m.label.padding * m.label.padding,
                                       y,
                                       R::color.withAlpha(0.f),
                                       R::color.withAlpha(isZero ? .5f : .22f)));

           #ifdef LIBREAUDIO_GRID_NO_DOTTED_LINE
            beginPath();
            moveTo(sx, y);
            lineTo(m.width * 0.5f, y);
            stroke();
           #else
            float x = margin;
            for (float middle = m.width * 0.5f; x < middle; x += m.dotWidth)
            {
                beginPath();
                moveTo(x, y);
                lineTo(x + m.dotPadding, y);
                stroke();
            }
           #endif

            strokePaint(linearGradient(ex - m.label.padding * m.label.padding,
                                       y,
                                       m.width - (sx - margin) * 0.5f,
                                       y,
                                       R::color.withAlpha(isZero ? .5f : .22f),
                                       R::color.withAlpha(0.f)));

           #ifdef LIBREAUDIO_GRID_NO_DOTTED_LINE
            beginPath();
            moveTo(m.width * 0.5f, y);
            lineTo(ex, y);
            stroke();
           #else
            for (; x < m.width - margin; x += m.dotWidth)
            {
                beginPath();
                moveTo(x, y);
                lineTo(x + m.dotPadding, y);
                stroke();
            }
           #endif
        }

        drawReferenceBorder<R>();
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
