// lab: Libre Audio Base-Widgets
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#pragma once

#include "base.hpp"

// --------------------------------------------------------------------------------------------------------------------
// metrics class that calculates and stores all reference sizes, adjusted for scale factor

template<class R, typename RType = float>
struct LabMetrics {
    const float s;
    const RType width;
    const RType height;
    const RType border;
    const RType borderRadius;
    const RType margin;
    const RType padding;
    const float fontSize;
    const float letterSpacing;

    explicit LabMetrics(const LabWidget* const widget)
        : s(widget->getScaleFactor()),
          width(widget->getWidth()),
          height(widget->getHeight()),
          border(d_roundToIntPositive(R::border * s)),
          borderRadius(d_roundToIntPositive(R::borderRadius * s)),
          margin(d_roundToIntPositive(R::margin * s)),
          padding(d_roundToIntPositive(R::padding * s)),
          fontSize(R::fontSize * s),
          letterSpacing(R::letterSpacing * s) {}

    explicit LabMetrics(const LabTopLevelWidget* const widget)
        : s(widget->getScaleFactor()),
          width(widget->getWidth()),
          height(widget->getHeight()),
          border(d_roundToIntPositive(R::border * s)),
          borderRadius(d_roundToIntPositive(R::borderRadius * s)),
          margin(d_roundToIntPositive(R::margin * s)),
          padding(d_roundToIntPositive(R::padding * s)),
          fontSize(R::fontSize * s),
          letterSpacing(R::letterSpacing * s) {}

    template<class R2>
    explicit LabMetrics(const LabMetrics<R2>& m)
        : s(m.s),
          width(d_roundToIntPositive(R::width * s)),
          height(d_roundToIntPositive(R::height * s)),
          border(d_roundToIntPositive(R::border * s)),
          borderRadius(d_roundToIntPositive(R::borderRadius * s)),
          margin(d_roundToIntPositive(R::margin * s)),
          padding(d_roundToIntPositive(R::padding * s)),
          fontSize(R::fontSize * s),
          letterSpacing(R::letterSpacing * s) {}
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
