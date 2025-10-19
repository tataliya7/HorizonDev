#pragma once

#include "Engine/HorizonEngineModule.h"

namespace Horizon
{
    enum WindowCreateFlagBits : uint32
    {
        HORIZON_WINDOW_CREATE_FLAG_BIT_NONE          = 0,
        HORIZON_WINDOW_CREATE_FLAG_BIT_RESIZABLE     = 1 << 0,
        HORIZON_WINDOW_CREATE_FLAG_BIT_BORDERLESS    = 1 << 1,
        HORIZON_WINDOW_CREATE_FLAG_BIT_MAXIMIZED     = 1 << 2,
        HORIZON_WINDOW_CREATE_FLAG_BIT_FULLSCREEN    = 1 << 3,
    };
    using WindowCreateFlags = uint32;

    struct WindowCreateInfo
    {
        uint32 width;
        uint32 height;
        const char* title;
        const char* icon;
        WindowCreateFlags flags;
    };

    enum class WindowState : uint8
    {
        Unknown,
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

        void ProcessEvents();

        bool ShouldClose() const;

        bool IsFocused() const
        {
            return focused;
        }

        void MaximizeWindow();

        void SetFullscreen(bool fullscreen);

        void SetWindowSize(uint32 width, uint32 height);

        using KeyPressEventCallback = std::function<void(KeyCode, bool)>;
        using KeyReleaseEventCallback = std::function<void(KeyCode)>;
        using MouseButtonPressEventCallback = std::function<void(MouseButtonID)>;
        using MouseButtonReleaseEventCallback = std::function<void(MouseButtonID)>;

        KeyPressEventCallback keyPressEventCallback;
        KeyReleaseEventCallback keyReleaseEventCallback;
        MouseButtonPressEventCallback mouseButtonPressEventCallback;
        MouseButtonReleaseEventCallback mouseButtonReleaseEventCallback;

        void InitForImGui();

    //private:

        void UpdateWindowState();
        void* handle;
        uint32 width;
        uint32 height;
        const char* title;
        bool focused;
        WindowState state;
    };

    extern bool WindowSystemInit();
    extern void WindowSystemExit();
}