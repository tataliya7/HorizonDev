#pragma once

#include "Core/CoreModule.h"

namespace HE::Audio
{
    extern bool AudioEngineInit();

    extern void AudioEngineExit();

    extern void PlaySoundEX(const char* filename);

    extern void StopAll();

    extern void PauseAll();
        
    extern void ResumeAll();
}
