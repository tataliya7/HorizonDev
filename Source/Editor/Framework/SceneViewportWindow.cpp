#include "SceneViewportWindow.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <ImGuizmo.h>

namespace HE
{
    SceneViewportWindow::SceneViewportWindow(const std::string& title, HorizonEditor* editor)
        : title(title)
        , editor(editor)
        , viewportPos(0, 0)
        , viewportSize(0, 0)
        , gizmoOperationType(ImGuizmo::OPERATION::TRANSLATE)
    {

    }

    //bool SceneViewportWindow::Pick(MouseButtonPressEvent& e)
    //{
    //    bool handled = false;
    //    if (e.GetMouseButtonID() == MouseButtonID::ButtonLeft && IsInViewport(e.GetScreenPos()))
    //    {
    //        const Vector2& screenPos = e.GetScreenPos();
    //        Vector2 windowPos = screenPos - mViewportPos;
    //        mEditor->OnSeleted(mEditor->mSceneContext.sceneRenderer->Pick(windowPos));
    //        // LOG_DEBUG("mouse pos in scene window: {}, {}. entity handle: {}.", (int32)windowPos.x, (int32)windowPos.y, pickingResult - 1);
    //    }
    //    return handled;
    //}

    //void SceneViewportWindow::OnEvent(Event& e)
    //{
    //    if (ImGuizmo::IsOver())
    //    {
    //        return;
    //    }
    //    {
    //        EventDispatcher dispatcher(e);
    //        dispatcher.Dispatch<MouseButtonPressEvent>(HORIZON_BIND_FUNCTION(SceneWindow::Pick));
    //    }
    //    if (e.IsHandled())
    //    {
    //        return;
    //    }
    //    switch (mEditor->mSceneContext.editorCameraControllerType)
    //    {
    //    case EditorCameraControllerType::Orbiter:
    //    {
    //        if (!mEditor->mSceneContext.orbiterCameraController->IsControlling())
    //        {
    //            EventDispatcher dispatcher(e);
    //            dispatcher.Dispatch<MouseMoveEvent>(HORIZON_BIND_FUNCTION(SceneWindow::OnMouseMove));
    //            dispatcher.Dispatch<MouseScrollEvent>(HORIZON_BIND_FUNCTION(SceneWindow::OnMouseScroll));
    //            dispatcher.Dispatch<MouseButtonPressEvent>(HORIZON_BIND_FUNCTION(SceneWindow::OnMouseButtonPress));
    //        }
    //        if (!e.mHandled)
    //        {
    //            mEditor->mSceneContext.orbiterCameraController->OnEvent(e);
    //        }
    //        break;
    //    }
    //    case EditorCameraControllerType::FirstPerson:
    //    {
    //        mEditor->mSceneContext.firstPersonCameraController->OnEvent(e);
    //        break;
    //    }
    //    default:
    //        break;
    //    }
    //
    //}

    //bool SceneViewportWindow::OnMouseScroll(MouseScrollEvent& e)
    //{
    //    bool handled = false;
    //    if (!IsInViewport(e.GetScreenPos()))
    //    {
    //        handled = true;
    //    }
    //    return handled;
    //}

    //bool SceneViewportWindow::OnMouseMove(MouseMoveEvent& e)
    //{
    //    bool handled = false;
    //    if (!IsInViewport(e.GetScreenPos()))
    //    {
    //        handled = true;
    //    }
    //    return handled;
    //}

    //bool SceneViewportWindow::OnMouseButtonPress(MouseButtonPressEvent& e)
    //{
    //    bool handled = false;
    //    if (!IsInViewport(e.GetScreenPos()))
    //    {
    //        handled = true;
    //    }
    //    return handled;
    //}

    bool SceneViewportWindow::IsInViewport(const Vector2& pos)
    {
        if ((pos.x > viewportPos.x) &&
            (pos.x < (viewportPos.x + viewportPos.x)) &&
            (pos.y > viewportPos.y) &&
            (pos.y < (viewportPos.y + viewportPos.y)))
        {
            return true;
        }
        return false;
    }

    float SceneViewportWindow::GetSnapValue()
    {
        switch (gizmoOperationType)
        {
            case  ImGuizmo::OPERATION::TRANSLATE: return 5.0f;
            case  ImGuizmo::OPERATION::ROTATE: return 10.0f;
            case  ImGuizmo::OPERATION::SCALE: return 0.1f;
        }
        return 0.0f;
    }

    void SceneViewportWindow::OnImGuiRender(bool& isOpen)
    {
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_MenuBar;

        //ImGui::SetNextWindowBgAlpha(0.0f);

        //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        //
        if (ImGui::Begin(title.c_str(), &isOpen, flags))
        {
        //    isFocused = ImGui::IsWindowFocused();

            if (ImGui::BeginMenuBar())
            {
                if (ImGui::Button("T"))
                {
                    gizmoOperationType = ImGuizmo::OPERATION::TRANSLATE;
                }
                if (ImGui::Button("R"))
                {
                    gizmoOperationType = ImGuizmo::OPERATION::ROTATE;
                }
                if (ImGui::Button("S"))
                {
                    gizmoOperationType = ImGuizmo::OPERATION::SCALE;
                }

                auto ToolbarButton = [](const char* id, RenderBackendTextureHandle icon)
                {
                    uint32 width = 32;
                    uint32 height = 32;

                    const bool clicked = ImGui::Button(id, ImVec2((float)width, (float)height));

                    auto* drawList = ImGui::GetWindowDrawList();
                    drawList->AddImage(icon.ToUnit64(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), IM_COL32(255, 255, 255, 255));

                    return clicked;
                };

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7255f, 0.8314f, 0.949f, 0.4f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7255f, 0.8314f, 0.949f, 1.0f));
                if (ToolbarButton("##Play", editor->playButtonIcon))
                {
                    /*if (editor->sceneViewportState == HorizonEditor::SceneViewportState::Edit)
                        OnScenePlay();
                    else if (m_SceneState != SceneState::Simulate)
                        OnSceneStop();*/
                }
                if (ToolbarButton("##Stop", editor->stopButtonIcon))
                {
                    printf("???");
                }
                if (ToolbarButton("##Pause", editor->pauseButtonIcon))
                {
                }
                ImGui::PopStyleColor(3);

                ImGui::EndMenuBar();
            }

    //            ImGui::Text("FPS: %.1f (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
    //            ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
    //            float lineHeight = ImGui::GetFont()->FontSize + ImGui::GetCurrentContext()->Style.FramePadding.y * 2.0f;
    //            ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
    ///*            if (ImGui::Button("Editor Camera"))
    //            {
    //                ImGui::OpenPopup("Editor Camera");
    //            }
    //            if (ImGui::BeginPopup("Editor Camera"))
    //            {
    //                static const char* types[] = { "Orbiter", "FirstPerson" };
    //                int item_type = (int)editor->mSceneContext.editorCameraControllerType;
    //                if (ImGui::Combo("Camera Controller Type", &item_type, types, IM_ARRAYSIZE(types), IM_ARRAYSIZE(types)))
    //                {
    //                    mEditor->mSceneContext.editorCameraControllerType = (EditorCameraControllerType)item_type;
    //                }

    //                PerspectiveCamera* camera = (PerspectiveCamera*)mEditor->mSceneContext.editorCamera.get();
    //                float yFov = glm::degrees(camera->GetVerticalFieldOfView());
    //                if (ImGui::DragFloat("Vertical Field Of View", &yFov, 1.0f, 4.0f, 120.0f))
    //                {
    //                    camera->SetVerticalFieldOfView(glm::radians(yFov));
    //                }
    //                float nearClippingPlane = camera->GetNearPlane();
    //                if (ImGui::DragFloat("Near Plane", &nearClippingPlane))
    //                {
    //                    camera->SetNearPlane(nearClippingPlane);
    //                }
    //                float farClippingPlane = camera->GetFarPlane();
    //                if (ImGui::DragFloat("Far Plane", &farClippingPlane))
    //                {
    //                    camera->SetFarPlane(farClippingPlane);
    //                }
    //                static float speed = 1.0f;
    //                if (ImGui::DragFloat("Camera Speed", &speed, 0.01f, 0.001f, 2.0f))
    //                {
    //                    mEditor->mSceneContext.orbiterCameraController->mSpeed = speed;
    //                    mEditor->mSceneContext.firstPersonCameraController->mSpeed = speed;
    //                }
    //                ImGui::EndPopup();
    //            }*/
    //            ImGui::EndMenuBar();
    //        }

    //        auto viewportSize = ImGui::GetContentRegionAvail();
    //        viewportSize = Vector2(viewportSize.x, viewportSize.y);

    //        if (editor->mSceneContext.sceneImageHandle != nullptr)
    //        {
    //            ImGui::Image(editor->mSceneContext.sceneImageHandle, viewportSize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
    //        }

    //        float windowWidth = ImGui::GetWindowWidth();
    //        float windowHeight = ImGui::GetWindowHeight();
    //        float delta = windowHeight - viewportSize.y;
    //        viewportPos = Vector2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y + delta);

    //        // Gizmos
    //        Entity* selectedEntity = editor->mSceneContext.selectedEntity;
    //        Bone* selectedBone = editor->mSceneContext.selectedBone;
    //        if (selectedEntity && gizmoOperationType != -1)
    //        {
    //            ImGuizmo::SetOrthographic(false);
    //            ImGuizmo::SetDrawlist();

    //            ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y + delta, viewportSize.x, viewportSize.y);

    //            // Editor camera
    //            Matrix4 cameraProjection = editor->mSceneContext.editorCamera->GetProjectionMatrix();
    //            cameraProjection[1][1] *= -1;
    //            Matrix4 cameraView = mEditor->mSceneContext.editorCamera->GetViewMatrix();

    //            static Matrix4 transformMatrix = Matrix4(1.0f);

    //            // Entity transform
    //            auto& transformComponent = selectedEntity->GetComponent<TransformComponent>();

    //            if (selectedBone == nullptr)
    //            {
    //                transformMatrix = transformComponent.localToWorldMatrix;
    //            }
    //            else
    //            {
    //                transformMatrix = selectedBone->GetLocalMatrix();
    //            }

    //            bool snap = Input::IsKeyPressed(KeyCode::LeftControl);

    //            float snapValue = GetSnapValue();
    //            float snapValues[3] = { snapValue, snapValue, snapValue };

    //            float deltaMatrix[16];
    //            ImGuizmo::Manipulate(glm::value_ptr(cameraView),
    //                glm::value_ptr(cameraProjection),
    //                (ImGuizmo::OPERATION)mGizmoOperationType,
    //                ImGuizmo::LOCAL,
    //                glm::value_ptr(transformMatrix),
    //                nullptr,
    //                snap ? snapValues : nullptr);

    //            ImGuizmo::DrawGrid(glm::value_ptr(cameraView),
    //                glm::value_ptr(cameraProjection),
    //                glm::value_ptr(Matrix4(1)),
    //                100.0f);

    //            if (ImGuizmo::IsUsing())
    //            {
    //                auto parent = selectedEntity->GetCreator()->GetEntityByHandle(selectedEntity->GetComponent<SceneHierarchyComponent>().parent);
    //                if (parent)
    //                {
    //                    transformMatrix = glm::inverse(parent->GetComponent<TransformComponent>().localToWorldMatrix) * transformMatrix;
    //                }
    //                selectedEntity->ReplaceComponent<TransformComponent>(transformMatrix);
    //            }
    //        }
        }
        ImGui::End();
    //    ImGui::PopStyleVar();
    }
}
