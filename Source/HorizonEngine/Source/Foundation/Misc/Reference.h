#pragma once

#include "Foundation/StdHeaders.h"

namespace Horizon
{
    template <typename Type>
    using Reference = std::shared_ptr<Type>;
}