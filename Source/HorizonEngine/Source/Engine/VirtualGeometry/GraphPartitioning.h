#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

namespace Horizon::GraphPartitioning
{
    struct AdjacencyList
    {
        uint32 nodeCount;
        uint32 adjacencyCount;
        std::vector<int32> adjacencyOffsets;
        std::vector<int32> adjacencyIndices;
        std::vector<int32> adjacencyWeights;
    };

    bool PartGraph(AdjacencyList& graph, uint32 minPartSize, uint32 maxPartSize, std::vector<int32>& partitions);
}