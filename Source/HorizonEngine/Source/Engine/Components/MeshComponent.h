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
         std::string meshSource;

         uint32 numVertices;
         uint32 numIndices;

         std::vector<Vector3> positions;
         std::vector<Vector3> normals;
         std::vector<Vector4> tangents;
         std::vector<Vector2> texCoords;
         std::vector<uint32> indices;
         std::vector<uint32> materialIndices;
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
             Vector3 boundsMin;
             Vector3 boundsMax;
         };
         std::vector<MeshSubset> subsets;
         //std::vector<Matrix4x4> transformData;
         //std::vector<Matrix4x4> transformDataTranspose;

         std::vector<Material> materials;

         //Mesh* GetMesh() const
         //{
         //    return mesh;
         //}

         uint32 GetSubsetCount() const
         {
             return (uint32)subsets.size();
         }

         uint32 GetMaterialCount() const;

         Vector3 boundsMin;
         Vector3 boundsMax;

         //EntityHandle armature = EntityHandle::Null;

         RenderBackendBufferHandle vertexBuffers[4];
         RenderBackendBufferHandle indexBuffer;
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

         MeshRenderObject* renderObject;
     };
 }