// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// FIXME remove this, helper for IDE
#include "config-custom.h"

#include "LibreAudioBaseUI.hpp"
#include "LibreAudioParameters.hpp"
#include "LibreAudioStates.hpp"

#include "EventHandlers.hpp"
#include "Layout.hpp"
#include "extra/Time.hpp"

// temp stuff
// #include "DearImGui.hpp"

#include "las-resources.h"

#include <string>
#include <vector>

#include "ui/colors.hpp"
#include "ui/metrics.hpp"
#include "ui/widgets.hpp"

#include "src/nanovg/nanovg.h"

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

static constexpr const char kStringInitPreset[] = "Init Preset \\/";
static constexpr const char kStringShortName[] = DISTRHO_PLUGIN_SHORTNAME;

using LibreAudioLogo = LibreAudioImageWidget<IMAGES_LA_PNG_DATA, IMAGES_LA_PNG_LEN>;
using LibreAudioPluginName = LibreAudioTextWidget<kStringShortName, true>;
using LibreAudioPresetWidget = LibreAudioTextWidget<kStringInitPreset>;

// TEST

/* ---- palette (from :root) ---------------------------------------------- */
#define TOP_ACC   nvgRGB(0xbe, 0xf1, 0xff)   /* --top-acc  : filter accent   */
#define TOP_CLICK nvgRGB(0xda, 0xc1, 0xf3)   /* --top-click: reverb accent   */
#define INK_3     nvgRGB(0x8c, 0x8e, 0x96)   /* deactivated text             */

typedef struct TopBarState {
    const char *name;         /* plugin wordmark, e.g. "CHORUS"              */
    const char *presetLabel;  /* current preset name                        */
    int expert;               /* 0 = Easy view, 1 = Expert view             */
    int bypass;               /* power cell lit when bypassed               */
    int prefsOpen;            /* menu (hamburger) active                    */
    int presetOpen;          /* preset caret flipped                        */
    int font;                 /* nvgCreateFont handle (UI font)             */
} TopBarState;

/* ---- pill body: rounded rect, top-light gradient, inset edges ---------- */
static void pill_bg(NVGcontext *vg, float x, float y, float w, float h) {
    float r = 8.0f;
    NVGpaint g = nvgLinearGradient(vg, x, y, x, y + h,
                                   nvgRGBA(255, 255, 255, 18),   /* .07 */
                                   nvgRGBA(255, 255, 255, 5));   /* .02 */
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, w, h, r);
    nvgFillPaint(vg, g);
    nvgFill(vg);
    /* inset hairline (approximates the layered inset box-shadows) */
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x + 0.5f, y + 0.5f, w - 1.0f, h - 1.0f, r - 0.5f);
    nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 13));
    nvgStrokeWidth(vg, 1.0f);
    nvgStroke(vg);
    /* top light edge */
    nvgBeginPath(vg);
    nvgMoveTo(vg, x + r, y + 0.75f);
    nvgLineTo(vg, x + w - r, y + 0.75f);
    nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 36));
    nvgStrokeWidth(vg, 1.0f);
    nvgStroke(vg);
}

/* centred text cell; returns advance width used */
static float cell_text(NVGcontext *vg, float x, float cy, const char *s,
                       float px, NVGcolor col, int font, float pad) {
    nvgFontFaceId(vg, font);
    nvgFontSize(vg, px);
    nvgFillColor(vg, col);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    float b[4];
    nvgTextBounds(vg, 0, 0, s, NULL, b);
    float tw = b[2] - b[0];
    nvgText(vg, x + pad, cy, s, NULL);
    return tw + pad * 2.0f;
}

/* ---- logo monogram (LA), 100-space paths scaled to `size` -------------- */
static void draw_logo(NVGcontext *vg, float x, float y, float size, NVGcolor col) {
    float s = size / 100.0f;
    nvgSave(vg);
    nvgTranslate(vg, x, y);
    nvgScale(vg, s, s);
    nvgStrokeColor(vg, col);
    nvgLineCap(vg, NVG_BUTT);
    nvgLineJoin(vg, NVG_MITER);
    nvgMiterLimit(vg, 2.2f);
    /* L */
    nvgStrokeWidth(vg, 12.5f);
    nvgBeginPath(vg);
    nvgMoveTo(vg, 30, 24); nvgLineTo(vg, 30, 72); nvgLineTo(vg, 58, 72);
    nvgStroke(vg);
    /* A outer */
    nvgBeginPath(vg);
    nvgMoveTo(vg, 61, 73); nvgLineTo(vg, 73.5f, 25); nvgLineTo(vg, 86, 73);
    nvgStroke(vg);
    /* A crossbar */
    nvgStrokeWidth(vg, 11.0f);
    nvgBeginPath(vg);
    nvgMoveTo(vg, 65, 56); nvgLineTo(vg, 82, 56);
    nvgStroke(vg);
    nvgRestore(vg);
}

/* ---- glyphs ------------------------------------------------------------- */
static void draw_chevron(NVGcontext *vg, float cx, float cy, float s, int left, NVGcolor col) {
    float d = left ? -1.0f : 1.0f;
    nvgBeginPath(vg);
    nvgMoveTo(vg, cx - d * s * 0.5f, cy - s);
    nvgLineTo(vg, cx + d * s * 0.5f, cy);
    nvgLineTo(vg, cx - d * s * 0.5f, cy + s);
    nvgStrokeColor(vg, col);
    nvgStrokeWidth(vg, 1.6f);
    nvgLineCap(vg, NVG_ROUND);
    nvgLineJoin(vg, NVG_ROUND);
    nvgStroke(vg);
}
static void draw_caret(NVGcontext *vg, float cx, float cy, float s, int flipped, NVGcolor col) {
    float d = flipped ? -1.0f : 1.0f;
    nvgBeginPath(vg);
    nvgMoveTo(vg, cx - s, cy - d * s * 0.6f);
    nvgLineTo(vg, cx + s, cy - d * s * 0.6f);
    nvgLineTo(vg, cx,     cy + d * s * 0.6f);
    nvgClosePath(vg);
    nvgFillColor(vg, col);
    nvgFill(vg);
}
static void draw_hamburger(NVGcontext *vg, float x, float y, NVGcolor col) {
    nvgStrokeColor(vg, col);
    nvgStrokeWidth(vg, 1.6f);
    nvgLineCap(vg, NVG_ROUND);
    for (int i = 0; i < 3; i++) {
        float ly = y + 4.0f + i * 3.5f;
        nvgBeginPath(vg);
        nvgMoveTo(vg, x + 2.5f, ly);
        nvgLineTo(vg, x + 12.5f, ly);
        nvgStroke(vg);
    }
}
/* power symbol: broken ring + top stem */
static void draw_power(NVGcontext *vg, float cx, float cy, float r, NVGcolor col) {
    nvgBeginPath(vg);
    nvgArc(vg, cx, cy, r, -1.30f, -1.30f - (2.0f * 3.14159265f - 0.90f), NVG_CW);
    nvgStrokeColor(vg, col);
    nvgStrokeWidth(vg, 1.6f);
    nvgLineCap(vg, NVG_ROUND);
    nvgStroke(vg);
    nvgBeginPath(vg);
    nvgMoveTo(vg, cx, cy - r - 1.0f);
    nvgLineTo(vg, cx, cy - 1.0f);
    nvgStroke(vg);
}

/* Measure a preset/pill label width for right-alignment; small helper. */
static float text_w(NVGcontext *vg, const char *s, float px, int font) {
    nvgFontFaceId(vg, font); nvgFontSize(vg, px);
    float b[4]; nvgTextBounds(vg, 0, 0, s, NULL, b);
    return b[2] - b[0];
}

/*
 * Draw the whole bar. (bx, by) is the top-left of the bar content box, bw its
 * width. Bar height is fixed by the tallest element (30px pills). Pixel-exact
 * to the source at 1x; wrap in nvgScale for HiDPI.
 */
void draw_topbar(NVGcontext *vg, float bx, float by, float bw, TopBarState st) {
    const float PILL_H = 30.0f, CELL_H = 26.0f;
    const float cy = by + PILL_H * 0.5f;
    int font = st.font;

    /* ---- left: logo + wordmark ---- */
    draw_logo(vg, bx, by, 30.0f, TOP_ACC);
    float wordX = bx + 30.0f + 10.0f;
    nvgFontFaceId(vg, font);
    nvgFontSize(vg, 14.0f);
    nvgFillColor(vg, TOP_ACC);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgTextLetterSpacing(vg, 0.7f);   /* ~.05em on 14px */
    char up[64]; int n = 0;
    for (const char *p = st.name; *p && n < 63; p++)
        up[n++] = (*p >= 'a' && *p <= 'z') ? (char)(*p - 32) : *p;
    up[n] = 0;
    nvgText(vg, wordX, cy, up, NULL);
    nvgTextLetterSpacing(vg, 0.0f);

    /* ---- right cluster: measure, then lay out left->right ---- */
    const float GAP = 12.0f, PAD = 2.0f;
    float presetW = 180.0f;
    float navW    = PAD * 2 + 34.0f * 2;              /* undo + redo cells    */
    float slotEasyW = text_w(vg, "EASY", 13, font);
    float slotExpW  = text_w(vg, "EXPERT", 13, font);
    float slotsW  = PAD * 2 + (slotEasyW + 18) + (slotExpW + 18); /* 9px pad/side */
    float sysW    = PAD * 2 + 34.0f + 34.0f;          /* menu + power         */

    float total = slotsW + GAP + sysW;
    if (st.expert) total += presetW + GAP + navW + GAP;

    float x = bx + bw - total;   /* right-aligned cluster start */

    /* preset pill (expert) */
    if (st.expert) {
        pill_bg(vg, x, by, presetW, PILL_H);
        nvgFontFaceId(vg, font); nvgFontSize(vg, 14);
        nvgFillColor(vg, st.presetOpen ? TOP_CLICK : TOP_ACC);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgText(vg, x + 12, cy, st.presetLabel ? st.presetLabel : "Init", NULL);
        draw_caret(vg, x + presetW - 14, cy, 3.5f, st.presetOpen, st.presetOpen ? TOP_CLICK : TOP_ACC);
        x += presetW + GAP;

        /* undo / redo */
        pill_bg(vg, x, by, navW, PILL_H);
        draw_chevron(vg, x + PAD + 17, cy, 4, 1, TOP_ACC);
        draw_chevron(vg, x + PAD + 34 + 17, cy, 4, 0, TOP_ACC);
        x += navW + GAP;
    }

    /* Easy | Expert */
    pill_bg(vg, x, by, slotsW, PILL_H);
    {
        float cellY = by + (PILL_H - CELL_H) * 0.5f;
        float ex = x + PAD;
        float ew = slotEasyW + 18;
        int easyOn = !st.expert && !st.prefsOpen && !st.presetOpen;
        int expOn  =  st.expert && !st.prefsOpen && !st.presetOpen;
        NVGcolor deact = (st.prefsOpen || st.presetOpen) ? INK_3 : TOP_ACC;
        /* selected cell underline */
        if (easyOn) { nvgBeginPath(vg); nvgMoveTo(vg, ex + 9, cellY + CELL_H - 4); nvgLineTo(vg, ex + ew - 9, cellY + CELL_H - 4); nvgStrokeColor(vg, TOP_ACC); nvgStrokeWidth(vg, 1.5f); nvgStroke(vg); }
        nvgFontFaceId(vg, font); nvgFontSize(vg, 13); nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, easyOn ? TOP_ACC : deact);
        nvgText(vg, ex + ew * 0.5f, cy, "Easy", NULL);
        ex += ew;
        float ew2 = slotExpW + 18;
        if (expOn) { nvgBeginPath(vg); nvgMoveTo(vg, ex + 9, cellY + CELL_H - 4); nvgLineTo(vg, ex + ew2 - 9, cellY + CELL_H - 4); nvgStrokeColor(vg, TOP_ACC); nvgStrokeWidth(vg, 1.5f); nvgStroke(vg); }
        nvgFillColor(vg, expOn ? TOP_ACC : deact);
        nvgText(vg, ex + ew2 * 0.5f, cy, "Expert", NULL);
    }
    x += slotsW + GAP;

    /* menu | power */
    pill_bg(vg, x, by, sysW, PILL_H);
    draw_hamburger(vg, x + PAD + 17 - 7.5f, by + (PILL_H - 15) * 0.5f, st.prefsOpen ? TOP_CLICK : TOP_ACC);
    draw_power(vg, x + PAD + 34 + 17, cy, 6.5f, st.bypass ? nvgRGB(0xff, 0x6b, 0x6b) : TOP_ACC);
}

// --------------------------------------------------------------------------------------------------------------------

enum WidgetIds {
    kWidgetIdStart = 1000,
    kWidgetUndo,
    kWidgetRedo,
    kWidgetSnapshotCopy,
    kWidgetSnapshotA,
    kWidgetSnapshotB,
    kWidgetSnapshotC,
    kWidgetSnapshotD,
    kWidgetEasy,
    kWidgetExpert,
    kWidgetMenu,
    kWidgetPower,
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioImageButton : public LibreAudioWidget,
                              public ButtonEventHandler
{
public:
    LibreAudioImageButton(NanoSubWidget* const parent, const uchar* const data, const uint dataSize)
        : LibreAudioWidget(parent),
          ButtonEventHandler(this),
          fImage(createImageFromMemory(data, dataSize, IMAGE_GENERATE_MIPMAPS)) {}

protected:
    virtual const Color& getColor() const
    {
        return gColors.get(this);
    }

    bool onMouse(const Widget::MouseEvent& ev) final
    {
        if (mouseEvent(ev))
            return true;
        return LibreAudioWidget::onMouse(ev);
    }

    bool onMotion(const Widget::MotionEvent& ev) final
    {
        if (motionEvent(ev))
            return true;
        return LibreAudioWidget::onMotion(ev);
    }

    void onNanoDisplay() final
    {
        const double size = Metrics::TopBar::smallImageSize * fScaleFactor;
        const uint width = getWidth();
        const uint height = getHeight();

        globalTint(getColor());

        beginPath();
        rect(0, 0, width, height);
        fillPaint(imagePattern((width - size) * 0.5, (height - size) * 0.5, size, size, 0.f, fImage, 1.f));
        fill();
    }

    void stateChanged(const State state, const State oldState) final
    {
        if ((state & kButtonStateHover) != 0)
        {
            if ((oldState & kButtonStateHover) == 0)
                getWindow().setCursor(kMouseCursorHand);
        }
        else
        {
            if ((oldState & kButtonStateHover) != 0)
                getWindow().setCursor(kMouseCursorArrow);
        }
    }

private:
    NanoImage fImage;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioImageCheckBox : public LibreAudioWidget,
                                public ButtonEventHandler
{
public:
    LibreAudioImageCheckBox(NanoSubWidget* const parent,
                            const uchar* const dataOn,
                            const uint dataOnSize,
                            const uchar* const dataOff,
                            const uint dataOffSize)
        : LibreAudioWidget(parent),
          ButtonEventHandler(this),
          fImageOn(createImageFromMemory(dataOn, dataOnSize, IMAGE_GENERATE_MIPMAPS)),
          fImageOff(createImageFromMemory(dataOff, dataOffSize, IMAGE_GENERATE_MIPMAPS))
    {
        setCheckable(true);
    }

protected:
    virtual const Color& getColor() const
    {
        return gColors.get(this);
    }

    bool onMouse(const Widget::MouseEvent& ev) final
    {
        if (mouseEvent(ev))
            return true;
        return LibreAudioWidget::onMouse(ev);
    }

    bool onMotion(const Widget::MotionEvent& ev) final
    {
        if (motionEvent(ev))
            return true;
        return LibreAudioWidget::onMotion(ev);
    }

    void onNanoDisplay() final
    {
        const double size = Metrics::TopBar::smallImageSize * fScaleFactor;
        const uint width = getWidth();
        const uint height = getHeight();

        globalTint(getColor());

        beginPath();
        rect(0, 0, width, height);
        fillPaint(imagePattern((width - size) * 0.5,
                               (height - size) * 0.5,
                               size,
                               size,
                               0.f,
                               isChecked() ? fImageOn : fImageOff,
                               1.f));
        fill();
    }

    void stateChanged(const State state, const State oldState) final
    {
        if ((state & kButtonStateHover) != 0)
        {
            if ((oldState & kButtonStateHover) == 0)
                getWindow().setCursor(kMouseCursorHand);
        }
        else
        {
            if ((oldState & kButtonStateHover) != 0)
                getWindow().setCursor(kMouseCursorArrow);
        }
    }

private:
    NanoImage fImageOn;
    NanoImage fImageOff;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioTextButton : public LibreAudioWidget,
                             public ButtonEventHandler
{
public:
    LibreAudioTextButton(NanoSubWidget* const parent, const char text[])
        : LibreAudioWidget(parent),
          ButtonEventHandler(this),
          fText(text) {}

protected:
    virtual const Color& getColor() const
    {
        return gColors.get(this);
    }

    bool onMouse(const Widget::MouseEvent& ev) final
    {
        if (mouseEvent(ev))
            return true;
        return LibreAudioWidget::onMouse(ev);
    }

    bool onMotion(const Widget::MotionEvent& ev) final
    {
        if (motionEvent(ev))
            return true;
        return LibreAudioWidget::onMotion(ev);
    }

    void onNanoDisplay() final
    {
        const uint width = getWidth();
        const uint height = getHeight();

        beginPath();
        rect(0, 0, width, height);
        fillColor(getColor());
        fontSize(Metrics::fontSize * fScaleFactor);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), fText);
    }

    void stateChanged(const State state, const State oldState) final
    {
        if ((state & kButtonStateHover) != 0)
        {
            if ((oldState & kButtonStateHover) == 0)
                getWindow().setCursor(kMouseCursorHand);
        }
        else
        {
            if ((oldState & kButtonStateHover) != 0)
                getWindow().setCursor(kMouseCursorArrow);
        }
    }

private:
    const char* const fText;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioUndoRedoWidget : public LibreAudioWidget
{
public:
    LibreAudioUndoRedoWidget(NanoSubWidget* const parent, ButtonEventHandler::Callback* const callback)
        : LibreAudioWidget(parent)
    {
        fUndoButton = new LibreAudioImageButton(this, IMAGES_UNDO_PNG_DATA, IMAGES_UNDO_PNG_LEN);
        fRedoButton = new LibreAudioImageButton(this, IMAGES_REDO_PNG_DATA, IMAGES_REDO_PNG_LEN);

        fUndoButton->setCallback(callback);
        fRedoButton->setCallback(callback);

        fUndoButton->setId(kWidgetUndo);
        fRedoButton->setId(kWidgetRedo);

        fLayout.widgets.push_back({ fUndoButton, Fixed });
        fLayout.widgets.push_back({ fRedoButton, Fixed });
    }

    void update(const bool canUndo, const bool canRedo)
    {
        fUndoButton->setEnabled(canUndo);
        fRedoButton->setEnabled(canRedo);
    }

protected:
    void onNanoDisplay() final
    {
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        LibreAudioWidget::onPositionChanged(ev);

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::UndoRedo::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::UndoRedo::margin * fScaleFactor);
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::smallImageSize * fScaleFactor);

        fLayout.setAbsolutePos(ev.pos.getX(),
                               ev.pos.getY() + (getHeight() - buttonSize) / 2,
                               padding,
                               margin);
    }

    void onResize(const ResizeEvent& ev) final
    {
        LibreAudioWidget::onResize(ev);

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::UndoRedo::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::UndoRedo::margin * fScaleFactor);
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::smallImageSize * fScaleFactor);

        fUndoButton->setSize(buttonSize, buttonSize);
        fRedoButton->setSize(buttonSize, buttonSize);

        fLayout.setWidth(ev.size.getWidth(), padding, margin);
        fLayout.setAbsolutePos(getAbsoluteX(),
                               getAbsoluteY() + (ev.size.getHeight() - buttonSize) / 2,
                               padding,
                               margin);
    }

private:
    HorizontalLayout fLayout;
    ScopedPointer<LibreAudioImageButton> fUndoButton;
    ScopedPointer<LibreAudioImageButton> fRedoButton;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioSnapshotsWidget : public LibreAudioWidget
{
public:
    LibreAudioSnapshotsWidget(NanoSubWidget* const parent, ButtonEventHandler::Callback* const callback)
        : LibreAudioWidget(parent)
    {
        fButtonCopy = new LibreAudioImageCheckBox(this,
                                                  IMAGES_X_PNG_DATA,
                                                  IMAGES_X_PNG_LEN,
                                                  IMAGES_COPY_PNG_DATA,
                                                  IMAGES_COPY_PNG_LEN);
        fButtonA = new LibreAudioTextButton(this, "A");
        fButtonB = new LibreAudioTextButton(this, "B");
        fButtonC = new LibreAudioTextButton(this, "C");
        fButtonD = new LibreAudioTextButton(this, "D");

        fButtonCopy->setCallback(callback);
        fButtonA->setCallback(callback);
        fButtonB->setCallback(callback);
        fButtonC->setCallback(callback);
        fButtonD->setCallback(callback);

        fButtonA->setCheckable(true);
        fButtonB->setCheckable(true);
        fButtonC->setCheckable(true);
        fButtonD->setCheckable(true);

        fButtonCopy->setId(kWidgetSnapshotCopy);
        fButtonA->setId(kWidgetSnapshotA);
        fButtonB->setId(kWidgetSnapshotB);
        fButtonC->setId(kWidgetSnapshotC);
        fButtonD->setId(kWidgetSnapshotD);

        fLayout.widgets.push_back({ fButtonCopy, Fixed });
        fLayout.widgets.push_back({ fButtonA, Fixed });
        fLayout.widgets.push_back({ fButtonB, Fixed });
        fLayout.widgets.push_back({ fButtonC, Fixed });
        fLayout.widgets.push_back({ fButtonD, Fixed });
    }

    void update(const bool isCopyingSnapshot, const uint8_t currentSnapshot)
    {
        fButtonCopy->setChecked(isCopyingSnapshot, false);
        fButtonA->setChecked(currentSnapshot == 0, false);
        fButtonB->setChecked(currentSnapshot == 1, false);
        fButtonC->setChecked(currentSnapshot == 2, false);
        fButtonD->setChecked(currentSnapshot == 3, false);
    }

protected:
    void onNanoDisplay() final
    {
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        LibreAudioWidget::onPositionChanged(ev);

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::margin * fScaleFactor);
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::smallImageSize * fScaleFactor);

        fLayout.setAbsolutePos(ev.pos.getX(),
                               ev.pos.getY() + (getHeight() - buttonSize) / 2,
                               padding,
                               margin);
    }

    void onResize(const ResizeEvent& ev) final
    {
        LibreAudioWidget::onResize(ev);

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::margin * fScaleFactor);
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::smallImageSize * fScaleFactor);

        fButtonCopy->setSize(buttonSize, buttonSize);
        fButtonA->setSize(buttonSize, buttonSize);
        fButtonB->setSize(buttonSize, buttonSize);
        fButtonC->setSize(buttonSize, buttonSize);
        fButtonD->setSize(buttonSize, buttonSize);

        fLayout.setWidth(ev.size.getWidth(), padding, margin);
        fLayout.setAbsolutePos(getAbsoluteX(),
                               getAbsoluteY() + (ev.size.getHeight() - buttonSize) / 2,
                               padding,
                               margin);
    }

private:
    HorizontalLayout fLayout;
    ScopedPointer<LibreAudioImageCheckBox> fButtonCopy;
    ScopedPointer<LibreAudioTextButton> fButtonA;
    ScopedPointer<LibreAudioTextButton> fButtonB;
    ScopedPointer<LibreAudioTextButton> fButtonC;
    ScopedPointer<LibreAudioTextButton> fButtonD;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioEasyExpertWidget : public LibreAudioWidget
{
public:
    LibreAudioEasyExpertWidget(NanoSubWidget* const parent, ButtonEventHandler::Callback* const callback)
        : LibreAudioWidget(parent)
    {
        fButtonEasy = new LibreAudioTextButton(this, "EASY");
        fButtonExpert = new LibreAudioTextButton(this, "EXPERT");

        fButtonEasy->setCallback(callback);
        fButtonExpert->setCallback(callback);

        fButtonEasy->setCheckable(true);
        fButtonExpert->setCheckable(true);

        fButtonEasy->setId(kWidgetEasy);
        fButtonExpert->setId(kWidgetExpert);

        fLayout.widgets.push_back({ fButtonEasy, Fixed });
        fLayout.widgets.push_back({ fButtonExpert, Fixed });
    }

    void update(const bool expertMode)
    {
        fButtonEasy->setChecked(!expertMode, false);
        fButtonExpert->setChecked(expertMode, false);
    }

protected:
    void onNanoDisplay() final
    {
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        LibreAudioWidget::onPositionChanged(ev);

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::EasyExpert::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::EasyExpert::margin * fScaleFactor);
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::smallImageSize * fScaleFactor);

        fLayout.setAbsolutePos(ev.pos.getX(),
                               ev.pos.getY() + (getHeight() - buttonSize) / 2,
                               padding,
                               margin);
    }

    void onResize(const ResizeEvent& ev) final
    {
        LibreAudioWidget::onResize(ev);

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::EasyExpert::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::EasyExpert::margin * fScaleFactor);
        const uint buttonWidth = d_roundToUnsignedInt(
            (Metrics::TopBar::Cluster::EasyExpert::width / 2 - Metrics::TopBar::Cluster::EasyExpert::margin - Metrics::TopBar::Cluster::EasyExpert::padding / 2)
            * fScaleFactor);
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::smallImageSize * fScaleFactor);

        fButtonEasy->setSize(buttonWidth, buttonSize);
        fButtonExpert->setSize(buttonWidth, buttonSize);

        fLayout.setWidth(ev.size.getWidth(), padding, margin);
        fLayout.setAbsolutePos(getAbsoluteX(),
                               getAbsoluteY() + (ev.size.getHeight() - buttonSize) / 2,
                               padding,
                               margin);
    }

private:
    HorizontalLayout fLayout;
    ScopedPointer<LibreAudioTextButton> fButtonEasy;
    ScopedPointer<LibreAudioTextButton> fButtonExpert;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioPowerButton : public LibreAudioImageButton
{
public:
    LibreAudioPowerButton(NanoSubWidget* const parent)
        : LibreAudioImageButton(parent, IMAGES_POWER_PNG_DATA, IMAGES_POWER_PNG_LEN)
    {
    }

protected:
    const Color& getColor() const override
    {
        if (! isEnabled())
            return gColors.ink3;
        if (isChecked())
            return fOffColor;
        // if (isHovered())
        //     return gColors.acc;
        return gColors.ink2;
    }

private:
    const Color fOffColor = Color::fromHTML("#ff5d5d");
};

class LibreAudioMenuWidget : public LibreAudioWidget
{
public:
    LibreAudioMenuWidget(NanoSubWidget* const parent, ButtonEventHandler::Callback* const callback)
        : LibreAudioWidget(parent)
    {
        fButtonMenu = new LibreAudioImageButton(this, IMAGES_MENU_PNG_DATA, IMAGES_MENU_PNG_LEN);
        fButtonPower = new LibreAudioPowerButton(this);

        fButtonMenu->setCallback(callback);
        fButtonPower->setCallback(callback);

        fButtonPower->setCheckable(true);

        fButtonMenu->setId(kWidgetMenu);
        fButtonPower->setId(kWidgetPower);

        fLayout.widgets.push_back({ fButtonMenu, Fixed });
        fLayout.widgets.push_back({ fButtonPower, Fixed });
    }

    void update(const bool enabled)
    {
        fButtonPower->setChecked(enabled, false);
    }

protected:
    void onNanoDisplay() final
    {
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        LibreAudioWidget::onPositionChanged(ev);

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::margin * fScaleFactor);
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::smallImageSize * fScaleFactor);

        fLayout.setAbsolutePos(ev.pos.getX(),
                               ev.pos.getY() + (getHeight() - buttonSize) / 2,
                               padding,
                               margin);
    }

    void onResize(const ResizeEvent& ev) final
    {
        LibreAudioWidget::onResize(ev);

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::margin * fScaleFactor);
        const uint buttonSize = d_roundToUnsignedInt(Metrics::TopBar::smallImageSize * fScaleFactor);

        fButtonMenu->setSize(buttonSize, buttonSize);
        fButtonPower->setSize(buttonSize, buttonSize);

        fLayout.setWidth(ev.size.getWidth(), padding, margin);
        fLayout.setAbsolutePos(getAbsoluteX(),
                               getAbsoluteY() + (ev.size.getHeight() - buttonSize) / 2,
                               padding,
                               margin);
    }

private:
    HorizontalLayout fLayout;
    ScopedPointer<LibreAudioImageButton> fButtonMenu;
    ScopedPointer<LibreAudioPowerButton> fButtonPower;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioTopBarCluster : public LibreAudioWidget
{
public:
    LibreAudioTopBarCluster(NanoSubWidget* const parent, ButtonEventHandler::Callback* const callback)
        : LibreAudioWidget(parent)
    {
        fPreset = new LibreAudioPresetWidget(this);
        fUndoRedo = new LibreAudioUndoRedoWidget(this, callback);
        fSnapshots = new LibreAudioSnapshotsWidget(this, callback);
        fEasyExpert = new LibreAudioEasyExpertWidget(this, callback);
        fMenu = new LibreAudioMenuWidget(this, callback);

        fLayout.widgets.push_back({ fPreset, Expanding });
        fLayout.widgets.push_back({ fUndoRedo, Fixed });
        fLayout.widgets.push_back({ fSnapshots, Fixed });
        fLayout.widgets.push_back({ fEasyExpert, Fixed });
        fLayout.widgets.push_back({ fMenu, Fixed });
    }

    void update(const bool canUndo,
                const bool canRedo,
                const bool isCopyingSnapshot,
                const uint8_t currentSnapshot,
                const bool expertMode,
                const bool enabled)
    {
        fUndoRedo->update(canUndo, canRedo);
        fSnapshots->update(isCopyingSnapshot, currentSnapshot);
        fEasyExpert->update(expertMode);
        fMenu->update(enabled);
    }

protected:
    void onNanoDisplay() final
    {
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        LibreAudioWidget::onPositionChanged(ev);

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::margin * fScaleFactor);
        fLayout.setAbsolutePos(ev.pos.getX(), ev.pos.getY(), padding, margin);
    }

    void onResize(const ResizeEvent& ev) final
    {
        LibreAudioWidget::onResize(ev);

        const uint height = ev.size.getHeight();

        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::Cluster::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::Cluster::margin * fScaleFactor);

        fPreset->setHeight(height);
        fUndoRedo->setSize(d_roundToUnsignedInt(Metrics::TopBar::Cluster::UndoRedo::width * fScaleFactor), height);
        fSnapshots->setSize(d_roundToUnsignedInt(Metrics::TopBar::Cluster::Snapshots::width * fScaleFactor), height);
        fEasyExpert->setSize(d_roundToUnsignedInt(Metrics::TopBar::Cluster::EasyExpert::width * fScaleFactor), height);
        fMenu->setSize(d_roundToUnsignedInt(Metrics::TopBar::Cluster::Menu::width * fScaleFactor), height);

        fLayout.setWidth(ev.size.getWidth(), padding, margin);
        fLayout.setAbsolutePos(getAbsoluteX(), getAbsoluteY(), padding, margin);
    }

private:
    HorizontalLayout fLayout;
    ScopedPointer<LibreAudioPresetWidget> fPreset;
    ScopedPointer<LibreAudioUndoRedoWidget> fUndoRedo;
    ScopedPointer<LibreAudioSnapshotsWidget> fSnapshots;
    ScopedPointer<LibreAudioEasyExpertWidget> fEasyExpert;
    ScopedPointer<LibreAudioMenuWidget> fMenu;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioTopBar : public LibreAudioWidget
{
public:
    LibreAudioTopBar(NanoTopLevelWidget* const parent, ButtonEventHandler::Callback* const callback)
        : LibreAudioWidget(parent)
    {
        fLogo = new LibreAudioLogo(this);
        fPluginName = new LibreAudioPluginName(this);
        fCluster = new LibreAudioTopBarCluster(this, callback);

        fLayout.widgets.push_back({ fLogo, Fixed });
        fLayout.widgets.push_back({ fPluginName, Expanding });
        fLayout.widgets.push_back({ fCluster, Fixed });

        // TEST
        fLogo->hide();
        fPluginName->hide();
        fCluster->hide();
    }

    void update(const bool canUndo,
                const bool canRedo,
                const bool isCopyingSnapshot,
                const uint8_t currentSnapshot,
                const bool expertMode,
                const bool enabled)
    {
        fCluster->update(canUndo, canRedo, isCopyingSnapshot, currentSnapshot, expertMode, enabled);
    }

protected:
    TopBarState tb = { .name="CHORUS", .presetLabel="Wide Shimmer",
                       .expert=1, .bypass=0, .prefsOpen=0, .presetOpen=0,
                       .font=0 };

    void onNanoDisplay() final
    {
        save();
        scale(fScaleFactor, fScaleFactor);
        draw_topbar(getContext(), 12, 12, getWidth() / fScaleFactor - 24, tb);
        restore();
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        LibreAudioWidget::onPositionChanged(ev);

        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::margin * fScaleFactor);
        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::padding * fScaleFactor);
        fLayout.setAbsolutePos(ev.pos.getX(), ev.pos.getY(), padding, margin);
    }

    void onResize(const ResizeEvent& ev) final
    {
        LibreAudioWidget::onResize(ev);

        const uint height = ev.size.getHeight();

        const uint margin = d_roundToUnsignedInt(Metrics::TopBar::margin * fScaleFactor);
        const uint marginLeft = d_roundToUnsignedInt(Metrics::TopBar::marginLeft * fScaleFactor);
        const uint marginRight = d_roundToUnsignedInt(Metrics::TopBar::marginRight * fScaleFactor);
        const uint padding = d_roundToUnsignedInt(Metrics::TopBar::padding * fScaleFactor);

        fLogo->setSize(d_roundToUnsignedInt(Metrics::TopBar::Logo::imageSize * fScaleFactor), height);
        fPluginName->setHeight(height);
        fCluster->setSize(d_roundToUnsignedInt(Metrics::TopBar::Cluster::width * fScaleFactor), height);

        fLayout.setWidth(ev.size.getWidth() + margin * 2 - marginLeft - marginRight, padding, margin);

        fLayout.setAbsolutePos(getAbsoluteX() + marginLeft - margin, getAbsoluteY(), padding, margin);
    }

private:
    HorizontalLayout fLayout;
    ScopedPointer<LibreAudioLogo> fLogo;
    ScopedPointer<LibreAudioPluginName> fPluginName;
    ScopedPointer<LibreAudioTopBarCluster> fCluster;
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioTestKnob : public LibreAudioWidget,
                           public KnobEventHandler
{
public:
    LibreAudioTestKnob(NanoSubWidget* const parent, KnobEventHandler::Callback* const callback)
        : LibreAudioWidget(parent),
          KnobEventHandler(this)
    {
        setCallback(callback);
        // setId()
        setRange(-1, 1);
        setValue(0, false);
    }

    void update(float value)
    {
        setValue(value, false);
    }

protected:
    bool onMouse(const MouseEvent& ev) final
    {
        return mouseEvent(ev, fScaleFactor);
    }

    bool onMotion(const MotionEvent& ev) final
    {
        return motionEvent(ev, fScaleFactor);
    }

    bool onScroll(const ScrollEvent& ev) final
    {
        return scrollEvent(ev);
    }

    void onNanoDisplay() final
    {
        const float cx = getWidth();
        const float cy = getHeight();
        const float size = getWidth() * 0.5f;

        constexpr float MIN_A = -150.0f;
        constexpr float MAX_A = 150.0f;
        constexpr float SWEEP = MAX_A - MIN_A;
        constexpr float rBody = 30.0f;
        constexpr float AW = 12.0f;
        constexpr float rArc = rBody + AW * 0.5f;
        const float S = size / 100.0f;

        const float norm = getNormalizedValue();
        const float valAngle   = MIN_A + norm * SWEEP;
        const float startAngle = fKnobStyle.invert ? MAX_A : (fKnobStyle.bipolar ? 0.0f : MIN_A);

        const Color& accent = fKnobStyle.enabled ? fKnobStyle.colorAccent : fKnobStyle.colorDisabled;

        save();
        translate(cx, cy);
        scale(S, S);
        translate(-50.0f, -50.0f);

        // /* --- gutter track: full sweep, near-black inset -------------------- */
        knobBand(50, 50, rArc, AW, MIN_A, MAX_A, nullptr, Color(0x14, 0x14, 0x16));

        /* --- value arc: accent, faded toward the start (alpha 0.8 -> 1.0) --- */
        if (std::abs(valAngle - startAngle) > 2.0f) {
            float gx0, gy0, gx1, gy1;
            knobPt(50, 50, rArc, startAngle, gx0, gy0);
            knobPt(50, 50, rArc, valAngle, gx1, gy1);

            const Color c0 = accent.withAlpha(0.8f);
            const Color c1 = accent.withAlpha(1.0f);
            Paint fade = linearGradient(gx0, gy0, gx1, gy1, c0, c1);
            knobBand(50, 50, rArc, AW, startAngle, valAngle, &fade, accent);
        }

        /* --- cast shadow under the cap ------------------------------------- */
        Paint sh = radialGradient(50, 53, rBody * 0.4f, rBody * 1.15f,
                                  Color(0, 0, 0, 180.f / 100.f), Color(0, 0, 0, 0));
        beginPath();
        circle(50, 53, rBody + 2.0f);
        fillPaint(sh);
        fill();

        /* --- knob body: vertical gradient + dark rim ----------------------- */
        Paint body = linearGradient(50, 50 - rBody, 50, 50 + rBody,
                                       Color(0x46, 0x46, 0x4d), Color(0x2c, 0x2c, 0x31));
        beginPath();
        circle(50, 50, rBody);
        fillPaint(body);
        fill();
        strokeColor(Color(0x0d, 0x0d, 0x0f));
        strokeWidth(1.0f);
        stroke();
        
        /* --- top bevel highlight (screen-blend approximated by additive-ish
        *     white with low alpha along the upper rim) --------------------- */
        Paint bevel = linearGradient(50, 50 - rBody, 50, 50,
                                     Color(255, 255, 255, 140.f / 100.f), Color(255, 255, 255, 0));
        beginPath();
        circle(50, 49.4f, rBody - 0.9f);
        strokeWidth(0.8f);
        strokePaint(bevel);
        stroke();
        
        /* --- pointer line -------------------------------------------------- */
        float lx0, ly0, lx1, ly1;
        knobPt(50, 50, 17, valAngle, lx0, ly0);
        knobPt(50, 50, 24, valAngle, lx1, ly1);
        beginPath();
        moveTo(lx0, ly0);
        lineTo(lx1, ly1);
        strokeColor(accent);
        strokeWidth(size > 100 ? 2.4f : 2.0f * 100.0f / size);
        lineCap(ROUND);
        stroke();

        restore();
    }

private:
    struct KnobStyle {
        Color colorAccent;
        Color colorDisabled;
        bool enabled;
        bool bipolar;
        bool invert;
    };
    const KnobStyle fKnobStyle = {
        .colorAccent = {0xc3, 0xd9, 0xff},
        .colorDisabled = {0x5d, 0x5d, 0x66},
        .enabled = true,
        .bipolar = true,
        .invert = false,
    };

    static constexpr float fValueMin = -1.f;
    static constexpr float fValueMax = 1.f;

    static inline constexpr float knobRad(float deg)
    {
        return (deg - 90.f) * M_PI / 180.f;
    }

    static inline void knobPt(float cx, float cy, float r, float deg, float& x, float& y)
    {
        const float a = deg * M_PI / 180.f;
        x = cx + r * std::sinf(a);
        y = cy - r * std::cosf(a);
    }

    void knobBand(float cx, float cy, float rArc, float w, float a0, float a1, Paint* paint, Color solid)
    {
        float lo = a0 < a1 ? a0 : a1;
        float hi = a0 < a1 ? a1 : a0;
        if (hi - lo < 0.05f)
            return;

        beginPath();
        arc(cx, cy, rArc, knobRad(lo), knobRad(hi), CW);
        strokeWidth(w);
        lineCap(ROUND);

        if (paint)
            strokePaint(*paint);
        else
            strokeColor(solid);

        stroke();
    }
};

class LibreAudioMainArea : public LibreAudioWidget,
                           public KnobEventHandler::Callback,
                           private IdleCallback
{
public:
    static constexpr uint32_t kTestParamId = kParametersMainStart + djFilter::kFaustParameterKnob;

    LibreAudioMainArea(NanoTopLevelWidget* const parent, LibreAudioBaseUI* const ui)
        : LibreAudioWidget(parent),
          fUI(ui),
          fTestKnob(this, this)
    {
        colors[0] = Color::fromHTML("#ffbfcb");
        colors[1] = Color::fromHTML("#ffdfad");
        colors[2] = Color::fromHTML("#d2fdd3");
        colors[3] = Color::fromHTML("#bef1ff");
        colors[4] = Color::fromHTML("#c3d9ff");
        colors[5] = Color::fromHTML("#dac1f3");
        colors[6] = Color::fromHTML("#ffdcf5");

        getWindow().addIdleCallback(this);
    }

    void update(float value)
    {
        fTestKnob.update(value);
    }

protected:
    void knobDragStarted(SubWidget* widget) final
    {
        fUI->parameterControlPressed(kTestParamId);
    }

    void knobDragFinished(SubWidget* widget) final
    {
        fUI->parameterControlReleased(kTestParamId);
    }

    void knobValueChanged(SubWidget* widget, float value) final
    {
        fTestKnob.update(value);
        fUI->parameterControlModified(kTestParamId, value);
    }

    void knobDoubleClicked(SubWidget*) final
    {
    }

    LibreAudioBaseUI* const fUI;
    LibreAudioTestKnob fTestKnob;

    struct Pt { float x; float y; };
    static constexpr int kStopWidth = 50;
    static constexpr int kNumStops = 6;
    std::array<Pt, kStopWidth * kNumStops + 1> pts;
    std::array<Color, kNumStops + 1> colors;

    static constexpr int WIN = 2;
    static constexpr int dbMax = 18;
    static constexpr int dbMin = -42;
    static constexpr int rw = 868;
    static constexpr int rh = 200;

    float ra() const
    {
        return 1.0 - (double)std::rand() / RAND_MAX * 0;
    }

    void tracePts(float hz, float amp, float ph)
    {
        for (int i = 0; i < std::size(pts); i++) {
            float t = (float)i / (std::size(pts) - 1);
            float y = /*baseY(t)*/ getHeight() * 0.5f - (amp * std::sin(2.f * M_PI * (t * hz * WIN - ph))) * ra() * fScaleFactor;
            // y = std::max(-40.f, std::min(h + 40, y));
            pts[i] = { t * getWidth() + (float)fScaleFactor * 100.f * ra() - 100.f * (float)fScaleFactor, y };
        }
    }

    float h1 = 0.f;
    float ph1 = 0.f;
    float last = 0.f;

    void idleCallback() final
    {
        int t = d_gettime_ms();
        float phase = t * 0.0001f;
        float dt = phase - last;
        if (!(dt >= 0) || dt > 0.1) dt = 0.016;
        last = phase;

        const float k = std::min(1.f, dt * 7.f);

        h1 += (3.0f - h1) * k;
        ph1 += h1 * dt;
        repaint();
    }

    void onNanoDisplay() final
    {
        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(0.f, 0.f, 0.f);
        fill();

        tracePts(h1, 100, ph1);

        strokeWidth(3.f * fScaleFactor);

        beginPath();
        moveTo(pts[0].x, pts[0].y);

        for (int i = 1, step = 0; i < kStopWidth * kNumStops; ++i)
        {
            if ((i % kStopWidth) == 0)
            {
                lineTo(pts[i].x, pts[i].y);
                strokePaint(linearGradient(pts[step * kStopWidth].x,
                                            pts[step * kStopWidth].y,
                                            pts[(step + 1) * kStopWidth].x,
                                            pts[(step + 1) * kStopWidth].y,
                                            colors[step],
                                            colors[step + 1]));
                stroke();
                ++step;

                beginPath();
                moveTo(pts[i].x, pts[i].y);
            }
            else
            {
                lineTo(pts[i].x, pts[i].y);
            }
        }

        lineTo(pts[kStopWidth * kNumStops].x, pts[kStopWidth * kNumStops].y);
        strokePaint(linearGradient(pts[kStopWidth * (kNumStops - 1)].x,
                                   pts[kStopWidth * (kNumStops - 1)].y,
                                   pts[kStopWidth * kNumStops].x,
                                   pts[kStopWidth * kNumStops].y,
                                   colors[(kNumStops - 1)],
                                   colors[kNumStops]));
        stroke();
    }

    void onPositionChanged(const PositionChangedEvent& ev)
    {
        LibreAudioWidget::onPositionChanged(ev);
        fTestKnob.setAbsolutePos(ev.pos);
    }

    void onResize(const ResizeEvent& ev)
    {
        LibreAudioWidget::onResize(ev);
        fTestKnob.setSize(ev.size.getWidth() / 2, ev.size.getHeight() / 2);
    }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioBottomBar : public LibreAudioWidget
{
public:
    LibreAudioBottomBar(NanoTopLevelWidget* const parent)
        : LibreAudioWidget(parent)
    {
    }

protected:
    void onNanoDisplay() final
    {
        fontSize(26.f * fScaleFactor);

        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(0.f, 0.f, 0.1f);
        fill();
        fillColor(1.f, 1.f, 1.f);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
        textBox(0.f, getHeight() * 0.5f, getWidth(), "This is the bottom-bar");
    }

    void onResize(const ResizeEvent& ev) final
    {
        LibreAudioWidget::onResize(ev);
    }
};

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioUI : public LibreAudioBaseUI,
                     private ButtonEventHandler::Callback
{
public:
    LibreAudioUI()
        : LibreAudioBaseUI()
    {
        createFontFromMemory("Saira Semi Condensed (Regular)",
                             FONTS_SAIRASEMICONDENSED_SEMIBOLD_TTF_DATA,
                             FONTS_SAIRASEMICONDENSED_SEMIBOLD_TTF_LEN,
                             false);
        createFontFromMemory("Saira Semi Condensed (SemiBold)",
                             FONTS_SAIRASEMICONDENSED_SEMIBOLD_TTF_DATA,
                             FONTS_SAIRASEMICONDENSED_SEMIBOLD_TTF_LEN,
                             false);
        fontFace("Saira Semi Condensed (Regular)");

        fTopBar = new LibreAudioTopBar(this, this);
        fMainArea = new LibreAudioMainArea(this, this);
        fBottomBar = new LibreAudioBottomBar(this);

        fLayout.widgets.push_back({ fTopBar, Fixed });
        fLayout.widgets.push_back({ fMainArea, Expanding });
        fLayout.widgets.push_back({ fBottomBar, Fixed });

        adjustSize();
    }

    ~LibreAudioUI() override
    {
    }

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // Widget Callbacks

    void buttonClicked(SubWidget* const widget, int) final
    {
        switch (widget->getId())
        {
        case kWidgetUndo:
            undo();
            break;
        case kWidgetRedo:
            redo();
            break;
        case kWidgetSnapshotCopy:
            // if (static_cast<LibreAudioSnapshotsWidget::ImageButton*>(widget)->isChecked())
            snapshotButtonClicked(kSnapshotButtonCopy);
            break;
        case kWidgetSnapshotA:
            if (static_cast<LibreAudioTextButton*>(widget)->isChecked())
                snapshotButtonClicked(kSnapshotButtonA);
            break;
        case kWidgetSnapshotB:
            if (static_cast<LibreAudioTextButton*>(widget)->isChecked())
                snapshotButtonClicked(kSnapshotButtonB);
            break;
        case kWidgetSnapshotC:
            if (static_cast<LibreAudioTextButton*>(widget)->isChecked())
                snapshotButtonClicked(kSnapshotButtonC);
            break;
        case kWidgetSnapshotD:
            if (static_cast<LibreAudioTextButton*>(widget)->isChecked())
                snapshotButtonClicked(kSnapshotButtonD);
            break;
        case kWidgetEasy:
            fExpertMode = false;
            break;
        case kWidgetExpert:
            fExpertMode = true;
            break;
        case kWidgetMenu:
            break;
        case kWidgetPower:
            parameterControlPressed(kCommonParameterBypass);
            parameterControlModified(kCommonParameterBypass,
                                     static_cast<LibreAudioPowerButton*>(widget)->isChecked() ? 1.f : 0.f);
            parameterControlReleased(kCommonParameterBypass);
            break;
        }
    }

    void onNanoDisplay() final
    {
        beginPath();
        rect(0, 0, getWidth(), getHeight());
        fillColor(gColors.background);
        fill();
        strokeColor(gColors.outline);
        strokeWidth(fScaleFactor * 2); // x2 for making it centered
        stroke();
    }

    void onResize(const ResizeEvent& ev) final
    {
        UI::onResize(ev);
        adjustSize();
    }

    void uiIdle() final
    {
        LibreAudioBaseUI::uiIdle();

        fMainArea->update(fParameterValuesRef[kParametersMainStart + djFilter::kFaustParameterKnob]);

        fTopBar->update(canUndo(),
                        canRedo(),
                        isCopyingSnapshot(),
                        getCurrentSnapshot(),
                        fExpertMode,
                        d_isNotZero(fParameterValuesRef[kCommonParameterBypass]));
    }

    void uiScaleFactorChanged(const double scaleFactor) final
    {
        fScaleFactor = scaleFactor;
        adjustSize();
    }

private:
    double fScaleFactor = getScaleFactor();
    bool fExpertMode = false;

    VerticalLayout fLayout;
    ScopedPointer<LibreAudioTopBar> fTopBar;
    ScopedPointer<LibreAudioMainArea> fMainArea;
    ScopedPointer<LibreAudioBottomBar> fBottomBar;

    void adjustSize()
    {
        const uint width = getWidth();
        const uint height = getHeight();
        const uint padding = d_roundToUnsignedInt(Metrics::Window::padding * fScaleFactor);
        const uint margin = d_roundToUnsignedInt(Metrics::Window::margin * fScaleFactor);

        fTopBar->setSize(width, d_roundToUnsignedInt(Metrics::TopBar::height * fScaleFactor));
        fMainArea->setWidth(width);
        fBottomBar->setSize(width, d_roundToUnsignedInt(Metrics::BottomBar::height * fScaleFactor));

        fLayout.setHeight(height, padding, margin);

        fLayout.setAbsolutePos(0, 0, padding, margin);
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibreAudioUI)
};

// --------------------------------------------------------------------------------------------------------------------

UI* createUI()
{
    return new LibreAudioUI();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
