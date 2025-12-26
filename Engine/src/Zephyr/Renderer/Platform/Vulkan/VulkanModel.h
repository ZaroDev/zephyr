#pragma once
#include "VulkanTypes.h"
#include "Renderer/Model.h"

namespace Zephyr
{
    class VulkanModel : public Model
    {
    public:
        VulkanModel(const std::array<std::vector<Mesh>, c_MaxLODCount>& meshes, u32 lodCount)
            : Model(meshes, lodCount)
        {
            for (u32 i = 0; i < lodCount; i++)
            {
                m_Buffers[i].resize(meshes[i].size());
            }
        }
        virtual ~VulkanModel() override = default;
        virtual AssetType GetType() const override { return Model::GetType(); }

        
        void AddBuffer(const GPUMeshBuffers& buffer, u32 lod, u32 index)
        {
            m_Buffers[lod][index] = buffer;
        }

        GPUMeshBuffers& GetBuffer(u32 lod, u32 index)
        {
            return m_Buffers[lod][index];
        }
        

    private:
        std::array<std::vector<GPUMeshBuffers>, c_MaxLODCount> m_Buffers;
    };
}
