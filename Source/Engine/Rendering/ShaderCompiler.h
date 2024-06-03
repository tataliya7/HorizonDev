#pragma once

#include "Core/CoreModule.h"

namespace HE
{
    enum class ShaderStage
    {
        Compute = 0,
        Vertex = 1,
        Pixel = 2,
        Task = 3,
        Mesh = 4,
        //RayGen = 5,
        //Miss = 6,
        //AnyHit = 7,
        //ClosestHit = 8,
        //Intersection = 9,
        Count = 5,
    };

    enum class ShadingLanguage
    {
        DXIL,
        SPIRV,
    };

    struct ShaderMacroDefine
    {
        std::string name;
        std::string value;
    };

    struct ShaderSource
    {
        const char* filename;
        const uint8* sourceData;
        uint64 sourceSize;
        const char* entryPoint;
        ShaderStage stage;
        const ShaderMacroDefine* defines;
        uint32 numDefines;
        const char** includeDirectories;
        uint32 numIncludeDirectories;
    };

    enum class HLSLShaderModel
    {
        ShaderModel_6_6,
        ShaderModel_6_7,
        ShaderModel_6_8,
    };

    struct HLSLShaderModelVersion
    {
        uint8 major;
        uint8 minor;
    };

    inline HLSLShaderModelVersion GetHLSLShaderModelVersion(HLSLShaderModel shaderModel)
    {
        HLSLShaderModelVersion version = { .major = 6, .minor = 6 };
        switch (shaderModel)
        {
        case HE::HLSLShaderModel::ShaderModel_6_6:
            version.major = 6;
            version.minor = 6;
            break;
        case HE::HLSLShaderModel::ShaderModel_6_7:
            version.major = 6;
            version.minor = 7;
            break;
        case HE::HLSLShaderModel::ShaderModel_6_8:
            version.major = 6;
            version.minor = 8;
            break;
        default:
            std::unreachable();
            break;
        }
        return version;
    }

    enum class ShaderOptimizationLevel
    {
        O0,
        O1,
        O2,
        O3,
    };

    struct ShaderCompilerSettings
    {
        bool generateDebugInfo = true;
        bool skipOptimization = true;
        bool warningAreErrors = false;
        bool enable16BitTypes = false;
        ShaderOptimizationLevel optimizationLevel = ShaderOptimizationLevel::O3;
        HLSLShaderModel shaderModel = HLSLShaderModel::ShaderModel_6_6;
    };

    class ShaderBlob
    {
    public:
        ShaderBlob()
            : data(nullptr), size(0) {}

        ~ShaderBlob()
        {
            Release();
        }

        ShaderBlob(ShaderBlob&) = delete;
        ShaderBlob(const ShaderBlob&) = delete;
        ShaderBlob& operator=(ShaderBlob&) = delete;
        ShaderBlob& operator=(const ShaderBlob&) = delete;

        bool IsValid() const
        {
            return data != nullptr && size > 0;
        }

        const uint8* GetData() const
        {
            return (uint8*)data;
        }

        uint64 GetSize() const
        {
            return size;
        }

        void Allocate(const void* srcData, uint64 srcSize)
        {
            assert(!IsValid() && srcData != nullptr && srcSize > 0);
            void* tmp = malloc(srcSize);
            if (tmp != nullptr)
            {
                memcpy(tmp, srcData, srcSize);
                data = tmp;
                size = srcSize;
            }
        }

        void Release()
        {
            if (IsValid())
            {
                delete data;
                data = nullptr;
                size = 0;
            }
        }

    protected:
        void* data;
        uint64 size;
    };

    struct ShaderCompilerOutput
    {
        ShaderBlob blob;
        std::unordered_set<std::string> includedFiles;
        std::string errorMessage;
    };

    class ShaderCompiler
    {
    public:
        virtual bool CompileShader(const ShaderCompilerSettings& settings, const ShaderSource& source, ShadingLanguage language, ShaderCompilerOutput* output) = 0;
    };
}