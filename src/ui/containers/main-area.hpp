// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "central-area.hpp"
#if LIBREAUDIO_WANT_COMMON_IO
#include "../widgets/io-meter.hpp"
#endif

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

template<class CentralAreaWidget = CentralAreaWidget<>>
class MainAreaContainerWidget : public LabReferenceContainerWidget<Reference::MainArea, Horizontal>
{
    using BaseWidget = LabReferenceContainerWidget<Reference::MainArea, Horizontal>;
    using BaseWidget::BaseWidget;

public:
    using CentralArea = CentralAreaWidget;

    [[nodiscard]] Point<int> getCentralAreaAbsolutePos() const noexcept
    {
        return fCentralArea->getAbsolutePos();
    }

    [[nodiscard]] Size<uint> getCentralAreaSize() const noexcept
    {
        return fCentralArea->getSize();
    }

    [[nodiscard]] float getCentralAreaBorderRadius() const noexcept
    {
        return fCentralArea->getBorderRadius();
    }

    const std::shared_ptr<CentralAreaWidget>& getCentralAreaWidget() const noexcept
    {
        return fCentralArea;
    }

private:
   #if LIBREAUDIO_WANT_COMMON_IO
    const std::shared_ptr<LabWidget> fInputMeter = addWidget<GainMeterWidget<Input>>();
   #endif
    const std::shared_ptr<CentralAreaWidget> fCentralArea = addWidget<CentralAreaWidget, Expanding>();
   #if LIBREAUDIO_WANT_COMMON_IO
    const std::shared_ptr<LabWidget> fOutputMeter = addWidget<GainMeterWidget<Output>>();
   #endif
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
