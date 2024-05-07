project "FidelityFX"
    kind "SharedLib"
    language "C++"
    cppdialect "C++latest"
    staticruntime "Off"
    location "%{wks.location}/Build/%{prj.name}"
    targetdir "%{wks.location}/Build/Bin/%{cfg.buildcfg}"

    files {
        "**.h",
        "**.c",
        "**.hpp",
        "**.cpp",
        "**.cppm",
        "**.inl",
    }

    links {
        "HorizonEngine",
    }

    includedirs {
        enginepath("Source"),
        thirdpartypath("glm/include"),
        thirdpartypath("entt/include"),
        thirdpartypath("imgui/imgui-1.89.9-docking"),
        thirdpartypath("python/310/include"),
    }
