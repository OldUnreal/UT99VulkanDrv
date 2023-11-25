#pragma once

class UVulkanRenderDevice;

class RenderPassManager
{
public:
	RenderPassManager(UVulkanRenderDevice* renderer);
	~RenderPassManager();

	std::unique_ptr<VulkanPipelineLayout> ScenePipelineLayout;
	std::unique_ptr<VulkanPipelineLayout> SceneBindlessPipelineLayout;
	std::unique_ptr<VulkanPipelineLayout> PresentPipelineLayout;

	void CreateRenderPass();
	void CreatePipelines();

	void CreatePresentRenderPass();
	void CreatePresentPipeline();

	void BeginScene(VulkanCommandBuffer* cmdbuffer, float r, float g, float b, float a);
	void EndScene(VulkanCommandBuffer* cmdbuffer);

	void BeginPresent(VulkanCommandBuffer* cmdbuffer);
	void EndPresent(VulkanCommandBuffer* cmdbuffer);

	VulkanPipeline* getPipeline(DWORD polyflags, bool bindless);
	VulkanPipeline* getEndFlashPipeline();
	VulkanPipeline* getLinePipeline(bool occludeLines, bool bindless) { return linepipeline[occludeLines][bindless].get(); }
	VulkanPipeline* getPointPipeline(bool occludeLines, bool bindless) { return pointpipeline[occludeLines][bindless].get(); }

	std::unique_ptr<VulkanRenderPass> SceneRenderPass;

	std::unique_ptr<VulkanRenderPass> PresentRenderPass;
	std::unique_ptr<VulkanPipeline> PresentPipeline[16];

private:
	void CreateScenePipelineLayout();
	void CreateSceneBindlessPipelineLayout();
	void CreatePresentPipelineLayout();

	UVulkanRenderDevice* renderer = nullptr;
	std::unique_ptr<VulkanPipeline> pipeline[2][32];
	std::unique_ptr<VulkanPipeline> linepipeline[2][2];
	std::unique_ptr<VulkanPipeline> pointpipeline[2][2];
};
