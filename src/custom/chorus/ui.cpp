// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// FIXME remove this, helper for IDE
#include "config-custom.h"

// TODO remove use of global vars in bittyhttp

// TODO make port arbitrary, run-time detection
#define WEBSERVER_STATIC_PORT_FIXME 8887

#include "LibreAudioBaseUI.hpp"
#include "LibreAudioParameters.hpp"
#include "LibreAudioStates.hpp"

#include "las-resources.h"

#include "Window.hpp"
#include "extra/Time.hpp"

#include <cerrno>

// --------------------------------------------------------------------------------------------------------------------
// WebServer details

#define MIMETYPE_HTML "text/html"
#define MIMETYPE_CSS "text/css"
#define MIMETYPE_JS "text/javascript"

extern "C" {
#include "WebServer.h"
}

struct WebServerFile {
    const char* filename;
    const char* type;
    const unsigned char* data;
    unsigned int size;
};

static constexpr const WebServerFile kWebServerFiles[] = {
    { "/", MIMETYPE_HTML, CUSTOM_CHORUS_INDEX_HTML_DATA, CUSTOM_CHORUS_INDEX_HTML_LEN },
};

bool FS_GetFileProperties(const char* const filename, struct WSPageProp* const prop)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(kWebServerFiles); ++i)
    {
        const WebServerFile& file = kWebServerFiles[i];

        if (std::strcmp(filename, file.filename)==0)
        {
            prop->NoCache = true;
            prop->DynamicFile = false;
            prop->Cookies = nullptr;
            prop->Gets = nullptr;
            prop->Posts = nullptr;
            prop->FileData = &file;
            return true;
        }
    }

    d_stderr2("File Not Found: %s", filename);
    return false;
}

void FS_SendFile(WebServerContext* const web, const void* const fileData)
{
    const WebServerFile* const filePtr = reinterpret_cast<const WebServerFile*>(fileData);
    DISTRHO_SAFE_ASSERT_RETURN(filePtr != nullptr,);

    const WebServerFile& file = *filePtr;
    d_stdout("Serving File: %s", file.filename);
    WS_WriteWhole(web, file.type, file.data, file.size);
}

t_ElapsedTime ReadElapsedClock()
{
    return d_gettime_ms() / 1000;
}

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

static char* _webserver_error = nullptr;

__attribute__((destructor))
static void _free_webserver_error()
{
    std::free(_webserver_error);
    _webserver_error = nullptr;
}

WebServer* webserver_init()
{
    SocketsCon_InitSocketConSystem();

    if (WebServer* const webServer = WS_Init(WEBSERVER_STATIC_PORT_FIXME))
        return webServer;

    const int err = errno;
    std::free(_webserver_error);
    _webserver_error = strdup(std::strerror(err));
    return nullptr;
}

int webserver_port(WebServer* const webServer)
{
    return WEBSERVER_STATIC_PORT_FIXME;
}

bool webserver_idle(WebServer* const webServer)
{
    WS_Tick(webServer);
    return true;
}

void webserver_close(WebServer* const webServer)
{
    WS_Shutdown(webServer);
    SocketsCon_ShutdownSocketConSystem();
}

// --------------------------------------------------------------------------------------------------------------------

class LibreAudioUI : public LibreAudioBaseUI
{
public:
    LibreAudioUI()
        : LibreAudioBaseUI()
    {
        // if (isDummyInstance())
        //     return;

        if (webServer == nullptr)
        {
            d_stderr2("Failed to start WebServer: %s", _webserver_error);
            return;
        }

        d_stdout("WebServer started with port: %d", webserver_port(webServer));

        WebViewOptions options;
        options.callback = [](void* const arg, char* const msg){
            static_cast<LibreAudioUI*>(arg)->webViewMessageCallback(msg);
        };
        options.callbackPtr = this;
        options.backgroundColor = 0x222222ff;
       // #ifndef NDEBUG
        options.developerToolsEnabled = true;
       // #endif
        options.initialJS = "RunningFromDPF = true;";

        if (! getWindow().createWebView("http://127.0.0.1:8887/", options))
        {
            d_stderr2("Failed to create WebView");
            return;
        }
    }

    ~LibreAudioUI() override
    {
        if (webServer != nullptr)
            webserver_close(webServer);
    }

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // Widget Callbacks

    void uiIdle() final
    {
        LibreAudioBaseUI::uiIdle();

        if (webServer != nullptr && ! webserver_idle(webServer))
        {
            webserver_close(webServer);
            webServer = nullptr;
        }
    }

private:
    WebServer* webServer = webserver_init();
    // bool webviewStarted = false;

    void webViewMessageCallback(char* msg)
    {
        d_stderr2("unknown or invalid web message received: %s", msg);
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
