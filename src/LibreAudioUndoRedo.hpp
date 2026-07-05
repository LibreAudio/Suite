// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DistrhoDetails.hpp"

#include <vector>

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

struct LibreAudioUndoRedo {
    struct Callback {
        virtual ~Callback() = default;
        virtual void undoRedoParameterChanged(uint32_t index, float value) = 0;
    };

    struct Parameter {
        uint32_t index; // used for live changes
        float value;
    };

    using Action = std::vector<Parameter>;

    LibreAudioUndoRedo(Callback* const callback);

    bool canUndo() const noexcept;
    bool canRedo() const noexcept;

    void push(const Parameter& param);
    void pushIfFirst(const Parameter& param);

    void undo();
    void redo();

private:
    Callback* const fCallback;
    std::vector<Action> fActions;
    uint32_t fPosition = UINT32_MAX;
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
