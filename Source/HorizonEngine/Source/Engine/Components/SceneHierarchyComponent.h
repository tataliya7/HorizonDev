#pragma once

#include "Foundation/FoundationModule.h"
#include "Engine/EntityComponentSystem/EntityManager.h"
#include "Engine/Serialization/Archive.h"

namespace Horizon
{
    class SceneHierarchyComponent
    {
    public:
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