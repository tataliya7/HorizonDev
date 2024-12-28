#include "Subsystem.h"

namespace Horizon
{
    Subsystem::~Subsystem()
    {

    }

    void Subsystem::Init()
    {

    }

    void Subsystem::Exit()
    {

    }

    Subsystem* SubsystemRegistry::GetSubsystemByTypeIndex(const std::type_index& index) const
    {
        const auto& found = subsystemMap.find(index);
        if (found != subsystemMap.end())
        {
            return found->second;
        }
        return nullptr;
    }
}