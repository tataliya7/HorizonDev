project "HorizonEngine"
    --kind "SharedLib"
    kind "StaticLib"
    language "C++"
    cppdialect "C++latest"
    staticruntime "Off"
    location "%{wks.location}/Build/%{prj.name}"
    targetdir "%{wks.location}/Build/Bin/%{cfg.buildcfg}"
    scanformoduledependencies "true"

    files {
        "**.lua",
        "Shaders/**.h",
        "Shaders/**.hsh",
        "Shaders/**.hsm",

        "Source/Foundation/**.h",
        "Source/Foundation/**.c",
        "Source/Foundation/**.hpp",
        "Source/Foundation/**.cpp",
        "Source/Foundation/**.cppm",
        "Source/Foundation/**.inl",

        "Source/Input/**.h",
        "Source/Input/**.c",
        "Source/Input/**.hpp",
        "Source/Input/**.cpp",
        "Source/Input/**.cppm",
        "Source/Input/**.inl",

        "Source/Rendering/**.h",
        "Source/Rendering/**.c",
        "Source/Rendering/**.hpp",
        "Source/Rendering/**.cpp",
        "Source/Rendering/**.cppm",
        "Source/Rendering/**.inl",

        "Source/Engine/**.h",
        "Source/Engine/**.c",
        "Source/Engine/**.hpp",
        "Source/Engine/**.cpp",
        "Source/Engine/**.cppm",
        "Source/Engine/**.inl",

        "Plugins/FidelityFX/Source/**.h",
        "Plugins/FidelityFX/Source/**.c",
        "Plugins/FidelityFX/Source/**.hpp",
        "Plugins/FidelityFX/Source/**.cpp",
        "Plugins/FidelityFX/Source/**.cppm",
        "Plugins/FidelityFX/Source/**.inl",
    }

    includedirs {
        enginepath("Source"),
        enginepath("Plugins/FidelityFX/Source"),
        thirdpartypath("entt/include"),
        thirdpartypath("dxc/dxc_2024_03_22/inc"),
        thirdpartypath("glfw/glfw-3.3.9.bin.WIN64/include"),
        thirdpartypath("glm/include"),
        thirdpartypath("spdlog/include"),
        thirdpartypath("mpmc/include"),
        thirdpartypath("imgui/imgui-1.89.9-docking"),
        thirdpartypath("stb/include"),
        thirdpartypath("physx/include"),
        thirdpartypath("optick/Optick_1.4.0/include"),
        thirdpartypath("vulkan/1.3.280.0/include"),
        thirdpartypath("vma/include"),
        thirdpartypath("directx/include"),
        thirdpartypath("python/310/include"),
        thirdpartypath("miniaudio/include"),
        thirdpartypath("streamline/include"),
        thirdpartypath("ffx-fsr2/include"),
        thirdpartypath("concurrentqueue/include")
    }

    links {
        --thirdpartypath("glfw/glfw-3.3.9.bin.WIN64/lib-vc2022/glfw3.lib"),
    }

-- group "EnginePlugins"
--     include "Plugins/FidelityFX"
-- group ""