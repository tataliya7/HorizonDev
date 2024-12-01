// module;
//
// #include "Foundation/FoundationModule.h"
// #include "Rendering/RenderingModule.h"
//
// #include <FidelityFX/host/ffx_brixelizergi.h>
// #include <FidelityFX/host/backends/dx12/ffx_dx12.h>
//
// module FidelityFX.BrixelizerGI:Private;
//
// import FidelityFX.BrixelizerGI;
//
// namespace Horizon
// {
//     struct FidelityFXBrixelizerGIState
//     {
//         FfxBrixelizerGIContextDescription brixelizerGIContextDescription;
//         FfxBrixelizerGIContext brixelizerGIContext;
//         bool initialized;
//         //uint32 viewportID;
//     };
//
//     class FidelityFXBrixelizerGI : public TemporalSuperSamplingInterface
//     {
//     public:
//         FidelityFXBrixelizerGI(RenderBackend* renderBackend);
//         virtual ~FidelityFXBrixelizerGI();
//         TemporalSuperSamplingConstants GetConstants() const
//         {
//             return constants;
//         }
//         void SetOptions(const TemporalSuperSamplingOptions& options) override;
//         void SetConstants(const TemporalSuperSamplingConstants& constants) override;
//         TemporalSuperSamplingOptimalSettings GetOptimalSettings() const override;
//         uint32 GetJitterPhaseCount(uint32 renderWidth, uint32 targetWidth) const override;
//         Vector2 GetJitterOffset(uint32 index, uint32 phaseCount) const override;
//         RenderGraphTextureHandle Dispatch(RenderGraph& renderGraph, const SceneView& view, const TemporalSuperSamplingDispatchDescription& dispatchDescription) override;
//
//     private:
//
//         friend bool FidelityFXSuperResolution2DispatchD3D12(
//             void* commandList,
//             void* context,
//             const RenderBackendTextureResource& output,
//             const RenderBackendTextureResource& color,
//             const RenderBackendTextureResource& depth,
//             const RenderBackendTextureResource& motionVectors,
//             const RenderBackendTextureResource& exposure);
//
//         friend bool FidelityFXSuperResolution2DispatchVulkan(
//             void* commandList,
//             void* context,
//             const RenderBackendTextureResource& output,
//             const RenderBackendTextureResource& color,
//             const RenderBackendTextureResource& depth,
//             const RenderBackendTextureResource& motionVectors,
//             const RenderBackendTextureResource& exposure);
//
//         RenderBackend* renderBackend;
//         FidelityFXBrixelizerGIState* state;
//         TemporalSuperSamplingOptions options;
//         TemporalSuperSamplingConstants constants;
//     };
//
//     bool FidelityFXBrixelizerGIDispatchD3D12(
//         void* commandList,
//         void* context,
//         const RenderBackendTextureResource& output,
//         const RenderBackendTextureResource& color,
//         const RenderBackendTextureResource& depth,
//         const RenderBackendTextureResource& motionVectors,
//         const RenderBackendTextureResource& exposure)
//     {
//         FidelityFXBrixelizerGI* brixelizerGI = static_cast<FidelityFXBrixelizerGI*>(context);
//         FidelityFXBrixelizerGIState& brixelizerGIState = *brixelizerGI->state;
//
//         RenderBackendDevice device = brixelizerGIState->renderBackend->GetNativeDevice();
//         ID3D12Device* d3d12Device = static_cast<ID3D12Device*>(device.device);
//         FfxDevice ffxDevice = ffxGetDeviceDX12(d3d12Device);
//
//         FfxBrixelizerGIContextDescription& ffxBrixelizerGIContextDescription = brixelizerGIState.brixelizerGIContextDescription;
//         FfxBrixelizerGIContext& ffxBrixelizerGIContext = brixelizerGIState.brixelizerGIContext;
//         {
//             {
//                 const size_t scratchBufferSize = ffxGetScratchMemorySizeDX12(1);
//                 void* scratchBuffer = malloc(scratchBufferSize);
//                 memset(scratchBuffer, 0, scratchBufferSize);
//
//                 FfxErrorCode errorCode = ffxGetInterfaceDX12(&ffxBrixelizerGIContextDescription.backendInterface, ffxDevice, scratchBuffer, scratchBufferSize, 1);
//                 FFX_ASSERT(errorCode == FFX_OK);
//             }
//             ffxBrixelizerGIContextDescription.flags              = ...;                    ///< A bit field representings various options.
//             ffxBrixelizerGIContextDescription.internalResolution = ...;                    ///< The scale at which Brixelizer GI will output GI at internally. The output will be internally upscaled to the specified displaySize.
//             ffxBrixelizerGIContextDescription.displaySize        = ...;                    ///< The size of the presentation resolution targeted by the upscaling process.
//
//             FfxErrorCode errorCode = ffxBrixelizerGIContextCreate(&ffxBrixelizerGIContext, &ffxBrixelizerGIContextDescription);
//             FFX_ASSERT(errorCode == FFX_OK);
//
//             errorCode = ffxBrixelizerGIContextDestroy(&ffxBrixelizerGIContext);
//             FFX_ASSERT(errorCode == FFX_OK);
//         }
//
//         FfxBrixelizerGIDispatchDescription ffxBrixelizerGIDispatchDescription = {};
//         ffxBrixelizerGIDispatchDescription.view = ...;                    ///< The view matrix for the scene in row major order.
//         ffxBrixelizerGIDispatchDescription.projection = ...;              ///< The projection matrix for the scene in row major order.
//         ffxBrixelizerGIDispatchDescription.prevView = ...;                ///< The view matrix for the previous frame of the scene in row major order.
//         ffxBrixelizerGIDispatchDescription.prevProjection = ...;          ///< The projection matrix for the scene in row major order.
//         ffxBrixelizerGIDispatchDescription.cameraPosition = ...;          ///< A 3-dimensional vector representing the position of the camera.
//         ffxBrixelizerGIDispatchDescription.startCascade = ...;            ///< The index of the start cascade for use with ray marching with Brixelizer.
//         ffxBrixelizerGIDispatchDescription.endCascade = ...;              ///< The index of the end cascade for use with ray marching with Brixelizer.
//         ffxBrixelizerGIDispatchDescription.rayPushoff = ...;              ///< The distance from a surface along the normal vector to offset the diffuse ray origin.
//         ffxBrixelizerGIDispatchDescription.sdfSolveEps = ...;             ///< The epsilon value for ray marching to be used with Brixelizer for diffuse rays.
//         ffxBrixelizerGIDispatchDescription.specularRayPushoff = ...;      ///< The distance from a surface along the normal vector to offset the specular ray origin.
//         ffxBrixelizerGIDispatchDescription.specularSDFSolveEps = ...;     ///< The epsilon value for ray marching to be used with Brixelizer for specular rays.
//         ffxBrixelizerGIDispatchDescription.tMin = ...;                    ///< The TMin value for use with Brixelizer.
//         ffxBrixelizerGIDispatchDescription.tMax = ...;                    ///< The TMax value for use with Brixelizer.
//         ffxBrixelizerGIDispatchDescription.environmentMap = ...;          ///< The environment map.
//         ffxBrixelizerGIDispatchDescription.prevLitOutput = ...;           ///< The lit output from the previous frame.
//         ffxBrixelizerGIDispatchDescription.depth = ...;                   ///< The input depth buffer.
//         ffxBrixelizerGIDispatchDescription.historyDepth = ...;            ///< The previous frame input depth buffer.
//         ffxBrixelizerGIDispatchDescription.normal = ...;                  ///< The input normal buffer.
//         ffxBrixelizerGIDispatchDescription.historyNormal = ...;           ///< The previous frame input normal buffer.
//         ffxBrixelizerGIDispatchDescription.roughness = ...;               ///< The resource containing roughness information.
//         ffxBrixelizerGIDispatchDescription.motionVectors = ...;           ///< The input motion vectors texture.
//         ffxBrixelizerGIDispatchDescription.noiseTexture = ...;            ///< The input blue noise texture.
//         ffxBrixelizerGIDispatchDescription.normalsUnpackMul = ...;        ///< A multiply factor to transform the normal to the space expected by Brixelizer GI.
//         ffxBrixelizerGIDispatchDescription.normalsUnpackAdd = ...;        ///< An offset to transform the normal to the space expected by Brixelizer GI.
//         ffxBrixelizerGIDispatchDescription.isRoughnessPerceptual = ...;   ///< A boolean to describe the space used to store roughness in the materialParameters texture. If false, we assume roughness squared was stored in the Gbuffer.
//         ffxBrixelizerGIDispatchDescription.roughnessChannel = ...;        ///< The channel to read the roughness from the roughness texture
//         ffxBrixelizerGIDispatchDescription.roughnessThreshold = ...;      ///< Regions with a roughness value greater than this threshold won't spawn specular rays.
//         ffxBrixelizerGIDispatchDescription.environmentMapIntensity = ...; ///< The value to scale the contribution from the environment map.
//         ffxBrixelizerGIDispatchDescription.motionVectorScale = ...;       ///< The scale factor to apply to motion vectors.
//         ffxBrixelizerGIDispatchDescription.sdfAtlas = ...;                ///< The SDF Atlas resource used by Brixelizer.
//         ffxBrixelizerGIDispatchDescription.bricksAABBs = ...;             ///< The brick AABBs resource used by Brixelizer.
//         ffxBrixelizerGIDispatchDescription.outputDiffuseGI = ...;         ///< A texture to write the output diffuse GI calculated by Brixelizer GI.
//         ffxBrixelizerGIDispatchDescription.outputSpecularGI = ...;        ///< A texture to write the output specular GI calculated by Brixelizer GI.
//         ffxBrixelizerGIDispatchDescription.brixelizerContext = ...;       ///< A pointer to the Brixelizer context for use with Brixelizer GI.
//         for (uint32 i = 0; i < 24; i++)
//         {
//             ffxBrixelizerGIDispatchDescription.cascadeAABBTrees[i] = ...; ///< The cascade AABB tree resources used by Brixelizer.
//             ffxBrixelizerGIDispatchDescription.cascadeBrickMaps[i] = ...; ///< The cascade brick map resources used by Brixelizer.
//         }
//
//         FfxCommandList ffxCommandList = ffxGetCommandListDX12(static_cast<ID3D12CommandList*>(commandList));
//
//         FfxErrorCode error = ffxBrixelizerGIContextDispatch(&ffxBrixelizerGIContext, &ffxBrixelizerGIDispatchDescription, ffxCommandList);
//         assert(error == FFX_OK);
//     }
// }