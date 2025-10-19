#include "WindowSystem.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <backends/imgui_impl_glfw.h>

#include <stb/stb_image.h>

namespace Horizon
{
    static void ErrorCallback(int errorCode, const char* description)
    {
        LogError(GLogger, std::format("GLFW error occurs. [error code]: {}, [desctiption]: {}.", errorCode, description));
    }

    bool WindowSystemInit()
    {
        glfwSetErrorCallback(ErrorCallback);
        if (glfwInit() != GLFW_TRUE)
        {
            LogError(GLogger, std::format("Failed to init glfw."));
            return false;
        }
        return true;
    }

    void WindowSystemExit()
    {
        glfwTerminate();
    }

    Window::Window(WindowCreateInfo* info)
        : handle(nullptr)
        , width(0)
        , height(0)
        , title(nullptr)
        , focused(false)
        , state(WindowState::Unknown)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        if (info->flags & HORIZON_WINDOW_CREATE_FLAG_BIT_MAXIMIZED)
        {
            glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
        }

        if (info->flags & HORIZON_WINDOW_CREATE_FLAG_BIT_RESIZABLE)
        {
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        }
        else
        {
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        }

        if (info->flags & HORIZON_WINDOW_CREATE_FLAG_BIT_BORDERLESS)
        {
            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        }
        else
        {
            glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        }

        GLFWmonitor* primaryMonitor = nullptr;
        if (info->flags & HORIZON_WINDOW_CREATE_FLAG_BIT_FULLSCREEN)
        {
            primaryMonitor = glfwGetPrimaryMonitor();
        }

        GLFWwindow* glfwWindow = glfwCreateWindow(std::max(1u, info->width), std::max(1u, info->height), info->title, primaryMonitor, nullptr);
        if (!glfwWindow)
        {
            LogFatal(GLogger, std::format("Failed to create main window"));
            return;
        }

        handle = glfwWindow;
        title = info->title;

        int32 w, h;
        glfwGetWindowSize(glfwWindow, &w, &h);
        width = w;
        height = h;

        glfwShowWindow(glfwWindow);
        glfwFocusWindow(glfwWindow);
        focused = true;

        UpdateWindowState();

        if (info->icon)
        {
            SetClassLongPtr(glfwGetWin32Window(glfwWindow), GCLP_HICON, (LONG_PTR)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(101)));
        }

        glfwSetWindowUserPointer(glfwWindow, this);
        glfwSetWindowFocusCallback(glfwWindow, [](GLFWwindow* glfwWindow, int focused)
        {
            Window* window = (Window*)glfwGetWindowUserPointer(glfwWindow);
            window->focused = (focused == GLFW_TRUE) ? true : false;
        });
        glfwSetWindowSizeCallback(glfwWindow, [](GLFWwindow* glfwWindow, int width, int height)
        {
            Window* window = (Window*)glfwGetWindowUserPointer(glfwWindow);
            glfwSetWindowSize(glfwWindow, width, height);
            int32 w, h;
            glfwGetWindowSize(glfwWindow, &w, &h);
            window->width = w;
            window->height = h;

            window->UpdateWindowState();
        });
        glfwSetWindowMaximizeCallback(glfwWindow, [](GLFWwindow* glfwWindow, int maximized)
        {
            Window* window = (Window*)glfwGetWindowUserPointer(glfwWindow);
            if (maximized == GLFW_TRUE)
            {
                window->state = WindowState::Maximized;
            }
        });
        glfwSetWindowCloseCallback(glfwWindow, [](GLFWwindow* glfwWindow)
        {
            glfwSetWindowShouldClose(glfwWindow, true);
        });
        glfwSetKeyCallback(glfwWindow, [](GLFWwindow* glfwWindow, int key, int scancode, int action, int modifiers)
        {
            Window* window = (Window*)glfwGetWindowUserPointer(glfwWindow);
            switch (action)
            {
            case GLFW_PRESS:
            {
                if (window->keyPressEventCallback)
                {
                    window->keyPressEventCallback((KeyCode)key, false);
                }
                break;
            }
            case GLFW_RELEASE:
            {
                if (window->keyReleaseEventCallback)
                {
                    window->keyReleaseEventCallback((KeyCode)key);
                }
                break;
            }
            case GLFW_REPEAT:
            {
                if (window->keyPressEventCallback)
                {
                    window->keyPressEventCallback((KeyCode)key, true);
                }
                break;
            }
            }
        });
        glfwSetMouseButtonCallback(glfwWindow, [](GLFWwindow* glfwWindow, int button, int action, int mods)
        {
            Window* window = (Window*)glfwGetWindowUserPointer(glfwWindow);
            switch (action)
            {
            case GLFW_PRESS:
            {
                if (window->mouseButtonPressEventCallback)
                {
                    window->mouseButtonPressEventCallback((MouseButtonID)button);
                }
                break;
            }
            case GLFW_RELEASE:
            {
                if (window->mouseButtonReleaseEventCallback)
                {
                    window->mouseButtonReleaseEventCallback((MouseButtonID)button);
                }
                break;
            }
            }
        });
    }

    Window::~Window()
    {
        GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(handle);
        if (glfwWindow)
        {
            glfwDestroyWindow(glfwWindow);
        }
    }

    uint64 Window::GetNativeHandle()
    {
        GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(handle);
        return (uint64)glfwGetWin32Window(glfwWindow);
    }

    void Window::ProcessEvents()
    {
        OPTICK_EVENT();

        // @todo glfwPollEvents() is so slow, move it to another thread.
        glfwPollEvents();
    }

    bool Window::ShouldClose() const
    {
        GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(handle);
        return glfwWindowShouldClose(glfwWindow);
    }

    void Window::SetFullscreen(bool fullscreen)
    {
        GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(handle);
        if (fullscreen)
        {
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(glfwWindow, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
        else
        {
            glfwSetWindowMonitor(glfwWindow, nullptr, 0, 0, width, height, GLFW_DONT_CARE);
        }
    }

    void Window::SetWindowSize(uint32 width, uint32 height)
    {
        GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(handle);

        glfwSetWindowSize(glfwWindow, width, height);

        int32 w, h;
        glfwGetWindowSize(glfwWindow, &w, &h);
        this->width = w;
        this->height = h;
    }

    void Window::MaximizeWindow()
    {
        GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(handle);
        glfwMaximizeWindow(glfwWindow);
    }

    void Window::UpdateWindowState()
    {
        GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(handle);
        GLFWmonitor* monitor = glfwGetWindowMonitor(glfwWindow);
        if (width == 0 || height == 0)
        {
            state = WindowState::Minimized;
        }
        else if (monitor != nullptr)
        {
            state = WindowState::Fullscreen;
        }
        else
        {
            int maximized = glfwGetWindowAttrib(glfwWindow, GLFW_MAXIMIZED);
            if (maximized)
            {
                state = WindowState::Maximized;
            }
            else
            {
                state = WindowState::Normal;
            }
        }
    }

    void Window::InitForImGui()
    {
        GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(handle);
        ImGui_ImplGlfw_InitForOther(glfwWindow, true);
    }
}