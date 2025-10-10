#include "Skeleton.h"

namespace Horizon
{
    Skeleton::Skeleton()
        : name("Unnamed")
    {

    }

    Skeleton::~Skeleton()
    {

    }

    SkeletonAnimation::SkeletonAnimation()
        : loop(false)
        , speed(1.0f)
		, duration(0)
		, targetSkeleton(nullptr)
	{

	}

	SkeletonAnimation::~SkeletonAnimation()
	{

	}

	void SkeletonAnimation::SamplePose(Pose& outPose)
	{
		// TODO
        float mCurrentTime = 0;
		{
			static auto startTime = std::chrono::high_resolution_clock::now();

			auto currentTime = std::chrono::high_resolution_clock::now();

			float time = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count()) / 1000.0f;

			mCurrentTime = time;

			if (mCurrentTime >= duration)
			{
				mCurrentTime = 0;
				startTime = currentTime;
			}
		}

		const uint32 boneCount = (uint32)targetSkeleton->joints.size();
		assert(boneCount == (uint32)skeletonAnimationTracks.size());

		outPose.transformData.resize(boneCount);

		for (uint32 boneIndex = 0; boneIndex < boneCount; boneIndex++)
		{
			auto& transformData = outPose.transformData[boneIndex];

			const Joint& joint = targetSkeleton->joints[boneIndex];
			assert(joint.parentIndex < (int)boneIndex);

			Sample_Internal(boneIndex, mCurrentTime, transformData.translation, transformData.rotation, transformData.scale);
			transformData.localTransform = Math::ComposeTransformationMatrix(transformData.translation, transformData.rotation, transformData.scale);

			if (joint.parentIndex != -1)
			{
				const auto& parentBone = outPose.transformData[joint.parentIndex];
				transformData.derivedLocalTransform = parentBone.derivedLocalTransform * transformData.localTransform;
			}
			else
			{
				transformData.derivedLocalTransform = transformData.localTransform;
			}

			transformData.bindTransform = joint.localBindTransform;
			transformData.jointTransform = transformData.derivedLocalTransform * glm::inverse(joint.localBindTransform);
		}
	}

	void SkeletonAnimation::Sample_Internal(uint32 boneIndex, float time, Vector3f& outTranslation, Quaternion& outRotaion, Vector3f& outScale)
	{
		SampleTranslation_Internal(boneIndex, time, outTranslation);
		SampleRotation_Internal(boneIndex, time, outRotaion);
		SampleScale_Internal(boneIndex, time, outScale);
	}

	void SkeletonAnimation::SampleTranslation_Internal(uint32 translationTrackIndex, float time, Vector3f& outTranslation)
	{
		auto& translationTrack = skeletonAnimationTracks[translationTrackIndex].translationTrack;

    	outTranslation = translationTrack.translations.front();
		if (time > translationTrack.times.back())
		{
			outTranslation = translationTrack.translations.back();
			return;
		}

		for (uint64 i = 0; i < translationTrack.times.size(); i++)
		{
			if (time >= translationTrack.times[i] && time <= translationTrack.times[i+1])
			{
				float ratio = (time - translationTrack.times[i]) / (translationTrack.times[i + 1] - translationTrack.times[i]);
				outTranslation = glm::mix(translationTrack.translations[i], translationTrack.translations[i + 1], ratio);
				return;
			}
		}
	}

	void SkeletonAnimation::SampleRotation_Internal(uint32 rotationTrackIndex, float time, Quaternion& outRotaion)
	{
        auto& rotationTrack = skeletonAnimationTracks[rotationTrackIndex].rotationTrack;

		outRotaion = rotationTrack.rotations.front();
		if (time > rotationTrack.times.back())
		{
			outRotaion = rotationTrack.rotations.back();
			return;
		}
		for (uint64 i = 0; i < rotationTrack.times.size(); i++)
		{
			if (time >= rotationTrack.times[i] && time <= rotationTrack.times[i + 1])
			{
				float ratio = (time - rotationTrack.times[i]) / (rotationTrack.times[i + 1] -  rotationTrack.times[i]);
				outRotaion = glm::normalize(glm::slerp(rotationTrack.rotations[i], rotationTrack.rotations[i + 1], ratio));
				return;
			}
		}
	}

	void SkeletonAnimation::SampleScale_Internal(uint32 scaleTrackIndex, float time, Vector3f& outScale)
	{
		auto& scaleTrack = skeletonAnimationTracks[scaleTrackIndex].scaleTrack;

    	outScale = scaleTrack.scales.front();
		if (time > scaleTrack.times.back())
		{
			outScale = scaleTrack.scales.back();
			return;
		}

		for (uint64 i = 0; i < scaleTrack.times.size(); i++)
		{
			if (time >= scaleTrack.times[i] && time <= scaleTrack.times[i + 1])
			{
				float ratio = (time - scaleTrack.times[i]) / (scaleTrack.times[i+1] - scaleTrack.times[i]);
				outScale = glm::mix(scaleTrack.scales[i], scaleTrack.scales[i + 1], ratio);
				return;
			}
		}
	}
}