//#pragma once
//
//#include "EditorWindow.h"
//#include "Events/Event.h"
//#include "Events/MouseEvent.h"
//#include "Core/Timestep.h"
//#include "Math/HorizonMath.h"
//
//namespace Horizon
//{
//    class SceneWindow : public EditorWindow
//    {
//    public:
//        SceneWindow(const String& title, Editor* editor);
//        ~SceneWindow() = default;
//        void Draw() override;
//        void OnEvent(Event& e);
//        bool OnMouseScroll(MouseScrollEvent& e);
//        bool OnMouseMove(MouseMoveEvent& e);
//        bool OnMouseButtonPress(MouseButtonPressEvent& e);
//        bool IsInViewport(const Vector2& pos);
//        bool IsFocused() { return mIsFocused; }
//        bool Pick(MouseButtonPressEvent& e);
//        void SetGizmoOperationType(int type) { mGizmoOperationType = type; }
//        const Vector2& GetViewportSize() const { return mViewportSize; }
//    private:
//        float GetSnapValue();
//        bool mIsFocused = true;
//        int mGizmoOperationType;
//        Vector2 mViewportPos;
//        Vector2 mViewportSize;
//    };
//}
#pragma once

#include "Engine/HorizonEngineModule.h"

#include "HorizonEditor.h"

namespace Horizon
{
    class SceneViewWindow
    {
    public:
        SceneViewWindow(const std::string& title, HorizonEditor* editor);
        ~SceneViewWindow() = default;
        void OnImGuiRender(bool& isOpen);
        //void OnEvent(Event& e);
        //bool OnMouseScroll(MouseScrollEvent& e);
        //bool OnMouseMove(MouseMoveEvent& e);
        //bool OnMouseButtonPress(MouseButtonPressEvent& e);
        //bool Pick(MouseButtonPressEvent& e);
        bool IsInViewport(const Vector2f& pos);
        bool IsFocused() { return isFocused; }
        void SetGizmoOperationType(int type) { gizmoOperationType = type; }
        const Vector2f& GetViewportSize() const { return viewportSize; }
    private:
        float GetSnapValue();
        HorizonEditor* editor;
        std::string title;
        bool isFocused = true;
        int gizmoOperationType;
        Vector2f viewportPos;
        Vector2f viewportSize;
    };
}
