project "HorizonEditor"
    kind "StaticLib"
    language "C++"
    cppdialect "C++latest"
    staticruntime "Off"
    location "%{wks.location}/Build/%{prj.name}"
    targetdir "%{wks.location}/Build/Bin/%{cfg.buildcfg}"
    scanformoduledependencies "true"

    files {
        "Premake5.lua",

        "Config/**.toml",

        "Source/Framework/**.h",
        "Source/Framework/**.c",
        "Source/Framework/**.hpp",
        "Source/Framework/**.cpp",
        "Source/Framework/**.cppm",
        "Source/Framework/**.inl",

        "Source/Editor/**.h",
        "Source/Editor/**.c",
        "Source/Editor/**.hpp",
        "Source/Editor/**.cpp",
        "Source/Editor/**.cppm",
        "Source/Editor/**.inl",

        -- "MaterialGraph/**.h",
        -- "MaterialGraph/**.c",
        -- "MaterialGraph/**.hpp",
        -- "MaterialGraph/**.cpp",
        -- "MaterialGraph/**.cppm",
        -- "MaterialGraph/**.inl",
    }

    links {
        "HorizonEngine",
    }

    includedirs {
        editorpath("Source"),
        editorpath("Plugins/UniversalSceneDescription/Source"),
        editorpath("Plugins/TimeOfDay/Source"),
        editorpath("Plugins/DevelopmentTools/RenderDoc/Source"),
        enginepath("Source"),
        thirdpartypath("glm/include"),
        thirdpartypath("glfw/glfw-3.3.9.bin.WIN64/include"),
        thirdpartypath("imgui/imgui-1.89.9-docking"),
        thirdpartypath("optick/Optick_1.4.0/include"),
        thirdpartypath("entt/include"),
        thirdpartypath("streamline/Streamline-2.4.15/include"),
        thirdpartypath("spdlog/include"),
        thirdpartypath("stb/include"),
        thirdpartypath("python/310/include"),
        thirdpartypath("OpenUSD/OpenUSD-25.05.01/include"),
    }

    filter "configurations:Debug"
        defines {
            "TBB_USE_DEBUG=0",
            "__TBB_NO_IMPLICIT_LINKAGE=1",
            "__TBBMALLOC_NO_IMPLICIT_LINKAGE=1",
        }

    filter "configurations:Test or Release"
        defines {
            "TBB_USE_DEBUG=0",
            "__TBB_NO_IMPLICIT_LINKAGE=1",
            "__TBBMALLOC_NO_IMPLICIT_LINKAGE=1",
        }

    filter "configurations:Debug"
        defines { "USE_OPTICK=1" }

    filter "configurations:Test"
        defines { "USE_OPTICK=1" }

    filter "configurations:Release"
        defines { "USE_OPTICK=0" }

group "Editor/Plugins"
    include "Plugins/TimeOfDay"
    include "Plugins/UniversalSceneDescription"
group ""

group "Editor/Plugins/DevelopmentTools"
    include "Plugins/DevelopmentTools/RenderDoc"
group ""