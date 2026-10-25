// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/button.hpp"

#include "../reference.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<const Color& kAccentColor = Reference::Colors::acc1>
class ComboBoxWidget final : public ReferenceButtonWidget<Reference::Widgets::ComboBox<kAccentColor>, kCornerAuto>
{
    using R = Reference::Widgets::ComboBox<kAccentColor>;
    using BaseWidget = ReferenceButtonWidget<R, kCornerAuto>;

public:
    explicit ComboBoxWidget(LabWidget* const parent, const FaustParameterIndex index)
        : BaseWidget(parent, kParametersMainStart + index),
          fParameter(kFaustParameters[index])
    {
        BaseWidget::setName(fParameter.name);

        BaseWidget::updateSize(false);
    }

private:
    const FaustParameter& fParameter;

    void onNanoDisplay() final
    {
        BaseWidget::onNanoDisplay();

        BaseWidget::fillColor(BaseWidget::isEnabled() ? Reference::Colors::ink : Reference::Colors::ink3);
        BaseWidget::fontSize(Reference::Common::fontSize * this->fScaleFactor);
        BaseWidget::textAlign(BaseWidget::ALIGN_CENTER | BaseWidget::ALIGN_MIDDLE);
        BaseWidget::textLetterSpacing(Reference::Common::letterSpacing * this->fScaleFactor);
        BaseWidget::text(BaseWidget::getWidth() * 0.5f, BaseWidget::getHeight() * 0.5f, "combo-box");
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
