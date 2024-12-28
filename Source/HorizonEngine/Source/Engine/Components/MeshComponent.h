 #pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"
#include "Engine/Serialization/Archive.h"

 namespace Horizon
 {
     /**
      * MeshComponent is used to create an instance of a renderable collection of triangles.
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
         std::vector<uint32> indices;
         std::vector<uint32> boneIndices;
         std::vector<float> boneWeights;

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

         std::vector<MaterialShaderParameters> materialData;
         std::vector<uint32> materialIndices;

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

         RenderBackendBufferHandle vertexBuffers[4];
         RenderBackendBufferHandle indexBuffer;

         RenderBackendBufferHandle materialBuffer;
         RenderBackendBufferHandle materialIndexBuffer;

         int previousTransformIndex = -1;

         int materialBufferOffset = 0;

         //RenderBackendBufferHandle transformBuffer;
         //RenderBackendBufferHandle transformTransposeBuffer;
         //RenderBackendBufferHandle previousTransformBuffer;

         //RenderBackendBufferHandle boneIndexBuffer;
         //RenderBackendBufferHandle boneWeightBuffer;
         //RenderBackendBufferHandle boneTransformBuffer;

         //RayTracingGeometry rayTracingGeometry;

         int64 updateCounter = -100;

         bool doubleSided = true;

         //bool IsSkinnedMesh() const
         //{
         //    return armature != EntityHandle::Null;
         //}

     private:

         //TriangleMesh* mesh;

         MeshRenderObject* renderObject = nullptr;
     };
 }