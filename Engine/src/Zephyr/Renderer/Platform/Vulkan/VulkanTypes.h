#pragma once
#include <vma/vk_mem_alloc.h>


namespace Zephyr
{
    struct AllocatedBuffer
    {
        VkBuffer Buffer;
        VmaAllocation Allocation;
        VmaAllocationInfo Info;
    };

    struct GPUMeshBuffers
    {
        AllocatedBuffer IndexBuffer;
        AllocatedBuffer VertexBuffer;
        VkDeviceAddress VertexBufferAddress;
    };

    struct GPUPushDrawConstants
    {
        Mat4 World;
        VkDeviceAddress VertexBuffer;
    };
}