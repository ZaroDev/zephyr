project "glad"
kind "StaticLib"
language "C"

targetdir("%{wks.location}/Binaries/" .. outputdir .. "/%{prj.name}")
objdir("%{wks.location}/Binaries/Intermediates/" .. outputdir .. "/%{prj.name}")

includedirs {"include"}
files {"include/glad/glad.h", "include/KHR/khrplatform.h", "src/glad.c"}
