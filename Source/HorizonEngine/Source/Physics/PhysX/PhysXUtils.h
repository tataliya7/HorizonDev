// #pragma once
//
// #include "Foundation/FoundationModule.h"
//
// #include <PxPhysicsAPI.h>
//
// namespace PhysXUtils
// {
//     inline physx::PxTransform ToPxTransform(const HE::Matrix4x4& transform)
//     {
//         HE::Quaternion quat = HE::Math::ConvertMatrix4x4ToQuaternion(transform);
//         physx::PxVec3 position(transform[3][0], transform[3][1], transform[3][2]);
//         physx::PxQuat rotation(quat.x, quat.y, quat.z, quat.w);
//         return physx::PxTransform(position, rotation);
//     }
//
//     inline physx::PxVec3 ToPxVec3(const HE::Vector3& v)
//     {
//         return physx::PxVec3(v.x, v.y, v.z);
//     }
//
//     inline physx::PxVec4 ToPxVec4(const HE::Vector4& v)
//     {
//         return physx::PxVec4(v.x, v.y, v.z, v.w);
//     }
//
//     inline physx::PxQuat ToPxQuat(const HE::Quaternion& q)
//     {
//         return physx::PxQuat(q.x, q.y, q.z, q.w);
//     }
//
//     inline HE::Matrix4x4 FromPxMat44(const physx::PxMat44& matrix)
//     {
//         return HE::Matrix4x4(
//             matrix.column0.x, matrix.column1.x, matrix.column2.x, matrix.column3.x,
//             matrix.column0.y, matrix.column1.y, matrix.column2.y, matrix.column3.y,
//             matrix.column0.z, matrix.column1.z, matrix.column2.z, matrix.column3.z,
//             matrix.column0.w, matrix.column1.w, matrix.column2.w, matrix.column3.w);
//     }
//
//     inline HE::Vector3 FromPxVec3(const physx::PxVec3& v)
//     {
//         return HE::Vector3(v.x, v.y, v.z);
//     }
//
//     inline HE::Vector4 FromPxVec4(const physx::PxVec4& v)
//     {
//         return HE::Vector4(v.x, v.y, v.z, v.w);
//     }
//
//     inline HE::Quaternion FromPxQuat(const physx::PxQuat& q)
//     {
//         return HE::Quaternion(q.x, q.y, q.z, q.w);
//     }
// }
