#pragma once

#include <HorizonEngine.h>

struct GLFWwindow;

namespace HE
{
    enum WindowCreateFlags
    {
        HORIZON_EXAMPLE_WINDOW_CREATE_FLAG_BIT_NONE = 0,
        HORIZON_EXAMPLE_WINDOW_CREATE_FLAG_BIT_RESIZABLE = 1 << 0,
        HORIZON_EXAMPLE_WINDOW_CREATE_FLAG_BIT_BORDERLESS = 1 << 1,
        HORIZON_EXAMPLE_WINDOW_CREATE_FLAG_BIT_MAXIMIZED = 1 << 2,
        HORIZON_EXAMPLE_WINDOW_CREATE_FLAG_BIT_FULLSCREEN = 1 << 3,
    };

    struct WindowCreateInfo
    {
        uint32 width;
        uint32 height;
        const char* title;
        const char* icon;
        WindowCreateFlags flags;
    };

    enum class WindowState
    {
        Normal,
        Minimized,
        Maximized,
        Fullscreen,
    };

    class Window
    {
    public:

        Window(WindowCreateInfo* info);
        virtual ~Window();

        uint32 GetWidth() const
        {
            return width;
        }

        uint32 GetHeight() const
        {
            return height;
        }

        WindowState GetState() const
        {
            return state;
        }

        uint64 GetNativeHandle();

        GLFWwindow* GetGLFWHandle()
        {
            return handle;
        }

        void ProcessEvents();

        bool ShouldClose() const;

        bool IsFocused() const
        {
            return focused;
        }

        void MaximizeWindow();

        void SetFullscreen(bool fullscreen);

        using KeyPressEventCallback = std::function<void(KeyCode, bool)>;
        using KeyReleaseEventCallback = std::function<void(KeyCode)>;
        using MouseButtonPressEventCallback = std::function<void(MouseButtonID)>;
        using MouseButtonReleaseEventCallback = std::function<void(MouseButtonID)>;

        KeyPressEventCallback keyPressEventCallback;
        KeyReleaseEventCallback keyReleaseEventCallback;
        MouseButtonPressEventCallback mouseButtonPressEventCallback;
        MouseButtonReleaseEventCallback mouseButtonReleaseEventCallback;

    private:

        void UpdateWindowState();

        GLFWwindow* handle;
        uint32 width;
        uint32 height;
        const char* title;
        bool focused;
        WindowState state;
    };

    extern bool GLFWInit();
    extern void GLFWExit();
}