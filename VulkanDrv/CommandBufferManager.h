#pragma once

#include <array>
#include <cstdint>

class UVulkanRenderDevice;

class CommandBufferManager
{
public:
	CommandBufferManager(UVulkanRenderDevice* renderer);
	~CommandBufferManager();

	void BeginFrame();
	void WaitForTransfer();
	void SubmitCommands(bool present, int presentWidth, int presentHeight, bool presentFullscreen);
	VulkanCommandBuffer* GetTransferCommands();
	VulkanCommandBuffer* GetDrawCommands();
	void DeleteFrameObjects();

	struct DeleteList
	{
		std::vector<std::unique_ptr<VulkanImage>> images;
		std::vector<std::unique_ptr<VulkanImageView>> imageViews;
		std::vector<std::unique_ptr<VulkanBuffer>> buffers;
		std::vector<std::unique_ptr<VulkanDescriptorSet>> descriptors;
	};
	std::array<std::unique_ptr<DeleteList>, MAX_FRAMES_IN_FLIGHT> FrameDeleteLists;
	DeleteList* GetCurrentDeleteList() { return FrameDeleteLists[CurrentFrameIndex].get(); }

	std::shared_ptr<VulkanSwapChain> SwapChain;
	int PresentImageIndex = -1;
	BITFIELD UsingVsync = 0;
	BITFIELD UsingHdr = 0;

private:
	UVulkanRenderDevice* renderer = nullptr;

	std::array<std::unique_ptr<VulkanSemaphore>, MAX_FRAMES_IN_FLIGHT> ImageAvailableSemaphores;
	std::array<std::unique_ptr<VulkanSemaphore>, MAX_FRAMES_IN_FLIGHT> RenderFinishedSemaphores;
	std::array<std::unique_ptr<VulkanSemaphore>, MAX_FRAMES_IN_FLIGHT> DrawFinishedSemaphores;
	std::array<std::unique_ptr<VulkanSemaphore>, MAX_FRAMES_IN_FLIGHT> TransferSemaphores;
	std::array<std::unique_ptr<VulkanFence>, MAX_FRAMES_IN_FLIGHT> RenderFinishedFences;
	std::unique_ptr<VulkanCommandPool> CommandPool;
	std::array<std::unique_ptr<VulkanCommandBuffer>, MAX_FRAMES_IN_FLIGHT> DrawCommandsArray;
	std::array<std::unique_ptr<VulkanCommandBuffer>, MAX_FRAMES_IN_FLIGHT> TransferCommandsArray;

	bool FrameBegun = false;
	bool IsFirstFrame = true;
	std::array<bool, MAX_FRAMES_IN_FLIGHT> DrawCommandsBegun = {};
	std::array<bool, MAX_FRAMES_IN_FLIGHT> TransferCommandsBegun = {};
};
