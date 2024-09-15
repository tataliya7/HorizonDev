project "USD"
    kind "StaticLib"
    language "C++"
    cppdialect "C++latest"
    staticruntime "Off"
    location "%{wks.location}/Build/%{prj.name}"
    targetdir "%{wks.location}/Build/Bin/%{cfg.buildcfg}"

    files {
        "**.h",
        "**.cpp",
        "**.cppm",
        "**.lua",
    }

    includedirs {
        enginepath("Source"),
        thirdpartypath("glm/include"),
        thirdpartypath("entt/include"),
        thirdpartypath("imgui/imgui-1.89.9-docking"),
        thirdpartypath("python/310/include"),

        thirdpartypath("usd/include"),
        thirdpartypath("usd/include/boost-1_78"),
        thirdpartypath("usd/include/tbb"),
    }

    defines {
        "TBB_SUPPRESS_DEPRECATED_MESSAGES=1",
    }

    filter "configurations:Debug"
        defines {
            "TBB_USE_DEBUG=1",
            --"__TBB_NO_IMPLICIT_LINKAGE=1",
        }

    filter "configurations:Test or Release"
        defines {
            "TBB_USE_DEBUG=0",
            --"__TBB_NO_IMPLICIT_LINKAGE=1",
        }
