#pragma once

#include "Core/CoreCommon.h"
#include "Core/Math/Math.h"
#include "Core/Serialization/Archive.h"

namespace Horizon
{
    enum class CameraProjectionMode
    {
        Perspective,
        Orthographic,
    };

    struct CameraComponent
    {
        CameraProjectionMode projectionMode;

        float nearClippingPlane;

        float farClippingPlane;

        float fieldOfView;

        float aspectRatio;

        bool overrideAspectRatio;

        // TODO: Support PostProcessingSettings
        //PostProcessingSettings postProcessingSettings;

        // Non-serialized
        Vector3 position;
        Quaternion rotation;
        Vector3 forwardVec;
        Vector3 rightVec;
        Vector3 upVec;
        Matrix4x4 viewMatrix;
        Matrix4x4 invViewMatrix;
        Matrix4x4 projectionMatrix;
        Matrix4x4 invProjectionMatrix;
        Frustum frustum;

        void Update()
        {
            static Quaternion zUpQuat = glm::rotate(Quaternion(), Math::DegreesToRadians(90.0), Vector3(1.0, 0.0, 0.0));

            invViewMatrix = Math::Compose(position, rotation * zUpQuat, Vector3(1.0f, 1.0f, 1.0f));
            viewMatrix = Math::InverseMatrix(invViewMatrix);
            projectionMatrix = Math::PerspectiveReverseZ_RH_ZO(Math::DegreesToRadians(fieldOfView), aspectRatio, nearClippingPlane, farClippingPlane);
            invProjectionMatrix = Math::InverseMatrix(projectionMatrix);

            //rightVec    = Math::Normalize(Vector3(1.0f, 0.0f, 0.0f) * rotation);
            //forwardVec  = Math::Normalize(Vector3(0.0f, 1.0f, 0.0f) * rotation);
            //upVec       = Math::Normalize(Vector3(0.0f, 0.0f, 1.0f) * rotation);

            rightVec   = Math::Normalize(rotation * Vector3(1.0f, 0.0f, 0.0f));
            forwardVec = Math::Normalize(rotation * Vector3(0.0f, 1.0f, 0.0f));
            upVec      = Math::Normalize(rotation * Vector3(0.0f, 0.0f, 1.0f));

            // Update frustum
            {
                // Calculate camera far plane corners
                float distance = farClippingPlane;
                float halfFovRad = Math::DegreesToRadians(fieldOfView) * 0.5f;
                float uLen = distance * Math::Tan(halfFovRad);
                float rLen = uLen * aspectRatio;
                Vector3 farCenterPoint = position + distance * forwardVec;
                Vector3 u = uLen * upVec;
                Vector3 r = rLen * rightVec;

                Vector3 corners[4];
                corners[0] = farCenterPoint - u - r; // left-bottom
                corners[1] = farCenterPoint - u + r; // right-bottom
                corners[2] = farCenterPoint + u - r; // left-up
                corners[3] = farCenterPoint + u + r; // right-up

                frustum.planes[0] = Math::GetPlane(position, corners[0], corners[2]); // left
                frustum.planes[1] = Math::GetPlane(position, corners[3], corners[1]); // right
                frustum.planes[2] = Math::GetPlane(position, corners[1], corners[0]); // bottom
                frustum.planes[3] = Math::GetPlane(position, corners[2], corners[3]); // up
                frustum.planes[4] = Math::GetPlane(-forwardVec, position + forwardVec * nearClippingPlane); // near
                frustum.planes[5] = Math::GetPlane(forwardVec, position + forwardVec * farClippingPlane);  // far
            }
        }
    };
}
