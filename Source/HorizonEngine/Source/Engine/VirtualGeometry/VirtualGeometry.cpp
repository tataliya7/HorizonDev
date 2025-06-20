#include "VirtualGeometry.h"

#include <metis.h>

namespace Horizon
{
    const size_t kClusterSize = 128;
    const size_t kGroupSize = 8;
    const bool kUseLocks = true;
    const bool kUseNormals = true;
    const bool kUseRetry = true;
    const bool kUseSpatial = false;
    const int kMetisSlop = 2;
    const float kSimplifyThreshold = 0.85f;

    struct ClusterIndices
    {
        std::vector<uint32> indices;
        std::vector<uint32> materialIndices;
        std::vector<Vector3f> normals;
        std::vector<Vector2f> uvs;
    };

    std::vector<ClusterIndices> BuildTriangleClusters(
        const VirtualGeometryVertexArray& vertices,
        const std::vector<uint32>& indices,
        const std::vector<uint32>& materialIndices,
        const std::vector<Vector3f>& normals,
        const std::vector<Vector2f>& uvs)
    {
        uint32 triangleCount = uint32(indices.size()) / 3;

        std::vector<std::vector<int>> trilist(vertices.position.size());
        for (size_t i = 0; i < indices.size(); ++i)
        {
            trilist[indices[i]].push_back(int(i / 3));
        }

        std::vector<idx_t> xadj(indices.size() / 3 + 1);
        std::vector<idx_t> adjncy;
        std::vector<idx_t> adjwgt;
        std::vector<idx_t> part(indices.size() / 3);

        std::vector<int> scratch;

        for (size_t i = 0; i < indices.size() / 3; i++)
        {
            uint32 a = indices[i * 3 + 0];
            uint32 b = indices[i * 3 + 1];
            uint32 c = indices[i * 3 + 2];

            scratch.clear();
            scratch.insert(scratch.end(), trilist[a].begin(), trilist[a].end());
            scratch.insert(scratch.end(), trilist[b].begin(), trilist[b].end());
            scratch.insert(scratch.end(), trilist[c].begin(), trilist[c].end());
            std::sort(scratch.begin(), scratch.end());

            for (size_t j = 0; j < scratch.size(); ++j)
            {
                if (scratch[j] == int(i))
                    continue;

                if (j == 0 || scratch[j] != scratch[j - 1])
                {
                    adjncy.push_back(scratch[j]);
                    adjwgt.push_back(1);
                }
                else if (j != 0)
                {
                    assert(scratch[j] == scratch[j - 1]);
                    adjwgt.back()++;
                }
            }

            xadj[i + 1] = int(adjncy.size());
        }

        idx_t nvtxs = idx_t(indices.size() / 3);
        idx_t ncon = 1;
        idx_t nparts = idx_t(indices.size() / 3 + (kClusterSize - kMetisSlop) - 1) / (kClusterSize - kMetisSlop);
        idx_t objval = 0;

        idx_t metisOptions[METIS_NOPTIONS];
        METIS_SetDefaultOptions(metisOptions);
        metisOptions[METIS_OPTION_UFACTOR] = 1;

        int result = METIS_PartGraphRecursive(
            &nvtxs,           // The number of vertices in the graph.
            &ncon,            // The number of balancing constraints.
            xadj.data(),      // The adjacency structure of the graph.
            adjncy.data(),    // The adjacency structure of the graph.
            NULL,             // The weights of the vertices.
            NULL,             // The size of the vertices for computing the total communication volume.
            adjwgt.data(),    // The weights of the edges.
            &nparts,          // The number of parts to partition the graph.
            NULL,             // This is an array of size nparts*ncon that specifies the desired weight for each partition and constraint.
            NULL,             // This is an array of size ncon that specifies the allowed load imbalance tolerance for each constraint.
            metisOptions,     // This is an array of options.
            &objval,          // Upon successful completion, this variable stores the edge-cut or the total communication volume of the partitioning solution.
            part.data());     // This is a vector of size nvtxs that upon successful completion stores the partition vector of the graph.

        if (result != METIS_OK)
        {
            // @todo log
            //return false;
        }

        std::vector<ClusterIndices> meshlets(nparts);

        for (uint32 i = 0; i < uint32(nvtxs); ++i)
        {
            meshlets[part[i]].indices.push_back(indices[i * 3 + 0]);
            meshlets[part[i]].indices.push_back(indices[i * 3 + 1]);
            meshlets[part[i]].indices.push_back(indices[i * 3 + 2]);
            meshlets[part[i]].materialIndices.push_back(materialIndices[i]);

            meshlets[part[i]].normals.push_back(normals[i * 3 + 0]);
            meshlets[part[i]].normals.push_back(normals[i * 3 + 1]);
            meshlets[part[i]].normals.push_back(normals[i * 3 + 2]);
            meshlets[part[i]].uvs.push_back(uvs[i * 3 + 0]);
            meshlets[part[i]].uvs.push_back(uvs[i * 3 + 1]);
            meshlets[part[i]].uvs.push_back(uvs[i * 3 + 2]);
        }

        for (int i = 0; i < nparts; ++i)
        {
            //meshlets[i].parent.error = FLT_MAX;

            // need to split the cluster further...
            if (meshlets[i].indices.size() > kClusterSize * 3)
            {
                std::vector<ClusterIndices> splits = BuildTriangleClusters(
                    vertices,
                    meshlets[i].indices,
                    meshlets[i].materialIndices,
                    meshlets[i].normals,
                    meshlets[i].uvs);

                assert(splits.size() > 1);

                meshlets[i] = splits[0];
                for (size_t j = 1; j < splits.size(); ++j)
                {
                    meshlets.push_back(splits[j]);
                }
            }
        }

        return meshlets;
    }

    bool BuildVirtualGeometry(
        const VirtualGeometryBuildSettings& settings,
        const VirtualGeometryBuildInput& input,
        VirtualGeometryBuildOutput& output)
    {
        if (input.indices.empty())
        {
            return false;
        }

        auto clusters = BuildTriangleClusters(
            input.vertices,
            input.indices,
            input.materialIndices,
            input.vertices.normals,
            input.vertices.textureCoordinates[0]);

        output.indices.clear();
        output.materialIndices.clear();
        output.meshlets.clear();

        uint32 triangleCount = 0;
        for (uint32 i = 0; i < uint32(clusters.size()); i++)
        {
            VirtualGeometryMeshlet meshlet =
            {
                .triangleOffset = triangleCount,
                .triangleCount = uint32(clusters[i].indices.size()) / 3
            };

            for (uint32 j = 0; j < uint32(clusters[i].indices.size()); j++)
            {
                output.indices.push_back(clusters[i].indices[j]);
            }
            for (uint32 j = 0; j < uint32(clusters[i].materialIndices.size()); j++)
            {
                output.materialIndices.push_back(clusters[i].materialIndices[j]);
            }
            assert(output.indices.size() == output.materialIndices.size() * 3);

            for (uint32 j = 0; j < uint32(clusters[i].normals.size()); j++)
            {
                output.vertices.normals.push_back(clusters[i].normals[j]);
            }

            for (uint32 j = 0; j < uint32(clusters[i].uvs.size()); j++)
            {
                output.vertices.textureCoordinates[0].push_back(clusters[i].uvs[j]);
            }

            output.meshlets.push_back(meshlet);

            triangleCount += uint32(clusters[i].indices.size()) / 3;
        }

        return true;
    }
}