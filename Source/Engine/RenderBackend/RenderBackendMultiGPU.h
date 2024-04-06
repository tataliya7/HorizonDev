#pragma once

#include "RenderBackendCommon.h"

#define RENDER_BACKEND_VIRTUAL_MGPU 1

namespace HE
{
    struct RenderBackendDeviceMask
    {
    public:

        FORCEINLINE static const RenderBackendDeviceMask All()
        {
            return RenderBackendDeviceMask(~0u);
        }

        FORCEINLINE static const RenderBackendDeviceMask None()
        {
            return RenderBackendDeviceMask(0u);
        }

        FORCEINLINE explicit RenderBackendDeviceMask(uint32 mask) : mask(mask) {}

        FORCEINLINE RenderBackendDeviceMask() : RenderBackendDeviceMask(RenderBackendDeviceMask::All()) {}

        FORCEINLINE uint32 Get() const
        {
            return RENDER_BACKEND_VIRTUAL_MGPU ? 1 : mask;
        }

        FORCEINLINE bool operator ==(const RenderBackendDeviceMask& rhs) const { return mask == rhs.mask; }
        FORCEINLINE bool operator !=(const RenderBackendDeviceMask& rhs) const { return mask != rhs.mask; }
        void operator |=(const RenderBackendDeviceMask& rhs) { mask |= rhs.mask; }
        void operator &=(const RenderBackendDeviceMask& rhs) { mask &= rhs.mask; }

        FORCEINLINE RenderBackendDeviceMask operator &(const RenderBackendDeviceMask& rhs) const
        {
            return RenderBackendDeviceMask(mask & rhs.mask);
        }

        FORCEINLINE RenderBackendDeviceMask operator |(const RenderBackendDeviceMask& rhs) const
        {
            return RenderBackendDeviceMask(mask | rhs.mask);
        }

        struct Iterator
        {
            static uint32 CountTrailingZeros(uint32 value)
            {
                if (value == 0)
                {
                    return 32;
                }
                unsigned long result;
                _BitScanForward(&result, value);
                return (uint32)result;
            }

            FORCEINLINE explicit Iterator(uint32 mask) : mask(mask), firstNonZeroBit(0)
            {
                firstNonZeroBit = CountTrailingZeros(mask);
            }

            FORCEINLINE explicit Iterator(const RenderBackendDeviceMask& gpuMask) : Iterator(gpuMask.mask)
            {

            }

            FORCEINLINE Iterator& operator++()
            {
                mask &= ~(1 << firstNonZeroBit);
                firstNonZeroBit = CountTrailingZeros(mask);
                return *this;
            }

            FORCEINLINE Iterator operator++(int)
            {
                Iterator copy(*this);
                ++(*this);
                return copy;
            }

            FORCEINLINE uint32 operator*() const { return firstNonZeroBit; }
            FORCEINLINE bool operator !=(const Iterator& rhs) const { return mask != rhs.mask; }
            FORCEINLINE explicit operator bool() const { return mask != 0; }
            FORCEINLINE bool operator !() const { return !(bool)*this; }

        private:

            uint32 mask;
            uint32 firstNonZeroBit;
        };

        FORCEINLINE friend RenderBackendDeviceMask::Iterator begin(const RenderBackendDeviceMask& gpuMask) { return RenderBackendDeviceMask::Iterator(gpuMask.mask); }
        FORCEINLINE friend RenderBackendDeviceMask::Iterator end(const RenderBackendDeviceMask& gpuMask) { return RenderBackendDeviceMask::Iterator(0); }

    private:

        uint32 mask;
    };
}