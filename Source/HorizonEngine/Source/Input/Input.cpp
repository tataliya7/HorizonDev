#include "Input.h"

// @todo
#include "Engine/Core/WindowSystem.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace Horizon::Input
{
    Window* CurrentWindow = nullptr;

    void SetCurrentContext(Window* window)
    {
        CurrentWindow = window;
    }

    bool GetKeyDown(KeyCode key)
    {
        GLFWwindow* glfwWindow = (GLFWwindow*)(CurrentWindow->handle);
        int state = glfwGetKey(glfwWindow, (int)key);
        return state == GLFW_PRESS;
    }

    bool GetKeyUp(KeyCode key)
    {
        GLFWwindow* glfwWindow = (GLFWwindow*)(CurrentWindow->handle);
        int state = glfwGetKey(glfwWindow, (int)key);
        return state == GLFW_RELEASE;
    }

    bool GetMouseButtonDown(MouseButtonID button)
    {
        GLFWwindow* glfwWindow = (GLFWwindow*)(CurrentWindow->handle);
        int state = glfwGetMouseButton(glfwWindow, (int)button);
        return state == GLFW_PRESS;
    }

    bool GetMouseButtonUp(MouseButtonID button)
    {
        GLFWwindow* glfwWindow = (GLFWwindow*)(CurrentWindow->handle);
        int state = glfwGetMouseButton(glfwWindow, (int)button);
        return state == GLFW_RELEASE;
    }

    void GetMousePosition(float& x, float& y)
    {
        GLFWwindow* glfwWindow = (GLFWwindow*)(CurrentWindow->handle);

        double xpos, ypos;
        glfwGetCursorPos(glfwWindow, &xpos, &ypos);
        x = (float)xpos;
        y = (float)ypos;
    }
}