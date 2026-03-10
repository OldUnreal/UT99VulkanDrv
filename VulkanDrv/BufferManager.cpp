
#include "Precomp.h"
#include "BufferManager.h"
#include "UVulkanRenderDevice.h"

BufferManager::BufferManager(UVulkanRenderDevice* renderer) : renderer(renderer)
{
	CreateSceneVertexBuffer();
	CreateSceneIndexBuffer();
	CreateUploadBuffer();
}

BufferManager::~BufferManager()
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		if (SceneVerticesArray[i]) { SceneVertexBuffers[i]->Unmap(); SceneVerticesArray[i] = nullptr; }
		if (SceneIndexesArray[i]) { SceneIndexBuffers[i]->Unmap(); SceneIndexesArray[i] = nullptr; }
	}
}

void BufferManager::CreateSceneVertexBuffer()
{
	size_t size = sizeof(SceneVertex) * SceneVertexBufferSize;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		SceneVertexBuffers[i] = BufferBuilder()
			.Usage(
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VMA_MEMORY_USAGE_UNKNOWN, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
			.MemoryType(
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				// Buggie: Omit VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT. 
				// Allocating mapped buffers in VRAM forces the CPU to write directly over the PCIe bus. 
				// If the GPU drops to a low frequency, these PCIe transactions become extremely slow, 
				// causing massive CPU stalls and OS-level WDDM lag. Keeping this in System RAM is much faster.
				//| VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			)
			.Size(size)
			.DebugName("SceneVertexBuffer")
			.Create(renderer->Device.get());

		SceneVerticesArray[i] = (SceneVertex*)SceneVertexBuffers[i]->Map(0, size);
	}
}

void BufferManager::CreateSceneIndexBuffer()
{
	size_t size = sizeof(uint32_t) * SceneIndexBufferSize;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		SceneIndexBuffers[i] = BufferBuilder()
			.Usage(
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VMA_MEMORY_USAGE_UNKNOWN, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
			.MemoryType(
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				// Buggie: Omit VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT. 
				// See comment above.
				//| VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			)
			.Size(size)
			.DebugName("SceneIndexBuffer")
			.Create(renderer->Device.get());

		SceneIndexesArray[i] = (uint32_t*)SceneIndexBuffers[i]->Map(0, size);
	}
}

void BufferManager::CreateUploadBuffer()
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		UploadBuffers[i] = BufferBuilder()
			.Usage(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VMA_MEMORY_USAGE_UNKNOWN, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
			.MemoryType(
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				// Buggie: Omit VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT. 
				// See comment above.
				//| VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			)
			.Size(UploadBufferSize)
			.DebugName("UploadBuffer")
			.Create(renderer->Device.get());

		UploadDataArray[i] = (uint8_t*)UploadBuffers[i]->Map(0, UploadBufferSize);
	}
}
