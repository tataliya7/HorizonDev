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

    class SceneViewTransformations
    {
    public:
        SceneViewTransformations()
            : isCameraJitteringApplied(false)
            , nonJitteredViewToClipMatrix(IdentityMatrix4x4)
            , worldToViewMatrix(IdentityMatrix4x4)
            , viewToWorldMatrix(IdentityMatrix4x4)
            , viewToClipMatrix(IdentityMatrix4x4)
            , clipToViewMatrix(IdentityMatrix4x4)
            , worldToClipMatrix(IdentityMatrix4x4)
            , clipToWorldMatrix(IdentityMatrix4x4)
        {

        }

        SceneViewTransformations(
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

        SceneViewTransformations(
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

    private:

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

        explicit SceneView(RenderScene* scene);

        RenderBackend* GetRenderBackend() const
        {

        }

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

        const Vector3& GetPreviousCameraPosition() const
        {
            return previousCameraPosition;
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

        const Vector2& GetPreviousCameraJitterOffset() const
        {
            return previousCameraJitterOffset;
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

        const SceneViewTransformations& GetTransformations() const
        {
            return transformations;
        }

        const SceneViewTransformations& GetPreviousTransformations() const
        {
            return previousTransformations;
        }

        void UpdateTransformations(const CameraComponent& camera)
        {
            perFrameShaderParameters.viewMatrix = view.camera.viewMatrix;
            perFrameShaderParameters.invViewMatrix = view.camera.invViewMatrix;
            perFrameShaderParameters.projectionMatrix = jitteredProjectionMatrix;
            perFrameShaderParameters.inverseProjectionMatrix = jitteredInvProjectionMatrix;
            perFrameShaderParameters.viewProjectionMatrix = jitteredProjectionMatrix * view.camera.viewMatrix;
            perFrameShaderParameters.invViewProjectionMatrix = view.camera.invViewMatrix * jitteredInvProjectionMatrix;
            perFrameShaderParameters.prevProjectionMatrix = jitteredPrevProjectionMatrix;
            perFrameShaderParameters.prevViewProjectionMatrix = jitteredPrevViewProjectionMatrix;
            perFrameShaderParameters.prevInvViewProjectionMatrix = jitteredPrevInvViewProjectionMatrix;
            perFrameShaderParameters.nonJitteredProjectionMatrix = view.camera.projectionMatrix;
            perFrameShaderParameters.nonJitteredInvProjectionMatrix = view.camera.invProjectionMatrix;
            perFrameShaderParameters.nonJitteredViewProjectionMatrix = view.camera.projectionMatrix * view.camera.viewMatrix;
            perFrameShaderParameters.nonJitteredInvViewProjectionMatrix = view.camera.invViewMatrix * view.camera.invProjectionMatrix;
            perFrameShaderParameters.nonJitteredPrevProjectionMatrix = nonJitteredPrevProjectionMatrix;
            perFrameShaderParameters.nonJitteredPrevViewProjectionMatrix = nonJitteredPrevViewProjectionMatrix;
            perFrameShaderParameters.nonJitteredPrevInvViewProjectionMatrix = nonJitteredPrevInvViewProjectionMatrix;
        }

    private:

        /**
         * The scene to be rendered.
         */
        RenderScene* scene;

        /**
         * The renderer used to render the scene.
         */
        SceneRenderer* sceneRenderer;

        /**
         * Render settings used to render the scene.
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

        Vector3 previousCameraPosition;

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

        Vector2 previousCameraJitterOffset;

        /**
         * Transformations of the current frame required for rasterization.
         */
        SceneViewTransformations transformations;

        /**
         * Transformations of the previous frame required for rasterization.
         */
        SceneViewTransformations previousTransformations;

        uint32 displayWidth;

        uint32 displayHeight;

        RenderBackendSwapChainHandle swapChain;

        RenderBackendTextureHandle targetTexture;

        /**
         * A point that represents the cursor's position in screen coordinates.
         */
        Vector2u cursorPosition;
    };
}