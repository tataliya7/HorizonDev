#pragma once

#include "RenderBackendCommon.h"
#include "RenderBackendCommandList.h"

namespace HE
{
    class RenderBackendGPUProfiler
    {
    public:

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
                RenderBackendTimingQueryHeapDesc timingQueryHeapDesc(RenderBackendMaxNumTimingQueryRegions);
                frames[i].timingQueryHeap = renderBackend->CreateTimingQueryHeap(~0u, &timingQueryHeapDesc, "GPUProfiler");

                RenderBackendBufferDesc bufferDesc = RenderBackendBufferDesc::CreateReadback(sizeof(uint64) * 2 * RenderBackendMaxNumTimingQueryRegions);
                frames[i].timingQueryResults = renderBackend->CreateBuffer(~0u, &bufferDesc, nullptr, "TimingQueryResults");
            }

            initialized = true;
        }
        bool initialized = false;
        RenderBackend* renderBackend;
        struct Frame
        {
            uint32 numRegions = 0;
            Region regions[RenderBackendMaxNumTimingQueryRegions];
            RenderBackendBufferHandle timingQueryResults;
            RenderBackendTimingQueryHeapHandle timingQueryHeap;
        };
        Frame frames[3];
        uint32 currentBufferIndex = 0;
        double results[RenderBackendMaxNumTimingQueryRegions];
    };
}