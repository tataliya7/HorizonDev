#pragma once

#include "RenderBackendCommon.h"

#define RENDER_BACKEND_VIRTUAL_MGPU 1

namespace Horizon
{
    struct RenderBackendDeviceMask
    {
    public:

        static const RenderBackendDeviceMask All()
        {
            return RenderBackendDeviceMask(~0u);
        }

        static const RenderBackendDeviceMask None()
        {
            return RenderBackendDeviceMask(0u);
        }

        explicit RenderBackendDeviceMask(uint32 mask) : mask(mask) {}

        RenderBackendDeviceMask() : RenderBackendDeviceMask(RenderBackendDeviceMask::All()) {}

        uint32 Get() const
        {
            return RENDER_BACKEND_VIRTUAL_MGPU ? 1 : mask;
        }

        bool operator ==(const RenderBackendDeviceMask& rhs) const { return mask == rhs.mask; }
        bool operator !=(const RenderBackendDeviceMask& rhs) const { return mask != rhs.mask; }
        void operator |=(const RenderBackendDeviceMask& rhs) { mask |= rhs.mask; }
        void operator &=(const RenderBackendDeviceMask& rhs) { mask &= rhs.mask; }

        RenderBackendDeviceMask operator &(const RenderBackendDeviceMask& rhs) const
        {
            return RenderBackendDeviceMask(mask & rhs.mask);
        }

        RenderBackendDeviceMask operator |(const RenderBackendDeviceMask& rhs) const
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

            explicit Iterator(uint32 mask) : mask(mask), firstNonZeroBit(0)
            {
                firstNonZeroBit = CountTrailingZeros(mask);
            }

            explicit Iterator(const RenderBackendDeviceMask& gpuMask) : Iterator(gpuMask.mask)
            {

            }

            Iterator& operator++()
            {
                mask &= ~(1 << firstNonZeroBit);
                firstNonZeroBit = CountTrailingZeros(mask);
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator copy(*this);
                ++(*this);
                return copy;
            }

            uint32 operator*() const { return firstNonZeroBit; }
            bool operator !=(const Iterator& rhs) const { return mask != rhs.mask; }
            explicit operator bool() const { return mask != 0; }
            bool operator !() const { return !(bool)*this; }

        private:

            uint32 mask;
            uint32 firstNonZeroBit;
        };

        friend RenderBackendDeviceMask::Iterator begin(const RenderBackendDeviceMask& gpuMask) { return RenderBackendDeviceMask::Iterator(gpuMask.mask); }
        friend RenderBackendDeviceMask::Iterator end(const RenderBackendDeviceMask& gpuMask) { return RenderBackendDeviceMask::Iterator(0); }

    private:

        uint32 mask;
    };
}