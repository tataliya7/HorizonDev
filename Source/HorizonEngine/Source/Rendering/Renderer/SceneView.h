#pragma once

#include "RendererCommon.h"
#include "RenderSettings.h"

namespace Horizon
{
    class RenderScene;

    constexpr float NearClippingPlaneDepthValue = 1.0f;
    constexpr float FarClippingPlaneDepthValue = 0.0f;

    constexpr float MinimumNearClippingPlane = 0.01f;

    struct CameraTransformations
    {
        CameraTransformations()
            : worldToViewMatrix(IdentityMatrix4x4f)
            , viewToWorldMatrix(IdentityMatrix4x4f)
            , viewToClipMatrix(IdentityMatrix4x4f)
            , clipToViewMatrix(IdentityMatrix4x4f)
            , worldToClipMatrix(IdentityMatrix4x4f)
            , clipToWorldMatrix(IdentityMatrix4x4f)
            , nonJitteredViewToClipMatrix(IdentityMatrix4x4f)
            , nonJitteredClipToViewMatrix(IdentityMatrix4x4f)
            , nonJitteredWorldToClipMatrix(IdentityMatrix4x4f)
            , nonJitteredClipToWorldMatrix(IdentityMatrix4x4f)
            , viewSpaceDepthToNDCSpaceDepthTransform(Vector2f(0.0f, 0.0f))
        {

        }

        void Reset()
        {
            worldToViewMatrix = IdentityMatrix4x4f;
            viewToWorldMatrix = IdentityMatrix4x4f;
            viewToClipMatrix = IdentityMatrix4x4f;
            clipToViewMatrix = IdentityMatrix4x4f;
            worldToClipMatrix = IdentityMatrix4x4f;
            clipToWorldMatrix = IdentityMatrix4x4f;
            nonJitteredViewToClipMatrix = IdentityMatrix4x4f;
            nonJitteredClipToViewMatrix = IdentityMatrix4x4f;
            nonJitteredWorldToClipMatrix = IdentityMatrix4x4f;
            nonJitteredClipToWorldMatrix = IdentityMatrix4x4f;
            viewSpaceDepthToNDCSpaceDepthTransform = Vector2f(0.0f, 0.0f);
        }

        void Update(const Vector3f& position, const Vector3f& rotation, float fieldOfView, float aspectRatio, float nearClippingPlane, float farClippingPlane)
        {
            // TODO: refactor this
            // TODO: make it constexpr
            static Quaternion zUpQuat = glm::rotate(Quaternion(), Math::DegreesToRadians(90.0), Vector3f(1.0, 0.0, 0.0));

            Quaternion cameraOrientation = Math::QuaternionFromEulerAngles(Math::DegreesToRadians(rotation));

            // TODO: calculate worldToViewMatrix first
            // viewToWorldMatrix = Math::ComposeTransformationMatrix(position, cameraOrientation * zUpQuat, Vector3f(1.0f, 1.0f, 1.0f));
            // worldToViewMatrix = Math::InverseMatrix(viewToWorldMatrix);
            worldToViewMatrix = glm::transpose(glm::mat4_cast(glm::normalize(cameraOrientation * zUpQuat))) * glm::translate(glm::mat4(1), -position);
            //viewToWorldMatrix = Math::InverseMatrix(worldToViewMatrix);

#if HORIZON_EXPERIMENTAL_INFINITE_PERSPECTIVE
            if (true)
            {
                // Infinite far plane and reversed-z
                // Reference: http://www.terathon.com/gdc07_lengyel.pdf
                viewToClipMatrix = Math::InfinitePerspectiveReversedZ_RH(fieldOfView, aspectRatio, nearClippingPlane);
            }
            else
#endif
            {
                // Swap the near and far plane to get a perspective matrix with reversed-z.
                viewToClipMatrix = glm::perspectiveRH_ZO(fieldOfView, aspectRatio, farClippingPlane, nearClippingPlane);
            }

            nonJitteredViewToClipMatrix = viewToClipMatrix;
            nonJitteredClipToViewMatrix = Math::InverseMatrix(nonJitteredViewToClipMatrix);
            nonJitteredWorldToClipMatrix = viewToClipMatrix * worldToViewMatrix;
            nonJitteredClipToWorldMatrix = Math::InverseMatrix(nonJitteredWorldToClipMatrix);
        }

        void ApplyJitterOffset(const Vector2f& jitterOffset, uint32 renderWidth, uint32 renderHeight)
        {
            // -y for clip space to screen space
            Vector2f offset = { jitterOffset.x * 2.0f / float(renderWidth), -jitterOffset.y * 2.0f / float(renderHeight) };

            /*
             * Horizon Engine uses a right-handed coordinate system,
             * the w component of clip space position is -Zc instead of Zc,
             * so it should be multiplied by -1.
             */
            viewToClipMatrix[2][0] += -offset.x;
            viewToClipMatrix[2][1] += -offset.y;
        }

        void Finalize();

        Matrix4x4f worldToViewMatrix;
        Matrix4x4f viewToWorldMatrix;
        Matrix4x4f viewToClipMatrix;
        Matrix4x4f clipToViewMatrix;
        Matrix4x4f worldToClipMatrix;
        Matrix4x4f clipToWorldMatrix;
        Matrix4x4f nonJitteredViewToClipMatrix;
        Matrix4x4f nonJitteredClipToViewMatrix;
        Matrix4x4f nonJitteredWorldToClipMatrix;
        Matrix4x4f nonJitteredClipToWorldMatrix;
        Vector2f viewSpaceDepthToNDCSpaceDepthTransform;
    };

    struct SceneViewDescription
    {
        RenderScene* scene;

        RenderSettings renderSettings;

        uint32 frameIndex;

        float deltaTimeInSeconds;

        bool reset;

        Vector3f cameraPosition;
        Vector3f cameraRotation;
        Vector3f cameraUpVector;
        Vector3f cameraRightVector;
        Vector3f cameraForwardVector;

        float verticalFOV;

        float aspectRatio;

        float nearClippingPlane;

        float farClippingPlane;

        Vector3f backgroundColor;

        uint32 targetWidth;

        uint32 targetHeight;

        RenderGraphPersistentTexture* targetTexture;

        uint32 displayWidth;

        uint32 displayHeight;

        RenderGraphPersistentTexture* displayTexture;

        RenderBackendSwapChainHandle swapChain;

        Vector2u cursorPosition;
    };

    /**
     *
     */
    class SceneView
    {
    public:

        SceneView(const SceneViewDescription& description);

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

        const Vector3f& GetCameraPosition() const
        {
            return cameraPosition;
        }

        const Vector3f& GetCameraUpVector() const
        {
            return cameraForwardVector;
        }

        const Vector3f& GetCameraRightVector() const
        {
            return cameraForwardVector;
        }

        const Vector3f& GetCameraForwardVector() const
        {
            return cameraForwardVector;
        }

        const Vector2f& GetCameraJitterOffset() const
        {
            return jitterOffset;
        }

        float GetFieldOfView() const
        {
            return verticalFOV;
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

        const CameraTransformations& GetCameraTransformations() const
        {
            return transformations;
        }

        bool IsPerspectiveProjection() const
        {
            return transformations.viewToClipMatrix[3][3] < 1.0f;
        }

        //
        // inline bool IsCameraJitteringApplied() const
        // {
        //     return isCameraJitteringApplied;
        // }
        //

        const Matrix4x4f& GetWorldToViewMatrix() const
        {
            return transformations.worldToViewMatrix;
        }

        const Matrix4x4f& GetViewToWorldMatrix() const
        {
            return transformations.viewToWorldMatrix;
        }

        const Matrix4x4f& GetViewToClipMatrix() const
        {
            return transformations.viewToClipMatrix;
        }

        const Matrix4x4f& GetClipToViewMatrix() const
        {
            return transformations.clipToViewMatrix;
        }

        //
        // inline const Matrix4x4f& GetNonJitteredViewToClipMatrix() const
        // {
        //     return nonJitteredViewToClipMatrix;
        // }
        //
        // inline const Matrix4x4f& GetWorldToClipMatrix() const
        // {
        //     return worldToClipMatrix;
        // }
        //
        // inline const Matrix4x4f& GetClipToWorldMatrix() const
        // {
        //     return clipToWorldMatrix;
        // }

    //private:

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

        Vector3f cameraPosition;
        Vector3f cameraRotation;
        Vector3f cameraUpVector;
        Vector3f cameraRightVector;
        Vector3f cameraForwardVector;

        /**
         * Vertical field of view in radians.
         */
        float verticalFOV;

        float tanHalfVerticalFOV;

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
        Vector3f backgroundColor;

        /**
         * The region of space in the scene that may appear on the screen.
         */
        Frustum viewFrustum;

        uint32 jitterPhaseCount;

        Vector2f jitterOffset;

        /**
         * Transformations of the current frame required for rasterization.
         */
        CameraTransformations transformations;

        uint32 targetWidth;

        uint32 targetHeight;

        RenderGraphPersistentTexture* targetTexture;

        uint32 displayWidth;

        uint32 displayHeight;

        RenderGraphPersistentTexture* displayTexture;

        RenderBackendSwapChainHandle swapChain;

        /**
         * A point that represents the cursor's position in screen coordinates.
         */
        Vector2u cursorPosition;

        void SetupViewFrustum()
        {
            float distance = farClippingPlane;
            float uLen = distance * tanHalfVerticalFOV;
            float rLen = uLen * aspectRatio;
            Vector3f farCenterPoint = cameraPosition + distance * cameraForwardVector;
            Vector3f u = uLen * cameraUpVector;
            Vector3f r = rLen * cameraRightVector;

            Vector3f corners[4];
            corners[0] = farCenterPoint - u - r; // left-bottom
            corners[1] = farCenterPoint - u + r; // right-bottom
            corners[2] = farCenterPoint + u - r; // left-up
            corners[3] = farCenterPoint + u + r; // right-up

            viewFrustum.planes[0] = Plane(cameraPosition, corners[0], corners[2]); // left
            viewFrustum.planes[1] = Plane(cameraPosition, corners[3], corners[1]); // right
            viewFrustum.planes[2] = Plane(cameraPosition, corners[1], corners[0]); // bottom
            viewFrustum.planes[3] = Plane(cameraPosition, corners[2], corners[3]); // up
            viewFrustum.planes[4] = Plane(cameraForwardVector, cameraPosition + cameraForwardVector * nearClippingPlane); // near
            viewFrustum.planes[5] = Plane(-cameraForwardVector, cameraPosition + cameraForwardVector * farClippingPlane); // far
        }
    };
}