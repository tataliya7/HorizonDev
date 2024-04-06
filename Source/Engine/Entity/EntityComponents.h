#pragma once

#include "Core/CoreModule.h"
#include "RenderBackend/RenderBackendModule.h"
#include "Entity/EntityManager.h"

#include <numbers>

namespace HE
{
    struct NameComponent
    {
        std::string name;
        NameComponent() = default;
        NameComponent(const NameComponent& other) = default;
        NameComponent(const std::string& name) : name(name) {}
        inline operator std::string& () { return name; }
        inline operator const std::string& () const { return name; }
        inline void operator=(const std::string& str) { name = str; }
        inline bool operator==(const std::string& str) const { return name.compare(str) == 0; }
    };

    struct TransformComponent
    {
    public:
        TransformComponent()
            : position(0.0f, 0.0f, 0.0f)
            , rotation(0.0f, 0.0f, 0.0f)
            , scale(1.0f, 1.0f, 1.0f)
            , matrix(Matrix4x4(1.0f)) {}

        Vector3 position;
        Vector3 rotation;
        Vector3 scale;
        Matrix4x4 matrix;

        void Update()
        {
            matrix = Math::Compose(position, Quaternion(Math::DegreesToRadians(rotation)), scale);
        }

        bool operator==(const TransformComponent& other) const
        {
            if (position != other.position)
            {
                return false;
            }
            if (rotation != other.rotation)
            {
                return false;
            }
            if (scale != other.scale)
            {
                return false;
            }
            return true;
        }

        bool operator!=(const TransformComponent& other) const
        {
            return !((*this) == other);
        }
    };

    struct TransformDirtyComponent
    {

    };

    struct SceneHierarchyComponent
    {
        uint32 depth;
        uint32 numChildren;
        EntityHandle parent;
        EntityHandle firstChild;
        EntityHandle next;
        EntityHandle prev;

        SceneHierarchyComponent()
            : depth(0)
            , numChildren(0)
            , parent()
            , firstChild()
            , next()
            , prev()
        {

        }
    };

    struct Frustum
    {
        Vector4 planes[6];
    };

    enum class CameraProjectionMode
    {
        Perspective,
        Orthographic,
    };

    struct CameraComponent
    {
        CameraProjectionMode projectionMode;
        float nearClippingPlane;
        float farClippingPlane;
        float fieldOfView;

        float aspectRatio;
        bool overrideAspectRatio;

        PostProcessingSettings postProcessingSettings;

        // Non-serialized
        Vector3 position;
        Quaternion rotation;
        Vector3 forwardVec;
        Vector3 rightVec;
        Vector3 upVec;
        Matrix4x4 viewMatrix;
        Matrix4x4 invViewMatrix;
        Matrix4x4 projectionMatrix;
        Matrix4x4 invProjectionMatrix;
        Frustum frustum;

        void Update()
        {
            static Quaternion zUpQuat = glm::rotate(Quaternion(), Math::DegreesToRadians(90.0), Vector3(1.0, 0.0, 0.0));

            invViewMatrix = Math::Compose(position, rotation * zUpQuat, Vector3(1.0f, 1.0f, 1.0f));
            viewMatrix = Math::Inverse(invViewMatrix);
            projectionMatrix = Math::PerspectiveReverseZ_RH_ZO(Math::DegreesToRadians(fieldOfView), aspectRatio, nearClippingPlane, farClippingPlane);
            invProjectionMatrix = Math::Inverse(projectionMatrix);

            //rightVec    = Math::Normalize(Vector3(1.0f, 0.0f, 0.0f) * rotation);
            //forwardVec  = Math::Normalize(Vector3(0.0f, 1.0f, 0.0f) * rotation);
            //upVec       = Math::Normalize(Vector3(0.0f, 0.0f, 1.0f) * rotation);

            rightVec   = Math::Normalize(rotation * Vector3(1.0f, 0.0f, 0.0f));
            forwardVec = Math::Normalize(rotation * Vector3(0.0f, 1.0f, 0.0f));
            upVec      = Math::Normalize(rotation * Vector3(0.0f, 0.0f, 1.0f));

            // Update frustum
            {
                // Calculate camera far plane corners
                float distance = farClippingPlane;
                float halfFovRad = Math::DegreesToRadians(fieldOfView) * 0.5f;
                float uLen = distance * Math::Tan(halfFovRad);
                float rLen = uLen * aspectRatio;
                Vector3 farCenterPoint = position + distance * forwardVec;
                Vector3 u = uLen * upVec;
                Vector3 r = rLen * rightVec;

                Vector3 corners[4];
                corners[0] = farCenterPoint - u - r; // left-bottom
                corners[1] = farCenterPoint - u + r; // right-bottom
                corners[2] = farCenterPoint + u - r; // left-up
                corners[3] = farCenterPoint + u + r; // right-up

                frustum.planes[0] = Math::GetPlane(position, corners[0], corners[2]); // left
                frustum.planes[1] = Math::GetPlane(position, corners[3], corners[1]); // right
                frustum.planes[2] = Math::GetPlane(position, corners[1], corners[0]); // bottom
                frustum.planes[3] = Math::GetPlane(position, corners[2], corners[3]); // up
                frustum.planes[4] = Math::GetPlane(-forwardVec, position + forwardVec * nearClippingPlane); // near
                frustum.planes[5] = Math::GetPlane(forwardVec, position + forwardVec * farClippingPlane);  // far
            }
        }
    };

    struct LightComponent
    {
        enum class LightType
        {
            Directional = 0,
            Point = 1,
            Spot = 2,
            Area = 3,
        };

        LightType type = LightType::Directional;

        // Common
        bool useColorTemperature = false;
        float colorTemperature = 6500.0f;
        Vector3 color = Vector3(1.0f, 1.0f, 1.0f);
        float luminousIntensity = 1.0f;

        float radius;
        /** Apex angle in degree. */
        float apexAngle = 0.545f;
        bool useRayTracingShadows = false;
        bool castShadows = false;

        // Cascade Shadow Maps
        int32 numShadowCascades = 3;
        float cascadeSplitLambda = 0.6f;
        float maxShadowDistance = 100.0f;
        float shadowMapDepthBiasConstantFactor = -1.0f;
        float shadowMapDepthBiasSlopeFactor = -5.0f;
        float shadowSharpeness;

        // Atmosphere
        Vector3 atmosphereLightDiskColorTint = Vector3(1.0f, 1.0f, 1.0f);

        float GetMaxShadowDistance() const
        {
            return maxShadowDistance;
        }

        // Punctual Light

        // Area Light

        static Vector3 GetLinearColorFromColorTemperature(float colorTemperature)
        {
            colorTemperature = std::clamp(colorTemperature, 1000.0f, 15000.0f);

            // Approximate Planckian locus in CIE 1960 UCS
            float u = (0.860117757f + 1.54118254e-4f * colorTemperature + 1.28641212e-7f * colorTemperature * colorTemperature) / (1.0f + 8.42420235e-4f * colorTemperature + 7.08145163e-7f * colorTemperature * colorTemperature);
            float v = (0.317398726f + 4.22806245e-5f * colorTemperature + 4.20481691e-8f * colorTemperature * colorTemperature) / (1.0f - 2.89741816e-5f * colorTemperature + 1.61456053e-7f * colorTemperature * colorTemperature);

            float x = 3.0f * u / (2.0f * u - 8.0f * v + 4.0f);
            float y = 2.0f * v / (2.0f * u - 8.0f * v + 4.0f);
            float z = 1.0f - x - y;

            float Y = 1.0f;
            float X = Y / y * x;
            float Z = Y / y * z;

            // XYZ to RGB with BT.709 primaries
            float R =  3.2404542f * X + -1.5371385f * Y + -0.4985314f * Z;
            float G = -0.9692660f * X +  1.8760108f * Y +  0.0415560f * Z;
            float B =  0.0556434f * X + -0.2040259f * Y +  1.0572252f * Z;

            return Vector3(R, G, B);
        }

        Vector3 GetPhysicalLightColor() const
        {
            Vector3 result = color * luminousIntensity;
            if (useColorTemperature)
            {
                result *= GetLinearColorFromColorTemperature(colorTemperature);
            }
            return result;
        }

        float GetSunLightHalfApexAngleRadian() const
        {
            return 0.5f * apexAngle * std::numbers::pi_v<float> / 180.0f;
        }

        bool CastShadows() const
        {
            return castShadows;
        }

        bool UseRayTracingShadows() const
        {
            return useRayTracingShadows;
        }

        uint32 GetNumDynamicShadowCascades() const
        {
            return numShadowCascades;
        }

        float GetCascadeSplitLambda() const
        {
            return cascadeSplitLambda;
        }

        Vector3 GetDirection() const
        {
            return forwardVec;
        }

        // Non-serialized
        Vector3 position;
        Vector3 forwardVec;
        Vector3 rightVec;
        Vector3 upVec;
    };

    struct EnvironmentLightComponent
    {
        EnvironmentLightComponent()
        {

        }

        std::string cubemap;
        uint32 cubemapResolution;

        const std::string& GetCubemap() const
        {
            return cubemap;
        }

        uint32 GetCubemapResolution() const
        {
            return cubemapResolution;
        }

        void SetCubemap(std::string newCubemap)
        {
            if (cubemap != newCubemap)
            {
                cubemap = newCubemap;
                SetDirty(true);
            }
        }

        void SetDirty(bool value)
        {
            dirty = value;
        }

        bool IsDirty()
        {
            return dirty;
        }

        RenderBackendTextureHandle GetEnvironmentMap() const
        {
            return environmentMap;
        }

        RenderBackendTextureHandle GetIrradianceEnvironmentMap() const
        {
            return irradianceEnvironmentMap;
        }

        RenderBackendBufferHandle GetIrradianceEnvironmentMapSH() const
        {
            return irradianceEnvironmentMapSH;
        }

        RenderBackendTextureHandle GetFilteredEnvironmentMap() const
        {
            return filteredEnvironmentMap;
        }

        bool dirty;

        RenderBackendTextureHandle environmentMap;
        RenderBackendTextureHandle irradianceEnvironmentMap;
        RenderBackendBufferHandle irradianceEnvironmentMapSH;
        RenderBackendTextureHandle filteredEnvironmentMap;
    };

    struct Material
    {
        std::string name;

        // BxDF
        Vector4 baseColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        float metallic = 0.0f;
        float roughness = 0.5f;
        float specular = 0.5f;
        float specularTint = 0.0f;
        float transmission = 0.0f;
        float transmissionRoughness = 0.0f;
        float clearcoat = 0.0f;
        float clearcoatRoughness = 0.0f;
        Vector4 emission = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
        float emissionStrength = 1.0f;
        float alpha = 1.0f;

        // SSS
        Vector4 sssSurfaceAlbedo = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        Vector4 sssMFP = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        float secondRoughness = 0.5f;
        float lobeMix = 0.0f;

        enum TextureSlot
        {
            BaseColorMap,
            MetallicRoughnessMap,
            SpecularGlossinessMap,
            NormalMap,
            EmissiveMap,
            Count
        };

        struct TextureMap
        {
            std::string path;
            bool used = false;
            RenderBackendTextureHandle gpuTexture;
        };
        TextureMap textures[16];
        bool useMetallicRoughnessWorkflow = false;
    };

    enum class RayTracingGeometryState
    {
        Invalid = 0,
        BuildRequired = 1,
        UpdateRequired = 2,
        UpToDate = 3,
    };

    struct RayTracingGeometry
    {
        RayTracingGeometryState state;
        RenderBackendRayTracingAccelerationStructureHandle blas;

        bool IsUpToDate() const
        {
            return state == RayTracingGeometryState::UpToDate;
        }
    };

    struct MeshComponent
    {
        std::string meshSource;

        uint32 numVertices;
        uint32 numIndices;

        std::vector<Vector3> positions;
        std::vector<Vector3> normals;
        std::vector<Vector4> tangents;
        std::vector<Vector2> texCoords;
        std::vector<uint32> indices;
        std::vector<uint32> materialIndices;
        std::vector<uint32> boneIndices;
        std::vector<float> boneWeights;

        struct MeshSubset
        {
            std::string name;
            uint32 baseVertex;
            uint32 baseIndex;
            uint32 numIndices;
            uint32 numVertices;
            //uint32 materialIndex;
            //uint32 transformIndex;
            Vector3 boundsMin;
            Vector3 boundsMax;
        };
        std::vector<MeshSubset> subsets;
        //std::vector<Matrix4x4> transformData;
        //std::vector<Matrix4x4> transformDataTranspose;

        std::vector<Material> materials;

        inline uint32 GetSubsetCount() const
        {
            return (uint32)subsets.size();
        }

        inline uint32 GetMaterialCount() const
        {
            return (uint32)materials.size();
        }

        Vector3 boundsMin;
        Vector3 boundsMax;

        EntityHandle armature = EntityHandle::Null;

        RenderBackendBufferHandle vertexBuffers[4];
        RenderBackendBufferHandle indexBuffer;
        RenderBackendBufferHandle materialIndexBuffer;

        int materialBufferOffset = 0;

        //RenderBackendBufferHandle transformBuffer;
        //RenderBackendBufferHandle transformTransposeBuffer;
        //RenderBackendBufferHandle previousTransformBuffer;

        RenderBackendBufferHandle boneIndexBuffer;
        RenderBackendBufferHandle boneWeightBuffer;
        RenderBackendBufferHandle boneTransformBuffer;

        RayTracingGeometry rayTracingGeometry;

        int64 updateCounter = -100;
        int previousTransformIndex = -1;

        bool doubleSided = true;

        inline bool IsSkinnedMesh() const
        {
            return armature != EntityHandle::Null;
        }
    };

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

    struct AudioListenerComponent
    {
        AudioListenerComponent() = default;
        AudioListenerComponent(const AudioListenerComponent& other) = default;
    };

    struct RigidBodyComponent
    {
        enum class Type
        {
            Static = 0,
            Dynamic = 1,
        };

        enum class CollisionShape
        {
            Box = 0,
            Sphere = 1,
            Capsule = 2,
            Mesh = 3,
        };

        struct BoxCollider
        {
            Vector3 halfExtent = Vector3(0.5f, 0.5f, 0.5f);
            Vector3 offset = Vector3(0.0f, 0.0f, 0.0f);
        };

        struct SphereCollider
        {
            float radius = 1.0f;
        };

        struct CapsuleCollider
        {
            float radius = 1.0f;
            float height = 1.0f;
        };

        Type type = Type::Static;
        CollisionShape shape = CollisionShape::Mesh;

        BoxCollider boxCollider;
        SphereCollider sphereCollider;
        CapsuleCollider capsuleCollider;

        bool disableGravity = false;
        bool isKinematic = false;

        float mass = 1.0f;
        float linearDamping = 0.01f;
        float angularDamping = 0.05f;

        void SetBoxCollider(const Vector3& halfExtent, const Vector3& offset)
        {
            shape = CollisionShape::Box;
            boxCollider.halfExtent = halfExtent;
            boxCollider.offset = offset;
        }

        void SetSphereCollider(float radius)
        {
            shape = CollisionShape::Sphere;
            sphereCollider.radius = radius;
        }

        void SetCapsuleCollider(float radius, float height)
        {
            shape = CollisionShape::Capsule;
            capsuleCollider.radius = radius;
            capsuleCollider.height = height;
        }

        void SetMeshCollider()
        {
            shape = CollisionShape::Mesh;
        }

        RigidBodyComponent() = default;
        RigidBodyComponent(const RigidBodyComponent& other) = default;
    };

    enum class AudioSourceComponentState : uint8
    {
        Playing,
        Stopped,
        Paused,
        FadingIn,
        FadingOut,
        Count
    };


    struct AudioSourceComponent
    {
        std::string audio;

        float volumeMultiplier = 1.0f;

        AudioSourceComponent() = default;
        AudioSourceComponent(const AudioSourceComponent& other) = default;

        //AudioSourceComponentState GetState() const
        //{
        //    if (!IsActive())
        //    {
        //        return EAudioComponentPlayState::Stopped;
        //    }

        //    if (bIsPaused)
        //    {
        //        return EAudioComponentPlayState::Paused;
        //    }

        //    if (bIsFadingOut)
        //    {
        //        return EAudioComponentPlayState::FadingOut;
        //    }

        //    // Get the current audio time seconds and compare when it started and the fade in duration
        //    float CurrentAudioTimeSeconds = GetAudioTimeSeconds();
        //    if (CurrentAudioTimeSeconds - TimeAudioComponentPlayed < FadeInTimeDuration)
        //    {
        //        return EAudioComponentPlayState::FadingIn;
        //    }

        //    // If we are not in any of the above states we are "playing"
        //    return EAudioComponentPlayState::Playing;
        //}

        //void AdjustVolume(float AdjustVolumeDuration, float AdjustVolumeLevel, const EAudioFaderCurve FadeCurve = EAudioFaderCurve::Linear)
        //{

        //}

        void SetPitchMultiplier(float NewPitchMultiplier)
        {

        }

        void Play()
        {
            // Audio::PlaySoundEX(audio.c_str());
        }
    };

    struct AnimationComponent
    {

    };

    struct InverseKinematicsComponent
    {

    };

    class Scriptable
    {
    public:
        Scriptable() {}
        virtual ~Scriptable() {}
        template<typename Component>
        Component& GetComponent() const
        {
            return manager->GetComponent<Component>(entity);
        }
        template<typename Component>
        Component* TryGetComponent()
        {
            return manager->TryGetComponent<Component>(entity);
        }
        EntityManager* manager;
        EntityHandle entity;
    };

    struct ScriptComponent
    {
        Scriptable* scriptable = nullptr;

        std::function<void()> ConstructorFunc;
        std::function<void()> DeonstructorFunc;

        std::function<void(Scriptable*)> OnCreateFunc;
        std::function<void(Scriptable*)> OnDestroyFunc;
        std::function<void(Scriptable*, float)> OnUpdateFunc;

        ScriptComponent() = default;

        template<typename T>
        void Bind()
        {
            ConstructorFunc = [&]() { scriptable = new T(); };
            DeonstructorFunc = [&]() { if (scriptable) { delete ((T*)scriptable); scriptable = nullptr; } };

            OnCreateFunc = [&](Scriptable* scriptable) { ((T*)scriptable)->OnCreate(); };
            OnDestroyFunc = [&](Scriptable* scriptable) { ((T*)scriptable)->OnDestroy(); };
            OnUpdateFunc = [&](Scriptable* scriptable, float deltaTime) { ((T*)scriptable)->OnUpdate(deltaTime); };
        }
    };
}
