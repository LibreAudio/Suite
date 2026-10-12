// Libre Audio Suite
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui/base.hpp"
#include "LibreAudioParameters.hpp"

#include <array>
#include <algorithm>
#include <cmath>

namespace LibreAudio {

// Tail Size controls the room and instrument spacing. The two damping
// frequencies add wall treatment; rotation and hovering stay independent.
class ReverbRoomWidget final : public LabWidget, private IdleCallback
{
    struct Vec3 { float x, y, z; };
    struct Projected { float x, y, z; };

    double fLastTime = 0;
    double fStartTime = 0;
    float fTailSize = 1;
    float fRoomSize = 1;
    float fAbsorption = 0;
    float fDecay = 1.8f;
    float fPredelay = 5;
    float fShape = 0.4f;
    float fDensity = 0.5f;
    float fAngle = 0.6f;
    float fTime = 0;
    float fScale = 1;
    float fCenterX = 0;
    float fCenterY = 0;
    Vec3 fObjectOrigin { 0, 0, 0 };
    float fObjectAngle = 0;

    static constexpr Color kRoomColor { 0.58f, 0.75f, 0.94f };
    static constexpr Color kInstrumentColor { 0.84f, 0.79f, 0.68f };
    static constexpr Color kMetalColor { 0.48f, 0.85f, 0.90f };

public:
    explicit ReverbRoomWidget(LabWidget* const parent)
        : LabWidget(parent)
    {
        fTailSize = fInterface->getParameterValue(kParametersMainStart + kFaustParameterTail_size);
        fAbsorption = absorptionTarget();
        fDecay = fInterface->getParameterValue(kParametersMainStart + kFaustParameterDecay);
        fPredelay = fInterface->getParameterValue(kParametersMainStart + kFaustParameterPredelay);
        fShape = fInterface->getParameterValue(kParametersMainStart + kFaustParameterTail_shape);
        fDensity = fInterface->getParameterValue(kParametersMainStart + kFaustParameterDensity);
        fLastTime = fStartTime = getTime();
        addIdleCallback(this);
    }

private:
    static Vec3 add(Vec3 a, Vec3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
    static Vec3 multiply(Vec3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }

    float absorptionTarget() const
    {
        // Equal contributions on each control's logarithmic frequency scale:
        // both maximum frequencies -> bare walls; both minima -> all panels.
        const auto amount = [this](const FaustParameterIndex id) {
            const auto& p = kFaustParameters[id];
            const float hz = std::clamp(fInterface->getParameterValue(kParametersMainStart + id), p.min, p.max);
            return std::log(p.max / hz) / std::log(p.max / p.min);
        };
        return 0.5f * (amount(kFaustParameterEr_damping) + amount(kFaustParameterHf_damp));
    }

    void idleCallback() final
    {
        const double now = getTime();
        const float dt = std::clamp(float(now - fLastTime), 0.f, 0.1f);
        fLastTime = now;
        if (!isVisible())
            return;
        const float target = fInterface->getParameterValue(kParametersMainStart + kFaustParameterTail_size);
        fTailSize += (1.f - std::exp(-dt / 0.18f)) * (target - fTailSize);
        fAbsorption += (1.f - std::exp(-dt / 0.18f)) * (absorptionTarget() - fAbsorption);
        const float blend = 1.f - std::exp(-dt / 0.18f);
        fDecay += blend * (fInterface->getParameterValue(kParametersMainStart + kFaustParameterDecay) - fDecay);
        fPredelay += blend * (fInterface->getParameterValue(kParametersMainStart + kFaustParameterPredelay) - fPredelay);
        fShape += blend * (fInterface->getParameterValue(kParametersMainStart + kFaustParameterTail_shape) - fShape);
        fDensity += blend * (fInterface->getParameterValue(kParametersMainStart + kFaustParameterDensity) - fDensity);
        fAngle = std::fmod(fAngle + dt * 0.075f, 6.2831853f);
        fTime = float(now - fStartTime);
        repaint();
    }

    Projected project(Vec3 p) const
    {
        const float x = p.x * std::cos(fAngle) + p.z * std::sin(fAngle);
        const float z = -p.x * std::sin(fAngle) + p.z * std::cos(fAngle);
        constexpr float pitch = 0.22f;
        const float y = p.y * std::cos(pitch) - z * std::sin(pitch);
        const float depth = p.y * std::sin(pitch) + z * std::cos(pitch);
        // Keep world dimensions fixed, but shrink distant objects naturally.
        // The camera stays outside the room throughout the Tail Size range.
        constexpr float cameraDistance = 6.f;
        const float perspective = cameraDistance / (cameraDistance - depth);
        return { fCenterX + x * perspective * fScale,
                 fCenterY - y * perspective * fScale, depth };
    }

    Vec3 objectPoint(Vec3 p) const
    {
        const float c = std::cos(fObjectAngle), s = std::sin(fObjectAngle);
        return add(fObjectOrigin, { p.x * c + p.z * s, p.y, -p.x * s + p.z * c });
    }

    void line(Vec3 a, Vec3 b, Color color = kInstrumentColor,
              float alpha = 0.65f, float width = 0.9f)
    {
        const auto pa = project(objectPoint(a)), pb = project(objectPoint(b));
        color.alpha = alpha * std::clamp(0.75f + (pa.z + pb.z) * 0.06f, 0.4f, 1.f);
        beginPath();
        moveTo(pa.x, pa.y);
        lineTo(pb.x, pb.y);
        strokeColor(color);
        strokeWidth(width * fScaleFactor);
        stroke();
    }

    void box(Vec3 center, Vec3 size, Color color = kInstrumentColor,
             float alpha = 0.65f, float width = 0.9f)
    {
        std::array<Vec3, 8> corners;
        for (uint i = 0; i < corners.size(); ++i)
            corners[i] = add(center, { (i & 1 ? 1.f : -1.f) * size.x,
                                      (i & 2 ? 1.f : -1.f) * size.y,
                                      (i & 4 ? 1.f : -1.f) * size.z });
        for (uint i = 0; i < corners.size(); ++i)
            for (uint axis = 1; axis <= 4; axis *= 2)
                if (!(i & axis))
                    line(corners[i], corners[i | axis], color, alpha, width);
    }

    void ring(Vec3 center, Vec3 u, Vec3 v, Color color, float alpha = 0.65f)
    {
        constexpr int segments = 20;
        Vec3 last = add(center, u);
        for (int i = 1; i <= segments; ++i)
        {
            const float a = i * 6.2831853f / segments;
            const Vec3 next = add(center, add(multiply(u, std::cos(a)), multiply(v, std::sin(a))));
            line(last, next, color, alpha, 0.8f);
            last = next;
        }
    }

    // Instrument vertices stay in local world units. Only their floor anchors
    // spread with the room, so changing Tail Size never scales an instrument.
    void place(float x, float z, float angle)
    {
        fObjectOrigin = { x * fRoomSize, -0.75f * fRoomSize, z * fRoomSize };
        fObjectAngle = angle;
    }

    void cylinder(Vec3 center, Vec3 u, Vec3 v, Vec3 axis, Color color = kMetalColor)
    {
        const Vec3 front = add(center, axis), back = add(center, multiply(axis, -1));
        ring(front, u, v, color);
        ring(back, u, v, color, 0.4f);
        for (Vec3 offset : { u, multiply(u, -1), v, multiply(v, -1) })
            line(add(front, offset), add(back, offset), color, 0.45f);
    }

    void tripod(float x, float z, float height, float radius = 0.12f)
    {
        line({ x, 0.04f, z }, { x, height, z }, kMetalColor);
        for (int i = 0; i < 3; ++i)
        {
            const float a = i * 6.2831853f / 3;
            line({ x, 0.15f, z }, { x + radius * std::cos(a), 0, z + radius * std::sin(a) }, kMetalColor);
        }
    }

    void piano()
    {
        place(-0.66f, -0.46f, 0.14f);
        // Upright cabinet, raised music desk, keyboard, pedals, and feet.
        box({ 0, 0.47f, -0.12f }, { 0.40f, 0.35f, 0.10f });
        box({ 0, 0.84f, -0.12f }, { 0.42f, 0.025f, 0.12f });
        box({ 0, 0.42f, 0.04f }, { 0.41f, 0.04f, 0.14f });
        box({ 0, 0.64f, 0 }, { 0.19f, 0.11f, 0.015f }, kInstrumentColor, 0.4f);
        for (int i = 0; i <= 18; ++i)
        {
            const float x = -0.37f + i * 0.74f / 18;
            line({ x, 0.465f, -0.02f }, { x, 0.465f, 0.17f }, kInstrumentColor, 0.55f, 0.65f);
            if (i < 18 && i % 7 != 2 && i % 7 != 6)
                box({ x + 0.016f, 0.478f, 0.025f }, { 0.008f, 0.012f, 0.04f }, kMetalColor, 0.6f, 0.65f);
        }
        for (float x : { -0.35f, 0.35f })
            box({ x, 0.10f, 0.09f }, { 0.045f, 0.10f, 0.055f });
        for (float x : { -0.06f, 0.f, 0.06f })
            line({ x, 0.08f, -0.01f }, { x, 0.035f, 0.17f }, kMetalColor);
    }

    void drumset()
    {
        place(0.59f, -0.42f, -0.20f);
        // Kick, two mounted toms, floor tom and snare.
        cylinder({ 0, 0.22f, 0.12f }, { 0.20f, 0, 0 }, { 0, 0.20f, 0 }, { 0, 0, 0.13f });
        ring({ 0.07f, 0.17f, 0.255f }, { 0.045f, 0, 0 }, { 0, 0.045f, 0 }, kInstrumentColor, 0.5f);
        for (float x : { -0.14f, 0.14f })
        {
            cylinder({ x, 0.53f, 0 }, { 0.105f, 0, 0 }, { 0, 0, 0.105f }, { 0, 0.07f, 0 });
            line({ x, 0.46f, 0 }, { 0, 0.40f, 0.06f }, kMetalColor);
        }
        cylinder({ 0.29f, 0.34f, -0.16f }, { 0.135f, 0, 0 }, { 0, 0, 0.135f }, { 0, 0.11f, 0 });
        cylinder({ -0.27f, 0.38f, 0.14f }, { 0.12f, 0, 0 }, { 0, 0, 0.12f }, { 0, 0.045f, 0 });
        tripod(-0.27f, 0.14f, 0.335f, 0.09f);
        for (float z : { -0.24f, -0.08f })
            line({ 0.37f, 0.24f, z }, { 0.37f, 0, z }, kMetalColor);
        // Two cymbals and a double hi-hat, each on a tripod.
        for (float x : { -0.36f, 0.35f })
        {
            const float z = x < 0 ? -0.20f : 0.22f;
            tripod(x, z, 0.84f);
            ring({ x, 0.82f, z }, { 0.17f, 0.018f, 0 }, { 0, 0, 0.17f }, kInstrumentColor);
            ring({ x, 0.84f, z }, { 0.035f, 0, 0 }, { 0, 0, 0.035f }, kInstrumentColor, 0.45f);
        }
        tripod(-0.38f, 0.30f, 0.57f, 0.09f);
        for (float y : { 0.55f, 0.57f })
            ring({ -0.38f, y, 0.30f }, { 0.10f, 0, 0 }, { 0, 0, 0.10f }, kInstrumentColor);
        box({ 0, 0.025f, 0.35f }, { 0.055f, 0.02f, 0.08f }, kMetalColor, 0.5f);
    }

    void guitarAndAmp()
    {
        place(-0.65f, 0.53f, 0.16f);
        box({ -0.18f, 0.19f, 0 }, { 0.21f, 0.18f, 0.13f });
        box({ -0.18f, 0.19f, 0.135f }, { 0.18f, 0.145f, 0.002f }, kMetalColor, 0.45f);
        for (int i = 0; i < 6; ++i)
            line({ -0.34f + i * 0.065f, 0.06f, 0.14f }, { -0.34f + i * 0.065f, 0.31f, 0.14f }, kMetalColor, 0.22f, 0.65f);
        box({ -0.18f, 0.385f, 0 }, { 0.065f, 0.015f, 0.025f });
        for (float x : { -0.30f, -0.24f, -0.18f, -0.12f, -0.06f })
            ring({ x, 0.345f, 0.135f }, { 0.008f, 0, 0 }, { 0, 0.008f, 0 }, kMetalColor);

        // A double-cutaway guitar body with shallow depth and a fretted neck.
        fObjectAngle = -0.22f;
        constexpr std::array<Vec3, 16> body {{
            { -0.03f, 0.48f, 0 }, { -0.08f, 0.51f, 0 }, { -0.10f, 0.43f, 0 },
            { -0.075f, 0.36f, 0 }, { -0.14f, 0.29f, 0 }, { -0.15f, 0.21f, 0 },
            { -0.11f, 0.13f, 0 }, { -0.04f, 0.10f, 0 }, { 0.05f, 0.10f, 0 },
            { 0.12f, 0.14f, 0 }, { 0.15f, 0.22f, 0 }, { 0.13f, 0.30f, 0 },
            { 0.065f, 0.36f, 0 }, { 0.10f, 0.44f, 0 }, { 0.075f, 0.50f, 0 }, { 0.03f, 0.47f, 0 }
        }};
        const Vec3 front { 0.25f, 0, 0.16f }, back { 0.25f, 0, 0.11f };
        for (uint i = 0; i < body.size(); ++i)
        {
            const auto next = body[(i + 1) % body.size()];
            line(add(front, body[i]), add(front, next));
            line(add(back, body[i]), add(back, next), kInstrumentColor, 0.35f);
            if (i % 3 == 0) line(add(front, body[i]), add(back, body[i]), kInstrumentColor, 0.35f);
        }
        box({ 0.25f, 0.64f, 0.135f }, { 0.025f, 0.20f, 0.02f });
        box({ 0.25f, 0.885f, 0.135f }, { 0.042f, 0.065f, 0.025f });
        for (int i = 0; i < 8; ++i)
            line({ 0.225f, 0.49f + i * 0.043f, 0.16f }, { 0.275f, 0.49f + i * 0.043f, 0.16f }, kMetalColor, 0.45f, 0.65f);
        line({ 0.25f, 0.19f, 0.17f }, { 0.25f, 0.91f, 0.17f }, kMetalColor, 0.5f, 0.65f);
        box({ 0.25f, 0.24f, 0.17f }, { 0.06f, 0.015f, 0.003f }, kMetalColor);
        tripod(0.25f, 0.12f, 0.32f, 0.12f);
    }

    void vocalMicrophone()
    {
        place(0.56f, 0.57f, 0.28f);
        tripod(0, 0, 0.97f, 0.16f);
        ring({ 0, 0.54f, 0 }, { 0.028f, 0, 0 }, { 0, 0, 0.028f }, kMetalColor);
        cylinder({ 0, 1.02f, 0 }, { 0.025f, 0, 0 }, { 0, 0, 0.025f }, { 0, 0.065f, 0 });
        ring({ 0, 1.087f, 0 }, { 0.025f, 0, 0 }, { 0, 0, 0.025f }, kMetalColor, 0.6f);
        // Microphone cable down the stand and along the floor.
        line({ 0.02f, 0.96f, 0 }, { 0.025f, 0.03f, 0.025f }, kMetalColor, 0.3f, 0.65f);
        line({ 0.025f, 0.03f, 0.025f }, { 0.13f, 0.005f, 0.09f }, kMetalColor, 0.3f, 0.65f);
        line({ 0.13f, 0.005f, 0.09f }, { 0.27f, 0.005f, 0.05f }, kMetalColor, 0.3f, 0.65f);
    }

    void transparentBox(Vec3 center, Vec3 halfSize, Color color, float alpha)
    {
        std::array<Projected, 8> corners;
        for (uint i = 0; i < corners.size(); ++i)
            corners[i] = project(objectPoint(add(center,
                { (i & 1 ? 1.f : -1.f) * halfSize.x,
                  (i & 2 ? 1.f : -1.f) * halfSize.y,
                  (i & 4 ? 1.f : -1.f) * halfSize.z })));
        constexpr std::array<std::array<uint, 4>, 6> faces {{
            {{ 0, 4, 6, 2 }}, {{ 1, 3, 7, 5 }},
            {{ 0, 1, 5, 4 }}, {{ 2, 6, 7, 3 }},
            {{ 0, 2, 3, 1 }}, {{ 4, 5, 7, 6 }}
        }};
        std::array<float, 6> depths {};
        std::array<uint, 6> order { 0, 1, 2, 3, 4, 5 };
        for (uint i = 0; i < faces.size(); ++i)
            for (uint corner : faces[i]) depths[i] += corners[corner].z * 0.25f;
        std::sort(order.begin(), order.end(), [&depths](uint a, uint b) { return depths[a] < depths[b]; });
        for (uint i : order)
        {
            const auto& face = faces[i];
            // Different face shades reveal thickness without wire outlines.
            const float shade = i < 2 ? 0.82f : i < 4 ? 1.f : 0.9f;
            beginPath();
            moveTo(corners[face[0]].x, corners[face[0]].y);
            for (uint j = 1; j < face.size(); ++j)
                lineTo(corners[face[j]].x, corners[face[j]].y);
            closePath();
            fillColor(Color(color.red * shade, color.green * shade, color.blue * shade, alpha));
            fill();
        }
    }

    void wallAbsorbers(Vec3 room)
    {
        constexpr int count = 24; // Six panels on each of the four walls.
        constexpr Color color { 0.73f, 0.64f, 0.86f };
        for (int rank = 0; rank < count; ++rank)
        {
            // Alternate walls as coverage grows, fading each panel smoothly.
            float fade = std::clamp(fAbsorption * count - rank, 0.f, 1.f);
            fade = fade * fade * (3.f - 2.f * fade);
            if (fade < 0.001f)
                continue;
            const int wall = rank % 4, slot = rank / 4;
            const float horizontal = (slot % 3 - 1) * 0.62f;
            const float y = (slot / 3 == 0 ? 0.36f : -0.36f) * room.y;
            const float depth = 0.018f;
            const Vec3 center = wall < 2
                ? Vec3 { horizontal * room.x, y, (wall == 0 ? -1.f : 1.f) * (room.z - depth) }
                : Vec3 { (wall == 2 ? -1.f : 1.f) * (room.x - depth), y, horizontal * room.z };
            const Vec3 halfSize = wall < 2
                ? Vec3 { room.x * 0.19f, room.y * 0.23f, depth }
                : Vec3 { depth, room.y * 0.23f, room.z * 0.19f };
            transparentBox(center, halfSize, color, fade * 0.10f);
        }
    }

    void reverbEnvelope(float w, float h)
    {
        // An illustrative amplitude envelope: delayed onset, shaped buildup,
        // then a tail reaching -60 dB after Decay seconds. A fixed logarithmic
        // time axis keeps short pre-delays and long decays visible together.
        // Same seven pastel spectrum stops as the spring-reverb scope.
        constexpr std::array<Color, 7> palette {{
            Color(0xff, 0xbf, 0xcb), Color(0xff, 0xdf, 0xad),
            Color(0xd2, 0xfd, 0xd3), Color(0xbe, 0xf1, 0xff),
            Color(0xc3, 0xd9, 0xff), Color(0xda, 0xc1, 0xf3),
            Color(0xff, 0xdc, 0xf5)
        }};
        constexpr uint samplesPerColor = 100;
        constexpr uint samples = samplesPerColor * (palette.size() - 1);
        std::array<float, samples + 1> upper, lower;
        const float left = 0, span = w;
        const float baseline = h * 0.50f, amplitude = h * 0.31f;
        const float shape = std::clamp(fShape, 0.f, 1.f);
        const float predelay = std::max(0.f, fPredelay) * 0.001f;
        const float decay = std::max(0.2f, fDecay);
        const float buildup = 0.006f + 0.30f * shape * shape;
        const float exponent = 0.35f + 2.65f * shape;
        constexpr float timeSpan = 24.f;
        constexpr float timeDetail = 0.04f;
        const float logSpan = std::log1p(timeSpan / timeDetail);
        for (uint i = 0; i <= samples; ++i)
        {
            const float time = timeDetail * std::expm1(float(i) / samples * logSpan);
            const float elapsed = time - predelay;
            float envelope = 0;
            if (elapsed > 0)
            {
                const float attack = std::pow(std::clamp(elapsed / buildup, 0.f, 1.f), exponent);
                const float tail = std::exp(-6.9077553f * std::max(0.f, elapsed - buildup) / decay);
                envelope = attack * tail;
            }
            // Fixed, irregular lobes suggest audio without jittering every
            // animation frame. The parameter-driven envelope contains them.
            const float phase = float(i);
            const float top = 0.22f + 0.78f * std::abs(0.60f * std::sin(phase * 1.71f)
                                                   + 0.25f * std::sin(phase * 0.63f + 1.f)
                                                   + 0.15f * std::sin(phase * 2.93f + 0.7f));
            const float bottom = 0.22f + 0.78f * std::abs(0.60f * std::sin(phase * 1.53f + 2.f)
                                                      + 0.25f * std::sin(phase * 0.79f)
                                                      + 0.15f * std::sin(phase * 2.61f + 0.3f));
            const float bed = h * 0.004f;
            upper[i] = amplitude * envelope * top + bed * (0.6f + 0.4f * top);
            lower[i] = amplitude * envelope * bottom + bed * (0.6f + 0.4f * bottom);
        }
        const float density = std::clamp(fDensity, 0.f, 1.f);
        if (density > 0.001f)
        {
            // Gaussian smoothing turns the discrete waveform into a diffuse
            // cloud as Density rises. Keep the original at the dry end.
            const auto sourceUpper = upper, sourceLower = lower;
            const float sigma = 0.35f + density * 4.f;
            constexpr int radius = 13;
            std::array<float, radius * 2 + 1> weights;
            float total = 0;
            for (int offset = -radius; offset <= radius; ++offset)
            {
                const float weight = std::exp(-0.5f * offset * offset / (sigma * sigma));
                weights[offset + radius] = weight;
                total += weight;
            }
            for (uint i = 0; i <= samples; ++i)
            {
                float top = 0, bottom = 0;
                for (int offset = -radius; offset <= radius; ++offset)
                {
                    const uint index = std::clamp(int(i) + offset, 0, int(samples));
                    const float weight = weights[offset + radius] / total;
                    top += sourceUpper[index] * weight;
                    bottom += sourceLower[index] * weight;
                }
                upper[i] += density * (top - upper[i]);
                lower[i] += density * (bottom - lower[i]);
            }
        }
        // Six gradient paths keep the spectrum continuous with few draw calls.
        for (uint band = 0; band + 1 < palette.size(); ++band)
        {
            const uint first = band * samplesPerColor, last = first + samplesPerColor;
            const float x0 = left + span * first / samples, x1 = left + span * last / samples;
            // Only translucent fill. Faint expanded silhouettes soften its
            // edges while keeping the room and starfield visible through it.
            for (int layer = 3; layer >= 0; --layer)
            {
                const float spread = 1.f + layer * (0.035f + 0.10f * density);
                Color a = palette[band], b = palette[band + 1];
                a.alpha = b.alpha = layer == 0 ? 0.075f : 0.014f;
                beginPath();
                moveTo(x0, baseline - upper[first] * spread);
                for (uint i = first + 1; i <= last; ++i)
                    lineTo(left + span * i / samples, baseline - upper[i] * spread);
                for (uint i = last; i > first; --i)
                    lineTo(left + span * i / samples, baseline + lower[i] * spread);
                lineTo(x0, baseline + lower[first] * spread);
                closePath();
                fillPaint(linearGradient(x0, baseline, x1, baseline, a, b));
                fill();
            }
        }
    }

    void onNanoDisplay() final
    {
        const float w = getWidth(), h = getHeight();
        if (w < 1 || h < 1)
            return;
        save();
        scissor(0, 0, w, h);
        reverbEnvelope(w, h);

        const auto& p = kFaustParameters[kFaustParameterTail_size];
        // Remap the control range to room scales 0.8 .. 2.0 so the smallest
        // room remains usable. Instrument dimensions stay fixed.
        const float size = (std::clamp(fTailSize, p.min, p.max) - p.min) / (p.max - p.min);
        fRoomSize = 0.8f + size * 1.2f;
        fScale = std::min(w * 0.19f, h * 0.30f);
        fCenterX = w * 0.5f;
        fCenterY = h * 0.35f + std::sin(fTime * 0.42f) * h * 0.013f;

        // Scale the room shell and floor, independently of instrument geometry.
        fObjectOrigin = { 0, 0, 0 };
        fObjectAngle = 0;
        const Vec3 room = multiply({ 1.35f, 0.75f, 1.1f }, fRoomSize);
        for (int i = 1; i < 6; ++i)
        {
            const float t = i / 6.f;
            line({ room.x * (2*t-1), -room.y, -room.z }, { room.x * (2*t-1), -room.y, room.z }, kRoomColor, 0.12f, 0.65f);
            line({ -room.x, -room.y, room.z * (2*t-1) }, { room.x, -room.y, room.z * (2*t-1) }, kRoomColor, 0.12f, 0.65f);
        }
        wallAbsorbers(room);
        piano();
        drumset();
        guitarAndAmp();
        vocalMicrophone();
        fObjectOrigin = { 0, 0, 0 };
        fObjectAngle = 0;
        box({ 0, 0, 0 }, room, kRoomColor, 0.045f, 4.f);
        box({ 0, 0, 0 }, room, kRoomColor, 0.48f, 1.f);
        restore();
    }
};

} // namespace LibreAudio
