#pragma once

#include "Core/CoreModule.h"

namespace HE
{
    {
        AllocateVirtualTexture(const VirtualTextureDescription & desc);
        void DestroyVirtualTexture(VirtualTexture * AllocatedVT);
    }

    struct VirtualTextureUpdateSettings
    {

    };

    struct VirtualTexturePage
    {
        VkOffset3D offset;
        VkExtent3D extent;
        VkSparseImageMemoryBind imageMemoryBind; // Sparse image memory bind for this page
        uint32 size; // Page (memory) size in bytes
        uint32 mipLevel;
        uint32 arrayLayer;
        uint32 index;
        bool del;
    };

    class VirtualTexture
    {
    public:
        void Update(const VirtualTextureUpdateSettings & settings);
    private:
        std::vector<VirtualTexturePage> pages;
    };

    class VirtualTextureFeedback
    {

    };

    void VirtualTextureSystemInit();
    void VirtualTextureSystemExit();
}