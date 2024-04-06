#pragma once

#include "Core/CoreModule.h"
#include "RenderBackend/RenderBackendModule.h"
#include "Rendering/ShaderCompiler.h"

namespace HE
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
        std::filesystem::path filename;
        std::string entryPoints[(uint32)RenderBackendShaderStage::Count];
        std::vector<std::wstring> defines;

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

        void AddDefine(const wchar_t* define)
        {
            defines.push_back(define);
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

    class ShaderLibrary_Deprecated
    {
    public:
        ShaderLibrary_Deprecated(RenderBackend* backend, ShaderCompiler* compiler, uint32 maxNumShaders, bool hotReloadEnabled);
        virtual ~ShaderLibrary_Deprecated() {}
        bool HotReload();
        void AddIncludeDirectory(const wchar_t* dir);
        bool LoadShader(uint32 id, ShaderDesc& desc, bool reload = false);
        RenderBackendShaderHandle GetShaderHandle(uint32 id);
        ShaderCompiler* shaderCompiler;
    private:
        bool hotReloadEnabled;
        uint32 maxNumShaders;
        RenderBackend* renderBackend;
        std::vector<std::wstring> includeDirs;
        std::vector<Shader> loadedShaders;
    };

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