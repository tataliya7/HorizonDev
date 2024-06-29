 #include "MeshComponent.h"

 namespace Horizon
 {
     uint32 MeshComponent::GetMaterialCount() const
     {
         if (true)
         {
             return (uint32)materials.size();
         }
         return 0;
     }
 }