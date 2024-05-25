#include "ShaderLibrary.h"

namespace Horizon
{
    void LoadShaderSourceFromFile(const char* filename, std::vector<uint8>& outData)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            // LogError(GLogger, std::format(("Failed to open shader source file.")));
            return;
        }
        size_t fileSize = (size_t)file.tellg();
        outData.resize(fileSize);
        file.seekg(0);
        file.read(reinterpret_cast<char*>(outData.data()), fileSize);
        file.close();
    }

    ShaderLibrary_Deprecated::ShaderLibrary_Deprecated(RenderBackend* backend, ShaderCompiler* compiler, uint32 maxNumShaders, bool hotReloadEnabled)
        : shaderCompiler(compiler)
        , hotReloadEnabled(hotReloadEnabled)
        , maxNumShaders(maxNumShaders)
        , renderBackend(backend)
    {
        loadedShaders.resize(maxNumShaders);

        shadingLanguage = (renderBackend->GetType() == RenderBackendType::Vulkan) ? ShadingLanguage::SPIRV : ShadingLanguage::DXIL;
    }

    void ShaderLibrary_Deprecated::AddIncludeDirectory(const char* dir)
    {
        includeDirs.push_back(dir);
    }

    bool ShaderLibrary_Deprecated::HotReload()
    {
        if (!hotReloadEnabled)
        {
            return false;
        }

        for (uint32 i = 0; i < loadedShaders.size(); i++)
        {
            if (!loadedShaders[i].compiled)
            {
                continue;
            }

            bool shouldRecompile = false;
            for (size_t index = 0; index < loadedShaders[i].relatedFiles.size(); index++)
            {
                const auto& file = loadedShaders[i].relatedFiles[index];
                auto lastModifiedTime = std::filesystem::last_write_time(file);
                if (lastModifiedTime != loadedShaders[i].lastModifiedTime[index])
                {
                    shouldRecompile = true;
                    break;
                }
            }

            if (shouldRecompile)
            {
                const ShaderID id = ShaderID(i);
                LoadShader(id, loadedShaders[i].desc);
            }
        }

        return true;
    }

    RenderBackendShaderHandle ShaderLibrary_Deprecated::GetShaderHandle(ShaderID id)
    {
        const uint32 shaderIndex = uint32(id);
        return loadedShaders[shaderIndex].handle;
    }

    bool ShaderLibrary_Deprecated::LoadShader(ShaderID id, ShaderDesc& desc)
    {
        std::filesystem::path path = std::filesystem::absolute(std::filesystem::path("../../../Shaders").append(desc.filename));
        std::string filename = path.string();

        std::vector<uint8> source;
        std::unordered_set<std::string> relatedFiles;

        relatedFiles.insert(filename);

        LoadShaderSourceFromFile(filename.c_str(), source);

        if (desc.type == ShaderDesc::Type::Compute)
        {
            assert(!desc.entryPoints[(uint32)RenderBackendShaderStage::Compute].empty());
        }
        else if (desc.type == ShaderDesc::Type::Graphics)
        {
            assert(!desc.entryPoints[(uint32)RenderBackendShaderStage::Vertex].empty());
            assert(!desc.entryPoints[(uint32)RenderBackendShaderStage::Pixel].empty());
        }
        else if (desc.type == ShaderDesc::Type::Mesh)
        {
            assert(!desc.entryPoints[(uint32)RenderBackendShaderStage::Mesh].empty());
        }
        else if (desc.type == ShaderDesc::Type::RayTracing)
        {
            assert(false);
        }
        else
        {
            assert(false);
        }

        RenderBackendShaderDesc shaderDesc = {};

        ShaderCompilerOutput compilerOutputs[(uint32)RenderBackendShaderStage::Count];

        bool succeed = true;
        for (uint32 stageIndex = 0; stageIndex < (uint32)RenderBackendShaderStage::Count; stageIndex++)
        {
            if (!desc.entryPoints[stageIndex].empty())
            {
                ShaderSource shaderSource;
                shaderSource.filename = filename.c_str();
                shaderSource.sourceData = source.data();
                shaderSource.sourceSize = source.size();
                shaderSource.entryPoint = desc.entryPoints[stageIndex].c_str();
                shaderSource.stage = ShaderStage(stageIndex);
                shaderSource.defines = desc.defines.data();
                shaderSource.numDefines = (uint32)desc.defines.size();
                shaderSource.includeDirectories = includeDirs.data();
                shaderSource.numIncludeDirectories = (uint32)includeDirs.size();

                succeed |= shaderCompiler->CompileShader(shaderCompilerSettings, shaderSource, shadingLanguage, &compilerOutputs[stageIndex]);

                if (succeed)
                {
                    shaderDesc.stages[stageIndex].data = compilerOutputs[stageIndex].blob.GetData();
                    shaderDesc.stages[stageIndex].size = compilerOutputs[stageIndex].blob.GetSize();
                    shaderDesc.entryPoints[stageIndex] = shaderSource.entryPoint;

                    for (const std::string& f : compilerOutputs[stageIndex].includedFiles)
                    {
                        relatedFiles.insert(f);
                    }
                }
                else
                {
                    // TODO: log error message
                }
            }
        }

#if 0
        else if (desc.type == ShaderDesc::Type::RayTracing)
        {
            {
                std::wstring stageEntry = std::wstring(desc.entryPoints[(uint32)RenderBackendShaderStage::RayGen].begin(), desc.entryPoints[(uint32)RenderBackendShaderStage::RayGen].end());
                success &= shaderCompiler->CompileShader_Depreacated(
                    source,
                    stageEntry.c_str(),
                    RenderBackendShaderStage::RayGen,
                    il,
                    includeDirs,
                    desc.defines,
                    &shaderDesc.stages[(uint32)RenderBackendShaderStage::RayGen],
                    &includedFiles);
                shaderDesc.entryPoints[(uint32)RenderBackendShaderStage::RayGen] = desc.entryPoints[(uint32)RenderBackendShaderStage::RayGen].c_str();
            }

            {
                std::wstring stageEntry = std::wstring(desc.entryPoints[(uint32)RenderBackendShaderStage::AnyHit].begin(), desc.entryPoints[(uint32)RenderBackendShaderStage::AnyHit].end());
                success &= shaderCompiler->CompileShader_Depreacated(
                    source,
                    stageEntry.c_str(),
                    RenderBackendShaderStage::AnyHit,
                    il,
                    includeDirs,
                    desc.defines,
                    &shaderDesc.stages[(uint32)RenderBackendShaderStage::AnyHit],
                    &includedFiles);
                shaderDesc.entryPoints[(uint32)RenderBackendShaderStage::AnyHit] = desc.entryPoints[(uint32)RenderBackendShaderStage::AnyHit].c_str();
            }

            {
                std::wstring stageEntry = std::wstring(desc.entryPoints[(uint32)RenderBackendShaderStage::ClosestHit].begin(), desc.entryPoints[(uint32)RenderBackendShaderStage::ClosestHit].end());
                success &= shaderCompiler->CompileShader_Depreacated(
                    source,
                    stageEntry.c_str(),
                    RenderBackendShaderStage::ClosestHit,
                    il,
                    includeDirs,
                    desc.defines,
                    &shaderDesc.stages[(uint32)RenderBackendShaderStage::ClosestHit],
                    &includedFiles);
                shaderDesc.entryPoints[(uint32)RenderBackendShaderStage::ClosestHit] = desc.entryPoints[(uint32)RenderBackendShaderStage::ClosestHit].c_str();
            }

            {
                std::wstring stageEntry = std::wstring(desc.entryPoints[(uint32)RenderBackendShaderStage::Miss].begin(), desc.entryPoints[(uint32)RenderBackendShaderStage::Miss].end());
                success &= shaderCompiler->CompileShader_Depreacated(
                    source,
                    stageEntry.c_str(),
                    RenderBackendShaderStage::Miss,
                    il,
                    includeDirs,
                    desc.defines,
                    &shaderDesc.stages[(uint32)RenderBackendShaderStage::Miss],
                    &includedFiles);
                shaderDesc.entryPoints[(uint32)RenderBackendShaderStage::Miss] = desc.entryPoints[(uint32)RenderBackendShaderStage::Miss].c_str();
            }
            {
                std::wstring stageEntry = std::wstring(desc.entryPoints[(uint32)RenderBackendShaderStage::Intersection].begin(), desc.entryPoints[(uint32)RenderBackendShaderStage::Intersection].end());
                success &= shaderCompiler->CompileShader_Depreacated(
                    source,
                    stageEntry.c_str(),
                    RenderBackendShaderStage::Intersection,
                    il,
                    includeDirs,
                    desc.defines,
                    &shaderDesc.stages[(uint32)RenderBackendShaderStage::Intersection],
                    &includedFiles);
                shaderDesc.entryPoints[(uint32)RenderBackendShaderStage::Intersection] = desc.entryPoints[(uint32)RenderBackendShaderStage::Intersection].c_str();
            }
        }
#endif

        const uint32 shaderIndex = uint32(id);
        if (succeed)
        {
            RenderBackendShaderHandle handle = renderBackend->CreateShader(&shaderDesc, desc.name.c_str());
            if (handle)
            {
                loadedShaders[shaderIndex].compiled = true;
                loadedShaders[shaderIndex].desc = desc;
                loadedShaders[shaderIndex].handle = handle;

                loadedShaders[shaderIndex].relatedFiles.clear();
                loadedShaders[shaderIndex].lastModifiedTime.clear();
                for (const std::string& f : relatedFiles)
                {
                    loadedShaders[shaderIndex].relatedFiles.emplace_back(f);
                    loadedShaders[shaderIndex].lastModifiedTime.emplace_back(std::filesystem::last_write_time(f));
                }
            }
            else
            {
                succeed = false;
            }
        }

        return succeed;
    }
}