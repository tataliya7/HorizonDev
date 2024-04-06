project "googletest"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"
    location "%{wks.location}/Build/%{prj.name}"
    targetdir "%{wks.location}/Build/Bin/%{cfg.buildcfg}"

    files {
        thirdpartypath("googletest/googletest-1.14.0/googletest/src/gtest-all.cc"),
    }

    includedirs {
        thirdpartypath("googletest/googletest-1.14.0/googletest"),
        thirdpartypath("googletest/googletest-1.14.0/googletest/include"),
    }

