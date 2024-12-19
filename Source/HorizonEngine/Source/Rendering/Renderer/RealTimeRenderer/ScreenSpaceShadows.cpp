#include "RealTimeRenderer.h"

#include "BendSSS/bend_sss_cpu.h"

namespace Horizon
{
    void DispatchScreenSpaceShadowsBend(
        RenderGraph& renderGraph,
        const ShaderLibrary* shaderLibrary,
        const SceneView& view,
        const LightRenderObject& light,
        const Extent2D& renderResolution,
        RenderGraphTextureHandle screenSpaceShadowMaskTexture)
    {
        // TODO
        const float surfaceThickness = 0.005f;

        const Vector4f lightDirection = view.transformations.worldToClipMatrix * Vector4f(light.GetDirection(), 0.0f);

        float lightProjection[4] = { lightDirection.x, lightDirection.y, lightDirection.z, lightDirection.w };
        int viewportSize[2] = { int(renderResolution.width), int(renderResolution.height) };
        int minRenderBounds[2] = { 0, 0 };
        int maxRenderBounds[2] = { viewportSize[0], viewportSize[1] };
        const Bend::DispatchList dispatchList = Bend::BuildDispatchList(lightProjection, viewportSize, minRenderBounds, maxRenderBounds);

        RenderGraphTextureDesc outputTextureDesc = RenderGraphTextureDesc::Create2D(
            renderResolution.width,
            renderResolution.height,
            RenderBackendTextureFormat::R32Float,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::UnorderedAccess);
        RenderGraphTextureHandle outputTexture = renderGraph.CreateTexture(outputTextureDesc, "BendSSSOutputTexture");

        for (int dispatchIndex = 0; dispatchIndex < dispatchList.DispatchCount; dispatchIndex++)
        {
            const Bend::DispatchData& dispatchData = dispatchList.Dispatch[dispatchIndex];

            renderGraph.AddPass(
                std::format("Bend SSS (Compute, {}x{})", outputTextureDesc.width, outputTextureDesc.height),
                RenderGraphPassFlags::Compute,
                [&](RenderGraphBuilder& builder)
                {
                    RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();

                    RenderGraphTextureHandle sceneDepthTexture = builder.ReadTexture(sceneTextures.sceneDepthTexture, RenderBackendResourceState::ShaderResource);
                    outputTexture = builder.WriteTexture(outputTexture, RenderBackendResourceState::UnorderedAccess);

                    return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                    {
                        uint32 threadGroupCountX = dispatchData.WaveCount[0];
                        uint32 threadGroupCountY = dispatchData.WaveCount[1];
                        uint32 threadGroupCountZ = dispatchData.WaveCount[2];

                        RenderBackendShaderConstants shaderConstants = {};
                        shaderConstants.BindTextureSRV(0, registry.GetTextureSRVBindlessResourceDescriptorIndex(sceneDepthTexture));
                        shaderConstants.BindTextureUAV(1, registry.GetTextureSRVBindlessResourceDescriptorIndex(outputTexture, 0));
                        shaderConstants.BindScalar(2, surfaceThickness);
                        shaderConstants.BindScalar(3, dispatchList.LightCoordinate_Shader[0]);
                        shaderConstants.BindScalar(4, dispatchList.LightCoordinate_Shader[1]);
                        shaderConstants.BindScalar(5, dispatchList.LightCoordinate_Shader[2]);
                        shaderConstants.BindScalar(6, dispatchList.LightCoordinate_Shader[3]);
                        shaderConstants.BindScalar(7, dispatchData.WaveOffset_Shader[0]);
                        shaderConstants.BindScalar(8, dispatchData.WaveOffset_Shader[1]);
                        shaderConstants.BindScalar(9, 1.0f / float(renderResolution.width));
                        shaderConstants.BindScalar(10, 1.0f / float(renderResolution.height));

                        RenderBackendShaderHandle computeShader = shaderLibrary->GetShader(ShaderID::ScreenSpaceShadowsBend);

                        commandList.Dispatch(
                            computeShader,
                            shaderConstants,
                            threadGroupCountX,
                            threadGroupCountY,
                            threadGroupCountZ);
                    };
                });
        }
    }

    void RealTimeRenderer::DispatchScreenSpaceShadows(
        RenderGraph& renderGraph,
        const SceneView& view,
        const LightRenderObject& light)
    {
#if !HORIZON_CONFIGURATION_RELEASE
        if (!light.IsDistantLight())
        {
            LogWarning(GLogger, "Currently screen space shadows only supports distant light.");
            return;
        }
#endif

        RealTimeRendererSceneTextures& sceneTextures = renderGraph.blackboard.Get<RealTimeRendererSceneTextures>();
        RenderGraphTextureHandle screenSpaceShadowMaskTexture = sceneTextures.shadowMaskTexture;

        //if ()
        //{
        //    DispatchScreenSpaceShadowsStochastic();
        //}
        //else
        {
            DispatchScreenSpaceShadowsBend(renderGraph, shaderLibrary, view, light, renderResolution, screenSpaceShadowMaskTexture);
        }
    }
}