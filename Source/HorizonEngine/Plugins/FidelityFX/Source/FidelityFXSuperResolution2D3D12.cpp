#include "FidelityFXSuperResolution2.h"
#include "FidelityFXSuperResolution2Private.h"

#include <ffx_fsr2.h>
#include <dx12/ffx_fsr2_dx12.h>

namespace Horizon
{
    bool FidelityFXSuperResolution2DispatchD3D12(
        void* commandList,
        void* context,
        const RenderBackendTextureResource& output,
        const RenderBackendTextureResource& color,
        const RenderBackendTextureResource& depth,
        const RenderBackendTextureResource& motionVectors)
    {
        // FfxFsr2ContextDescription& fsr2InitializationParameters = device->fsr2InitializationParameters;
        // FfxFsr2Context& fsr2Context = device->fsr2Context;
        //
        // // Setup interface.
        // static uint32 previousRenderWidth = 0;
        // static uint32 previousRenderHeight = 0;
        // static uint32 previousTargetWidth = 0;
        // static uint32 previousTargetHeight = 0;
        //
        // if (previousRenderWidth != command.renderWidth ||
        //     previousRenderHeight != command.renderHeight ||
        //     previousTargetWidth != command.targetWidth ||
        //     previousTargetHeight != command.targetHeight)
        // {
        //     // only destroy contexts which are live
        //     if (fsr2InitializationParameters.callbacks.scratchBuffer != nullptr)
        //     {
        //         ffxFsr2ContextDestroy(&fsr2Context);
        //         free(fsr2InitializationParameters.callbacks.scratchBuffer);
        //         fsr2InitializationParameters.callbacks.scratchBuffer = nullptr;
        //     }
        //
        //     const size_t scratchBufferSize = ffxFsr2GetScratchMemorySizeDX12();
        //     void* scratchBuffer = malloc(scratchBufferSize);
        //     FfxErrorCode errorCode = ffxFsr2GetInterfaceDX12(&fsr2InitializationParameters.callbacks, device->GetID3D12Device(), scratchBuffer, scratchBufferSize);
        //     FFX_ASSERT(errorCode == FFX_OK);
        //
        //     fsr2InitializationParameters.device = ffxGetDeviceDX12(device->GetID3D12Device());
        //     fsr2InitializationParameters.maxRenderSize.width = command.renderWidth;
        //     fsr2InitializationParameters.maxRenderSize.height = command.renderHeight;
        //     fsr2InitializationParameters.displaySize.width = command.targetWidth;
        //     fsr2InitializationParameters.displaySize.height = command.targetHeight;
        //     //fsr2InitializationParameters.flags = FFX_FSR2_ENABLE_AUTO_EXPOSURE;
        //
        //     // if (m_bInvertedDepth)
        //     {
        //         fsr2InitializationParameters.flags |= FFX_FSR2_ENABLE_DEPTH_INVERTED;
        //         //fsr2InitializationParameters.flags |= FFX_FSR2_ENABLE_DEPTH_INFINITE;
        //     }
        //
        //     if (device->fsr2EnableDebugCheck)
        //     {
        //         fsr2InitializationParameters.flags |= FFX_FSR2_ENABLE_DEBUG_CHECKING;
        //         fsr2InitializationParameters.fpMessage = &FSR2MessageCallBack;
        //     }
        //
        //     // Input data is HDR
        //     fsr2InitializationParameters.flags |= FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE;
        //
        //     errorCode = ffxFsr2ContextCreate(&fsr2Context, &fsr2InitializationParameters);
        //     FFX_ASSERT(errorCode == FFX_OK);
        //
        //     previousRenderWidth = command.renderWidth;
        //     previousRenderHeight = command.renderHeight;
        //     previousTargetWidth = command.targetWidth;
        //     previousTargetHeight = command.targetHeight;
        //
        //     // device->WaitIdle();
        // }
        //
        // D3D12Texture* output = device->GetTexture(command.output);
        // D3D12Texture* color = device->GetTexture(command.color);
        // D3D12Texture* depth = device->GetTexture(command.depth);
        // D3D12Texture* motionVectors = device->GetTexture(command.motionVectors);
        //
        // FfxFsr2DispatchDescription fsr2DispatchParameters = {};
        //
        // fsr2DispatchParameters.color = ffxGetResourceDX12(
        //     &fsr2Context,
        //     color->GetID3D12Resource(),
        //     L"FSR2_InputColor",
        //     FFX_RESOURCE_STATE_COMPUTE_READ);
        //
        // fsr2DispatchParameters.depth = ffxGetResourceDX12(
        //     &fsr2Context,
        //     depth->GetID3D12Resource(),
        //     L"FSR2_InputDepth",
        //     FFX_RESOURCE_STATE_COMPUTE_READ);
        //
        // fsr2DispatchParameters.motionVectors = ffxGetResourceDX12(
        //     &fsr2Context,
        //     motionVectors->GetID3D12Resource(),
        //     L"FSR2_InputMotionVectors",
        //     FFX_RESOURCE_STATE_COMPUTE_READ);
        //
        // fsr2DispatchParameters.output = ffxGetResourceDX12(
        //     &fsr2Context,
        //     output->GetID3D12Resource(),
        //     L"FSR2_OutputUpscaledColor",
        //     FFX_RESOURCE_STATE_UNORDERED_ACCESS);
        //
        // if (true)
        // {
        //     fsr2DispatchParameters.exposure = ffxGetResourceDX12(
        //         &fsr2Context,
        //         nullptr,
        //         L"FSR2_InputExposure");
        // }
        // else
        // {
        //     // TODO
        // }
        //
        // if (true)
        // {
        //     fsr2DispatchParameters.reactive = ffxGetResourceDX12(
        //         &fsr2Context,
        //         nullptr,
        //         L"FSR2_EmptyInputReactiveMap");
        // }
        // else
        // {
        //     // TODO
        // }
        //
        // if (true)
        // {
        //     fsr2DispatchParameters.transparencyAndComposition = ffxGetResourceDX12(
        //         &fsr2Context,
        //         nullptr,
        //         L"FSR2_EmptyTransparencyAndCompositionMap");
        // }
        // else
        // {
        //     // TODO
        // }
        //
        // fsr2DispatchParameters.commandList = ffxGetCommandListDX12(commandList->GetID3D12GraphicsCommandList());
        // fsr2DispatchParameters.jitterOffset.x = command.jitterOffsetX;
        // fsr2DispatchParameters.jitterOffset.y = command.jitterOffsetY;
        // fsr2DispatchParameters.motionVectorScale.x = (float)command.renderWidth;
        // fsr2DispatchParameters.motionVectorScale.y = (float)command.renderHeight;
        // fsr2DispatchParameters.reset = command.reset;
        // fsr2DispatchParameters.enableSharpening = command.enableSharpening;
        // fsr2DispatchParameters.sharpness = command.sharpeness;
        // fsr2DispatchParameters.frameTimeDelta = command.deltaTime * 1000.0f; // @note frameTimeDelta is expressed in milliseconds
        // fsr2DispatchParameters.preExposure = 1.0f;
        // fsr2DispatchParameters.renderSize.width = command.renderWidth;
        // fsr2DispatchParameters.renderSize.height = command.renderHeight;
        // fsr2DispatchParameters.cameraFar = command.cameraFarPlane;
        // fsr2DispatchParameters.cameraNear = command.cameraNearPlane;
        // fsr2DispatchParameters.cameraFovAngleVertical = command.cameraFovAngleVertical;
        // fsr2DispatchParameters.viewSpaceToMetersFactor = 1.0f;
        //
        // if (fsr2InitializationParameters.flags & FFX_FSR2_ENABLE_DEPTH_INVERTED)
        // {
        //     std::swap(fsr2DispatchParameters.cameraFar, fsr2DispatchParameters.cameraNear);
        // }
        //
        // FfxErrorCode errorCode = ffxFsr2ContextDispatch(&fsr2Context, &fsr2DispatchParameters);
        // FFX_ASSERT(errorCode == FFX_OK);

        return false;
    }
}