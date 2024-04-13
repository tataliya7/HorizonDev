#include "Core/CoreModule.h"
#include "Rendering/ShaderCompiler.h"
#include "Rendering/DxcShaderCompiler/DxcShaderCompiler.h"

#include <windows.h>
#include <wrl/client.h>

#include <dxcapi.h>
#include <d3d12shader.h>
#include <dxcerrors.h>
#include <dxcisense.h>

namespace HE
{
    namespace DxcUtils
    {
        std::wstring Widen(const std::string& input)
        {
            std::wstring result = {};
            if (input.length() > 0)
            {
                int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input.c_str(), (int)input.size(), NULL, 0);
                if (length > 0)
                {
                    result.resize(length);
                    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input.c_str(), (int)input.size(), result.data(), (int)result.size());
                }
            }
            return result;
        }
    }

    struct DXCShaderCompilerIncludeHandler : public IDxcIncludeHandler
    {
        std::unordered_set<std::wstring> dependencies = {};

        Microsoft::WRL::ComPtr<IDxcIncludeHandler> defaultIncludeHandler = nullptr;

        HRESULT STDMETHODCALLTYPE LoadSource(
            _In_z_ LPCWSTR pFilename,                                 // Candidate filename.
            _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource  // Resultant source object for included file, nullptr if not found.
        ) override
        {
            HRESULT hr = defaultIncludeHandler->LoadSource(pFilename, ppIncludeSource);
            if (SUCCEEDED(hr))
            {
                dependencies.insert(pFilename);
            }
            return hr;
        }
        HRESULT STDMETHODCALLTYPE QueryInterface(
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override
        {
            return defaultIncludeHandler->QueryInterface(riid, ppvObject);
        }
        ULONG STDMETHODCALLTYPE AddRef(void) override
        {
            return 0;
        }
        ULONG STDMETHODCALLTYPE Release(void) override
        {
            return 0;
        }
    };

    class DXCShaderCompiler : public ShaderCompiler
    {
    public:
        DXCShaderCompiler()
        {

        }
        virtual ~DXCShaderCompiler()
        {

        }
        bool CompileShader_Depreacated(
            std::vector<uint8> source,
            const wchar_t* entry,
            RenderBackendShaderStage stage,
            ShadingLanguage intermediateLanguage,
            const std::vector<std::wstring>& includeDirs,
            const std::vector<std::wstring>& defines,
            RenderBackendShaderBlob* outBlob,
            std::unordered_set<std::wstring>* outIncludedFiles = nullptr) override;
        void ReleaseShaderBlob(void* instance, RenderBackendShaderBlob* blob) override;
        bool CompileShader(const ShaderCompilerSettings& settings, const ShaderSource& source, ShadingLanguage language, ShaderCompilerOutput* output) override;
    };

    bool DXCShaderCompiler::CompileShader_Depreacated(
        std::vector<uint8> source,
        const wchar_t* entry,
        RenderBackendShaderStage stage,
        ShadingLanguage intermediateLanguage,
        const std::vector<std::wstring>& includeDirs,
        const std::vector<std::wstring>& defines,
        RenderBackendShaderBlob* outBlob,
        std::unordered_set<std::wstring>* outIncludedFiles)
    {
        Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = nullptr;
        Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler = nullptr;
        HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));

        if (FAILED(hr))
        {
            return false;
        }
        hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
        if (FAILED(hr))
        {
            return false;
        }

        static const LPCWSTR targetProfiles[] = {
            TEXT("vs_6_6"),
            TEXT("ps_6_6"),
            TEXT("cs_6_6"),
            TEXT("lib_6_3"), // raygen
            TEXT("lib_6_3"), // any hit
            TEXT("lib_6_3"), // closest hit
            TEXT("lib_6_3"), // miss
            TEXT("lib_6_3"), // intersection
            TEXT("as_6_6"),  // task
            TEXT("ms_6_6"),  // mesh
        };

        //Microsoft::WRL::ComPtr<IDxcCompilerArgs> args;
        //HRESULT hr = DxcCreateInstance(CLSID_DxcCompilerArgs, IID_PPV_ARGS(&args));
        //if (FAILED(hr))
        //{
        //    return false;
        //}

        //// TODO: Source Level Debugging with HLSL
        //// https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/SourceLevelDebuggingHLSL.rst#using-debug-names
        ////static const WCHAR* compilerArgs[] = { TEXT("-P"), TEXT("-Zi"), TEXT("-Fd"), TEXT("-Zss") };
        //static const WCHAR* compilerArgs[] = { TEXT("-P") };
        //static const DxcDefine compilerDefines[] = { { TEXT("DXC"), nullptr} };
        //hr = args->AddArguments(compilerArgs, ARRAY_SIZE(compilerArgs));
        //ASSERT(SUCCEEDED(hr));

        Microsoft::WRL::ComPtr<IDxcCompilerArgs> dxcCompilerArgs;
        hr = DxcCreateInstance(CLSID_DxcCompilerArgs, IID_PPV_ARGS(&dxcCompilerArgs));
        if (FAILED(hr))
        {
            return false;
        }

        std::vector<LPCWSTR> args = {};
        switch (intermediateLanguage)
        {
        case ShadingLanguage::DXIL:
            args.push_back(TEXT("-Fd"));
            break;
        case ShadingLanguage::SPIRV:
            args.push_back(TEXT("-spirv"));
            args.push_back(TEXT("-fspv-target-env=vulkan1.3"));
            //args.push_back(TEXT("-fspv-use-legacy-buffer-matrix-order"));
            args.push_back(TEXT("-fvk-use-scalar-layout"));
            args.push_back(TEXT("-fvk-use-dx-position-w"));
            break;
        default:
            std::unreachable();
            return false;
        }

        if (true)
        {
            args.push_back(TEXT("-fspv-debug=vulkan-with-source"));
        }

        args.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX

        if (true)
        {
            args.push_back(DXC_ARG_DEBUG); //-Zi
            //args.push_back(DXC_ARG_SKIP_OPTIMIZATIONS); //-Od
            // Cannot specify both /Zss and /Zsb
            args.push_back(DXC_ARG_DEBUG_NAME_FOR_SOURCE); //-Zss
            //args.push_back(DXC_ARG_DEBUG_NAME_FOR_BINARY); //-Zsb
        }

        if (true)
        {
            //args.push_back(DXC_ARG_OPTIMIZATION_LEVEL3); //-O3
        }

        args.push_back(TEXT("-T"));
        args.push_back(targetProfiles[(uint32)stage]);

        args.push_back(TEXT("-E"));
        args.push_back(entry);

        for (auto& includeDir : includeDirs)
        {
            args.push_back(TEXT("-I"));
            args.push_back(includeDir.c_str());
        }
        for (auto& define : defines)
        {
            args.push_back(TEXT("-D"));
            args.push_back(define.c_str());
        }

        const bool isRaytracingStage = ((stage >= RenderBackendShaderStage::RayGen) && (stage <= RenderBackendShaderStage::Intersection));

        const DxcBuffer buffer = {
            .Ptr = source.data(),
            .Size = source.size(),
            .Encoding = DXC_CP_UTF8
        };

        struct IncludeHandler : public IDxcIncludeHandler
        {
            std::unordered_set<std::wstring> dependencies;

            Microsoft::WRL::ComPtr<IDxcIncludeHandler> dxcIncludeHandler;

            HRESULT STDMETHODCALLTYPE LoadSource(
                _In_z_ LPCWSTR pFilename,                                 // Candidate filename.
                _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource  // Resultant source object for included file, nullptr if not found.
            ) override
            {
                HRESULT hr = dxcIncludeHandler->LoadSource(pFilename, ppIncludeSource);
                if (SUCCEEDED(hr))
                {
                    dependencies.insert(pFilename);
                }
                return hr;
            }
            HRESULT STDMETHODCALLTYPE QueryInterface(
                /* [in] */ REFIID riid,
                /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override
            {
                return dxcIncludeHandler->QueryInterface(riid, ppvObject);
            }
            ULONG STDMETHODCALLTYPE AddRef(void) override
            {
                return 0;
            }
            ULONG STDMETHODCALLTYPE Release(void) override
            {
                return 0;
            }
        };

        std::string errorMessage;

        IncludeHandler includeHandler;
        hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler.dxcIncludeHandler);
        ASSERT(SUCCEEDED(hr));

        //Microsoft::WRL::ComPtr<IDxcIncludeHandler> pIncludeHandler;
        //hr = dxcUtils->CreateDefaultIncludeHandler(&pIncludeHandler);
        //ASSERT(SUCCEEDED(hr));

        // TODO: fix hlsl::Exception
        Microsoft::WRL::ComPtr<IDxcResult> dxcResult;
        hr = dxcCompiler->Compile(&buffer, args.data(), (uint32)args.size(), &includeHandler, IID_PPV_ARGS(&dxcResult));
        ASSERT(SUCCEEDED(hr));

        Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors;
        hr = dxcResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
        ASSERT(SUCCEEDED(hr));
        if (errors != nullptr && errors->GetStringLength() != 0)
        {
            errorMessage = errors->GetStringPointer();
            LogError(GLogger, std::format("Shader compilation failed! Message: {}", errorMessage.c_str()));
        }

        dxcResult->GetStatus(&hr);
        if (SUCCEEDED(hr))
        {
            Microsoft::WRL::ComPtr<IDxcBlob> blob;
            hr = dxcResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&blob), nullptr);
            ASSERT(SUCCEEDED(hr));

            outBlob->size = blob->GetBufferSize();
            outBlob->data = (uint8*)malloc(outBlob->size);
            memcpy(outBlob->data, blob->GetBufferPointer(), outBlob->size);
        }

        if (outIncludedFiles)
        {
            outIncludedFiles->merge(includeHandler.dependencies);
        }

        return SUCCEEDED(hr);
    }

    void DXCShaderCompiler::ReleaseShaderBlob(void* instance, RenderBackendShaderBlob* blob)
    {
        delete(blob->data);
    }

    bool DXCShaderCompiler::CompileShader(const ShaderCompilerSettings& settings, const ShaderSource& source, ShadingLanguage language, ShaderCompilerOutput* output)
    {
        assert(output != nullptr);

        HRESULT hr = S_OK;

        Microsoft::WRL::ComPtr<IDxcCompilerArgs> dxcCompilerArgs = nullptr;
        hr = DxcCreateInstance(CLSID_DxcCompilerArgs, IID_PPV_ARGS(&dxcCompilerArgs));
        if (FAILED(hr))
        {
            output->errorMessage = std::format("Failed to create DxcCompilerArgs, HRESULT: {:#010x}.", (uint32)hr);
            return false;
        }

        Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = nullptr;
        hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
        if (FAILED(hr))
        {
            output->errorMessage = std::format("Failed to create DxcUtils, HRESULT: {:#010x}.", (uint32)hr);
            return false;
        }

        Microsoft::WRL::ComPtr<IDxcLibrary> dxcLibrary = nullptr;
        hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&dxcLibrary));
        if (FAILED(hr))
        {
            output->errorMessage = std::format("Failed to create DxcLibrary, HRESULT: {:#010x}.", (uint32)hr);
            return false;
        }

        Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler = nullptr;
        hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
        if (FAILED(hr))
        {
            output->errorMessage = std::format("Failed to create DxcCompiler, HRESULT: {:#010x}.", (uint32)hr);
            return false;
        }

        DXCShaderCompilerIncludeHandler includeHandler;
        hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler.defaultIncludeHandler);
        if (FAILED(hr))
        {
            output->errorMessage = std::format("Failed to create include handler, HRESULT: {:#010x}.", (uint32)hr);
            return false;
        }

        HLSLShaderModelVersion shaderTargetVersion = GetHLSLShaderModelVersion(settings.shaderModel);

        std::wstring targetProfile = {};
        switch (source.stage)
        {
        case ShaderStage::Compute:
            targetProfile = std::format(L"cs_{}_{}", shaderTargetVersion.major, shaderTargetVersion.minor);
            break;
        case ShaderStage::Vertex:
            targetProfile = std::format(L"vs_{}_{}", shaderTargetVersion.major, shaderTargetVersion.minor);
            break;
        case ShaderStage::Pixel:
            targetProfile = std::format(L"ps_{}_{}", shaderTargetVersion.major, shaderTargetVersion.minor);
            break;
        case ShaderStage::Task:
            targetProfile = std::format(L"as_{}_{}", shaderTargetVersion.major, shaderTargetVersion.minor);
            break;
        case ShaderStage::Mesh:
            targetProfile = std::format(L"ms_{}_{}", shaderTargetVersion.major, shaderTargetVersion.minor);
            break;
        default:
            std::unreachable();
            break;
        }
#if 1
        std::wstring filename = DxcUtils::Widen(source.filename);
        std::wstring entryPoint = DxcUtils::Widen(source.entryPoint);
        std::vector<LPCWSTR> arguments =
        {
            filename.c_str(),
            L"-E", entryPoint.c_str(),
            L"-T", targetProfile.c_str(),
        };

        switch (language)
        {
        case ShadingLanguage::DXIL:
            arguments.push_back(L"-Fd");
            break;
        case ShadingLanguage::SPIRV:
            arguments.push_back(L"-spirv");
            arguments.push_back(L"-fspv-target-env=vulkan1.3");
            arguments.push_back(L"-fvk-use-scalar-layout");
            arguments.push_back(L"-fvk-use-dx-position-w");
            break;
        default:
            std::unreachable();
            break;
        }

        if (settings.generateDebugInfo)
        {
            arguments.push_back(L"-Zi");
            arguments.push_back(L"-Zss");
            arguments.push_back(L"-Qembed_debug");

            if (language == ShadingLanguage::SPIRV)
            {
                arguments.push_back(L"-fspv-debug=vulkan-with-source");
            }

            if (language == ShadingLanguage::DXIL)
            {
                // TODO
            }
        }

        if (settings.skipOptimization)
        {
            arguments.push_back(L"-Od");
        }
        else
        {
            switch (settings.optimizationLevel)
            {
            case ShaderOptimizationLevel::O0:
                arguments.push_back(L"-O0");
                break;
            case ShaderOptimizationLevel::O1:
                arguments.push_back(L"-O1");
                break;
            case ShaderOptimizationLevel::O2:
                arguments.push_back(L"-O2");
                break;
            case ShaderOptimizationLevel::O3:
                arguments.push_back(L"-O3");
                break;
            default:
                std::unreachable();
                break;
            }
        }

        if (settings.warningAreErrors)
        {
            arguments.push_back(L"-WX");
        }

        if (settings.enable16BitTypes)
        {
            arguments.push_back(L"-enable-16bit-types");
        }

        std::vector<std::wstring> includeDirectories(source.numIncludeDirectories);
        for (uint32 index = 0; index < source.numIncludeDirectories; index++)
        {
            includeDirectories[index] = DxcUtils::Widen(source.includeDirectories[index]);

            arguments.push_back(L"-I");
            arguments.push_back(includeDirectories[index].c_str());
        }

        std::vector<DxcDefine> dxcDefines(source.numDefines);
        std::vector<std::wstring> defineNames(source.numDefines);
        std::vector<std::wstring> defineValues(source.numDefines);
        for (uint32 index = 0; index < source.numDefines; index++)
        {
            defineNames[index] = DxcUtils::Widen(source.defines[index].name);
            defineValues[index] = DxcUtils::Widen(source.defines[index].value);

            dxcDefines[index].Name = defineNames[index].c_str();
            dxcDefines[index].Value = defineValues[index].c_str();
        }

        dxcCompilerArgs->AddArguments(arguments.data(), (uint32)arguments.size());
        dxcCompilerArgs->AddDefines(dxcDefines.data(), (uint32)dxcDefines.size());

        const DxcBuffer dxcBuffer = {
            .Ptr = (LPCVOID)source.sourceData,
            .Size = (SIZE_T)source.sourceSize,
            .Encoding = DXC_CP_UTF8
        };

        Microsoft::WRL::ComPtr<IDxcResult> dxcResult = nullptr;
 /*       hr = dxcCompiler->Compile(&dxcBuffer, arguments.data(), (uint32)arguments.size(), &includeHandler, IID_PPV_ARGS(&dxcResult));
        if (SUCCEEDED(hr))
        {
            HRESULT resultStatus = S_OK;
            assert(SUCCEEDED(dxcResult->GetStatus(&resultStatus)));
            if (SUCCEEDED(resultStatus))
            {
                Microsoft::WRL::ComPtr<IDxcBlob> dxcBlob = nullptr;
                assert(SUCCEEDED(dxcResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&dxcBlob), nullptr)));
                outBlob->size = dxcBlob->GetBufferSize();
                outBlob->data = (uint8*)malloc(dxcBlob->size);
            }
            else
            {
                return false;
            }
        }
        else if (result)
        {
            Microsoft::WRL::ComPtr<IDxcBlobEncoding> errorBlob;
            hres = result->GetErrorBuffer(&errorBlob);
            if (SUCCEEDED(hres) && errorBlob) {
                std::cerr << "Shader compilation failed :\n\n" << (const char*)errorBlob->GetBufferPointer();
                throw std::runtime_error("Compilation failed");
            }
        }*/

#endif
        return true;
    }

    ShaderCompiler* CreateDXCShaderCompiler()
    {
        DXCShaderCompiler* dxcCompiler = new DXCShaderCompiler();
        return dxcCompiler;
    }

    void DestroyDXCShaderCompiler(ShaderCompiler* compiler)
    {
        DXCShaderCompiler* dxcCompiler = (DXCShaderCompiler*)compiler;
        delete dxcCompiler;
    }
}