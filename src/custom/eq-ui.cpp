// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/containers/main-area.hpp"
#include "ui/containers/top-bar.hpp"
#include "ui/containers/ui.hpp"

#include "LibreAudioParameters.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <vector>

// --------------------------------------------------------------------------------------------------------------------

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------
// Port of the "Equalizer v8" prototype. The bands live in this UI only for now: nothing here reads or writes the
// eq.dsp parameters yet. The prototype lets bands be added and removed freely, the DSP has a fixed set of eight
// sections, so wiring the two together is a separate step.
//
// Geometry is in prototype px, which map 1:1 to plugin px at scale 1 (the prototype is 910 px wide, like the window).
// Text is drawn kTextScale larger, matching how the other widgets of the suite size their text against the
// prototypes, and whatever is laid out around text grows with it.

static constexpr const float kTextScale = 1.4f;

static constexpr const float kFreqMin = 10.f;
static constexpr const float kFreqMax = 30000.f;
static constexpr const float kGainLimit = 24.f;

// display padding above and below the curve area
static constexpr const float kDisplayPadTop = 24.f;
static constexpr const float kDisplayPadBottom = 40.f;

static constexpr const float kGap = 10.f;
static constexpr const float kPianoHeight = 26.f;
static constexpr const float kPianoBlackHeight = 15.f;
static constexpr const float kBarHeight = 62.f;
static constexpr const float kBoxWidth = 92.f;
static constexpr const float kBoxHeight = 44.f;
static constexpr const float kArrowWidth = 16.f;
static constexpr const float kRowGap = 6.f;
static constexpr const float kIconWidth = 22.f;
static constexpr const float kIconHeight = 14.f;
static constexpr const float kIconGap = 8.f;
static constexpr const float kMenuButtonWidth = 26.f;
static constexpr const float kMenuButtonHeight = 24.f;

// the display is drawn see-through, so the analyser shader underneath shows
static constexpr const float kPanelAlpha = 0.72f;

static constexpr const double kDoubleClickTime = 0.4;
static constexpr const double kAddDebounceTime = 0.45;
static constexpr const double kRemoveGraceTime = 0.6;

// --------------------------------------------------------------------------------------------------------------------

namespace EqColors {
    static constexpr const Color ink = Reference::Colors::ink;
    static constexpr const Color ink2 = Reference::Colors::ink2;
    static constexpr const Color ink3 = Reference::Colors::ink3;
    static constexpr const Color line2 = Reference::Colors::line2;
    static constexpr const Color track = Reference::Colors::track;
    static constexpr const Color off { 0x5d, 0x5d, 0x66 };
    static constexpr const Color nodeOff { 0x57, 0x57, 0x61 };
    static constexpr const Color dotOff { 0x62, 0x62, 0x6b };
    static constexpr const Color barOff { 0x45, 0x45, 0x4d };
    static constexpr const Color dark { 0x16, 0x16, 0x1a };
    static constexpr const Color darker { 0x17, 0x17, 0x1b };
    static constexpr const Color blackKey { 0x13, 0x13, 0x16 };
    static constexpr const Color whiteKey { 0x3a, 0x3b, 0x42 };
    static constexpr const Color panelTop { 0x19, 0x1a, 0x1e };
    static constexpr const Color panelBottom { 0x1e, 0x1f, 0x24 };
    static constexpr const Color fftDark { 0x10, 0x12, 0x17 };
    static constexpr const Color menu { 0x1e, 0x1f, 0x24, 0.96f };
    static constexpr const Color tipTop { 0x23, 0x23, 0x27 };
    static constexpr const Color tipBottom { 0x2a, 0x2a, 0x30 };
    static constexpr const Color tipText2 { 0xa4, 0xa6, 0xad };

    // the Libre Audio rainbow, left to right across the display
    static constexpr const std::array<Color, 7> rainbow {{
        { 0xff, 0xbf, 0xcb }, { 0xff, 0xdf, 0xad }, { 0xd2, 0xfd, 0xd3 }, { 0xbe, 0xf1, 0xff },
        { 0xc3, 0xd9, 0xff }, { 0xda, 0xc1, 0xf3 }, { 0xff, 0xdc, 0xf5 },
    }};

    // band colours, handed out by band number
    static constexpr const std::array<Color, 8> bands {{
        { 0xc3, 0xd9, 0xff }, { 0xda, 0xc1, 0xf3 }, { 0xff, 0xbf, 0xcb }, { 0xff, 0xdf, 0xad },
        { 0xd2, 0xfd, 0xd3 }, { 0xb8, 0xec, 0xff }, { 0xf2, 0xf0, 0xb0 }, { 0xff, 0xdc, 0xf5 },
    }};
}

// --------------------------------------------------------------------------------------------------------------------
// One EQ band, and the analog-prototype magnitude model the display draws it with.

enum class EqBandType : uint8_t { HighPass, LowShelf, Peak, HighShelf, LowPass };
enum class EqChannel : uint8_t { Stereo, Mid, Side };

static constexpr const std::array<EqBandType, 5> kBandTypes {
    EqBandType::HighPass, EqBandType::LowShelf, EqBandType::Peak, EqBandType::HighShelf, EqBandType::LowPass
};

struct EqBand {
    uint32_t id;
    int n;              // the number shown on the node, lowest free one when created
    EqBandType type;
    EqChannel channel;
    bool on;
    float freq, defFreq;
    float gain;
    float q, defQ;
    float adaptiveQ;    // bells only: widens Q as gain rises, 0 = off
    int slope;          // cuts only: 1..4, times 6 dB/oct
    Color color;
    double created;

    // animation state
    float soloAlpha;
    float labelAlpha;

    [[nodiscard]] bool isCut() const noexcept
    {
        return type == EqBandType::HighPass || type == EqBandType::LowPass;
    }

    // a 6 dB/oct cut has no resonance
    [[nodiscard]] bool hasQ() const noexcept
    {
        return !isCut() || slope > 1;
    }
};

struct EqResponse {
    static float freqToNorm(const float f) noexcept
    {
        return std::log10(f / kFreqMin) / std::log10(kFreqMax / kFreqMin);
    }

    static float normToFreq(const float t) noexcept
    {
        return kFreqMin * std::pow(10.f, t * std::log10(kFreqMax / kFreqMin));
    }

    static float qToNorm(const float q) noexcept
    {
        return std::log(q / 0.3f) / std::log(26.7f);
    }

    static float normToQ(const float n) noexcept
    {
        return 0.3f * std::pow(26.7f, n);
    }

    // resonance of a cut, as the dB it adds at the corner
    static float qToDb(const float q) noexcept
    {
        return 20.f * std::log10(q / 0.707f);
    }

    static float bandDb(const float f, const EqBand& b) noexcept
    {
        if (! b.on)
            return 0.f;

        const float o = f / b.freq;

        if (b.isCut())
        {
            const float base = b.type == EqBandType::HighPass
                             ? -10.f * std::log10(1.f + std::pow(1.f / o, 2.f * b.slope))
                             : -10.f * std::log10(1.f + std::pow(o, 2.f * b.slope));
            const float lo = std::log(o) / 0.38f;
            const float res = b.slope <= 1 ? 0.f : qToDb(b.q) * std::exp(-lo * lo);
            return base + res;
        }

        const float q = b.type == EqBandType::Peak
                      ? b.q * (1.f + b.adaptiveQ * (std::abs(b.gain) / 18.f) * 1.6f)
                      : b.q;
        const float a = std::pow(10.f, b.gain / 40.f);
        const float o2 = o * o;
        const float sa = std::sqrt(a);

        switch (b.type)
        {
        case EqBandType::Peak: {
            const float num = (1.f - o2) * (1.f - o2) + std::pow(a * o / q, 2.f);
            const float den = (1.f - o2) * (1.f - o2) + std::pow(o / (a * q), 2.f);
            return 10.f * std::log10(num / den);
        }
        case EqBandType::LowShelf: {
            const float num = a * a * ((a - o2) * (a - o2) + std::pow(sa * o / q, 2.f));
            const float den = (1.f - a * o2) * (1.f - a * o2) + std::pow(sa * o / q, 2.f);
            return 10.f * std::log10(num / den);
        }
        case EqBandType::HighShelf: {
            const float num = a * a * ((1.f - a * o2) * (1.f - a * o2) + std::pow(sa * o / q, 2.f));
            const float den = (a - o2) * (a - o2) + std::pow(sa * o / q, 2.f);
            return 10.f * std::log10(num / den);
        }
        default:
            return 0.f;
        }
    }

    // the summed response of every band
    static float totalDb(const float f, const std::vector<EqBand>& bands) noexcept
    {
        float db = 0.f;
        for (const EqBand& b : bands)
            db += bandDb(f, b);
        return db;
    }

    // the summed response of the bands feeding one half of mid/side, Stereo bands feed both
    static float totalDb(const float f, const std::vector<EqBand>& bands, const EqChannel skip) noexcept
    {
        float db = 0.f;
        for (const EqBand& b : bands)
            if (b.channel != skip)
                db += bandDb(f, b);
        return db;
    }

    // the type a new band gets, by where it lands across the display
    static EqBandType typeAt(const float t) noexcept
    {
        return t < 0.1f ? EqBandType::HighPass
             : t < 0.27f ? EqBandType::LowShelf
             : t < 0.76f ? EqBandType::Peak
             : t < 0.9f ? EqBandType::HighShelf
             : EqBandType::LowPass;
    }

    static const char* typeName(const EqBandType type) noexcept
    {
        switch (type)
        {
        case EqBandType::HighPass: return "High Pass";
        case EqBandType::LowShelf: return "Low Shelf";
        case EqBandType::Peak: return "Bell";
        case EqBandType::HighShelf: return "High Shelf";
        case EqBandType::LowPass: return "Low Pass";
        }
        return "";
    }

    // "95 Hz", "3.20 kHz", "12.0 kHz"
    static void formatFreq(char* const buffer, const size_t size, const float hz, const bool withUnit = true)
    {
        if (hz >= 1000.f)
            std::snprintf(buffer, size, hz >= 10000.f ? "%.1f%s" : "%.2f%s", hz / 1000.f, withUnit ? " kHz" : "");
        else
            std::snprintf(buffer, size, "%d%s", d_roundToInt(hz), withUnit ? " Hz" : "");
    }

    // "+3.0", "−2.5", "±0.0"
    static void formatDb(char* const buffer, const size_t size, const float db)
    {
        const char* const sign = db > 0.05f ? "+" : db < -0.05f ? "\xe2\x88\x92" : "\xc2\xb1";
        std::snprintf(buffer, size, "%s%.1f", sign, std::abs(db));
    }

    static float midiOf(const float f) noexcept
    {
        return 69.f + 12.f * std::log2(f / 440.f);
    }

    static float freqOfMidi(const float m) noexcept
    {
        return 440.f * std::pow(2.f, (m - 69.f) / 12.f);
    }

    static bool isBlackKey(const int m) noexcept
    {
        switch (((m % 12) + 12) % 12)
        {
        case 1: case 3: case 6: case 8: case 10:
            return true;
        }
        return false;
    }
};

// --------------------------------------------------------------------------------------------------------------------

class EqWidget final : public LabReferenceWidget<Reference::Stage>,
                       private IdleCallback
{
    using R = Reference::Stage;
    using BaseWidget = LabReferenceWidget<R>;

public:
    enum class Analyser : uint8_t { Pre, Post, Off };

    explicit EqWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        for (uint32_t i = 0; i < kFaustParameterCount; ++i)
            fParamValues[i] = fInterface->getParameterValue(kParametersMainStart + i);

        addIdleCallback(this);
    }

    // where the curve display sits, for the analyser shader underneath it
    [[nodiscard]] Rectangle<int> getDisplayArea() const noexcept
    {
        const Box d = layoutDisplay();
        return Rectangle<int>(d_roundToInt(d.x), d_roundToInt(d.y), d_roundToInt(d.w), d_roundToInt(d.h));
    }

    [[nodiscard]] float getDisplayBorderRadius() const noexcept
    {
        return 8.f * fScaleFactor;
    }

    [[nodiscard]] bool isAnalyserVisible() const noexcept
    {
        return fAnalyser != Analyser::Off && ! isBypassed();
    }

private:
    struct Box {
        float x = 0.f, y = 0.f, w = 0.f, h = 0.f;

        [[nodiscard]] bool contains(const float px, const float py) const noexcept
        {
            return px >= x && px < x + w && py >= y && py < y + h;
        }
    };

    enum class Field : uint8_t { Gain, Slope, Freq, Q };

    enum class DragKind : uint8_t { None, Node, Label, Box, PianoDot };

    struct Drag {
        DragKind kind = DragKind::None;
        uint32_t id = 0;
        Field field = Field::Gain;
        float x = 0.f, y = 0.f;
        float gain = 0.f, freq = 0.f, q = 0.f;
        int slope = 2;
        float dbMax = 0.f;
    };

    struct LabelHit {
        uint32_t id;
        Field field;
        Box box;
    };

    // click targets, for telling a double click on the same thing from two clicks on different things
    enum Target : uint32_t {
        kTargetNode = 1u << 24,
        kTargetLabel = 2u << 24,
        kTargetPianoDot = 3u << 24,
        kTargetBox = 4u << 24,
    };

    enum Caption : uint8_t {
        kCaptionReset,
        kCaptionControls,
        kCaptionPiano,
        kCaptionAnalyser,
        kCaptionRange,
        kCaptionCount
    };

    static constexpr const std::array<int, 5> kRanges { 3, 6, 12, 24, 0 }; // 0 = auto

    float fParamValues[kFaustParameterCount];

    std::vector<EqBand> fBands;
    uint32_t fBandSeq = 0;
    uint32_t fActive = 0;

    Analyser fAnalyser = Analyser::Post;
    uint8_t fRangeIndex = 4;
    bool fShowPiano = true;
    bool fShowControls = true;
    float fDbView = 3.f;

    // pointer state
    Drag fDrag;
    uint32_t fHoverNode = 0;
    uint32_t fHoverPianoDot = 0;
    uint32_t fPianoTip = 0;
    int fHoverLabel = -1;
    int fHoverCaption = -1;
    float fGhostX = -1.f;
    uint32_t fLastClickTarget = 0;
    double fLastClickTime = 0.0;
    double fLastAddTime = 0.0;
    double fLastIdleTime = 0.0;
    MouseCursor fCursor = kMouseCursorArrow;

    // node context menu
    bool fMenuOpen = false;
    bool fMenuEntered = false;
    uint32_t fMenuBand = 0;
    float fMenuX = 0.f, fMenuY = 0.f;

    // hit areas from the last paint
    std::vector<LabelHit> fLabelHits;
    std::array<Box, kCaptionCount> fCaptionBoxes {};
    std::array<Box, 3> fChannelBoxes {};

    // ----------------------------------------------------------------------------------------------------------------
    // band list

    [[nodiscard]] bool isBypassed() const noexcept
    {
        return fInterface->getParameterValue(kParametersCommonStart + kCommonParameterBypass) > 0.5f;
    }

    EqBand* findBand(const uint32_t id) noexcept
    {
        for (EqBand& b : fBands)
            if (b.id == id)
                return &b;
        return nullptr;
    }

    static void applyTypeDefaults(EqBand& b) noexcept
    {
        if (b.isCut())
        {
            b.slope = 2;
            b.q = b.defQ = 0.707f;
        }
        else
        {
            b.q = b.defQ = b.type == EqBandType::Peak ? 1.f : 0.7f;
        }
    }

    EqBand& addBand(const float freq, const float gain, const EqBandType type)
    {
        int n = 1;
        while (std::any_of(fBands.begin(), fBands.end(), [n](const EqBand& b) { return b.n == n; }))
            ++n;

        EqBand b {};
        b.id = ++fBandSeq;
        b.n = n;
        b.type = type;
        b.channel = EqChannel::Stereo;
        b.on = true;
        b.freq = b.defFreq = freq;
        b.gain = b.isCut() ? 0.f : gain;
        b.slope = 2;
        b.color = EqColors::bands[(n - 1) % EqColors::bands.size()];
        b.created = getTime();
        applyTypeDefaults(b);

        fBands.push_back(b);
        fActive = b.id;
        return fBands.back();
    }

    void removeBand(const uint32_t id)
    {
        fBands.erase(std::remove_if(fBands.begin(), fBands.end(), [id](const EqBand& b) { return b.id == id; }),
                     fBands.end());

        if (fActive == id)
            fActive = 0;
        if (fMenuBand == id)
            fMenuOpen = false;
    }

    void setBandType(EqBand& b, const EqBandType type)
    {
        if (b.type == type)
            return;

        b.type = type;
        applyTypeDefaults(b);
        if (b.isCut())
            b.gain = 0.f;
    }

    // bands in frequency order, as the arrows in the option bar step through them
    [[nodiscard]] std::vector<EqBand*> bandsByFreq()
    {
        std::vector<EqBand*> sorted;
        sorted.reserve(fBands.size());
        for (EqBand& b : fBands)
            sorted.push_back(&b);
        std::stable_sort(sorted.begin(), sorted.end(), [](const EqBand* a, const EqBand* b) { return a->freq < b->freq; });
        return sorted;
    }

    // the band the option bar shows: the selected one, or the lowest if none is
    EqBand* barBand()
    {
        if (fBands.empty())
            return nullptr;
        if (EqBand* const b = findBand(fActive))
            return b;
        return bandsByFreq().front();
    }

    // the range the display should show, from the settings or from the curve
    [[nodiscard]] float targetDbMax() const
    {
        if (const int range = kRanges[fRangeIndex]; range != 0)
            return static_cast<float>(range);

        float peak = 0.f;
        std::vector<EqBand> tonal;
        for (const EqBand& b : fBands)
        {
            if (b.isCut())
                continue;
            tonal.push_back(b);
            if (b.on)
                peak = std::max(peak, std::abs(b.gain));
        }

        for (int i = 0; i <= 200; ++i)
            peak = std::max(peak, std::abs(EqResponse::totalDb(EqResponse::normToFreq(i / 200.f), tonal)));

        for (const float r : { 3.f, 6.f, 12.f })
            if (r >= peak)
                return r;
        return 24.f;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // layout, in widget px

    [[nodiscard]] Box layoutInner() const noexcept
    {
        const float pad = R::padding * fScaleFactor;
        return { pad, pad, getWidth() - pad * 2.f, getHeight() - pad * 2.f };
    }

    [[nodiscard]] Box layoutDisplay() const noexcept
    {
        const float s = fScaleFactor;
        Box d = layoutInner();
        if (fShowPiano)
            d.h -= (kPianoHeight + kGap) * s;
        if (fShowControls)
            d.h -= (kBarHeight + kGap) * s;
        d.h = std::max(d.h, 80.f * s);
        return d;
    }

    [[nodiscard]] Box layoutPiano() const noexcept
    {
        const Box d = layoutDisplay();
        return { d.x, d.y + d.h + kGap * fScaleFactor, d.w, kPianoHeight * fScaleFactor };
    }

    [[nodiscard]] Box layoutBar() const noexcept
    {
        const Box d = fShowPiano ? layoutPiano() : layoutDisplay();
        return { d.x, d.y + d.h + kGap * fScaleFactor, d.w, kBarHeight * fScaleFactor };
    }

    // the row of number boxes in the middle of the option bar: arrow, gain, freq, Q, arrow
    [[nodiscard]] std::array<Box, 5> layoutBarRow() const noexcept
    {
        const float s = fScaleFactor;
        const Box bar = layoutBar();
        const float rowW = (kArrowWidth * 2.f + kBoxWidth * 3.f + kRowGap * 4.f) * s;
        const float y = bar.y + (bar.h - kBoxHeight * s) * 0.5f;
        float x = bar.x + (bar.w - rowW) * 0.5f;

        std::array<Box, 5> row;
        for (int i = 0; i < 5; ++i)
        {
            const float w = (i == 0 || i == 4 ? kArrowWidth : kBoxWidth) * s;
            row[i] = { x, y, w, kBoxHeight * s };
            x += w + kRowGap * s;
        }
        return row;
    }

    // centre of the free space left and right of the box row
    [[nodiscard]] float barSideCentre(const bool right) const noexcept
    {
        const Box bar = layoutBar();
        const float rowW = (kArrowWidth * 2.f + kBoxWidth * 3.f + kRowGap * 4.f) * fScaleFactor;
        const float side = (bar.w - rowW) * 0.25f;
        return right ? bar.x + bar.w - side : bar.x + side;
    }

    [[nodiscard]] std::array<Box, 5> layoutTypeIcons() const noexcept
    {
        const float s = fScaleFactor;
        const Box bar = layoutBar();
        const float groupW = (kIconWidth * 5.f + kIconGap * 4.f) * s;
        const float y = bar.y + (bar.h - kIconHeight * s) * 0.5f;
        float x = barSideCentre(false) - groupW * 0.5f;

        std::array<Box, 5> icons;
        for (Box& icon : icons)
        {
            icon = { x, y, kIconWidth * s, kIconHeight * s };
            x += (kIconWidth + kIconGap) * s;
        }
        return icons;
    }

    [[nodiscard]] Box layoutMenu() const noexcept
    {
        const float s = fScaleFactor;
        const Box d = layoutDisplay();
        const float w = (6.f + kMenuButtonWidth * 5.f + 4.f) * s;
        const float h = (6.f + kMenuButtonHeight * 2.f + 5.f) * s;
        return {
            std::clamp(fMenuX - 34.f * s, d.x + 4.f * s, d.x + d.w - w - 4.f * s),
            std::clamp(fMenuY + 10.f * s, d.y + 4.f * s, d.y + d.h - h - 4.f * s),
            w, h
        };
    }

    // curve-area mapping inside the display
    struct Plot {
        float x, y, w, h;   // display box
        float padT, hUse;
        float dbMax;

        [[nodiscard]] float xOf(const float f) const noexcept
        {
            return x + EqResponse::freqToNorm(f) * w;
        }

        [[nodiscard]] float freqAt(const float px) const noexcept
        {
            return std::clamp(EqResponse::normToFreq(std::clamp((px - x) / w, 0.f, 1.f)), kFreqMin, kFreqMax);
        }

        [[nodiscard]] float yOf(const float db) const noexcept
        {
            return y + padT + (dbMax - std::clamp(db, -dbMax, dbMax)) / (2.f * dbMax) * hUse;
        }

        // the curve itself may run off the bottom, the display clips it
        [[nodiscard]] float yCurve(const float db) const noexcept
        {
            return std::clamp(y + padT + (dbMax - std::min(db, dbMax)) / (2.f * dbMax) * hUse, y + padT, y + h + 400.f);
        }

        [[nodiscard]] float dbAt(const float py) const noexcept
        {
            return dbMax - (py - y - padT) / hUse * 2.f * dbMax;
        }
    };

    [[nodiscard]] Plot plot() const noexcept
    {
        const float s = fScaleFactor;
        const Box d = layoutDisplay();
        const float padT = kDisplayPadTop * s;
        return { d.x, d.y, d.w, d.h, padT, std::max(40.f * s, d.h - padT - kDisplayPadBottom * s), fDbView };
    }

    [[nodiscard]] static Point<float> nodePoint(const EqBand& b, const Plot& p) noexcept
    {
        if (b.isCut())
            return { p.xOf(b.freq), b.hasQ() ? p.yOf(EqResponse::qToDb(b.q)) : p.yOf(0.f) };
        return { p.xOf(b.freq), p.yOf(b.gain) };
    }

    // ----------------------------------------------------------------------------------------------------------------
    // small drawing helpers

    static Color withAlpha(const Color& c, const float a) noexcept
    {
        return Color(c, c.alpha * a);
    }

    static Color rainbowAt(float t) noexcept
    {
        t = std::clamp(t, 0.f, 1.f) * (EqColors::rainbow.size() - 1);
        const size_t i = std::min(static_cast<size_t>(t), EqColors::rainbow.size() - 2);
        return Color(EqColors::rainbow[i], EqColors::rainbow[i + 1], t - i);
    }

    void glowDot(const float x, const float y, const float r, const float glow, const Color& c, const float alpha)
    {
        beginPath();
        circle(x, y, r + glow);
        fillPaint(radialGradient(x, y, r * 0.6f, r + glow, withAlpha(c, alpha), withAlpha(c, 0.f)));
        fill();
    }

    // The five filter-shape icons, from the prototype's 18x12 SVG paths.
    void typeIconPath(const EqBandType type, const float x, const float y, const float sx, const float sy)
    {
        const auto M = [&](const float a, const float b) { moveTo(x + a * sx, y + b * sy); };
        const auto L = [&](const float a, const float b) { lineTo(x + a * sx, y + b * sy); };
        const auto C = [&](const float a, const float b, const float c, const float d, const float e, const float f) {
            bezierTo(x + a * sx, y + b * sy, x + c * sx, y + d * sy, x + e * sx, y + f * sy);
        };

        beginPath();
        switch (type)
        {
        case EqBandType::HighPass:
            M(1.5f, 11.f); C(3.f, 11.f, 3.5f, 4.f, 6.5f, 4.f); L(16.5f, 4.f);
            break;
        case EqBandType::LowShelf:
            M(1.5f, 3.f); L(5.f, 3.f); C(7.5f, 3.f, 7.5f, 9.f, 10.f, 9.f); L(16.5f, 9.f);
            break;
        case EqBandType::Peak:
            M(1.5f, 9.f); L(5.f, 9.f); C(7.f, 9.f, 7.5f, 2.5f, 9.f, 2.5f); C(10.5f, 2.5f, 11.f, 9.f, 13.f, 9.f); L(16.5f, 9.f);
            break;
        case EqBandType::HighShelf:
            M(1.5f, 9.f); L(8.f, 9.f); C(10.5f, 9.f, 10.5f, 3.f, 13.f, 3.f); L(16.5f, 3.f);
            break;
        case EqBandType::LowPass:
            M(1.5f, 4.f); L(11.5f, 4.f); C(14.5f, 4.f, 15.f, 11.f, 16.5f, 11.f);
            break;
        }
    }

    void strokeTypeIcon(const EqBandType type, const Box& box, const Color& color, const float width)
    {
        typeIconPath(type, box.x, box.y, box.w / 18.f, box.h / 12.f);
        lineCap(ROUND);
        lineJoin(ROUND);
        strokeColor(color);
        strokeWidth(width);
        stroke();
    }

    // Strokes a polyline through the rainbow. NanoVG gradients have two stops, so the line goes in six pieces,
    // one per pair of neighbouring colours.
    void strokeRainbow(const std::vector<float>& xs, const std::vector<float>& ys, const float x0, const float w,
                       const float width, const float alpha)
    {
        const int n = static_cast<int>(xs.size());
        const int segs = static_cast<int>(EqColors::rainbow.size()) - 1;

        lineCap(ROUND);
        lineJoin(ROUND);
        strokeWidth(width);

        for (int k = 0; k < segs; ++k)
        {
            const int i0 = std::max(0, (n - 1) * k / segs);
            const int i1 = std::min(n - 1, (n - 1) * (k + 1) / segs + 1);

            beginPath();
            moveTo(xs[i0], ys[i0]);
            for (int i = i0 + 1; i <= i1; ++i)
                lineTo(xs[i], ys[i]);

            strokePaint(linearGradient(x0 + w * k / segs, 0, x0 + w * (k + 1) / segs, 0,
                                       withAlpha(EqColors::rainbow[k], alpha),
                                       withAlpha(EqColors::rainbow[k + 1], alpha)));
            stroke();
        }
    }

    // Fills between two polylines (or down to a flat line when `bottom` is empty) through the rainbow.
    void fillRainbow(const std::vector<float>& xs, const std::vector<float>& top, const std::vector<float>& bottom,
                     const float flatY, const float x0, const float w, const float alpha)
    {
        const int n = static_cast<int>(xs.size());
        const int segs = static_cast<int>(EqColors::rainbow.size()) - 1;

        for (int k = 0; k < segs; ++k)
        {
            const int i0 = (n - 1) * k / segs;
            const int i1 = (n - 1) * (k + 1) / segs;

            beginPath();
            moveTo(xs[i0], top[i0]);
            for (int i = i0 + 1; i <= i1; ++i)
                lineTo(xs[i], top[i]);
            if (bottom.empty())
            {
                lineTo(xs[i1], flatY);
                lineTo(xs[i0], flatY);
            }
            else
            {
                for (int i = i1; i >= i0; --i)
                    lineTo(xs[i], bottom[i]);
            }
            closePath();

            fillPaint(linearGradient(x0 + w * k / segs, 0, x0 + w * (k + 1) / segs, 0,
                                     withAlpha(EqColors::rainbow[k], alpha),
                                     withAlpha(EqColors::rainbow[k + 1], alpha)));
            fill();
        }
    }

    // NanoVG has no dashes: walks the polyline and strokes every other `dash` px, each dash in the rainbow colour
    // under it.
    void strokeDashedRainbow(const std::vector<float>& xs, const std::vector<float>& ys, const float x0, const float w,
                             const float dash, const float width, const float alpha)
    {
        lineCap(BUTT);
        strokeWidth(width);

        float walked = 0.f;
        for (size_t i = 1; i < xs.size(); ++i)
        {
            const float dx = xs[i] - xs[i - 1];
            const float dy = ys[i] - ys[i - 1];
            const float len = std::sqrt(dx * dx + dy * dy);
            if (len <= 0.f)
                continue;

            float t = 0.f;
            while (t < len)
            {
                const float phase = std::fmod(walked + t, dash * 2.f);
                const float step = std::min(len - t, (phase < dash ? dash : dash * 2.f) - phase);

                if (phase < dash)
                {
                    const float ax = xs[i - 1] + dx * t / len, ay = ys[i - 1] + dy * t / len;
                    const float bx = xs[i - 1] + dx * (t + step) / len, by = ys[i - 1] + dy * (t + step) / len;
                    beginPath();
                    moveTo(ax, ay);
                    lineTo(bx, by);
                    strokeColor(withAlpha(rainbowAt(((ax + bx) * 0.5f - x0) / w), alpha));
                    stroke();
                }
                t += step;
            }
            walked += len;
        }
    }

    // text sized like the rest of the suite, see kTextScale
    void setFont(const char* const face, const float protoPx, const float spacingEm = 0.f)
    {
        const float px = protoPx * kTextScale * fScaleFactor;
        fontFace(face);
        fontSize(px);
        textLetterSpacing(spacingEm * px);
    }

    float textWidth(const char* const str)
    {
        Rectangle<float> bounds;
        return textBounds(0, 0, str, nullptr, bounds);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // painting

    void onNanoDisplay() final
    {
        drawReferenceBackground<R, kCornerBoth>();

        const bool on = ! isBypassed();

        drawDisplay(on);
        drawCaptions();

        if (fShowPiano)
            drawPiano(on);

        if (fShowControls)
            drawBar(on);

        if (fMenuOpen)
            drawMenu();

        if (fShowPiano)
            drawPianoTip();

        drawReferenceBorder<R>();
    }

    void drawDisplay(const bool on)
    {
        const float s = fScaleFactor;
        const Plot p = plot();
        const float y0 = p.yOf(0.f);

        // panel
        beginPath();
        roundedRect(p.x, p.y, p.w, p.h, getDisplayBorderRadius());
        fillPaint(linearGradient(0, p.y, 0, p.y + p.h,
                                 withAlpha(EqColors::panelTop, kPanelAlpha), withAlpha(EqColors::panelBottom, kPanelAlpha)));
        fill();

        save();
        scissor(p.x, p.y, p.w, p.h);

        // darkening towards the bottom, where the analyser sits
        {
            const float top = p.y + p.h - 170.f * s;
            const float mid = top + 170.f * s * 0.58f;
            beginPath();
            rect(p.x, top, p.w, mid - top);
            fillPaint(linearGradient(0, top, 0, mid, withAlpha(EqColors::fftDark, 0.f), withAlpha(EqColors::fftDark, 0.55f)));
            fill();
            beginPath();
            rect(p.x, mid, p.w, p.y + p.h - mid);
            fillPaint(linearGradient(0, mid, 0, p.y + p.h, withAlpha(EqColors::fftDark, 0.55f), withAlpha(EqColors::fftDark, 0.86f)));
            fill();
        }

        // frequency grid: 2..9 of every decade faint, 100 / 1k / 10k stronger
        strokeWidth(1.f);
        for (const float dec : { 10.f, 100.f, 1000.f, 10000.f })
        {
            for (int m = 1; m <= 9; ++m)
            {
                const float f = dec * m;
                if (f < kFreqMin || f > kFreqMax || (m == 1 && dec == 10.f))
                    continue;
                const float x = std::round(p.xOf(f)) + 0.5f;
                beginPath();
                moveTo(x, p.y);
                lineTo(x, p.y + p.h);
                strokeColor(withAlpha(EqColors::line2, m == 1 ? 0.5f : 0.22f));
                stroke();
            }
        }

        // alternate 1 dB stripes, faded out with the range
        {
            const int steps = static_cast<int>(std::floor(p.dbMax));
            fillColor(Color(1.f, 1.f, 1.f, 0.028f));
            for (int k = 1; k < steps + 1; k += 2)
            {
                const float lo = static_cast<float>(k), hi = std::min(static_cast<float>(k + 1), p.dbMax);
                for (const float sign : { 1.f, -1.f })
                {
                    const float ya = p.yOf(sign * lo), yb = p.yOf(sign * hi);
                    beginPath();
                    rect(p.x, std::min(ya, yb), p.w, std::abs(yb - ya));
                    fill();
                }
            }
        }

        // 0 dB
        beginPath();
        moveTo(p.x, std::round(y0) + 0.5f);
        lineTo(p.x + p.w, std::round(y0) + 0.5f);
        strokeColor(withAlpha(EqColors::line2, 0.8f));
        stroke();

        // the summed curve, and for mid/side a second one for the side
        const bool split = std::any_of(fBands.begin(), fBands.end(),
                                       [](const EqBand& b) { return b.on && b.channel != EqChannel::Stereo; });
        const int steps = std::max(240, d_roundToInt(p.w / s));

        std::vector<float> xs(steps + 1), ym(steps + 1), ys;
        if (split)
            ys.resize(steps + 1);

        for (int i = 0; i <= steps; ++i)
        {
            const float t = static_cast<float>(i) / steps;
            const float f = EqResponse::normToFreq(t);
            xs[i] = p.x + t * p.w;
            ym[i] = p.yCurve(EqResponse::totalDb(f, fBands, EqChannel::Side));
            if (split)
                ys[i] = p.yCurve(EqResponse::totalDb(f, fBands, EqChannel::Mid));
        }

        if (on)
        {
            fillRainbow(xs, ym, {}, y0, p.x, p.w, 0.18f);
            strokeRainbow(xs, ym, p.x, p.w, 6.f * s, 0.12f);
            strokeRainbow(xs, ym, p.x, p.w, 2.2f * s, 1.f);

            if (split)
            {
                fillRainbow(xs, ym, ys, 0.f, p.x, p.w, 0.09f);
                strokeDashedRainbow(xs, ys, p.x, p.w, 3.f * s, 1.3f * s, 0.85f);
            }
        }
        else
        {
            beginPath();
            moveTo(xs[0], ym[0]);
            for (int i = 1; i <= steps; ++i)
                lineTo(xs[i], ym[i]);
            lineTo(xs[steps], y0);
            lineTo(xs[0], y0);
            closePath();
            fillColor(withAlpha(EqColors::off, 0.1f));
            fill();

            beginPath();
            moveTo(xs[0], ym[0]);
            for (int i = 1; i <= steps; ++i)
                lineTo(xs[i], ym[i]);
            lineJoin(ROUND);
            strokeColor(EqColors::off);
            strokeWidth(2.2f * s);
            stroke();
        }

        // what a click would add here
        if (fGhostX >= 0.f && on && fDrag.kind == DragKind::None)
        {
            const float gx = std::clamp(fGhostX, p.x + 14.f * s, p.x + p.w - 14.f * s);
            const Box icon { gx - 10.8f * s, y0 + 9.f * s, 18.f * 1.2f * s, 12.f * 1.2f * s };
            strokeTypeIcon(EqResponse::typeAt((fGhostX - p.x) / p.w), icon, withAlpha(EqColors::ink2, 0.28f), 1.f * s);
        }

        // the hovered or selected band on its own, faded out around 0 dB where it would sit on the sum
        for (const EqBand& b : fBands)
        {
            if (b.soloAlpha <= 0.001f)
                continue;

            beginPath();
            for (int i = 0; i <= steps; ++i)
            {
                const float y = p.yCurve(EqResponse::bandDb(EqResponse::normToFreq(static_cast<float>(i) / steps), b));
                if (i == 0)
                    moveTo(xs[i], y);
                else
                    lineTo(xs[i], y);
            }

            const float fade = 0.085f * p.h;
            const Color c = withAlpha(b.color, b.soloAlpha);

            lineJoin(ROUND);
            strokeWidth(1.3f * s);

            save();
            intersectScissor(p.x, p.y, p.w, y0 - p.y);
            strokePaint(linearGradient(0, y0 - fade, 0, y0, c, withAlpha(c, 0.f)));
            stroke();
            restore();

            save();
            intersectScissor(p.x, y0, p.w, p.y + p.h - y0);
            strokePaint(linearGradient(0, y0, 0, y0 + fade, withAlpha(c, 0.f), c));
            stroke();
            restore();
        }

        // nodes, enabled ones on top
        fLabelHits.clear();

        std::vector<const EqBand*> order;
        for (const EqBand& b : fBands)
            order.push_back(&b);
        std::stable_sort(order.begin(), order.end(), [on](const EqBand* a, const EqBand* b) {
            return (a->on && on ? 1 : 0) < (b->on && on ? 1 : 0);
        });

        for (const EqBand* const b : order)
            drawNode(*b, p, on);

        restore();
    }

    void drawNode(const EqBand& b, const Plot& p, const bool on)
    {
        const float s = fScaleFactor;
        const Point<float> pt = nodePoint(b, p);
        const float x = pt.getX(), y = pt.getY();
        const bool act = fActive == b.id;
        const bool dim = ! b.on || ! on;

        // ring: whole for stereo, top and bottom arcs for mid, left and right for side
        {
            const float rr = 12.f * s;
            const Color c = dim ? EqColors::nodeOff : withAlpha(b.color, act ? 0.8f : 0.5f);

            strokeColor(c);
            strokeWidth(1.4f * s);
            lineCap(ROUND);

            if (b.channel == EqChannel::Stereo)
            {
                beginPath();
                circle(x, y, rr);
                stroke();
            }
            else
            {
                // prototype angles run clockwise from 12 o'clock, NanoVG's from 3 o'clock
                const float start = b.channel == EqChannel::Mid ? -45.f : 45.f;
                for (const float a : { start, start + 180.f })
                {
                    beginPath();
                    arc(x, y, rr, (a - 90.f) * M_PI / 180.f, (a + 90.f - 90.f) * M_PI / 180.f, CW);
                    stroke();
                }
            }
        }

        // dot, with its glow and number
        {
            const float r = (act ? 8.f : 6.5f) * s;
            if (! dim)
                glowDot(x, y, r, (act ? 10.f : 5.f) * s, b.color, 0.55f);

            beginPath();
            circle(x, y, r);
            fillColor(dim ? EqColors::nodeOff : b.color);
            fill();

            char tag[8];
            std::snprintf(tag, sizeof(tag), "%d", b.n);
            setFont("regular", 8.5f / kTextScale * 1.2f);
            textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
            fillColor(EqColors::dark);
            text(x, y + 0.5f * s, tag, nullptr);
        }

        // values next to the node, each draggable on its own
        if (b.labelAlpha > 0.001f && ! dim)
        {
            char buffer[32];
            struct Row { Field field; char text[32]; } rows[3];
            int count = 0;

            if (b.isCut())
            {
                rows[count].field = Field::Slope;
                std::snprintf(rows[count++].text, 32, "%d dB/oct", b.slope * 6);
            }
            else
            {
                EqResponse::formatDb(buffer, sizeof(buffer), b.gain);
                rows[count].field = Field::Gain;
                std::snprintf(rows[count++].text, 32, "%s dB", buffer);
            }

            rows[count].field = Field::Freq;
            EqResponse::formatFreq(rows[count++].text, 32, b.freq);

            if (b.hasQ())
            {
                rows[count].field = Field::Q;
                std::snprintf(rows[count++].text, 32, "Q %.2f", b.q);
            }

            const float rowH = 12.f * kTextScale * s;
            const bool below = ! b.isCut() && b.gain < 0.f;
            const float lx = std::clamp(x, p.x + 44.f * s, p.x + p.w - 44.f * s);
            const float ly = below ? y + 16.f * s + rowH : y - 16.f * s - (count - 1) * rowH;

            setFont("mono", 10.f);
            textAlign(ALIGN_CENTER | ALIGN_BASELINE);

            for (int i = 0; i < count; ++i)
            {
                const float ry = ly + i * rowH;
                const float tw = textWidth(rows[i].text);
                const bool hot = fHoverLabel >= 0 && fHoverLabel == static_cast<int>(fLabelHits.size());

                fLabelHits.push_back({ b.id, rows[i].field, { lx - tw * 0.5f - 3.f * s, ry - rowH * 0.8f, tw + 6.f * s, rowH } });

                // drop shadow, then the text
                fillColor(Color(0.f, 0.f, 0.f, 0.6f * b.labelAlpha));
                text(lx, ry + 1.f * s, rows[i].text, nullptr);
                fillColor(withAlpha(hot ? Color(1.f, 1.f, 1.f) : b.color, 0.72f * b.labelAlpha));
                text(lx, ry, rows[i].text, nullptr);
            }
        }
    }

    void drawCaptions()
    {
        const float s = fScaleFactor;
        const Box d = layoutDisplay();
        const float bottom = d.y + d.h - 12.f * s;
        const float gap = 22.f * s;

        setFont("mono", 8.5f, 0.06f);

        const auto caption = [this](const float x, const float y, const int align, const char* const str,
                                    const Color& color) {
            textAlign(align);
            fillColor(color);
            text(x, y, str, nullptr);
        };

        // clickable ones, each remembering where it was drawn
        const auto clickable = [&](const Caption which, const float x, const float w, const char* const str,
                                   const bool isOn) {
            const Color c = fHoverCaption == which ? EqColors::ink : isOn ? EqColors::ink2 : EqColors::ink3;
            caption(x, bottom, ALIGN_LEFT | ALIGN_BOTTOM, str, c);
            fCaptionBoxes[which] = { x - 3.f * s, bottom - 14.f * s, w + 6.f * s, 16.f * s };
        };

        {
            const char* const str = "reset";
            clickable(kCaptionReset, d.x + 16.f * s, textWidth(str), str, false);
        }

        {
            const float wc = textWidth("controls"), wp = textWidth("piano");
            const float x = d.x + (d.w - wc - gap - wp) * 0.5f;
            clickable(kCaptionControls, x, wc, "controls", fShowControls);
            clickable(kCaptionPiano, x + wc + gap, wp, "piano", fShowPiano);
        }

        {
            // fixed-width values, so the captions do not shift as they change
            const char* const analyser = fAnalyser == Analyser::Pre ? "analyser pre " :
                                         fAnalyser == Analyser::Post ? "analyser post" : "analyser off ";
            char range[16];
            if (kRanges[fRangeIndex] == 0)
                std::snprintf(range, sizeof(range), "range auto  ");
            else
                std::snprintf(range, sizeof(range), "range %2d dB", kRanges[fRangeIndex]);

            const float wr = textWidth(range), wa = textWidth(analyser);
            const float xr = d.x + d.w - 16.f * s - wr;
            clickable(kCaptionRange, xr, wr, range, false);
            clickable(kCaptionAnalyser, xr - gap - wa, wa, analyser, false);
        }

        // axis labels
        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "\xc2\xb1%d dB", d_roundToInt(targetDbMax()));
        caption(d.x + d.w - 10.f * s, d.y + 27.f * s, ALIGN_RIGHT | ALIGN_TOP, buffer, EqColors::ink3);

        caption(d.x + 10.f * s, d.y + 8.f * s, ALIGN_LEFT | ALIGN_TOP, "10", EqColors::ink3);
        caption(d.x + EqResponse::freqToNorm(100.f) * d.w + 4.f * s, d.y + 8.f * s, ALIGN_LEFT | ALIGN_TOP, "100", EqColors::ink3);
        caption(d.x + EqResponse::freqToNorm(1000.f) * d.w + 4.f * s, d.y + 8.f * s, ALIGN_LEFT | ALIGN_TOP, "1k", EqColors::ink3);
        caption(d.x + EqResponse::freqToNorm(10000.f) * d.w + 4.f * s, d.y + 8.f * s, ALIGN_LEFT | ALIGN_TOP, "10k", EqColors::ink3);
    }

    // x of a note's centre on the keyboard strip
    [[nodiscard]] static float keyX(const Box& k, const float m) noexcept
    {
        return k.x + EqResponse::freqToNorm(EqResponse::freqOfMidi(m)) * k.w;
    }

    void drawPiano(const bool on)
    {
        const float s = fScaleFactor;
        const Box k = layoutPiano();
        const float blackH = kPianoBlackHeight * s;
        const int m0 = static_cast<int>(std::ceil(EqResponse::midiOf(kFreqMin) - 0.5f));
        const int m1 = static_cast<int>(std::floor(EqResponse::midiOf(kFreqMax) + 0.5f));

        beginPath();
        roundedRect(k.x, k.y, k.w, k.h, 6.f * s);
        fillColor(EqColors::whiteKey);
        fill();

        save();
        scissor(k.x, k.y, k.w, k.h);

        const auto lightKey = [&](const EqBand& b, const bool onlyBlack) {
            const int m = d_roundToInt(EqResponse::midiOf(b.freq));
            const bool black = EqResponse::isBlackKey(m);
            if (onlyBlack && ! black)
                return;
            const float half = black ? 0.42f : 0.5f;
            const float xa = keyX(k, m - half), xb = keyX(k, m + half);
            beginPath();
            rect(xa, k.y, xb - xa, black ? blackH : k.h);
            fillColor(withAlpha(b.color, b.id == fActive ? 0.55f : onlyBlack ? 0.3f : 0.28f));
            fill();
        };

        if (on)
            for (const EqBand& b : fBands)
                if (b.on)
                    lightKey(b, false);

        // key separators: every black key, and between E-F and B-C
        strokeWidth(1.f);
        for (int m = m0; m <= m1; ++m)
        {
            const int pc = ((m % 12) + 12) % 12;
            float x;
            Color c(0.f, 0.f, 0.f, 0.45f);
            if (EqResponse::isBlackKey(m))
                x = keyX(k, m);
            else if (pc == 4 || pc == 11)
                x = keyX(k, m + 0.5f), c = Color(0.f, 0.f, 0.f, pc == 11 ? 0.75f : 0.45f);
            else
                continue;
            beginPath();
            moveTo(std::round(x) + 0.5f, k.y);
            lineTo(std::round(x) + 0.5f, k.y + k.h);
            strokeColor(c);
            stroke();
        }

        fillColor(EqColors::blackKey);
        for (int m = m0; m <= m1; ++m)
        {
            if (! EqResponse::isBlackKey(m))
                continue;
            const float xa = keyX(k, m - 0.42f), xb = keyX(k, m + 0.42f);
            beginPath();
            rect(xa, k.y, xb - xa, blackH);
            fill();
        }

        if (on)
            for (const EqBand& b : fBands)
                if (b.on)
                    lightKey(b, true);

        restore();

        // inset edge
        beginPath();
        roundedRect(k.x + 0.5f, k.y + 0.5f, k.w - 1.f, k.h - 1.f, 6.f * s);
        strokeColor(Color(0.f, 0.f, 0.f, 0.45f));
        strokeWidth(1.f);
        stroke();

        // a dot per band, where its frequency really is
        for (const EqBand& b : fBands)
        {
            const bool lit = b.on && on;
            const bool big = b.id == fActive || b.id == fHoverPianoDot;
            const float x = k.x + EqResponse::freqToNorm(b.freq) * k.w;
            const float y = k.y + 20.f * s;
            const float r = 4.f * s * (big ? 1.3f : 1.f);

            if (lit)
                glowDot(x, y, r, 6.f * s, b.color, 0.45f);

            beginPath();
            circle(x, y, r);
            fillColor(lit ? b.color : EqColors::dotOff);
            fill();
            strokeColor(Color(0.f, 0.f, 0.f, 0.45f));
            strokeWidth(1.f * s);
            stroke();
        }
    }

    // note name and frequency above the hovered or dragged piano dot
    void drawPianoTip()
    {
        const EqBand* const b = findBand(fPianoTip);
        if (b == nullptr)
            return;

        static constexpr const char* const kNoteNames[12] = {
            "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
        };

        const float s = fScaleFactor;
        const Box k = layoutPiano();
        const int m = d_roundToInt(EqResponse::midiOf(b->freq));

        char note[8], freq[16];
        std::snprintf(note, sizeof(note), "%s%d", kNoteNames[((m % 12) + 12) % 12], static_cast<int>(std::floor(m / 12.f)) - 1);
        EqResponse::formatFreq(freq, sizeof(freq), b->freq);

        setFont("regular", 12.5f);
        const float w1 = textWidth(note);
        setFont("regular", 12.f);
        const float w2 = textWidth(freq);

        const float lh = 13.f * kTextScale * s;
        const float w = std::max(w1, w2) + 18.f * s;
        const float h = lh * 2.f + 10.f * s;
        const float cx = k.x + EqResponse::freqToNorm(b->freq) * k.w;
        const float x = std::clamp(cx - w * 0.5f, 0.f, getWidth() - w);
        const float y = k.y - 8.f * s - h;

        beginPath();
        roundedRect(x, y + 4.f * s, w, h, 6.f * s);
        fillColor(Color(0.f, 0.f, 0.f, 0.3f));
        fill();

        beginPath();
        roundedRect(x, y, w, h, 6.f * s);
        fillPaint(linearGradient(0, y, 0, y + h, EqColors::tipTop, EqColors::tipBottom));
        fill();

        textAlign(ALIGN_CENTER | ALIGN_TOP);
        setFont("regular", 12.5f);
        fillColor(EqColors::ink);
        text(x + w * 0.5f, y + 5.f * s, note, nullptr);
        setFont("regular", 12.f);
        fillColor(EqColors::tipText2);
        text(x + w * 0.5f, y + 5.f * s + lh, freq, nullptr);
    }

    void drawBar(const bool on)
    {
        const float s = fScaleFactor;
        const Box bar = layoutBar();

        // glass frame
        beginPath();
        roundedRect(bar.x, bar.y, bar.w, bar.h, 10.f * s);
        fillColor(Color(1.f, 1.f, 1.f, 0.05f));
        fill();
        strokeColor(Color(1.f, 1.f, 1.f, 0.05f));
        strokeWidth(1.f);
        stroke();

        beginPath();
        moveTo(bar.x + 10.f * s, bar.y + 0.5f);
        lineTo(bar.x + bar.w - 10.f * s, bar.y + 0.5f);
        strokeColor(Color(1.f, 1.f, 1.f, 0.12f));
        stroke();

        EqBand* const b = barBand();

        if (b == nullptr)
        {
            setFont("regular", 12.f, 0.04f);
            textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
            fillColor(EqColors::ink3);
            text(bar.x + bar.w * 0.5f, bar.y + bar.h * 0.5f, "Click curve to add a band", nullptr);
            return;
        }

        const bool lit = b->on && on;

        // filter shapes on the left
        {
            const std::array<Box, 5> icons = layoutTypeIcons();
            for (size_t i = 0; i < icons.size(); ++i)
            {
                const bool sel = b->type == kBandTypes[i];
                if (sel && lit)
                    glowDot(icons[i].x + icons[i].w * 0.5f, icons[i].y + icons[i].h * 0.5f, 4.f * s, 8.f * s, b->color, 0.25f);
                const Color c = sel ? (lit ? b->color : EqColors::ink2) : withAlpha(EqColors::ink3, 0.7f);
                strokeTypeIcon(kBandTypes[i], icons[i], c, 1.2f * s * 22.f / 18.f);
            }
        }

        // arrows and number boxes in the middle
        {
            const std::array<Box, 5> row = layoutBarRow();
            const bool canStep = fBands.size() > 1;
            char value[32];

            for (const int i : { 0, 4 })
            {
                const Box& a = row[i];
                const float cx = a.x + a.w * 0.5f, cy = a.y + a.h * 0.5f;
                const float dx = (i == 0 ? -1.f : 1.f) * 1.65f * s;
                beginPath();
                moveTo(cx - dx, cy - 4.5f * s);
                lineTo(cx + dx, cy);
                lineTo(cx - dx, cy + 4.5f * s);
                lineCap(ROUND);
                lineJoin(ROUND);
                strokeColor(withAlpha(EqColors::ink3, canStep ? 1.f : 0.35f));
                strokeWidth(1.1f * s);
                stroke();
            }

            if (b->isCut())
            {
                std::snprintf(value, sizeof(value), "%d", b->slope * 6);
                drawNumberBox(row[1], "SLOPE", value, "dB/oct", (b->slope - 1) / 3.f, *b, lit, true);
            }
            else
            {
                EqResponse::formatDb(value, sizeof(value), b->gain);
                drawNumberBox(row[1], "GAIN", value, "dB", (b->gain + kGainLimit) / (2.f * kGainLimit), *b, lit, true);
            }

            EqResponse::formatFreq(value, sizeof(value), b->freq, false);
            drawNumberBox(row[2], "FREQUENCY", value, b->freq >= 1000.f ? "kHz" : "Hz",
                          EqResponse::freqToNorm(b->freq), *b, lit, true);

            if (b->hasQ())
            {
                std::snprintf(value, sizeof(value), "%.2f", b->q);
                drawNumberBox(row[3], "Q", value, nullptr, EqResponse::qToNorm(b->q), *b, lit, true);
            }
            else
            {
                drawNumberBox(row[3], "Q", "\xe2\x80\x93", nullptr, 0.f, *b, lit, false);
            }
        }

        // channel on the right
        {
            static constexpr const char* const kLabels[3] = { "BOTH", "MID", "SIDE" };
            const float gap = 12.f * s;

            setFont("regular", 11.f, 0.06f);
            textAlign(ALIGN_LEFT | ALIGN_MIDDLE);

            float widths[3], total = gap * 2.f;
            for (int i = 0; i < 3; ++i)
                total += widths[i] = textWidth(kLabels[i]);

            float x = barSideCentre(true) - total * 0.5f;
            const float cy = bar.y + bar.h * 0.5f;

            for (int i = 0; i < 3; ++i)
            {
                const bool sel = static_cast<int>(b->channel) == i;
                fillColor(sel ? (lit ? b->color : EqColors::ink2) : withAlpha(EqColors::ink3, 0.7f));
                text(x, cy, kLabels[i], nullptr);
                fChannelBoxes[i] = { x - 3.f * s, cy - 10.f * s, widths[i] + 6.f * s, 20.f * s };
                x += widths[i] + gap;
            }
        }
    }

    void drawNumberBox(const Box& box, const char* const label, const char* const value, const char* const unit,
                       const float fraction, const EqBand& b, const bool lit, const bool enabled)
    {
        const float s = fScaleFactor;
        const float r = 5.f * s;

        beginPath();
        roundedRect(box.x, box.y, box.w, box.h, r);
        fillColor(EqColors::track);
        fill();

        // inset shading
        beginPath();
        roundedRect(box.x, box.y, box.w, box.h, r);
        fillPaint(linearGradient(0, box.y, 0, box.y + 4.f * s, Color(0.f, 0.f, 0.f, 0.55f), Color(0.f, 0.f, 0.f, 0.f)));
        fill();
        strokeColor(Color(0.f, 0.f, 0.f, 0.45f));
        strokeWidth(1.f);
        stroke();

        const float px = box.x + 8.f * s;

        setFont("regular", 7.5f, 0.11f);
        textAlign(ALIGN_LEFT | ALIGN_TOP);
        fillColor(EqColors::ink3);
        text(px, box.y + 5.f * s, label, nullptr);

        setFont("mono", 13.f);
        textAlign(ALIGN_LEFT | ALIGN_BASELINE);
        fillColor(lit && enabled ? b.color : EqColors::off);
        const float base = box.y + box.h - 9.f * s;
        const float vw = text(px, base, value, nullptr) - px;

        if (unit != nullptr)
        {
            setFont("regular", 8.f);
            fillColor(EqColors::ink3);
            text(px + vw + 4.f * s, base, unit, nullptr);
        }

        // bottom bar
        save();
        scissor(box.x, box.y + box.h - 2.f * s, box.w, 2.f * s);
        beginPath();
        roundedRect(box.x, box.y, box.w, box.h, r);
        fillColor(Color(0.f, 0.f, 0.f, 0.5f));
        fill();
        beginPath();
        roundedRect(box.x, box.y, std::max(1.f, std::clamp(fraction, 0.f, 1.f) * box.w), box.h, r);
        fillColor(lit ? b.color : EqColors::barOff);
        fill();
        restore();
    }

    void drawMenu()
    {
        const EqBand* const b = findBand(fMenuBand);
        if (b == nullptr)
            return;

        const float s = fScaleFactor;
        const Box m = layoutMenu();

        beginPath();
        roundedRect(m.x, m.y + 3.f * s, m.w, m.h, 7.f * s);
        fillColor(Color(0.f, 0.f, 0.f, 0.35f));
        fill();

        beginPath();
        roundedRect(m.x, m.y, m.w, m.h, 7.f * s);
        fillColor(EqColors::menu);
        fill();
        strokeColor(Color(0.f, 0.f, 0.f, 0.5f));
        strokeWidth(1.f);
        stroke();

        for (size_t i = 0; i < kBandTypes.size(); ++i)
        {
            const Box cell = menuTypeCell(i);
            const bool sel = b->type == kBandTypes[i];
            if (sel)
            {
                beginPath();
                roundedRect(cell.x, cell.y, cell.w, cell.h, 5.f * s);
                fillColor(b->color);
                fill();
            }
            const Box icon { cell.x + (cell.w - 18.f * s) * 0.5f, cell.y + (cell.h - 12.f * s) * 0.5f, 18.f * s, 12.f * s };
            strokeTypeIcon(kBandTypes[i], icon, sel ? EqColors::darker : EqColors::ink2, 1.3f * s);
        }

        beginPath();
        moveTo(m.x + 7.f * s, m.y + (3.f + kMenuButtonHeight + 2.5f) * s);
        lineTo(m.x + m.w - 7.f * s, m.y + (3.f + kMenuButtonHeight + 2.5f) * s);
        strokeColor(Color(1.f, 1.f, 1.f, 0.08f));
        strokeWidth(1.f);
        stroke();

        static constexpr const char* const kLabels[3] = { "Both", "Mid", "Side" };
        setFont("regular", 11.5f / kTextScale * 1.2f, 0.02f);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);

        for (int i = 0; i < 3; ++i)
        {
            const Box cell = menuChannelCell(i);
            const bool sel = static_cast<int>(b->channel) == i;
            if (sel)
            {
                beginPath();
                roundedRect(cell.x, cell.y, cell.w, cell.h, 5.f * s);
                fillColor(b->color);
                fill();
            }
            fillColor(sel ? EqColors::darker : EqColors::ink2);
            text(cell.x + cell.w * 0.5f, cell.y + cell.h * 0.5f, kLabels[i], nullptr);
        }
    }

    [[nodiscard]] Box menuTypeCell(const size_t i) const noexcept
    {
        const float s = fScaleFactor;
        const Box m = layoutMenu();
        return { m.x + (3.f + i * (kMenuButtonWidth + 1.f)) * s, m.y + 3.f * s, kMenuButtonWidth * s, kMenuButtonHeight * s };
    }

    [[nodiscard]] Box menuChannelCell(const int i) const noexcept
    {
        const float s = fScaleFactor;
        const Box m = layoutMenu();
        const float w = (m.w - 6.f * s - 2.f * s) / 3.f;
        return { m.x + 3.f * s + i * (w + 1.f * s), m.y + (3.f + kMenuButtonHeight + 5.f) * s, w, kMenuButtonHeight * s };
    }

    // ----------------------------------------------------------------------------------------------------------------
    // idle: parameter changes and the small animations

    void idleCallback() override
    {
        bool changed = false;

        for (uint32_t i = 0; i < kFaustParameterCount; ++i)
        {
            if (const float value = fInterface->getParameterValue(kParametersMainStart + i); d_isNotEqual(fParamValues[i], value))
            {
                fParamValues[i] = value;

                // NOTE perhaps recalculate coeffs here?

                changed = true;
            }
        }

        const double now = getTime();
        const float dt = fLastIdleTime > 0.0 ? static_cast<float>(std::min(now - fLastIdleTime, 0.1)) : 0.f;
        fLastIdleTime = now;

        // range follows the curve smoothly, the prototype's 0.16 per frame at 60 fps
        if (const float target = targetDbMax(); d_isNotEqual(fDbView, target))
        {
            const float k = 1.f - std::pow(1.f - 0.16f, dt * 60.f);
            fDbView += (target - fDbView) * k;
            if (std::abs(target - fDbView) < 0.02f)
                fDbView = target;
            changed = true;
        }

        const bool on = ! isBypassed();

        for (EqBand& b : fBands)
        {
            const bool shown = b.on && on && (b.id == fActive || b.id == fHoverNode);
            changed |= approach(b.soloAlpha, shown ? 0.55f : 0.f, 0.55f / 0.5f * dt);
            changed |= approach(b.labelAlpha, shown ? 1.f : 0.f, dt / 0.3f);
        }

        if (changed)
            repaint();
    }

    static bool approach(float& value, const float target, const float step) noexcept
    {
        if (d_isEqual(value, target))
            return false;
        value = value < target ? std::min(target, value + step) : std::max(target, value - step);
        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // pointer

    bool isDoubleClick(const uint32_t target)
    {
        const double now = getTime();
        const bool dbl = target == fLastClickTarget && now - fLastClickTime < kDoubleClickTime;
        fLastClickTarget = dbl ? 0 : target;
        fLastClickTime = now;
        return dbl;
    }

    // the node under the pointer, topmost first
    EqBand* nodeAt(const float x, const float y)
    {
        const Plot p = plot();
        const bool on = ! isBypassed();
        const float r = 16.f * fScaleFactor;

        EqBand* hit = nullptr;
        bool hitLit = false;
        for (EqBand& b : fBands)
        {
            const Point<float> pt = nodePoint(b, p);
            const float dx = pt.getX() - x, dy = pt.getY() - y;
            const bool lit = b.on && on;
            if (dx * dx + dy * dy <= r * r && (hit == nullptr || lit || ! hitLit))
            {
                hit = &b;
                hitLit = lit;
            }
        }
        return hit;
    }

    int labelAt(const float x, const float y) const
    {
        for (size_t i = fLabelHits.size(); i-- > 0;)
            if (fLabelHits[i].box.contains(x, y))
                return static_cast<int>(i);
        return -1;
    }

    EqBand* pianoDotAt(const float x, const float y)
    {
        const Box k = layoutPiano();
        const float r = 7.f * fScaleFactor;
        EqBand* hit = nullptr;
        for (EqBand& b : fBands)
        {
            const float dx = k.x + EqResponse::freqToNorm(b.freq) * k.w - x;
            const float dy = k.y + 20.f * fScaleFactor - y;
            if (dx * dx + dy * dy <= r * r)
                hit = &b;
        }
        return hit;
    }

    void startDrag(const DragKind kind, const EqBand& b, const float x, const float y, const Field field = Field::Gain)
    {
        fDrag.kind = kind;
        fDrag.id = b.id;
        fDrag.field = field;
        fDrag.x = x;
        fDrag.y = y;
        fDrag.gain = b.gain;
        fDrag.freq = b.freq;
        fDrag.q = b.q;
        fDrag.slope = b.slope;
        fDrag.dbMax = kind == DragKind::Box ? targetDbMax() : fDbView;
    }

    // If the range zooms mid-drag, carry on from where the pointer is rather than jump.
    void rebaseDragOnZoom(const EqBand& b, const float y, const float dbMax)
    {
        if (d_isEqual(fDrag.dbMax, dbMax))
            return;
        fDrag.dbMax = dbMax;
        fDrag.y = y;
        fDrag.gain = b.gain;
        fDrag.q = b.q;
    }

    void setCursorIfChanged(const MouseCursor cursor)
    {
        if (fCursor == cursor)
            return;
        fCursor = cursor;
        getWindow().setCursor(cursor);
    }

    bool onMouse(const Widget::MouseEvent& ev) final
    {
        const float x = ev.pos.getX(), y = ev.pos.getY();

        if (! ev.press)
        {
            if (ev.button != kMouseButtonLeft || fDrag.kind == DragKind::None)
                return BaseWidget::onMouse(ev);

            if (fDrag.kind == DragKind::PianoDot && ! pianoDotAt(x, y))
                fPianoTip = 0;
            fDrag = Drag();
            repaint();
            return true;
        }

        if (! contains(ev.pos))
            return BaseWidget::onMouse(ev);

        // any click closes the menu, and a click on it picks from it
        if (fMenuOpen)
        {
            fMenuOpen = false;
            repaint();

            if (layoutMenu().contains(x, y))
            {
                if (EqBand* const b = findBand(fMenuBand))
                {
                    for (size_t i = 0; i < kBandTypes.size(); ++i)
                        if (menuTypeCell(i).contains(x, y))
                            setBandType(*b, kBandTypes[i]);
                    for (int i = 0; i < 3; ++i)
                        if (menuChannelCell(i).contains(x, y))
                            b->channel = static_cast<EqChannel>(i);
                }
                return true;
            }

            if (layoutDisplay().contains(x, y) && ! nodeAt(x, y))
                return true;
        }

        if (ev.button == kMouseButtonRight)
        {
            if (layoutDisplay().contains(x, y))
            {
                if (EqBand* const b = nodeAt(x, y))
                {
                    fActive = b->id;
                    fMenuBand = b->id;
                    fMenuOpen = true;
                    fMenuEntered = false;
                    fMenuX = x;
                    fMenuY = y;
                    repaint();
                    return true;
                }
            }
            return BaseWidget::onMouse(ev);
        }

        if (ev.button != kMouseButtonLeft)
            return BaseWidget::onMouse(ev);

        const bool shift = (ev.mod & kModifierShift) != 0;

        if (pressCaption(x, y) || pressDisplay(x, y, shift) || pressPiano(x, y, shift) || pressBar(x, y))
        {
            repaint();
            return true;
        }

        return BaseWidget::onMouse(ev);
    }

    bool pressCaption(const float x, const float y)
    {
        for (int i = 0; i < kCaptionCount; ++i)
        {
            if (! fCaptionBoxes[i].contains(x, y))
                continue;

            switch (i)
            {
            case kCaptionReset:
                fBands.clear();
                fActive = 0;
                break;
            case kCaptionControls:
                fShowControls = ! fShowControls;
                break;
            case kCaptionPiano:
                fShowPiano = ! fShowPiano;
                break;
            case kCaptionAnalyser:
                fAnalyser = static_cast<Analyser>((static_cast<int>(fAnalyser) + 1) % 3);
                break;
            case kCaptionRange:
                fRangeIndex = (fRangeIndex + 1) % kRanges.size();
                break;
            }
            return true;
        }
        return false;
    }

    bool pressDisplay(const float x, const float y, const bool shift)
    {
        if (! layoutDisplay().contains(x, y))
            return false;

        // a value next to a node
        if (const int li = labelAt(x, y); li >= 0)
        {
            const LabelHit hit = fLabelHits[li];
            if (EqBand* const b = findBand(hit.id))
            {
                fActive = b->id;

                if (isDoubleClick(kTargetLabel | (b->id << 2) | static_cast<uint32_t>(hit.field)))
                {
                    switch (hit.field)
                    {
                    case Field::Gain: b->gain = 0.f; break;
                    case Field::Slope: b->slope = 2; break;
                    case Field::Freq: b->freq = b->defFreq; break;
                    case Field::Q: b->q = b->defQ; break;
                    }
                    return true;
                }

                startDrag(DragKind::Label, *b, x, y, hit.field);
                return true;
            }
        }

        // a node
        if (EqBand* const b = nodeAt(x, y))
        {
            if (shift)
            {
                b->on = ! b->on;
                return true;
            }

            if (isDoubleClick(kTargetNode | b->id))
            {
                if (getTime() - b->created >= kRemoveGraceTime)
                    removeBand(b->id);
                return true;
            }

            fActive = b->id;
            startDrag(DragKind::Node, *b, x, y);
            return true;
        }

        // empty space: a new band here, already being dragged
        if (isBypassed())
            return true;

        const double now = getTime();
        if (now - fLastAddTime < kAddDebounceTime)
            return true;
        fLastAddTime = now;

        const Plot p = plot();
        const EqBandType type = EqResponse::typeAt((x - p.x) / p.w);
        EqBand& b = addBand(p.freqAt(x), std::clamp(p.dbAt(y), -kGainLimit, kGainLimit), type);
        startDrag(DragKind::Node, b, x, y);
        fGhostX = -1.f;
        return true;
    }

    bool pressPiano(const float x, const float y, const bool shift)
    {
        if (! fShowPiano || ! layoutPiano().contains(x, y))
            return false;

        if (EqBand* const b = pianoDotAt(x, y))
        {
            if (shift)
            {
                b->on = ! b->on;
                return true;
            }

            if (isDoubleClick(kTargetPianoDot | b->id))
            {
                removeBand(b->id);
                fPianoTip = 0;
                return true;
            }

            fActive = b->id;
            fPianoTip = b->id;
            startDrag(DragKind::PianoDot, *b, x, y);
            return true;
        }

        if (isBypassed())
            return true;

        const double now = getTime();
        if (now - fLastAddTime < kAddDebounceTime)
            return true;
        fLastAddTime = now;

        const Box k = layoutPiano();
        const float t = std::clamp((x - k.x) / k.w, 0.f, 1.f);
        const float f = EqResponse::normToFreq(t);
        const float snapped = std::clamp(EqResponse::freqOfMidi(std::round(EqResponse::midiOf(f))), kFreqMin, kFreqMax);
        EqBand& b = addBand(snapped, 0.f, EqResponse::typeAt(t));
        fPianoTip = b.id;
        startDrag(DragKind::PianoDot, b, x, y);
        return true;
    }

    bool pressBar(const float x, const float y)
    {
        if (! fShowControls || ! layoutBar().contains(x, y))
            return false;

        EqBand* const b = barBand();
        if (b == nullptr)
            return true;

        const std::array<Box, 5> icons = layoutTypeIcons();
        for (size_t i = 0; i < icons.size(); ++i)
        {
            if (icons[i].contains(x, y))
            {
                setBandType(*b, kBandTypes[i]);
                return true;
            }
        }

        for (int i = 0; i < 3; ++i)
        {
            if (fChannelBoxes[i].contains(x, y))
            {
                b->channel = static_cast<EqChannel>(i);
                return true;
            }
        }

        const std::array<Box, 5> row = layoutBarRow();

        for (const int i : { 0, 4 })
        {
            if (row[i].contains(x, y) && fBands.size() > 1)
            {
                const std::vector<EqBand*> sorted = bandsByFreq();
                const int n = static_cast<int>(sorted.size());
                const int idx = static_cast<int>(std::find(sorted.begin(), sorted.end(), b) - sorted.begin());
                fActive = sorted[(idx + (i == 0 ? -1 : 1) + n) % n]->id;
                return true;
            }
        }

        static constexpr const Field kBoxFields[3] = { Field::Gain, Field::Freq, Field::Q };

        for (int i = 1; i <= 3; ++i)
        {
            if (! row[i].contains(x, y))
                continue;

            Field field = kBoxFields[i - 1];
            if (field == Field::Gain && b->isCut())
                field = Field::Slope;
            if (field == Field::Q && ! b->hasQ())
                return true;

            if (isDoubleClick(kTargetBox | (b->id << 2) | static_cast<uint32_t>(field)))
            {
                switch (field)
                {
                case Field::Gain: b->gain = 0.f; break;
                case Field::Slope: b->slope = 2; break;
                case Field::Freq: b->freq = b->defFreq; break;
                case Field::Q: b->q = b->defQ; break;
                }
                return true;
            }

            startDrag(DragKind::Box, *b, x, y, field);
            return true;
        }

        return true;
    }

    bool onMotion(const Widget::MotionEvent& ev) final
    {
        const float x = ev.pos.getX(), y = ev.pos.getY();

        if (fDrag.kind != DragKind::None)
        {
            if (EqBand* const b = findBand(fDrag.id))
                dragTo(*b, x, y);
            repaint();
            return true;
        }

        updateHover(x, y);
        return BaseWidget::onMotion(ev);
    }

    void dragTo(EqBand& b, const float x, const float y)
    {
        const float s = fScaleFactor;
        const Plot p = plot();
        const float dx = x - fDrag.x;

        switch (fDrag.kind)
        {
        case DragKind::None:
            break;

        // the node itself: frequency across, gain (or a cut's resonance) up and down
        case DragKind::Node:
            rebaseDragOnZoom(b, y, p.dbMax);
            b.freq = p.freqAt(x);
            if (! b.isCut())
            {
                b.gain = std::clamp(fDrag.gain + (fDrag.y - y) / p.hUse * 2.f * p.dbMax, -kGainLimit, kGainLimit);
            }
            else if (b.hasQ())
            {
                const float db = EqResponse::qToDb(fDrag.q) + (fDrag.y - y) / p.hUse * 2.f * p.dbMax;
                b.q = std::clamp(0.707f * std::pow(10.f, db / 20.f), EqResponse::normToQ(0.f), EqResponse::normToQ(1.f));
            }
            break;

        // one value next to a node
        case DragKind::Label:
            rebaseDragOnZoom(b, y, p.dbMax);
            switch (fDrag.field)
            {
            case Field::Slope:
                b.slope = std::clamp(d_roundToInt(fDrag.slope + (fDrag.y - y) / s / 30.f), 1, 4);
                break;
            case Field::Gain:
                b.gain = std::clamp(fDrag.gain + (fDrag.y - y) / p.hUse * 2.f * p.dbMax, -kGainLimit, kGainLimit);
                break;
            case Field::Freq:
                b.freq = std::clamp(EqResponse::normToFreq(std::clamp(EqResponse::freqToNorm(fDrag.freq) + dx / p.w, 0.f, 1.f)),
                                    kFreqMin, kFreqMax);
                break;
            case Field::Q:
                b.q = EqResponse::normToQ(std::clamp(EqResponse::qToNorm(fDrag.q) + (fDrag.y - y) / p.h, 0.f, 1.f));
                break;
            }
            break;

        // a number box in the option bar, at fixed prototype-px rates
        case DragKind::Box: {
            const float dbMax = targetDbMax();
            rebaseDragOnZoom(b, y, dbMax);
            const float dy = (fDrag.y - y) / s;
            switch (fDrag.field)
            {
            case Field::Slope:
                b.slope = std::clamp(d_roundToInt(fDrag.slope + dy / 30.f), 1, 4);
                break;
            case Field::Gain:
                b.gain = std::clamp(fDrag.gain + dy / 260.f * 2.f * dbMax, -kGainLimit, kGainLimit);
                break;
            case Field::Freq:
                b.freq = std::clamp(EqResponse::normToFreq(std::clamp(EqResponse::freqToNorm(fDrag.freq) + dx / s / 420.f, 0.f, 1.f)),
                                    kFreqMin, kFreqMax);
                break;
            case Field::Q:
                b.q = EqResponse::normToQ(std::clamp(EqResponse::qToNorm(fDrag.q) + dy / 420.f, 0.f, 1.f));
                break;
            }
            break;
        }

        // a piano dot: frequency snaps to notes
        case DragKind::PianoDot: {
            const Box k = layoutPiano();
            const float f = EqResponse::normToFreq(std::clamp((x - k.x) / k.w, 0.f, 1.f));
            b.freq = std::clamp(EqResponse::freqOfMidi(std::round(EqResponse::midiOf(f))), kFreqMin, kFreqMax);
            const float dy = (fDrag.y - y) / s;
            if (! b.isCut())
                b.gain = std::clamp(fDrag.gain + dy * 0.1f, -kGainLimit, kGainLimit);
            else if (b.hasQ())
                b.q = EqResponse::normToQ(std::clamp(EqResponse::qToNorm(fDrag.q) + dy / 300.f, 0.f, 1.f));
            break;
        }
        }
    }

    void updateHover(const float x, const float y)
    {
        const uint32_t prevNode = fHoverNode, prevDot = fHoverPianoDot, prevTip = fPianoTip;
        const int prevLabel = fHoverLabel, prevCaption = fHoverCaption;
        const float prevGhost = fGhostX;
        const bool prevMenu = fMenuOpen;

        fHoverNode = 0;
        fHoverPianoDot = 0;
        fHoverLabel = -1;
        fHoverCaption = -1;
        fGhostX = -1.f;
        MouseCursor cursor = kMouseCursorArrow;

        if (fMenuOpen)
        {
            // the menu closes once the pointer has been in it and leaves
            const bool inside = layoutMenu().contains(x, y);
            if (inside)
                fMenuEntered = true;
            else if (fMenuEntered)
                fMenuOpen = false;
            if (inside)
                cursor = kMouseCursorHand;
        }

        for (int i = 0; i < kCaptionCount; ++i)
            if (fCaptionBoxes[i].contains(x, y))
                fHoverCaption = i;

        if (fHoverCaption >= 0)
        {
            cursor = kMouseCursorHand;
        }
        else if (layoutDisplay().contains(x, y))
        {
            if (const int li = labelAt(x, y); li >= 0)
            {
                fHoverLabel = li;
                fHoverNode = fLabelHits[li].id;
                cursor = fLabelHits[li].field == Field::Freq ? kMouseCursorLeftRight : kMouseCursorUpDown;
            }
            else if (const EqBand* const b = nodeAt(x, y))
            {
                fHoverNode = b->id;
                cursor = kMouseCursorHand;
            }
            else if (! fMenuOpen || ! layoutMenu().contains(x, y))
            {
                fGhostX = x;
            }
        }
        else if (fShowPiano && layoutPiano().contains(x, y))
        {
            if (const EqBand* const b = pianoDotAt(x, y))
            {
                fHoverPianoDot = b->id;
                cursor = kMouseCursorHand;
            }
        }
        else if (fShowControls && layoutBar().contains(x, y) && ! fBands.empty())
        {
            const std::array<Box, 5> row = layoutBarRow();
            const EqBand* const b = barBand();
            if (row[2].contains(x, y))
                cursor = kMouseCursorLeftRight;
            else if (row[1].contains(x, y) || (row[3].contains(x, y) && b->hasQ()))
                cursor = kMouseCursorUpDown;
            else if (row[0].contains(x, y) || row[4].contains(x, y))
                cursor = kMouseCursorHand;

            for (const Box& icon : layoutTypeIcons())
                if (icon.contains(x, y))
                    cursor = kMouseCursorHand;
            for (const Box& ch : fChannelBoxes)
                if (ch.contains(x, y))
                    cursor = kMouseCursorHand;
        }

        fPianoTip = fHoverPianoDot;

        setCursorIfChanged(cursor);

        if (prevNode != fHoverNode || prevDot != fHoverPianoDot || prevTip != fPianoTip || prevLabel != fHoverLabel ||
            prevCaption != fHoverCaption || d_isNotEqual(prevGhost, fGhostX) || prevMenu != fMenuOpen)
            repaint();
    }

    bool onScroll(const Widget::ScrollEvent& ev) final
    {
        const float x = ev.pos.getX(), y = ev.pos.getY();
        const float dir = ev.delta.getY() > 0.0 ? 1.f : ev.delta.getY() < 0.0 ? -1.f : 0.f;
        const float fine = (ev.mod & kModifierShift) != 0 ? 0.25f : 1.f;

        if (d_isZero(dir) || ! contains(ev.pos))
            return BaseWidget::onScroll(ev);

        // over a node: Q, or a cut's slope
        if (layoutDisplay().contains(x, y))
        {
            if (EqBand* const b = nodeAt(x, y))
            {
                fActive = b->id;
                if (b->isCut())
                    b->slope = std::clamp(b->slope + static_cast<int>(dir), 1, 4);
                else
                    b->q = EqResponse::normToQ(std::clamp(EqResponse::qToNorm(b->q) + dir * 0.05f, 0.f, 1.f));
                repaint();
                return true;
            }
        }

        // over a piano dot: Q
        if (fShowPiano && layoutPiano().contains(x, y))
        {
            if (EqBand* const b = pianoDotAt(x, y); b != nullptr && b->hasQ())
            {
                fActive = b->id;
                b->q = EqResponse::normToQ(std::clamp(EqResponse::qToNorm(b->q) + dir * 0.05f * fine, 0.f, 1.f));
                repaint();
                return true;
            }
        }

        // over a number box
        if (fShowControls && layoutBar().contains(x, y))
        {
            if (EqBand* const b = barBand())
            {
                const std::array<Box, 5> row = layoutBarRow();
                if (row[1].contains(x, y))
                {
                    if (b->isCut())
                        b->slope = std::clamp(b->slope + static_cast<int>(dir), 1, 4);
                    else
                        b->gain = std::clamp(b->gain + dir * 0.5f * fine, -kGainLimit, kGainLimit);
                }
                else if (row[2].contains(x, y))
                {
                    b->freq = std::clamp(EqResponse::normToFreq(std::clamp(EqResponse::freqToNorm(b->freq) + dir * 0.01f * fine, 0.f, 1.f)),
                                         kFreqMin, kFreqMax);
                }
                else if (row[3].contains(x, y) && b->hasQ())
                {
                    b->q = EqResponse::normToQ(std::clamp(EqResponse::qToNorm(b->q) + dir * 0.02f * fine, 0.f, 1.f));
                }
                else
                {
                    return BaseWidget::onScroll(ev);
                }
                repaint();
                return true;
            }
        }

        return BaseWidget::onScroll(ev);
    }
};

// --------------------------------------------------------------------------------------------------------------------

class EqMainArea final : public MainAreaContainerWidget<EqWidget>
{
public:
    EqMainArea(LabTopLevelWidget* const parent)
        : MainAreaContainerWidget(parent) {}

    [[nodiscard]] Rectangle<int> getDisplayAbsoluteArea() const noexcept
    {
        const Rectangle<int> area = fStage->getDisplayArea();
        const Point<int> pos = fStage->getAbsolutePos();
        return Rectangle<int>(pos.getX() + area.getX(), pos.getY() + area.getY(), area.getSize());
    }

    [[nodiscard]] float getDisplayBorderRadius() const noexcept
    {
        return fStage->getDisplayBorderRadius();
    }

    [[nodiscard]] bool isAnalyserVisible() const noexcept
    {
        return fStage->isAnalyserVisible();
    }
};

// --------------------------------------------------------------------------------------------------------------------
// The background shader fills the stage as usual; the analyser shader sits under the curve display only, and
// follows it as the piano and option bar are shown or hidden.

class EqRootWidget final : public RootBaseWidget,
                           private IdleCallback
{
    std::shared_ptr<TopBar> fTopBar = addWidget<TopBar>();
    std::shared_ptr<EqMainArea> fMainArea = addWidget<EqMainArea, Expanding>();

    Rectangle<int> fLastDisplayArea;
    bool fLastAnalyserVisible = true;

public:
    EqRootWidget(Window& window, LabUIWidgetInterface* const iface)
        : RootBaseWidget(window, iface)
    {
        addIdleCallback(this);
    }

private:
    void idleCallback() final
    {
        const Rectangle<int> area = fMainArea->getDisplayAbsoluteArea();

        if (area.getPos() != fLastDisplayArea.getPos() || area.getSize() != fLastDisplayArea.getSize() ||
            fMainArea->isAnalyserVisible() != fLastAnalyserVisible)
            updateSize(false);
    }

    void updateSize(const bool updateChildren) final
    {
        RootBaseWidget::updateSize(updateChildren);

        fLastDisplayArea = fMainArea->getDisplayAbsoluteArea();
        fLastAnalyserVisible = fMainArea->isAnalyserVisible();

        if (fShaders.empty())
            return;

        ShaderBaseWidget* const background = fShaders.front();
        background->setAbsolutePos(fMainArea->getMainAreaAbsolutePos());
        background->setSize(fMainArea->getMainAreaSize());
        background->setBorderRadius(fMainArea->getMainAreaBorderRadius());

        if (fShaders.size() < 2)
            return;

        ShaderBaseWidget* const analyser = fShaders.back();
        analyser->setAbsolutePos(fLastDisplayArea.getPos());
        analyser->setSize(Size<uint>(fLastDisplayArea.getWidth(), fLastDisplayArea.getHeight()));
        analyser->setBorderRadius(fMainArea->getDisplayBorderRadius());
        analyser->setVisible(fLastAnalyserVisible);
    }
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new LibreAudio::UI<LibreAudio::EqRootWidget>();
}

END_NAMESPACE_DISTRHO
