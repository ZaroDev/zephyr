IncludeDir = {}
IncludeDir["GLFW"] = "%{wks.location}/Engine/Vendor/GLFW/include"
IncludeDir["spdlog"] = "%{wks.location}/Engine/Vendor/spdlog/include"
IncludeDir["glad"] = "%{wks.location}/Engine/Vendor/glad/include"
IncludeDir["assimp"] = "%{wks.location}/Engine/Vendor/assimp/include"
IncludeDir["glm"] = "%{wks.location}/Engine/Vendor/glm"
IncludeDir["meshoptimizer"] = "%{wks.location}/Engine/Vendor/meshoptimizer/include"
IncludeDir["ImGui"] = "%{wks.location}/Engine/Vendor/ImGui"
IncludeDir["ImGuizmo"] = "%{wks.location}/Editor/Vendor/ImGuizmo"
IncludeDir["IconHeaders"] = "%{wks.location}/Editor/Vendor/IconFontCppHeaders"

LibraryDir = {}
LibraryDir["assimp"] = "%{wks.location}/Engine/Vendor/assimp/lib/%{cfg.buildcfg}"
LibraryDir["meshoptimizer"] = "%{wks.location}/Engine/Vendor/meshoptimizer/lib/%{cfg.buildcfg}"

Library = {}
Library["assimp"] = "%{LibraryDir.assimp}/assimp.lib"
Library["meshoptimizer"] = "%{LibraryDir.meshoptimizer}/meshoptimizer.lib"
