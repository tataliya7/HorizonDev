#pragma once

#include "Core/CoreModule.h"

namespace Horizon
{
    class AudioSystem final : public EngineSubsystem
    {
    public:

        bool Init() override;

        void Exit() override;

        void PlaySoundEX(const char* filename);

        void StopAll();

        void PauseAll();

        void ResumeAll();
    private:

        ma_engine engine;
    };
}
