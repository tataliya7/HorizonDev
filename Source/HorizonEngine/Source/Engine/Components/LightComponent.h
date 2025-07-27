#pragma once

#include "Foundation/FoundationModule.h"
#include "Engine/Serialization/SerializationModule.h"

namespace Horizon
{
    class RenderScene;
    class LightRenderObject;

    class LightComponent
    {
    public:

        enum class Type
        {
            Distant = 0,
            Point = 1,
            Spot = 2,
            Area = 3,
            Mesh = 4,
        };

        Type type = Type::Distant;

        // Common

        bool useColorTemperature = false;

        /** Color temperature in Kelvin. */
        float colorTemperature = 6500.0f;

        Vector3f color = Vector3f(1.0f, 1.0f, 1.0f);

        float luminousIntensity = 1.0f;

        float radius;

        /** Apex angle in degrees. */
        float apexAngleInDegrees = 0.545f;

        bool useRayTracingShadows = false;

        bool castDynamicShadows = true;

        float maxShadowDistance = 100.0f;

        // Cascade Shadow Maps
        uint32 shadowCascadeCount = 4;

        uint32 shadowMapSize = 2048;

        float shadowCascadeSplitLambda = 0.5f;

        float shadowCascadeBlendScale = 0.1f;

        float shadowFadeOutFactor = 0.1f;

        float shadowMapDepthBiasConstantFactor = -1.0f;

        float shadowMapDepthBiasSlopeFactor = -5.0f;

        float shadowSharpness;

        // Screen Space Shadows
        bool enableScreenSpaceShadows = false;

        float screenSpaceShadowsSurfaceThickness = 0.005f;

        float screenSpaceShadowsShadowContrast = 4.0f;

        bool usedAsAtmosphericLight = false;

        bool enableScreenSpaceLightShafts = false;

        float lightShaftsIntensity = 1.0f;

        Vector3f lightShaftsColor = Vector3f(1.0f, 1.0f, 1.0f);

        // Atmosphere
        Vector3f atmosphericLightDiskColorFactor = Vector3f(1.0f, 1.0f, 1.0f);

        Vector3f GetAtmosphericLightDiskColorFactor() const
        {
            return atmosphericLightDiskColorFactor;
        }

        float GetMaxShadowDistance() const
        {
            return maxShadowDistance;
        }

        // Punctual Light

        // Area Light

        static Vector3f GetLinearColorFromColorTemperature(float colorTemperature)
        {
            colorTemperature = std::clamp(colorTemperature, 1000.0f, 15000.0f);

            // Approximate Planckian locus in CIE 1960 UCS
            float u = (0.860117757f + 1.54118254e-4f * colorTemperature + 1.28641212e-7f * colorTemperature * colorTemperature) / (1.0f + 8.42420235e-4f * colorTemperature + 7.08145163e-7f * colorTemperature * colorTemperature);
            float v = (0.317398726f + 4.22806245e-5f * colorTemperature + 4.20481691e-8f * colorTemperature * colorTemperature) / (1.0f - 2.89741816e-5f * colorTemperature + 1.61456053e-7f * colorTemperature * colorTemperature);

            float x = 3.0f * u / (2.0f * u - 8.0f * v + 4.0f);
            float y = 2.0f * v / (2.0f * u - 8.0f * v + 4.0f);
            float z = 1.0f - x - y;

            float Y = 1.0f;
            float X = Y / y * x;
            float Z = Y / y * z;

            // XYZ to RGB with BT.709 primaries
            float R =  3.2404542f * X + -1.5371385f * Y + -0.4985314f * Z;
            float G = -0.9692660f * X +  1.8760108f * Y +  0.0415560f * Z;
            float B =  0.0556434f * X + -0.2040259f * Y +  1.0572252f * Z;

            return Vector3f(R, G, B);
        }

        Vector3f GetPhysicalLightColor() const
        {
            Vector3f result = color * luminousIntensity;
            if (useColorTemperature)
            {
                result *= GetLinearColorFromColorTemperature(colorTemperature);
            }
            return result;
        }

        float GetHalfApexAngleInRadians() const
        {
            return 0.5f * apexAngleInDegrees * std::numbers::pi_v<float> / 180.0f;
        }

        bool CastDynamicShadows() const
        {
            return castDynamicShadows;
        }

        bool UseRayTracingShadows() const
        {
            return useRayTracingShadows;
        }

        uint32 GetNumDynamicShadowCascades() const
        {
            return shadowCascadeCount;
        }

        float GetCascadeSplitLambda() const
        {
            return shadowCascadeSplitLambda;
        }

        Vector3f GetDirection() const
        {
            return direction;
        }

        // Non-serialized
        Vector3f position;
        Vector3f direction;
        Vector3f rightVec;
        Vector3f upVec;

        bool IsRenderObjectValid() const;
        void CreateRenderObject(RenderScene* scene);
        void DestroyRenderObject(RenderScene* scene);
        void UpdateRenderObject();

    private:

        LightRenderObject* renderObject = nullptr;
    };
}