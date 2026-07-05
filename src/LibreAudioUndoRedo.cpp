// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LibreAudioUndoRedo.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

LibreAudioUndoRedo::LibreAudioUndoRedo(Callback* const callback)
    : fCallback(callback)
{
}

bool LibreAudioUndoRedo::canUndo() const noexcept
{
    return !fActions.empty() && fPosition != UINT32_MAX && fPosition != 0;
}

bool LibreAudioUndoRedo::canRedo() const noexcept
{
    return !fActions.empty() && fPosition < fActions.size() - 1;
}

void LibreAudioUndoRedo::push(const Parameter& param)
{
    DISTRHO_SAFE_ASSERT_RETURN(fPosition != UINT32_MAX,)

    if (const uint32_t toErase = fActions.size() - fPosition - 1)
        fActions.erase(fActions.cbegin() + toErase, fActions.end());

    const std::vector<Parameter> action = { param };
    fActions.emplace_back(std::move(action));
    fPosition = fActions.size() - 1;
}

void LibreAudioUndoRedo::pushIfFirst(const Parameter& param)
{
    if (fPosition != UINT32_MAX || ! fActions.empty())
        return;

    const std::vector<Parameter> action = { param };
    fActions.emplace_back(std::move(action));
    fPosition = 0;
}

void LibreAudioUndoRedo::undo()
{
    DISTRHO_SAFE_ASSERT_RETURN(canUndo(),);

    --fPosition;

    for (const Parameter& param : fActions[fPosition])
        fCallback->undoRedoParameterChanged(param.index, param.value);
}

void LibreAudioUndoRedo::redo()
{
    DISTRHO_SAFE_ASSERT_RETURN(canRedo(),);

    ++fPosition;

    for (const Parameter& param : fActions[fPosition])
        fCallback->undoRedoParameterChanged(param.index, param.value);
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
