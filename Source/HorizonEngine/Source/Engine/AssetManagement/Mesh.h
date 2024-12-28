#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"
//#include "Physics/PhysicsModule.h"
#include "Engine/EntityComponentSystem/EntityManager.h"
#include "Engine/Serialization/SerializationModule.h"

namespace Horizon
{
//     class TriangleMeshRenderData
//     {
//     public:
//
//         VertexBuffer vertexBuffer0;
//         VertexBuffer vertexBuffer1;
//         VertexBuffer vertexBuffer2;
//         VertexBuffer vertexBuffer3;
//
//         IndexBuffer indexBuffer;
//         /** Geometry for ray tracing. */
//         FRayTracingGeometry RayTracingGeometry;
//     private:
//
//     };
//
#if 0
     /**
      * A mesh is a type of geometry that consists of a set of triangles.
      */
     class Mesh
     {
     public:

         uint32 GetVertexCount() const;

         uint32 GetTriangleCount() const;

         AABB GetBounds() const;

         bool IsValidMaterialIndex(uint32 materialIndex) const;

         void SetMaterial(uint32 materialIndex, Material* material);

         Material* GetMaterial(uint32 materialIndex) const;

     private:

         bool keepCPUData;

         TriangleMeshRenderData* renderData;

         std::vector<Material> materials;
     };
#endif
}