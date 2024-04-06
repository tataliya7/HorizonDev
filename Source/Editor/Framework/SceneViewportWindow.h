#pragma once

#include <HorizonEngine.h>

#include "HorizonEditor.h"

namespace HE
{
    class SceneViewportWindow
    {
    public:
        SceneViewportWindow(const std::string& title, HorizonEditor* editor);
        ~SceneViewportWindow() = default;
        void OnImGuiRender(bool& isOpen);
        //void OnEvent(Event& e);
        //bool OnMouseScroll(MouseScrollEvent& e);
        //bool OnMouseMove(MouseMoveEvent& e);
        //bool OnMouseButtonPress(MouseButtonPressEvent& e);
        //bool Pick(MouseButtonPressEvent& e);
        bool IsInViewport(const Vector2& pos);
        bool IsFocused() { return isFocused; }
        void SetGizmoOperationType(int type) { gizmoOperationType = type; }
        const Vector2& GetViewportSize() const { return viewportSize; }
    private:
        float GetSnapValue();
        HorizonEditor* editor;
        std::string title;
        bool isFocused = true;
        int gizmoOperationType;
        Vector2 viewportPos;
        Vector2 viewportSize;
    };
}
