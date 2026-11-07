// lab: Libre Audio Base-Widgets
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#pragma once

#include "base.hpp"
#include "metrics.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

template<class R>
class LabReferenceTextWidget : public LabReferenceWidget<R>
{
    using BaseWidget = LabReferenceWidget<R>;

public:
    explicit LabReferenceTextWidget(LabWidget* const parent, const char* const text)
        : BaseWidget(parent),
          fText(text)
    {
        updateSize(false);
    }

private:
    const char* const fText;

    void onNanoDisplay() final
    {
        BaseWidget::onNanoDisplay();

        BaseWidget::template drawReferenceText<R>(fText);
    }

    void updateSize(const bool updateChildren) final
    {
        const LabMetrics<R, uint> m { this };

        Rectangle<float> bounds;
        BaseWidget::fontSize(m.fontSize);
        BaseWidget::textAlign(BaseWidget::ALIGN_LEFT | BaseWidget::ALIGN_MIDDLE);
        BaseWidget::textLetterSpacing(m.letterSpacing);
        BaseWidget::textBounds(0, 0, fText, nullptr, bounds);

        BaseWidget::setSize(bounds.getWidth() + m.margin * 2, bounds.getHeight() + m.margin * 2);

        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

template<class R, const char text[]>
class LabReferenceStaticTextWidget : public LabReferenceTextWidget<R>
{
    using BaseWidget = LabReferenceTextWidget<R>;

public:
    explicit LabReferenceStaticTextWidget(LabWidget* const parent)
        : BaseWidget(parent, text) {}
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
