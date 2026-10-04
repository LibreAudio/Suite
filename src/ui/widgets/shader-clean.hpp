// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../_lab/interface.hpp"

#include "LibreAudioIPC.hpp"

#include "Application.hpp"
#include "DistrhoUtils.hpp"
#include "SubWidget.hpp"
#include "TopLevelWidget.hpp"

#include "extra/String.hpp"
#include "extra/ValueSmoother.hpp"

#include "las-resources.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#include "OpenGL-include.hpp"

#ifdef DISTRHO_OS_WINDOWS
#include "extra/Windows-include.h"
extern "C" {
__declspec(dllimport) PROC WINAPI wglGetProcAddress(LPCSTR);
}
#endif

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class ShaderBaseWidget : public SubWidget
{
public:
    explicit ShaderBaseWidget(TopLevelWidget* const parent, LabUIWidgetInterface* const iface)
        : SubWidget(parent),
          fInterface(iface) {}

    void setBorderRadius(const float borderRadius) noexcept
    {
        if (d_isEqual(fBorderRadius, borderRadius))
            return;
        fBorderRadius = borderRadius;
        repaint();
    }

protected:
    LabUIWidgetInterface* const fInterface;
    float fBorderRadius = 0.f;
};

// --------------------------------------------------------------------------------------------------------------------

template<const char src[], uint size, uint textureSize = 0>
class BackgroundShaderWidget final : public ShaderBaseWidget,
                                     public IdleCallback
{
   #if defined(DGL_USE_OPENGL3) && !defined(DGL_USE_GLES2)
    static constexpr const GLenum kSingleChannelFormat = GL_RED;
   #else
    static constexpr const GLenum kSingleChannelFormat =  GL_LUMINANCE;
   #endif

public:
    explicit BackgroundShaderWidget(TopLevelWidget* const parent, LabUIWidgetInterface* const iface)
        : ShaderBaseWidget(parent, iface),
          fParent(parent)
    {
        parent->addIdleCallback(this, kTargetIdleTimeMs);

       #ifdef DISTRHO_OS_WINDOWS
        if (! initGL())
            return;
       #endif

        const GLuint program = glCreateProgram();
        DISTRHO_SAFE_ASSERT_RETURN(program != 0,);

        const GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
        DISTRHO_SAFE_ASSERT_RETURN(vertex != 0,);

        glGenBuffers(std::size(gl3.buffers), gl3.buffers);
        glGenTextures(std::size(gl3.textures), gl3.textures);

        static constexpr const char kShaderHeader[] =
           #if defined(DGL_USE_GLES3)
            "#version 300 es\n"
            "#define LIBREAUDIO_GL3\n"
           #elif defined(DGL_USE_GLES2)
            "#version 130\n"
            "#define LIBREAUDIO_GL2\n"
           #elif defined(DGL_USE_OPENGL3)
            "#version 150 core\n"
            "#define LIBREAUDIO_GL3\n"
            #else
            "#define LIBREAUDIO_GL2\n"
           #endif
            "#define LIBREAUDIO_HOSTED\n"
        ;

        static constexpr const char* const vertexSource[] = {
            kShaderHeader,
            SHADERS_LIBREAUDIO_VERT_DATA,
        };
        static constexpr const GLint vertexSourceLen[] = {
            sizeof(kShaderHeader) - 1,
            SHADERS_LIBREAUDIO_VERT_LEN,
        };
        glShaderSource(vertex, ARRAY_SIZE(vertexSource), vertexSource, vertexSourceLen);
        glCompileShader(vertex);

        int status;
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &status);
        if (status == 0)
        {
            GLint len = 0;
            glGetShaderiv(vertex, GL_INFO_LOG_LENGTH, &len);

            std::vector<GLchar> errorLog(len);
            glGetShaderInfoLog(vertex, len, &len, errorLog.data());

            d_stderr2("vertex error: %s", errorLog.data());
            std::abort();
            return;
        }

        const GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
        DISTRHO_SAFE_ASSERT_RETURN(fragment != 0,);

        static constexpr const char* const fragmentSource[] = {
            kShaderHeader,
            SHADERS_LIBREAUDIO_FRAG_DATA,
            src,
        };
        static constexpr const GLint fragmentSourceLen[] = {
            sizeof(kShaderHeader) - 1,
            SHADERS_LIBREAUDIO_FRAG_LEN,
            size,
        };
        glShaderSource(fragment, ARRAY_SIZE(fragmentSource), fragmentSource, fragmentSourceLen);
        glCompileShader(fragment);

        glGetShaderiv(fragment, GL_COMPILE_STATUS, &status);
        if (status == 0)
        {
            GLint len = 0;
            glGetShaderiv(fragment, GL_INFO_LOG_LENGTH, &len);

            std::vector<GLchar> errorLog(len);
            glGetShaderInfoLog(fragment, len, &len, errorLog.data());

            d_stderr2("fragment error: %s", errorLog.data());
            std::abort();
            return;
        }

        glAttachShader(program, fragment);
        glAttachShader(program, vertex);
        glLinkProgram(program);

        glDeleteShader(fragment);
        glDeleteShader(vertex);

        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status == 0)
        {
            GLint len = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

            std::vector<GLchar> errorLog(len);
            glGetProgramInfoLog(program, len, &len, errorLog.data());

            d_stderr2("------------------------------ glGetProgramiv error: %s", errorLog.data());
            std::abort();
            return;
        }

        gl3.program = program;
        gl3.iMouse = glGetUniformLocation(program, "iMouse");
        gl3.iResolution = glGetUniformLocation(program, "iResolution");
        gl3.iTime = glGetUniformLocation(program, "iTime");

        gl3.dpfBounds = glGetAttribLocation(program, "_dpf_bounds");
        gl3.dpfBorderRadius = glGetUniformLocation(program, "_dpf_border_radius");
        gl3.dpfPosition = glGetUniformLocation(program, "_dpf_position");
        gl3.dpfScaleFactor = glGetUniformLocation(program, "_dpf_scale_factor");

        if constexpr (textureSize != 0)
        {
            gl3.dpfTexture = glGetUniformLocation(program, "_dpf_texture_data");
            gl3.dpfWaveformStart = glGetUniformLocation(program, "_dpf_texture_start");

            fTextureData.resize(textureSize, 0.f);

            glBindTexture(GL_TEXTURE_2D, gl3.textures[0]);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // GL_LINEAR
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); // GL_CLAMP_TO_EDGE
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

            static constexpr const float trans[] = { 0.f, 0.f, 0.f, 0.f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, trans);

            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGBA16F_ARB,
                         textureSize,
                         1,
                         0,
                         kSingleChannelFormat,
                         GL_FLOAT,
                         fTextureData.data());

            glBindTexture(GL_TEXTURE_2D, 0);
        }

        if (const uint32_t count = fInterface->getParameterCount())
        {
            gl3.parameterValues = new GLint[count];
            String symbol;

            for (uint32_t i = 0; i < count; ++i)
            {
                gl3.parameterValues[i] = -1;

                const char* const parameterSymbol = fInterface->getParameterSymbol(i);
                DISTRHO_SAFE_ASSERT_UINT_CONTINUE(parameterSymbol != nullptr, i);

                symbol = "u_";
                symbol += parameterSymbol;
                gl3.parameterValues[i] = glGetUniformLocation(program, symbol);
            }
        }
    }

    ~BackgroundShaderWidget() final
    {
        fParent->removeIdleCallback(this);

        if (gl3.program == 0)
            return;

        delete[] gl3.parameterValues;

        glDeleteBuffers(std::size(gl3.buffers), gl3.buffers);
        glDeleteTextures(std::size(gl3.textures), gl3.textures);

        glDeleteProgram(gl3.program);
    }

    std::enable_if_t<textureSize != 0, void> replace(const float values[])
    {
        std::memcpy(fTextureData.data(), values, textureSize * sizeof(float));

        if (! fPendingDisplay)
        {
            fPendingDisplay = true;
            repaint();
        }
    }

    std::enable_if_t<textureSize != 0, void> push(const float value)
    {
        fTextureData[fTextureDataTail++] = value;

        if (fTextureDataTail == fTextureData.size())
            fTextureDataTail = 0;

        if (! fPendingDisplay)
        {
            fPendingDisplay = true;
            repaint();
        }
    }

private:
    void idleCallback() final
    {
        if (! fPendingDisplay)
        {
            fPendingDisplay = true;
            repaint();
        }

        static double last = getApp().getTime();
        if (const double t = getApp().getTime(); t - last > 1)
        {
            last = t;
            d_stdout("average repaint time: %f", fAverageTime * 1000);
        }
    }

    void onDisplay() final
    {
        fPendingDisplay = false;

        const TopLevelWidget* const tlw = getTopLevelWidget();

        const uint width = getWidth();
        const uint height = getHeight();

        const double time = getApp().getTime() - fStartTime;

        if (d_isZero(fStartTime) || time - fLastTime > 5.0)
        {
            fAverageTime = 0;
        }
        else if (d_isZero(fAverageTime))
        {
            fAverageTime = time - fLastTime;
        }
        else
        {
            fAverageTime = ((time - fLastTime) + fAverageTime * 4) * 0.2;
        }

        fLastTime = time;

        glUseProgram(gl3.program);

        glUniform1f(gl3.dpfBorderRadius, fBorderRadius);
        glUniform2f(gl3.dpfPosition, getAbsoluteX(), tlw->getHeight() - height - getAbsoluteY());
        glUniform1f(gl3.dpfScaleFactor, fInterface->getScaleFactor());

        glUniform3f(gl3.iMouse, fMousePos.getX(), fMousePos.getY(), fMouseZ);
        glUniform3f(gl3.iResolution, width, height, 0.f);
        glUniform1f(gl3.iTime, time);

        if constexpr (textureSize != 0)
        {
            glUniform1f(gl3.dpfWaveformStart,
                        static_cast<float>(textureSize - fTextureDataTail - 1) / (textureSize - 1));

            glBindTexture(GL_TEXTURE_2D, gl3.textures[0]);

            glTexSubImage2D(GL_TEXTURE_2D,
                            0,
                            0,
                            0,
                            textureSize,
                            1,
                            kSingleChannelFormat,
                            GL_FLOAT,
                            fTextureData.data());
        }

        if (const uint32_t count = fInterface->getParameterCount())
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                if (gl3.parameterValues[i] >= 0)
                    glUniform1f(gl3.parameterValues[i], fInterface->getParameterValue(i));
            }
        }

        static const constexpr GLfloat vertices[] = { -1, 1, -1, -1, 1, -1, 1, 1 };
        glBindBuffer(GL_ARRAY_BUFFER, gl3.buffers[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(gl3.dpfBounds);
        glVertexAttribPointer(gl3.dpfBounds, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

        static constexpr const GLubyte order[] = { 0, 1, 2, 0, 2, 3 };
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl3.buffers[1]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(order), order, GL_STATIC_DRAW);
        glDrawElements(GL_TRIANGLES, ARRAY_SIZE(order), GL_UNSIGNED_BYTE, nullptr);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(gl3.dpfBounds);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        if constexpr (textureSize != 0)
            glBindTexture(GL_TEXTURE_2D, 0);

        glUseProgram(0);
    }

    bool onMouse(const MouseEvent& ev) final
    {
        if (ev.button == kMouseButtonLeft)
            fMouseZ = ev.press ? 1.f : 0.f;
        return SubWidget::onMouse(ev);
    }

    bool onMotion(const MotionEvent& ev) final
    {
        fMousePos = ev.pos;
        return SubWidget::onMotion(ev);
    }

    void onResize(const ResizeEvent& ev) final
    {
        if (fFirstResize)
        {
            fFirstResize = false;
            fMousePos = Point<double>(ev.size.getWidth() * 0.5, ev.size.getHeight() * 0.5);
        }
        SubWidget::onResize(ev);
    }

    struct {
        GLuint buffers[2];
        GLuint textures[1];
        GLuint program;
        GLint dpfBounds;
        GLint dpfBorderRadius;
        GLint dpfPosition;
        GLint dpfScaleFactor;
        GLint dpfTexture;
        GLint dpfWaveformStart;
        GLint iMouse;
        GLint iResolution;
        GLint iTime;
        GLint* parameterValues;
    } gl3 = {};

    TopLevelWidget* const fParent;

    double fAverageTime = 0;
    double fLastTime = 0;
    const double fStartTime = getApp().getTime();

    std::vector<float> fTextureData;
    uint32_t fTextureDataTail = 0;

    bool fPendingDisplay = true;
    bool fFirstResize = true;
    Point<double> fMousePos;
    float fMouseZ = 0.f;

   #ifdef DISTRHO_OS_WINDOWS
    #define DGL_EXT(PROC, func) PROC func;
    DGL_EXT(PFNGLATTACHSHADERPROC,             glAttachShader)
    DGL_EXT(PFNGLBINDBUFFERPROC,               glBindBuffer)
    DGL_EXT(PFNGLBUFFERDATAPROC,               glBufferData)
    DGL_EXT(PFNGLCOMPILESHADERPROC,            glCompileShader)
    DGL_EXT(PFNGLCREATEPROGRAMPROC,            glCreateProgram)
    DGL_EXT(PFNGLCREATESHADERPROC,             glCreateShader)
    DGL_EXT(PFNGLDELETEBUFFERSPROC,            glDeleteBuffers)
    DGL_EXT(PFNGLDELETEPROGRAMPROC,            glDeleteProgram)
    DGL_EXT(PFNGLDELETESHADERPROC,             glDeleteShader)
    DGL_EXT(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray)
    DGL_EXT(PFNGLENABLEVERTEXATTRIBARRAYPROC,  glEnableVertexAttribArray)
    DGL_EXT(PFNGLGENBUFFERSPROC,               glGenBuffers)
    DGL_EXT(PFNGLGETATTRIBLOCATIONPROC,        glGetAttribLocation)
    DGL_EXT(PFNGLGETPROGRAMINFOLOGPROC,         glGetProgramInfoLog)
    DGL_EXT(PFNGLGETPROGRAMIVPROC,             glGetProgramiv)
    DGL_EXT(PFNGLGETSHADERINFOLOGPROC,         glGetShaderInfoLog)
    DGL_EXT(PFNGLGETSHADERIVPROC,              glGetShaderiv)
    DGL_EXT(PFNGLGETUNIFORMLOCATIONPROC,       glGetUniformLocation)
    DGL_EXT(PFNGLLINKPROGRAMPROC,              glLinkProgram)
    DGL_EXT(PFNGLSHADERSOURCEPROC,             glShaderSource)
    DGL_EXT(PFNGLUNIFORM1FPROC,                glUniform1f)
    DGL_EXT(PFNGLUNIFORM1IPROC,                glUniform1i)
    DGL_EXT(PFNGLUNIFORM2FPROC,                glUniform2f)
    DGL_EXT(PFNGLUNIFORM3FPROC,                glUniform3f)
    DGL_EXT(PFNGLUSEPROGRAMPROC,               glUseProgram)
    DGL_EXT(PFNGLVERTEXATTRIBPOINTERPROC,      glVertexAttribPointer)
    #undef DGL_EXT

    bool initGL()
    {
        #define DGL_EXT(PROC, func) \
            func = (PROC) wglGetProcAddress ( #func ); \
            DISTRHO_SAFE_ASSERT_RETURN(func != nullptr, false);
        DGL_EXT(PFNGLATTACHSHADERPROC,             glAttachShader)
        DGL_EXT(PFNGLBINDBUFFERPROC,               glBindBuffer)
        DGL_EXT(PFNGLBUFFERDATAPROC,               glBufferData)
        DGL_EXT(PFNGLCOMPILESHADERPROC,            glCompileShader)
        DGL_EXT(PFNGLCREATEPROGRAMPROC,            glCreateProgram)
        DGL_EXT(PFNGLCREATESHADERPROC,             glCreateShader)
        DGL_EXT(PFNGLDELETEBUFFERSPROC,            glDeleteBuffers)
        DGL_EXT(PFNGLDELETEPROGRAMPROC,            glDeleteProgram)
        DGL_EXT(PFNGLDELETESHADERPROC,             glDeleteShader)
        DGL_EXT(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray)
        DGL_EXT(PFNGLENABLEVERTEXATTRIBARRAYPROC,  glEnableVertexAttribArray)
        DGL_EXT(PFNGLGENBUFFERSPROC,               glGenBuffers)
        DGL_EXT(PFNGLGETATTRIBLOCATIONPROC,        glGetAttribLocation)
        DGL_EXT(PFNGLGETPROGRAMINFOLOGPROC,        glGetProgramInfoLog)
        DGL_EXT(PFNGLGETPROGRAMIVPROC,             glGetProgramiv)
        DGL_EXT(PFNGLGETSHADERINFOLOGPROC,         glGetShaderInfoLog)
        DGL_EXT(PFNGLGETSHADERIVPROC,              glGetShaderiv)
        DGL_EXT(PFNGLGETUNIFORMLOCATIONPROC,       glGetUniformLocation)
        DGL_EXT(PFNGLLINKPROGRAMPROC,              glLinkProgram)
        DGL_EXT(PFNGLSHADERSOURCEPROC,             glShaderSource)
        DGL_EXT(PFNGLUNIFORM1FPROC,                glUniform1f)
        DGL_EXT(PFNGLUNIFORM1IPROC,                glUniform1i)
        DGL_EXT(PFNGLUNIFORM2FPROC,                glUniform2f)
        DGL_EXT(PFNGLUNIFORM3FPROC,                glUniform3f)
        DGL_EXT(PFNGLUSEPROGRAMPROC,               glUseProgram)
        DGL_EXT(PFNGLVERTEXATTRIBPOINTERPROC,      glVertexAttribPointer)
        #undef DGL_EXT
        return true;
    }
   #endif
};

// --------------------------------------------------------------------------------------------------------------------

} /* namespace LibreAudio */
