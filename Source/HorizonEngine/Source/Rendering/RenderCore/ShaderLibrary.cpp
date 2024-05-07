#include "Rendering/ShaderLibrary.h"

namespace Horizon
{
    ShaderLibrary_DEPRECATED::ShaderLibrary_DEPRECATED(RenderBackend* backend, ShaderCompiler* compiler, uint32 maxNumShaders, bool hotReloadEnabled)
        : renderBackend(backend)
        , shaderCompiler(compiler)
        , maxNumShaders(maxNumShaders)
        , hotReloadEnabled(hotReloadEnabled)
    {
        loadedShaders.resize(maxNumShaders);
    }

    void ShaderLibrary_DEPRECATED::AddIncludeDirectory(const char* dir)
    {
        includeDirs.push_back(dir);
    }

    bool ShaderLibrary_DEPRECATED::HotReload()
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
                LoadShader(i, loadedShaders[i].desc, true);
            }
        }

        return true;
    }

    RenderBackendShaderHandle ShaderLibrary_DEPRECATED::GetShaderHandle(ShaderID id)
    {
        return loadedShaders[id].handle;
    }

    bool ShaderLibrary_DEPRECATED::LoadShader(ShaderID id, ShaderDesc& desc, bool reload)
    {
        std::vector<uint8> source;
        {
            if (!reload)
            {
                desc.filename = std::filesystem::path("../../../Shaders").append(desc.filename.string());
            }
            LoadShaderSourceFromFile(desc.filename.string().c_str(), source);
        }

        ShadingLanguage shadingLanguage = (GRenderBackend->GetType() == RenderBackendType::Vulkan) ? ShadingLanguage::SPIRV : ShadingLanguage::DXIL;

        std::filesystem::path path = std::filesystem::absolute(desc.filename);
        std::string filename = path.string();

        ShaderCompilerSettings shaderCompilerSettings;

        ShaderCompilerOutput shaderCompilerOutput1;
        ShaderCompilerOutput shaderCompilerOutput2;
        ShaderCompilerOutput shaderCompilerOutput3;

        std::unordered_set<std::wstring> includedFiles;

        RenderBackendShaderDesc shaderDesc = {};
        bool success = true;
        if (desc.type == ShaderDesc::Type::Graphics)
        {
            if (!desc.entryPoints[(uint32)RenderBackendShaderStage::Vertex].empty())
            {
                ShaderSource shaderSource;
                shaderSource.filename = filename.c_str();
                shaderSource.sourceData = source.data();
                shaderSource.sourceSize = source.size();
                shaderSource.entryPoint = desc.entryPoints[(uint32)RenderBackendShaderStage::Vertex].c_str();
                shaderSource.stage = ShaderStage::Vertex;
                shaderSource.defines = desc.defines.data();
                shaderSource.numDefines = (uint32)desc.defines.size();
                shaderSource.includeDirectories = includeDirs.data();
                shaderSource.numIncludeDirectories = (uint32)includeDirs.size();

                bool succeed = shaderCompiler->CompileShader(shaderCompilerSettings, shaderSource, shadingLanguage, &shaderCompilerOutput1);
                if (!shaderCompilerOutput1.errorMessage.empty())
                {
                    LogError(GLogger, std::format("{}", shaderCompilerOutput1.errorMessage));
                }

                shaderDesc.stages[(uint32)RenderBackendShaderStage::Vertex].data = shaderCompilerOutput1.blob.GetData();
                shaderDesc.stages[(uint32)RenderBackendShaderStage::Vertex].size = shaderCompilerOutput1.blob.GetSize();
                shaderDesc.entryPoints[(uint32)RenderBackendShaderStage::Vertex] = shaderSource.entryPoint;
            }
            else
            {
                if (!desc.entryPoints[(uint32)RenderBackendShaderStage::Task].empty())
                {
                    ShaderSource shaderSource;
                    shaderSource.filename = filename.c_str();
                    shaderSource.sourceData = source.data();
                    shaderSource.sourceSize = source.size();
                    shaderSource.entryPoint = desc.entryPoints[(uint32)RenderBackendShaderStage::Task].c_str();
                    shaderSource.stage = ShaderStage::Task;
                    shaderSource.defines = desc.defines.data();
                    shaderSource.numDefines = (uint32)desc.defines.size();
                    shaderSource.includeDirectories = includeDirs.data();
                    shaderSource.numIncludeDirectories = (uint32)includeDirs.size();

                    bool succeed = shaderCompiler->CompileShader(shaderCompilerSettings, shaderSource, shadingLanguage, &shaderCompilerOutput1);

                    shaderDesc.stages[(uint32)RenderBackendShaderStage::Task].data = shaderCompilerOutput1.blob.GetData();
                    shaderDesc.stages[(uint32)RenderBackendShaderStage::Task].size = shaderCompilerOutput1.blob.GetSize();
                    shaderDesc.entryPoints[(uint32)RenderBackendShaderStage::Task] = shaderSource.entryPoint;
                }
                {
                    ShaderSource shaderSource;
                    shaderSource.filename = filename.c_str();
                    shaderSource.sourceData = source.data();
                    shaderSource.sourceSize = source.size();
                    shaderSource.entryPoint = desc.entryPoints[(uint32)RenderBackendShaderStage::Mesh].c_str();
                    shaderSource.stage = ShaderStage::Mesh;
                    shaderSource.defines = desc.defines.data();
                    shaderSource.numDefines = (uint32)desc.defines.size();
                    shaderSource.includeDirectories = includeDirs.data();
                    shaderSource.numIncludeDirectories = (uint32)includeDirs.size();

                    bool succeed = shaderCompiler->CompileShader(shaderCompilerSettings, shaderSource, shadingLanguage, &shaderCompilerOutput2);

                    shaderDesc.stages[(uint32)RenderBackendShaderStage::Mesh].data = shaderCompilerOutput2.blob.GetData();
                    shaderDesc.stages[(uint32)RenderBackendShaderStage::Mesh].size = shaderCompilerOutput2.blob.GetSize();
                    shaderDesc.entryPoints[(uint32)RenderBackendShaderStage::Mesh] = shaderSource.entryPoint;
                }
            }

            ShaderSource shaderSource;
            shaderSource.filename = filename.c_str();
            shaderSource.sourceData = source.data();
            shaderSource.sourceSize = source.size();
            shaderSource.entryPoint = desc.entryPoints[(uint32)RenderBackendShaderStage::Pixel].c_str();
            shaderSource.stage = ShaderStage::Pixel;
            shaderSource.defines = desc.defines.data();
            shaderSource.numDefines = (uint32)desc.defines.size();
            shaderSource.includeDirectories = includeDirs.data();
            shaderSource.numIncludeDirectories = (uint32)includeDirs.size();

            bool succeed = shaderCompiler->CompileShader(shaderCompilerSettings, shaderSource, shadingLanguage, &shaderCompilerOutput3);

            shaderDesc.stages[(uint32)RenderBackendShaderStage::Pixel].data = shaderCompilerOutput3.blob.GetData();
            shaderDesc.stages[(uint32)RenderBackendShaderStage::Pixel].size = shaderCompilerOutput3.blob.GetSize();
            shaderDesc.entryPoints[(uint32)RenderBackendShaderStage::Pixel] = shaderSource.entryPoint;
        }
        else if (desc.type == ShaderDesc::Type::Compute)
        {
            ShaderSource shaderSource;
            shaderSource.filename = filename.c_str();
            shaderSource.sourceData = source.data();
            shaderSource.sourceSize = source.size();
            shaderSource.entryPoint = desc.entryPoints[(uint32)RenderBackendShaderStage::Compute].c_str();
            shaderSource.stage = ShaderStage::Compute;
            shaderSource.defines = desc.defines.data();
            shaderSource.numDefines = (uint32)desc.defines.size();
            shaderSource.includeDirectories = includeDirs.data();
            shaderSource.numIncludeDirectories = (uint32)includeDirs.size();

            bool succeed = shaderCompiler->CompileShader(shaderCompilerSettings, shaderSource, shadingLanguage, &shaderCompilerOutput1);

            shaderDesc.stages[(uint32)RenderBackendShaderStage::Compute].data = shaderCompilerOutput1.blob.GetData();
            shaderDesc.stages[(uint32)RenderBackendShaderStage::Compute].size = shaderCompilerOutput1.blob.GetSize();
            shaderDesc.entryPoints[(uint32)RenderBackendShaderStage::Compute] = shaderSource.entryPoint;
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

        std::unordered_set<std::string> releatedFiles;
        releatedFiles.insert(path.string());

        for (std::wstring wstr : includedFiles)
        {
            std::string wstr_to_str = UTF16ToUTF8(wstr);
            releatedFiles.insert(wstr_to_str);
        }

        if (success)
        {
            RenderBackendShaderHandle handle = renderBackend->CreateShader(~0u, &shaderDesc, desc.name.c_str());
            if (handle)
            {
                loadedShaders[id].desc = desc;
                loadedShaders[id].handle = handle;
                loadedShaders[id].relatedFiles.clear();
                loadedShaders[id].lastModifiedTime.clear();
                for (const auto& file : releatedFiles)
                {
                    loadedShaders[id].relatedFiles.push_back(file);
                    loadedShaders[id].lastModifiedTime.push_back(std::filesystem::last_write_time(file));
                }
                loadedShaders[id].compiled = true;
            }
            else
            {
                success = false;
                assert(false);
            }
        }

        return success;
    }

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
        file.read((char*)outData.data(), fileSize);
        file.close();
    }
}