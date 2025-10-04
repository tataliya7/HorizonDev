#include "MeshComponent.h"

// @todo Remove these
#include "Engine/HorizonEngineModule.h"
#include "Engine/VirtualGeometry/VirtualGeometry.h"

namespace Horizon
{
    MeshComponent::MeshComponent()
    {

    }

    MeshComponent::~MeshComponent()
    {

    }

    bool MeshComponent::IsRenderObjectValid() const
    {
        return renderObject != nullptr;
    }

    void MeshComponent::CreateRenderObject(RenderScene* scene)
    {
        assert(renderObject == nullptr);
        if (renderObject == nullptr)
        {
            RenderSystem* renderSystem = HorizonEngine::GetInstance()->GetSubsystem<RenderSystem>();
            RenderBackend* renderBackend = renderSystem->GetRenderBackend();

            renderObject = new MeshRenderObject();

            renderObject->vertexCount = vertexCount;
            renderObject->indexCount = indexCount;

            renderObject->localToWorldMatrix = localToWorldMatrix;
            renderObject->worldToLocalMatrix = Math::InverseMatrix(localToWorldMatrix);

            vertices.resize(positions.size());

            assert(positions.size() == normals.size());
            assert(positions.size() == tangents.size());
            assert(positions.size() == texCoords.size());

            for (size_t i = 0; i < vertices.size(); i++)
            {
                vertices[i].position = positions[i];
                vertices[i].padding0 = 0;
                vertices[i].padding1 = 0;
                vertices[i].normal = normals[i];
                vertices[i].tangent = tangents[i];
                vertices[i].textureCoordinates[0] = texCoords[i];
                vertices[i].textureCoordinates[1] = Vector2f(0, 0);
            }

            RenderBackendBufferDescription vertexBufferDesc = RenderBackendBufferDescription::CreateByteAddress(sizeof(VirtualGeometryVertex) * vertices.size());
            renderObject->vertexBuffer = renderBackend->CreateBuffer(&vertexBufferDesc, vertices.data(), "VertexBuffer");

            {
                VirtualGeometryBuildSettings settings;
                VirtualGeometryBuildInput input;
                input.vertices.position = positions;
                input.vertices.normals = normals;
                input.vertices.tangents = tangents;
                input.vertices.textureCoordinates[0] = texCoords;
                input.indices = indices;
                input.materialIndices = materialIndices;
                VirtualGeometryBuildOutput output;
                bool result = BuildVirtualGeometry(settings, input, output);

                RenderBackendBufferDescription meshletBufferDesc = RenderBackendBufferDescription::CreateByteAddress(sizeof(GPUSceneMeshletData) * output.meshlets.size());
                renderObject->meshletBuffer = renderBackend->CreateBuffer(&meshletBufferDesc, output.meshlets.data(), "MeshletBuffer");

                assert(indexCount == output.indices.size());
                RenderBackendBufferDescription meshletVertexBufferDesc = RenderBackendBufferDescription::CreateIndex(sizeof(uint32), static_cast<uint32>(output.indices.size()));
                renderObject->meshletVertexBuffer = renderBackend->CreateBuffer(&meshletVertexBufferDesc, output.indices.data(), "MeshletVertexBuffer");
                renderObject->meshletCount = static_cast<uint32>(output.meshlets.size());

                materialIndices = output.materialIndices;

                scene->meshlets = output.meshlets;

                for (auto m : scene->meshlets)
                {
                    if (skeleton)
                    {
                        m.isSkinned = 1;
                    }
                }
            }

            // @todo Refactor this.
            {
                RenderBackendBufferDescription vertexBuffer0Desc = RenderBackendBufferDescription::CreateByteAddress(vertexCount * sizeof(Vector3f));
                //vertexBuffer0Desc.flags |= RenderBackendBufferCreateFlags::RayTracingAccelerationStructure; // TODO
                renderObject->vertexBuffers[0] = renderBackend->CreateBuffer(&vertexBuffer0Desc, positions.data(), "VertexPosition");

                // RenderBackendBufferDescription vertexBuffer1Desc = RenderBackendBufferDescription::CreateByteAddress(normals.size() * sizeof(Vector3f));
                // renderObject->vertexBuffers[1] = renderBackend->CreateBuffer(&vertexBuffer1Desc, normals.data(), "VertexNormal");
                //
                // RenderBackendBufferDescription vertexBuffer2Desc = RenderBackendBufferDescription::CreateByteAddress(tangents.size() * sizeof(Vector4f));
                // renderObject->vertexBuffers[2] = renderBackend->CreateBuffer(&vertexBuffer2Desc, tangents.data(), "VertexTangent");
                //
                // if (!texCoords.empty())
                // {
                //     RenderBackendBufferDescription vertexBuffer3Desc = RenderBackendBufferDescription::CreateByteAddress(texCoords.size() * sizeof(Vector2f));
                //     renderObject->vertexBuffers[3] = renderBackend->CreateBuffer(&vertexBuffer3Desc, texCoords.data(), "VertexTextureCoord0");
                // }

                if (indexCount > 0)
                {
                    RenderBackendBufferDescription indexBufferDesc = RenderBackendBufferDescription::CreateIndex(sizeof(uint32), indexCount);
                    //indexBufferDesc.flags |= RenderBackendBufferCreateFlags::RayTracingAccelerationStructure; // TODO
                    renderObject->indexBuffer = renderBackend->CreateBuffer(&indexBufferDesc, indices.data(), "IndexBuffer");
                    renderObject->indexBuffer = renderObject->meshletVertexBuffer;
                }

                for (Material& material : materials)
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
                    materialData.emplace_back(materialShaderParameters);
                }

                RenderBackendBufferDescription materialBufferDesc = RenderBackendBufferDescription::CreateByteAddress(materialData.size() * sizeof(MaterialShaderParameters));
                renderObject->materialBuffer = renderBackend->CreateBuffer(&materialBufferDesc, materialData.data(), "MaterialBuffer");

                RenderBackendBufferDescription materialIndexBufferDesc = RenderBackendBufferDescription::CreateByteAddress((indexCount / 3) * sizeof(uint32));
                renderObject->materialIndexBuffer = renderBackend->CreateBuffer(&materialIndexBufferDesc, materialIndices.data(), "MaterialIndexBuffer");
            }

            if (skeleton)
            {
                RenderBackendBufferDescription jointIndexBufferDesc = RenderBackendBufferDescription::CreateByteAddress(jointIndices.size() * sizeof(int32));
                renderObject->jointIndexBuffer = renderBackend->CreateBuffer(&jointIndexBufferDesc, jointIndices.data(), "jointIndexBuffer");

                RenderBackendBufferDescription jointWeightBufferDesc = RenderBackendBufferDescription::CreateByteAddress(jointWeights.size() * sizeof(float));
                renderObject->jointWeightBuffer = renderBackend->CreateBuffer(&jointWeightBufferDesc, jointWeights.data(), "jointWeightBuffer");

                RenderBackendBufferDescription jointTransformBufferDesc = RenderBackendBufferDescription::CreateByteAddress(jointTransforms.size() * sizeof(Matrix4x4f));
                renderObject->jointTransformBuffer = renderBackend->CreateBuffer(&jointTransformBufferDesc, jointTransforms.data(), "jointTransformBuffer");
            }

            if (visible)
            {
                Matrix4x4f transformData = IdentityMatrix4x4f;
                RenderBackendBufferDescription transformBufferRowMajorDescription = RenderBackendBufferDescription::CreateByteAddress(sizeof(Matrix4x4f));
                renderObject->transformBufferRowMajor = renderBackend->CreateBuffer(&transformBufferRowMajorDescription, &transformData, "RowMajorTransformBuffer");

                RenderBackendRayTracingGeometryDescription geometryDescription = {};
                geometryDescription.type = RenderBackendRayTracingGeometryType::Triangles;
                geometryDescription.flags = RenderBackendRayTracingGeometryFlags::Opaque;
                geometryDescription.triangleDescription.indexCount = indexCount;
                geometryDescription.triangleDescription.vertexCount = vertexCount;
                geometryDescription.triangleDescription.vertexStride = 3 * sizeof(float);
                geometryDescription.triangleDescription.vertexBuffer = renderObject->vertexBuffers[0];
                geometryDescription.triangleDescription.vertexOffset = 0;
                geometryDescription.triangleDescription.indexBuffer = renderObject->indexBuffer;
                geometryDescription.triangleDescription.indexOffset = 0;//mesh->baseIndex * sizeof(uint32);
                geometryDescription.triangleDescription.transformBuffer = renderObject->transformBufferRowMajor;
                geometryDescription.triangleDescription.transformOffset = 0 * static_cast<uint32>(sizeof(float)) * 16;

                std::vector<RenderBackendRayTracingGeometryDescription> geometryDescriptions;
                geometryDescriptions.push_back(geometryDescription);

                RenderBackendRayTracingBottomLevelAccelerationStructureDescription blasDesc =
                {
                    .buildFlags = RenderBackendRayTracingAccelerationStructureBuildFlags::PreferFastTrace,
                    .geometryCount = static_cast<uint32>(geometryDescriptions.size()),
                    .geometryDescriptions = geometryDescriptions.data()
                };

                renderObject->bottomLevelAccelerationStructure = renderBackend->CreateRayTracingBottomLevelAccelerationStructure(&blasDesc, "BLAS");

                scene->GetRayTracingScene()->RequestBuildRayTracingBLAS(renderObject->bottomLevelAccelerationStructure);
            }

            scene->AddMesh(renderObject);
        }
    }

    void MeshComponent::DestroyRenderObject(RenderScene* scene)
    {

    }

    void MeshComponent::UpdateRenderObject()
    {
        if (renderObject)
        {
            renderObject->localToWorldMatrix = localToWorldMatrix;
            renderObject->worldToLocalMatrix = Math::InverseMatrix(localToWorldMatrix);
        }
    }

    uint32 MeshComponent::GetMaterialCount() const
    {
        if (true)
        {
            return (uint32)materials.size();
        }
        return 0;
    }
}