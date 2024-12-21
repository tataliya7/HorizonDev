#pragma once

#include "ShaderCompilerCommon.h"

namespace Horizon
{
    enum class ShaderStage
    {
        Compute = 0,
        Vertex = 1,
        Pixel = 2,
        Amplification = 3,
        Mesh = 4,
        RayGen = 5,
        Miss = 6,
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
        case HLSLShaderModel::ShaderModel_6_6:
            version.major = 6;
            version.minor = 6;
            break;
        case HLSLShaderModel::ShaderModel_6_7:
            version.major = 6;
            version.minor = 7;
            break;
        case HLSLShaderModel::ShaderModel_6_8:
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

    struct ShaderCompilerOptions
    {
        bool generateDebugInfo;
        bool skipOptimization;
        bool warningAreErrors;
        bool enable16BitTypes;
        bool inlineRayTracing;
        ShaderOptimizationLevel optimizationLevel;
    };

    struct ShaderMacroDefine
    {
        std::string name;
        std::string value;
    };

    struct ShaderSourceDescription
    {
        const char* filename;
        const char* entryPoint;
        ShaderStage stage;
        const uint8* code;
        uint64 codeSize;
        const ShaderMacroDefine* defines;
        uint32 numDefines;
        const char** includeDirectories;
        uint32 numIncludeDirectories;
        HLSLShaderModel shaderModel;
    };

    class ShaderBlob
    {
    public:
        ShaderBlob()
            : data(nullptr)
            , size(0) {}

        ~ShaderBlob()
        {
            Release();
        }

        ShaderBlob(ShaderBlob&&) = delete;
        ShaderBlob(const ShaderBlob&) = delete;
        ShaderBlob& operator=(ShaderBlob&&) = delete;
        ShaderBlob& operator=(const ShaderBlob&) = delete;

        bool IsValid() const
        {
            return data != nullptr && size > 0;
        }

        const uint8* GetData() const
        {
            return static_cast<uint8*>(data);
        }

        uint64 GetSize() const
        {
            return size;
        }

        void Allocate(const void* srcData, uint64 srcSize);

        void Release();

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

        /**
         * Compile a single entry point.
         */
        virtual bool CompileShader(const ShaderCompilerOptions& options, const ShaderSourceDescription& source, ShadingLanguage language, ShaderCompilerOutput* output) = 0;

        /**
         * Experimental: Compile a library.
         */
        //virtual bool CompileShaderModule(const ShaderCompilerOptions& options, const ShaderModuleDescription& module, ShadingLanguage language, ShaderCompilerOutput* output) = 0;
    };
}