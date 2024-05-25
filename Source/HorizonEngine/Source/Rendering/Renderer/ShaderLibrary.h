#pragma once

#include "RendererCommon.h"
#include "ShaderID_Deprecated.h"

namespace Horizon
{
    struct ShaderDesc
    {
        enum class Type
        {
            Compute,
            Graphics,
            Mesh,
            RayTracing,
        };

        Type type;
        std::string name;
        std::string filename;
        std::string entryPoints[(uint32)RenderBackendShaderStage::Count];
        std::vector<ShaderMacroDefine> defines;

        static ShaderDesc CreateCompute(const std::string& filename, const std::string& csMain)
        {
            ShaderDesc desc;
            desc.type = ShaderDesc::Type::Compute;
            desc.filename = filename;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Compute] = csMain;
            return desc;
        }

        static ShaderDesc CreateGraphics(const std::string& filename, const std::string& vsMain, const std::string& psMain)
        {
            ShaderDesc desc;
            desc.type = ShaderDesc::Type::Graphics;
            desc.filename = filename;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Vertex] = vsMain;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Pixel] = psMain;
            return desc;
        }

        static ShaderDesc CreateMesh(const std::string& filename, const std::string& tsMain, const std::string& msMain, const std::string& psMain)
        {
            ShaderDesc desc;
            desc.type = ShaderDesc::Type::Mesh;
            desc.filename = filename;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Task] = tsMain;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Mesh] = msMain;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Pixel] = psMain;
            return desc;
        }

        static ShaderDesc CreateMesh(const std::string& filename, const std::string& msMain, const std::string& psMain)
        {
            ShaderDesc desc;
            desc.type = ShaderDesc::Type::Mesh;
            desc.filename = filename;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Mesh] = msMain;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Pixel] = psMain;
            return desc;
        }

        static ShaderDesc CreateRayTracing()
        {
            // TODO
        }

        void AddDefine(const char* name, uint32 value)
        {
            ShaderMacroDefine& define = defines.emplace_back();
            define.name = name;
            define.value = std::format("{}", value);
        }
    };

    struct Shader
    {
        ShaderID id;
        ShaderDesc desc;
        RenderBackendShaderProgramHandle handle;
        std::vector<std::filesystem::path> relatedFiles;
        std::vector<std::chrono::time_point<std::chrono::file_clock>> lastModifiedTime;
        bool compiled = false;
    };

    class ShaderLibrary_Deprecated
    {
    public:
        ShaderLibrary_Deprecated(RenderBackend* backend, ShaderCompiler* compiler, uint32 maxNumShaders, bool hotReloadEnabled);
        virtual ~ShaderLibrary_Deprecated() {}
        bool HotReload();
        void AddIncludeDirectory(const char* dir);
        bool LoadShader(ShaderID id, ShaderDesc& desc);
        RenderBackendShaderProgramHandle GetShaderProgramHandle(ShaderID id);
        ShaderCompiler* shaderCompiler;
    private:
        bool hotReloadEnabled;
        uint32 maxNumShaders;
        RenderBackend* renderBackend;
        ShadingLanguage shadingLanguage;
        ShaderCompilerOptions shaderCompilerOptions;
        std::vector<const char*> includeDirs;
        std::vector<Shader> loadedShaders;
    };

    class ShaderLibrary
    {
    public:
    private:
        ShadingLanguage shadingLanguage;
    };

    void LoadShaderSourceFromFile(const char* filename, std::vector<uint8>& outData);

    // https://therealmjp.github.io/posts/shader-permutations-part1/
    // https://therealmjp.github.io/posts/shader-permutations-part2/

    //
    //class
    //{
    //public:
    //    class;
    //    class;
    //    static bool ShouldCompilePermutation()
    //    {

    //    }
    //}
}