project "HorizonEditor"
    kind "StaticLib"
    language "C++"
    cppdialect "C++latest"
    staticruntime "Off"
    location "%{wks.location}/Build/%{prj.name}"
    targetdir "%{wks.location}/Build/Bin/%{cfg.buildcfg}"
    scanformoduledependencies "true"

    files {
        --"Source/Framework/EditorSubsystem.h",
        "Source/Framework/WindowSystem.h",
        "Source/Framework/WindowSystem.cpp",
        "Source/Framework/HorizonEditor.h",
        "Source/Framework/HorizonEditor.cpp",
        "Source/Framework/StbImage.cpp",

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
        "**.lua",
    }

    links {
        "HorizonEngine",
        --"RenderDoc",
    }

    includedirs {
        editorpath("Source"),
        enginepath("Source"),
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
        "HORIZON_EDITOR=1",
    }

-- group "EditorPlugins"
--     include "Plugins/RenderDoc"
--     include "Plugins/USD"
--     include "Plugins/TimeOfDay"
--     include "Plugins/LookDevStudio"
-- group ""