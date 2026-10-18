// lab: Libre Audio Base-Widgets
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#pragma once

#include "Application.hpp"
#include "NanoVG.hpp"

START_NAMESPACE_DGL

class LabUIWidgetInterface;

// --------------------------------------------------------------------------------------------------------------------

enum Corner : uint8_t {
    kCornerNone,
    kCornerLeft,
    kCornerRight,
    kCornerBoth,
};

enum Orientation : bool {
    kHorizontal,
    kVertical,
};

constexpr inline const Corner kCornerAuto = static_cast<Corner>(UINT8_MAX);

// widget ids are assigned after parameters (defined elsewhere), so leave a big enough gap to ensure no overlap
constexpr inline const uint16_t kWidgetIdsStart = 0xfff + 1;

// --------------------------------------------------------------------------------------------------------------------
// base widget class

template <class BaseWidget>
class BaseWidgetOf : public BaseWidget
{
public:
    explicit BaseWidgetOf(BaseWidgetOf<NanoSubWidget>* const parent)
        : BaseWidget(parent),
          fInterface(parent->fInterface),
          fScaleFactor(parent->fScaleFactor) {}

    explicit BaseWidgetOf(BaseWidgetOf<NanoTopLevelWidget>* const parent)
        : BaseWidget(parent),
          fInterface(parent->fInterface),
          fScaleFactor(parent->fScaleFactor) {}

    explicit BaseWidgetOf(Window& windowToMapTo, LabUIWidgetInterface* const iface)
        : BaseWidget(windowToMapTo),
          fInterface(iface),
          fScaleFactor(windowToMapTo.getScaleFactor()) {}

    ~BaseWidgetOf() override
    {
        for (IdleCallback* callback : fCallbacks)
        {
            if constexpr (std::is_same_v<BaseWidget, NanoSubWidget>)
                BaseWidget::getWindow().removeIdleCallback(callback);
            else
                BaseWidget::removeIdleCallback(callback);
        }
    }

    void addIdleCallback(IdleCallback* const callback)
    {
        fCallbacks.push_back(callback);

        if constexpr (std::is_same_v<BaseWidget, NanoSubWidget>)
            BaseWidget::getWindow().addIdleCallback(callback);
        else
            BaseWidget::addIdleCallback(callback);
    }

    [[nodiscard]] double getTime() const
    {
        return BaseWidget::getApp().getTime();
    }

    [[nodiscard]] bool timeEllapsed(const double lastTime, const double wantedTime, const double timeNow) const noexcept
    {
        return d_isNotZero(lastTime) ? timeNow - lastTime >= wantedTime : false;
    }

    [[nodiscard]] bool timeEllapsed(const double lastTime, const double wantedTime) const
    {
        return timeEllapsed(lastTime, wantedTime, getTime());
    }

    [[nodiscard]] bool timeNotEllapsed(const double lastTime, const double wantedTime, const double timeNow) const noexcept
    {
        return d_isNotZero(lastTime) ? timeNow - lastTime < wantedTime : false;
    }

    [[nodiscard]] bool timeNotEllapsed(const double lastTime, const double wantedTime) const
    {
        return timeNotEllapsed(lastTime, wantedTime, getTime());
    }

protected:
    LabUIWidgetInterface* const fInterface;
    float fScaleFactor;

    template <class R, Corner _corner = kCornerAuto>
    void drawReferenceBackground()
    {
        drawReferenceBackground<R, R::backgroundColor, _corner>();
    }

    template <class R, const Color& backgroundColor, Corner _corner = kCornerAuto>
    void drawReferenceBackground()
    {
        constexpr Corner corner = _corner != kCornerAuto ? _corner : R::borderRadius != 0 ? kCornerBoth : kCornerNone;
       #ifdef __GNUC__
        #pragma GCC poison _corner
       #endif
        static_assert(corner == kCornerNone || R::borderRadius != 0, "corner != none requires borderRadius");

        const float w = BaseWidget::getWidth();
        const float h = BaseWidget::getHeight();

        if constexpr (d_isNotZero(backgroundColor.alpha))
        {
            BaseWidget::beginPath();

            if constexpr (R::borderRadius != 0 && corner != kCornerNone)
                BaseWidget::roundedRect(0, 0, w, h, R::borderRadius * fScaleFactor);
            else
                BaseWidget::rect(0, 0, w, h);

            if constexpr (corner == kCornerLeft || corner == kCornerRight)
            {
                DISTRHO_CUSTOM_SAFE_ASSERT_RETURN("Corners must have opaque color",
                                                    d_isEqual(R::backgroundColor.alpha, 1.f),);
            }

            BaseWidget::fillColor(backgroundColor);
            BaseWidget::fill();

            if constexpr (corner == kCornerLeft)
            {
                BaseWidget::beginPath();
                BaseWidget::rect(w * 0.5f, 0, w * 0.5f, h);
                BaseWidget::fill();
            }
            if constexpr (corner == kCornerRight)
            {
                BaseWidget::beginPath();
                BaseWidget::rect(0, 0, w * 0.5f, h);
                BaseWidget::fill();
            }
        }

        if constexpr (d_isNotZero(R::shadowColor.alpha) && R::shadowFeather != 0)
        {
            const float r = R::borderRadius * fScaleFactor;
            const float sx = R::shadowOffset.getX() * fScaleFactor;
            const float sy = R::shadowOffset.getY() * fScaleFactor;
            const float sw = R::shadowSize.getWidth() * fScaleFactor;
            const float sh = R::shadowSize.getHeight() * fScaleFactor;
            const float feather = R::shadowFeather * fScaleFactor;
            const float fx = sx - feather;
            const float fy = sy - feather;
            const float fw = w + sx + feather * 2;
            const float fh = h + sy + feather * 2;

            BaseWidget::beginPath();

            BaseWidget::rect(fx, fy, fw, fh);

            if constexpr (R::borderRadius != 0)
                BaseWidget::roundedRect(0, 0, w, h, r);
            else
                BaseWidget::roundedRect(0, 0, w, h);

            BaseWidget::pathWinding(BaseWidget::CW);
            BaseWidget::fillPaint(BaseWidget::boxGradient(sx - sw * 0.5f,
                                                          sy - sh * 0.5f,
                                                          w + sw,
                                                          h + sh,
                                                          r,
                                                          feather,
                                                          R::shadowColor,
                                                          Color(R::shadowColor, 0.f)));
            BaseWidget::fill();
        }
    }

    template <class R>
    void drawReferenceBorder()
    {
        drawReferenceBorder<R, R::borderColor>();
    }

    template <class R, const Color& borderColor>
    void drawReferenceBorder()
    {
        static_assert(d_isZero(R::borderHighlightColor.alpha) || R::borderRadius != 0,
                      "borderRadius must be != 0 if using border highlight color");

            const float w = BaseWidget::getWidth();
            const float h = BaseWidget::getHeight();

        if constexpr (R::border != 0 && d_isNotZero(borderColor.alpha))
        {
            const float border = d_roundToIntPositive(R::border * fScaleFactor);
            const float borderh = border * 0.5f;

            const float sx = borderh;
            const float sy = borderh;
            const float ex = w - borderh;
            const float ey = h - borderh;

            BaseWidget::beginPath();

            if constexpr (R::borderRadius != 0)
            {
                const float r = R::borderRadius * fScaleFactor;
                DISTRHO_SAFE_ASSERT_RETURN(r < w * 0.5f,);
                DISTRHO_SAFE_ASSERT_RETURN(r < h * 0.5f,);
                DISTRHO_SAFE_ASSERT_RETURN(r > border,);

                const float arcRadius = r - border;

                BaseWidget::moveTo(sx + r, sy);
                BaseWidget::arcTo(sx, sy, sx, ey - r, arcRadius);
                BaseWidget::lineTo(sx, ey - r);
                BaseWidget::arcTo(sx, ey, sx + r, ey, arcRadius);
                BaseWidget::lineTo(ex - r, ey);
                BaseWidget::arcTo(ex, ey, ex, ey - r, arcRadius);
                BaseWidget::lineTo(ex, sx + r);
                BaseWidget::arcTo(ex, sy, ex - r, sy, arcRadius);
            }
            else
            {
                BaseWidget::moveTo(sx, sy);
                BaseWidget::lineTo(sx, ey);
                BaseWidget::lineTo(ex, ey);
                BaseWidget::lineTo(ex, sy);
            }

            BaseWidget::closePath();
            BaseWidget::strokeColor(borderColor);
            BaseWidget::strokeWidth(border);
            BaseWidget::stroke();

            if constexpr (R::borderRadius != 0 && d_isNotZero(R::borderHighlightColor.alpha))
            {
                const float r = R::borderRadius * fScaleFactor;

                // BaseWidget::save();
                // BaseWidget::scissor(0, 0, w, r);
                BaseWidget::strokePaint(BaseWidget::linearGradient(0,
                                                                   0,
                                                                   0,
                                                                   r,
                                                                   R::borderHighlightColor,
                                                                   Color(R::borderHighlightColor, 0.f)));
                BaseWidget::stroke();
                // BaseWidget::restore();
            }

            if constexpr (R::borderRadius != 0 && d_isNotZero(R::borderShadowColor.alpha))
            {
                const float r = R::borderRadius * fScaleFactor;

                // BaseWidget::save();
                // BaseWidget::scissor(0, h - r, w, r);
                BaseWidget::strokePaint(BaseWidget::linearGradient(0,
                                                                   h - r,
                                                                   0,
                                                                   h,
                                                                   Color(R::borderShadowColor, 0.f),
                                                                   R::borderShadowColor));
                BaseWidget::stroke();
                // BaseWidget::restore();
            }
        }
    }

    template <class R>
    void drawReferenceLine(const float sx, const float sy, const float ex, const float ey)
    {
        drawReferenceLine<R, R::color>(sx, sy, ex, ey);
    }

    template <class R, const Color& color>
    void drawReferenceLine(const float sx, const float sy, const float ex, const float ey)
    {
        if constexpr (d_isNotZero(R::color.alpha))
        {
            BaseWidget::beginPath();
            BaseWidget::moveTo(sx, sy);
            BaseWidget::lineTo(ex, ey);
            BaseWidget::strokeColor(R::color);
            if constexpr (R::width != 0)
                BaseWidget::strokeWidth(R::width * fScaleFactor);
            else if constexpr (R::height != 0)
                BaseWidget::strokeWidth(R::height * fScaleFactor);
            else
                BaseWidget::strokeWidth(fScaleFactor);
            BaseWidget::stroke();
        }
    }

    template <class R>
    void drawReferenceText(const char* const text)
    {
        drawReferenceText<R, R::color>(text);
    }

    template <class R, const Color& color>
    void drawReferenceText(const char* const text)
    {
        if constexpr (d_isNotZero(color.alpha))
        {
            const float w = BaseWidget::getWidth();
            const float h = BaseWidget::getHeight();

            BaseWidget::fillColor(color);
            BaseWidget::fontSize(R::fontSize * fScaleFactor);
            BaseWidget::textAlign(BaseWidget::ALIGN_CENTER | BaseWidget::ALIGN_MIDDLE);
            BaseWidget::textLetterSpacing(R::letterSpacing * fScaleFactor);
            BaseWidget::text(w * 0.5f, h * 0.5f, text);
        }
    }

    template <class R>
    void updateReferenceSize()
    {
        if constexpr (R::width != 0)
            BaseWidget::setWidth(d_roundToUnsignedInt(R::width * fScaleFactor));

        if constexpr (R::height != 0)
            BaseWidget::setHeight(d_roundToUnsignedInt(R::height * fScaleFactor));
    }

    void updateScaleFactor(const float scaleFactor)
    {
        fScaleFactor = scaleFactor;

        const std::list<SubWidget*> children = BaseWidget::getChildren();

        for (SubWidget* const child : children)
            static_cast<BaseWidgetOf<NanoSubWidget>*>(child)->updateScaleFactor(scaleFactor);
    }

    virtual void updateSize(const bool updateChildren = true)
    {
        if (! updateChildren)
            return;

        const std::list<SubWidget*> children = BaseWidget::getChildren();

        for (SubWidget* const child : children)
            static_cast<BaseWidgetOf<NanoSubWidget>*>(child)->updateSize(true);
    }

private:
    // FIXME remove this
    std::vector<IdleCallback*> fCallbacks;

    friend class BaseWidgetOf<NanoSubWidget>;
    friend class BaseWidgetOf<NanoTopLevelWidget>;
};

// --------------------------------------------------------------------------------------------------------------------
// top-level widget class

class LabTopLevelWidget : public BaseWidgetOf<NanoTopLevelWidget>
{
    using BaseWidget = BaseWidgetOf<NanoTopLevelWidget>;

public:
    explicit LabTopLevelWidget(Window& windowToMapTo, LabUIWidgetInterface* const iface)
        : BaseWidget(windowToMapTo, iface) {}

    void setWidth() = delete;
    void setHeight() = delete;
    void setSize() = delete;

protected:
    void onNanoDisplay() override
    {
    }
};

// --------------------------------------------------------------------------------------------------------------------
// (sub) widget class

class LabWidget : public BaseWidgetOf<NanoSubWidget>
{
    using BaseWidget = BaseWidgetOf<NanoSubWidget>;

public:
    explicit LabWidget(LabTopLevelWidget* const parent)
        : BaseWidget(parent) {}

    explicit LabWidget(LabWidget* const parent)
        : BaseWidget(parent) {}
};

// --------------------------------------------------------------------------------------------------------------------
// do not allow using `BaseWidgetOf` after this point

#ifdef __GNUC__
#pragma GCC poison BaseWidgetOf
#endif

// --------------------------------------------------------------------------------------------------------------------
// reference (sub) widget class

template<class R, Corner corner = kCornerAuto>
class LabReferenceWidget : public LabWidget
{
    using BaseWidget = LabWidget;

public:
    explicit LabReferenceWidget(LabTopLevelWidget* const parent)
        : BaseWidget(parent)
    {
        updateReferenceSize<R>();
    }

    explicit LabReferenceWidget(LabWidget* const parent)
        : BaseWidget(parent)
    {
        updateReferenceSize<R>();
    }

    [[nodiscard]] float getBorderRadius() const noexcept
    {
        return R::borderRadius * fScaleFactor;
    }

protected:
    void onNanoDisplay() override
    {
        drawReferenceBackground<R, corner>();
        drawReferenceBorder<R>();
    }

    void updateSize(const bool updateChildren) override
    {
        updateReferenceSize<R>();
        BaseWidget::updateSize(updateChildren);
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
