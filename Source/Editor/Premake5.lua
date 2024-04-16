project "HorizonEditor"
    kind "StaticLib"
    language "C++"
    cppdialect "C++latest"
    staticruntime "Off"
    location "%{wks.location}/Build/%{prj.name}"
    targetdir "%{wks.location}/Build/Bin/%{cfg.buildcfg}"
    scanformoduledependencies "true"

    files {
        "HorizonEditor.h",
        "Premake5.lua",

        "Framework/**.h",
        "Framework/**.c",
        "Framework/**.hpp",
        "Framework/**.cpp",
        "Framework/**.cppm",
        "Framework/**.inl",

        "ShadeGraph/**.h",
        "ShadeGraph/**.c",
        "ShadeGraph/**.hpp",
        "ShadeGraph/**.cpp",
        "ShadeGraph/**.cppm",
        "ShadeGraph/**.inl",
    }

    links {
        "HorizonEngine",
        "RenderDoc",
    }

    includedirs {
        editorpath(""),
        enginepath(""),
        thirdpartypath("glm/include"),
        thirdpartypath("glfw/glfw-3.3.9.bin.WIN64/include"),
        thirdpartypath("imgui/imgui-1.89.9-docking"),
        thirdpartypath("ImGuizmo/ImGuizmo-1.83"),
        thirdpartypath("optick/Optick_1.4.0/include"),
        thirdpartypath("entt/include"),
        thirdpartypath("streamline/include"),
        thirdpartypath("spdlog/include"),
        thirdpartypath("stb/include"),
    }

    defines {
        "WITH_HORIZON_EDITOR=1",
    }

group "EditorPlugins"
    include "Plugins/RenderDoc"
    include "Plugins/USD"
    include "Plugins/TimeOfDay"
    include "Plugins/LookDevStudio"
group ""