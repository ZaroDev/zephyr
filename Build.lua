-- premake5.lua
workspace "Zephyr"
architecture "x64"
configurations {"Debug", "Release", "Dist"}
startproject "Editor"

-- Workspace-wide build options for MSVC
filter "system:windows"
buildoptions {"/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus"}

defines{ "_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS", "_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING" }

outputdir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"

include "Dependencies.lua"

group "Engine"
include "Engine/Build-Engine.lua"
group ""

group "Engine/Dependencies"
include "Engine/Vendor/GLFW"
include "Engine/Vendor/glad"
include "Engine/Vendor/ImGui"
include "Engine/Vendor/nvrhi"
group ""



group "Editor"
include "Editor/Build-Editor.lua"
group ""
