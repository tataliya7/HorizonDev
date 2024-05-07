#include "AudioSystem.h"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

namespace Horizon
{
    ma_engine engine;

    bool AudioEngineInit()
    {
        ma_result result = ma_engine_init(NULL, &engine);
        if (result != MA_SUCCESS)
        {
            printf("Failed to initialize audio engine.");
            return false;
        }
        return true;
    }

    void AudioEngineExit()
    {
        ma_engine_uninit(&engine);
    }

    void PlaySoundEX(const char* filename)
    {
        ma_engine_play_sound(&engine, filename, NULL);
    }

    void StopAll()
    {

    }

    void PauseAll()
    {

    }

    void ResumeAll()
    {

    }
}
