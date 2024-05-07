#pragma once

#include "Core/CoreModule.h"
#include "Entity/EntityComponents.h"
#include "RenderBackend/RenderBackendModule.h"
#include "Rendering/RenderSettings.h"

namespace Horizon
{
    class Scene;
    class EngineSubsystem;

    class CameraTransformations
    {
    public:

        CameraTransformations(
            const Matrix4x4& viewMatrix,
            const Matrix4x4& projectionMatrix)
        {
            worldToViewMatrix = viewMatrix;
            viewToWorldMatrix = Math::InverseMatrix(viewMatrix);
            viewToClipMatrix = projectionMatrix;
            clipToViewMatrix = Math::InverseMatrix(projectionMatrix);
            nonJitteredViewToClipMatrix = projectionMatrix;
            worldToClipMatrix = viewToClipMatrix * worldToViewMatrix;
            clipToWorldMatrix = viewToWorldMatrix * clipToViewMatrix;
            cameraJitterOffset = ZeroVector2;
            isCameraJitteringApplied = false;
        }

        CameraTransformations(
            const Matrix4x4& viewMatrix,
            const Matrix4x4& projectionMatrix,
            const Vector2& jitterOffset)
        {
            Matrix4x4 jitteredProjectionMatrix = projectionMatrix;
            jitteredProjectionMatrix[2][0] += -jitterOffset.x;
            jitteredProjectionMatrix[2][1] += -jitterOffset.y;

            worldToViewMatrix = viewMatrix;
            viewToWorldMatrix = Math::InverseMatrix(viewMatrix);
            viewToClipMatrix = jitteredProjectionMatrix;
            clipToViewMatrix = Math::InverseMatrix(jitteredProjectionMatrix);
            nonJitteredViewToClipMatrix = projectionMatrix;
            worldToClipMatrix = viewToClipMatrix * worldToViewMatrix;
            clipToWorldMatrix = viewToWorldMatrix * clipToViewMatrix;
            cameraJitterOffset = jitterOffset;
            isCameraJitteringApplied = true;
        }
    };

    class SceneView
    {
    public:
        inline bool IsPerspectiveProjection() const
        {
            return viewToClipMatrix[3][3] < 1.0f;
        }

        inline bool IsCameraJitteringApplied() const
        {
            return isCameraJitteringApplied;
        }

        inline const Matrix4x4& GetWorldToViewMatrix() const
        {
            return worldToViewMatrix;
        }

        inline const Matrix4x4& GetViewToWorldMatrix() const
        {
            return viewToWorldMatrix;
        }

        inline const Matrix4x4& GetViewToClipMatrix() const
        {
            return viewToClipMatrix;
        }

        inline const Matrix4x4& GetClipToViewMatrix() const
        {
            return clipToViewMatrix;
        }

        inline const Matrix4x4& GetNonJitteredViewToClipMatrix() const
        {
            return nonJitteredViewToClipMatrix;
        }

        inline const Matrix4x4& GetWorldToClipMatrix() const
        {
            return worldToClipMatrix;
        }

        inline const Matrix4x4& GetClipToWorldMatrix() const
        {
            return clipToWorldMatrix;
        }

        inline const Vector2& GetCameraJitterOffset() const
        {
            return cameraJitterOffset;
        }
    private:

        SceneView()
            : worldToViewMatrix(IdentityMatrix4x4)
            , viewToWorldMatrix(IdentityMatrix4x4)
            , viewToClipMatrix(IdentityMatrix4x4)
            , clipToViewMatrix(IdentityMatrix4x4)
            , nonJitteredViewToClipMatrix(IdentityMatrix4x4)
            , worldToClipMatrix(IdentityMatrix4x4)
            , clipToWorldMatrix(IdentityMatrix4x4)
            , cameraJitterOffset(ZeroVector2)
            , isCameraJitteringApplied(false)
        {

        }

        float fieldOfView;
        float nearClippingPlane;
        float farClippingPlane;

        Vector3 cameraPosition;
        Vector3 cameraRotation;

        Frustum viewFrustum;

        uint32 cameraJitterPhaseCount;
        Vector2 cameraJitterOffset;
        bool isCameraJitteringApplied;
        Matrix4x4 nonJitteredViewToClipMatrix;

        Matrix4x4 worldToViewMatrix;
        Matrix4x4 viewToWorldMatrix;
        Matrix4x4 viewToClipMatrix;
        Matrix4x4 clipToViewMatrix;
        Matrix4x4 worldToClipMatrix;
        Matrix4x4 clipToWorldMatrix;

        bool reset;

        CameraTransformations transformations;

        void UpdateTransformations(const CameraComponent& camera)
        {

        }
    };

    enum class SceneViewVisualizationMode
    {
        Litghting,
        Wireframe,
        Illuminance,
        WorldSpaceNormal,
        PrimitiveID,
        MaterialID,
        MotionVectors,
        AmbientOcclusion,
        ShadowMask,
        SurfelGISurfel,
        SurfelGIHeatmap,
    };

    class SceneView
    {
    public:
        Scene* scene;
        Vector2i cursorPosition;
        Vector3 backgroundColor;
        RenderSettings renderSettings;
        bool cameraCut;
        float deltaTime;
        uint32 frameIndex;
        uint32 displayWidth;
        uint32 displayHeight;
        RenderBackendSwapChainHandle swapChain;
        SceneViewVisualizationMode visualizationMode;
    private:
        SceneRenderer* renderer;
    };
}