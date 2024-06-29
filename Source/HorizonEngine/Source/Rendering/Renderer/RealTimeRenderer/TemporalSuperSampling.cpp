#include "RealTimeRenderer.h"
#include "TemporalSuperSampling.h"

namespace Horizon
{
    void JitterProjectionMatrix(Matrix4x4& outProjectionMatrix, Vector2& outJitterOffset, const Extent2D& renderResolution, float upscaleRatio)
    {
        static const auto HaltonSequence = [](uint32 index, uint32 base)
            {
                float f = 1.0f, result = 0.0f;
                for (uint32 i = index; i > 0;)
                {
                    f /= static_cast<float>(base);
                    result = result + f * static_cast<float>(i % base);
                    i = static_cast<uint32>(floorf(static_cast<float>(i) / static_cast<float>(base)));
                }
                return result;
            };

        uint32 temporalSampleCount = 32;
        temporalSampleCount = uint32(float(temporalSampleCount) * std::max(1.0f, upscaleRatio * upscaleRatio));
        temporalSampleCount = Math::Clamp(temporalSampleCount, 1, 255);

        static uint32 temporalSampleIndex = 0;
        temporalSampleIndex = (temporalSampleIndex + 1) % temporalSampleCount;

        // Unit pixel space offset
        float offsetX = HaltonSequence(temporalSampleIndex + 1, 2) - 0.5f;
        float offsetY = HaltonSequence(temporalSampleIndex + 1, 3) - 0.5f;

        // Clip space offset [-1, 1]
        // -y for clip space to uv space
        Vector2 jitterOffset = { offsetX * 2.0f / (float)renderResolution.width, -offsetY * 2.0f / (float)renderResolution.height };

        /*
         * Horizon Engine uses righted-handed coordinate system,
         * the w component of clip space position is -Zc instead of Zc,
         * so it should be multiplied by -1.
         */
        outProjectionMatrix[2][0] += -jitterOffset.x;
        outProjectionMatrix[2][1] += -jitterOffset.y;
        outJitterOffset = { offsetX, offsetY };
    }

    bool RealTimeRenderer::IsSuperResolutionEnabled() const
    {
        return features.enableSuperResolution;
    }

    RenderGraphTextureHandle DispatchCustomTemporalSuperSampling(TemporalSuperSamplingInterface* interface, RenderGraph& renderGraph, const SceneView& view, const TemporalSuperSamplingDispatchDescription& dispatchDescription)
    {
        RenderGraphTextureHandle outputTexture = interface->Dispatch(renderGraph, view, dispatchDescription);
        return outputTexture;
    }
}