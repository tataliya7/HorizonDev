#include "USDSkelSkeletonImporter.h"

#include "USDIncludeBegin.h"
#include <pxr/usd/usdSkel/skeletonQuery.h>
#include <pxr/usd/usdSkel/bindingAPI.h>
#include <pxr/usd/usdSkel/cache.h>
#include <pxr/usd/usdSkel/utils.h>
#include "USDIncludeEnd.h"
#include "USDUtility.h"

namespace Horizon::USDImporter
{
    USDSkelSkeletonImporter::USDSkelSkeletonImporter(USDImportContext& context)
        : context(&context)
    {

    }

    Skeleton* USDSkelSkeletonImporter::ImportSkeleton(const pxr::UsdSkelSkeleton& usdSkelSkeleton)
    {
        if (!usdSkelSkeleton)
        {
            return nullptr;
        }

        pxr::UsdSkelCache usdSkelCache;
        const pxr::UsdSkelSkeletonQuery& usdSkelSkeletonQuery = usdSkelCache.GetSkelQuery(usdSkelSkeleton);
        if (!usdSkelSkeletonQuery.IsValid())
        {
            return nullptr;
        }

        pxr::VtTokenArray jointOrder = usdSkelSkeletonQuery.GetJointOrder();
        const pxr::UsdSkelTopology skelTopology = usdSkelSkeletonQuery.GetTopology();

        if (jointOrder.size() != skelTopology.size())
        {
            return nullptr;
        }

        const size_t numJoints = skelTopology.GetNumJoints();
        if (numJoints == 0)
        {
            return nullptr;
        }

        const std::string& skeletonName = usdSkelSkeleton.GetPrim().GetName().GetString();

        Skeleton* skeleton = new Skeleton();
        skeleton->SetName(skeletonName);
        skeleton->joints.clear();
        skeleton->joints.resize(numJoints);

        if (numJoints == 0)
        {
            return nullptr;
        }

        pxr::VtMatrix4dArray jointWorldBindTransforms;
        if (!usdSkelSkeletonQuery.GetJointWorldBindTransforms(&jointWorldBindTransforms))
        {
            return nullptr;
        }

        if (numJoints != jointWorldBindTransforms.size())
        {
            return nullptr;
        }

        for (size_t i = 0; i < numJoints; i++)
        {
		    pxr::SdfPath jointPath(jointOrder[i]);
            pxr::GfMatrix4f bindTransform(jointWorldBindTransforms[i]);
            const int parentIndex = skelTopology.GetParent(i);

            Joint& joint = skeleton->joints[i];
            joint.name = jointPath.GetName();
            joint.parentIndex = parentIndex;
            joint.bindTransform = UsdToHorizon::ConvertMatrix(bindTransform);
        }

        const pxr::UsdSkelAnimQuery& usdSkelAnimQuery = usdSkelSkeletonQuery.GetAnimQuery();

        if (!usdSkelAnimQuery)
        {
            return nullptr;
        }

        std::vector<double> jointTransformTimeSamples;
        usdSkelAnimQuery.GetJointTransformTimeSamples(&jointTransformTimeSamples);

        if (jointTransformTimeSamples.empty())
        {
            return nullptr;
        }

        const size_t numJointTransformTimeSamples = jointTransformTimeSamples.size();

        //pxr::VtTokenArray jointOrder = usdSkelAnimQuery.GetJointOrder();

        pxr::VtMatrix4dArray usdJointLocalTransforms;
        for (double time : jointTransformTimeSamples)
        {
            if (!usdSkelAnimQuery.ComputeJointLocalTransforms(&usdJointLocalTransforms, time))
            {
                continue;
            }

            for (size_t jointIndex = 0; jointIndex < usdJointLocalTransforms.size(); jointIndex++)
            {
                pxr::GfMatrix4d localTransform = usdJointLocalTransforms[jointIndex];

                pxr::GfVec3f translation; pxr::GfQuatf rotation; pxr::GfVec3h scale;
                if (!pxr::UsdSkelDecomposeTransform(localTransform, &translation, &rotation, &scale))
                {
                    continue;
                }


            }
        }

        return skeleton;
    }
}