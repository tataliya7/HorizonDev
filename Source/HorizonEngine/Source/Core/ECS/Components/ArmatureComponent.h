#pragma once

#include "Core/CoreCommon.h"
#include "Core/Math/Math.h"
#include "Core/Serialization/Archive.h"

namespace Horizon
{
    struct ArmatureComponent
    {
        struct Bone
        {
            std::string name;
            int32 parentIndex = -1;
            std::vector<uint32> children;
            Matrix4x4 inverseBindMatrix = Matrix4x4(1);
        };
        struct Pose
        {
            std::vector<Matrix4x4> boneMatrices;
        };
        std::vector<Bone> bones;
        std::map<std::string, int32> boneMap;
        Pose pose;
    };
}
