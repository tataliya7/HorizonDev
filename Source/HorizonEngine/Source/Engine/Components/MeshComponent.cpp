#include "MeshComponent.h"

// @todo Remove this
#include "Engine/HorizonEngineModule.h"

#include <meshoptimizer.h>

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
            renderObject = new MeshRenderObject();

            renderObject->vertexCount = vertexCount;
            renderObject->indexCount = indexCount;

            renderObject->localToWorldMatrix = localToWorldMatrix;
            renderObject->worldToLocalMatrix = Math::InverseMatrix(localToWorldMatrix);

            vertices.resize(positions.size());

            for (size_t i = 0; i < vertices.size(); i++)
            {
                vertices[i].position = positions[i];
                vertices[i].normal = normals[i];
                vertices[i].tangent = tangents[i];
                vertices[i].textureCoordinates[0] = texCoords[i];
            }

            const size_t max_vertices = 64;
            const size_t max_triangles = 124;
            const float cone_weight = 0.0f;

            size_t max_meshlets = meshopt_buildMeshletsBound(indices.size(), max_vertices, max_triangles);
            std::vector<meshopt_Meshlet> m(max_meshlets);
            std::vector<unsigned int> meshlet_vertices(max_meshlets * max_vertices);
            std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);

            size_t meshlet_count = meshopt_buildMeshlets(
                m.data(),
                meshlet_vertices.data(),
                meshlet_triangles.data(),
                indices.data(),
                indices.size(),
                &vertices[0].position.x,
                vertices.size(),
                sizeof(VirtualGeometryVertex),
                max_vertices,
                max_triangles,
                cone_weight);

            const meshopt_Meshlet& last = m[meshlet_count - 1];

            meshlet_vertices.resize(last.vertex_offset + last.vertex_count);
            meshlet_triangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
            m.resize(meshlet_count);

            // meshopt_Bounds bounds = meshopt_computeMeshletBounds(
            //     &meshlet_vertices[m.vertex_offset],
            //     &meshlet_triangles[m.triangle_offset],
            //     m.triangle_count,
            //     &vertices[0].position.x,
            //     vertices.size(),
            //     sizeof(VirtualGeometryVertex));

            // @todo Refactor this.
            {
                RenderSystem* renderSystem = HorizonEngine::GetInstance()->GetSubsystem<RenderSystem>();
                RenderBackend* renderBackend = renderSystem->GetRenderBackend();

                RenderBackendBufferDesc vertexBuffer0Desc = RenderBackendBufferDesc::CreateByteAddress(vertexCount * sizeof(Vector3f));
                vertexBuffer0Desc.flags |= RenderBackendBufferCreateFlags::RayTracingAccelerationStructure; // TODO
                renderObject->vertexBuffers[0] = renderBackend->CreateBuffer(&vertexBuffer0Desc, positions.data(), "VertexPosition");

                RenderBackendBufferDesc vertexBuffer1Desc = RenderBackendBufferDesc::CreateByteAddress(normals.size() * sizeof(Vector3f));
                renderObject->vertexBuffers[1] = renderBackend->CreateBuffer(&vertexBuffer1Desc, normals.data(), "VertexNormal");

                RenderBackendBufferDesc vertexBuffer2Desc = RenderBackendBufferDesc::CreateByteAddress(tangents.size() * sizeof(Vector4f));
                renderObject->vertexBuffers[2] = renderBackend->CreateBuffer(&vertexBuffer2Desc, tangents.data(), "VertexTangent");

                if (!texCoords.empty())
                {
                    RenderBackendBufferDesc vertexBuffer3Desc = RenderBackendBufferDesc::CreateByteAddress(texCoords.size() * sizeof(Vector2f));
                    renderObject->vertexBuffers[3] = renderBackend->CreateBuffer(&vertexBuffer3Desc, texCoords.data(), "VertexTextureCoord0");
                }

                if (indexCount > 0)
                {
                    RenderBackendBufferDesc indexBufferDesc = RenderBackendBufferDesc::CreateIndex(sizeof(uint32), indexCount);
                    indexBufferDesc.flags |= RenderBackendBufferCreateFlags::RayTracingAccelerationStructure; // TODO
                    renderObject->indexBuffer = renderBackend->CreateBuffer(&indexBufferDesc, indices.data(), "IndexBuffer");

                    uint32 indexOffset = 0;
                    uint32 meshletCount = Math::CeilDiv(indexCount, IndexCountPerMeshlet);
                    for (uint32 meshletIndex = 0; meshletIndex < meshletCount; meshletIndex++)
                    {
                        GPUSceneMeshletData meshletData =
                        {
                            .vertexOffset = indexOffset,
                            .triangleOffset =  0,
                            .vertexCount = 0,
                            .triangleCount = (meshletIndex == (meshletCount - 1)) ? (indexCount % IndexCountPerMeshlet) : IndexCountPerMeshlet
                        };
                        meshlets.push_back(meshletData);

                        indexOffset += IndexCountPerMeshlet;
                    }

                    RenderBackendBufferDesc meshletBufferDesc = RenderBackendBufferDesc::CreateByteAddress(sizeof(GPUSceneMeshletData) * meshletCount);
                    renderObject->meshletBuffer = renderBackend->CreateBuffer(&meshletBufferDesc, meshlets.data(), "MeshletBuffer");
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

                RenderBackendBufferDesc materialBufferDesc = RenderBackendBufferDesc::CreateByteAddress(materialData.size() * sizeof(MaterialShaderParameters));
                renderObject->materialBuffer = renderBackend->CreateBuffer(&materialBufferDesc, materialData.data(), "MaterialBuffer");

                RenderBackendBufferDesc materialIndexBufferDesc = RenderBackendBufferDesc::CreateByteAddress((indexCount / 3) * sizeof(uint32));
                renderObject->materialIndexBuffer = renderBackend->CreateBuffer(&materialIndexBufferDesc, materialIndices.data(), "MaterialIndexBuffer");
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