#pragma once

#include "Core/CoreModule.h"
#include "RenderBackend/RenderBackendModule.h"

namespace HE
{
    enum class ShaderIntermediateLanguage
    {
        DXIL,
        SPIRV,
    };

    class ShaderCompiler
    {
    public:
        virtual bool CompileShader(
            std::vector<uint8> source,
            const wchar_t* entry,
            RenderBackendShaderStage stage,
            ShaderIntermediateLanguage intermediateLanguage,
            const std::vector<std::wstring>& includeDirs,
            const std::vector<std::wstring>& defines,
            RenderBackendShaderBlob* outBlob,
            std::unordered_set<std::wstring>* outIncludedFiles = nullptr) = 0;
        virtual void ReleaseShaderBlob(void* instance, RenderBackendShaderBlob* blob) = 0;
    };

    void LoadShaderSourceFromFile(const char* filename, std::vector<uint8>& outData);
}