#include "USDSkelSkeletonImporter.h"

#include "USDIncludeBegin.h"
#include <pxr/usd/usdSkel/skeletonQuery.h>
#include <pxr/usd/usdSkel/animation.h>
#include <pxr/usd/usdSkel/bindingAPI.h>
#include <pxr/usd/usdSkel/blendShape.h>
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

        const pxr::UsdSkelTopology skelTopology = usdSkelSkeletonQuery.GetTopology();

        pxr::VtTokenArray jointOrder = usdSkelSkeletonQuery.GetJointOrder();

        if (jointOrder.size() != skelTopology.size())
        {
            return nullptr;
        }

        const size_t numJoints = skelTopology.GetNumJoints();
        if (numJoints == 0)
        {
            return nullptr;
        }

        pxr::VtMatrix4dArray jointWorldBindTransforms;
        if (!usdSkelSkeletonQuery.GetJointWorldBindTransforms(&jointWorldBindTransforms))
        {
            return nullptr;
        }

        if (jointWorldBindTransforms.size() != numJoints)
        {
            return nullptr;
        }

        const std::string& skeletonName = usdSkelSkeleton.GetPrim().GetName().GetString();

        Skeleton* skeleton = new Skeleton();
        skeleton->SetName(skeletonName);

        for (size_t i = 0; i < numJoints; i++)
        {
            pxr::GfMatrix4f bindTransform(jointWorldBindTransforms[i]);
            //skeleton.bindTransform = UsdToHorizon::ConvertMatrix(bindTransform);
        }

        for (size_t i = 0; i < numJoints; i++)
        {
            const int parentIndex = skelTopology.GetParent(i);

            //if (parentIndex >= numJoints)
            //{
            //    assert(false);
            //}


        }

        return skeleton;
#if 0
        if (prim.IsInstanceProxy())
        {
            return;
        }

        pxr::UsdSkelBindingAPI skelBindingApi = pxr::UsdSkelBindingAPI::Apply(prim);
        if (!skelBindingApi)
        {
            return;
        }

        pxr::UsdSkelSkeleton skelSkeleton = skelBindingApi.GetInheritedSkeleton();
        if (!skelSkeleton)
        {
            return;
        }

        pxr::VtArray<pxr::TfToken> joints;
        if (skelBindingApi.GetJointsAttr().HasAuthoredValue())
        {
            skelBindingApi.GetJointsAttr().Get(&joints);
        }
        else if (skelSkeleton.GetJointsAttr().HasAuthoredValue())
        {
            skelSkeleton.GetJointsAttr().Get(&joints);
        }

        if (joints.empty())
        {
            return;
        }

        pxr::UsdGeomPrimvar jointIndicesPrimvar = skelBindingApi.GetJointIndicesPrimvar();
        if (!(jointIndicesPrimvar && jointIndicesPrimvar.HasAuthoredValue()))
        {
            return;
        }

        pxr::UsdGeomPrimvar jointWeightsPrimvar = skelBindingApi.GetJointWeightsPrimvar();
        if (!(jointWeightsPrimvar && jointWeightsPrimvar.HasAuthoredValue()))
        {
            return;
        }

        int jointIndicesElementSize = jointIndicesPrimvar.GetElementSize();
        int jointWeightsElementSize = jointWeightsPrimvar.GetElementSize();

        if (jointIndicesElementSize != jointWeightsElementSize)
        {
            //WM_reportf(RPT_WARNING, "%s: Joint weights and joint indices element size mismatch for prim %s", __func__, prim.GetPath().GetAsString().c_str());
            return;
        }

        pxr::VtIntArray joint_indices;
        joint_indices_primvar.ComputeFlattened(&joint_indices);

        pxr::VtFloatArray joint_weights;
        joint_weights_primvar.ComputeFlattened(&joint_weights);

        if (joint_indices.empty() || joint_weights.empty())
        {
            return;
        }

        if (joint_indices.size() != joint_weights.size())
        {
            return;
        }


        // const pxr::TfToken interpolation = jointWeightsPrimvar.GetInterpolation();
        // if (interpolation != pxr::UsdGeomTokens->constant)
        // {
        //
        // }

        for (uint32 vertexIndex = 0; vertexIndex < ; vertexIndex++)
        {
            for (uint32 j = 0; j < jointIndicesElementSize; j++)
            {
                const uint32 jointIndex = ;
                const float jointWeight = ;

                mesh->SetJointForVertex();
            }
        }
#endif
    }
}