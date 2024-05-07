#pragma once

#include "Engine/HorizonEngineModule.h"

namespace Horizon
{
    class FirstPersonCameraController : public Scriptable
    {
    public:

        FirstPersonCameraController() = default;
        virtual ~FirstPersonCameraController() = default;

        float cameraSpeed = 1.0f;
        float maxTranslationVelocity = FLOAT_MAX;
        float maxRotationVelocity = FLOAT_MAX;
        float translationMultiplier = 10.0f;
        float rotationMultiplier = 10.0f;

        void OnCreate()
        {

        }

        void OnDestroy()
        {

        }

        void OnUpdate(float deltaTime)
        {
            auto& tranform = GetComponent<TransformComponent>();
            UpdateTransform(deltaTime, tranform.position, tranform.rotation);
        }

    protected:

        void UpdateTransform(float deltaTime, Vector3& outCameraPosition, Vector3& outCameraEuler);
    };
}