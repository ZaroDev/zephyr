#pragma once
#include <Zephyr/Core/Log.h>
#include <vulkan/vulkan_core.h>

#define VK_CHECK(x) (x == VK_SUCCESS)
#define VK_ASSERT(x) if(!VK_CHECK(x)) CORE_ASSERT(false);


#define VK_TRACE(...) CORE_TRACE(__VA_ARGS__)
#define VK_INFO(...) CORE_INFO(__VA_ARGS__)
#define VK_WARN(...) CORE_WARN(__VA_ARGS__)
#define VK_ERROR(...) CORE_ERROR(__VA_ARGS__)
#define VK_CRITICAL(...) CORE_CRITICAL(__VA_ARGS__)