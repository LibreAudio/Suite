// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "stage.hpp"
#if LIBREAUDIO_WANT_COMMON_IO
#include "../widgets/gain-meter.hpp"
#endif

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<class ReferenceStageWidget = StageWidget<>>
class MainAreaContainerWidget : public ReferenceContainerWidget<Reference::MainArea>
{
    using BaseWidget = ReferenceContainerWidget<Reference::MainArea>;
    using BaseWidget::BaseWidget;

public:
    using StageWidget = ReferenceStageWidget;

    [[nodiscard]] Point<int> getMainAreaAbsolutePos() const noexcept
    {
        return fStage->getAbsolutePos();
    }

    [[nodiscard]] Size<uint> getMainAreaSize() const noexcept
    {
        return fStage->getSize();
    }

    [[nodiscard]] float getMainAreaBorderRadius() const noexcept
    {
        return fStage->getBorderRadius();
    }

    const std::shared_ptr<StageWidget>& getStageWidget() const noexcept
    {
        return fStage;
    }

private:
   #if LIBREAUDIO_WANT_COMMON_IO
    const std::array<std::shared_ptr<LabWidget>, 2> fMeters = {
        addWidget<GainMeterWidget<Input>>(),
        addWidget<GainMeterWidget<Output>>(),
    };
   #endif
    const std::shared_ptr<StageWidget> fStage = addWidget<StageWidget, Expanding>();
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
