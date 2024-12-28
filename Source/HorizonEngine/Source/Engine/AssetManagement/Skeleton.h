#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"
//#include "Physics/PhysicsModule.h"
#include "Engine/EntityComponentSystem/EntityManager.h"
#include "Engine/Serialization/SerializationModule.h"

namespace Horizon
{
    class Skeleton
    {
    public:

        Skeleton();
        virtual ~Skeleton();

        std::string GetName() const
        {
            return name;
        }

        void SetName(const std::string& newName)
        {
            name = newName;
        }

        uint32 GetJointCount() const
        {
            return jointCount;
        }

    private:

        std::string name;

        uint32 jointCount;

        std::vector<int> parentIndices;
        std::vector<std::string> jointNames;
        std::vector<Matrix4x4f> bindTransforms;
    };

    class SkeletonAnimation
    {

    };
}