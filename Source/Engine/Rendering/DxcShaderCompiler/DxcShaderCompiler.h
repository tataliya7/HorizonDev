#pragma once

namespace HE
{
    class ShaderCompiler;
    extern ShaderCompiler* CreateDxcShaderCompiler();
    extern void DestroyDxcShaderCompiler(ShaderCompiler* compiler);
}