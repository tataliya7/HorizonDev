#pragma once

#include "RenderBackendCommon.h"
#include "RenderBackendConfig.h"
#include "RenderBackendHandles.h"
#include "RenderBackendTextureFormat.h"
#include "RenderBackendTypes.h"
#include "RenderBackendInterface.h"
#include "RenderBackendCommandList.h"

namespace Horizon
{
    class RenderBackendGPUProfiler
    {
    public:
        static const uint32 MaxTimingQueryRegionCount = 128;

        RenderBackendGPUProfiler(RenderBackend* renderBackend);
        virtual ~RenderBackendGPUProfiler();

        void BeginFrame(RenderBackendCommandList* commandList);
        void EndFrame(RenderBackendCommandList* commandList);

        uint32 BeginRegion(RenderBackendCommandList* commandList, const char* name);
        void EndRegion(uint32 regionID, RenderBackendCommandList* commandList = nullptr);

        uint32 GetRegionCount() const
        {
            return frames[(currentBufferIndex + 1) % 3].numRegions;
        }

        const char* GetRegionName(uint32 regionID) const
        {
            return frames[(currentBufferIndex + 1) % 3].regions[regionID].name.c_str();
        }

        double GetRegionTime(uint32 regionID) const
        {
            return results[regionID];
        }

    private:
        struct Region
        {
            bool used = false;
            std::string name;
            RenderBackendCommandList* commandList;
        };
        void Init()
        {
            for (uint32 i = 0; i < 3; i++)
            {
                RenderBackendTimingQueryHeapDesc timingQueryHeapDesc(MaxTimingQueryRegionCount);
                frames[i].timingQueryHeap = renderBackend->CreateTimingQueryHeap(&timingQueryHeapDesc, "GPUProfiler");

                RenderBackendBufferDescription bufferDesc = RenderBackendBufferDescription::CreateReadback(sizeof(uint64) * 2 * MaxTimingQueryRegionCount);
                frames[i].timingQueryResults = renderBackend->CreateBuffer(&bufferDesc, nullptr, "TimingQueryResults");
            }

            initialized = true;
        }
        bool initialized = false;
        RenderBackend* renderBackend;
        struct Frame
        {
            uint32 numRegions = 0;
            Region regions[MaxTimingQueryRegionCount];
            RenderBackendBufferHandle timingQueryResults;
            RenderBackendTimingQueryHeapHandle timingQueryHeap;
        };
        Frame frames[3];
        uint32 currentBufferIndex = 0;
        double results[MaxTimingQueryRegionCount] = {};
    };
}
