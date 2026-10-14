// Libre Audio Suite
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace LibreAudio::ParameterValue {

inline float normalized(const float value, const float minimum, const float maximum)
{
    return maximum > minimum ? std::clamp((value - minimum) / (maximum - minimum), 0.f, 1.f) : 0.f;
}

inline int decimalsForStep(const float step)
{
    if (!(step > 0) || !std::isfinite(step))
        return 3;
    double scaled = step;
    for (int decimals = 0; decimals < 9; ++decimals, scaled *= 10)
    {
        const double tolerance = 2 * std::numeric_limits<float>::epsilon() * std::max(1.0, std::abs(scaled));
        if (std::round(scaled) > 0 && std::abs(scaled - std::round(scaled)) <= tolerance)
            return decimals;
    }
    return 9;
}

inline float scrollStep(const float value, const float minimum, const float maximum,
                        const float step, const int direction)
{
    if (!(step > 0) || !std::isfinite(step) || direction == 0)
        return value;
    const double index = std::round((double(value) - minimum) / step);
    const double next = double(minimum) + (index + (direction > 0 ? 1 : -1)) * step;
    return static_cast<float>(std::clamp(next, double(minimum), double(maximum)));
}

} // namespace LibreAudio::ParameterValue
