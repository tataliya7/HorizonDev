#include "RenderBackendGPUProfiler.h"

namespace HE
{
    RenderBackendGPUProfiler::RenderBackendGPUProfiler(RenderBackend* renderBackend)
        : renderBackend(renderBackend)
    {

    }

    RenderBackendGPUProfiler::~RenderBackendGPUProfiler()
    {

    }

    void RenderBackendGPUProfiler::BeginFrame(RenderBackendCommandList* commandList)
    {
        if (!initialized)
        {
            Init();
        }

        frames[currentBufferIndex].numRegions = 0;
        for (auto& region : frames[currentBufferIndex].regions)
        {
            region.used = false;
        }

        const float timestampPeriod = 1.0;
        double millisecondsPerTick = 1e-6f * timestampPeriod;

        uint32 index = (currentBufferIndex + 1) % 3;
        void* data = nullptr;
        auto buffer = frames[index].timingQueryResults;
        renderBackend->MapBuffer(buffer, &data);
        for (uint32 i = 0; i < frames[index].numRegions; i++)
        {
            results[i] = millisecondsPerTick * (double)(((uint64*)data)[2 * i + 1] - ((uint64*)data)[2 * i + 0]);
        }
        renderBackend->UnmapBuffer(buffer);
    }

    void RenderBackendGPUProfiler::EndFrame(RenderBackendCommandList* commandList)
    {
        auto& timingQueryHeap = frames[currentBufferIndex].timingQueryHeap;
        auto& timingQueryResults = frames[currentBufferIndex].timingQueryResults;
        auto& numRegions = frames[currentBufferIndex].numRegions;

        commandList->ResolveTimingQueryResults(timingQueryHeap, 0, numRegions, timingQueryResults, 0);

        currentBufferIndex = (currentBufferIndex + 1) % 3;
    }

    uint32 RenderBackendGPUProfiler::BeginRegion(RenderBackendCommandList* commandList, const char* name)
    {
        auto& timingQueryHeap = frames[currentBufferIndex].timingQueryHeap;
        auto& regions = frames[currentBufferIndex].regions;
        auto& numRegions = frames[currentBufferIndex].numRegions;

        uint32 regionID = numRegions;
        assert(!regions[regionID].used && regionID < RenderBackendMaxNumTimingQueryRegions);
        regions[regionID].used = true;
        regions[regionID].name = name;
        regions[regionID].commandList = commandList;
        regions[regionID].commandList->BeginTimingQuery(timingQueryHeap, regionID);

        numRegions++;

        return regionID;
    }

    void RenderBackendGPUProfiler::EndRegion(uint32 regionID, RenderBackendCommandList* commandList)
    {
        auto& timingQueryHeap = frames[currentBufferIndex].timingQueryHeap;
        auto& regions = frames[currentBufferIndex].regions;

        assert(regions[regionID].used && regionID < RenderBackendMaxNumTimingQueryRegions);
        if (commandList)
        {
            commandList->EndTimingQuery(timingQueryHeap, regionID);
        }
        else
        {
            regions[regionID].commandList->EndTimingQuery(timingQueryHeap, regionID);
        }
    }
}