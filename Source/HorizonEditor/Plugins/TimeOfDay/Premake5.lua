project "TimeOfDay"
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
        thirdpartypath("optick/Optick_1.4.0/include"), -- @todo Remove this
    }

    links {
        "HorizonEngine",
    }
