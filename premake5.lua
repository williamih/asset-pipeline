local MAC_EMBEDDED_HELPER_PATH = "bin/%{cfg.buildcfg}/Asset Pipeline.app/Contents/Resources/Asset Pipeline Helper.app"

workspace "AssetPipeline"
    configurations { "Debug", "Release" }
    platforms { "OSX" }

project("Common")
    kind "StaticLib"
    language "C++"
    targetdir("bin/%{cfg.buildcfg}")

    files { "Common/Source/**.h", "Common/Source/**.c", "Common/Source/**.cpp" }

    includedirs "Common/Source"
    includedirs "Common/Source/lua/src"
    includedirs "Common/Source/lua/etc"

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter "platforms:OSX"
        files { "Common/Source/**.m", "Common/Source/**.mm" }
        architecture "x64"
        links {
            "Cocoa.framework",
        }
        xcodebuildsettings {
            ["MACOSX_DEPLOYMENT_TARGET"] = "10.11",
            ["CLANG_ENABLE_OBJC_ARC"] = "YES",
            ["OTHER_CPLUSPLUSFLAGS"] = "-std=c++14",
        }

project("Asset Pipeline Helper")
    kind "WindowedApp"
    language "C++"
    targetdir("bin/%{cfg.buildcfg}")

    files { "Helper/Source/**.h", "Helper/Source/**.c", "Helper/Source/**.cpp" }

    links { "Common" }

    includedirs "Helper/Source"
    includedirs "Common/Source"

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter "platforms:OSX"
        files {
            "Helper/Source/**.m", "Helper/Source/**.mm", "Helper/Source/**.swift",
            "Helper/Mac/**.xib", "Helper/Mac/**.strings", "Helper/Mac/Info.plist"
        }
        architecture "x64"
        links {
            "Cocoa.framework",
        }
        xcodebuildsettings {
            ["MACOSX_DEPLOYMENT_TARGET"] = "10.11",
            ["CLANG_ENABLE_OBJC_ARC"] = "YES",
            ["OTHER_CPLUSPLUSFLAGS"] = "-std=c++14",
            ["SWIFT_OBJC_BRIDGING_HEADER"] = "Common/Source/Mac/AssetPipeline-Bridging-Header.h",
        }
        linkoptions { "-lstdc++" }

    filter "files:**.xib"
        buildaction "Resource"
    filter "files:**.plist"
        buildaction "Resource"
    filter "files:**.strings"
        buildaction "Resource"

project("Asset Pipeline")
    kind "WindowedApp"
    language "C++"
    targetdir("bin/%{cfg.buildcfg}")

    files {
        "MainApp/Source/**.h", "MainApp/Source/**.c", "MainApp/Source/**.cpp",
    }

    dependson { "Asset Pipeline Helper" }

    links { "Common" }

    includedirs "MainApp/Source"
    includedirs "Common/Source"

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter "platforms:OSX"
        files {
            "MainApp/Source/**.m", "MainApp/Source/**.mm", "MainApp/Source/**.swift",
            "MainApp/Mac/**.xib", "MainApp/Mac/**.strings", "MainApp/Mac/Info.plist"
        }
        architecture "x64"
        links {
            "Cocoa.framework",
        }
        xcodebuildsettings {
            ["MACOSX_DEPLOYMENT_TARGET"] = "10.11",
            ["CLANG_ENABLE_OBJC_ARC"] = "YES",
            ["OTHER_CPLUSPLUSFLAGS"] = "-std=c++14",
            ["SWIFT_OBJC_BRIDGING_HEADER"] = "Common/Source/Mac/AssetPipeline-Bridging-Header.h",
        }
        linkoptions { "-lstdc++" }

        postbuildcommands {
            string.format('ditto "bin/%%{cfg.buildcfg}/Asset Pipeline Helper.app" "%s"', MAC_EMBEDDED_HELPER_PATH),
        }
        postbuildcommands {
            'cp -r Scripts/ "bin/%{cfg.buildcfg}/Asset Pipeline.app/Contents/Resources"',
            'cp -r Images/ "bin/%{cfg.buildcfg}/Asset Pipeline.app/Contents/Resources"',
        }

    filter "files:**.xib"
        buildaction "Resource"
    filter "files:**.plist"
        buildaction "Resource"
    filter "files:**.strings"
        buildaction "Resource"
