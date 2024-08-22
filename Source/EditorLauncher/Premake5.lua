project "HorizonEditorLauncher"
    -- kind "WindowedApp"
    kind "ConsoleApp"
    entrypoint "mainCRTStartup"
    language "C++"
    cppdialect "C++latest"
    staticruntime "Off"
    location "%{wks.location}/Build/%{prj.name}"
    targetdir "%{wks.location}/Build/Bin/%{cfg.buildcfg}"
    debugdir "%{cfg.targetdir}"
    scanformoduledependencies "true"

    files {
        "**.h",
        "**.c",
        "**.hpp",
        "**.cpp",
        "**.cppm",
        "**.lua",
    }

    includedirs {
        enginepath(""),
        editorpath(""),
    }

    links {
        "HorizonEngine",
        "HorizonEditor",
        "imgui",
        "USD",
        "RenderDoc",
        thirdpartypath("glfw/glfw-3.3.9.bin.WIN64/lib-vc2022/glfw3.lib"),
        thirdpartypath("dxc/dxc_2024_03_22/lib/x64/dxcompiler.lib"),
        thirdpartypath("vulkan/1.3.280.0/lib/vulkan-1.lib"),
        thirdpartypath("optick/Optick_1.4.0/lib/x64/release/OptickCore.lib"),
        thirdpartypath("streamline/lib/x64/sl.interposer.lib"),
        thirdpartypath("python/310/libs/python310.lib"),
    }

    postbuildcommands {
        "{COPY} %{wks.location}/ThirdParty/optick/Optick_1.4.0/lib/x64/release/OptickCore.dll %{cfg.targetdir}",

        "{COPY} %{wks.location}/ThirdParty/streamline/bin/x64/development/nvngx_dlss.dll %{cfg.targetdir}",
        "{COPY} %{wks.location}/ThirdParty/streamline/bin/x64/development/nvngx_dlssg.dll %{cfg.targetdir}",
        "{COPY} %{wks.location}/ThirdParty/streamline/bin/x64/development/NvLowLatencyVk.dll %{cfg.targetdir}",
        "{COPY} %{wks.location}/ThirdParty/streamline/bin/x64/development/sl.common.dll %{cfg.targetdir}",
        "{COPY} %{wks.location}/ThirdParty/streamline/bin/x64/development/sl.dlss.dll %{cfg.targetdir}",
        "{COPY} %{wks.location}/ThirdParty/streamline/bin/x64/development/sl.dlss_g.dll %{cfg.targetdir}",
        "{COPY} %{wks.location}/ThirdParty/streamline/bin/x64/development/sl.interposer.dll %{cfg.targetdir}",
        "{COPY} %{wks.location}/ThirdParty/streamline/bin/x64/development/sl.reflex.dll %{cfg.targetdir}",

        "{COPY} %{wks.location}/ThirdParty/dxc/dxc_2024_03_22/bin/x64/dxcompiler.dll %{cfg.targetdir}",
        "{COPY} %{wks.location}/ThirdParty/dxc/dxc_2024_03_22/bin/x64/dxil.dll %{cfg.targetdir}",
    }

    includedirs {
        enginepath(""),
        editorpath(""),
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
    }

    filter "configurations:Debug"
        links {
            thirdpartypath("physx/lib/debug/PhysX_64.lib"),
            thirdpartypath("physx/lib/debug/PhysXCommon_64.lib"),
            thirdpartypath("physx/lib/debug/PhysXFoundation_64.lib"),
            thirdpartypath("physx/lib/debug/PhysXExtensions_static_64.lib"),
            thirdpartypath("physx/lib/debug/PhysXPvdSDK_static_64.lib"),
            thirdpartypath("physx/lib/debug/PhysXVehicle_static_64.lib"),
            thirdpartypath("physx/lib/debug/PhysXVehicle2_static_64.lib"),
            thirdpartypath("physx/lib/debug/PhysXCharacterKinematic_static_64.lib"),
            thirdpartypath("physx/lib/debug/PhysXCooking_64.lib"),
            thirdpartypath("physx/lib/debug/PVDRuntime_64.lib"),
            thirdpartypath("physx/lib/debug/SceneQuery_static_64.lib"),

            thirdpartypath("python/310/libs/python310.lib"),

            thirdpartypath("ffx-fsr2/lib/ffx_fsr2_api_dx12_x64d.lib"),
            thirdpartypath("ffx-fsr2/lib/ffx_fsr2_api_vk_x64d.lib"),
            thirdpartypath("ffx-fsr2/lib/ffx_fsr2_api_x64d.lib"),

            thirdpartypath("usd/lib/debug/usd_ms.lib"),
            thirdpartypath("usd/lib/debug/tbb_debug.lib"),
            thirdpartypath("usd/lib/debug/tbbmalloc_debug.lib"),
            thirdpartypath("usd/lib/debug/tbbmalloc_proxy_debug.lib"),
            thirdpartypath("usd/lib/debug/MaterialXCore.lib"),
            thirdpartypath("usd/lib/debug/MaterialXFormat.lib"),
            thirdpartypath("usd/lib/debug/MaterialXGenGlsl.lib"),
            thirdpartypath("usd/lib/debug/MaterialXGenMdl.lib"),
            thirdpartypath("usd/lib/debug/MaterialXGenMsl.lib"),
            thirdpartypath("usd/lib/debug/MaterialXGenOsl.lib"),
            thirdpartypath("usd/lib/debug/MaterialXGenShader.lib"),
            thirdpartypath("usd/lib/debug/MaterialXRender.lib"),
            thirdpartypath("usd/lib/debug/MaterialXRenderGlsl.lib"),
            thirdpartypath("usd/lib/debug/MaterialXRenderHw.lib"),
            thirdpartypath("usd/lib/debug/MaterialXRenderOsl.lib"),
        }
        postbuildcommands {
            "{COPY} %{wks.location}/ThirdParty/physx/lib/debug/PhysX_64.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/physx/lib/debug/PhysXFoundation_64.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/physx/lib/debug/PhysXCommon_64.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/physx/lib/debug/PhysXCooking_64.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/physx/lib/debug/PVDRuntime_64.dll %{cfg.targetdir}",

            "{COPY} %{wks.location}/ThirdParty/python/310/bin/python3.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/python/310/bin/python310.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/python/310/bin %{cfg.targetdir}/python310/bin",
            "{COPY} %{wks.location}/ThirdParty/python/310/lib %{cfg.targetdir}/python310/lib",
            "{COPY} %{wks.location}/ThirdParty/python/310/DLLs %{cfg.targetdir}/python310/DLLs",

            "{COPY} %{wks.location}/ThirdParty/ffx-fsr2/lib/ffx_fsr2_api_dx12_x64d.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/ffx-fsr2/lib/ffx_fsr2_api_vk_x64d.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/ffx-fsr2/lib/ffx_fsr2_api_x64d.dll %{cfg.targetdir}",

            "{COPY} %{wks.location}/ThirdParty/usd/lib/debug/usd %{cfg.targetdir}/usd",
            "{COPY} %{wks.location}/ThirdParty/usd/lib/debug/usd_ms.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/debug/tbb_debug.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/debug/tbbmalloc_debug.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/debug/tbbmalloc_proxy_debug.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/debug/MaterialXCore.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/debug/MaterialXFormat.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/debug/MaterialXGenGlsl.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/debug/MaterialXGenMdl.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/debug/MaterialXGenMsl.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/debug/MaterialXGenOsl.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/debug/MaterialXGenShader.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/debug/MaterialXRender.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/debug/MaterialXRenderGlsl.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/debug/MaterialXRenderHw.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/debug/MaterialXRenderOsl.dll %{cfg.targetdir}",
        }

    filter "configurations:Development or Release"
        links {
            thirdpartypath("physx/lib/release/PhysX_64.lib"),
            thirdpartypath("physx/lib/release/PhysXCommon_64.lib"),
            thirdpartypath("physx/lib/release/PhysXFoundation_64.lib"),
            thirdpartypath("physx/lib/release/PhysXExtensions_static_64.lib"),
            thirdpartypath("physx/lib/release/PhysXPvdSDK_static_64.lib"),
            thirdpartypath("physx/lib/release/PhysXVehicle_static_64.lib"),
            thirdpartypath("physx/lib/release/PhysXVehicle2_static_64.lib"),
            thirdpartypath("physx/lib/release/PhysXCharacterKinematic_static_64.lib"),
            thirdpartypath("physx/lib/release/PhysXCooking_64.lib"),
            thirdpartypath("physx/lib/release/PVDRuntime_64.lib"),
            thirdpartypath("physx/lib/release/SceneQuery_static_64.lib"),

            thirdpartypath("python/310/libs/python310.lib"),

            thirdpartypath("ffx-fsr2/lib/ffx_fsr2_api_dx12_x64.lib"),
            thirdpartypath("ffx-fsr2/lib/ffx_fsr2_api_vk_x64.lib"),
            thirdpartypath("ffx-fsr2/lib/ffx_fsr2_api_x64.lib"),

            thirdpartypath("usd/lib/release/usd_ms.lib"),
            thirdpartypath("usd/lib/release/tbb.lib"),
            thirdpartypath("usd/lib/release/tbbmalloc.lib"),
            thirdpartypath("usd/lib/release/tbbmalloc_proxy.lib"),
            thirdpartypath("usd/lib/release/MaterialXCore.lib"),
            thirdpartypath("usd/lib/release/MaterialXFormat.lib"),
            thirdpartypath("usd/lib/release/MaterialXGenGlsl.lib"),
            thirdpartypath("usd/lib/release/MaterialXGenMdl.lib"),
            thirdpartypath("usd/lib/release/MaterialXGenMsl.lib"),
            thirdpartypath("usd/lib/release/MaterialXGenOsl.lib"),
            thirdpartypath("usd/lib/release/MaterialXGenShader.lib"),
            thirdpartypath("usd/lib/release/MaterialXRender.lib"),
            thirdpartypath("usd/lib/release/MaterialXRenderGlsl.lib"),
            thirdpartypath("usd/lib/release/MaterialXRenderHw.lib"),
            thirdpartypath("usd/lib/release/MaterialXRenderOsl.lib"),
        }
        postbuildcommands {
            "{COPY} %{wks.location}/ThirdParty/physx/lib/release/PhysX_64.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/physx/lib/release/PhysXFoundation_64.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/physx/lib/release/PhysXCommon_64.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/physx/lib/release/PhysXCooking_64.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/physx/lib/release/PVDRuntime_64.dll %{cfg.targetdir}",

            "{COPY} %{wks.location}/ThirdParty/python/310/bin/python3.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/python/310/bin/python310.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/python/310/bin %{cfg.targetdir}/python310/bin",
            "{COPY} %{wks.location}/ThirdParty/python/310/lib %{cfg.targetdir}/python310/lib",
            "{COPY} %{wks.location}/ThirdParty/python/310/DLLs %{cfg.targetdir}/python310/DLLs",

            "{COPY} %{wks.location}/ThirdParty/ffx-fsr2/lib/ffx_fsr2_api_dx12_x64.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/ffx-fsr2/lib/ffx_fsr2_api_vk_x64.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/ffx-fsr2/lib/ffx_fsr2_api_x64.dll %{cfg.targetdir}",

            "{COPY} %{wks.location}/ThirdParty/usd/lib/release/usd %{cfg.targetdir}/usd",
            "{COPY} %{wks.location}/ThirdParty/usd/lib/release/usd_ms.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/release/tbb.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/release/tbbmalloc.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/release/tbbmalloc_proxy.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/release/MaterialXCore.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/release/MaterialXFormat.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/release/MaterialXGenGlsl.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/release/MaterialXGenMdl.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/release/MaterialXGenMsl.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/release/MaterialXGenOsl.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/release/MaterialXGenShader.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/release/MaterialXRender.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/release/MaterialXRenderGlsl.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/release/MaterialXRenderHw.dll %{cfg.targetdir}",
            "{COPY} %{wks.location}/ThirdParty/usd/bin/release/MaterialXRenderOsl.dll %{cfg.targetdir}",
        }
