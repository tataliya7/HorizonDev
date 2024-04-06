#include "USDSkelSkeletonImporter.h"

#include "USDIncludeBegin.h"
#include <pxr/usd/usdSkel/skeletonQuery.h>
#include <pxr/usd/usdSkel/animation.h>
#include <pxr/usd/usdSkel/bindingAPI.h>
#include <pxr/usd/usdSkel/blendShape.h>
#include <pxr/usd/usdSkel/cache.h>
#include <pxr/usd/usdSkel/utils.h>
#include "USDIncludeEnd.h"

namespace HE::USDImporter
{
    void ImportSkeletonBinding(const pxr::UsdPrim& prim)
    {
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

        pxr::UsdGeomPrimvar joint_indices_primvar = skelBindingApi.GetJointIndicesPrimvar();
        if (!(joint_indices_primvar && joint_indices_primvar.HasAuthoredValue()))
        {
            return;
        }

        pxr::UsdGeomPrimvar joint_weights_primvar = skelBindingApi.GetJointWeightsPrimvar();
        if (!(joint_weights_primvar && joint_weights_primvar.HasAuthoredValue()))
        {
            return;
        }

        int joint_indices_elem_size = joint_indices_primvar.GetElementSize();
        int joint_weights_elem_size = joint_weights_primvar.GetElementSize();

        if (joint_indices_elem_size != joint_weights_elem_size)
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
    }
}