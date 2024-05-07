#pragma once

#include "Core/CoreCommon.h"
#include "Core/Math/Math.h"
#include "Core/Serialization/Archive.h"

namespace Horizon
{
    class NameComponent
    {
    public:
        NameComponent(const char* name) : name(name) {}
        NameComponent(const std::string& name) : name(name) {}

        inline operator std::string& () { return name; }
        inline operator const std::string& () const { return name; }
        inline void operator=(const std::string& str) { name = str; }
        inline bool operator==(const std::string& str) const { return name.compare(str) == 0; }

        const char* GetName() const;
    private:
        std::string name;
    };
}
