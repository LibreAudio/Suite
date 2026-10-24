// Double Click helper widget for DPF
// Copyright (C) 2022-2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#pragma once

#include "DearImGui.hpp"

#include "las-resources.h"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

inline constexpr ImVec4 ImVec4Color(const Color& c)
{
    return ImVec4(c.red, c.green, c.blue, c.alpha);
}

// --------------------------------------------------------------------------------------------------------------------

class DoubleClickHelper final : public ImGuiTopLevelWidget
{
public:
    struct Callback {
        virtual ~Callback() {}
        virtual void doubleClickHelperDone(SubWidget* widget, const char* text) = 0;
    };

    explicit DoubleClickHelper(TopLevelWidget* const tlw,
                               Callback* const cb,
                               SubWidget* const w,
                               const char* const text,
                               const Rectangle<int>& area,
                               const Color& textColor,
                               const Color& highlightColor,
                               const float borderRadius,
                               const float fontSize,
                               const bool isNumber)
        : ImGuiTopLevelWidget(tlw->getWindow(), fontSize),
          callback(cb),
          widget(w)
    {
        pos = ImVec2(area.getX(), area.getY());
        size = ImVec2(area.getWidth(), area.getHeight());
 
        std::strncpy(textBuf, text, sizeof(textBuf) - 1);

        ImGuiStyle& style(ImGui::GetStyle());
        style.WindowPadding = ImVec2(0.f, (area.getHeight() - fontSize) * 0.5f);
        style.WindowRounding = borderRadius;
        style.WindowBorderSize = 0.f;
        style.ChildRounding = style.ChildBorderSize = 0.f;
        style.PopupRounding = style.PopupBorderSize = 0.f;
        style.FrameRounding = style.FrameBorderSize = 0.f;
        style.Colors[ImGuiCol_FrameBg] = ImVec4();
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4();
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4();
        style.Colors[ImGuiCol_NavHighlight] = ImVec4();
        style.Colors[ImGuiCol_Text] = ImVec4Color(textColor);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4Color(textColor);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4Color(highlightColor);

        ImGuiIO& io(ImGui::GetIO());
        ImFontConfig fc;
        fc.FontDataOwnedByAtlas = false;
        fc.OversampleH = 1;
        fc.OversampleV = 1;
        fc.PixelSnapH = true;
        io.Fonts->Clear();

        if (isNumber)
        {
            textFlags |= 0
                |ImGuiInputTextFlags_CharsDecimal
                |ImGuiInputTextFlags_CharsScientific
                |ImGuiInputTextFlags_CharsNoBlank
                |ImGuiInputTextFlags_AutoSelectAll;

            io.Fonts->AddFontFromMemoryTTF(const_cast<uchar*>(FONTS_SPLINESANSMONO_REGULAR_TTF_DATA),
                                           FONTS_SPLINESANSMONO_REGULAR_TTF_LEN,
                                           d_roundToIntPositive(fontSize),
                                           &fc);
        }
        else
        {
            io.Fonts->AddFontFromMemoryTTF(const_cast<uchar*>(FONTS_INTER_18PT_REGULAR_TTF_DATA),
                                           FONTS_INTER_18PT_REGULAR_TTF_LEN,
                                           d_roundToIntPositive(fontSize),
                                           &fc);
        }

        io.Fonts->Build();
    }

private:
    Callback* const callback;
    SubWidget* const widget;

    bool closeOnNextIdle = false;
    bool firstOpen = true;
    char textBuf[64] = {};
    ImGuiInputTextFlags textFlags = ImGuiInputTextFlags_EnterReturnsTrue;
    ImVec2 pos, size;

    void idleCallback() override
    {
        ImGuiTopLevelWidget::idleCallback();

        if (closeOnNextIdle)
        {
            closeOnNextIdle = false;
            callback->doubleClickHelperDone(widget, textBuf);
            return;
        }
    }

    void onImGuiDisplay() override
    {
        const bool wasFirstOpen = firstOpen;

        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(size);
        ImGui::SetNextWindowContentSize(size);
        ImGui::SetNextWindowFocus();

        constexpr const ImGuiWindowFlags windowFlags = 0
            |ImGuiWindowFlags_NoDecoration
            |ImGuiWindowFlags_NoMove
            |ImGuiWindowFlags_NoScrollWithMouse
            |ImGuiWindowFlags_AlwaysAutoResize;

        ImGui::Begin("Double Click Helper", nullptr, windowFlags);

        if (wasFirstOpen)
        {
            firstOpen = false;
            ImGui::SetKeyboardFocusHere();
        }

        closeOnNextIdle |= ImGui::InputText("", textBuf, sizeof(textBuf), textFlags);

        if (!wasFirstOpen && !ImGui::IsItemActive())
            closeOnNextIdle = true;

        ImGui::End();
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
