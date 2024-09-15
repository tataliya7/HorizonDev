#include "RealTimeRenderer.h"
#include "TemporalSuperSampling.h"

namespace Horizon
{
    uint32 TemporalSuperSamplingGetJitterPhaseCount(uint32 renderWidth, uint32 targetWidth)
    {
        constexpr float basePhaseCount = 32.0f;
        float upscaleRatio = float(targetWidth) / float(renderWidth);
        uint32 jitterPhaseCount = uint32(basePhaseCount * std::max(1.0f, upscaleRatio * upscaleRatio));
        jitterPhaseCount = std::clamp(jitterPhaseCount, 1u, 255u);
        return jitterPhaseCount;
    }

    Vector2 TemporalSuperSamplingGetJitterOffset(uint32 index, uint32 phaseCount)
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

        float offsetX = HaltonSequence(index + 1, 2) - 0.5f;
        float offsetY = HaltonSequence(index + 1, 3) - 0.5f;

        // Unit pixel space offset
        Vector2 jitterOffset = Vector2(offsetX, offsetY);
        return jitterOffset;
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