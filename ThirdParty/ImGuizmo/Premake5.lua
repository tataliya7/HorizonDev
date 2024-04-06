project "ImGuizmo"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"
    location "%{wks.location}/Build/%{prj.name}"
    targetdir "%{wks.location}/Build/Bin/%{cfg.buildcfg}"

    files {
        "ImGuizmo-1.83/ImCurveEdit.cpp",
        "ImGuizmo-1.83/ImCurveEdit.h",
        "ImGuizmo-1.83/ImGradient.cpp",
        "ImGuizmo-1.83/ImGradient.h",
        "ImGuizmo-1.83/ImGuizmo.cpp",
        "ImGuizmo-1.83/ImGuizmo.h",
        "ImGuizmo-1.83/ImSequencer.cpp",
        "ImGuizmo-1.83/ImSequencer.h",
        "ImGuizmo-1.83/ImZoomSlider.h",
        "Premake5.lua",
    }

    links {
        "imgui",
    }

    includedirs {
        thirdpartypath("imgui/imgui-1.89.9-docking"),
        thirdpartypath("ImGuizmo/ImGuizmo-1.83"),
    }

