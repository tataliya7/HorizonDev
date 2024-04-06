#pragma once

#include "Core/CoreCommon.h"
#include "Core/Math/Math.h"
#include "Entity/EntityModule.h"

namespace HE
{
    struct Asset
    {
        std::string filename;
    };

    class AssetImporter
    {
    public:
        virtual void ImportAsset(const char* file, EntityHandle& targetEntity) = 0;
    };

    class AssetManager
    {
    public:
        static bool ImportAsset(const std::filesystem::path& filepath)
        {
            Asset* asset = new Asset();
            asset->filename = filepath.string();
            ImportedAssets[asset->filename] = std::shared_ptr<Asset>(asset);
            return true;
        }
        static void AddAsset(const std::string& path, Asset* asset)
        {
            ImportedAssets.emplace(path, asset);
        }
        template<typename T>
        static T* GetAsset(const std::string& assetHandle)
        {
            if (ImportedAssets.find(assetHandle) == ImportedAssets.end())
            {
                return nullptr;
            }
            return (T*)(ImportedAssets[assetHandle].get());
        }

        static std::unordered_map<std::string, std::shared_ptr<Asset>> ImportedAssets;
    };

    enum MaterialFlags
    {
        MATERIAL_FLAGS_NONE = 0x00000000,
        MATERIAL_FLAGS_USE_BASE_COLOR_MAP = 0x00000001,
        MATERIAL_FLAGS_USE_NORMAL_MAP = 0x00000002,
        MATERIAL_FLAGS_USE_METALLIC_ROUGHNESS_MAP = 0x00000004,
        MATERIAL_FLAGS_USE_EMISSIVE_MAP = 0x00000008,
        MATERIAL_FLAGS_USE_SPECULAR_MAP = 0x00000010,
        MATERIAL_FLAGS_SPECULAR_GLOSSINESS_WORKFLOW = 0x00000020,
        MATERIAL_FLAGS_MASK_ALL = 0xffffffff,
    };

    struct Bone
    {
        std::string name;
        int32 parentIndex = -1;
        std::vector<uint32> children;
        Matrix4x4 inverseBindMatrix = Matrix4x4(1);
        Matrix4x4 transform = Matrix4x4(1);
    };

    class Skeleton
    {
    public:
        Skeleton() = default;
        ~Skeleton() = default;

        const Bone* GetRoot() const { return &mBoneTree[0]; }

        Matrix4x4 inverseTransform;
        std::vector<Bone> mBoneTree;
        std::map<std::string, int32> boneMap;
    };

    struct BoneMotion
    {
        int32 translationTrackIndex = -1;
        int32 rotaionTrackIndex = -1;
        int32 scaleTrackIndex = -1;
    };

    struct Pose
    {
        uint32 boneCount = 0;
        std::vector<Matrix4x4> boneMatrices;
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
        std::vector<Vector3> translations;
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
        std::vector<Vector3> scales;
        InterpolationType interpolation;
    };

    class AnimationSequence
    {
    public:

        AnimationSequence()
            : mFrameCounter(0)
            , mSpeed(1.0f)
            , mCurrentTime(0)
            , mTimeLengthInSeconds(0)
            , mTargetSkeleton(nullptr)
        {

        }

        ~AnimationSequence() = default;

        std::vector<TranslationTrack> mTranslationTracks;
        std::vector<RotationTrack> mRotationTracks;
        std::vector<ScaleTrack> mScaleTracks;

        std::vector<BoneMotion> mBoneMotions;

        uint32 mFrameCounter;

        float mSpeed;
        float mCurrentTime;
        float mTimeLengthInSeconds;

        void SamplePose(Pose& outPose)
        {
            float targetFrameRate = 60.0f;

            // TODO
            {
                /*static auto startTime = std::chrono::high_resolution_clock::now();

                auto currentTime = std::chrono::high_resolution_clock::now();

                float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

                mCurrentTime = std::max(0.0f, time);*/

                mCurrentTime += 0.01f * mSpeed / targetFrameRate;
                if (mCurrentTime >= mTimeLengthInSeconds)
                {
                    mCurrentTime = 0;
                    // startTime = currentTime;
                }
            }

            const uint32 boneCount = (uint32)mTargetSkeleton->mBoneTree.size();
            assert(boneCount == (uint32)mBoneMotions.size());
            outPose.boneMatrices.resize(boneCount);

            for (uint32 boneIndex = 0; boneIndex < boneCount; boneIndex++)
            {
                auto& bone = mTargetSkeleton->mBoneTree[boneIndex];
                assert(bone.parentIndex < (int)boneIndex);

                Vector3 translation;
                Quaternion rotation;
                Vector3 scale;
                Sample_Internal(boneIndex, mCurrentTime, translation, rotation, scale);

                Matrix4x4 nodeTransform = glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(glm::normalize(rotation)) * glm::scale(glm::mat4(1.0f), scale);
                Matrix4x4 parentTransform = (bone.parentIndex == -1) ? Matrix4x4(1) : mTargetSkeleton->mBoneTree[bone.parentIndex].transform;

                bone.transform = parentTransform * nodeTransform;
                //outPose.boneMatrices[boneIndex] = mTargetSkeleton->inverseTransform * bone.transform * bone.inverseBindMatrix;

                outPose.boneMatrices[boneIndex] = bone.transform * bone.inverseBindMatrix;
            }
        }

        Skeleton* GetTargetSkeleton() const { return mTargetSkeleton; }

        // TODO: Should be private member.
        /// Each Animation Sequence targets a specific Skeleton and can only be played on that Skeleton.
        /// This means, in order to share animations between multiple Skeletal Meshes, each of the meshes must use the same Skeleton.
        Skeleton* mTargetSkeleton;

    private:
        void Sample_Internal(uint32 boneIndex, float time, Vector3& outTranslation, Quaternion& outRotation, Vector3& outScale)
        {
            if (mBoneMotions[boneIndex].translationTrackIndex >= 0)
            {
                SampleTranslation_Internal(mBoneMotions[boneIndex].translationTrackIndex, time, outTranslation);
            }
            else
            {
                outTranslation = Vector3(0.0f, 0.0f, 0.0f);
            }
            if (mBoneMotions[boneIndex].rotaionTrackIndex >= 0)
            {
                SampleRotation_Internal(mBoneMotions[boneIndex].rotaionTrackIndex, time, outRotation);
            }
            else
            {
                outRotation = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
            }
            if (mBoneMotions[boneIndex].scaleTrackIndex >= 0)
            {
                SampleScale_Internal(mBoneMotions[boneIndex].scaleTrackIndex, time, outScale);
            }
            else
            {
                outScale = Vector3(0.0f, 0.0f, 0.0f);
            }
        }

        void SampleTranslation_Internal(uint32 translationTrackIndex, float time, Vector3& outTranslation)
        {
            auto& translationTrack = mTranslationTracks[translationTrackIndex];
            if (time > translationTrack.times.back())
            {
                outTranslation = translationTrack.translations.back();
                return;
            }
            if (time < translationTrack.times.front())
            {
                outTranslation = translationTrack.translations.front();
                return;
            }
            if (translationTrack.times.size() == 1)
            {
                outTranslation = translationTrack.translations[0];
                return;
            }
            else
            {
                for (uint64 i = 0; i < translationTrack.times.size(); i++)
                {
                    if (time >= translationTrack.times[i] && time <= translationTrack.times[i + 1])
                    {
                        float ratio = (time - translationTrack.times[i]) / (translationTrack.times[i + 1] - translationTrack.times[i]);
                        outTranslation = glm::mix(translationTrack.translations[i], translationTrack.translations[i + 1], ratio);
                        return;
                    }
                }
            }
            outTranslation = Vector3(0, 0, 0);
        }
        void SampleRotation_Internal(uint32 rotationTrackIndex, float time, Quaternion& outRotation)
        {
            auto& rotationTrack = mRotationTracks[rotationTrackIndex];
            if (time > rotationTrack.times.back())
            {
                outRotation = rotationTrack.rotations.back();
                return;
            }
            if (time < rotationTrack.times.front())
            {
                outRotation = rotationTrack.rotations.front();
                return;
            }
            if (rotationTrack.times.size() == 1)
            {
                outRotation = rotationTrack.rotations[0];
                return;
            }
            else
            {
                for (uint64 i = 0; i < rotationTrack.times.size(); i++)
                {
                    if (time >= rotationTrack.times[i] && time <= rotationTrack.times[i + 1])
                    {
                        float ratio = (time - rotationTrack.times[i]) / (rotationTrack.times[i + 1] - rotationTrack.times[i]);
                        outRotation = glm::normalize(glm::lerp(rotationTrack.rotations[i], rotationTrack.rotations[i + 1], ratio));
                        return;
                    }
                }
            }
            outRotation = Quaternion(1, 0, 0, 0);
        }
        void SampleScale_Internal(uint32 scaleTrackIndex, float time, Vector3& outScale)
        {
            auto& scaleTrack = mScaleTracks[scaleTrackIndex];
            if (time > scaleTrack.times.back())
            {
                outScale = scaleTrack.scales.back();
                return;
            }
            if (time < scaleTrack.times.front())
            {
                outScale = scaleTrack.scales.front();
                return;
            }
            if (scaleTrack.times.size() == 1)
            {
                outScale = scaleTrack.scales[0];
                return;
            }
            else
            {
                for (uint64 i = 0; i < scaleTrack.times.size(); i++)
                {
                    if (time >= scaleTrack.times[i] && time <= scaleTrack.times[i + 1])
                    {
                        float ratio = (time - scaleTrack.times[i]) / (scaleTrack.times[i + 1] - scaleTrack.times[i]);
                        outScale = glm::mix(scaleTrack.scales[i], scaleTrack.scales[i + 1], ratio);
                        return;
                    }
                }
            }
            outScale = Vector3(1, 1, 1);
        };
    };
}
