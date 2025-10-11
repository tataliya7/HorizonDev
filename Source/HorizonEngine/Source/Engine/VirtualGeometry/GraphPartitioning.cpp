#include "GraphPartitioning.h"

#include <metis.h>

namespace Horizon::GraphPartitioning
{
    bool PartGraph(AdjacencyList& graph, uint32 minPartSize, uint32 maxPartSize, std::vector<int32>& partitions)
    {
        uint32 nodeCount = graph.nodeCount;
        uint32 nodeCountPerPartition = (minPartSize + maxPartSize) / 2;
        uint32 desiredPartitionCount = std::max(2u, Math::CeilDiv(nodeCount, nodeCountPerPartition));

        idx_t nvtxs = static_cast<idx_t>(nodeCount);
        idx_t ncon = 1;
        idx_t nparts = static_cast<idx_t>(desiredPartitionCount);
        idx_t objval = 0;

        std::vector<idx_t>& xadj = graph.adjacencyOffsets;
        std::vector<idx_t>& adjncy = graph.adjacencyIndices;
        std::vector<idx_t>& adjwgt = graph.adjacencyWeights;

        idx_t options[METIS_NOPTIONS];
        METIS_SetDefaultOptions(options);
        options[METIS_OPTION_UFACTOR] = 1; // What does this mean?

        std::vector<idx_t> part(nvtxs);

        int result = METIS_PartGraphRecursive(
            &nvtxs,           // The number of vertices in the graph.
            &ncon,            // The number of balancing constraints.
            xadj.data(),      // The adjacency structure of the graph.
            adjncy.data(),    // The adjacency structure of the graph.
            nullptr,          // The weights of the vertices.
            nullptr,          // The size of the vertices for computing the total communication volume.
            adjwgt.data(),    // The weights of the edges.
            &nparts,          // The number of parts to partition the graph.
            nullptr,          // This is an array of size nparts*ncon that specifies the desired weight for each partition and constraint.
            nullptr,          // This is an array of size ncon that specifies the allowed load imbalance tolerance for each constraint.
            options,          // This is an array of options.
            &objval,          // Upon successful completion, this variable stores the edge-cut or the total communication volume of the partitioning solution.
            part.data());     // This is a vector of size nvtxs that upon successful completion stores the partition vector of the graph.

        if (result == METIS_OK)
        {
            partitions.resize(nvtxs);
            for (uint32 i = 0; i < static_cast<uint32>(partitions.size()); i++)
            {
                partitions[i] = part[i];
            }

            return true;
        }

        return false;
    }
}