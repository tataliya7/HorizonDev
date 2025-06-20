#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"

namespace Horizon
{
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
    };

    struct VirtualGeometryBuildSettings
    {

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
    };

    bool BuildVirtualGeometry(const VirtualGeometryBuildSettings& settings, const VirtualGeometryBuildInput& input, VirtualGeometryBuildOutput& output);
}