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
        Matrix4x4f bindTransform;
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
                Matrix4x4f localToWorldMatrix;
                Matrix4x4f localTransform;
                Matrix4x4f derivedLocalTransform;
            };
            std::vector<JointTransformData> transformData;
        };

        enum class InterpolationType
        {
            Linear,
            Step,
            CatmullRomSpline,
            CubicSpline,
        };

        struct TranslationTrack
        {
            std::vector<float> times;
            std::vector<Vector3f> translations;
            InterpolationType interpolation;
        };

        struct RotationTrack
        {
            std::vector<float> times;
            std::vector<Quaternion> rotations;
            InterpolationType interpolation;
        };

        struct ScaleTrack
        {
            std::vector<float> times;
            std::vector<Vector3f> scales;
            InterpolationType interpolation;
        };

        struct BoneMotion
        {
            int32 translationTrackIndex = -1;
            int32 rotationTrackIndex = -1;
            int32 scaleTrackIndex = -1;
        };

    public:

        SkeletonAnimation();
        ~SkeletonAnimation();

        std::vector<TranslationTrack> mTranslationTracks;
        std::vector<RotationTrack> mRotationTracks;
        std::vector<ScaleTrack> mScaleTracks;

        std::vector<BoneMotion> mBoneMotions;

        uint32 mFrameCounter;

        float mTimeLengthInSeconds;
        float mSpeed;
        float mCurrentTime;

        void SamplePose(Pose& outPose);

        Skeleton* GetTargetSkeleton() const { return mTargetSkeleton; }

        /**
         * Each skeleton animation clip targets a specific skeleton and can only be played on that skeleton.
         * This means, in order to share animations between multiple skinned meshes, each of the meshes must use the same skeleton.
         */
        Skeleton* mTargetSkeleton;

    private:
        void Sample_Internal(uint32 boneIndex, float time, Vector3f& outTranslation, Quaternion& outRotation, Vector3f& outScale);
        void SampleTranslation_Internal(uint32 translationTrackIndex, float time, Vector3f& outTranslation);
        void SampleRotation_Internal(uint32 rotationTrackIndex, float time, Quaternion& outRotation);
        void SampleScale_Internal(uint32 scaleTrackIndex, float time, Vector3f& outScale);
    };
}