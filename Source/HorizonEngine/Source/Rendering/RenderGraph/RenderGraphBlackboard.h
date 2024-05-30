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
        static void RegisterStructType()
        {
            bool firstTime = false;
            GetStructTypeIndex<StructType>(firstTime);
            assert(firstTime && "RegisterStructType() must be called before other functions, and it should be called only once.");
        }

        template<typename StructType>
        StructType& Create()
        {
            bool firstTime = false;
            const uint32 structTypeIndex = GetStructTypeIndex<StructType>(firstTime);
            assert(!firstTime && "RegisterStructType() must be called before Create().");
            assert((blackboard[structTypeIndex] == nullptr) && "RenderGraphBlackboard duplicate Create() called. Only one Create() call per struct type is allowed.");
            void* result = blackboard[structTypeIndex] = HE_ARENA_ALLOC(arena, sizeof(StructType));
            assert(result);
            return static_cast<StructInstanceContainer<StructType>*>(result)->instance;
        }

        template<typename StructType>
        StructType& Get() const
        {
            bool firstTime = false;
            const uint32 structTypeIndex = GetStructTypeIndex<StructType>(firstTime);
            assert(!firstTime && "RegisterStructType() must be called before Get().");
            assert((blackboard[structTypeIndex] != nullptr) && std::format("Failed to get instance of struct '{}'. Please register and create instance for this type first.", typeid(StructType).name()).c_str());
            StructInstanceContainer<StructType>* result = static_cast<StructInstanceContainer<StructType>*>(blackboard[structTypeIndex]);
            return result->instance;
        }

        template<typename StructType>
        std::optional<StructType>& GetOptional() const
        {
            bool firstTime = false;
            const uint32 structTypeIndex = GetStructTypeIndex<StructType>(firstTime);
            assert(!firstTime && "RegisterStructType() must be called before GetOptional().");
            if (blackboard[structTypeIndex] != nullptr)
            {
                return *(static_cast<const StructInstanceContainer<StructType>*>(blackboard[structTypeIndex])->instance);
            }
            return std::nullopt;
        }

    private:

        static uint32 RegisteredStructTypeCount;

        template<typename StructType>
        struct StructInstanceContainer
        {
            template<typename... Args>
            StructInstanceContainer(Args&&... args) : instance(std::forward<Args&&>(args)...) {}
            StructType instance;
        };

        template<typename StructType>
        static uint32 GetStructTypeIndex(bool& outFirstTime)
        {
            static uint32 index = UINT32_MAX;
            if (index == UINT32_MAX)
            {
                index = RegisteredStructTypeCount;
                RegisteredStructTypeCount++;
                outFirstTime = true;
                return index;
            }
            outFirstTime = false;
            return index;
        }

        MemoryArena* arena;

        std::vector<void*> blackboard = {};
    };

    template<typename StructType>
    struct RenderGraphBlackboardRegistry
    {
        RenderGraphBlackboardRegistry()
        {
            RenderGraphBlackboard::RegisterStructType<StructType>();
        }
    };
}