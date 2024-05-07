#pragma once

namespace Horizon
{
    enum class AudioSourceComponentState : uint8
    {
        Playing,
        Stopped,
        Paused,
        FadingIn,
        FadingOut,
    };

    struct AudioSourceComponent
    {
        std::string audio;

        float volumeMultiplier = 1.0f;

        AudioSourceComponent() = default;
        AudioSourceComponent(const AudioSourceComponent& other) = default;

        //AudioSourceComponentState GetState() const
        //{
        //    if (!IsActive())
        //    {
        //        return EAudioComponentPlayState::Stopped;
        //    }

        //    if (bIsPaused)
        //    {
        //        return EAudioComponentPlayState::Paused;
        //    }

        //    if (bIsFadingOut)
        //    {
        //        return EAudioComponentPlayState::FadingOut;
        //    }

        //    // Get the current audio time seconds and compare when it started and the fade in duration
        //    float CurrentAudioTimeSeconds = GetAudioTimeSeconds();
        //    if (CurrentAudioTimeSeconds - TimeAudioComponentPlayed < FadeInTimeDuration)
        //    {
        //        return EAudioComponentPlayState::FadingIn;
        //    }

        //    // If we are not in any of the above states we are "playing"
        //    return EAudioComponentPlayState::Playing;
        //}

        //void AdjustVolume(float AdjustVolumeDuration, float AdjustVolumeLevel, const EAudioFaderCurve FadeCurve = EAudioFaderCurve::Linear)
        //{

        //}

        void SetPitchMultiplier(float NewPitchMultiplier)
        {

        }

        void Play()
        {
            // Audio::PlaySoundEX(audio.c_str());
        }
    };
}
