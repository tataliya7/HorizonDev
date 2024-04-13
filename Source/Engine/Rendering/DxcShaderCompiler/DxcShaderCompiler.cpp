#include "Core/CoreModule.h"
#include "Rendering/ShaderCompiler.h"
#include "Rendering/DxcShaderCompiler/DxcShaderCompiler.h"

#include <windows.h>
#include <atlbase.h> // TODO: remove this, UE5 will ruin the environment, you must install the specific VS package

#include <dxcapi.h>
#include <d3d12shader.h>
#include <dxcerrors.h>
#include <dxcisense.h>

namespace hlsl {

    /// <summary>
    /// Exception stores off information about an error and its error message for
    /// later consumption by the hlsl compiler tools.
    /// </summary>
    struct Exception : public std::exception {
        /// <summary>HRESULT error code. Must be a failure.</summary>
        HRESULT hr;
        std::string msg;

        Exception(HRESULT errCode) : hr(errCode) {}
        Exception(HRESULT errCode, const std::string& errMsg)
            : hr(errCode), msg(errMsg) {}

        // what returns a formatted message with the error code and the message used
        // to create the message.
        virtual const char* what() const throw() { return msg.c_str(); }
    };

} // namespace hlsl

namespace HE
{
    class DxcShaderCompiler : public ShaderCompiler
    {
    public:
        DxcShaderCompiler(CComPtr<IDxcCompilerArgs> args)
            : args(args)
        {

        }
        virtual ~DxcShaderCompiler()
        {
            if (args != nullptr)
            {
               args.Release();
            }
        }
        bool CompileShader(
            std::vector<uint8> source,
            const wchar_t* entry,
            RenderBackendShaderStage stage,
            ShaderIntermediateLanguage intermediateLanguage,
            const std::vector<std::wstring>& includeDirs,
            const std::vector<std::wstring>& defines,
            RenderBackendShaderBlob* outBlob,
            std::unordered_set<std::wstring>* outIncludedFiles = nullptr) override;
        void ReleaseShaderBlob(void* instance, RenderBackendShaderBlob* blob) override;
    private:
        CComPtr<IDxcCompilerArgs> args;
    };

    int filter(unsigned int code, struct _EXCEPTION_POINTERS* pExceptionInfo) {
        static char scratch[32];
        // report all errors with fputs to prevent any allocation
        if (code == EXCEPTION_ACCESS_VIOLATION) {
            // use pExceptionInfo to document and report error
            fputs("access violation. Attempted to ", stderr);
            if (pExceptionInfo->ExceptionRecord->ExceptionInformation[0])
                fputs("write", stderr);
            else
                fputs("read", stderr);
            fputs(" from address ", stderr);
            sprintf_s(scratch, _countof(scratch), "0x%p\n",
                (void*)pExceptionInfo->ExceptionRecord->ExceptionInformation[1]);
            fputs(scratch, stderr);
            return EXCEPTION_EXECUTE_HANDLER;
        }
        if (code == EXCEPTION_STACK_OVERFLOW) {
            // use pExceptionInfo to document and report error
            fputs("stack overflow\n", stderr);
            return EXCEPTION_EXECUTE_HANDLER;
        }
        fputs("Unrecoverable Error ", stderr);
        sprintf_s(scratch, _countof(scratch), "0x%08x\n", code);
        fputs(scratch, stderr);
        return EXCEPTION_CONTINUE_SEARCH;
    }

    HRESULT Compile(IDxcCompiler3* pCompiler, const DxcBuffer* pSource, LPCWSTR* pszArgs,
        UINT32 argCt, IDxcIncludeHandler* pIncludeHandler, IDxcResult** pResults) {
        try {
            return pCompiler->Compile(
                pSource,                // Source buffer.
                pszArgs,                // Array of pointers to arguments.
                argCt,                  // Number of arguments.
                pIncludeHandler,        // User-provided interface to handle #include directives (optional).
                IID_PPV_ARGS(pResults) // Compiler output status, buffer, and errors.
            );
        }
        catch (const hlsl::Exception& hlslException) {
            // UNRECOVERABLE ERROR!
            // At this point, state could be extremely corrupt. Terminate the process
            printf("%s\n", hlslException.what());
            return E_FAIL;
        }
    }

    bool DxcShaderCompiler::CompileShader(
        std::vector<uint8> source,
        const wchar_t* entry,
        RenderBackendShaderStage stage,
        ShaderIntermediateLanguage intermediateLanguage,
        const std::vector<std::wstring>& includeDirs,
        const std::vector<std::wstring>& defines,
        RenderBackendShaderBlob* outBlob,
        std::unordered_set<std::wstring>* outIncludedFiles)
    {
        assert(args);

        CComPtr<IDxcUtils> dxcUtils;
        CComPtr<IDxcCompiler3> dxcCompiler;
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

        std::vector<LPCWSTR> args = {};
        switch (intermediateLanguage)
        {
        case ShaderIntermediateLanguage::DXIL:
            args.push_back(TEXT("-Fd"));
            break;
        case ShaderIntermediateLanguage::SPIRV:
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
            args.push_back(DXC_ARG_SKIP_OPTIMIZATIONS); //-0d
            // Cannot specify both /Zss and /Zsb
            args.push_back(DXC_ARG_DEBUG_NAME_FOR_SOURCE); //-Zss
            //args.push_back(DXC_ARG_DEBUG_NAME_FOR_BINARY); //-Zsb
        }

        if (true)
        {
            //args.push_back(DXC_ARG_OPTIMIZATION_LEVEL3); //-O3
            //args.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX
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

        switch (stage)
        {
        case HE::RenderBackendShaderStage::Vertex:
            args.push_back(TEXT("-D VERTEX_SHADER"));
            break;
        case HE::RenderBackendShaderStage::Pixel:
            args.push_back(TEXT("-D PIXEL_SHADER"));
            break;
        case HE::RenderBackendShaderStage::Compute:
            args.push_back(TEXT("-D COMPUTE_SHADER"));
            break;
        case HE::RenderBackendShaderStage::RayGen:
            break;
        case HE::RenderBackendShaderStage::AnyHit:
            break;
        case HE::RenderBackendShaderStage::ClosestHit:
            break;
        case HE::RenderBackendShaderStage::Miss:
            break;
        case HE::RenderBackendShaderStage::Intersection:
            break;
        case HE::RenderBackendShaderStage::Task:
            args.push_back(TEXT("-D TASK_SHADER"));
            break;
        case HE::RenderBackendShaderStage::Mesh:
            args.push_back(TEXT("-D MESH_SHADER"));
            break;
        default:
            break;
        }

        const bool isRaytracingStage = ((stage >= RenderBackendShaderStage::RayGen) && (stage <= RenderBackendShaderStage::Intersection));

        const DxcBuffer buffer = {
            .Ptr = source.data(),
            .Size = source.size(),
            .Encoding = DXC_CP_ACP
        };

        struct IncludeHandler : public IDxcIncludeHandler
        {
            std::unordered_set<std::wstring> dependencies;

            CComPtr<IDxcIncludeHandler> dxcIncludeHandler;

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

        //CComPtr<IDxcIncludeHandler> pIncludeHandler;
        //hr = dxcUtils->CreateDefaultIncludeHandler(&pIncludeHandler);
        //ASSERT(SUCCEEDED(hr));

        // TODO: fix hlsl::Exception
        CComPtr<IDxcResult> dxcResult;
        hr = Compile(dxcCompiler, &buffer, args.data(), (uint32)args.size(), &includeHandler, &dxcResult);
        ASSERT(SUCCEEDED(hr));

        CComPtr<IDxcBlobUtf8> errors;
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
            CComPtr<IDxcBlob> blob;
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

    void DxcShaderCompiler::ReleaseShaderBlob(void* instance, RenderBackendShaderBlob* blob)
    {
        delete(blob->data);
    }

    ShaderCompiler* CreateDxcShaderCompiler()
    {
        CComPtr<IDxcCompilerArgs> args;
        HRESULT hr = DxcCreateInstance(CLSID_DxcCompilerArgs, IID_PPV_ARGS(&args));
        if (FAILED(hr))
        {
            return nullptr;
        }

        // TODO: Source Level Debugging with HLSL
        // https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/SourceLevelDebuggingHLSL.rst#using-debug-names
        //static const WCHAR* compilerArgs[] = { TEXT("-P"), TEXT("-Zi"), TEXT("-Fd"), TEXT("-Zss") };
        static const WCHAR* compilerArgs[] = { TEXT("-P") };
        static const DxcDefine compilerDefines[] = { { TEXT("DXC"), nullptr} };
        hr = args->AddArguments(compilerArgs, ARRAY_SIZE(compilerArgs));
        ASSERT(SUCCEEDED(hr));
        hr = args->AddDefines(compilerDefines, ARRAY_SIZE(compilerDefines));
        ASSERT(SUCCEEDED(hr));

        DxcShaderCompiler* dxcCompiler = new DxcShaderCompiler(args);
        return dxcCompiler;
    }

    void DestroyDxcShaderCompiler(ShaderCompiler* compiler)
    {
        DxcShaderCompiler* dxcCompiler = (DxcShaderCompiler*)compiler;
        delete dxcCompiler;
    }
}