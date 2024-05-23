#include "EditorCamera.h"

namespace Horizon
{
    EditorCameraController::EditorCameraController()
        : translationVelocity(0.0f, 0.0f, 0.0f)
        , rotationVelocity(0.0f, 0.0f, 0.0f)
    {

    }

    bool EditorCameraController::IsMoving() const
    {
        if ((translationVelocity.x != 0.0f) || (translationVelocity.y != 0.0f) || (translationVelocity.z != 0.0f))
        {
            return true;
        }
        return false;
    }

    bool EditorCameraController::IsRotating() const
    {
        if ((rotationVelocity.x != 0.0f) || (rotationVelocity.y != 0.0f) || (rotationVelocity.z != 0.0f))
        {
            return true;
        }
        return false;
    }

    void EditorCameraController::Update(const EditorCameraControllerUserImpulseData& userImpulseData, float deltaTimeInSeconds, float translationVelocityScale, Vector3& outCameraPosition, Vector3& outCameraRotation)
    {
        // Translation
        UpdatePosition(userImpulseData, deltaTimeInSeconds, translationVelocityScale, outCameraRotation, outCameraPosition);

        // Rotation
        UpdateRotation(userImpulseData, deltaTimeInSeconds, outCameraRotation);
    }

    void EditorCameraController::UpdatePosition(const EditorCameraControllerUserImpulseData& userImpulseData, float deltaTimeInSeconds, float translationVelocityScale, const Vector3& cameraRotation, Vector3& outCameraPosition)
    {
        Vector3 localSpaceTranslationImpulse = Vector3(
            userImpulseData.moveRightLeftImpulse,          // pitch
            userImpulseData.moveForwardBackwardImpulse,    // roll
            userImpulseData.moveUpDownImpulse              // yaw
        );

        Vector3 translationAcceleration;
        {
            // Compute camera orientation, and then transform the translation impulse from local space to world space.
            const Quaternion cameraOrientation = Quaternion(Math::DegreesToRadians(cameraRotation));
            Vector3 worldSpaceTranslationImpulse = cameraOrientation * localSpaceTranslationImpulse;
            translationAcceleration = worldSpaceTranslationImpulse * settings.translationAccelerationRate * translationVelocityScale;
        }

        if (settings.enablePhysicallyBasedTranslation)
        {
            // Accelerate the movement velocity.
            translationVelocity += translationAcceleration * deltaTimeInSeconds;

            // Apply damping to the translation.
            {
                const float dampingFactor = Math::Clamp(settings.translationVelocityDampingAmount * deltaTimeInSeconds, 0.0f, 1.0f);
                translationVelocity -= translationVelocity * dampingFactor;
            }
        }
        else
        {
            translationVelocity = translationAcceleration;
        }

        // Clamp translation velocity to a valid range.
        if (Math::LengthSquared(translationVelocity) > Math::Square(settings.maxTranslationVelocity * translationVelocityScale))
        {
            translationVelocity = Math::Normalize(translationVelocity) * settings.maxTranslationVelocity * translationVelocityScale;
        }

        if (Math::LengthSquared(translationVelocity) < Math::Square(Epsilon))
        {
            translationVelocity = Vector3(0.0f, 0.0f, 0.0f);
        }

        outCameraPosition += translationVelocity * deltaTimeInSeconds;
    }

    void EditorCameraController::UpdateRotation(const EditorCameraControllerUserImpulseData& userImpulseData, float deltaTimeInSeconds, Vector3& outCameraRotation)
    {
        Vector3 rotationImpulse = Vector3(
            userImpulseData.rotatePitchImpulse,    // pitch
            userImpulseData.rotateRollImpulse,     // roll
            userImpulseData.rotateYawImpulse       // yaw
        );

        // Iterate for each euler axis (pitch, roll and yaw).
        for (int32 eulerAxis = 0; eulerAxis < 3; eulerAxis++)
        {
            float currentRotationVelocity = rotationVelocity[eulerAxis];
            float currentRotationAcceleration = rotationImpulse[eulerAxis] * settings.rotationAccelerationRate;

            if (settings.enablePhysicallyBasedRotation)
            {
                // Accelerate the rotation velocity.
                currentRotationVelocity += currentRotationAcceleration * deltaTimeInSeconds;

                // Apply damping to the rotation.
                {
                    const float dampingFactor = Math::Clamp(settings.rotationVelocityDampingAmount * deltaTimeInSeconds, 0.0f, 1.0f);
                    currentRotationVelocity -= currentRotationVelocity * dampingFactor;
                }
            }
            else
            {
                currentRotationVelocity = currentRotationAcceleration;
            }

            // Clamp rotation velocity to a valid range.
            currentRotationVelocity = Math::Clamp(currentRotationVelocity, -settings.maxRotationVelocity, settings.maxRotationVelocity);
            if (Math::Abs(currentRotationVelocity) < Epsilon)
            {
                currentRotationVelocity = 0.0f;
            }

            rotationVelocity[eulerAxis] = currentRotationVelocity;

            outCameraRotation[eulerAxis] += currentRotationVelocity * deltaTimeInSeconds;

            // Limit the pitch angle to a valid range.
            if (eulerAxis == 0)
            {
                // Warp the angle to -180 to 180.
                float eulerAngle = Math::Fmod(outCameraRotation[eulerAxis], 360.0f);
                if (eulerAngle > 180.f)
                {
                    eulerAngle -= 360.f;
                }
                else if (eulerAngle < -180.f)
                {
                    eulerAngle += 360.f;
                }

                if (settings.enablePitchAngleRestriction)
                {
                    outCameraRotation[eulerAxis] = Math::Clamp(eulerAngle, settings.minPitchRotation, settings.maxPitchRotation);
                }
            }
        }
    }

    void EditorCamera::Update(float deltaTimeInSeconds)
    {
        // TODO: make it configurable
        float impulse = 5.0;

        if (Input::GetKeyDown(KeyCode::D))
        {
            userImpulseData.moveRightLeftImpulse += impulse;
        }
        if (Input::GetKeyDown(KeyCode::A))
        {
            userImpulseData.moveRightLeftImpulse -= impulse;
        }
        if (Input::GetKeyDown(KeyCode::W))
        {
            userImpulseData.moveForwardBackwardImpulse += impulse;
        }
        if (Input::GetKeyDown(KeyCode::S))
        {
            userImpulseData.moveForwardBackwardImpulse -= impulse;
        }
        if (Input::GetKeyDown(KeyCode::E))
        {
            userImpulseData.moveUpDownImpulse += impulse;
        }
        if (Input::GetKeyDown(KeyCode::Q))
        {
            userImpulseData.moveUpDownImpulse -= impulse;
        }

        static Vector2 lastMousePos = { 0.0f, 0.0f };
        Vector2 mousePos = lastMousePos;
        Input::GetMousePosition(mousePos.x, mousePos.y);
        Vector2 mouseMovement = (mousePos - lastMousePos);
        lastMousePos = mousePos;

        if (Input::GetMouseButtonDown(MouseButtonID::ButtonMiddle))
        {
            userImpulseData.moveRightLeftImpulse -= mouseMovement.x * controller.settings.translationMultiplier;
            userImpulseData.moveUpDownImpulse += mouseMovement.y * controller.settings.translationMultiplier;
        }
        else if (Input::GetMouseButtonDown(MouseButtonID::ButtonRight))
        {
            userImpulseData.rotatePitchImpulse -= mouseMovement.y * controller.settings.rotationMultiplier;
            userImpulseData.rotateYawImpulse -= mouseMovement.x * controller.settings.rotationMultiplier;
        }

        controller.Update(userImpulseData, deltaTimeInSeconds, cameraSpeed, position, rotation);
    }

}