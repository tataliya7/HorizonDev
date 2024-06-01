#pragma once

#include "Foundation/FoundationModule.h"

namespace Horizon
{
    class Subsystem
    {
    public:
        virtual ~Subsystem();
        virtual void Init();
        virtual void Exit();
    };

    class SubsystemRegistry
    {
    public:

        template<typename SubsystemType>
        SubsystemType* RegisterSubsystem()
        {
            static std::type_index index = typeid(SubsystemType);

            SubsystemType* instance = new SubsystemType();
            // TODO: handle multi-threading access
            subsystemMap.emplace(index, instance);

            return instance;
        }

        template<typename SubsystemType>
        SubsystemType* GetSubsystem() const
        {
            static_assert(std::is_base_of_v<Subsystem, SubsystemType>, "SubsystemType must be derived from Subsystem.");

            static std::type_index index = typeid(SubsystemType);
            return static_cast<SubsystemType*>(GetSubsystemByTypeIndex(index));
        }

    private:

        Subsystem* GetSubsystemByTypeIndex(const std::type_index& index) const;

        std::map<std::type_index, Subsystem*> subsystemMap;
    };
}