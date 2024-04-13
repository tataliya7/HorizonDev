project "Tests"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"
    location "%{wks.location}/Build/%{prj.name}"
    targetdir "%{wks.location}/Build/Bin/%{cfg.buildcfg}"
    debugdir "%{cfg.targetdir}"

    files {
        "**.h",
        "**.c",
        "**.hpp",
        "**.cpp",
        "**.cppm",
        "**.inl",
        "**.lua",
    }

    links {
        "HorizonEngine",
        "googletest",
    }

    includedirs {
        "",
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
        thirdpartypath("python/310/include"),
        thirdpartypath("miniaudio/include"),
        thirdpartypath("streamline/include"),
        thirdpartypath("ffx-fsr2/include"),
        thirdpartypath("googletest/googletest-1.14.0/googletest/include"),
    }

    filter "configurations:Debug"
        links {
            thirdpartypath("optick/Optick_1.4.0/lib/x64/release/OptickCore.lib"),
            thirdpartypath("python/310/libs/python310_d.lib"),
        }
        postbuildcommands {
            "{COPY} %{wks.location}/ThirdParty/optick/Optick_1.4.0/lib/x64/release/OptickCore.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/python/310/bin/python3_d.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/python/310/bin/python310_d.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/python/310/bin %{cfg.targetdir}/python310/bin",
            "{COPY} %{wks.location}/ThirdParty/python/310/lib %{cfg.targetdir}/python310/lib",
            "{COPY} %{wks.location}/ThirdParty/python/310/DLLs %{cfg.targetdir}/python310/DLLs",
        }

    filter "configurations:Development or Release"
        links {
            thirdpartypath("optick/Optick_1.4.0/lib/x64/release/OptickCore.lib"),
            thirdpartypath("python/310/libs/python310.lib"),
        }
        postbuildcommands {
            "{COPY} %{wks.location}/ThirdParty/optick/Optick_1.4.0/lib/x64/release/OptickCore.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/python/310/bin/python3.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/python/310/bin/python310.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/python/310/bin %{cfg.targetdir}/python310/bin",
            "{COPY} %{wks.location}/ThirdParty/python/310/lib %{cfg.targetdir}/python310/lib",
            "{COPY} %{wks.location}/ThirdParty/python/310/DLLs %{cfg.targetdir}/python310/DLLs",
        }
