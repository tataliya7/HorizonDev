#include "Rendering/ShaderLibrary.h"

namespace HE
{
    ShaderLibrary_Deprecated::ShaderLibrary_Deprecated(RenderBackend* backend, ShaderCompiler* compiler, uint32 maxNumShaders, bool hotReloadEnabled)
        : renderBackend(backend)
        , shaderCompiler(compiler)
        , maxNumShaders(maxNumShaders)
        , hotReloadEnabled(hotReloadEnabled)
    {
        loadedShaders.resize(maxNumShaders);
    }

    void ShaderLibrary_Deprecated::AddIncludeDirectory(const wchar_t* dir)
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
                LoadShader(i, loadedShaders[i].desc, true);
            }
        }

        return true;
    }

    RenderBackendShaderHandle ShaderLibrary_Deprecated::GetShaderHandle(uint32 id)
    {
        return loadedShaders[id].handle;
    }

    bool ShaderLibrary_Deprecated::LoadShader(uint32 id, ShaderDesc& desc, bool reload)
    {
        std::vector<uint8> source;
        {
            if (!reload)
            {
                desc.filename = std::filesystem::path("../../../Shaders").append(desc.filename.string());
            }
            LoadShaderSourceFromFile(desc.filename.string().c_str(), source);
        }

        ShaderIntermediateLanguage il = (GRenderBackend->GetType() == RenderBackendType::Vulkan) ? ShaderIntermediateLanguage::SPIRV : ShaderIntermediateLanguage::DXIL;

        std::filesystem::path path = std::filesystem::absolute(desc.filename);

        std::unordered_set<std::wstring> includedFiles;

        RenderBackendShaderDesc shaderDesc = {};
        bool success = true;
        if (desc.type == ShaderDesc::Type::Graphics)
        {
            if (!desc.entryPoints[(uint32)RenderBackendShaderStage::Vertex].empty())
            {
                std::wstring vertexStageEntry = std::wstring(desc.entryPoints[(uint32)RenderBackendShaderStage::Vertex].begin(), desc.entryPoints[(uint32)RenderBackendShaderStage::Vertex].end());
                success &= shaderCompiler->CompileShader(
                    source,
                    vertexStageEntry.c_str(),
                    RenderBackendShaderStage::Vertex,
                    il,
                    includeDirs,
                    desc.defines,
                    &shaderDesc.stages[(uint32)RenderBackendShaderStage::Vertex],
                    &includedFiles);
                shaderDesc.entryPoints[(uint32)RenderBackendShaderStage::Vertex] = desc.entryPoints[(uint32)RenderBackendShaderStage::Vertex].c_str();
            }
            else
            {
                if (!desc.entryPoints[(uint32)RenderBackendShaderStage::Task].empty())
                {
                    std::wstring entryPoint = std::wstring(desc.entryPoints[(uint32)RenderBackendShaderStage::Task].begin(), desc.entryPoints[(uint32)RenderBackendShaderStage::Task].end());
                    success &= shaderCompiler->CompileShader(
                        source,
                        entryPoint.c_str(),
                        RenderBackendShaderStage::Task,
                        il,
                        includeDirs,
                        desc.defines,
                        &shaderDesc.stages[(uint32)RenderBackendShaderStage::Task],
                        &includedFiles);
                    shaderDesc.entryPoints[(uint32)RenderBackendShaderStage::Task] = desc.entryPoints[(uint32)RenderBackendShaderStage::Task].c_str();
                }
                std::wstring entryPoint = std::wstring(desc.entryPoints[(uint32)RenderBackendShaderStage::Mesh].begin(), desc.entryPoints[(uint32)RenderBackendShaderStage::Mesh].end());
                success &= shaderCompiler->CompileShader(
                    source,
                    entryPoint.c_str(),
                    RenderBackendShaderStage::Mesh,
                    il,
                    includeDirs,
                    desc.defines,
                    &shaderDesc.stages[(uint32)RenderBackendShaderStage::Mesh],
                    &includedFiles);
                shaderDesc.entryPoints[(uint32)RenderBackendShaderStage::Mesh] = desc.entryPoints[(uint32)RenderBackendShaderStage::Mesh].c_str();
            }

            std::wstring pixelStageEntry = std::wstring(desc.entryPoints[(uint32)RenderBackendShaderStage::Pixel].begin(), desc.entryPoints[(uint32)RenderBackendShaderStage::Pixel].end());
            success &= shaderCompiler->CompileShader(
                source,
                pixelStageEntry.c_str(),
                RenderBackendShaderStage::Pixel,
                il,
                includeDirs,
                desc.defines,
                &shaderDesc.stages[(uint32)RenderBackendShaderStage::Pixel],
                &includedFiles);
            shaderDesc.entryPoints[(uint32)RenderBackendShaderStage::Pixel] = desc.entryPoints[(uint32)RenderBackendShaderStage::Pixel].c_str();
        }
        else if (desc.type == ShaderDesc::Type::Compute)
        {
            std::wstring computeStageEntry = std::wstring(desc.entryPoints[(uint32)RenderBackendShaderStage::Compute].begin(), desc.entryPoints[(uint32)RenderBackendShaderStage::Compute].end());
            success &= shaderCompiler->CompileShader(
                source,
                computeStageEntry.c_str(),
                RenderBackendShaderStage::Compute,
                il,
                includeDirs,
                desc.defines,
                &shaderDesc.stages[(uint32)RenderBackendShaderStage::Compute],
                &includedFiles);
            shaderDesc.entryPoints[(uint32)RenderBackendShaderStage::Compute] = desc.entryPoints[(uint32)RenderBackendShaderStage::Compute].c_str();
        }
        else if (desc.type == ShaderDesc::Type::RayTracing)
        {
            {
                std::wstring stageEntry = std::wstring(desc.entryPoints[(uint32)RenderBackendShaderStage::RayGen].begin(), desc.entryPoints[(uint32)RenderBackendShaderStage::RayGen].end());
                success &= shaderCompiler->CompileShader(
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
                success &= shaderCompiler->CompileShader(
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
                success &= shaderCompiler->CompileShader(
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
                success &= shaderCompiler->CompileShader(
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
                success &= shaderCompiler->CompileShader(
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
}