#pragma once

#include "Subsystem.h"

namespace Horizon
{
    class HorizonEngine
    {
    public:

        static HorizonEngine* GetInstance()
        {
            return Instance;
        }

        HorizonEngine();
        ~HorizonEngine();

        HorizonEngine(HorizonEngine&&) = delete;
        HorizonEngine(const HorizonEngine&) = delete;
        HorizonEngine& operator=(HorizonEngine&&) = delete;
        HorizonEngine& operator=(const HorizonEngine&) = delete;

        void Tick(float deltaTimeInSeconds);

        template <typename SubsystemType>
        SubsystemType* GetSubsystem() const
        {
            return subsystemRegistry.GetSubsystem<SubsystemType>();
        }

        // TODO
        class StreamlineContext* streamlineContext = nullptr;

    private:

        friend void InitializeEngine();

        static HorizonEngine* Instance;

        void RegisterAndInitializeSubsystems();

        SubsystemRegistry subsystemRegistry;
    };

    extern void InitializeEngine();
}