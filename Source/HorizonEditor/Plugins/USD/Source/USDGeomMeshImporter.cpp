#include "USDGeomMeshImporter.h"
#include "USDSkelSkeletonImporter.h"
#include "USDUtils.h"

#include "USDIncludeBegin.h"
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/types.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/subset.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include "USDIncludeEnd.h"

namespace Horizon::USDImporter
{
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

        pxr::UsdGeomPrimvarsAPI primvarsAPI(geomMesh);

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
        pxr::UsdGeomPrimvar primvar = primvarsAPI.GetPrimvar(UsdTokens::normals);
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
        std::vector<pxr::UsdGeomPrimvar> primvars = primvarsAPI.GetPrimvars();
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
            pxr::UsdGeomPrimvar uvPrimvar = primvarsAPI.GetPrimvar(uvToken);
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

        MeshComponent& mesh = entityManager->AddComponent<MeshComponent>(entity);
        mesh.vertexCount = vertexCount;
        mesh.indexCount = indexCount;

        // Positions
        {
            mesh.positions.resize(positions.size());
            for (size_t i = 0; i < positions.size(); i++)
            {
                mesh.positions[i] = Vector3(positions[i][0], positions[i][1], positions[i][2]);
            }
        }

        // Normals
        {
            if (!normals.empty())
            {
                mesh.normals.resize(normals.size());
                for (size_t i = 0; i < normals.size(); i++)
                {
                    mesh.normals[i] = Vector3(normals[i][0], normals[i][1], normals[i][2]);
                }
            }
            else
            {
                for (uint32 faceIndex = 0; faceIndex < faceCount; faceIndex++)
                {
                    mesh.normals.push_back(glm::cross(mesh.positions[faceVertexIndices[3 * faceIndex + 2]] - mesh.positions[faceVertexIndices[3 * faceIndex + 0]], mesh.positions[faceVertexIndices[3 * faceIndex + 1]] - mesh.positions[faceVertexIndices[3 * faceIndex + 0]]));
                    mesh.normals.push_back(glm::cross(mesh.positions[faceVertexIndices[3 * faceIndex + 2]] - mesh.positions[faceVertexIndices[3 * faceIndex + 1]], mesh.positions[faceVertexIndices[3 * faceIndex + 0]] - mesh.positions[faceVertexIndices[3 * faceIndex + 1]]));
                    mesh.normals.push_back(glm::cross(mesh.positions[faceVertexIndices[3 * faceIndex + 0]] - mesh.positions[faceVertexIndices[3 * faceIndex + 2]], mesh.positions[faceVertexIndices[3 * faceIndex + 1]] - mesh.positions[faceVertexIndices[3 * faceIndex + 2]]));
                }
            }
        }

        // UVs
        {
            mesh.texCoords.resize(uvs.size());
            for (size_t i = 0; i < uvs.size(); i++)
            {
                mesh.texCoords[i] = Vector2(uvs[i][0], uvs[i][1]);
            }

            for (size_t faceIndex = 0; faceIndex < faceVertexCounts.size(); faceIndex++)
            {
                assert(faceVertexCounts[faceIndex] == 3);
                mesh.indices.push_back(faceVertexIndices[3 * faceIndex + 0]);
                mesh.indices.push_back(faceVertexIndices[3 * faceIndex + 1]);
                mesh.indices.push_back(faceVertexIndices[3 * faceIndex + 2]);
            }
        }

        // Colors
        {

        }

        // TODO: support tangents
        mesh.tangents.resize(mesh.normals.size());
        for (size_t i = 0; i < mesh.normals.size(); i++)
        {
            mesh.tangents[i] = Vector4(0, 0, 0, 0);
        }

        RenderSystem* renderSystem = HorizonEngine::GetInstance()->GetSubsystem<RenderSystem>();
        RenderBackend* renderBackend = renderSystem->GetRenderBackend();

        RenderBackendBufferDesc vertexBuffer0Desc = RenderBackendBufferDesc::CreateByteAddress(mesh.vertexCount * sizeof(Vector3));
        vertexBuffer0Desc.flags |= RenderBackendBufferCreateFlags::RayTracingAccelerationStructure; // TODO
        mesh.vertexBuffers[0] = renderBackend->CreateBuffer(&vertexBuffer0Desc, mesh.positions.data(), "VertexPosition");

        RenderBackendBufferDesc vertexBuffer1Desc = RenderBackendBufferDesc::CreateByteAddress(mesh.normals.size() * sizeof(Vector3));
        mesh.vertexBuffers[1] = renderBackend->CreateBuffer(&vertexBuffer1Desc, mesh.normals.data(), "VertexNormal");

        RenderBackendBufferDesc vertexBuffer2Desc = RenderBackendBufferDesc::CreateByteAddress(mesh.tangents.size() * sizeof(Vector4));
        mesh.vertexBuffers[2] = renderBackend->CreateBuffer(&vertexBuffer2Desc, mesh.tangents.data(), "VertexTangent");

        if (!mesh.texCoords.empty())
        {
            RenderBackendBufferDesc vertexBuffer3Desc = RenderBackendBufferDesc::CreateByteAddress(mesh.texCoords.size() * sizeof(Vector2));
            mesh.vertexBuffers[3] = renderBackend->CreateBuffer(&vertexBuffer3Desc, mesh.texCoords.data(), "VertexTextureCoord0");
        }

        if (mesh.indexCount > 0)
        {
            RenderBackendBufferDesc indexBufferDesc = RenderBackendBufferDesc::CreateIndex(sizeof(uint32), mesh.indexCount);
            indexBufferDesc.flags |= RenderBackendBufferCreateFlags::RayTracingAccelerationStructure; // TODO
            mesh.indexBuffer = renderBackend->CreateBuffer(&indexBufferDesc, mesh.indices.data(), "IndexBuffer");
        }

        MeshComponent::MeshSubset& subset = mesh.subsets.emplace_back();
        subset.baseVertex = 0;
        subset.baseIndex = 0;
        subset.numIndices = mesh.indexCount;
        subset.numVertices = mesh.vertexCount;

        TransformComponent& transformComponent = scene->GetEntityManager()->GetComponent<TransformComponent>(entity);

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

        for (Material& material : mesh.materials)
        {
            MaterialShaderParameters materialShaderParameters;
            materialShaderParameters.baseColor = material.baseColor;
            materialShaderParameters.metallic = material.metallic;
            materialShaderParameters.roughness = material.roughness;
            materialShaderParameters.specular = material.specular;
            materialShaderParameters.specularTint = material.specularTint;
            materialShaderParameters.emission = material.emission;
            materialShaderParameters.emissionStrength = material.emissionStrength;
            materialShaderParameters.sssSurfaceAlbedo = material.sssSurfaceAlbedo;
            materialShaderParameters.sssMFP = material.sssSurfaceAlbedo;
            materialShaderParameters.secondRoughness = material.secondRoughness;
            materialShaderParameters.lobeMix = material.lobeMix;
            materialShaderParameters.flags = 0;
            if (material.useMetallicRoughnessWorkflow)
            {
                materialShaderParameters.flags |= MATERIAL_FLAGS_BIT_USE_METALLIC_ROUGHNESS_WORKFLOW;
            }
            for (uint32 slot = 0; slot < RendererMaxMaterialTextureSlotCount; slot++)
            {
                if (material.textures[slot].used)
                {
                    materialShaderParameters.textures[slot].bindlessTextureIndex = renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(material.textures[slot].gpuTexture);
                }
            }
            mesh.materialData.emplace_back(materialShaderParameters);
        }

        RenderBackendBufferDesc materialBufferDesc = RenderBackendBufferDesc::CreateByteAddress(mesh.materialData.size() * sizeof(MaterialShaderParameters));
        mesh.materialBuffer = renderBackend->CreateBuffer(&materialBufferDesc, mesh.materialData.data(), "MaterialBuffer");

        RenderBackendBufferDesc materialIndexBufferDesc = RenderBackendBufferDesc::CreateByteAddress((mesh.indexCount / 3) * sizeof(uint32));
        mesh.materialIndexBuffer = renderBackend->CreateBuffer(&materialIndexBufferDesc, mesh.materialIndices.data(), "MaterialIndexBuffer");

        // TODO
        mesh.CreateRenderObject(scene->GetRenderScene());

        ImportSkeletonBinding(prim);
    }
}