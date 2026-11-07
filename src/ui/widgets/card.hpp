// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/button.hpp"

#include "../reference.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class CardWidget : public LabReferenceButtonWidget<Reference::Widgets::Card, kCornerBoth>
{
    using R = Reference::Widgets::Card;
    using BaseWidget = LabReferenceButtonWidget<R, kCornerBoth>;

public:
    explicit CardWidget(LabWidget* const parent, const uint id, const char* const subtitle)
        : BaseWidget(parent, id),
          fSubtitle(subtitle)
    {
        BaseWidget::updateSize(false);
    }

    explicit CardWidget(LabWidget* const parent, const uint id, const char* const title, const char* const subtitle)
        : BaseWidget(parent, id),
          fSubtitle(subtitle)
    {
        setName(title);
        BaseWidget::updateSize(false);
    }

private:
    const char* const fSubtitle;

    void onNanoDisplay() final
    {
        const float w = BaseWidget::getWidth();
        const float h = BaseWidget::getHeight();
        const bool isChecked = BaseWidget::isChecked();

        if (isChecked)
        {
            const float f = std::min(w, h) * 0.125f;
            const float f2 = f * 2;
            const float f4 = f * 4;

            BaseWidget::beginPath();

            if constexpr (R::borderRadius != 0)
                BaseWidget::roundedRect(-f2, -f2, w + f4, h + f4, R::borderRadius * fScaleFactor);
            else
                BaseWidget::rect(-f2, -f2, w + f4, h + f4);

            BaseWidget::fillPaint(BaseWidget::boxGradient(-f * 0.5f,
                                                          -f * 0.5f,
                                                          w + f,
                                                          h + f,
                                                          R::borderRadius * fScaleFactor * 2.f,
                                                          f2,
                                                          Color(R::glowColor, 0.2f),
                                                          Reference::Colors::transparent));
            BaseWidget::fill();
        }

        if (BaseWidget::isCheckable() && BaseWidget::isChecked())
            drawReferenceBackground<R, R::backgroundColor〡selected, kCornerBoth>();
        else
            drawReferenceBackground<R, R::backgroundColor, kCornerBoth>();

        if (BaseWidget::isHovered())
        {
            BaseWidget::beginPath();

            if constexpr (R::borderRadius != 0)
                BaseWidget::roundedRect(0, 0, w, h, R::borderRadius * fScaleFactor);
            else
                BaseWidget::rect(0, 0, w, h);

            // BaseWidget::fillPaint(BaseWidget::boxGradient(0,
            //                                               0,
            //                                               w + std::min(w, h) * 0.25f,
            //                                               h + std::min(w, h) * 0.25f,
            //                                               R::borderRadius * fScaleFactor,
            //                                               std::min(w, h),
            //                                               Reference::Colors::transparent,
            //                                               R::glowColor));

            BaseWidget::fillPaint(BaseWidget::radialGradient(w, w, std::min(w, h), std::max(w, h) * 2.5f,
                                                             Reference::Colors::transparent, R::glowColor));

            BaseWidget::fill();
        }

        const uint margin = d_roundToUnsignedInt(R::margin * this->fScaleFactor);

        BaseWidget::fillColor(isChecked ? R::Title::color〡selected : R::Title::color);
        BaseWidget::fontSize(R::Title::fontSize * this->fScaleFactor);
        BaseWidget::textAlign(BaseWidget::ALIGN_CENTER | BaseWidget::ALIGN_TOP);
        BaseWidget::textLetterSpacing(R::Title::letterSpacing * this->fScaleFactor);
        BaseWidget::text(w * 0.5f, margin, getName());

        BaseWidget::fillColor(R::Subtitle::color);
        BaseWidget::fontSize(R::Subtitle::fontSize * this->fScaleFactor);
        BaseWidget::textAlign(BaseWidget::ALIGN_CENTER | BaseWidget::ALIGN_MIDDLE);
        BaseWidget::textLetterSpacing(R::Subtitle::letterSpacing * this->fScaleFactor);
        BaseWidget::text(w * 0.5f, h * 0.5f + margin * 0.5f, fSubtitle);

        if (BaseWidget::isCheckable() && BaseWidget::isChecked())
            drawReferenceBorder<R, R::borderColor〡selected>();
        else if (BaseWidget::isHovered())
            drawReferenceBorder<R, R::glowColor>();
        else
            drawReferenceBorder<R, R::borderColor>();
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
