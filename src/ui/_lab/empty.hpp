// lab: Libre Audio Base-Widgets
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#pragma once

#include "base.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------
// empty widget class, useful for making space and alignment of other widgets

class LabEmptyWidget final : public LabWidget
{
    using LabWidget::LabWidget;

private:
    void onNanoDisplay() final
    {
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
