// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "NanoVG.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioUIWidgetInterface
{
public:
    enum SnapshotButton {
        kSnapshotButtonCopy,
        kSnapshotButtonA,
        kSnapshotButtonB,
        kSnapshotButtonC,
        kSnapshotButtonD,
    };

    virtual ~LibreAudioUIWidgetInterface() = default;

    virtual uint32_t getParameterCount() const noexcept = 0;
    virtual float getParameterValue(const uint32_t index) const noexcept = 0;

    virtual bool canUndo() const noexcept = 0;
    virtual bool canRedo() const noexcept = 0;
    virtual bool isCopyingSnapshot() const noexcept = 0;
    virtual uint8_t getCurrentSnapshot() const noexcept = 0;

    virtual void undo() = 0;
    virtual void redo() = 0;

    virtual void parameterControlPressed(uint32_t index) = 0;
    virtual void parameterControlReleased(uint32_t index) = 0;
    virtual void parameterControlModified(uint32_t index, float value) = 0;

    virtual void snapshotButtonClicked(SnapshotButton button) = 0;
};

// --------------------------------------------------------------------------------------------------------------------
// base widget class

template <class BaseWidget = NanoSubWidget>
class LibreAudioBaseWidget : public BaseWidget
{
public:
    explicit LibreAudioBaseWidget(LibreAudioBaseWidget<NanoSubWidget>* const parent)
        : BaseWidget(parent),
          fInterface(parent->fInterface),
          fScaleFactor(parent->getTopLevelWidget()->getScaleFactor()) {}

    explicit LibreAudioBaseWidget(LibreAudioBaseWidget<NanoTopLevelWidget>* const parent)
        : BaseWidget(parent),
          fInterface(parent->fInterface),
          fScaleFactor(parent->getScaleFactor()) {}

    explicit LibreAudioBaseWidget(Window& windowToMapTo, LibreAudioUIWidgetInterface* const iface)
        : BaseWidget(windowToMapTo),
          fInterface(iface),
          fScaleFactor(windowToMapTo.getScaleFactor()) {}

    // FIXME make protected
    LibreAudioUIWidgetInterface* const fInterface;

protected:
    float fScaleFactor;
};

using LibreAudioWidget = LibreAudioBaseWidget<NanoSubWidget> ;
using LibreAudioTopLevelWidget = LibreAudioBaseWidget<NanoTopLevelWidget>;

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
