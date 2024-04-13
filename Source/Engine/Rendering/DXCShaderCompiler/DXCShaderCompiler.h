#pragma once

namespace HE
{
    class ShaderCompiler;
    extern ShaderCompiler* CreateDXCShaderCompiler();
    extern void DestroyDXCShaderCompiler(ShaderCompiler* compiler);
}