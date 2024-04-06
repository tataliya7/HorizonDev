project "meshoptimizer"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"
    location "%{wks.location}/Build/%{prj.name}"
    targetdir "%{wks.location}/Build/Bin/%{cfg.buildcfg}"

    files {
        "meshoptimizer-0.20/src/meshoptimizer.h",
        "meshoptimizer-0.20/src/allocator.cpp",
        "meshoptimizer-0.20/src/clusterizer.cpp",
        "meshoptimizer-0.20/src/indexcodec.cpp",
        "meshoptimizer-0.20/src/indexgenerator.cpp",
        "meshoptimizer-0.20/src/overdrawanalyzer.cpp",
        "meshoptimizer-0.20/src/overdrawoptimizer.cpp",
        "meshoptimizer-0.20/src/quantization.cpp",
        "meshoptimizer-0.20/src/simplifier.cpp",
        "meshoptimizer-0.20/src/spatialorder.cpp",
        "meshoptimizer-0.20/src/stripifier.cpp",
        "meshoptimizer-0.20/src/vcacheanalyzer.cpp",
        "meshoptimizer-0.20/src/vcacheoptimizer.cpp",
        "meshoptimizer-0.20/src/vertexcodec.cpp",
        "meshoptimizer-0.20/src/vertexfilter.cpp",
        "meshoptimizer-0.20/src/vfetchanalyzer.cpp",
        "meshoptimizer-0.20/src/vfetchoptimizer.cpp",
        "Premake5.lua",
    }

    includedirs {
        thirdpartypath("meshoptimizer/meshoptimizer-0.20"),
    }

