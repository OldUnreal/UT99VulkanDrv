#pragma once

#include "Precomp.h"
#include "ShaderManager.h"
#include <array>
#include <cstddef>

class UVulkanRenderDevice;
struct SceneVertex;

class BufferManager
{
public:
	BufferManager(UVulkanRenderDevice* renderer);
	~BufferManager();

	std::array<std::unique_ptr<VulkanBuffer>, MAX_FRAMES_IN_FLIGHT> SceneVertexBuffers;
	std::array<std::unique_ptr<VulkanBuffer>, MAX_FRAMES_IN_FLIGHT> SceneIndexBuffers;
	std::array<std::unique_ptr<VulkanBuffer>, MAX_FRAMES_IN_FLIGHT> UploadBuffers;

	std::array<SceneVertex*, MAX_FRAMES_IN_FLIGHT> SceneVerticesArray = {};
	std::array<uint32_t*, MAX_FRAMES_IN_FLIGHT> SceneIndexesArray = {};
	std::array<uint8_t*, MAX_FRAMES_IN_FLIGHT> UploadDataArray = {};

	std::array<size_t, MAX_FRAMES_IN_FLIGHT> UploadBufferPositions = { 0 };

	static const int SceneVertexBufferSize = 1 * 1024 * 1024;
	static const int SceneIndexBufferSize = 1 * 1024 * 1024;

	static const int UploadBufferSize = 64 * 1024 * 1024;

private:
	void CreateSceneVertexBuffer();
	void CreateSceneIndexBuffer();
	void CreateUploadBuffer();

	UVulkanRenderDevice* renderer = nullptr;
};
