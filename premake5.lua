local OS_ID = os.get()

local QT_MAC_BASE_PATH = "/Applications/Qt5.7.0/5.7/clang_64"
local QT_MAC_LIB_BASE_PATH = QT_MAC_BASE_PATH.."/lib"
local MAC_EMBEDDED_HELPER_PATH = "bin/%{cfg.buildcfg}/Asset Pipeline.app/Contents/Resources/Asset Pipeline Helper.app"

function GetCommandMkdirRecursive(path)
    if OS_ID == "macosx" then
        return string.format('mkdir -p "%s"', path)
    end
end

function GetQtLibPath(name)
    if OS_ID == "macosx" then
        return QT_MAC_LIB_BASE_PATH.."/"..name..".framework"
    end
end

function GetQtHeadersPath(name)
    if OS_ID == "macosx" then
        return GetQtLibPath(name).."/Headers"
    end
end

function GetQtRccCommand(input, output)
    if OS_ID == "macosx" then
        local rccPath = QT_MAC_BASE_PATH.."/bin/rcc"
        return string.format("%s %s -o %s", rccPath, input, output)
    end
end

function GetQtMocCommand(input, output)
    if OS_ID == "macosx" then
        local mocPath = QT_MAC_BASE_PATH.."/bin/moc"
        return string.format('%s "%s" -o "%s"', mocPath, input, output)
    end
end

function GetQtMocOutputPath(projName, file)
    local index, _, name = string.find(file, "([^/]*)%.h$")
    local path = string.sub(file, 1, index - 1)
    return projName.."/generated/moc/"..path.."moc_"..name..".cpp"
end

function GetQtMocOutputFilePaths(projName, files)
    local result = {}
    for _, file in ipairs(files) do
        result[#result+1] = GetQtMocOutputPath(projName, file)
    end
    return result
end

function GetQtMocCommands(projName, files)
    local result = {}
    for _, file in ipairs(files) do
        local input = projName.."/Source/"..file
        local output = GetQtMocOutputPath(projName, file)
        result[#result+1] = GetQtMocCommand(input, output)
    end
    return result
end

HELPER_MOC_INPUT_FILES = {
    "ProjectsWindow.h",
    "HelperApp.h",
    "AddProjectWindow.h"
}

MAINAPP_MOC_INPUT_FILES = {
    "SystemTrayApp.h",
}

QT_LIBS = {
    "QtCore",
    "QtGui",
    "QtWidgets",
    "QtNetwork",
}

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
    files(GetQtMocOutputFilePaths("Helper", HELPER_MOC_INPUT_FILES))

    prebuildcommands(GetCommandMkdirRecursive("Helper/generated/moc"))
    prebuildcommands(GetQtMocCommands("Helper", HELPER_MOC_INPUT_FILES))

    links { "Common" }
    for _, lib in ipairs(QT_LIBS) do
        links(GetQtLibPath(lib))
    end

    includedirs "Helper/Source"
    includedirs "Common/Source"
    for _, lib in ipairs(QT_LIBS) do
        includedirs(GetQtHeadersPath(lib))
    end

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter "platforms:OSX"
        files {
            "Helper/Source/**.m", "Helper/Source/**.mm",
            "Helper/Mac/**.strings", "Helper/Mac/Info.plist"
        }
        architecture "x64"
        links {
            "Cocoa.framework",
        }
        frameworkdirs { QT_MAC_LIB_BASE_PATH }
        xcodebuildsettings {
            ["MACOSX_DEPLOYMENT_TARGET"] = "10.11",
            ["CLANG_ENABLE_OBJC_ARC"] = "YES",
            ["OTHER_CPLUSPLUSFLAGS"] = "-std=c++14",
            ["PRODUCT_BUNDLE_IDENTIFIER"] = "com.williamih.assetpipelinehelper",
        }
        linkoptions { "-lstdc++" }

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
        "MainApp/generated/QtResources.cpp",
    }
    files(GetQtMocOutputFilePaths("MainApp", MAINAPP_MOC_INPUT_FILES))

    dependson { "Asset Pipeline Helper" }
    prebuildcommands(GetCommandMkdirRecursive("MainApp/generated/moc"))
    prebuildcommands(GetQtMocCommands("MainApp", MAINAPP_MOC_INPUT_FILES))
    prebuildcommands(GetQtRccCommand("Resources.qrc", "MainApp/generated/QtResources.cpp"))

    links { "Common" }

    includedirs "MainApp/Source"
    includedirs "Common/Source"
    for _, lib in ipairs(QT_LIBS) do
        includedirs(GetQtHeadersPath(lib))
    end

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter "platforms:OSX"
        files {
            "MainApp/Source/**.m", "MainApp/Source/**.mm",
            "MainApp/Mac/**.strings", "MainApp/Mac/Info.plist"
        }
        architecture "x64"
        links {
            "Cocoa.framework",
        }
        frameworkdirs { QT_MAC_LIB_BASE_PATH }
        xcodebuildsettings {
            ["MACOSX_DEPLOYMENT_TARGET"] = "10.11",
            ["CLANG_ENABLE_OBJC_ARC"] = "YES",
            ["OTHER_CPLUSPLUSFLAGS"] = "-std=c++14",
            -- N.B. NSUserNotificationCenter popup notifications don't work if
            -- this isn't set!
            ["PRODUCT_BUNDLE_IDENTIFIER"] = "com.williamih.assetpipeline",
        }
        for _, lib in ipairs(QT_LIBS) do
            xcodeembeddedframeworks(GetQtLibPath(lib))
        end
        linkoptions { "-lstdc++" }

        postbuildcommands {
            string.format('ditto "bin/%%{cfg.buildcfg}/Asset Pipeline Helper.app" "%s"', MAC_EMBEDDED_HELPER_PATH),
            string.format('mkdir -p "%s/Contents/Frameworks"', MAC_EMBEDDED_HELPER_PATH),
        }
        postbuildcommands {
            'cp -r Scripts/ "bin/%{cfg.buildcfg}/Asset Pipeline.app/Contents/Resources"',
        }
        for _, lib in ipairs(QT_LIBS) do
            postbuildcommands {
                string.format('rm -f "%s/Contents/Frameworks/%s.framework"', MAC_EMBEDDED_HELPER_PATH, lib),
                string.format('ln -s ../../../../Frameworks/%s.framework "%s/Contents/Frameworks/%s.framework"',
                              lib, MAC_EMBEDDED_HELPER_PATH, lib),
            }
        end

    filter "files:**.plist"
        buildaction "Resource"
    filter "files:**.strings"
        buildaction "Resource"
