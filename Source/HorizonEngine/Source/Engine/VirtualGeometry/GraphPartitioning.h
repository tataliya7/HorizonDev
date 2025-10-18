#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

namespace Horizon::GraphPartitioning
{
    struct AdjacencyList
    {
        uint32 nodeCount;
        std::vector<int32> adjacencyOffsets;
        std::vector<int32> adjacencyIndices;
        std::vector<int32> adjacencyWeights;
    };

    bool PartitionGraph(AdjacencyList& graph, uint32 partitionCount, int32* partitionIndices);

    bool PartitionGraphRecursive(AdjacencyList& graph, uint32 partitionCount, int32* partitionIndices);
}