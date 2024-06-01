#pragma once

#include "Core/CoreCommon.h"
#include "Core/Math/Math.h"
#include "Core/Serialization/Archive.h"

namespace Horizon
{
    class SceneHierarchyComponent
    {
        uint32 depth;
        uint32 numChildren;
        EntityHandle parent;
        EntityHandle firstChild;
        EntityHandle next;
        EntityHandle prev;

        SceneHierarchyComponent()
            : depth(0)
            , numChildren(0)
            , parent()
            , firstChild()
            , next()
            , prev()
        {

        }
    };
}
