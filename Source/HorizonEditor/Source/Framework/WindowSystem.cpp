#include "WindowSystem.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <stb/stb_image.h>

#include <windows.h>

#include <optick.h>

namespace Horizon
{
    static void ErrorCallback(int errorCode, const char* description)
    {
        LogError(GLogger, std::format("GLFW error occurs. [error code]: {}, [desctiption]: {}.", errorCode, description));
    }

    bool GLFWInit()
    {
        glfwSetErrorCallback(ErrorCallback);
        if (glfwInit() != GLFW_TRUE)
        {
            LogError(GLogger, std::format("Failed to init glfw."));
            return false;
        }
        return true;
    }

    void GLFWExit()
    {
        glfwTerminate();
    }

    Window::Window(WindowCreateInfo* info)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        if (info->flags & HORIZON_EXAMPLE_WINDOW_CREATE_FLAG_BIT_MAXIMIZED)
        {
            glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
        }

        if (info->flags & HORIZON_EXAMPLE_WINDOW_CREATE_FLAG_BIT_RESIZABLE)
        {
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        }
        else
        {
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        }

        if (info->flags & HORIZON_EXAMPLE_WINDOW_CREATE_FLAG_BIT_BORDERLESS)
        {
            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        }
        else
        {
            glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        }

        GLFWmonitor* primaryMonitor = nullptr;
        if (info->flags & HORIZON_EXAMPLE_WINDOW_CREATE_FLAG_BIT_FULLSCREEN)
        {
            primaryMonitor = glfwGetPrimaryMonitor();
        }

        handle = glfwCreateWindow(info->width, info->height, info->title, primaryMonitor, nullptr);
        if (!handle)
        {
            LogFatal(GLogger, std::format("Failed to create main window"));
            return;
        }

        title = info->title;

        int32 w, h;
        glfwGetWindowSize(handle, &w, &h);
        width = w;
        height = h;

        glfwShowWindow(handle);
        glfwFocusWindow(handle);
        focused = true;

        UpdateWindowState();

        if (info->icon)
        {
            int iw = 0, ih = 0, c = 0;
            stbi_set_flip_vertically_on_load(false);
            unsigned char* data = stbi_load(info->icon, &iw, &ih, &c, STBI_rgb_alpha);

            GLFWimage glfwImage = {
                .width = iw,
                .height = ih,
                .pixels = data
            };
            glfwSetWindowIcon(handle, 1, &glfwImage);

            stbi_image_free(data);
        }

        glfwSetWindowUserPointer(handle, this);
        glfwSetWindowFocusCallback(handle, [](GLFWwindow* glfwWindow, int focused)
        {
            Window* window = (Window*)glfwGetWindowUserPointer(glfwWindow);
            window->focused = (focused == GLFW_TRUE) ? true : false;
        });
        glfwSetWindowSizeCallback(handle, [](GLFWwindow* glfwWindow, int width, int height)
        {
            Window* window = (Window*)glfwGetWindowUserPointer(glfwWindow);
            glfwSetWindowSize(glfwWindow, width, height);
            int32 w, h;
            glfwGetWindowSize(glfwWindow, &w, &h);
            window->width = w;
            window->height = h;

            window->UpdateWindowState();
        });
        glfwSetWindowMaximizeCallback(handle, [](GLFWwindow* glfwWindow, int maximized)
        {
            Window* window = (Window*)glfwGetWindowUserPointer(glfwWindow);
            if (maximized == GLFW_TRUE)
            {
                window->state = WindowState::Maximized;
            }
        });
        glfwSetWindowCloseCallback(handle, [](GLFWwindow* glfwWindow)
        {
            glfwSetWindowShouldClose(glfwWindow, true);
        });
        glfwSetKeyCallback(handle, [](GLFWwindow* glfwWindow, int key, int scancode, int action, int modifiers)
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
        glfwSetMouseButtonCallback(handle, [](GLFWwindow* glfwWindow, int button, int action, int mods)
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
        if (handle)
        {
            glfwDestroyWindow(handle);
        }
    }

    uint64 Window::GetNativeHandle()
    {
        return (uint64)glfwGetWin32Window(handle);
    }

    void Window::ProcessEvents()
    {
        OPTICK_EVENT();
        glfwPollEvents();
    }

    bool Window::ShouldClose() const
    {
        return glfwWindowShouldClose(handle);
    }

    void Window::SetFullscreen(bool fullscreen)
    {
        if (fullscreen)
        {
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(handle, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
        else
        {
            glfwSetWindowMonitor(handle, nullptr, 0, 0, width, height, GLFW_DONT_CARE);
        }
    }

    void Window::MaximizeWindow()
    {
        glfwMaximizeWindow(handle);
    }

    void Window::UpdateWindowState()
    {
        GLFWmonitor* monitor = glfwGetWindowMonitor(handle);
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
            int maximized = glfwGetWindowAttrib(handle, GLFW_MAXIMIZED);
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
}