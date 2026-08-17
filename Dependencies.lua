VULKAN_SDK = os.getenv("VULKAN_SDK")

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
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["nvrhi"] ="%{wks.location}/Engine/Vendor/nvrhi/include"
-- nvrhi/d3d12.h includes <directx/d3d12.h> from the DirectX-Headers submodule it vendors -
-- NVRHI-D3D12's own project has this on its include path, but Engine (which also includes
-- nvrhi/d3d12.h directly from RHI/D3D12Device.cpp/Device.cpp) needs it too.
IncludeDir["DirectXHeaders"] ="%{wks.location}/Engine/Vendor/nvrhi/thirdparty/DirectX-Headers/include"

LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
LibraryDir["assimp"] = "%{wks.location}/Engine/Vendor/assimp/lib/%{cfg.buildcfg}"
LibraryDir["meshoptimizer"] = "%{wks.location}/Engine/Vendor/meshoptimizer/lib/%{cfg.buildcfg}"

Library = {}
Library["VulkanSDK"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
Library["slang"] = "%{LibraryDir.VulkanSDK}/slang.lib"
Library["assimp"] = "%{LibraryDir.assimp}/assimp.lib"
Library["meshoptimizer"] = "%{LibraryDir.meshoptimizer}/meshoptimizer.lib"
