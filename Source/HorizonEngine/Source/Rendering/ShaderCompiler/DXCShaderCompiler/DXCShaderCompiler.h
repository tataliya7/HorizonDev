#pragma once

namespace Horizon
{
    class ShaderCompiler;

    extern ShaderCompiler* CreateDXCShaderCompiler();

    extern void DestroyDXCShaderCompiler(ShaderCompiler* compiler);
}
