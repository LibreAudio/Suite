// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/base.hpp"
#include "../_lab/interface.hpp"

#include "../reference.hpp"

#include "LibreAudioParameters.hpp"

#include <algorithm>
#include <cmath>

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------
// An output parameter as a VU style needle, in a box drawn like a number box, with a threshold parameter marked on its
// scale. The pivot sits below the box, so the needle swings across the arc's upper part. The scale runs over the
// meter's range; from the threshold up it is drawn in the threshold's accent colour at half opacity, and the threshold
// itself is a dotted mark across the arc.

template<const Color& kThresholdColor = Reference::Colors::acc1>
struct VuMeterReference : Reference::Zero {
    using Box = Reference::Widgets::NumberBox<>;
    static constexpr const Color scaleColor = Reference::Colors::ink3;
    static constexpr const Color needleColor = Reference::Colors::ink;
    static constexpr const Color thresholdColor = kThresholdColor;
    // pivot below the box's bottom edge, in box heights
    static constexpr const float pivotDrop = 0.9f;
    // room left and right of the arc, and above it
    static constexpr const float margin = 10.f;
    static constexpr const float top = 9.f;
    // widest swing either side of vertical, in degrees
    static constexpr const float maxAngle = 50.f;
    // time constant the needle follows the meter with, in seconds
    static constexpr const double timeConstant = 0.1;
    // threshold mark: how far it reaches inside and outside the arc, and its dots' spacing and radius
    static constexpr const float thresholdInside = 12.f;
    static constexpr const float thresholdOutside = 4.f;
    static constexpr const float thresholdDotGap = 2.5f;
    static constexpr const float thresholdDotRadius = 0.8f;
};

// kHeight, in unscaled pixels, may make the box taller than a number box; the meter keeps its size, centred in it
template<const Color& kThresholdColor = Reference::Colors::acc1, uint kHeight = Reference::Widgets::NumberBox<>::height>
class VuMeterWidget final : public LabWidget,
                            private IdleCallback
{
    using R = VuMeterReference<kThresholdColor>;

    const uint32_t fMeterId;
    const uint32_t fThresholdId;
    const float fMin;
    const float fMax;

    // as drawn
    float fValue;
    float fThreshold;
    double fLastTime = 0;

public:
    explicit VuMeterWidget(LabWidget* const parent, const FaustParameterIndex meter, const FaustParameterIndex threshold)
        : LabWidget(parent),
          fMeterId(kParametersMainStart + meter),
          fThresholdId(kParametersMainStart + threshold),
          fMin(kFaustParameters[meter].min),
          fMax(kFaustParameters[meter].max),
          fValue(kFaustParameters[meter].min),
          fThreshold(kFaustParameters[threshold].init)
    {
        updateSize(false);
        addIdleCallback(this);
    }

private:
    void idleCallback() final
    {
        const double now = getApp().getTime();
        const double dt = fLastTime != 0 ? std::min(now - fLastTime, 0.1) : 0;
        fLastTime = now;

        const float target = fInterface->getParameterValue(fMeterId);
        const float threshold = fInterface->getParameterValue(fThresholdId);
        bool changed = false;

        if (std::abs(target - fValue) < (fMax - fMin) * 0.0005f)
        {
            if (d_isNotEqual(target, fValue))
            {
                fValue = target;
                changed = true;
            }
        }
        else
        {
            fValue += (target - fValue) * static_cast<float>(1.0 - std::exp(-dt / R::timeConstant));
            changed = true;
        }

        if (d_isNotEqual(threshold, fThreshold))
        {
            fThreshold = threshold;
            changed = true;
        }

        if (changed)
            repaint();
    }

    void onNanoDisplay() final
    {
        const float s = fScaleFactor;
        const float w = getWidth();
        const float h = getHeight();

        drawReferenceBackground<typename R::Box>();

        // inset shading and border, as a number box draws them
        beginPath();
        roundedRect(0, 0, w, h, R::Box::borderRadius * s);
        fillPaint(linearGradient(0, 0, 0, R::Box::margin * s,
                                 R::Box::insetShadingColor, Reference::Colors::transparent));
        fill();
        strokeColor(R::Box::backgroundColor);
        strokeWidth(R::Box::border * s * 0.5f);
        stroke();

        // the arc: as wide as the box allows, up to maxAngle either side, laid out for a number box's height and
        // centred in this one
        const float bh = R::Box::height * s;
        const float cx = w * 0.5f;
        const float cy = (h - bh) * 0.5f + bh * (1.f + R::pivotDrop);
        const float radius = bh * (1.f + R::pivotDrop) - R::top * s;
        const float halfWidth = std::max(1.f, cx - R::margin * s);
        const float maxAngle = std::min(R::maxAngle * static_cast<float>(M_PI) / 180.f,
                                        std::asin(std::min(1.f, halfWidth / radius)));

        // the meter's minimum at the left end, its maximum at the right, as an angle from vertical
        const auto angleOf = [=](const float value) {
            return -maxAngle + std::clamp((value - fMin) / (fMax - fMin), 0.f, 1.f) * 2.f * maxAngle;
        };
        const auto pointAt = [=](const float angle, const float r) {
            return Point<float>(cx + std::sin(angle) * r, cy - std::cos(angle) * r);
        };

        save();
        intersectScissor(0, 0, w, h);

        // scale: grey below the threshold, the threshold's colour at half opacity from it up
        const float thresholdAngle = angleOf(fThreshold);
        const auto arc = [&](const float from, const float to, const Color& color) {
            if (to - from < 0.001f)
                return;
            beginPath();
            this->arc(cx, cy, radius, from - static_cast<float>(M_PI) * 0.5f, to - static_cast<float>(M_PI) * 0.5f, CW);
            strokeColor(color);
            strokeWidth(1.2f * s);
            stroke();
        };
        arc(-maxAngle, thresholdAngle, R::scaleColor);
        arc(thresholdAngle, maxAngle, R::thresholdColor.withAlpha(0.5f));

        // ticks at every quarter of the range, coloured as the scale under them
        for (int quarter = 0; quarter <= 4; ++quarter)
        {
            const float angle = -maxAngle + quarter * 0.5f * maxAngle;
            const Point<float> inner = pointAt(angle, radius - 4.f * s);
            const Point<float> outer = pointAt(angle, radius);
            beginPath();
            moveTo(inner.getX(), inner.getY());
            lineTo(outer.getX(), outer.getY());
            strokeColor(angle >= thresholdAngle ? R::thresholdColor.withAlpha(0.5f) : R::scaleColor);
            strokeWidth(1.f * s);
            stroke();
        }

        // the threshold, dotted across the arc
        {
            const float gap = R::thresholdDotGap * s;

            beginPath();
            for (float r = radius - R::thresholdInside * s; r <= radius + R::thresholdOutside * s; r += gap)
            {
                const Point<float> dot = pointAt(thresholdAngle, r);
                circle(dot.getX(), dot.getY(), R::thresholdDotRadius * s);
            }
            fillColor(R::thresholdColor);
            fill();
        }

        // the needle, from the pivot below the box to just past the arc
        {
            const Point<float> tip = pointAt(angleOf(fValue), radius + 3.f * s);
            beginPath();
            moveTo(cx, cy);
            lineTo(tip.getX(), tip.getY());
            lineCap(ROUND);
            strokeColor(R::needleColor);
            strokeWidth(1.5f * s);
            stroke();
        }

        restore();
    }

    // kHeight tall, as wide as its container makes it
    void updateSize(const bool updateChildren) final
    {
        setSize(std::max(1u, getWidth()), d_roundToUnsignedInt(kHeight * fScaleFactor));
        LabWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
