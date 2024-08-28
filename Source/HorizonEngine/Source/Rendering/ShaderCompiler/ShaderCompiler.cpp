#include "ShaderCompiler.h"

namespace Horizon
{
    void ShaderBlob::Allocate(const void* srcData, uint64 srcSize)
    {
        assert(!IsValid() && srcData != nullptr && srcSize > 0);
        void* memory = malloc(srcSize);
        if (memory != nullptr)
        {
            memcpy(memory, srcData, srcSize);
            data = memory;
            size = srcSize;
        }
    }

    void ShaderBlob::Release()
    {
        /*if (IsValid())
        {
            free(data);
            data = nullptr;
            size = 0;
        }*/
    }
}