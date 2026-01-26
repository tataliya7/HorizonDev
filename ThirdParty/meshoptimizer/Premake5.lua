project "meshoptimizer"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"
    location "%{wks.location}/Build/%{prj.name}"
    targetdir "%{wks.location}/Build/Bin/%{cfg.buildcfg}"

    files {
        "meshoptimizer-1.0/src/*.h",
        "meshoptimizer-1.0/src/*.cpp",
        "Premake5.lua",
    }

    includedirs {
        thirdpartypath("meshoptimizer/meshoptimizer-1.0"),
    }
