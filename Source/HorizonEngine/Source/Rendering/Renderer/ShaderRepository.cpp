#include "ShaderRepository.h"

namespace Horizon
{
    static ShaderRepository* GlobalShaderRepository = nullptr;

    ShaderRepository* GetGlobalShaderRepository()
    {
        assert(GlobalShaderRepository != nullptr);
        return GlobalShaderRepository;
    }

    static bool LoadShaderSourceFromFile(const char* filename, std::vector<uint8>& outData)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            return false;
        }
        size_t fileSize = (size_t)file.tellg();
        outData.resize(fileSize);
        file.seekg(0);
        file.read(reinterpret_cast<char*>(outData.data()), fileSize);
        file.close();
        return true;
    }

    ShaderRepository::ShaderRepository(RenderBackend* renderBackend, const std::string& rootDirectory)
    {
        this->renderBackend = renderBackend;
        this->rootDirectory = rootDirectory;
        this->shadingLanguage = (renderBackend->GetType() == RenderBackendType::Vulkan) ? ShadingLanguage::SPIRV : ShadingLanguage::DXIL;
        this->hotReloadEnabled = true;

        loadedShaders.resize((size_t)ShaderID::Count);
    }

    ShaderRepository::~ShaderRepository()
    {

    }

    bool ShaderRepository::HotReload()
    {
        OPTICK_EVENT();

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

    RenderBackendShaderHandle ShaderRepository::GetShader(ShaderID id) const
    {
        const uint32 shaderIndex = uint32(id);
        return loadedShaders[shaderIndex].handle;
    }

    bool ShaderRepository::LoadShader(ShaderID id, ShaderDesc& desc)
    {
        std::filesystem::path path = std::filesystem::absolute(std::filesystem::path("../../../Source/HorizonEngine").append(desc.filename));
        std::string filename = path.generic_string();
        std::string dir = path.parent_path().generic_string();

        std::vector<uint8> source;
        std::unordered_set<std::string> relatedFiles;

        relatedFiles.insert(filename);

        bool open = LoadShaderSourceFromFile(filename.c_str(), source);
        if (!open)
        {
            LogError(GLogger, std::format("Failed to open shader source file: {}.", filename));
            return false;
        }

        std::string rootDirectoryABS1 = std::filesystem::absolute(rootDirectory).generic_string();

        std::vector<const char*> includeDirectories;
        includeDirectories.push_back(rootDirectoryABS1.c_str());
        includeDirectories.push_back(dir.c_str());

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
            shaderSource.shaderModel = HLSLShaderModel::ShaderModel_6_6;

            ShaderCompiler* shaderCompiler = CreateDXCShaderCompiler();
            if (shaderCompiler)
            {
                succeed = shaderCompiler->CompileShader(desc.shaderCompilerOptions, shaderSource, shadingLanguage, &compilerOutput);
                DestroyDXCShaderCompiler(shaderCompiler);
            }

            if (succeed)
            {
                //LogInfo(GLogger, std::format("Shader compilation succeeded. Path: {}, Entry Point: {}.", filename, desc.entryFunctionName));
                for (const std::string& f : compilerOutput.includedFiles)
                {
                    relatedFiles.insert(f);
                }
            }
            else
            {
                LogError(GLogger, std::format("Shader compilation failed. Path: {}, Entry Point: {}, Message: {}.", filename, desc.entryFunctionName, compilerOutput.errorMessage));
                abort();
            }
        }

        const uint32 shaderIndex = uint32(id);
        if (succeed)
        {
            RenderBackendShaderDesc shaderDesc =
            {
                .stage = static_cast<RenderBackendShaderStage>(desc.stage),
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