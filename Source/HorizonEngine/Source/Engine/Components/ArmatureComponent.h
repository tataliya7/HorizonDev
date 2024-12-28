#pragma once

#include "Foundation/FoundationModule.h"
#include "Engine/Serialization/SerializationModule.h"

namespace Horizon
{
    struct ArmatureComponent
    {
        struct Bone
        {
            std::string name;
            int32 parentIndex = -1;
            std::vector<uint32> children;
            Matrix4x4f inverseBindMatrix = Matrix4x4f(1);
        };
        struct Pose
        {
            std::vector<Matrix4x4f> boneMatrices;
        };
        std::vector<Bone> bones;
        std::map<std::string, int32> boneMap;
        Pose pose;
    };
}