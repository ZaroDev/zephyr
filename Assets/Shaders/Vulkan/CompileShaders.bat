@echo off


call %VULKAN_SDK%/Bin/glslc.exe gradient.comp -o gradient.comp.spv
call %VULKAN_SDK%/Bin/glslc.exe colored_triangle.frag -o colored_triangle.frag.spv
call %VULKAN_SDK%/Bin/glslc.exe colored_triangle.vert -o colored_triangle.vert.spv

call %VULKAN_SDK%/Bin/glslc.exe colored_triangle_mesh.frag -o colored_triangle_mesh.frag.spv
call %VULKAN_SDK%/Bin/glslc.exe colored_triangle_mesh.vert -o colored_triangle_mesh.vert.spv