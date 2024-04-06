#pragma once

#include "Core/CoreCommon.h"

#include <codecvt>

namespace HE
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

    /**
     * Implements a Universal Unique Identifier.
     */
    struct Guid
    {
        /** Generate a new Guid. */
        static Guid Generate();
        /** Convert Guid to std::string. */
        static std::string ToString(const Guid& uuid);
        /** The first component. */
        uint64 a;
        /** The second component. */
        uint64 b;
        Guid() : a(0), b(0) {}
        Guid(uint64 a, uint64 b) : a(a), b(b) {}
        FORCEINLINE operator bool() const noexcept
        {
            return (a != 0) || (b != 0);
        }
        FORCEINLINE bool operator==(const Guid& rhs) const noexcept
        {
            return (a == rhs.a) && (b == rhs.b);
        }
        FORCEINLINE bool operator!=(const Guid& rhs) const noexcept
        {
            return !(*this == rhs);
        }
    };

    /** The Guid should be 128 bits. */
    static_assert(sizeof(Guid) == 16, "Wrong Guid size");

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

    uint32 Crc32(const void* data, uint64 size, uint32 crc = 0);

    class ConsoleManager
    {
    public:

    private:

    };
}

//namespace HE
//{
//    CpuTimer Time::Timer = CpuTimer();
//    float Time::DeltaTime = 0.0f;
//    uint32 Time::FrameCounter = 0;
//}