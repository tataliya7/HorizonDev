// #include "Entity.h"
//
// #include <assert.h>
//
// namespace ECS
// {
//
//     {
//         check(&NewArchetype != this);
//
//         const int32 AbsoluteIndex = EntityMap.FindAndRemoveChecked(Entity.Index);
//         const int32 ChunkIndex = AbsoluteIndex / NumEntitiesPerChunk;
//         const int32 IndexWithinChunk = AbsoluteIndex % NumEntitiesPerChunk;
//         FMassArchetypeChunk& Chunk = Chunks[ChunkIndex];
//
//         const int32 NewAbsoluteIndex = NewArchetype.AddEntityInternal(Entity, Chunk.GetSharedFragmentValues());
//         const int32 NewChunkIndex = NewAbsoluteIndex / NewArchetype.NumEntitiesPerChunk;
//         const int32 NewIndexWithinChunk = NewAbsoluteIndex % NewArchetype.NumEntitiesPerChunk;
//         FMassArchetypeChunk& NewChunk = NewArchetype.Chunks[NewChunkIndex];
//
//         MoveFragmentsToAnotherArchetypeInternal(NewArchetype, { NewChunk.GetRawMemory(), NewIndexWithinChunk }, { Chunk.GetRawMemory(), IndexWithinChunk }, /*Count=*/1);
//
//         RemoveEntityInternal(AbsoluteIndex);
//     }
//
//     void FMassArchetypeData::RemoveEntity(FMassEntityHandle Entity)
//     {
//         const int32 AbsoluteIndex = EntityMap.FindAndRemoveChecked(Entity.Index);
//         const int32 chunkIndex = AbsoluteIndex / NumEntitiesPerChunk;
//         const uint32_t indexWithinChunk = AbsoluteIndex % NumEntitiesPerChunk;
//         ArchetypeChunk& chunk = chunks[chunkIndex];
//
//         assert(chunk.entityCount >= 1);
//
//         for (const FMassArchetypeFragmentConfig& FragmentConfig : FragmentConfigs)
//         {
//             // Destroy the fragment data
//             void* DyingFragmentPtr = FragmentConfig.GetFragmentData(Chunk.GetRawMemory(), IndexWithinChunk);
//             FragmentConfig.FragmentType->DestroyStruct(DyingFragmentPtr);
//         }
//
//         const int32 ChunkIndex = AbsoluteIndex / NumEntitiesPerChunk;
//         const int32 IndexWithinChunk = AbsoluteIndex % NumEntitiesPerChunk;
//         FMassArchetypeChunk& Chunk = Chunks[ChunkIndex];
//
//         if (indexWithinChunk != chunk.entityCount - 1)
//         {
//             for (const FMassArchetypeFragmentConfig& FragmentConfig : FragmentConfigs)
//             {
//                 void* dyingFragmentPtr = FragmentConfig.GetFragmentData(Chunk.GetRawMemory(), IndexWithinChunk);
//                 void* MovingFragmentPtr = FragmentConfig.GetFragmentData(Chunk.GetRawMemory(), IndexToSwapFrom);
//
//                 memcpy(dyingFragmentPtr, MovingFragmentPtr, FragmentConfig.FragmentType->GetStructureSize());
//             }
//
//             // Update the entity table and map
//             const FMassEntityHandle EntityBeingSwapped = Chunk.GetEntityArrayElementRef(EntityListOffsetWithinChunk, IndexToSwapFrom);
//             Chunk.GetEntityArrayElementRef(EntityListOffsetWithinChunk, IndexWithinChunk) = EntityBeingSwapped;
//             EntityMap.FindChecked(EntityBeingSwapped.Index) = AbsoluteIndex;
//         }
//
//         chunk.entityCount = chunk.entityCount - 1;
//     }
//
//     void EntityManager::DestroyEntity(EntityHandle entityHandle)
//     {
//         assert(IsEntityValid(entityHandle));
//
//         EntityData& entity = entities[entityHandle.index];
//         ArchetypeData* archetype = entity.archetype;
//
//         if (archetype)
//         {
//             archetype->RemoveEntity(Entity);
//         }
//
//         // Release entity
//         entity.Reset();
//         freeEntityList.push_back(entityHandle.index);
//     }
//
//     bool EntityManager::IsEntityValid(EntityHandle entityHandle) const
//     {
//         return (entityHandle.index < entities.size()) && (entityHandle.version == entities[entityHandle.index].version);
//     }
//
//     void EntityManager::MoveEntityToArchetype(EntityHandle entityHandle, ArchetypeHandle archetypeHandle)
//     {
//         assert(IsEntityValid(entityHandle));
//
//         ArchetypeData& newArchetype = FMassArchetypeHelper::ArchetypeDataFromHandleChecked(archetypeHandle);
//
//         EntityData& entity = entities[entityHandle.index];
//         entity.archetype = archetypeHandle.data;
//         entity.archetype->MoveEntityToAnotherArchetype(Entity, NewArchetype);
//     }
// }