#include "InspectorUI_DEPRECATED.h"
#include "HorizonEditor.h"
#include "EditorSceneManager.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Horizon
{
    void HorizonEditor::DrawInspectorWindow(bool* open)
    {
        if (ImGui::Begin("Inspector", open))
        {
            EntityHandle selectedEntity = HorizonEditor::GetInstance()->GetSelectedEntity();
            if (selectedEntity != EntityHandle::Null)
            {
                ImGui::PushID((int)uint64(selectedEntity));

                TransformComponent& transformComponent = editorSceneManager->GetActiveScene()->GetEntityManager()->GetComponent<TransformComponent>(selectedEntity);
                bool dirty = DrawComponentUI_TransformComponent("Transform Component", transformComponent);
                if (dirty)
                {
                    editorSceneManager->GetActiveScene()->GetEntityManager()->ReplaceComponent<TransformComponent>(selectedEntity, transformComponent);
                }

                if (editorSceneManager->GetActiveScene()->GetEntityManager()->HasComponent<CameraComponent>(selectedEntity))
                {
                    CameraComponent& cameraComponent = editorSceneManager->GetActiveScene()->GetEntityManager()->GetComponent<CameraComponent>(selectedEntity);
                    bool dirty = DrawComponentUI_CameraComponent("Camera Component", cameraComponent);
                }

                if (editorSceneManager->GetActiveScene()->GetEntityManager()->HasComponent<LightComponent>(selectedEntity))
                {
                    LightComponent& lightComponent = editorSceneManager->GetActiveScene()->GetEntityManager()->GetComponent<LightComponent>(selectedEntity);
                    bool dirty = DrawComponentUI_LightComponent("Light Component", lightComponent);
                }

                if (editorSceneManager->GetActiveScene()->GetEntityManager()->HasComponent<SkyLightComponent>(selectedEntity))
                {
                    SkyLightComponent& skyLightComponent = editorSceneManager->GetActiveScene()->GetEntityManager()->GetComponent<SkyLightComponent>(selectedEntity);
                    bool dirty = DrawComponentUI_SkyLightComponent("Sky Light Component", skyLightComponent);
                }

                if (editorSceneManager->GetActiveScene()->GetEntityManager()->HasComponent<SkyAtmosphereComponent>(selectedEntity))
                {
                    SkyAtmosphereComponent& skyAtmosphereComponent = editorSceneManager->GetActiveScene()->GetEntityManager()->GetComponent<SkyAtmosphereComponent>(selectedEntity);
                    bool dirty = DrawComponentUI_SkyAtmosphereComponent("Sky Atmosphere Component", skyAtmosphereComponent);
                }

                if (editorSceneManager->GetActiveScene()->GetEntityManager()->HasComponent<LocalFogVolumeComponent>(selectedEntity))
                {
                    LocalFogVolumeComponent& localFogVolumeComponent = editorSceneManager->GetActiveScene()->GetEntityManager()->GetComponent<LocalFogVolumeComponent>(selectedEntity);
                    bool dirty = DrawComponentUI_LocalFogVolumeComponent("Local Volumetric Fog Component", localFogVolumeComponent);
                }

                //if (editorSceneManager->GetActiveScene()->GetEntityManager()->HasComponent<ArmatureComponent>(selectedEntity))
                //{
                //    auto& armatureComponent = editorSceneManager->GetActiveScene()->GetEntityManager()->GetComponent<ArmatureComponent>(selectedEntity);
                //    bool dirty = DrawComponentUI_ArmatureComponent("Armature Component", armatureComponent);
                //}

                ImGui::PopID();
            }
        }
        ImGui::End();
    }

    bool DrawComponentUI_TransformComponent(const char* lable, TransformComponent& component)
    {
        bool dirty = false;
        if (ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Position");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat3("##Position", static_cast<float*>(&component.position.x)))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Rotation");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat3("##Rotation", static_cast<float*>(&component.rotation.x)))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Scale");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat3("##Scale", static_cast<float*>(&component.scale.x)))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();
        }
        return dirty;
    }

    bool DrawComponentUI_CameraComponent(const char* lable, CameraComponent& component)
    {
        bool dirty = false;
        if (ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Projection Mode");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);

            static const char* projectionModeNames[] = { "Perspective", "Orthographic" };
            int projectionMode = (int)component.projectionMode;
            ImGui::Combo("##CameraProjectionMode", &projectionMode, projectionModeNames, IM_ARRAYSIZE(projectionModeNames));
            component.projectionMode = (CameraProjectionMode)projectionMode;

            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Near Clipping Plane");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##NearClippingPlane", &component.nearClippingPlane))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Far Clipping Plane");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##FarClippingPlane", &component.farClippingPlane))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Field of View");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##FieldOfView", &component.fieldOfView))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Override Aspect Ratio");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::Checkbox("##OverrideAspectRatio", &component.overrideAspectRatio))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();


            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Aspect Ratio");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##AspectRatio", &component.aspectRatio))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();
        }
        return dirty;
    }

    bool DrawComponentUI_LightComponent(const char* lable, LightComponent& component)
    {
        bool dirty = false;
        if (ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            //if (component.type == LightComponent::LightType::Directional)
            {
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Color");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::ColorEdit3("##Color", &component.color.x))
                {
                    dirty = true;
                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();
            }
            //else if (component.type == LightComponent::LightType::Point)
            //{
            //    ImGui::AlignTextToFramePadding();
            //    ImGui::TextUnformatted("Luminance");
            //    ImGui::NextColumn();
            //    ImGui::PushItemWidth(-1);
            //    if (ImGui::DragFloat3("##Luminance", static_cast<float*>(&component.color.x)))
            //    {
            //        dirty = true;
            //    }
            //    ImGui::PopItemWidth();
            //    ImGui::NextColumn();
            //}

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Luminous Intensity");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##LuminousIntensity", &component.luminousIntensity, 0.001f, 0.0f, 200000.0f))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            if (component.type == LightComponent::Type::Point)
            {
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Radius");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::DragFloat("##Radius", &component.radius))
                {
                    dirty = true;
                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();
            }

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Apex Angle");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##ApexAngle", &component.apexAngleInDegrees))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Use Color Temperature");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::Checkbox("##UseColorTemperature", &component.useColorTemperature))
            {

            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Color Temperature");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##ColorTemperature", &component.colorTemperature))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Cast Dynamic Shadows");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::Checkbox("##CastDynamicShadows", &component.castDynamicShadows))
            {

            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Use Ray Tracing Shadows");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::Checkbox("##UseRayTracingShadows", &component.useRayTracingShadows))
            {

            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Shadow Cascade Count");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);

            float v_min = 0.0f; float v_max = 4.0f;
            if (ImGui::DragScalar("##NumShadowCascades", ImGuiDataType_U32, &component.shadowCascadeCount, 1.0f, &v_min, &v_max))
            {

            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Shadow Cascade Split Lambda");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##shadowCascadeSplitLambda", &component.shadowCascadeSplitLambda, 0.01f, 0.0f, 1.0f))
            {

            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Shadow Cascade Transition Scale");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##shadowCascadeTransitionScale", &component.shadowCascadeTransitionScale, 0.01f, 0.0f, 1.0f))
            {

            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Max Shadow Distance");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##maxShadowDistance", &component.maxShadowDistance, 0.01f, 0.0f, 1000.0f))
            {

            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Max Shadow Distance");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##maxShadowDistance", &component.maxShadowDistance, 0.01f, 0.0f, 1000.0f))
            {

            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Shadow Fade Out Factor");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##shadowFadeOutFactor", &component.shadowFadeOutFactor, 0.01f, 0.0f, 1.0f))
            {

            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Shadow Map Depth Bias Constant Factor");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##shadowMapDepthBiasConstantFactor", &component.shadowMapDepthBiasConstantFactor, 0.001f))
            {

            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Shadow Map Depth Bias Slope Factor");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##shadowMapDepthBiasSlopeFactor", &component.shadowMapDepthBiasSlopeFactor, 0.001f))
            {

            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            if (component.type == LightComponent::Type::Distant)
            {
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Atmospheric Light Disk Color Factor");
                ImGui::NextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::ColorEdit3("##atmosphericLightDiskColorFactor", &component.atmosphericLightDiskColorFactor.r))
                {

                }
                ImGui::PopItemWidth();
                ImGui::NextColumn();
            }

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();
        }
        return dirty;
    }

    bool DrawComponentUI_SkyLightComponent(const char* lable, SkyLightComponent& component)
    {
        bool dirty = false;
        if (ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            int cubemapSize = int(component.cubemapSize);

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Cubemap Size");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::InputInt("##SkyLightCubemapSize", &cubemapSize))
            {
                component.cubemapSize = uint32(cubemapSize);
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();
        }
        return dirty;
    }

    bool DrawComponentUI_SkyAtmosphereComponent(const char* lable, SkyAtmosphereComponent& component)
    {
        bool dirty = false;
        if (ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Ground Radius");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##SkyAtmosphereComponent_groundRadius", &component.groundRadius))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Ground Albedo");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::ColorEdit3("##SkyAtmosphereComponent_groundAlbedo", static_cast<float*>(&component.groundAlbedo.x)))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Atmosphere Height");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##SkyAtmosphereComponent_atmosphereHeight", &component.atmosphereHeight))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Rayleigh Scattering Scale");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##SkyAtmosphereComponent_rayleighScatteringScale", &component.rayleighScatteringScale))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Rayleigh Scattering");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::ColorEdit3("##SkyAtmosphereComponent_rayleighScattering", static_cast<float*>(&component.rayleighScattering.x)))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Rayleigh Exponential Distribution");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##SkyAtmosphereComponent_rayleighExponentialDistribution", &component.rayleighExponentialDistribution))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Mie Scattering Scale");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##SkyAtmosphereComponent_mieScatteringScale", &component.mieScatteringScale))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Mie Scattering");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::ColorEdit3("##SkyAtmosphereComponent_mieScattering", static_cast<float*>(&component.mieScattering.x)))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Mie Absorption Scale");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##SkyAtmosphereComponent_mieAbsorptionScale", &component.mieAbsorptionScale))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Mie Absorption");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::ColorEdit3("##SkyAtmosphereComponent_mieAbsorption", static_cast<float*>(&component.mieAbsorption.x)))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Mie Anisotropy");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##SkyAtmosphereComponent_mieAsymmetry", &component.mieAsymmetry))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Mie Exponential Distribution");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##SkyAtmosphereComponent_mieExponentialDistribution", &component.mieExponentialDistribution))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Ozone Absorption Scale");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##SkyAtmosphereComponent_absorptionExtinctionScale", &component.absorptionExtinctionScale))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Ozone Absorption");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::ColorEdit3("##SkyAtmosphereComponent_absorptionExtinction", static_cast<float*>(&component.absorptionExtinction.x)))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Sky Luminance Scale");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##SkyAtmosphereComponent_skyLuminanceScale", &component.skyLuminanceScale))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Sky Luminance Color");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::ColorEdit3("##SkyAtmosphereComponent_skyLuminanceColor", &component.skyLuminanceColor.x))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();
        }
        return dirty;
    }

    bool DrawComponentUI_LocalFogVolumeComponent(const char* lable, LocalFogVolumeComponent& component)
    {
        bool dirty = false;
        if (ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Scattering");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat3("##LFV_Scattering", static_cast<float*>(&component.scattering.x)))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Absorption");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat3("##LFV_Absorption", static_cast<float*>(&component.absorption.x)))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Emission");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat3("##LFV_Emission", static_cast<float*>(&component.emission.x)))
            {
                dirty = true;
            }
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();
        }
        return dirty;
    }

    //void DrawBoneNode(ArmatureComponent& component, uint32 boneIndex)
    //{
    //    auto& bone = component.bones[boneIndex];

    //    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    //    // TODO: id
    //    bool opened = ImGui::TreeNodeEx((void*)(uint64)boneIndex, flags, bone.name.c_str());
    //    if (ImGui::IsItemClicked())
    //    {

    //    }

    //    if (opened)
    //    {
    //        for (const auto& childIndex : bone.children)
    //        {
    //            DrawBoneNode(component, childIndex);
    //        }
    //        ImGui::TreePop();
    //    }
    //}

    //bool DrawArmatureComponentUI(const char* lable, ArmatureComponent& component)
    //{
    //    bool dirty = false;
    //    if (ImGui::CollapsingHeader(lable, ImGuiTreeNodeFlags_DefaultOpen))
    //    {
    //        DrawBoneNode(component, 0);
    //    }
    //    return dirty;
    //}
}
