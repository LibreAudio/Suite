// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/button.hpp"

#include "LibreAudioParameters.hpp"

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<class R, Corner corner = kCornerAuto>
class ReferenceParameterButtonWidget : public LabReferenceButtonWidget<R, corner>
{
    using BaseWidget = LabReferenceButtonWidget<R, corner>;

public:
    explicit ReferenceParameterButtonWidget(LabWidget* const parent, const FaustParameterIndex index)
        : BaseWidget(parent, kParametersMainStart + index),
          fParameter(kFaustParameters[index])
    {
        BaseWidget::setName(fParameter.name);

        if (fParameter.isBoolean)
        {
            DISTRHO_SAFE_ASSERT(d_isZero(fParameter.min));
            DISTRHO_SAFE_ASSERT(d_isNotZero(fParameter.max));

            BaseWidget::setCheckable(true);
            BaseWidget::setChecked(d_isNotZero(fParameter.init), false);
        }
    }

protected:
    const FaustParameter& fParameter;
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
