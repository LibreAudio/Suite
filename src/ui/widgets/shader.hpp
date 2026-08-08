// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "Application.hpp"
#include "SubWidget.hpp"
#include "TopLevelWidget.hpp"

#include "las-resources.h"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioBackgroundShaderWidget final : public SubWidget,
                                               public IdleCallback
{
public:
    explicit LibreAudioBackgroundShaderWidget(TopLevelWidget* const parent)
        : SubWidget(parent)
    {
        parent->addIdleCallback(this);

        const GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
        DISTRHO_SAFE_ASSERT_RETURN(vertex != 0,);

        const GLuint program = glCreateProgram();
        DISTRHO_SAFE_ASSERT_RETURN(program != 0,);

        glGenBuffers(2, gl3.buffers);

        static constexpr const unsigned char* const vertexSource[] = {
            SHADERS_LIBREAUDIO_VERT_DATA,
        };
        static constexpr const GLint vertexSourceLen[] = {
            SHADERS_LIBREAUDIO_VERT_LEN,
        };
        glShaderSource(vertex, ARRAY_SIZE(vertexSource), reinterpret_cast<const GLchar* const*>(vertexSource), vertexSourceLen);
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

        static constexpr const unsigned char* const fragmentSource[] = {
            SHADERS_LIBREAUDIO_HEADER_FRAG_DATA,
            SHADERS_SHADERTOY_AURORA_FRAG_DATA,
        };
        static constexpr const GLint fragmentSourceLen[] = {
            SHADERS_LIBREAUDIO_HEADER_FRAG_LEN,
            SHADERS_SHADERTOY_AURORA_FRAG_LEN,
        };
        glShaderSource(fragment, ARRAY_SIZE(fragmentSource), reinterpret_cast<const GLchar* const*>(fragmentSource), fragmentSourceLen);
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
        gl3.iResolution = glGetUniformLocation(program, "iResolution");
        gl3.iTime = glGetUniformLocation(program, "iTime");
        gl3.dpfBounds = glGetAttribLocation(program, "_dpf_bounds");
        gl3.dpfBorderRadius = glGetUniformLocation(program, "_dpf_border_radius");
        gl3.dpfPosition = glGetUniformLocation(program, "_dpf_position");
    }

    void setBorderRadius(const uint borderRadius) noexcept
    {
        if (fBorderRadius == borderRadius)
            return;
        fBorderRadius = borderRadius;
        repaint();
    }

private:
    void idleCallback() final
    {
        repaint();
    }

    void onDisplay() final
    {
        const TopLevelWidget* const tlw = getTopLevelWidget();

        const uint width = getWidth();
        const uint height = getHeight();

        glUseProgram(gl3.program);

        glUniform2f(gl3.dpfPosition, getAbsoluteX(), tlw->getHeight() - height - getAbsoluteY());
        glUniform2f(gl3.iResolution, width, height);

        glUniform1f(gl3.iTime, getApp().getTime());
        glUniform1f(gl3.dpfBorderRadius, fBorderRadius);

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

        glUseProgram(0);
    }

private:
    struct {
        GLuint buffers[2];
        GLuint program;
        GLint dpfBounds;
        GLint dpfBorderRadius;
        GLint dpfPosition;
        GLint iResolution;
        GLint iTime;
    } gl3;
    uint fBorderRadius = 0;
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
