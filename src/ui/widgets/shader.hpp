// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "shader-clean.hpp"

#include <functional>
#include <list>
#include <string>

namespace LibreAudio {

// --------------------------------------------------------------------------------------------------------------------

class BotShaderBaseWidget : public ShaderBaseWidget
{
public:
    explicit BotShaderBaseWidget(TopLevelWidget* const parent, LabUIWidgetInterface* const iface)
        : ShaderBaseWidget(parent, iface) {}

    // A float uniform that is not a parameter -- something only the UI knows, like an animation's progress.
    // getter is asked once per repaint; a shader that does not declare the uniform simply never sees it.
    void setCustomUniform(const char* const name, std::function<float()> getter)
    {
        fCustomUniforms.push_back(CustomUniform { name, -2, std::move(getter) });
    }

protected:
    struct CustomUniform {
        std::string name;
        GLint location; // -2 until looked up
        std::function<float()> getter;
    };

    std::list<CustomUniform> fCustomUniforms;
};

// --------------------------------------------------------------------------------------------------------------------

template<const char src[], uint size, uint textureSize = 0>
class BotShaderWidget final : public BotShaderBaseWidget,
                              public IdleCallback
{
   #if defined(DGL_USE_OPENGL3) && !defined(DGL_USE_GLES2)
    static constexpr const GLenum kSingleChannelFormat = GL_RED;
   #else
    static constexpr const GLenum kSingleChannelFormat =  GL_LUMINANCE;
   #endif

public:
    explicit BotShaderWidget(TopLevelWidget* const parent, LabUIWidgetInterface* const iface)
        : BotShaderBaseWidget(parent, iface),
          fParent(parent)
    {
        // 8ms was 125 Hz: on a 60 Hz display more than half of those frames were rendered
        // and then thrown away. The shader widgets all cover the same area, so the window
        // redraws at whichever callback is fastest -- this has to stay in step with them.
        parent->addIdleCallback(this, 16);

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
            "#version 100\n"
            "#define LIBREAUDIO_GL2\n"
           #elif defined(DGL_USE_OPENGL3)
            "#version 150 core\n"
            "#define LIBREAUDIO_GL3\n"
           #else
            "#define LIBREAUDIO_GL2\n"
           #endif
            "#define LIBREAUDIO_HOSTED_BOT\n"
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

        // FIXME remove these, rely on iTime instead
        gl3.fixmeLevelSlow = glGetUniformLocation(program, "iLevelSlow");
        gl3.fixmeLevelFast = glGetUniformLocation(program, "iLevelFast");
        gl3.fixmeLevelSlowTime = glGetUniformLocation(program, "iLevelSlowTime");

        gl3.dpfBounds = glGetAttribLocation(program, "_dpf_bounds");
        gl3.dpfBorderRadius = glGetUniformLocation(program, "_dpf_border_radius");
        gl3.dpfPosition = glGetUniformLocation(program, "_dpf_position");
        gl3.dpfScaleFactor = glGetUniformLocation(program, "_dpf_scale_factor");

        if constexpr (textureSize != 0)
        {
            gl3.dpfTextureData = glGetUniformLocation(program, "_dpf_texture_data");
            gl3.dpfTextureSize = glGetUniformLocation(program, "_dpf_texture_size");
            gl3.dpfTextureStart = glGetUniformLocation(program, "_dpf_texture_start");

            fTextureData.resize(textureSize, 0.f);
            fTextureDataTail = textureSize - 1;

            glActiveTexture(GL_TEXTURE0);
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

                // remember the input meters so the level smoothers can follow them
                if (std::strcmp(parameterSymbol, "input_peak_L") == 0)
                    fPeakParameterL = static_cast<int>(i);
                else if (std::strcmp(parameterSymbol, "input_peak_R") == 0)
                    fPeakParameterR = static_cast<int>(i);
            }
        }

        // Shaders cannot smooth anything themselves -- a fragment program keeps no
        // state between frames -- so the two brightness envelopes are integrated
        // here and handed over as plain uniforms. They advance once per repaint,
        // which the idle callback above fixes at 16 ms.
        fLevelSlow.setSampleRate(1.f / kFrameSeconds);
        fLevelSlow.setTimeConstant(kLevelSlowSeconds);
        fLevelSlow.setTargetValue(kLevelSilenceDb);
        fLevelSlow.clearToTargetValue();

        fLevelFast.setSampleRate(1.f / kFrameSeconds);
        fLevelFast.setTimeConstant(kLevelFastSeconds);
        fLevelFast.setTargetValue(kLevelSilenceDb);
        fLevelFast.clearToTargetValue();

        fMouseX.setSampleRate(1.0 / 0.008);
        fMouseX.setTimeConstant(0.5);

        fMouseY.setSampleRate(1.0 / 0.008);
        fMouseY.setTimeConstant(0.5);
    }

    ~BotShaderWidget() final
    {
        fParent->removeIdleCallback(this);

        if (gl3.program == 0)
            return;

        delete[] gl3.parameterValues;

        glDeleteBuffers(std::size(gl3.buffers), gl3.buffers);
        glDeleteTextures(std::size(gl3.textures), gl3.textures);

        glDeleteProgram(gl3.program);
    }

    // std::enable_if_t<textureSize != 0, void>
    void replace(const float values[textureSize])
    {
        // float* const data = fTextureData.data();
        // for (uint32_t i = 0; i < textureSize; ++i)
        //     data[i] = (data[i] + values[i]) * 0.5f;

        std::memcpy(fTextureData.data(), values, textureSize * sizeof(float));

        if (! fPendingDisplay)
        {
            fPendingDisplay = true;
            repaint();
        }
    }

    // std::enable_if_t<textureSize != 0, void>
    void push(const float value)
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
    }

    void onDisplay() final
    {
        fPendingDisplay = false;

        const TopLevelWidget* const tlw = getTopLevelWidget();

        const uint width = getWidth();
        const uint height = getHeight();

        const double time = getApp().getTime() - fStartTime;
        const double frameSeconds = std::clamp<double>(time - fLastTime, 0.0, 0.1);
        fLastTime = time;

        glUseProgram(gl3.program);

        glUniform1f(gl3.dpfBorderRadius, fBorderRadius);
        glUniform2f(gl3.dpfPosition, getAbsoluteX(), tlw->getHeight() - height - getAbsoluteY());
        glUniform1f(gl3.dpfScaleFactor, fInterface->getScaleFactor());

        glUniform3f(gl3.iMouse, fMouseX.next(), fMouseY.next(), fMouseZ);
        glUniform3f(gl3.iResolution, width, height, 0.f);
        glUniform1f(gl3.iTime, time);

        // Peak of the two input meters, in dBFS, run through a slow and a fast
        // envelope. Shaders blend the two to decide how bright to draw.
        {
            float peakDb = kLevelSilenceDb;

            if (fPeakParameterL >= 0)
                peakDb = std::max(peakDb, fInterface->getParameterValue(fPeakParameterL));

            if (fPeakParameterR >= 0)
                peakDb = std::max(peakDb, fInterface->getParameterValue(fPeakParameterR));

            fLevelSlow.setTargetValue(peakDb);
            fLevelFast.setTargetValue(peakDb);

            const float levelSlowDb = fLevelSlow.next();

            // A shader that drives a *rate* from the level -- the starfield flies
            // faster when the input is hot -- cannot just multiply iTime by it:
            // that would move everything already on screen every time the level
            // changed. It needs the integral of the level over time, which, like
            // the envelopes themselves, only the host can keep. Weighted by the
            // normalised level so the shader can scale it into its own units.
            const float levelSlowNorm = std::min(std::max((levelSlowDb - kLevelFloorDb) / (kLevelCeilDb - kLevelFloorDb), 0.f), 1.f);

            // Rates want their own timing, and an asymmetric one: a fly-through
            // picks up speed as the music arrives and coasts back down when it
            // stops, so the fall is the longer of the two. Braking as briskly as
            // it accelerated would read as the picture being yanked back.
            const float levelTimeSeconds = levelSlowNorm >= fLevelSlowHeld ? kLevelTimeAttackSeconds
                                                                           : kLevelTimeReleaseSeconds;

            fLevelSlowHeld += (levelSlowNorm - fLevelSlowHeld)
                            * (1.f - std::exp(-frameSeconds / levelTimeSeconds));

            fLevelSlowTime += fLevelSlowHeld * frameSeconds;

            glUniform1f(gl3.fixmeLevelSlow, levelSlowDb);
            glUniform1f(gl3.fixmeLevelFast, fLevelFast.next());
            glUniform1f(gl3.fixmeLevelSlowTime, fLevelSlowTime);
        }

        if constexpr (textureSize != 0)
        {
            glUniform1i(gl3.dpfTextureData, 0);
            glUniform1i(gl3.dpfTextureSize, textureSize);
            glUniform1f(gl3.dpfTextureStart,
                        static_cast<float>(textureSize - fTextureDataTail - 1) / (textureSize - 1));

            glActiveTexture(GL_TEXTURE0);
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

        for (CustomUniform& uniform : fCustomUniforms)
        {
            if (uniform.location == -2)
                uniform.location = glGetUniformLocation(gl3.program, uniform.name.c_str());
            if (uniform.location >= 0)
                glUniform1f(uniform.location, uniform.getter());
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
        const float w = getWidth();
        const float h = getHeight();
        fMouseX.setTargetValue(w / 2 - ev.pos.getX() / w * (w / 4));
        fMouseY.setTargetValue(h / 2 + ev.pos.getY() / h * (h / 4));
        return SubWidget::onMotion(ev);
    }

    void onPositionChanged(const PositionChangedEvent& ev) final
    {
        fMouseX.setTargetValue(getWidth() * 0.5f);
        fMouseY.setTargetValue(getHeight() * 0.5f);
        fMouseX.clearToTargetValue();
        fMouseY.clearToTargetValue();
        SubWidget::onPositionChanged(ev);
    }

    void onResize(const ResizeEvent& ev) final
    {
        fMouseX.setTargetValue(ev.size.getWidth() * 0.5f);
        fMouseY.setTargetValue(ev.size.getHeight() * 0.5f);
        if (fFirstResize)
        {
            fFirstResize = false;
            fMouseX.clearToTargetValue();
            fMouseY.clearToTargetValue();
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
        GLint dpfTextureData;
        GLint dpfTextureSize;
        GLint dpfTextureStart;
        GLint iMouse;
        GLint iResolution;
        GLint iTime;
        GLint fixmeLevelSlow;
        GLint fixmeLevelFast;
        GLint fixmeLevelSlowTime;
        GLint* parameterValues;
    } gl3 = {};

    // Brightness envelope timing, plus the level stood in for silence (the input
    // meters bottom out at -70 dBFS).
    //
    // These are T60 -- time to cover 99.9% of a step -- which is a lot brisker than
    // it reads: ExponentialValueSmoother divides by 6.91 internally, so the actual
    // one-pole tau is T60/6.91 and most of the movement lands in the first seventh
    // of the quoted time. 1.5 s here is a tau of ~0.22 s, which reads as "follows
    // the phrase" rather than the twitch that 0.5 s gave.
    static constexpr const float kLevelSlowSeconds = 5.0f;
    static constexpr const float kLevelFastSeconds = 1.5f;
    static constexpr const float kLevelSilenceDb = -70.0f;

    // Nominal repaint period, matching the idle callback.
    static constexpr const float kFrameSeconds = 0.016f;

    // The window iLevelSlowTime normalises the slow envelope over, in dBFS.
    // Shaders reading it must use the same one -- it is meterFloorDb/meterCeilDb
    // in shadertoy-cloudstarfield.frag and friends.
    static constexpr const float kLevelFloorDb = -40.0f;
    static constexpr const float kLevelCeilDb  =   0.0f;

    // How quickly iLevelSlowTime takes a level up, and how slowly it gives one
    // back. Unlike the envelopes above these are plain one-pole taus, not T60s:
    // each covers 63% of the distance in the time quoted and all but a twentieth
    // of it in three times that.
    //
    // They sit on top of iLevelSlow's own smoothing rather than replacing it, so
    // the rise is the two in series -- an attack of 0 here would still not be
    // instant. Keep the attack the shorter of the two: arriving with the music and
    // outlasting it is the asymmetry the flight wants.
    static constexpr const float kLevelTimeAttackSeconds  = 1.0f;
    static constexpr const float kLevelTimeReleaseSeconds = 2.0f;

    TopLevelWidget* const fParent;

    const double fStartTime = getApp().getTime();
    double fLastTime = fStartTime;

    std::vector<float> fTextureData;
    uint32_t fTextureDataTail = 0;

    bool fPendingDisplay = true;
    bool fFirstResize = true;
    ExponentialValueSmoother fLevelSlow;
    ExponentialValueSmoother fLevelFast;
    float fLevelSlowTime = 0.f;
    float fLevelSlowHeld = 0.f;
    int fPeakParameterL = -1;
    int fPeakParameterR = -1;
    LinearValueSmoother fMouseX;
    LinearValueSmoother fMouseY;
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
