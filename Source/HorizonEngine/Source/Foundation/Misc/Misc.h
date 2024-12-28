#pragma once

#include "Foundation/StdHeaders.h"
#include "Foundation/Definitions.h"
#include "Foundation/FundamentalTypes.h"

#include <codecvt>

namespace Horizon
{
    // Character Encoding
    // https://utf8everywhere.org/
    inline std::string UTF16ToUTF8(const wchar_t* s)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(s);
    }

    inline std::wstring UTF8ToUTF16(const char* s)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(s);
    }

    inline std::string UTF16ToUTF8(const std::wstring& s)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(s);
    }

    inline std::wstring UTF8ToUTF16(const std::string& s)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(s);
    }

    template<class T>
    class Singleton
    {
    private:
        Singleton(const Singleton<T>&) = delete;
        Singleton& operator=(const Singleton<T>&) = delete;
    protected:
        static T* Instance;
        Singleton(void)
        {
            ASSERT(!Instance && "Only one instance can exist for a singlton class");
            Instance = static_cast<T*>(this);
        }
        ~Singleton()
        {
            ASSERT(Instance && "No instance of this singleton has been initialized.");
            Instance = nullptr;
        }
    public:
        static T& Get()
        {
            return *Instance;
        }
        static T* GetPtr()
        {
            return Instance;
        }
    };

    //class Timestep
    //{
    //public:
    //    Timestep(float deltaTimeInSeconds = 0.0f) : deltaTimeInSeconds(deltaTimeInSeconds) {}
    //    float Seconds() const
    //    {
    //        return deltaTimeInSeconds;
    //    }
    //    float Milliseconds() const
    //    {
    //        return deltaTimeInSeconds * 1000.0f;
    //    }
    //private:
    //    float deltaTimeInSeconds;
    //};

    //class CpuTimer
    //{
    //public:

    //    using Clock = std::chrono::high_resolution_clock;
    //    using TimePoint = Clock::time_point;

    //    static TimePoint getCurrentTimePoint()
    //    {
    //        return Clock::now();
    //    }

    //    CpuTimer() : startTime(Clock::now()) {}

    //    void Reset()
    //    {
    //        startTime = Clock::now();
    //    }

    //    float ElapsedSeconds() const
    //    {
    //        return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - startTime).count() * 0.001f * 0.001f * 0.001f;
    //    }

    //    float ElapsedMilliseconds() const
    //    {
    //        return ElapsedSeconds() * 1000.0f;
    //    }

    //private:

    //    TimePoint startTime;
    //};

    //class Time
    //{
    //public:
    //    static void Reset()
    //    {
    //        Timer.Reset();
    //    }
    //    static float Now()
    //    {
    //        return Timer.ElapsedSeconds();
    //    }
    //    static float GetDeltaTime()
    //    {
    //        return DeltaTime;
    //    }
    //private:
    //    static CpuTimer Timer;
    //    static float DeltaTime;
    //    static uint32 FrameCounter;
    //};

    uint32 CRC32(const void* data, uint64 size, uint32 crc = 0);

    class ConsoleManager
    {
    public:

    private:

    };
}

//namespace Horizon
//{
//    CpuTimer Time::Timer = CpuTimer();
//    float Time::DeltaTime = 0.0f;
//    uint32 Time::FrameCounter = 0;
//}