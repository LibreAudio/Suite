// Libre Audio Suite
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace LibreAudio::ReverbRoomCamera {

inline constexpr float kPitch = 0.22f;
// A longer camera distance/focal length softens depth exaggeration while
// viewport fitting keeps the room's overall footprint prominent.
inline constexpr float kDistance = 11.5f;

struct Bounds { float left, right, bottom, top; };

inline float distanceForSize(float normalizedSize)
{
    const float size = std::clamp(normalizedSize, 0.f, 1.f);
    return kDistance - 1.f * size * size;
}

// Bounds over a complete rotation, independent of the current angle. The
// corners rotate on a circle in x/z; y remains unchanged before camera tilt.
inline Bounds rotationBounds(float roomSize, float cameraDistance = kDistance)
{
    const float radius = std::hypot(1.35f, 1.1f) * roomSize;
    const float height = 0.75f * roomSize;
    const float sine = std::sin(kPitch), cosine = std::cos(kPitch);
    const float nearest = cameraDistance - height * sine;
    const float maxX = kDistance * radius
        / std::sqrt(nearest * nearest - radius * radius * cosine * cosine);
    Bounds bounds { -maxX, maxX, std::numeric_limits<float>::max(),
                    -std::numeric_limits<float>::max() };
    for (float y : { -height, height })
        for (float z : { -radius, radius })
        {
            const float projectedY = kDistance * (y * cosine - z * sine)
                / (cameraDistance - y * sine - z * cosine);
            bounds.bottom = std::min(bounds.bottom, projectedY);
            bounds.top = std::max(bounds.top, projectedY);
        }
    return bounds;
}

inline float fitScale(float width, float height, float padding, float hover)
{
    const auto bounds = rotationBounds(2.f);
    const float availableWidth = std::max(1.f, width - 2.f * padding);
    const float availableHeight = std::max(1.f, height - 2.f * (padding + hover));
    return std::min(availableWidth / (bounds.right - bounds.left),
                    availableHeight / (bounds.top - bounds.bottom));
}

} // namespace LibreAudio::ReverbRoomCamera
