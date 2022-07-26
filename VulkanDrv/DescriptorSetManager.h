#pragma once

#include "VulkanObjects.h"

class UVulkanRenderDevice;
class VulkanTexture;

class DescriptorSetManager
{
public:
	DescriptorSetManager(UVulkanRenderDevice* renderer);
	~DescriptorSetManager();

	VulkanDescriptorSet* GetTextureDescriptorSet(DWORD PolyFlags, VulkanTexture* tex, VulkanTexture* lightmap = nullptr, VulkanTexture* macrotex = nullptr, VulkanTexture* detailtex = nullptr, bool clamp = false);
	void ClearCache();

	VulkanDescriptorSetLayout* SceneDescriptorSetLayout = nullptr;

private:
	void CreateSceneDescriptorSetLayout();

	UVulkanRenderDevice* renderer = nullptr;

	std::vector<VulkanDescriptorPool*> SceneDescriptorPool;
	int SceneDescriptorPoolSetsLeft = 0;

	struct TexDescriptorKey
	{
		TexDescriptorKey(VulkanTexture* tex, VulkanTexture* lightmap, VulkanTexture* detailtex, VulkanTexture* macrotex, uint32_t sampler) : tex(tex), lightmap(lightmap), detailtex(detailtex), macrotex(macrotex), sampler(sampler) { }

		bool operator<(const TexDescriptorKey& other) const
		{
			if (tex != other.tex)
				return tex < other.tex;
			else if (lightmap != other.lightmap)
				return lightmap < other.lightmap;
			else if (detailtex != other.detailtex)
				return detailtex < other.detailtex;
			else if (macrotex != other.macrotex)
				return macrotex < other.macrotex;
			else
				return sampler < other.sampler;
		}

		VulkanTexture* tex;
		VulkanTexture* lightmap;
		VulkanTexture* detailtex;
		VulkanTexture* macrotex;
		uint32_t sampler;
	};

	std::map<TexDescriptorKey, VulkanDescriptorSet*> TextureDescriptorSets;
};
