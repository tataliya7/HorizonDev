#include "ShaderLibrary.h"

namespace Horizon
{
    static void LoadShaderSourceFromFile(const char* filename, std::vector<uint8>& outData)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            LogError(GLogger, std::format(("Failed to open shader source file.")));
            return;
        }
        size_t fileSize = (size_t)file.tellg();
        outData.resize(fileSize);
        file.seekg(0);
        file.read(reinterpret_cast<char*>(outData.data()), file.tellg());
        file.close();
    }

    ShaderLibrary::ShaderLibrary(RenderBackend* renderBackend, const std::string& rootDirectory)
    {
        this->renderBackend = renderBackend;
        this->rootDirectory = rootDirectory;
        this->shadingLanguage = (renderBackend->GetType() == RenderBackendType::Vulkan) ? ShadingLanguage::SPIRV : ShadingLanguage::DXIL;

        // TODO:
        this->shaderCompilerOptions = {
            .generateDebugInfo = true,
            .skipOptimization = true,
            .warningAreErrors = true,
            .enable16BitTypes = false,
            .optimizationLevel = ShaderOptimizationLevel::O3
        };

        this->hotReloadEnabled = true;
    }

    bool ShaderLibrary::HotReload()
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

    RenderBackendShaderHandle ShaderLibrary::GetShader(ShaderID id) const
    {
        const uint32 shaderIndex = uint32(id);
        return loadedShaders[shaderIndex].handle;
    }

    bool ShaderLibrary::LoadShader(ShaderID id, ShaderDesc& desc)
    {
        std::filesystem::path path = std::filesystem::absolute(std::filesystem::path("../../../Shaders").append(desc.filename));
        std::string filename = path.string();

        std::vector<uint8> source;
        std::unordered_set<std::string> relatedFiles;

        relatedFiles.insert(filename);

        LoadShaderSourceFromFile(filename.c_str(), source);

        std::vector<const char*> includeDirectories;
        includeDirectories.push_back(rootDirectory.c_str());

        ShaderCompilerOutput compilerOutput;

        bool succeed = false;
        if (desc.entryFunctionName != nullptr)
        {
            ShaderSourceDescription shaderSource;
            shaderSource.filename = filename.c_str();
            shaderSource.code = source.data();
            shaderSource.codeSize = source.size();
            shaderSource.entryPoint = desc.entryFunctionName;
            shaderSource.stage = desc.stage;
            shaderSource.defines = desc.defines.data();
            shaderSource.numDefines = static_cast<uint32>(desc.defines.size());
            shaderSource.includeDirectories = includeDirectories.data();
            shaderSource.numIncludeDirectories = static_cast<uint32>(includeDirectories.size());

            ShaderCompiler* shaderCompiler = CreateDXCShaderCompiler();
            if (shaderCompiler)
            {
                succeed = shaderCompiler->CompileShader(shaderCompilerOptions, shaderSource, shadingLanguage, &compilerOutput);
                DestroyDXCShaderCompiler(shaderCompiler);
            }

            if (succeed)
            {
                for (const std::string& f : compilerOutput.includedFiles)
                {
                    relatedFiles.insert(f);
                }
            }
            else
            {
                // TODO: log error message
            }
        }

        const uint32 shaderIndex = uint32(id);
        if (succeed)
        {
            RenderBackendShaderDesc shaderDesc = {
                .code = compilerOutput.blob.GetData(),
                .codeSize = compilerOutput.blob.GetSize(),
                .entryFunctionName = desc.entryFunctionName
            };
            RenderBackendShaderHandle handle = renderBackend->CreateShader(&shaderDesc, desc.entryFunctionName);
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