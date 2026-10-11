// lab: Libre Audio Base-Widgets
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#pragma once

#include "base.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

template<class R>
class ReferenceTextWidget : public LabReferenceWidget<R>
{
    using BaseWidget = LabReferenceWidget<R>;

public:
    explicit ReferenceTextWidget(LabWidget* const parent, const char* const text)
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
        const uint margin2x = d_roundToUnsignedInt(R::margin * this->fScaleFactor) * 2;

        Rectangle<float> bounds;
        BaseWidget::fontSize(R::fontSize * this->fScaleFactor);
        BaseWidget::textAlign(0);
        BaseWidget::textLetterSpacing(R::letterSpacing * this->fScaleFactor);
        BaseWidget::textBounds(0, 0, fText, nullptr, bounds);

        BaseWidget::setSize(/*bounds.getX() +*/ bounds.getWidth() + margin2x,
                            /*bounds.getY() +*/ bounds.getHeight() + margin2x);

        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

template<class R, const char text[]>
class ReferenceStaticTextWidget : public ReferenceTextWidget<R>
{
    using BaseWidget = ReferenceTextWidget<R>;

public:
    explicit ReferenceStaticTextWidget(LabWidget* const parent)
        : BaseWidget(parent, text) {}
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
