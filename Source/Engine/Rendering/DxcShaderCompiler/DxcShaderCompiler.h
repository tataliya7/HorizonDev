#pragma once

namespace HE
{
    enum class HLSLShaderModel
    {
        ShaderModel_6_6,
        ShaderModel_6_7,
        ShaderModel_6_8,
    };

    class ShaderCompiler;
    extern ShaderCompiler* CreateDxcShaderCompiler();
    extern void DestroyDxcShaderCompiler(ShaderCompiler* compiler);
}