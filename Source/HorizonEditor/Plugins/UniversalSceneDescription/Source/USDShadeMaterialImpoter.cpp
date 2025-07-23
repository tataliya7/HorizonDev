#include "USDShadeMaterialImpoter.h"
#include "USDUtility.h"

namespace Horizon::USDImporter
{
    USDImporter::USDShadeMaterialImpoter::USDShadeMaterialImpoter(USDImportContext& context)
        : context(&context)
    {

    }

    Material USDShadeMaterialImpoter::ImportMaterial(const pxr::UsdShadeMaterial& usdShadeMaterial)
    {
        if (!usdShadeMaterial)
        {
            return {};
        }

        RenderSystem* renderSystem = HorizonEngine::GetInstance()->GetSubsystem<RenderSystem>();
        RenderBackend* renderBackend = renderSystem->GetRenderBackend();
        ShaderCollection* shaderLibrary = renderSystem->GetShaderLibrary();

        std::string materialName = usdShadeMaterial.GetPrim().GetName().GetString();

        Material newMaterial;

        bool isPreviewSurfaceShader = false;
        pxr::UsdShadeShader surfaceShader = usdShadeMaterial.ComputeSurfaceSource();
        if (surfaceShader)
        {
            pxr::TfToken shaderId;
            bool validShaderId = surfaceShader.GetShaderId(&shaderId);
            if (validShaderId && (shaderId == UsdTokens::UsdPreviewSurface))
            {
                isPreviewSurfaceShader = true;
            }
        }

        if (isPreviewSurfaceShader)
        {
            pxr::UsdShadeInput diffuseColorInput = surfaceShader.GetInput(UsdTokens::diffuseColor);
            if (diffuseColorInput)
            {
                if (diffuseColorInput.HasConnectedSource())
                {
                    pxr::UsdShadeConnectableAPI source;
                    pxr::TfToken sourceName;
                    pxr::UsdShadeAttributeType sourceType;
                    diffuseColorInput.GetConnectedSource(&source, &sourceName, &sourceType);

                    if (source && source.GetPrim().IsA<pxr::UsdShadeShader>())
                    {
                        pxr::UsdShadeShader sourceShader(source.GetPrim());
                        if (sourceShader)
                        {
                            pxr::TfToken shaderId;
                            if (sourceShader.GetShaderId(&shaderId))
                            {
                                if (shaderId == UsdTokens::UsdUVTexture)
                                {
                                    pxr::UsdShadeInput fileInput = sourceShader.GetInput(UsdTokens::file);
                                    if (fileInput)
                                    {
                                        pxr::VtValue fileValue;
                                        if (fileInput.Get(&fileValue) && fileValue.IsHolding<pxr::SdfAssetPath>())
                                        {
                                            const pxr::SdfAssetPath& assetPath = fileValue.Get<pxr::SdfAssetPath>();
                                            std::string path = assetPath.GetResolvedPath();
                                            if (!path.empty())
                                            {
                                                printf("Path: %s\n", path.c_str());

                                                RenderBackendTextureHandle gpuTexture;
                                                if (context->textureMap.find(path) != context->textureMap.end())
                                                {
                                                    gpuTexture = context->textureMap.at(path);
                                                }
                                                else
                                                {
                                                    gpuTexture = LoadTextureFromFile(renderBackend, shaderLibrary, path.c_str(), true, true, RenderBackendTextureFormat::R8G8B8A8UnormSrgb);
                                                    context->textureMap.emplace(path, gpuTexture);
                                                }

                                                newMaterial.textures[Material::TextureSlot::BaseColorMap].path = path;
                                                newMaterial.textures[Material::TextureSlot::BaseColorMap].gpuTexture = gpuTexture;
                                                newMaterial.textures[Material::TextureSlot::BaseColorMap].used = true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    pxr::VtValue value;
                    if (diffuseColorInput.GetAttr().HasAuthoredValue() &&
                        diffuseColorInput.GetAttr().Get(&value) &&
                        value.IsHolding<pxr::GfVec3f>())
                    {
                        pxr::GfVec3f vec3f = value.UncheckedGet<pxr::GfVec3f>();
                        newMaterial.baseColor = Vector4f(vec3f[0], vec3f[1], vec3f[2], 1.0f);
                    }
                }
            }

            pxr::UsdShadeInput normalInput = surfaceShader.GetInput(UsdTokens::normal);
            if (normalInput)
            {
                if (normalInput.HasConnectedSource())
                {
                    pxr::UsdShadeConnectableAPI source;
                    pxr::TfToken sourceName;
                    pxr::UsdShadeAttributeType sourceType;
                    normalInput.GetConnectedSource(&source, &sourceName, &sourceType);

                    if (source && source.GetPrim().IsA<pxr::UsdShadeShader>())
                    {
                        pxr::UsdShadeShader sourceShader(source.GetPrim());
                        if (sourceShader)
                        {
                            pxr::TfToken shaderId;
                            if (sourceShader.GetShaderId(&shaderId))
                            {
                                if (shaderId == UsdTokens::UsdUVTexture)
                                {
                                    pxr::UsdShadeInput fileInput = sourceShader.GetInput(UsdTokens::file);
                                    if (fileInput)
                                    {
                                        pxr::VtValue fileValue;
                                        if (fileInput.Get(&fileValue) && fileValue.IsHolding<pxr::SdfAssetPath>())
                                        {
                                            const pxr::SdfAssetPath& assetPath = fileValue.Get<pxr::SdfAssetPath>();
                                            std::string path = assetPath.GetResolvedPath();
                                            if (!path.empty())
                                            {
                                                printf("Path: %s\n", path.c_str());

                                                RenderBackendTextureHandle gpuTexture;
                                                if (context->textureMap.find(path) != context->textureMap.end())
                                                {
                                                    gpuTexture = context->textureMap.at(path);
                                                }
                                                else
                                                {
                                                    gpuTexture = LoadTextureFromFile(renderBackend, shaderLibrary, path.c_str(), true, true, RenderBackendTextureFormat::R8G8B8A8Unorm);
                                                    context->textureMap.emplace(path, gpuTexture);
                                                }

                                                newMaterial.textures[Material::TextureSlot::NormalMap].path = path;
                                                newMaterial.textures[Material::TextureSlot::NormalMap].gpuTexture = gpuTexture;
                                                newMaterial.textures[Material::TextureSlot::NormalMap].used = true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            pxr::UsdShadeInput metallicInput = surfaceShader.GetInput(UsdTokens::metallic);
            if (metallicInput)
            {
                if (metallicInput.HasConnectedSource())
                {
                    pxr::UsdShadeConnectableAPI source;
                    pxr::TfToken sourceName;
                    pxr::UsdShadeAttributeType sourceType;
                    metallicInput.GetConnectedSource(&source, &sourceName, &sourceType);

                    if (source && source.GetPrim().IsA<pxr::UsdShadeShader>())
                    {
                        pxr::UsdShadeShader sourceShader(source.GetPrim());
                        if (sourceShader)
                        {
                            pxr::TfToken shaderId;
                            if (sourceShader.GetShaderId(&shaderId))
                            {
                                if (shaderId == UsdTokens::UsdUVTexture)
                                {
                                    pxr::UsdShadeInput fileInput = sourceShader.GetInput(UsdTokens::file);
                                    if (fileInput)
                                    {
                                        pxr::VtValue fileValue;
                                        if (fileInput.Get(&fileValue) && fileValue.IsHolding<pxr::SdfAssetPath>())
                                        {
                                            const pxr::SdfAssetPath& assetPath = fileValue.Get<pxr::SdfAssetPath>();
                                            std::string path = assetPath.GetResolvedPath();
                                            if (!path.empty())
                                            {
                                                printf("Path: %s\n", path.c_str());

                                                RenderBackendTextureHandle gpuTexture;
                                                if (context->textureMap.find(path) != context->textureMap.end())
                                                {
                                                    gpuTexture = context->textureMap.at(path);
                                                }
                                                else
                                                {
                                                    gpuTexture = LoadTextureFromFile(renderBackend, shaderLibrary, path.c_str());
                                                    context->textureMap.emplace(path, gpuTexture);
                                                }

                                                newMaterial.textures[Material::TextureSlot::MetallicRoughnessMap].path = path;
                                                newMaterial.textures[Material::TextureSlot::MetallicRoughnessMap].gpuTexture = gpuTexture;
                                                newMaterial.textures[Material::TextureSlot::MetallicRoughnessMap].used = true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    pxr::VtValue value;
                    if (metallicInput.GetAttr().HasAuthoredValue() &&
                        metallicInput.GetAttr().Get(&value) &&
                        value.IsHolding<float>())
                    {
                        newMaterial.metallic = value.Get<float>();
                    }
                }
            }

            pxr::UsdShadeInput roughnessInput = surfaceShader.GetInput(UsdTokens::roughness);
            if (roughnessInput)
            {
                if (roughnessInput.HasConnectedSource())
                {
                    pxr::UsdShadeConnectableAPI source;
                    pxr::TfToken sourceName;
                    pxr::UsdShadeAttributeType sourceType;
                    roughnessInput.GetConnectedSource(&source, &sourceName, &sourceType);

                    if (source && source.GetPrim().IsA<pxr::UsdShadeShader>())
                    {
                        pxr::UsdShadeShader sourceShader(source.GetPrim());
                        if (sourceShader)
                        {
                            pxr::TfToken shaderId;
                            if (sourceShader.GetShaderId(&shaderId))
                            {
                                if (shaderId == UsdTokens::UsdUVTexture)
                                {
                                    pxr::UsdShadeInput fileInput = sourceShader.GetInput(UsdTokens::file);
                                    if (fileInput)
                                    {
                                        pxr::VtValue fileValue;
                                        if (fileInput.Get(&fileValue) && fileValue.IsHolding<pxr::SdfAssetPath>())
                                        {
                                            const pxr::SdfAssetPath& assetPath = fileValue.Get<pxr::SdfAssetPath>();
                                            std::string path = assetPath.GetResolvedPath();
                                            if (!path.empty())
                                            {
                                                printf("Path: %s\n", path.c_str());

                                                RenderBackendTextureHandle gpuTexture;
                                                if (context->textureMap.find(path) != context->textureMap.end())
                                                {
                                                    gpuTexture = context->textureMap.at(path);
                                                }
                                                else
                                                {
                                                    gpuTexture = LoadTextureFromFile(renderBackend, shaderLibrary, path.c_str());
                                                    context->textureMap.emplace(path, gpuTexture);
                                                }

                                                newMaterial.textures[Material::TextureSlot::MetallicRoughnessMap].path = path;
                                                newMaterial.textures[Material::TextureSlot::MetallicRoughnessMap].gpuTexture = gpuTexture;
                                                newMaterial.textures[Material::TextureSlot::MetallicRoughnessMap].used = true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    pxr::VtValue value;
                    if (roughnessInput.GetAttr().HasAuthoredValue() &&
                        roughnessInput.GetAttr().Get(&value) &&
                        value.IsHolding<float>())
                    {
                        newMaterial.roughness = value.Get<float>();
                    }
                }
            }

            pxr::UsdShadeInput emissiveInput = surfaceShader.GetInput(UsdTokens::emissiveColor);
            if (emissiveInput)
            {
                pxr::VtValue value;
                if (emissiveInput.GetAttr().HasAuthoredValue() &&
                    emissiveInput.GetAttr().Get(&value) &&
                    value.IsHolding<float>())
                {
                    pxr::GfVec3f vec3f = value.UncheckedGet<pxr::GfVec3f>();
                    newMaterial.emission = Vector4f(vec3f[0], vec3f[1], vec3f[2], 1.0f);
                }
            }
        }

        return newMaterial;
    }
}