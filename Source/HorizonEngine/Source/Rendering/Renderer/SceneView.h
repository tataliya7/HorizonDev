#pragma once

#include "RendererCommon.h"
#include "RenderSettings.h"

namespace Horizon
{
    class RenderScene;

    enum class SceneViewDebugVisualizationMode
    {
        Lighting,
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

    struct CameraTransformations
    {
        CameraTransformations()
            : worldToViewMatrix(IdentityMatrix4x4)
            , viewToWorldMatrix(IdentityMatrix4x4)
            , viewToClipMatrix(IdentityMatrix4x4)
            , clipToViewMatrix(IdentityMatrix4x4)
            , worldToClipMatrix(IdentityMatrix4x4)
            , clipToWorldMatrix(IdentityMatrix4x4)
            , nonJitteredViewToClipMatrix(IdentityMatrix4x4)
        {

        }

        void Reset()
        {
            worldToViewMatrix = IdentityMatrix4x4;
            viewToWorldMatrix = IdentityMatrix4x4;
            viewToClipMatrix = IdentityMatrix4x4;
            clipToViewMatrix = IdentityMatrix4x4;
            worldToClipMatrix = IdentityMatrix4x4;
            clipToWorldMatrix = IdentityMatrix4x4;
            nonJitteredViewToClipMatrix = IdentityMatrix4x4;
        }

        void Update(const Vector3& position, const Vector3& rotation, float fieldOfView, float aspectRatio, float nearClippingPlane, float farClippingPlane)
        {
            // TODO: refactor this
            // TODO: make it constexpr
            static Quaternion zUpQuat = glm::rotate(Quaternion(), Math::DegreesToRadians(90.0), Vector3(1.0, 0.0, 0.0));

            Quaternion cameraOrientation = Math::QuaternionFromEulerAngles(Math::DegreesToRadians(rotation));

            // TODO: calculate worldToViewMatrix first
            viewToWorldMatrix = Math::ComposeTransformMatrix(position, cameraOrientation * zUpQuat, Vector3(1.0f, 1.0f, 1.0f));
            worldToViewMatrix = Math::InverseMatrix(viewToWorldMatrix);

            viewToClipMatrix = Math::PerspectiveReverseZ_RH_ZO(Math::DegreesToRadians(fieldOfView), aspectRatio, nearClippingPlane, farClippingPlane);
            clipToViewMatrix = Math::InverseMatrix(viewToClipMatrix);

            worldToClipMatrix = viewToClipMatrix * worldToViewMatrix;
            clipToWorldMatrix = viewToWorldMatrix * clipToViewMatrix;
        }

        //
        // CameraTransformations(
        //     const Matrix4x4& viewMatrix,
        //     const Matrix4x4& projectionMatrix)
        // {
        //     worldToViewMatrix = viewMatrix;
        //     viewToWorldMatrix = Math::InverseMatrix(viewMatrix);
        //     viewToClipMatrix = projectionMatrix;
        //     clipToViewMatrix = Math::InverseMatrix(projectionMatrix);
        //     nonJitteredViewToClipMatrix = projectionMatrix;
        //     worldToClipMatrix = viewToClipMatrix * worldToViewMatrix;
        //     clipToWorldMatrix = viewToWorldMatrix * clipToViewMatrix;
        //     cameraJitterOffset = ZeroVector2;
        //     isCameraJitteringApplied = false;
        // }
        //
        // CameraTransformations(
        //     const Matrix4x4& viewMatrix,
        //     const Matrix4x4& projectionMatrix,
        //     const Vector2& jitterOffset)
        // {
        //     Matrix4x4 jitteredProjectionMatrix = projectionMatrix;
        //     jitteredProjectionMatrix[2][0] += -jitterOffset.x;
        //     jitteredProjectionMatrix[2][1] += -jitterOffset.y;
        //
        //     worldToViewMatrix = viewMatrix;
        //     viewToWorldMatrix = Math::InverseMatrix(viewMatrix);
        //     viewToClipMatrix = jitteredProjectionMatrix;
        //     clipToViewMatrix = Math::InverseMatrix(jitteredProjectionMatrix);
        //     nonJitteredViewToClipMatrix = projectionMatrix;
        //     worldToClipMatrix = viewToClipMatrix * worldToViewMatrix;
        //     clipToWorldMatrix = viewToWorldMatrix * clipToViewMatrix;
        //     cameraJitterOffset = jitterOffset;
        //     isCameraJitteringApplied = true;
        // }
        //
        // inline bool IsPerspectiveProjection() const
        // {
        //     return viewToClipMatrix[3][3] < 1.0f;
        // }
        //
        // inline bool IsCameraJitteringApplied() const
        // {
        //     return isCameraJitteringApplied;
        // }
        //
        // inline const Matrix4x4& GetWorldToViewMatrix() const
        // {
        //     return worldToViewMatrix;
        // }
        //
        // inline const Matrix4x4& GetViewToWorldMatrix() const
        // {
        //     return viewToWorldMatrix;
        // }
        //
        // inline const Matrix4x4& GetViewToClipMatrix() const
        // {
        //     return viewToClipMatrix;
        // }
        //
        // inline const Matrix4x4& GetClipToViewMatrix() const
        // {
        //     return clipToViewMatrix;
        // }
        //
        // inline const Matrix4x4& GetNonJitteredViewToClipMatrix() const
        // {
        //     return nonJitteredViewToClipMatrix;
        // }
        //
        // inline const Matrix4x4& GetWorldToClipMatrix() const
        // {
        //     return worldToClipMatrix;
        // }
        //
        // inline const Matrix4x4& GetClipToWorldMatrix() const
        // {
        //     return clipToWorldMatrix;
        // }

        Matrix4x4 worldToViewMatrix;
        Matrix4x4 viewToWorldMatrix;
        Matrix4x4 viewToClipMatrix;
        Matrix4x4 clipToViewMatrix;
        Matrix4x4 worldToClipMatrix;
        Matrix4x4 clipToWorldMatrix;
        Matrix4x4 nonJitteredViewToClipMatrix;
    };

    /**
     *
     */
    class SceneView
    {
    public:

        bool HasValidScene() const
        {
            return scene != nullptr;
        }

        RenderScene* GetRenderScene() const
        {
            return scene;
        }

        const RenderSettings& GetRenderSettings() const
        {
            return renderSettings;
        }

        void SetRenderSettings(const RenderSettings& settings)
        {
            renderSettings = settings;
        }

        bool NeedToBeReset() const
        {
            return reset;
        }

        uint32 GetFrameIndex() const
        {
            return frameIndex;
        }

        float GetDeltaTimeInSeconds() const
        {
            return deltaTimeInSeconds;
        }

        const Vector3& GetCameraPosition() const
        {
            return cameraPosition;
        }

        const Vector3& GetCameraUpVector() const
        {
            return cameraForwardVector;
        }

        const Vector3& GetCameraRightVector() const
        {
            return cameraForwardVector;
        }

        const Vector3& GetCameraForwardVector() const
        {
            return cameraForwardVector;
        }

        const Vector2& GetCameraJitterOffset() const
        {
            return cameraJitterOffset;
        }

        float GetFieldOfView() const
        {
            return fieldOfView;
        }

        float GetAspectRatio() const
        {
            return aspectRatio;
        }

        float GetFarClippingPlane() const
        {
            return farClippingPlane;
        }

        float GetNearClippingPlane() const
        {
            return nearClippingPlane;
        }

        const CameraTransformations& GetTransformations() const
        {
            return transformations;
        }

        // void UpdateTransformations(const CameraComponent& camera)
        // {
        //     perFrameShaderParameters.viewMatrix = view.camera.viewMatrix;
        //     perFrameShaderParameters.invViewMatrix = view.camera.invViewMatrix;
        //     perFrameShaderParameters.projectionMatrix = jitteredProjectionMatrix;
        //     perFrameShaderParameters.inverseProjectionMatrix = jitteredInvProjectionMatrix;
        //     perFrameShaderParameters.viewProjectionMatrix = jitteredProjectionMatrix * view.camera.viewMatrix;
        //     perFrameShaderParameters.invViewProjectionMatrix = view.camera.invViewMatrix * jitteredInvProjectionMatrix;
        //     perFrameShaderParameters.prevProjectionMatrix = jitteredPrevProjectionMatrix;
        //     perFrameShaderParameters.prevViewProjectionMatrix = jitteredPrevViewProjectionMatrix;
        //     perFrameShaderParameters.prevInvViewProjectionMatrix = jitteredPrevInvViewProjectionMatrix;
        //     perFrameShaderParameters.nonJitteredProjectionMatrix = view.camera.projectionMatrix;
        //     perFrameShaderParameters.nonJitteredInvProjectionMatrix = view.camera.invProjectionMatrix;
        //     perFrameShaderParameters.nonJitteredViewProjectionMatrix = view.camera.projectionMatrix * view.camera.viewMatrix;
        //     perFrameShaderParameters.nonJitteredInvViewProjectionMatrix = view.camera.invViewMatrix * view.camera.invProjectionMatrix;
        //     perFrameShaderParameters.nonJitteredPrevProjectionMatrix = nonJitteredPrevProjectionMatrix;
        //     perFrameShaderParameters.nonJitteredPrevViewProjectionMatrix = nonJitteredPrevViewProjectionMatrix;
        //     perFrameShaderParameters.nonJitteredPrevInvViewProjectionMatrix = nonJitteredPrevInvViewProjectionMatrix;
        // }

        /**
         * The scene to be rendered.
         */
        RenderScene* scene;

        /**
         * The renderer used to render the scene.
         */
        //SceneRenderer* sceneRenderer;

        /**
         * Global render settings for rendering the scene.
         */
        RenderSettings renderSettings;

        /**
         * Visualization mode for debugging.
         */
        SceneViewDebugVisualizationMode debugVisualizationMode;

        /**
         * The time elapsed since the last frame (expressed in seconds).
         */
        float deltaTimeInSeconds;

        /**
         * The number of frames this view is rendered.
         */
        uint32 frameIndex;

        /**
         * Whether the rendering state need to be reset.
         */
        bool reset;

        /**
         * Whether setting the far clipping plane to infinity.
         *
         * @see https://chaosinmotion.com/2010/09/06/goodbye-far-clipping-plane/
         */
        bool enableInfiniteFarClippingPlane;

        /**
         * Whether apply sub-pixel jitter.
         */
        bool enableSubpixelJitter;

        Vector3 cameraPosition;
        Vector3 cameraRotation;
        Vector3 cameraUpVector;
        Vector3 cameraRightVector;
        Vector3 cameraForwardVector;

        /**
         * Vertical field of view in degrees.
         */
        float fieldOfView;

        /**
         * The aspect ratio of the scene color texture (expressed as a ratio of width to height).
         */
        float aspectRatio;

        /**
         * The near plane of the view frustum.
         */
        float nearClippingPlane;

        /**
         * The far plane of the view frustum.
         */
        float farClippingPlane;

        /**
         * The color used to clear the scene color texture.
         */
        Vector3 backgroundColor;

        /**
         * The region of space in the scene that may appear on the screen.
         */
        Frustum viewFrustum;

        uint32 cameraJitterPhaseCount;

        Vector2 cameraJitterOffset;

        /**
         * Transformations of the current frame required for rasterization.
         */
        CameraTransformations transformations;

        uint32 targetWidth;

        uint32 targetHeight;

        RenderGraphPersistentTexture* targetTexture;

        uint32 displayWidth;

        uint32 displayHeight;

        RenderBackendSwapChainHandle swapChain;

        /**
         * A point that represents the cursor's position in screen coordinates.
         */
        Vector2u cursorPosition;
    };
}