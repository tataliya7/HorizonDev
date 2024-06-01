#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"
#include "Core/Serialization/SerializationModule.h"

namespace Horizon
{
    enum class LightType
    {
        Distant = 0,
        Point = 1,
        Spot = 2,
        Area = 3,
        Mesh = 4,
    };

    class LightComponent
    {
    public:

        LightType type = LightType::Distant;

        // Common

        bool useColorTemperature = false;

        /** Color temperature in Kelvin. */
        float colorTemperature = 6500.0f;

        Vector3 color = Vector3(1.0f, 1.0f, 1.0f);
        float luminousIntensity = 1.0f;

        float radius;

        /** Apex angle in degrees. */
        float apexAngleInDegrees = 0.545f;

        bool useRayTracingShadows = false;

        bool castShadows = false;

        // Cascade Shadow Maps
        int32 numShadowCascades = 3;

        float cascadeSplitLambda = 0.6f;

        float maxShadowDistance = 100.0f;

        float shadowMapDepthBiasConstantFactor = -1.0f;

        float shadowMapDepthBiasSlopeFactor = -5.0f;

        float shadowSharpeness;

        // Atmosphere
        Vector3 atmosphericLightDiskColorFactor = Vector3(1.0f, 1.0f, 1.0f);

        Vector3 GetAtmosphericLightDiskColorFactor() const
        {
            return atmosphericLightDiskColorFactor;
        }

        float GetMaxShadowDistance() const
        {
            return maxShadowDistance;
        }

        // Punctual Light

        // Area Light

        static Vector3 GetLinearColorFromColorTemperature(float colorTemperature)
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

            return Vector3(R, G, B);
        }

        Vector3 GetPhysicalLightColor() const
        {
            Vector3 result = color * luminousIntensity;
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

        bool CastShadows() const
        {
            return castShadows;
        }

        bool UseRayTracingShadows() const
        {
            return useRayTracingShadows;
        }

        uint32 GetNumDynamicShadowCascades() const
        {
            return numShadowCascades;
        }

        float GetCascadeSplitLambda() const
        {
            return cascadeSplitLambda;
        }

        Vector3 GetDirection() const
        {
            return forwardVec;
        }

        // Non-serialized
        Vector3 position;
        Vector3 forwardVec;
        Vector3 rightVec;
        Vector3 upVec;

    private:

        LightRenderProxy* renderProxy = nullptr;
    };
}
