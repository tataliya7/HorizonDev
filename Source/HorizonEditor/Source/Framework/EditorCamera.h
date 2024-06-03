#pragma once

#include "Engine/HorizonEngineModule.h"

namespace Horizon
{
    struct EditorCameraControllerUserImpulseData
    {
        float moveRightLeftImpulse                = 0.0f;
        float moveForwardBackwardImpulse          = 0.0f;
        float moveUpDownImpulse                   = 0.0f;
        float rotatePitchImpulse                  = 0.0f;
        float rotateRollImpulse                   = 0.0f;
        float rotateYawImpulse                    = 0.0f;

        void Reset()
        {
            moveRightLeftImpulse                  = 0.0f;
            moveForwardBackwardImpulse            = 0.0f;
            moveUpDownImpulse                     = 0.0f;
            rotatePitchImpulse                    = 0.0f;
            rotateRollImpulse                     = 0.0f;
            rotateYawImpulse                      = 0.0f;
        }
    };

    struct EditorCameraControllerSettings
    {
        float translationMultiplier               = 1.0f;
        float rotationMultiplier                  = 1.0f;
        float translationAccelerationRate         = 20000.0f;
        float rotationAccelerationRate            = 16000.0f;
        float maxTranslationVelocity              = std::numeric_limits<float>::max();
        float maxRotationVelocity                 = std::numeric_limits<float>::max();
        float minPitchRotation                    = -90.0f;
        float maxPitchRotation                    = 90.0f;
        float translationVelocityDampingAmount    = 20.0f;
        float rotationVelocityDampingAmount       = 25.0f;
        bool enablePitchAngleRestriction          = true;
        bool enablePhysicallyBasedTranslation     = false;
        bool enablePhysicallyBasedRotation        = false;
    };

    class EditorCameraController
    {
    public:

        EditorCameraController();

        bool IsMoving() const;

        bool IsRotating() const;

        void Update(const EditorCameraControllerUserImpulseData& userImpulseData, float deltaTimeInSeconds, float translationVelocityScale, Vector3& outCameraPosition, Vector3& outCameraRotation);

        /** Camera controller settings. */
        EditorCameraControllerSettings settings;

    private:

        static constexpr float Epsilon = 1.e-4f;

        void UpdatePosition(const EditorCameraControllerUserImpulseData& userImpulseData, float deltaTimeInSeconds, float translationVelocityScale, const Vector3& cameraRotation, Vector3& outCameraPosition);

        void UpdateRotation(const EditorCameraControllerUserImpulseData& userImpulseData, float deltaTimeInSeconds, Vector3& outCameraRotation);

        /** Translation velocity in meters per second. */
        Vector3 translationVelocity;

        /** Rotation velocity (pitch, roll and yaw) in degrees per second. */
        Vector3 rotationVelocity;
    };

    class EditorCamera
    {
        //CameraProjectionMode projectionMode;

    public:

        Vector3 GetPosition() const
        {
            return position;
        }

        Vector3 GetRotation() const
        {
            return rotation;
        }

        float nearClippingPlane;

        float farClippingPlane;

        float fieldOfView;

        float aspectRatio;

        bool overrideAspectRatio;

        float cameraSpeed;

        EditorCameraControllerUserImpulseData userImpulseData;

    //private:

        Vector3 position;

        Vector3 rotation;

        EditorCameraController controller;

        void Update(float deltaTimeInSeconds);
    };
}