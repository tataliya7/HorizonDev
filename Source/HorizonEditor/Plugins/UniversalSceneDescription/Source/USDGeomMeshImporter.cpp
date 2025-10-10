#include "USDGeomMeshImporter.h"
#include "USDSkelSkeletonImporter.h"
#include "USDUtility.h"

#include "USDIncludeBegin.h"
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/types.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/subset.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdSkel/skeletonQuery.h>
#include <pxr/usd/usdSkel/bindingAPI.h>
#include <pxr/usd/usdSkel/animQuery.h>
#include <pxr/usd/usdSkel/cache.h>
#include <pxr/usd/usdSkel/utils.h>
#include "USDIncludeEnd.h"

namespace Horizon::USDImporter
{
    void ImportSkeletonBinding(const USDImportContext* context, const pxr::UsdPrim& prim, MeshComponent& mesh)
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

        // pxr::UsdSkelSkeleton skelSkeleton;
        // if (!skelBindingApi.GetSkeleton(&skelSkeleton))
        // {
        //     return;
        // }

        pxr::UsdSkelSkeleton usdSkelSkeleton = skelBindingApi.GetInheritedSkeleton();
        if (!usdSkelSkeleton)
        {
            return;
        }

        pxr::VtArray<pxr::TfToken> joints;
        if (skelBindingApi.GetJointsAttr().HasAuthoredValue())
        {
            skelBindingApi.GetJointsAttr().Get(&joints);
        }
        else if (usdSkelSkeleton.GetJointsAttr().HasAuthoredValue())
        {
            usdSkelSkeleton.GetJointsAttr().Get(&joints);
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

        int jointIndicesPrimvarElementSize = jointIndicesPrimvar.GetElementSize();
        int jointWeightsPrimvarElementSize = jointWeightsPrimvar.GetElementSize();

        if (jointIndicesPrimvarElementSize != jointWeightsPrimvarElementSize)
        {
            return;
        }

        mesh.relevantJointCountPerVertex = jointIndicesPrimvarElementSize;

        pxr::VtIntArray jointIndices;
        jointIndicesPrimvar.ComputeFlattened(&jointIndices);

        pxr::VtFloatArray jointWeights;
        jointWeightsPrimvar.ComputeFlattened(&jointWeights);

        if (jointIndices.empty() || jointWeights.empty())
        {
            return;
        }

        if (jointIndices.size() != jointWeights.size())
        {
            return;
        }

        // const pxr::TfToken interpolation = jointWeightsPrimvar.GetInterpolation();
        // if (interpolation != pxr::UsdGeomTokens->constant)
        // {
        //
        // }

        std::string skeletonPath = usdSkelSkeleton.GetPath().GetAsString();
        mesh.skeleton = context->skeletons.find(skeletonPath)->second;

        const uint32 jointCount = mesh.skeleton->joints.size();
        mesh.jointTransforms.resize(jointCount);
        for (uint32 jointIndex = 0; jointIndex < mesh.jointTransforms.size(); jointIndex++)
        {
            mesh.jointTransforms[jointIndex] = mesh.skeleton->joints[jointIndex].localBindTransform;
        }

        mesh.jointIndices.resize(mesh.vertexCount * jointIndicesPrimvarElementSize);
        mesh.jointWeights.resize(mesh.vertexCount * jointIndicesPrimvarElementSize);

        assert(jointIndices.size() == mesh.vertexCount * jointIndicesPrimvarElementSize);
        assert(jointWeights.size() == mesh.vertexCount * jointIndicesPrimvarElementSize);

        for (uint32 i = 0; i < mesh.jointIndices.size(); i++)
        {
            mesh.jointIndices[i] = -1;
            mesh.jointWeights[i] = 0.0f;
        }

        for (uint32 vertexIndex = 0; vertexIndex < mesh.vertexCount; vertexIndex++)
        {
            for (uint32 j = 0; j < static_cast<uint32>(jointIndicesPrimvarElementSize); j++)
            {
                const int32 jointIndex = jointIndices[vertexIndex * jointIndicesPrimvarElementSize + j];
                const float jointWeight = jointWeights[vertexIndex * jointIndicesPrimvarElementSize + j];

                mesh.jointIndices[vertexIndex * jointIndicesPrimvarElementSize + j] = jointIndex;
                mesh.jointWeights[vertexIndex * jointIndicesPrimvarElementSize + j] = jointWeight;
            }
        }

        pxr::UsdSkelCache usdSkelCache;
        const pxr::UsdSkelSkeletonQuery& usdSkelSkeletonQuery = usdSkelCache.GetSkelQuery(usdSkelSkeleton);
        if (!usdSkelSkeletonQuery.IsValid())
        {
            return;
        }

        const pxr::UsdSkelAnimQuery& usdSkelAnimQuery = usdSkelSkeletonQuery.GetAnimQuery();

        if (!usdSkelAnimQuery)
        {
            return;
        }

        std::vector<double> jointTransformTimeSamples;
        usdSkelAnimQuery.GetJointTransformTimeSamples(&jointTransformTimeSamples);

        if (jointTransformTimeSamples.empty())
        {
            return;
        }

        //pxr::VtTokenArray jointOrder = usdSkelAnimQuery.GetJointOrder();

        SkeletonAnimation* skeletonAnimation = new SkeletonAnimation();

        skeletonAnimation->skeletonAnimationTracks.resize(jointCount);

        float duration = 0.0f;

        for (double time : jointTransformTimeSamples)
        {
            pxr::VtMatrix4dArray usdJointLocalTransforms;
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

                auto& skeletonAnimationTracks = skeletonAnimation->skeletonAnimationTracks[jointIndex];

                skeletonAnimationTracks.translationTrack.times.push_back(static_cast<float>(time) / 30.0f);
                skeletonAnimationTracks.translationTrack.translations.push_back(Vector3f(translation[0], translation[1], translation[2]));

                skeletonAnimationTracks.rotationTrack.times.push_back(static_cast<float>(time) / 30.0f);
                skeletonAnimationTracks.rotationTrack.rotations.push_back(Quaternion(rotation.GetReal(), rotation.GetImaginary()[0], rotation.GetImaginary()[1], rotation.GetImaginary()[2]));

                skeletonAnimationTracks.scaleTrack.times.push_back(static_cast<float>(time) / 30.0f);
                skeletonAnimationTracks.scaleTrack.scales.push_back(Vector3f(scale[0], scale[1], scale[2]));

                duration = std::max(duration, static_cast<float>(time) / 30.0f);
            }
        }

        skeletonAnimation->duration = duration;
        skeletonAnimation->targetSkeleton = mesh.skeleton;

        mesh.skeletonAnimation = skeletonAnimation;
    }

    static pxr::UsdShadeMaterial ComputeBoundMaterial(const pxr::UsdPrim& prim)
    {
        pxr::UsdShadeMaterialBindingAPI bindingAPI = pxr::UsdShadeMaterialBindingAPI(prim);
        pxr::UsdShadeMaterial material = bindingAPI.ComputeBoundMaterial();
        if (!material)
        {
            material = bindingAPI.ComputeBoundMaterial(pxr::UsdShadeTokens->preview);
        }
        if (!material)
        {
            material = bindingAPI.ComputeBoundMaterial(pxr::UsdShadeTokens->full);
        }
        return material;
    }

    USDGeomMeshImporter::USDGeomMeshImporter(const USDImportContext& context, const pxr::UsdPrim& prim)
        : USDPrimImporter(context, prim)
        , geomMesh(prim)
        , hasUVs(false)
    {

    }

    void USDGeomMeshImporter::CreateEntity(Scene* scene)
    {
        entity = scene->CreateEntity(GetName());
    }

    void USDGeomMeshImporter::AddComponents(Scene* scene)
    {
        if (!geomMesh)
        {
            return;
        }

        if (parent != nullptr)
        {
            scene->SetParent(entity, parent->GetEntity());
        }

        float time = 0.0f;

        pxr::UsdGeomPrimvarsAPI usdGeomPrimvarsApi(geomMesh);

        pxr::VtVec3fArray positions;
        pxr::VtVec3fArray normals;
        pxr::VtVec2fArray uvs;
        pxr::TfToken uvInterpolationType;
        pxr::TfToken normalInterpolationType;

        uint64 faceCount = 0;

        pxr::UsdAttribute faceVertexCountsAttribute = geomMesh.GetFaceVertexCountsAttr();

        pxr::VtIntArray faceVertexCounts;
        if (faceVertexCountsAttribute)
        {
            faceVertexCountsAttribute.Get(&faceVertexCounts, time);
            faceCount = faceVertexCounts.size();
        }

        pxr::UsdAttribute faceVertexIndicesAttribute = geomMesh.GetFaceVertexIndicesAttr();

        pxr::VtIntArray faceVertexIndices;
        if (faceVertexIndicesAttribute)
        {
            faceVertexIndicesAttribute.Get(&faceVertexIndices, time);
        }

        if (faceVertexIndices.size() != 3 * faceVertexCounts.size())
        {
            printf("This mesh is not triangle list, skip!\n");
            return;
        }

        geomMesh.GetPointsAttr().Get(&positions, time);
        pxr::UsdGeomPrimvar primvar = usdGeomPrimvarsApi.GetPrimvar(UsdTokens::normals);
        if (primvar.HasValue())
        {
            primvar.ComputeFlattened(&normals, time);
            normalInterpolationType = primvar.GetInterpolation();
        }
        else
        {
            geomMesh.GetNormalsAttr().Get(&normals, time);
            normalInterpolationType = geomMesh.GetNormalsInterpolation();
        }

        std::vector<pxr::TfToken> uvTokens;
        std::vector<pxr::UsdGeomPrimvar> primvars = usdGeomPrimvarsApi.GetPrimvars();
        for (pxr::UsdGeomPrimvar p : primvars)
        {
            pxr::TfToken name = p.GetPrimvarName();
            pxr::SdfValueTypeName type = p.GetTypeName();

            bool isUV = false;
            if ((type == pxr::SdfValueTypeNames->TexCoord2hArray) ||
                (type == pxr::SdfValueTypeNames->TexCoord2fArray) ||
                (type == pxr::SdfValueTypeNames->TexCoord2dArray))
            {
                isUV = true;
            }
            else if ((name == UsdTokens::st) && (type == pxr::SdfValueTypeNames->Float2Array))
            {
                isUV = true;
            }

            if (isUV)
            {
                pxr::TfToken interpolationType = p.GetInterpolation();
                if ((interpolationType != pxr::UsdGeomTokens->faceVarying) && (interpolationType != pxr::UsdGeomTokens->vertex))
                {
                    continue;
                }
                uvTokens.push_back(p.GetBaseName());
                hasUVs = true;
            }
        }

        if (hasUVs)
        {
            // TODO: multi-layer UV
            pxr::TfToken uvToken = uvTokens.front();
            pxr::UsdGeomPrimvar uvPrimvar = usdGeomPrimvarsApi.GetPrimvar(uvToken);
            if (uvPrimvar)
            {
                uvPrimvar.ComputeFlattened(&uvs, time);
                uvInterpolationType = uvPrimvar.GetInterpolation();
            }
        }

        EntityManager* entityManager = scene->GetEntityManager();

        //assert(positions.size() == normals.size());
        //assert(positions.size() == uvs.size());

        const uint32 vertexCount = (uint32)positions.size(); // TODO
        const uint32 indexCount = (uint32)faceVertexIndices.size();

        printf("import mesh sdf path: %s, vertex count %d index count %d.\n", geomMesh.GetPath().GetString().c_str(), vertexCount, indexCount);

        TransformComponent& transformComponent = scene->GetEntityManager()->GetComponent<TransformComponent>(entity);
        transformComponent.scale = Vector3f(0.01f, 0.01f, 0.01f);

        MeshComponent& mesh = entityManager->AddComponent<MeshComponent>(entity);
        mesh.vertexCount = vertexCount;
        mesh.indexCount = indexCount;

        // Positions
        {
            mesh.positions.resize(positions.size());
            for (size_t i = 0; i < positions.size(); i++)
            {
                mesh.positions[i] = Vector3f(positions[i][0], positions[i][1], positions[i][2]);
            }
        }

        // Indices
        for (size_t faceIndex = 0; faceIndex < faceVertexCounts.size(); faceIndex++)
        {
            assert(faceVertexCounts[faceIndex] == 3);
            mesh.indices.push_back(faceVertexIndices[3 * faceIndex + 0]);
            mesh.indices.push_back(faceVertexIndices[3 * faceIndex + 1]);
            mesh.indices.push_back(faceVertexIndices[3 * faceIndex + 2]);
        }

        // Normals
        {
            if (normals.empty())
            {
                for (uint32 faceIndex = 0; faceIndex < faceCount; faceIndex++)
                {
                    normals.push_back(pxr::GfCross(positions[faceVertexIndices[3 * faceIndex + 2]] - positions[faceVertexIndices[3 * faceIndex + 0]], positions[faceVertexIndices[3 * faceIndex + 1]] - positions[faceVertexIndices[3 * faceIndex + 0]]));
                    normals.push_back(pxr::GfCross(positions[faceVertexIndices[3 * faceIndex + 2]] - positions[faceVertexIndices[3 * faceIndex + 1]], positions[faceVertexIndices[3 * faceIndex + 0]] - positions[faceVertexIndices[3 * faceIndex + 1]]));
                    normals.push_back(pxr::GfCross(positions[faceVertexIndices[3 * faceIndex + 0]] - positions[faceVertexIndices[3 * faceIndex + 2]], positions[faceVertexIndices[3 * faceIndex + 1]] - positions[faceVertexIndices[3 * faceIndex + 2]]));
                }

                normalInterpolationType = pxr::UsdGeomTokens->faceVarying;
            }

            if (normalInterpolationType == pxr::UsdGeomTokens->faceVarying)
            {
                assert(normals.size() == faceCount * 3);

                std::vector<uint32> relevantFaceCount(vertexCount);
                std::vector<Vector3f> vertexNormals(vertexCount);

                for (uint32 i = 0; i < vertexCount; i++)
                {
                    relevantFaceCount[i] = 0;
                    vertexNormals[i] = Vector3f(0.0f, 0.0f, 0.0f);
                }

                for (size_t faceIndex = 0; faceIndex < faceCount; faceIndex++)
                {
                    vertexNormals[faceVertexIndices[3 * faceIndex + 0]] += Vector3f(normals[3 * faceIndex + 0][0], normals[3 * faceIndex + 0][1], normals[3 * faceIndex + 0][2]);
                    vertexNormals[faceVertexIndices[3 * faceIndex + 1]] += Vector3f(normals[3 * faceIndex + 1][0], normals[3 * faceIndex + 1][1], normals[3 * faceIndex + 1][2]);
                    vertexNormals[faceVertexIndices[3 * faceIndex + 2]] += Vector3f(normals[3 * faceIndex + 2][0], normals[3 * faceIndex + 2][1], normals[3 * faceIndex + 2][2]);

                    relevantFaceCount[faceVertexIndices[3 * faceIndex + 0]] += 1;
                    relevantFaceCount[faceVertexIndices[3 * faceIndex + 1]] += 1;
                    relevantFaceCount[faceVertexIndices[3 * faceIndex + 2]] += 1;
                }

                for (size_t i = 0; i < vertexNormals.size(); i++)
                {
                    vertexNormals[i] /= static_cast<float>(relevantFaceCount[i]);
                    vertexNormals[i] = glm::normalize(vertexNormals[i]);
                }

                mesh.normals = vertexNormals;
            }
            else
            {
                mesh.normals.resize(normals.size());
                for (size_t i = 0; i < normals.size(); i++)
                {
                    mesh.normals[i] = Vector3f(normals[i][0], normals[i][1], normals[i][2]);
                }
            }
        }

        // UVs
        {
            if (uvInterpolationType == pxr::UsdGeomTokens->faceVarying)
            {
                assert(uvs.size() == faceCount * 3);

                std::vector<uint32> relevantFaceCount(vertexCount);
                std::vector<Vector2f> vertexUVs(vertexCount);

                for (uint32 i = 0; i < vertexCount; i++)
                {
                    relevantFaceCount[i] = 0;
                    vertexUVs[i] = Vector2f(0.0f, 0.0f);
                }

                for (size_t faceIndex = 0; faceIndex < faceCount; faceIndex++)
                {
                    vertexUVs[faceVertexIndices[3 * faceIndex + 0]] += Vector2f(uvs[3 * faceIndex + 0][0], uvs[3 * faceIndex + 0][1]);
                    vertexUVs[faceVertexIndices[3 * faceIndex + 1]] += Vector2f(uvs[3 * faceIndex + 1][0], uvs[3 * faceIndex + 1][1]);
                    vertexUVs[faceVertexIndices[3 * faceIndex + 2]] += Vector2f(uvs[3 * faceIndex + 2][0], uvs[3 * faceIndex + 2][1]);

                    relevantFaceCount[faceVertexIndices[3 * faceIndex + 0]] += 1;
                    relevantFaceCount[faceVertexIndices[3 * faceIndex + 1]] += 1;
                    relevantFaceCount[faceVertexIndices[3 * faceIndex + 2]] += 1;
                }

                for (size_t i = 0; i < vertexUVs.size(); i++)
                {
                    vertexUVs[i] /= static_cast<float>(relevantFaceCount[i]);
                }

                mesh.texCoords = vertexUVs;
            }
            else
            {
                mesh.texCoords.resize(uvs.size());
                for (size_t i = 0; i < uvs.size(); i++)
                {
                    mesh.texCoords[i] = Vector2f(uvs[i][0], uvs[i][1]);
                }
            }
        }

        mesh.tangents.resize(mesh.normals.size());
        for (size_t i = 0; i < mesh.tangents.size(); i++)
        {
            mesh.tangents[i] = Vector4f(1.0f, .0f, 0.0f, 1.0f);
        }

        if (hasUVs)
        {
            std::vector<Vector3f> tan1(vertexCount);
            std::vector<Vector3f> tan2(vertexCount);

            for (size_t i = 0; i < tan1.size(); i++)
            {
                tan1[i] = Vector3f(0.0f);
                tan2[i] = Vector3f(0.0f);
            }

            for (uint64 faceIndex = 0; faceIndex < faceCount; faceIndex++)
            {
                uint32 i0 = faceVertexIndices[3 * faceIndex + 0];
                uint32 i1 = faceVertexIndices[3 * faceIndex + 1];
                uint32 i2 = faceVertexIndices[3 * faceIndex + 2];

                Vector3f p0 = mesh.positions[i0];
                Vector3f p1 = mesh.positions[i1];
                Vector3f p2 = mesh.positions[i2];

                Vector3f v = p1 - p0;
                Vector3f w = p2 - p0;

                float sx = mesh.texCoords[i1].x - mesh.texCoords[i0].x;
                float sy = mesh.texCoords[i1].y - mesh.texCoords[i0].y;
                float tx = mesh.texCoords[i2].x - mesh.texCoords[i0].x;
                float ty = mesh.texCoords[i2].y - mesh.texCoords[i0].y;

                float dir = (tx * sy - ty * sx) < 0.0f ? -1.0f : 1.0f;

                if (std::abs(tx * sy - ty * sx) <= std::numeric_limits<float>::epsilon())
                {
                    sx = 0.0f;
                    sy = 1.0f;
                    tx = 1.0f;
                    ty = 0.0f;
                }

                Vector3f tangent, bitangent;
                tangent.x = (w.x * sy - v.x * ty) * dir;
                tangent.y = (w.y * sy - v.y * ty) * dir;
                tangent.z = (w.z * sy - v.z * ty) * dir;
                bitangent.x = (w.x * sx - v.x * tx) * dir;
                bitangent.y = (w.y * sx - v.y * tx) * dir;
                bitangent.z = (w.z * sx - v.z * tx) * dir;

                tan1[i0] += tangent;
                tan1[i1] += tangent;
                tan1[i2] += tangent;

                tan2[i0] += bitangent;
                tan2[i1] += bitangent;
                tan2[i2] += bitangent;
            }

            std::vector<Vector4f> vertexTangents(vertexCount);
            for (uint32 i = 0; i < vertexCount; i++)
            {
                const Vector3f& n = mesh.normals[i];
                const Vector3f& t = glm::normalize(tan1[i]);
                const Vector3f& b = glm::normalize(tan2[i]);

                // Gram-Schmidt orthogonalize
                Vector3f ttt = glm::normalize(t - n * glm::dot(n, t));
                vertexTangents[i].x = ttt.x;
                vertexTangents[i].y = ttt.y;
                vertexTangents[i].z = ttt.z;

                // Calculate handedness
                vertexTangents[i].w = (glm::dot(glm::cross(n, t), b) < 0.0f) ? -1.0f : 1.0f;
            }

            mesh.tangents = vertexTangents;
        }

        // Colors
        {

        }

        MeshComponent::MeshSubset& subset = mesh.subsets.emplace_back();
        subset.baseVertex = 0;
        subset.baseIndex = 0;
        subset.numIndices = mesh.indexCount;
        subset.numVertices = mesh.vertexCount;

        uint32 numFaces = (uint32)mesh.indices.size() / 3;
        mesh.materialIndices.resize(numFaces);

        uint32 numMaterials = 0;
        std::map<pxr::SdfPath, uint32> materialIndexMap;

        //pxr::UsdShadeMaterialBindingAPI usdShadeMaterialBindingAPI = pxr::UsdShadeMaterialBindingAPI(geomMesh);
        //std::vector<pxr::UsdGeomSubset> usdGeomSubsets = usdShadeMaterialBindingAPI.GetMaterialBindSubsets();
        std::vector<pxr::UsdGeomSubset> usdGeomSubsets = pxr::UsdGeomSubset::GetAllGeomSubsets(geomMesh);
        if (!usdGeomSubsets.empty())
        {
            for (const pxr::UsdGeomSubset& subset : usdGeomSubsets)
            {
                pxr::UsdShadeMaterial usdShadeMaterial = ComputeBoundMaterial(subset.GetPrim());
                if (!usdShadeMaterial)
                {
                    continue;
                }

                pxr::SdfPath materialSdfPath = usdShadeMaterial.GetPath();
                if (materialSdfPath.IsEmpty())
                {
                    continue;
                }

                printf("Meshsubset %s     MaterialSdfPath: %s\n", subset.GetPath().GetString().c_str(), materialSdfPath.GetString().c_str());

                if (materialIndexMap.find(materialSdfPath) == materialIndexMap.end())
                {
                    materialIndexMap[materialSdfPath] = numMaterials;
                    mesh.materials.emplace_back(context->materialMap.at(materialSdfPath.GetString()));
                    numMaterials++;
                }

                uint32 materialIndex = materialIndexMap[materialSdfPath];

                pxr::UsdAttribute indicesAttribute = subset.GetIndicesAttr();

                pxr::VtIntArray faceIndices;
                indicesAttribute.Get(&faceIndices, time);

                for (int faceIndex : faceIndices)
                {
                    mesh.materialIndices[faceIndex] = materialIndex;
                }
            }
        }

        ImportSkeletonBinding(context, prim, mesh);

        // TODO
        mesh.CreateRenderObject(scene->GetRenderScene());
    }
}