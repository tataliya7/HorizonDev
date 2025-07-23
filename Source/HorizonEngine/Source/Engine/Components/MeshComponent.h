#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"
#include "Engine/Serialization/Archive.h"

namespace Horizon
{
    class Skeleton;

    /**
     * MeshComponent is used to create an instance of a renderable collection consist of triangles.
     */
    class MeshComponent
    {
    public:

        MeshComponent();
        ~MeshComponent();
        bool IsRenderObjectValid() const;
        void CreateRenderObject(RenderScene* scene);
        void DestroyRenderObject(RenderScene* scene);
        void UpdateRenderObject();

        std::string meshSource;

        uint32 vertexCount;
        uint32 indexCount;

        std::vector<Vector3f> positions;
        std::vector<Vector3f> normals;
        std::vector<Vector4f> tangents;
        std::vector<Vector2f> texCoords;
        std::vector<VirtualGeometryVertex> vertices;
        std::vector<uint32> indices;
        std::vector<int32> jointIndices;
        std::vector<float> jointWeights;
        std::vector<Matrix4x4f> jointTransforms;
        std::vector<GPUSceneMeshletData> meshlets;

        struct MeshSubset
        {
            std::string name;
            uint32 baseVertex;
            uint32 baseIndex;
            uint32 numIndices;
            uint32 numVertices;
            //uint32 materialIndex;
            //uint32 transformIndex;
            Vector3f boundsMin;
            Vector3f boundsMax;
        };
        std::vector<MeshSubset> subsets;
        //std::vector<Matrix4x4f> transformData;
        //std::vector<Matrix4x4f> transformDataTranspose;
        Matrix4x4f localToWorldMatrix;

        std::vector<Material> materials;
        std::vector<uint32> materialIndices;

        std::vector<MaterialShaderParameters> materialData;

        //Mesh* GetMesh() const
        //{
        //    return mesh;
        //}

        uint32 GetSubsetCount() const
        {
            return (uint32)subsets.size();
        }

        uint32 GetMaterialCount() const;

        Vector3f boundsMin;
        Vector3f boundsMax;

        //EntityHandle armature = EntityHandle::Null;

        int previousTransformIndex = -1;

        int materialBufferOffset = 0;

        //RayTracingGeometry rayTracingGeometry;

        int64 updateCounter = -100;

        bool doubleSided = true;

        //bool IsSkinnedMesh() const
        //{
        //    return armature != EntityHandle::Null;
        //}

        Skeleton* skeleton = nullptr;

    private:

        //TriangleMesh* mesh;

        MeshRenderObject* renderObject = nullptr;
    };
}