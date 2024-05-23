#pragma once

#include "Foundation/StdHeaders.h"
#include "Foundation/Definitions.h"
#include "Foundation/FundamentalTypes.h"

#define HE_DEFAULT_ALIGNMENT uint64(8)

#define HE_ARENA_ALLOC(arena, size)                                          (::Horizon::ArenaRealloc(arena, nullptr, 0,       size,    HE_DEFAULT_ALIGNMENT, __FILE__, __LINE__))
#define HE_ARENA_FREE(arena, ptr, size)                                      (::Horizon::ArenaRealloc(arena, ptr,     size,    0,       HE_DEFAULT_ALIGNMENT, __FILE__, __LINE__))
#define HE_ARENA_REALLOC(arena, ptr, oldSize, newSize)                       (::Horizon::ArenaRealloc(arena, ptr,     oldSize, newSize, HE_DEFAULT_ALIGNMENT, __FILE__, __LINE__))
#define HE_ARENA_ALIGNED_ALLOC(arena, size, alignment)                       (::Horizon::ArenaRealloc(arena, nullptr, 0,       size,    alignment,            __FILE__, __LINE__))
#define HE_ARENA_ALIGNED_FREE(arena, ptr, size, alignment)                   (::Horizon::ArenaRealloc(arena, ptr,     size,    0,       alignment,            __FILE__, __LINE__))
#define HE_ARENA_ALIGNED_REALLOC(arena, ptr, oldSize, newSize, alignment)    (::Horizon::ArenaRealloc(arena, ptr,     oldSize, newSize, alignment,            __FILE__, __LINE__))

namespace Horizon
{
    class MemoryArena
    {
    public:
        MemoryArena() = default;
        virtual ~MemoryArena() = default;
        MemoryArena(const MemoryArena&) = delete;
        MemoryArena& operator=(const MemoryArena&) = delete;
        virtual void* Alloc(uint64 size, uint64 alignment) = 0;
        virtual void Free(void* ptr, uint64 size) = 0;
        virtual const char* GetName() const = 0;
    };

    class HeapArena : public MemoryArena
    {
    public:
        HeapArena() = default;
        HeapArena(const char* name) : name(name) {}
        ~HeapArena() = default;
        HeapArena(const HeapArena& rhs) = delete;
        HeapArena& operator=(const HeapArena& rhs) = delete;
        void* Alloc(uint64 size, uint64 alignment = alignof(std::max_align_t)) override
        {
            return _aligned_malloc(size, alignment);
        }
        void Free(void* ptr, uint64 size) override
        {
            _aligned_free(ptr);
        }
        char const* GetName() const override
        {
            return name;
        }
    private:
        const char* name = nullptr;
    };

    class LinearArena : public MemoryArena
    {
    public:
        LinearArena(const char* name, uint64 size)
            : name(name), size(size)
        {
            begin = malloc(size);
        }
        ~LinearArena()
        {
            free(begin);
        }
        LinearArena(const LinearArena& rhs) = delete;
        LinearArena& operator=(const LinearArena& rhs) = delete;
        void* Alloc(uint64 size, uint64 alignment)
        {
            void* const p = Align(GetCurrent(), alignment);
            void* const c = Add(p, size);
            bool success = (c <= Add(begin, this->size));
            if (success)
            {
                SetCurrent(c);
            }
            return success ? p : nullptr;
        }
        void Free(void* ptr, uint64 size)
        {

        }
        void Reset()
        {
            used = 0;
        }
        uint64 Allocated() const
        {
            return used;
        }
        uint64 Available() const
        {
            return size - used;
        }
        char const* GetName() const override
        {
            return name;
        }
    private:
        inline void* Add(void* ptr, uint64 offset)
        {
            return (void*)(uint64(ptr) + offset);
        }
        inline void* Align(void* ptr, uint64 alignment)
        {
            assert(alignment && !(alignment & alignment - 1));
            return (void*)((uint64(ptr) + alignment - 1) & ~(alignment - 1));
        }
        void* GetCurrent()
        {
            return Add(begin, used);
        }
        void SetCurrent(void* ptr)
        {
            used = uint64(ptr) - uint64(begin);
        }
    private:
        const char* name = nullptr;
        void* begin = nullptr;
        uint64 size = 0;
        uint64 used = 0;
    };

    class FrameAllocator : public MemoryArena
    {

    };

    extern LinearArena* GArena;

    void* ArenaRealloc(MemoryArena* arena, void* ptr, uint64 oldSize, uint64 newSize, uint64 alignment, const char* file, uint32 line);
}
