#include "RenderGraphBlackboard.h"

namespace Horizon
{
    std::atomic<uint32> RenderGraphBlackboard::RegisteredStructTypeCount = 0;

    RenderGraphBlackboard::RenderGraphBlackboard(MemoryArena* arena)
        : arena(arena)
    {
        blackboard.resize(RegisteredStructTypeCount);
        for (size_t index = 0; index < blackboard.size(); index++)
        {
            blackboard[index] = nullptr;
        }
    }
}