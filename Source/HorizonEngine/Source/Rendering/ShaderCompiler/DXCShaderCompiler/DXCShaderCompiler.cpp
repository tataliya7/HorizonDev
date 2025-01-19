#include "../ShaderCompiler.h"
#include "DXCShaderCompiler.h"

#include <windows.h>
#include <wrl/client.h>

#include <dxcapi.h>
#include <dxcerrors.h>
#include <dxcisense.h>
#include <d3d12shader.h>

namespace DXCUtils
{
    std::wstring Widen(const std::string& input)
    {
        std::wstring result = {};
        if (input.length() > 0)
        {
            int length = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), int(input.size()), NULL, 0);
            if (length > 0)
            {
                result.resize(length);
                MultiByteToWideChar(CP_UTF8, 0, input.c_str(), int(input.size()), result.data(), int(result.size()));
            }
        }
        return result;
    }

    std::string Narrow(const std::wstring& input)
    {
        std::string result = {};
        if (input.length() > 0)
        {
            int length = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), int(input.size()), NULL, 0, NULL, NULL);
            if (length > 0)
            {
                result.resize(length);
                WideCharToMultiByte(CP_UTF8, 0, input.c_str(), int(input.size()), result.data(), int(result.size()), NULL, NULL);
            }
        }
        return result;
    }
}

/*
 * https://docs.vulkan.org/guide/latest/hlsl.html
 */

namespace Horizon
{
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
        bool CompileShader(const ShaderCompilerOptions& options, const ShaderSourceDescription& source, ShadingLanguage language, ShaderCompilerOutput* output) override;
    };

    bool DXCShaderCompiler::CompileShader(const ShaderCompilerOptions& options, const ShaderSourceDescription& source, ShadingLanguage language, ShaderCompilerOutput* output)
    {
        assert(output != nullptr);
        assert(output->blob.IsValid() == false);
        assert(output->errorMessage.empty() == true);
        assert(output->includedFiles.empty() == true);

        HRESULT hr = S_OK;

        Microsoft::WRL::ComPtr<IDxcCompilerArgs> dxcCompilerArgs = nullptr;
        hr = DxcCreateInstance(CLSID_DxcCompilerArgs, IID_PPV_ARGS(&dxcCompilerArgs));
        if (FAILED(hr))
        {
            output->errorMessage = std::format("Failed to create DxcCompilerArgs, HRESULT: {:#010x}.", uint32(hr));
            return false;
        }

        Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = nullptr;
        hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
        if (FAILED(hr))
        {
            output->errorMessage = std::format("Failed to create DxcUtils, HRESULT: {:#010x}.", uint32(hr));
            return false;
        }

        Microsoft::WRL::ComPtr<IDxcLibrary> dxcLibrary = nullptr;
        hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&dxcLibrary));
        if (FAILED(hr))
        {
            output->errorMessage = std::format("Failed to create DxcLibrary, HRESULT: {:#010x}.", uint32(hr));
            return false;
        }

        Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler = nullptr;
        hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
        if (FAILED(hr))
        {
            output->errorMessage = std::format("Failed to create DxcCompiler, HRESULT: {:#010x}.", uint32(hr));
            return false;
        }

        DXCShaderCompilerIncludeHandler includeHandler;
        hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler.defaultIncludeHandler);
        if (FAILED(hr))
        {
            output->errorMessage = std::format("Failed to create include handler, HRESULT: {:#010x}.", uint32(hr));
            return false;
        }

        HLSLShaderModelVersion shaderModelVersion = GetHLSLShaderModelVersion(source.shaderModel);

        std::wstring targetProfile;
        switch (source.stage)
        {
        case ShaderStage::Compute:
            targetProfile = std::format(L"cs_{}_{}", shaderModelVersion.major, shaderModelVersion.minor);
            break;
        case ShaderStage::Vertex:
            targetProfile = std::format(L"vs_{}_{}", shaderModelVersion.major, shaderModelVersion.minor);
            break;
        case ShaderStage::Pixel:
            targetProfile = std::format(L"ps_{}_{}", shaderModelVersion.major, shaderModelVersion.minor);
            break;
        case ShaderStage::Amplification:
            targetProfile = std::format(L"as_{}_{}", shaderModelVersion.major, shaderModelVersion.minor);
            break;
        case ShaderStage::Mesh:
            targetProfile = std::format(L"ms_{}_{}", shaderModelVersion.major, shaderModelVersion.minor);
            break;
        case ShaderStage::RayGen:
        case ShaderStage::Miss:
        case ShaderStage::AnyHit:
        case ShaderStage::ClosestHit:
        case ShaderStage::Intersection:
            targetProfile = std::format(L"lib_{}_{}", shaderModelVersion.major, shaderModelVersion.minor);
            break;
        default:
            std::unreachable();
            break;
        }

        std::wstring filename = DXCUtils::Widen(source.filename);

        std::wstring entryPoint = DXCUtils::Widen(source.entryPoint);

        std::vector<LPCWSTR> arguments =
        {
            filename.c_str(),
            L"-E", entryPoint.c_str(),
            L"-T", targetProfile.c_str(),
        };

        // Pack matrices in column-major order
        arguments.push_back(L"-Zpc");

        switch (language)
        {
        case ShadingLanguage::DXIL:
            //arguments.push_back(L"-Fd");
            arguments.push_back(L"-disable-payload-qualifiers");
            break;
        case ShadingLanguage::SPIRV:
            arguments.push_back(L"-spirv");
            arguments.push_back(L"-fspv-target-env=vulkan1.3");
            arguments.push_back(L"-fvk-use-scalar-layout");
            arguments.push_back(L"-fvk-use-dx-position-w");
            arguments.push_back(L"-fvk-allow-rwstructuredbuffer-arrays");
            break;
        default:
            std::unreachable();
            break;
        }

        if (options.generateDebugInfo)
        {
            arguments.push_back(L"-Zi");
            arguments.push_back(L"-Zss");
            arguments.push_back(L"-Qembed_debug");

            if (language == ShadingLanguage::SPIRV)
            {
                arguments.push_back(L"-fspv-debug=vulkan-with-source");
                //arguments.push_back(L"-fspv-print-all");
            }

            if (language == ShadingLanguage::DXIL)
            {
                // TODO
            }
        }

        if (options.skipOptimization)
        {
            arguments.push_back(L"-Od");
        }
        else
        {
            switch (options.optimizationLevel)
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

        if (options.warningAreErrors)
        {
            arguments.push_back(L"-WX");
        }

        if (options.enable16BitTypes)
        {
            arguments.push_back(L"-enable-16bit-types");
        }

        std::vector<std::wstring> includeDirectories(source.numIncludeDirectories);
        for (uint32 index = 0; index < source.numIncludeDirectories; index++)
        {
            includeDirectories[index] = DXCUtils::Widen(source.includeDirectories[index]);

            arguments.push_back(L"-I");
            arguments.push_back(includeDirectories[index].c_str());
        }

        std::vector<DxcDefine> dxcDefines(source.numDefines);
        std::vector<std::wstring> defineNames(source.numDefines);
        std::vector<std::wstring> defineValues(source.numDefines);
        for (uint32 index = 0; index < source.numDefines; index++)
        {
            defineNames[index] = DXCUtils::Widen(source.defines[index].name);
            defineValues[index] = DXCUtils::Widen(source.defines[index].value);

            dxcDefines[index].Name = defineNames[index].c_str();
            dxcDefines[index].Value = defineValues[index].c_str();
        }

        dxcCompilerArgs->AddArguments(arguments.data(), static_cast<UINT32>(arguments.size()));
        dxcCompilerArgs->AddDefines(dxcDefines.data(), static_cast<UINT32>(dxcDefines.size()));

        const DxcBuffer dxcBuffer =
        {
            .Ptr = static_cast<LPCVOID>(source.code),
            .Size = static_cast<SIZE_T>(source.codeSize),
            .Encoding = DXC_CP_UTF8
        };

        Microsoft::WRL::ComPtr<IDxcResult> dxcResult = nullptr;
        hr = dxcCompiler->Compile(&dxcBuffer, dxcCompilerArgs->GetArguments(), dxcCompilerArgs->GetCount(), &includeHandler, IID_PPV_ARGS(&dxcResult));
        assert(dxcResult != nullptr);

        Microsoft::WRL::ComPtr<IDxcBlobUtf8> dxcErrorBlob = nullptr;
        hr = dxcResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&dxcErrorBlob), nullptr);
        if (SUCCEEDED(hr) && (dxcErrorBlob != nullptr) && (dxcErrorBlob->GetStringLength() != 0))
        {
            output->errorMessage = std::string(dxcErrorBlob->GetStringPointer());
        }

        if (FAILED(hr))
        {
            return false;
        }

        HRESULT resultStatus = S_OK;
        hr = dxcResult->GetStatus(&resultStatus);
        if (FAILED(hr) || FAILED(resultStatus))
        {
            output->errorMessage += std::format("Failed to get result status, result status: {:#010x}, HRESULT: {:#010x}.", uint32(resultStatus), uint32(hr));
            return false;
        }

        Microsoft::WRL::ComPtr<IDxcBlob> dxcBlob = nullptr;
        hr = dxcResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&dxcBlob), nullptr);
        if (FAILED(hr))
        {
            output->errorMessage += std::format("Failed to get output, HRESULT: {:#010x}.", uint32(hr));
            return false;
        }

        // TODO: Manage memory allocation of shader blobs
        output->blob.Allocate(dxcBlob->GetBufferPointer(), dxcBlob->GetBufferSize());

        for (const std::wstring& dependency : includeHandler.dependencies)
        {
            output->includedFiles.insert(DXCUtils::Narrow(dependency));
        }

        return true;
    }

    ShaderCompiler* CreateDXCShaderCompiler()
    {
        DXCShaderCompiler* dxcCompiler = new DXCShaderCompiler();
        return dxcCompiler;
    }

    void DestroyDXCShaderCompiler(ShaderCompiler* compiler)
    {
        assert(dynamic_cast<DXCShaderCompiler*>(compiler) != nullptr);
        DXCShaderCompiler* dxcCompiler = static_cast<DXCShaderCompiler*>(compiler);
        delete dxcCompiler;
    }
}