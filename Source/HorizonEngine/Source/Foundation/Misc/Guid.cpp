#include "Foundation/Misc/Misc.h"

#include <windows.h>
#include <objbase.h>

namespace Horizon
{
    void GenerateGuidImpl(Guid* guid)
    {
        assert(CoCreateGuid((GUID*)guid) == S_OK);
    }

    Guid Guid::Generate()
    {
        Guid guid;
        GenerateGuidImpl(&guid);
        return guid;
    }

    std::string Guid::ToString(const Guid& guid)
    {
        return std::string();
    }
}