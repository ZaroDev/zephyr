project "Engine"
kind "StaticLib"
language "C++"
cppdialect "C++20"
targetdir "Binaries/%{cfg.buildcfg}"
staticruntime "off"

pchheader "pch.h"
pchsource "src/pch.cpp"

files {"src/**.h", "src/**.hpp", "src/**.c", "src/**.cpp", "src/**.cc"}

includedirs {"src", "src/Zephyr", "%{IncludeDir.GLFW}", "%{IncludeDir.spdlog}",
             "%{IncludeDir.glad}", "%{IncludeDir.glm}", "%{IncludeDir.ImGui}"}

links {"GLFW", "glad", "ImGui"}

targetdir("../Binaries/" .. outputdir .. "/%{prj.name}")
objdir("../Binaries/Intermediates/" .. outputdir .. "/%{prj.name}")

filter "system:windows"
systemversion "latest"

filter "configurations:Debug"
defines {"DEBUG", "ENABLE_ASSERTS"}
runtime "Debug"
symbols "On"

filter "configurations:Release"
defines {"RELEASE", "ENABLE_ASSERTS"}
runtime "Release"
optimize "On"
symbols "On"

filter "configurations:Dist"
defines {"DIST"}
runtime "Release"
optimize "On"
symbols "Off"
