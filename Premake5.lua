enginedir = "%{wks.location}/Source/HorizonEngine"
function enginepath(path)
    return enginedir .. "/" .. path
end

editordir = "%{wks.location}/Source/HorizonEditor"
function editorpath(path)
    return editordir .. "/" .. path
end

thirdpartydir = "%{wks.location}/ThirdParty"
function thirdpartypath(path)
    return thirdpartydir .. "/" .. path
end

plugindir = "%{wks.location}/Plugins"
function pluginpath(path)
    return plugindir .. "/" .. path
end

function sourcedirs(dirs)
    if type(dirs) ~= "table" then dirs = {dirs} end
    for _, dir in ipairs(dirs) do
    files {
        dir .. "/**.h",
        dir .. "/**.c",
        dir .. "/**.hpp",
        dir .. "/**.cpp",
        dir .. "/**.cppm",
        dir .. "/**.inl",
    }
    end
end

function plugin(name)
    project(name)
        kind "SharedLib"
        language "C++"
        cppdialect "C++20"
        location "%{wks.location}/BuildPlugins/%{prj.name}"
        targetdir "%{wks.location}/Build/Bin/%{cfg.buildcfg}"
        sourcedirs {
            "%{wks.location}/Plugins/" .. name
        }
end

workspace "Horizon"
    startproject "HorizonEditorLauncher"
    location ""
    configurations {
        "Debug",
        "Test",
        "Release",
    }
    flags {
        "MultiProcessorCompile",
        "FatalWarnings",
    }

filter { 'files:**.cppm' }
    buildaction 'ClCompile'

filter "configurations:Debug"
    defines { "DEBUG", "HORIZON_CONFIGURATION_DEBUG" }
    runtime "Debug"
    optimize "Off"
    symbols "On"
    -- https://learn.microsoft.com/en-us/cpp/error-messages/tool-errors/linker-tools-warning-lnk4098?view=msvc-170
    linkoptions {
        "/NODEFAULTLIB:libcmt.lib",
        "/NODEFAULTLIB:msvcrt.lib",
        "/NODEFAULTLIB:libcmtd.lib",
    }

filter "configurations:Test"
    defines { "NDEBUG", "HORIZON_CONFIGURATION_TEST" }
    runtime "Release"
    optimize "Speed"
    symbols "On"
    -- https://learn.microsoft.com/en-us/cpp/error-messages/tool-errors/linker-tools-warning-lnk4098?view=msvc-170
    linkoptions {
        "/NODEFAULTLIB:libcmt.lib",
        "/NODEFAULTLIB:libcmtd.lib",
        "/NODEFAULTLIB:msvcrtd.lib",
    }

filter "configurations:Release"
    defines { "NDEBUG", "HORIZON_CONFIGURATION_RELEASE" }
    runtime "Release"
    optimize "Speed"
    symbols "Off"
    -- https://learn.microsoft.com/en-us/cpp/error-messages/tool-errors/linker-tools-warning-lnk4098?view=msvc-170
    linkoptions {
        "/NODEFAULTLIB:libcmt.lib",
        "/NODEFAULTLIB:libcmtd.lib",
        "/NODEFAULTLIB:msvcrtd.lib",
    }

filter "system:windows"
    platforms "Win64"
    systemversion "latest"

filter "platforms:Win64"
    defines {
        "NOMINMAX",
        "_CRT_SECURE_NO_WARNINGS",
        "_SILENCE_CXX23_ALIGNED_STORAGE_DEPRECATION_WARNING",
        "_SILENCE_CXX20_CISO646_REMOVED_WARNING",
        "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING", --TODO: delete this
        "HE_ENBALE_STREAMLINE_SUPPORT=0",
    }
    staticruntime "Off"
    architecture "x64"
    buildoptions {
        "/utf-8",
        --"/wd5105",
    }
    linkoptions {
        --"/ignore:4006",
    }
    disablewarnings {
    }

group "Editor"
    include "Source/HorizonEditor"
    include "Source/HorizonEditorLauncher"
group ""

group "Engine"
    include "Source/HorizonEngine"
group ""

group "ThirdParty"
    include "ThirdParty/imgui"
    include "ThirdParty/googletest"
    include "ThirdParty/meshoptimizer"
group ""

-- group "Tests"
--     include "Tests"
-- group ""
