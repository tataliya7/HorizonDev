#pragma once

#include "RenderGraphCommon.h"

namespace Horizon
{
    class RenderGraphBlackboard
    {
    public:

        RenderGraphBlackboard(MemoryArena* arena);

        RenderGraphBlackboard(RenderGraphBlackboard&& other) = delete;
        RenderGraphBlackboard(const RenderGraphBlackboard& other) = delete;
        RenderGraphBlackboard& operator=(RenderGraphBlackboard&&) = delete;
        RenderGraphBlackboard& operator=(const RenderGraphBlackboard&) = delete;

        template<typename StructType>
        StructType& Create()
        {
            const uint32 structTypeIndex = GetStructTypeIndex<StructType>();
            if (structTypeIndex >= blackboard.size())
            {
                blackboard.push_back(nullptr);
            }
            assert((blackboard[structTypeIndex] == nullptr) && "RenderGraphBlackboard duplicate Create() called. Only one Create() call per struct type is allowed.");
            void* result = blackboard[structTypeIndex] = HE_ARENA_ALLOC(arena, sizeof(StructType));
            assert(result);
            return static_cast<StructInstanceContainer<StructType>*>(result)->instance;
        }

        template<typename StructType>
        StructType& Get() const
        {
            const uint32 structTypeIndex = GetStructTypeIndex<StructType>();
            assert((blackboard[structTypeIndex] != nullptr) && std::format("Failed to get instance of struct '{}'. Please register and create instance for this type first.", typeid(StructType).name()).c_str());
            StructInstanceContainer<StructType>* result = static_cast<StructInstanceContainer<StructType>*>(blackboard[structTypeIndex]);
            return result->instance;
        }

        template<typename StructType>
        std::optional<StructType>& GetOptional() const
        {
            const uint32 structTypeIndex = GetStructTypeIndex<StructType>();
            if (blackboard[structTypeIndex] != nullptr)
            {
                return *(static_cast<const StructInstanceContainer<StructType>*>(blackboard[structTypeIndex])->instance);
            }
            return std::nullopt;
        }

    private:

        static std::atomic<uint32> RegisteredStructTypeCount;

        template<typename StructType>
        struct StructInstanceContainer
        {
            template<typename... Args>
            StructInstanceContainer(Args&&... args) : instance(std::forward<Args&&>(args)...) {}
            StructType instance;
        };

        // TODO: Optimize this, use static reflection instead.
        template<typename StructType>
        static uint32 GetStructTypeIndex()
        {
            static uint32 index = UINT32_MAX;
            if (index == UINT32_MAX)
            {
                index = RegisteredStructTypeCount.load();
                RegisteredStructTypeCount.fetch_add(1);
                return index;
            }
            return index;
        }

        MemoryArena* arena;

        std::vector<void*> blackboard = {};
    };
}