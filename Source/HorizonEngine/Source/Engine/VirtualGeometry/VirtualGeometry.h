#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

namespace Horizon
{
    struct VirtualGeometryVertexLayoutDescription
    {
        bool useNormals;
        bool useTangents;
        bool useColors;
        uint32 textureCoordinateCount;
        uint32 relevantJointCount;
    };

    struct VirtualGeometryVertexArray
    {
        std::vector<Vector3f> position;
        std::vector<Vector3f> normals;
        std::vector<Vector4f> tangents;
        std::vector<Vector2f> textureCoordinates[1];
    };

    struct VirtualGeometryMeshlet
    {
        uint32 vertexOffset;
        uint32 triangleOffset;
        uint32 vertexCount;
        uint32 triangleCount;
        Vector3f boundingBoxCenter;
        Vector3f boundingBoxExtent;
        uint32 meshletGroupIndex;
        float error;
    };

    struct VirtualGeometryMeshletGroup
    {
        uint32 levelIndex;
        uint32 meshletOffset;
        uint32 meshletCount;
        float error;
        float parentError;
        std::vector<uint32> meshletIndices;
    };

    struct VirtualGeometryHierarchyNode
    {
        // Not implemented.
    };

    struct VirtualGeometryBuildSettings
    {
        uint32 maxLevelCount;
        uint32 minMeshletSize;
        uint32 maxMeshletSize;
        uint32 minMeshletGroupSize;
        uint32 maxMeshletGroupSize;
    };

    struct VirtualGeometryBuildInput
    {
        VirtualGeometryVertexArray vertices;
        std::vector<uint32> indices;
        std::vector<uint32> materialIndices;
    };

    struct VirtualGeometryBuildOutput
    {
        VirtualGeometryVertexArray vertices;
        std::vector<uint32> indices;
        std::vector<uint32> materialIndices;
        std::vector<VirtualGeometryMeshlet> meshlets;
        std::vector<VirtualGeometryMeshletGroup> meshletGroups;
    };

    bool VirtualGeometryBuildMesh(const VirtualGeometryBuildSettings& settings, const VirtualGeometryBuildInput& input, VirtualGeometryBuildOutput& output);
}