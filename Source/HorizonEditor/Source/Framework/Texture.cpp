
namespace Horizon
{
    RenderBackendTextureHandle LoadTextureFromHDRFile(RenderBackend* renderBackend, const char* filename, RenderBackendTextureDesc* outDesc)
    {
        if (!stbi_is_hdr(filename))
        {
            return RenderBackendTextureHandle::Null;
        }

        int iw = 0, ih = 0, c = 0;
        stbi_set_flip_vertically_on_load(false);
        void* data = stbi_loadf(filename, &iw, &ih, &c, STBI_rgb_alpha);
        if (data == nullptr)
        {
            return RenderBackendTextureHandle::Null;
        }

        assert(iw >= 0 && ih >= 0 && c >= 0);

        uint64 bufferSize = uint64(iw) * uint64(ih) * 4 * sizeof(float);

        RenderBackendTextureDesc desc = RenderBackendTextureDesc::CreateTexture2D(iw, ih, 1, RenderBackendTextureFormat::RGBA32Float);
        RenderBackendTextureHandle texture = renderBackend->CreateTexture(&desc, data, filename);

        stbi_image_free(data);

        if (outDesc)
        {
            *outDesc = desc;
        }

        return texture;
    }

    void LoadFile(const char* filename, std::vector<uint8>& outData)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            // LogError(GLogger, std::format(("Failed to open shader source file.")));
            return;
        }
        size_t fileSize = (size_t)file.tellg();
        outData.resize(fileSize);
        file.seekg(0);
        file.read(reinterpret_cast<char*>(outData.data()), fileSize);
        file.close();
    }

    RenderBackendTextureHandle LoadTextureFromFile(RenderBackend* renderBackend, ShaderLibrary* shaderLibrary, const char* filename, bool autoMipmaps, bool filpY, RenderBackendTextureFormat format)
    {
        RenderBackendTextureHandle texture = RenderBackendTextureHandle::Null;

        std::filesystem::path path(filename);
        //if (path.extension() == ".dds")
        //{
        //    std::vector<uint8> data;
        //    LoadFile(filename, data);
        //    int size = data.size();
        //    void* dds_data = data.data();
        //    assert(dds_data);
        //    ddsktx_texture_info tc = { 0 };
        //    if (ddsktx_parse(&tc, dds_data, size, NULL))
        //    {
        //        assert(tc.depth == 1);
        //        assert(!(tc.flags & DDSKTX_TEXTURE_FLAG_CUBEMAP));
        //        assert(tc.num_layers == 1);
        //    }

        //    //typedef struct ddsktx_texture_info
        //    //{
        //    //    int                 data_offset;   // start offset of pixel data
        //    //    int                 size_bytes;
        //    //    ddsktx_format       format;
        //    //    unsigned int        flags;         // ddsktx_texture_flags
        //    //    int                 width;
        //    //    int                 height;
        //    //    int                 depth;
        //    //    int                 num_layers;
        //    //    int                 num_mips;
        //    //    int                 bpp;
        //    //    int                 metadata_offset; // ktx only
        //    //    int                 metadata_size;   // ktx only
        //    //} ddsktx_texture_info;

        //    PixelFormat format = PixelFormat::RGBA8Unorm;
        //    switch (tc.format)
        //    {
        //    case DDSKTX_FORMAT_BC1: format = PixelFormat::BC1Unorm;  break;
        //    case DDSKTX_FORMAT_BC2: format = PixelFormat::BC2Unorm;  break;
        //    case DDSKTX_FORMAT_BC3: format = PixelFormat::BC3Unorm;  break;
        //    case DDSKTX_FORMAT_BC4: format = PixelFormat::BC4Unorm;  break;
        //    case DDSKTX_FORMAT_BC5: format = PixelFormat::BC5Unorm;  break;
        //    case DDSKTX_FORMAT_BC6H: format = PixelFormat::BC6HSF;  break;
        //    case DDSKTX_FORMAT_BC7: format = PixelFormat::BC7Unorm;  break;
        //    }

        //    RenderBackendTextureDesc desc = RenderBackendTextureDesc::CreateTexture2D(tc.width, tc.height, tc.num_mips, format);
        //    texture = RenderBackendCreateTexture(renderBackend, ~0u, &desc, nullptr, filename);

        //    {
        //        RenderBackendTextureUploadDataDesc uploadDataDesc;
        //        for (uint32 mipLevel = 0; mipLevel < tc.num_mips; mipLevel++)
        //        {
        //            ddsktx_sub_data sub_data;
        //            ddsktx_get_sub(&tc, &sub_data, dds_data, size, 0, 0, mipLevel);
        //            uploadDataDesc.AddSubresourceData(mipLevel, 0, sub_data.buff, sub_data.width, sub_data.height, sub_data.size_bytes, sub_data.row_pitch_bytes);
        //        }
        //        RenderBackendUploadTexture(renderBackend, texture, uploadDataDesc);
        //    }
        //    //free(dds_data);
        //}
        //else
        {
            int iw = 0, ih = 0, c = 0;
            stbi_set_flip_vertically_on_load(filpY);

            unsigned char* data = stbi_load(filename, &iw, &ih, &c, STBI_default);

            if (data == nullptr)
            {
                return RenderBackendTextureHandle::Null;
            }
            uint64 bufferSize = iw * ih * 4;
            unsigned char* buffer = static_cast<unsigned char*>(_aligned_malloc(bufferSize, 32));

            for (uint32 y = 0; y < uint32(ih); y++)
            {
                for (uint32 x = 0; x < uint32(iw); x++)
                {
                    uint32 idx = x + y * iw;
                    switch (c)
                    {
                    case STBI_grey:
                    {
                        buffer[idx * 4 + 0] = data[idx];
                        buffer[idx * 4 + 2] = buffer[idx * 4 + 1] = buffer[idx * 4 + 0];
                        buffer[idx * 4 + 3] = 255;
                        break;
                    }
                    case STBI_grey_alpha:
                    {
                        buffer[idx * 4 + 0] = data[idx * 2 + 0];
                        buffer[idx * 4 + 2] = buffer[idx * 4 + 1] = buffer[idx * 4 + 0];
                        buffer[idx * 4 + 3] = data[idx * 2 + 1];
                        break;
                    }
                    case STBI_rgb:
                    {
                        buffer[idx * 4 + 0] = data[idx * 3 + 0];
                        buffer[idx * 4 + 1] = data[idx * 3 + 1];
                        buffer[idx * 4 + 2] = data[idx * 3 + 2];
                        buffer[idx * 4 + 3] = 255;
                        break;
                    }
                    case STBI_rgb_alpha:
                    {
                        buffer[idx * 4 + 0] = data[idx * 4 + 0];
                        buffer[idx * 4 + 1] = data[idx * 4 + 1];
                        buffer[idx * 4 + 2] = data[idx * 4 + 2];
                        buffer[idx * 4 + 3] = data[idx * 4 + 3];
                        break;
                    }
                    default: break;
                    }
                }
            }

            stbi_image_free(data);

            uint32 mipLeveles = autoMipmaps ? Math::MaxNumMipLevels(iw, ih) : 1;

            RenderBackendTextureDesc desc = RenderBackendTextureDesc::CreateTexture2D(
                iw,
                ih,
                mipLeveles,
                format);
            texture = renderBackend->CreateTexture(&desc, buffer, filename);

            _aligned_free(buffer);

            if (mipLeveles > 1)
            {
                uint32 deviceMask = ~0u;

                RenderBackendCommandList* commandList = new RenderBackendCommandList(GArena);
                Texture2DGenerateMips(shaderLibrary, *commandList, texture, iw, ih, mipLeveles);
                renderBackend->SubmitCommandLists(&commandList, 1, RenderBackendSwapChainHandle::Null);
                renderBackend->FlushRenderDevices();

                delete commandList;

            }
        }

        return texture;
    }
}