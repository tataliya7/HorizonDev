#pragma once

#include "Core/CoreModule.h"
#include "RenderBackend/RenderBackendModule.h"
#include "Rendering/ShaderCompiler.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/ShaderID_DEPRECATED.h"

namespace Horizon
{
    struct ShaderDesc
    {
        enum class Type
        {
            Graphics,
            Compute,
            RayTracing,
        };

        Type type;
        std::string name;
        std::string filenames[(uint32)RenderBackendShaderStage::Count];
        std::string entryPoints[(uint32)RenderBackendShaderStage::Count];
        std::vector<ShaderMacroDefine> defines;

        static ShaderDesc CreateGraphics(const std::string& vsFile, const std::string& vsMain, const std::string& psFile, const std::string& psMain)
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
            desc.type = ShaderDesc::Type::Graphics;
            desc.filename = filename;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Task] = tsMain;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Mesh] = msMain;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Pixel] = psMain;
            return desc;
        }

        static ShaderDesc CreateMesh(const std::string& filename, const std::string& msMain, const std::string& psMain)
        {
            ShaderDesc desc;
            desc.type = ShaderDesc::Type::Graphics;
            desc.filename = filename;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Mesh] = msMain;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Pixel] = psMain;
            return desc;
        }

        static ShaderDesc CreateCompute(const std::string& filename, const std::string& csMain)
        {
            ShaderDesc desc;
            desc.type = ShaderDesc::Type::Compute;
            desc.filename = filename;
            desc.entryPoints[(uint32)RenderBackendShaderStage::Compute] = csMain;
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
        ShaderDesc desc;
        RenderBackendShaderHandle handle;
        std::vector<std::filesystem::path> relatedFiles;
        std::vector<std::chrono::time_point<std::chrono::file_clock>> lastModifiedTime;
        bool compiled = false;
    };

    class ShaderLibrary_DEPRECATED
    {
    public:
        ShaderLibrary_DEPRECATED(RenderBackend* backend, ShaderCompiler* compiler, uint32 maxNumShaders, bool hotReloadEnabled);
        virtual ~ShaderLibrary_DEPRECATED() {}
        bool HotReload();
        void AddIncludeDirectory(const char* dir);
        bool LoadShader(ShaderID id, ShaderDesc& desc, bool reload = false);
        RenderBackendShaderHandle GetShaderHandle(ShaderID id);
        ShaderCompiler* shaderCompiler;
    private:
        bool hotReloadEnabled;
        uint32 maxNumShaders;
        RenderBackend* renderBackend;
        std::vector<const char*> includeDirs;
        std::vector<Shader> loadedShaders;
    };

    void LoadShaderSourceFromFile(const char* filename, std::vector<uint8>& outData);

    //class ShaderLibrary
    //{
    //public:
    //};

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