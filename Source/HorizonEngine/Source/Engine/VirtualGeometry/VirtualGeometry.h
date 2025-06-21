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
        std::vector<GPUSceneMeshletData> meshlets;
    };

    bool BuildVirtualGeometry(const VirtualGeometryBuildSettings& settings, const VirtualGeometryBuildInput& input, VirtualGeometryBuildOutput& output);
}