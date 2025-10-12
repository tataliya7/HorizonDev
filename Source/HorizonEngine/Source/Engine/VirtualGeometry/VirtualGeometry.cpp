#include "VirtualGeometry.h"
#include "GraphPartitioning.h"

#include <meshoptimizer.h>

namespace Horizon
{
    struct TriangleCluster
    {
        std::vector<uint32> indices;
        std::vector<uint32> materialIndices;
    };

    static Bounds3D VirtualGeometryComputeMeshletBounds(const std::vector<Vector3f>& vertices, const std::vector<uint32>& indices)
    {
        Bounds3D bounds;
        for (uint32 i = 0; i < indices.size(); i++)
        {
            if (i == 0)
            {
                bounds.minimum = vertices[indices[i]];
                bounds.maximum = vertices[indices[i]];
            }
            else
            {
                bounds.minimum.x = std::min(bounds.minimum.x, vertices[indices[i]].x);
                bounds.minimum.y = std::min(bounds.minimum.y, vertices[indices[i]].y);
                bounds.minimum.z = std::min(bounds.minimum.z, vertices[indices[i]].z);
                bounds.maximum.x = std::max(bounds.maximum.x, vertices[indices[i]].x);
                bounds.maximum.y = std::max(bounds.maximum.y, vertices[indices[i]].y);
                bounds.maximum.z = std::max(bounds.maximum.z, vertices[indices[i]].z);
            }
        }
        return bounds;
    }

    static std::vector<TriangleCluster> BuildTriangleClusters(
        const VirtualGeometryVertexArray& vertices,
        const std::vector<uint32>& indices,
        const std::vector<uint32>& materialIndices,
        uint32 minMeshletSize,
        uint32 maxMeshletSize)
    {
        uint32 triangleCount = static_cast<uint32>(indices.size()) / 3;

        std::vector<std::vector<int>> trilist(vertices.position.size());
        for (size_t i = 0; i < indices.size(); ++i)
        {
            trilist[indices[i]].push_back(int(i / 3));
        }

        GraphPartitioning::AdjacencyList graph;
        graph.nodeCount = triangleCount;
        graph.adjacencyOffsets.resize(triangleCount + 1);

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

        uint32 trianglesPerMeshlet = (minMeshletSize + maxMeshletSize) / 2;
        uint32 meshletCount = std::max(2u, Math::CeilDiv(graph.nodeCount, trianglesPerMeshlet));

        std::vector<TriangleCluster> meshlets(meshletCount);

        std::vector<int32> partitionIndices(graph.nodeCount);
        bool result = GraphPartitioning::PartitionGraph(graph, meshletCount, partitionIndices.data());
        if (result)
        {
            for (uint32 triangleIndex = 0; triangleIndex < graph.nodeCount; triangleIndex++)
            {
                meshlets[partitionIndices[triangleIndex]].indices.push_back(indices[triangleIndex * 3 + 0]);
                meshlets[partitionIndices[triangleIndex]].indices.push_back(indices[triangleIndex * 3 + 1]);
                meshlets[partitionIndices[triangleIndex]].indices.push_back(indices[triangleIndex * 3 + 2]);
                meshlets[partitionIndices[triangleIndex]].materialIndices.push_back(materialIndices[triangleIndex]);
            }

            for (uint32 meshletIndex = 0; meshletIndex < meshletCount; meshletIndex++)
            {
                // Need to split the cluster further...
                if ((3 * trianglesPerMeshlet) < static_cast<uint32>(meshlets[meshletIndex].indices.size()))
                {
                    std::vector<TriangleCluster> splits = BuildTriangleClusters(
                        vertices,
                        meshlets[meshletIndex].indices,
                        meshlets[meshletIndex].materialIndices,
                        minMeshletSize,
                        maxMeshletSize);

                    assert(splits.size() > 1);

                    meshlets[meshletIndex] = splits[0];
                    for (size_t j = 1; j < splits.size(); ++j)
                    {
                        meshlets.push_back(splits[j]);
                    }
                }
            }
        }

        return meshlets;
    }

    struct MeshletEdge
    {
        explicit MeshletEdge(uint32 a, uint32 b)
            : first(std::min(a, b))
            , second(std::max(a, b))
        {

        }

        struct Hash
        {
            std::size_t operator()(const MeshletEdge& edge) const
            {
                static_assert(std::is_same_v<std::size_t, uint64>);
                return (static_cast<uint64>(edge.second) << 32) | static_cast<uint64>(edge.first);
            }
        };

        bool operator==(const MeshletEdge& other) const
        {
            return first == other.first && second == other.second;
        }

        uint32 first;
        uint32 second;
    };

    bool VirtualGeometryBuildMesh(
        const VirtualGeometryBuildSettings& settings,
        const VirtualGeometryBuildInput& input,
        VirtualGeometryBuildOutput& output)
    {
        if (input.indices.empty())
        {
            return false;
        }

        // Only triangle meshes are supported.
        if (input.indices.size() % 3 != 0)
        {
            return false;
        }

        std::vector<TriangleCluster> triangleClusters = BuildTriangleClusters(
            input.vertices,
            input.indices,
            input.materialIndices,
            settings.minMeshletSize,
            settings.maxMeshletSize);

        uint32 totalTriangleCount = 0;

        std::vector<VirtualGeometryMeshlet> meshlets;
        for (uint32 meshletIndex = 0; meshletIndex < triangleClusters.size(); meshletIndex++)
        {
            assert(triangleClusters[meshletIndex].indices.size() % 3 == 0);
            uint32 meshletTriangleCount = static_cast<uint32>(triangleClusters[meshletIndex].indices.size()) / 3;

            Bounds3D meshletBounds = VirtualGeometryComputeMeshletBounds(input.vertices.position, triangleClusters[meshletIndex].indices);

            VirtualGeometryMeshlet meshlet =
            {
                .vertexOffset = 0,
                .triangleOffset = totalTriangleCount,
                .vertexCount = 0,
                .triangleCount = meshletTriangleCount,
                .boundingBoxCenter = meshletBounds.GetCenter(),
                .boundingBoxExtent = meshletBounds.GetExtent(),
            };
            meshlets.push_back(meshlet);

            totalTriangleCount += meshletTriangleCount;
        }

        for (uint32 lodIndex = 0; lodIndex < settings.maxLODCount; lodIndex++)
        {
            // Step 1: Group
            std::vector<VirtualGeometryMeshletGroup> meshletGroups;
            {
                auto GroupMeshlets = [&](const std::vector<VirtualGeometryMeshlet>& meshletsToGroup)
                {
                    VirtualGeometryMeshletGroup meshletGroup;
                    for (uint32 i = 0; i < meshletsToGroup.size(); ++i)
                    {
                        meshletGroup.meshletIndices.push_back(i);
                    }
                    return meshletGroup;
                };

                if (meshlets.size() < settings.minMeshletSize)
                {
                    VirtualGeometryMeshletGroup meshletGroup = GroupMeshlets(meshlets);
                    meshletGroups.push_back(meshletGroup);
                }
                else
                {
                    std::unordered_map<MeshletEdge, std::unordered_set<uint32>, MeshletEdge::Hash> edges2Meshlets;
                    std::unordered_map<uint32, std::unordered_set<MeshletEdge, MeshletEdge::Hash>> meshlets2Edges;

                    for (uint32 meshletIndex = 0; meshletIndex < meshlets.size(); meshletIndex++)
                    {
                        const VirtualGeometryMeshlet& meshlet = meshlets[meshletIndex];
                        for (uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount; triangleIndex++)
                        {
                            for (uint32 i = 0; i < 3; i++)
                            {
                                uint32 vertexIndex0 = input.indices[meshlet.triangleOffset + triangleIndex * 3 + i];
                                uint32 vertexIndex1 = input.indices[meshlet.triangleOffset + triangleIndex * 3 + (i + 1) % 3];
                                MeshletEdge edge(vertexIndex0, vertexIndex1);
                                if (edge.first != edge.second)
                                {
                                    edges2Meshlets[edge].insert(meshletIndex);
                                    meshlets2Edges[meshletIndex].insert(edge);
                                }
                            }
                        }
                    }

                    // Remove edges which are not connected to 2 different meshlets.
                    std::erase_if(edges2Meshlets, [&](const auto& pair) { return pair.second.size() <= 1; });

                    if (edges2Meshlets.empty())
                    {
                        VirtualGeometryMeshletGroup meshletGroup = GroupMeshlets(meshlets);
                        meshletGroups.push_back(meshletGroup);
                    }
                    else
                    {
                        GraphPartitioning::AdjacencyList graph;
                        graph.nodeCount = static_cast<uint32>(meshlets.size());
                        graph.adjacencyOffsets.reserve(graph.nodeCount + 1);

                        for (uint32 meshletIndex = 0; meshletIndex < meshlets.size(); meshletIndex++)
                        {
                            int32 adjacencyOffset = static_cast<int32>(graph.adjacencyIndices.size());

                            for (const auto& edge : meshlets2Edges[meshletIndex])
                            {
                                auto connectionsIter = edges2Meshlets.find(edge);
                                if (connectionsIter == edges2Meshlets.end())
                                {
                                    // This edge is not connected to any meshlets.
                                    continue;
                                }

                                const auto& connections = connectionsIter->second;
                                for (const auto& connectedMeshlet : connections)
                                {
                                    if (connectedMeshlet != meshletIndex)
                                    {
                                        auto existingEdgeIter = std::find(graph.adjacencyIndices.begin() + adjacencyOffset, graph.adjacencyIndices.end(), connectedMeshlet);
                                        if (existingEdgeIter == graph.adjacencyIndices.end())
                                        {
                                            assert(graph.adjacencyIndices.size() == graph.adjacencyWeights.size());
                                            graph.adjacencyIndices.push_back(static_cast<int32>(connectedMeshlet));
                                            graph.adjacencyWeights.push_back(1);
                                        }
                                        else // More than one meshlet is connected to this edge.
                                        {
                                            std::ptrdiff_t ptrdiff = existingEdgeIter - graph.adjacencyIndices.begin();
                                            assert(ptrdiff >= 0);
                                            graph.adjacencyWeights[ptrdiff]++;
                                        }
                                    }
                                }
                            }
                            graph.adjacencyOffsets.push_back(adjacencyOffset);
                        }

                        graph.adjacencyOffsets.push_back(static_cast<int32>(graph.adjacencyIndices.size()));

                        uint32 meshletCountPerGroup = (settings.minMeshletGroupSize + settings.maxMeshletGroupSize) / 2;
                        uint32 meshletGroupCount = std::max(2u, Math::CeilDiv(graph.nodeCount, meshletCountPerGroup));

                        std::vector<int32> partitionIndices(graph.nodeCount);
                        bool result = GraphPartitioning::PartitionGraph(graph, meshletGroupCount, partitionIndices.data());
                        assert(result == true);

                        meshletGroups.resize(meshletGroupCount);
                        for(uint32 meshletIndex = 0; meshletIndex < meshlets.size(); meshletIndex++)
                        {
                            int32 meshletGroupIndex = partitionIndices[meshletIndex];
                            meshletGroups[meshletGroupIndex].meshletIndices.push_back(meshletIndex);
                        }
                    }
                }
            }

            for (const VirtualGeometryMeshletGroup& meshletGroup : meshletGroups)
            {
                // Step 2: Merge
                std::vector<uint32> mergedVertexIndices;
                {
                    for (uint32 meshletIndex : meshletGroup.meshletIndices)
                    {
                        const VirtualGeometryMeshlet& meshlet = meshlets[meshletIndex];
                        for (uint32 triangleIndex = 0; triangleIndex < meshlet.triangleCount; triangleIndex++)
                        {
                            for (uint32 i = 0; i < 3; i++)
                            {
                                uint32 vertexIndex = input.indices[meshlet.triangleOffset + triangleIndex * 3 + i];
                                mergedVertexIndices.push_back(vertexIndex);
                            }
                        }
                    }
                }

                // Step 3: Simplify
                std::vector<uint32> simplifiedVertexIndices(mergedVertexIndices.size());
                {
                    float targetError = 0.0f;
                    float simplificationError = 0.0f;
                    uint32 targetIndexCount = static_cast<uint32>(mergedVertexIndices.size()) / 2;
                    uint32 options = meshopt_SimplifyLockBorder | meshopt_SimplifySparse | meshopt_SimplifyErrorAbsolute;

                    constexpr uint32 VirtualGeometryVertexAttributeCount = 9;
                    constexpr float AttributeWeights[VirtualGeometryVertexAttributeCount] =
                    {
                        0.05f, 0.05f, // uv
                        0.5f, 0.5f, 0.5f, // normal
                        0.001f, 0.001f, 0.001f, 0.05f // tangent, .w is sign, weight bigger.
                    };

                    // https://github.com/zeux/meshoptimizer/issues/149
                    uint64 simplifiedVertexIndexCount = meshopt_simplify(
                        simplifiedVertexIndices.data(),
                        mergedVertexIndices.data(),
                        mergedVertexIndices.size(),
                        &input.vertices.position[0].x,
                        input.vertices.position.size(),
                        sizeof(Vector3f),
                        targetIndexCount,
                        targetError,
                        options,
                        &simplificationError);

                    simplifiedVertexIndices.resize(simplifiedVertexIndexCount);
                }

                // Step 4: Split
                {
                    // std::vector<TriangleCluster> simplifiedTriangleClusters = BuildTriangleClusters(
                    //     input.vertices,
                    //     simplifiedVertexIndices,
                    //     simplifiedMaterialIndices,
                    //     settings.minMeshletSize,
                    //     settings.maxMeshletSize);
                }
            }
        }

        output.indices.clear();
        output.materialIndices.clear();
        output.meshlets.clear();

        for (uint32 meshletIndex = 0; meshletIndex < meshlets.size(); meshletIndex++)
        {
            for (uint32 k = 0; k < triangleClusters[meshletIndex].indices.size(); k++)
            {
                output.indices.push_back(triangleClusters[meshletIndex].indices[k]);
            }
            for (uint32 k = 0; k < triangleClusters[meshletIndex].materialIndices.size(); k++)
            {
                output.materialIndices.push_back(triangleClusters[meshletIndex].materialIndices[k]);
            }
            assert(output.indices.size() == output.materialIndices.size() * 3);

            output.meshlets.push_back(meshlets[meshletIndex]);
        }

        return true;
    }
}