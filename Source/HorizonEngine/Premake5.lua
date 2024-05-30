project "HorizonEngine"
    kind "StaticLib"
    language "C++"
    cppdialect "C++latest"
    staticruntime "Off"
    location "%{wks.location}/Build/%{prj.name}"
    targetdir "%{wks.location}/Build/Bin/%{cfg.buildcfg}"
    scanformoduledependencies "true"

    files {
        -- "**.h",
        -- "**.c",
        -- "**.hpp",
        -- "**.cpp",
        -- "**.cppm",
        -- "**.inl",
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
        "Source/Input/**.lua",

        -- "Source/Rendering/RenderBackend/**.h",
        -- "Source/Rendering/RenderBackend/**.c",
        -- "Source/Rendering/RenderBackend/**.hpp",
        -- "Source/Rendering/RenderBackend/**.cpp",
        -- "Source/Rendering/RenderBackend/**.cppm",
        -- "Source/Rendering/RenderBackend/**.inl",
        -- "Source/Rendering/RenderBackend/**.lua",

        -- "Source/Rendering/RenderGraph/**.h",
        -- "Source/Rendering/RenderGraph/**.c",
        -- "Source/Rendering/RenderGraph/**.hpp",
        -- "Source/Rendering/RenderGraph/**.cpp",
        -- "Source/Rendering/RenderGraph/**.cppm",
        -- "Source/Rendering/RenderGraph/**.inl",
        -- "Source/Rendering/RenderGraph/**.lua",

        "Source/Rendering/**.h",
        "Source/Rendering/**.c",
        "Source/Rendering/**.hpp",
        "Source/Rendering/**.cpp",
        "Source/Rendering/**.cppm",
        "Source/Rendering/**.inl",

--         "Source/Core/**.h",
--         "Source/Core/**.c",
--         "Source/Core/**.hpp",
--         "Source/Core/**.cpp",
--         "Source/Core/**.cppm",
--         "Source/Core/**.inl",
--         "Source/Core/**.lua",

        "Source/Engine/HorizonEngineModule.h",
        "Source/Engine/HorizonEngineVersion.h",

        "Source/Engine/**.h",
        "Source/Engine/**.c",
        "Source/Engine/**.hpp",
        "Source/Engine/**.cpp",
        "Source/Engine/**.cppm",
        "Source/Engine/**.inl",
        "Source/Engine/**.lua",
    }

    includedirs {
        enginepath("Source"),
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
