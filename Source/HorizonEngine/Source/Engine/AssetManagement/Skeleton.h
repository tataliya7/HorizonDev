#pragma once

#include "Foundation/FoundationModule.h"
#include "Rendering/RenderingModule.h"
//#include "Physics/PhysicsModule.h"
#include "Engine/EntityComponentSystem/EntityManager.h"
#include "Engine/Serialization/SerializationModule.h"

namespace Horizon
{
    class Joint
    {
    public:
        std::string name;
        int parentIndex;
        Matrix4x4f localBindTransform;
        Matrix4x4f restTransform;
    };

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
            return static_cast<uint32>(joints.size());
        }

    //private:

        std::string name;

        std::vector<Joint> joints;
    };

    enum class SkeletonAnimationInterpolationType
    {
        Linear,
        Step,
        CatmullRomSpline,
        CubicSpline,
    };

    struct SkeletonAnimationTranslationTrack
    {
        std::vector<float> times;
        std::vector<Vector3f> translations;
        //InterpolationType interpolation;
    };

    struct SkeletonAnimationRotationTrack
    {
        std::vector<float> times;
        std::vector<Quaternion> rotations;
        //InterpolationType interpolation;
    };

    struct SkeletonAnimationScaleTrack
    {
        std::vector<float> times;
        std::vector<Vector3f> scales;
        //InterpolationType interpolation;
    };

    struct SkeletonAnimationTrack
    {
        std::string name;

        uint32 jointIndex;

        SkeletonAnimationTranslationTrack translationTrack;

        SkeletonAnimationRotationTrack rotationTrack;

        SkeletonAnimationScaleTrack scaleTrack;
    };

    class SkeletonAnimation
    {
    public:

        struct Pose
        {
            struct JointTransformData
            {
                Vector3f translation;
                Quaternion rotation;
                Vector3f scale;
                Matrix4x4f localTransform;
                Matrix4x4f derivedLocalTransform = IdentityMatrix4x4f;
                Matrix4x4f bindTransform;
                Matrix4x4f jointTransform;
            };
            std::vector<JointTransformData> transformData;
        };

    public:

        SkeletonAnimation();

        ~SkeletonAnimation();

        /**
         * Whether to loop this animation by default.
         */
        bool loop;

    public:

        float speed;

        void SamplePose(Pose& outPose);

        Skeleton* GetTargetSkeleton() const
        {
            return targetSkeleton;
        }

        float GetDuration() const
        {
            return duration;
        }

        std::vector<SkeletonAnimationTrack> skeletonAnimationTracks;

    //private:

        /**
         * Each skeleton animation targets a specific skeleton and can only be played on that skeleton.
         * This means, to share animations between multiple skinned meshes, each of the meshes must use the same skeleton.
         */
        Skeleton* targetSkeleton;

        /*
         * The length of time (in seconds) that an animation takes to complete one cycle.
         */
        float duration;

        void Sample_Internal(uint32 boneIndex, float time, Vector3f& outTranslation, Quaternion& outRotation, Vector3f& outScale);

        void SampleTranslation_Internal(uint32 translationTrackIndex, float time, Vector3f& outTranslation);

        void SampleRotation_Internal(uint32 rotationTrackIndex, float time, Quaternion& outRotation);

        void SampleScale_Internal(uint32 scaleTrackIndex, float time, Vector3f& outScale);
    };
}