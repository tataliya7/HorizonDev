// #pragma once
//
// #include <vector>
//
// namespace ECS
// {
//     struct MetatypeHash
//     {
//         size_t name_hash{ 0 };
//         size_t matcher_hash{ 0 };
//
//         bool operator==(const MetatypeHash& other) const
//         {
//             return name_hash == other.name_hash;
//         }
//
//         template<typename T>
//         static constexpr const char* name_detail()
//         {
//             return __FUNCSIG__;
//         }
//
//         template<typename T>
//         static constexpr size_t hash()
//         {
//             static_assert(!std::is_reference_v<T>, "dont send references to hash");
//             static_assert(!std::is_const_v<T>, "dont send const to hash");
//             return hash_fnv1a(name_detail<T>());
//         }
//     };
//
//     struct Metatype
//     {
//         using ConstructorFn = void(void*);
//         using DestructorFn = void(void*);
//
//         MetatypeHash hash;
//
//         const char* name{ "none" };
//         ConstructorFn* constructor;
//         DestructorFn* destructor;
//         uint16_t size{ 0 };
//         uint16_t align{ 0 };
//
//         bool is_empty() const { return align == 0; };
//
//         template<typename T>
//         static constexpr MetatypeHash build_hash()
//         {
//             using sanitized = std::remove_const_t<std::remove_reference_t<T>>;
//
//             MetatypeHash hash;
//             hash.name_hash = MetatypeHash::hash<sanitized>();
//             hash.matcher_hash |= (uint64_t)0x1L << (uint64_t)((hash.name_hash) % 63L);
//             return hash;
//         }
//
//         template<typename T>
//         static constexpr Metatype build() {
//
//             Metatype meta{};
//             meta.hash = build_hash<T>();
//
//             if constexpr (std::is_empty_v<T>)
//             {
//                 meta.align = 0;
//                 meta.size = 0;
//             }
//             else {
//                 meta.align = alignof(T);
//                 meta.size = sizeof(T);
//             }
//
//             meta.constructor = [](void* p) {
//                 new(p) T{};
//             };
//             meta.destructor = [](void* p) {
//                 ((T*)p)->~T();
//             };
//
//             return meta;
//         };
//     };
//
//     template<typename T>
//     static const Metatype* get_metatype()
//     {
//         static const Metatype* mt = []() {
//             constexpr size_t name_hash = Metatype::build_hash<T>().name_hash;
//
//             auto type = metatype_cache.find(name_hash);
//             if (type == metatype_cache.end()) {
//                 constexpr Metatype newtype = Metatype::build<T>();
//                 metatype_cache[name_hash] = newtype;
//             }
//             return &metatype_cache[name_hash];
//         }();
//         return mt;
//     }
//
//     // TODO: Make it configurable
//     uint32_t ChunkSize = 128 * 1024; // 128KB
//
//     struct alignas(8) EntityHandle
//     {
//         uint32_t index;
//         uint32_t version;
//     };
//     static_assert(sizeof(EntityHandle) == sizeof(uint64_t));
//     static_assert(alignof(EntityHandle) == sizeof(uint64_t));
//
//     struct ArchetypeChunk
//     {
//         uint32_t entityCount = 0;
//         uint32_t capacity = 0;
//         uint8_t* memory = nullptr;
//         uint32_t size = 0;
//
//         bool IsFull() const
//         {
//             return entityCount == capacity;
//         }
//     };
//
//     struct ArchetypeData
//     {
//         uint32_t fullChunks;
//         std::vector<ArchetypeChunk> chunkList;
//     };
//
//     struct EntityData
//     {
//         uint32_t version = 0;
//         ArchetypeData* archetype = nullptr;
//         uint32_t chunkIndex = 0;
//         uint32_t indexWithinChunk = 0;
//
//         bool IsValid() const
//         {
//             return (archetype != nullptr) && (version != 0);
//         }
//
//         void Reset()
//         {
//             archetype = nullptr;
//             version = 0;
//         }
//     };
//
//     struct ArchetypeHandle
//     {
//         ArchetypeData* data;
//     };
//
//     class EntityManager
//     {
//     public:
//
//         EntityHandle CreateEntity(const ArchetypeHandle& archetypeHandle, const FMassArchetypeSharedFragmentValues& SharedFragmentValues = {});
//
//         void DestroyEntity(EntityHandle entityHandle);
//
//         bool IsEntityValid(EntityHandle entityHandle) const;
//
//         void MoveEntityToArchetype(EntityHandle entityHandle, ArchetypeHandle archetypeHandle);
//
//         template<typename Component>
//         Component& GetComponent(EntityHandle entityHandle) const
//         {
//             const EntityData& entity = entities[entityHandle.index];
//             auto acrray = get_chunk_array<Component>(entity.chunk);
//             assert(acrray.chunkOwner != nullptr);
//             return acrray[storage.chunkIndex];
//         }
//
//         template<typename Component>
//         void AddComponent(EntityHandle entityHandle)
//         {
//             const Metatype* temporalMetatypeArray[32];
//             const Metatype* type = get_metatype<C>();
//
//             Archetype* oldarch = get_entity_archetype(world, id);
//             ChunkComponentList* oldlist = oldarch->componentList;
//             bool typeFound = false;
//             int lenght = oldlist->components.size();
//             for (int i = 0; i < oldlist->components.size(); i++)
//             {
//                 temporalMetatypeArray[i] = oldlist->components[i].type;
//
//                 //the pointers for metatypes are allways fully stable
//                 if (temporalMetatypeArray[i] == type)
//                     {
//                     typeFound = true;
//                 }
//             }
//
//             Archetype* newArch = oldarch;
//             if (!typeFound)
//             {
//                 temporalMetatypeArray[lenght] = type;
//                 sort_metatypes(temporalMetatypeArray, lenght + 1);
//                 lenght++;
//                 newArch = find_or_create_archetype(world, temporalMetatypeArray, lenght);
//                 set_entity_archetype(newArch, id);
//             }
//         }
//
//     private:
//         std::vector<EntityData> entities;
//         std::vector<uint32_t> freeEntityList;
//     };
// }