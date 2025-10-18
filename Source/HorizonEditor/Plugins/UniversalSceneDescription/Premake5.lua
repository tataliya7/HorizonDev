project "UniversalSceneDescription"
    kind "StaticLib"
    language "C++"
    cppdialect "C++latest"
    staticruntime "Off"
    location "%{wks.location}/Build/%{prj.name}"
    targetdir "%{wks.location}/Build/Bin/%{cfg.buildcfg}"

    files {
        "**.lua",
        "**.toml",
        "**.h",
        "**.c",
        "**.hpp",
        "**.cpp",
        "**.cppm",
        "**.inl",
    }

    includedirs {
        enginepath("Source"),
        thirdpartypath("glm/include"),
        thirdpartypath("entt/include"),
        thirdpartypath("imgui/imgui-1.89.9-docking"),
        thirdpartypath("python/310/include"),
        thirdpartypath("OpenUSD/OpenUSD-25.05.01/include"),
        thirdpartypath("optick/Optick_1.4.0/include"), -- @todo Remove this
    }

    defines {
        "TBB_SUPPRESS_DEPRECATED_MESSAGES=1",
    }

    filter "configurations:Debug"
        defines {
            "TBB_USE_DEBUG=1",
            "__TBB_NO_IMPLICIT_LINKAGE=1",
            "__TBBMALLOC_NO_IMPLICIT_LINKAGE=1",
        }

    filter "configurations:Test or Release"
        defines {
            "TBB_USE_DEBUG=0",
            "__TBB_NO_IMPLICIT_LINKAGE=1",
            "__TBBMALLOC_NO_IMPLICIT_LINKAGE=1",
        }
