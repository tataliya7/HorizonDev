project "HorizonEngine"
    kind "StaticLib"
    language "C++"
    cppdialect "C++latest"
    staticruntime "Off"
    location "%{wks.location}/Build/%{prj.name}"
    targetdir "%{wks.location}/Build/Bin/%{cfg.buildcfg}"
    scanformoduledependencies "true"

    files {
        "**.h",
        "**.c",
        "**.hpp",
        "**.cpp",
        "**.cppm",
        "**.inl",
        "**.lua",
    }

    includedirs {
        enginepath(""),
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
