// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/ui.hpp"

// compat fixes
#define StaticContainerPairWidget StaticContainerSizeHintPairWidget
#define LabKnobWidget LabParameterWidget
#define StageWidget CentralAreaWidget
#define ReferenceContainerWidget LabReferenceContainerWidget

#include "ui/bot/deesser-ui.hpp"

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::UI<LibreAudio::DeEsserRootWidget>();
}

END_NAMESPACE_DISTRHO
