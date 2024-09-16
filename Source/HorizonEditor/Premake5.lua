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
        "Source/Framework/HorizonEditorUI.h",
        "Source/Framework/HorizonEditorUI.cpp",
        "Source/Framework/StbImage.cpp",
        "Source/Framework/EditorCamera.h",
        "Source/Framework/EditorCamera.cpp",
        "Source/Framework/EditorSceneManager.h",
        "Source/Framework/EditorSceneManager.cpp",
        "Source/Framework/TextureImporter.h",
        "Source/Framework/TextureImporter.cpp",
        "Source/Framework/InspectorUI_DEPRECATED.h",
        "Source/Framework/InspectorUI_DEPRECATED.cpp",
        "Source/Framework/Gizmo.h",
        "Source/Framework/Gizmo.cpp",

        "Source/Editor/**.h",
        "Source/Editor/**.c",
        "Source/Editor/**.hpp",
        "Source/Editor/**.cpp",
        "Source/Editor/**.cppm",
        "Source/Editor/**.inl",

        "Plugins/USD/**.h",
        "Plugins/USD/**.c",
        "Plugins/USD/**.hpp",
        "Plugins/USD/**.cpp",
        "Plugins/USD/**.cppm",
        "Plugins/USD/**.inl",

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
        "RenderDoc",
    }

    includedirs {
        editorpath("Source"),
        editorpath("Plugins/USD/Source"),
        editorpath("Plugins/RenderDoc/Source"),
        enginepath("Source"),
        thirdpartypath("glm/include"),
        thirdpartypath("glfw/glfw-3.3.9.bin.WIN64/include"),
        thirdpartypath("imgui/imgui-1.89.9-docking"),
        thirdpartypath("optick/Optick_1.4.0/include"),
        thirdpartypath("entt/include"),
        thirdpartypath("streamline/Streamline-2.4.15/include"),
        thirdpartypath("spdlog/include"),
        thirdpartypath("stb/include"),
        thirdpartypath("usd/include"),
        thirdpartypath("usd/include/boost-1_78"),
        thirdpartypath("usd/include/tbb"),
    }

    defines {
        "HORIZON_EDITOR=1",
    }

    filter "configurations:Debug"
        defines { "USE_OPTICK=1" }

    filter "configurations:Test"
        defines { "USE_OPTICK=1" }

    filter "configurations:Release"
        defines { "USE_OPTICK=0" }

group "EditorPlugins"
    include "Plugins/RenderDoc"
    -- include "Plugins/USD"
    -- include "Plugins/TimeOfDay"
    -- include "Plugins/LookDevStudio"
group ""