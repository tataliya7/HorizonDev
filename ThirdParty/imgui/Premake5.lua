project "imgui"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"
    location "%{wks.location}/Build/%{prj.name}"
    targetdir "%{wks.location}/Build/Bin/%{cfg.buildcfg}"

    files {
        "imgui-1.89.9-docking/imconfig.h",
        "imgui-1.89.9-docking/imgui.cpp",
        "imgui-1.89.9-docking/imgui.h",
        "imgui-1.89.9-docking/imgui_draw.cpp",
        "imgui-1.89.9-docking/imgui_internal.h",
        "imgui-1.89.9-docking/imgui_tables.cpp",
        "imgui-1.89.9-docking/imgui_widgets.cpp",
        "imgui-1.89.9-docking/imstb_rectpack.h",
        "imgui-1.89.9-docking/imstb_textedit.h",
        "imgui-1.89.9-docking/imstb_truetype.h",
        "imgui-1.89.9-docking/backends/imgui_impl_dx12.cpp",
        "imgui-1.89.9-docking/backends/imgui_impl_dx12.h",
        "imgui-1.89.9-docking/backends/imgui_impl_glfw.cpp",
        "imgui-1.89.9-docking/backends/imgui_impl_glfw.h",
        "imgui-1.89.9-docking/backends/imgui_impl_vulkan.cpp",
        "imgui-1.89.9-docking/backends/imgui_impl_vulkan.h",
        "Premake5.lua",
    }

    links {
        thirdpartypath("glfw/glfw-3.3.9.bin.WIN64/lib-vc2022/glfw3.lib"),
    }

    includedirs {
        thirdpartypath("glfw/glfw-3.3.9.bin.WIN64/include"),
        thirdpartypath("vulkan/1.4.304.0/Include"),
        thirdpartypath("imgui/imgui-1.89.9-docking"),
    }
