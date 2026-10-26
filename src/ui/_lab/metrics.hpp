// lab: Libre Audio Base-Widgets
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#pragma once

#include "base.hpp"

// --------------------------------------------------------------------------------------------------------------------
// metrics class that calculates and stores all reference sizes, adjusted for scale factor

template<class R>
struct LabMetrics {
    const float s;
    const float width;
    const float height;
    const float border;
    const float borderRadius;
    const float fontSize;
    const float letterSpacing;
    const float margin;
    const float padding;

    explicit LabMetrics(LabWidget* const widget)
        : s(widget->getScaleFactor()),
          width(/*R::width != 0 ? d_roundToIntPositive(R::width * s) :*/ widget->getWidth()),
          height(/*R::height != 0 ? d_roundToIntPositive(R::height * s) :*/ widget->getHeight()),
          border(d_roundToIntPositive(R::border * s)),
          borderRadius(d_roundToIntPositive(R::borderRadius * s)),
          fontSize(R::fontSize * s),
          letterSpacing(R::letterSpacing * s),
          margin(d_roundToIntPositive(R::margin * s)),
          padding(d_roundToIntPositive(R::padding * s)) {}
};

// --------------------------------------------------------------------------------------------------------------------
// extra classes for specific fields

template<class R, class R2>
struct LabMetricsWidth {
    const float width;

    LabMetricsWidth(const LabMetrics<R>& m)
        : width(R2::width * m.s) {}
};

template<class R, class R2>
struct LabMetricsHeight {
    const float height;

    LabMetricsHeight(const LabMetrics<R>& m)
        : height(R2::height * m.s) {}
};

template<class R, class R2>
struct LabMetricsFontSize {
    const float fontSize;
    const float letterSpacing;

    LabMetricsFontSize(const LabMetrics<R>& m)
        : fontSize(R2::fontSize * m.s),
          letterSpacing(R2::letterSpacing * m.s) {}
};

// --------------------------------------------------------------------------------------------------------------------
