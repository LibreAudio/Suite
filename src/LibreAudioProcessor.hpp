// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// --------------------------------------------------------------------------------------------------------------------
// generic audio processor

struct LibreAudioProcessor {
    virtual ~LibreAudioProcessor() = default;
    [[nodiscard]] virtual float get(int index) const = 0;
    virtual void init(int) = 0;
    virtual void rate(int) = 0;
    virtual void reset() = 0;
    virtual void run(int samples, float** buffers) = 0;
    virtual void set(int index, float value) = 0;
    // optional
    [[nodiscard]] virtual float latency() const { return 0.f; }
};

// --------------------------------------------------------------------------------------------------------------------
