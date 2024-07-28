 #include "MeshComponent.h"

 namespace Horizon
 {
     MeshComponent::MeshComponent()
     {

     }

     MeshComponent::~MeshComponent()
     {

     }

     bool MeshComponent::IsRenderObjectValid() const
     {
         return renderObject != nullptr;
     }

     void MeshComponent::CreateRenderObject(RenderScene* scene)
     {
         assert(renderObject == nullptr);
         if (renderObject == nullptr)
         {
             renderObject = new MeshRenderObject();

             renderObject->vertexCount = vertexCount;
             renderObject->indexCount = indexCount;

             renderObject->vertexBuffers[0] = vertexBuffers[0];
             renderObject->vertexBuffers[1] = vertexBuffers[1];
             renderObject->vertexBuffers[2] = vertexBuffers[2];
             renderObject->vertexBuffers[3] = vertexBuffers[3];

             renderObject->indexBuffer = indexBuffer;

             renderObject->materialBuffer = materialBuffer;
             renderObject->materialIndexBuffer = materialIndexBuffer;

             scene->AddMesh(renderObject);
         }
     }

     void MeshComponent::DestroyRenderObject(RenderScene* scene)
     {

     }

     void MeshComponent::UpdateRenderObject()
     {
         if (renderObject)
         {
             //renderObject->transform = ;
         }
     }

     uint32 MeshComponent::GetMaterialCount() const
     {
         if (true)
         {
             return (uint32)materials.size();
         }
         return 0;
     }
 }