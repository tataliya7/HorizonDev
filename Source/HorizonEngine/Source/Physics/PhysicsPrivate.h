#pragma once

#include "Core/CoreModule.h"

#include <PxPhysicsAPI.h>

#define PHYSX_PVD_HOST "127.0.0.1"

namespace Horizon
{
    extern physx::PxDefaultAllocator        GPhysXAllocator;
    extern physx::PxDefaultErrorCallback    GPhysXErrorCallback;
    extern physx::PxDefaultCpuDispatcher* GPhysXCpuDispatcher;
    extern physx::PxFoundation* GPhysXFoundation;
    extern physx::PxPvd* GPhysXPvd;
    extern physx::PxPhysics* GPhysXSDK;
}
