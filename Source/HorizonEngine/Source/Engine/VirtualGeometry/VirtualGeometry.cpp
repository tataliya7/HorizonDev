#include "VirtualGeometry.h"
#include "GraphPartitioning.h"

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
        const std::vector<uint32>& materialIndices)
    {
        uint32 triangleCount = uint32(indices.size()) / 3;

        std::vector<std::vector<int>> trilist(vertices.position.size());
        for (size_t i = 0; i < indices.size(); ++i)
        {
            trilist[indices[i]].push_back(int(i / 3));
        }

        GraphPartitioning::AdjacencyList graph;
        graph.nodeCount = triangleCount;
        graph.adjacencyOffsets.resize(triangleCount + 1);
        std::vector<int32> part;

        std::vector<int> scratch;

        for (size_t i = 0; i < triangleCount; i++)
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
                    graph.adjacencyIndices.push_back(scratch[j]);
                    graph.adjacencyWeights.push_back(1);
                }
                else if (j != 0)
                {
                    assert(scratch[j] == scratch[j - 1]);
                    graph.adjacencyWeights.back()++;
                }
            }

            graph.adjacencyOffsets[i + 1] = int(graph.adjacencyIndices.size());
        }

        bool result = GraphPartitioning::PartGraph(graph, kClusterSize, kClusterSize, part);

        uint32 nparts = static_cast<uint32>(part.size());

        if (!result)
        {

        }

        std::vector<ClusterIndices> meshlets(nparts);

        for (uint32 i = 0; i < uint32(graph.nodeCount); ++i)
        {
            meshlets[part[i]].indices.push_back(indices[i * 3 + 0]);
            meshlets[part[i]].indices.push_back(indices[i * 3 + 1]);
            meshlets[part[i]].indices.push_back(indices[i * 3 + 2]);
            meshlets[part[i]].materialIndices.push_back(materialIndices[i]);
        }

        for (uint32 i = 0; i < nparts; ++i)
        {
            //meshlets[i].parent.error = FLT_MAX;

            // need to split the cluster further...
            if (meshlets[i].indices.size() > kClusterSize * 3)
            {
                std::vector<ClusterIndices> splits = BuildTriangleClusters(
                    vertices,
                    meshlets[i].indices,
                    meshlets[i].materialIndices);

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
            input.materialIndices);

        output.indices.clear();
        output.materialIndices.clear();
        output.meshlets.clear();

        uint32 triangleCount = 0;
        for (uint32 i = 0; i < uint32(clusters.size()); i++)
        {
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

            Bounds3D bounds;
            for (uint32 j = 0; j < uint32(clusters[i].indices.size()); j++)
            {
                if (j == 0)
                {
                    bounds.minimum = input.vertices.position[clusters[i].indices[j]];
                    bounds.maximum = input.vertices.position[clusters[i].indices[j]];
                }
                else
                {
                    bounds.minimum.x = std::min(bounds.minimum.x, input.vertices.position[clusters[i].indices[j]].x);
                    bounds.minimum.y = std::min(bounds.minimum.y, input.vertices.position[clusters[i].indices[j]].y);
                    bounds.minimum.z = std::min(bounds.minimum.z, input.vertices.position[clusters[i].indices[j]].z);
                    bounds.maximum.x = std::max(bounds.maximum.x, input.vertices.position[clusters[i].indices[j]].x);
                    bounds.maximum.y = std::max(bounds.maximum.y, input.vertices.position[clusters[i].indices[j]].y);
                    bounds.maximum.z = std::max(bounds.maximum.z, input.vertices.position[clusters[i].indices[j]].z);
                }
            }

            GPUSceneMeshletData meshlet =
            {
                .vertexOffset = 0,
                .triangleOffset = triangleCount,
                .vertexCount =  0,
                .triangleCount = uint32(clusters[i].indices.size()) / 3,
                .boundingBoxCenter = bounds.GetCenter(),
                .padding0 = 0,
                .boundingBoxExtent = bounds.GetExtent(),
                .isSkinned = 0
            };

            output.meshlets.push_back(meshlet);

            triangleCount += uint32(clusters[i].indices.size()) / 3;
        }

        return true;
    }
}