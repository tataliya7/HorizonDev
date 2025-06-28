#include "RenderGraphResourceRegistry.h"
#include "RenderGraph.h"

namespace Horizon
{
    RenderGraphTexture* RenderGraphResourceRegistry::GetTexture(RenderGraphTextureHandle handle) const
    {
        return renderGraph->textures[handle.GetIndex()];
    }

    RenderGraphBuffer* RenderGraphResourceRegistry::GetBuffer(RenderGraphBufferHandle handle) const
    {
        return renderGraph->buffers[handle.GetIndex()];
    }

    RenderGraphTexture* RenderGraphResourceRegistry::GetImportedTexture(RenderBackendTextureHandle handle) const
    {
        if (renderGraph->importedTextures.find(handle.ToUnit64()) == renderGraph->importedTextures.end())
        {
            return nullptr;
        }
        return GetTexture(renderGraph->importedTextures[handle.ToUnit64()]);
    }

    RenderGraphBuffer* RenderGraphResourceRegistry::GetImportedBuffer(RenderBackendBufferHandle handle) const
    {
        if (renderGraph->importedBuffers.find(handle.ToUnit64()) == renderGraph->importedBuffers.end())
        {
            return nullptr;
        }
        return GetBuffer(renderGraph->importedBuffers[handle.ToUnit64()]);
    }

    const RenderBackendTextureDesc& RenderGraphResourceRegistry::GetTextureDesc(RenderGraphTextureHandle handle) const
    {
        return GetTexture(handle)->GetDesc();
    }

    const RenderBackendBufferDescription& RenderGraphResourceRegistry::GetBufferDesc(RenderGraphBufferHandle handle) const
    {
        return GetBuffer(handle)->GetDesc();
    }

    RenderBackendTextureHandle RenderGraphResourceRegistry::GetRenderBackendTextureHandle(RenderGraphTextureHandle handle) const
    {
        return GetTexture(handle)->GetRenderBackendTextureHandle();
    }

    RenderBackendBufferHandle RenderGraphResourceRegistry::GetRenderBackendBufferHandle(RenderGraphBufferHandle handle) const
    {
        return GetBuffer(handle)->GetRenderBackendBufferHandle();
    }

    int32 RenderGraphResourceRegistry::GetTextureSRVBindlessResourceDescriptorIndex(RenderGraphTextureHandle handle) const
    {
        RenderBackendTextureHandle renderBackendTextureHandle = GetRenderBackendTextureHandle(handle);
        return renderGraph->renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(renderBackendTextureHandle);
    }

    int32 RenderGraphResourceRegistry::GetTextureSRVBindlessResourceDescriptorIndex(RenderGraphTextureHandle handle, uint32 mipLevel) const
    {
        RenderBackendTextureHandle renderBackendTextureHandle = GetRenderBackendTextureHandle(handle);
        return renderGraph->renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(renderBackendTextureHandle, mipLevel);
    }

    int32 RenderGraphResourceRegistry::GetTextureUAVBindlessResourceDescriptorIndex(
        RenderGraphTextureHandle handle,
        uint32 mipLevel) const
    {
        RenderBackendTextureHandle renderBackendTextureHandle = GetRenderBackendTextureHandle(handle);
        return renderGraph->renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(renderBackendTextureHandle, mipLevel);
    }

    int32 RenderGraphResourceRegistry::GetBufferCBVBindlessResourceDescriptorIndex(RenderGraphBufferHandle handle) const
    {
        RenderBackendBufferHandle renderBackendBufferHandle = GetRenderBackendBufferHandle(handle);
        return renderGraph->renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(renderBackendBufferHandle);
    }

    int32 RenderGraphResourceRegistry::GetBufferSRVBindlessResourceDescriptorIndex(RenderGraphBufferHandle handle) const
    {
        RenderBackendBufferHandle renderBackendBufferHandle = GetRenderBackendBufferHandle(handle);
        return renderGraph->renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(renderBackendBufferHandle);
    }

    int32 RenderGraphResourceRegistry::GetBufferUAVBindlessResourceDescriptorIndex(RenderGraphBufferHandle handle) const
    {
        RenderBackendBufferHandle renderBackendBufferHandle = GetRenderBackendBufferHandle(handle);
        return renderGraph->renderBackend->GetBufferUAVBindlessResourceDescriptorIndex(renderBackendBufferHandle);
    }

    RenderBackendPushConstantValues RenderGraphResourceRegistry::GetPushConstantValues() const
    {
        RenderBackendPushConstantValues pushConstantValues = {};
        for (uint32 i = 0; i < RenderBackendPushConstantsSlotCount; i++)
        {
            pushConstantValues.data[i].scalarTypeInt = -1;
            if (pass->shaderConstantsBindings[i].type == RenderGraphShaderConstantBinding::Type::UntrackedResource)
            {
                pushConstantValues.data[i].descriptorIndex = pass->shaderConstantsBindings[i].descriptorIndex;
            }
            else if (pass->shaderConstantsBindings[i].type == RenderGraphShaderConstantBinding::Type::BufferSRV)
            {
                pushConstantValues.data[i].descriptorIndex = GetBufferSRVBindlessResourceDescriptorIndex(pass->shaderConstantsBindings[i].bufferHandle);
            }
            else if (pass->shaderConstantsBindings[i].type == RenderGraphShaderConstantBinding::Type::BufferUAV)
            {
                pushConstantValues.data[i].descriptorIndex = GetBufferUAVBindlessResourceDescriptorIndex(pass->shaderConstantsBindings[i].bufferHandle);
            }
            else if (pass->shaderConstantsBindings[i].type == RenderGraphShaderConstantBinding::Type::TextureSRV)
            {
                pushConstantValues.data[i].descriptorIndex = GetTextureSRVBindlessResourceDescriptorIndex(pass->shaderConstantsBindings[i].textureHandle);
            }
            else if (pass->shaderConstantsBindings[i].type == RenderGraphShaderConstantBinding::Type::TextureUAV)
            {
                pushConstantValues.data[i].descriptorIndex = GetTextureUAVBindlessResourceDescriptorIndex(pass->shaderConstantsBindings[i].textureHandle, 0);
            }
            else if (pass->shaderConstantsBindings[i].type == RenderGraphShaderConstantBinding::Type::ScalarInt)
            {
                pushConstantValues.data[i].scalarTypeInt = pass->shaderConstantsBindings[i].scalarTypeInt;
            }
            else if (pass->shaderConstantsBindings[i].type == RenderGraphShaderConstantBinding::Type::ScalarUnit)
            {
                pushConstantValues.data[i].scalarTypeUint = pass->shaderConstantsBindings[i].scalarTypeUint;
            }
            else if (pass->shaderConstantsBindings[i].type == RenderGraphShaderConstantBinding::Type::ScalarFloat)
            {
                pushConstantValues.data[i].scalarTypeFloat = pass->shaderConstantsBindings[i].scalarTypeFloat;
            }
        }
        return pushConstantValues;
    }
}