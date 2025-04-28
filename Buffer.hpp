// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2025 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include "MemoryObject.hpp"

namespace QmVk {

using namespace std;

class QMVK_EXPORT Buffer : public MemoryObject, public enable_shared_from_this<Buffer>
{
    Buffer(const Buffer &) = delete;

    friend class MemoryObjectDescr;

public:
    static shared_ptr<Buffer> create(
        const shared_ptr<Device> &device,
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        const MemoryPropertyFlags &memoryPropertyFlags
    );
    static shared_ptr<Buffer> createVerticesWrite(
        const shared_ptr<Device> &device,
        vk::DeviceSize size,
        bool requireDeviceLocal,
        uint32_t heap = ~0u
    );
    static shared_ptr<Buffer> createUniformWrite(
        const shared_ptr<Device> &device,
        vk::DeviceSize size,
        uint32_t heap = ~0u
    );
    static shared_ptr<Buffer> createUniformTexelBuffer(
        const shared_ptr<Device> &device,
        vk::DeviceSize size,
        uint32_t heap = ~0u
    );

    static shared_ptr<Buffer> createFromDeviceMemory(
        const shared_ptr<Device> &device,
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::DeviceMemory deviceMemory,
        vk::MemoryPropertyFlags memoryPropertyFlags,
        vk::UniqueBuffer *bufferIn = nullptr
    );

public:
    Buffer(
        const shared_ptr<Device> &device,
        vk::DeviceSize size,
        vk::BufferUsageFlags usage
    );
    ~Buffer();

private:
    void init(const MemoryPropertyFlags *userMemoryPropertyFlags);

public:
    inline vk::DeviceSize size() const;
    inline vk::BufferUsageFlags usage() const;

    void copyTo(
        const shared_ptr<Buffer> &dstBuffer,
        const shared_ptr<CommandBuffer> &externalCommandBuffer = nullptr,
        const vk::BufferCopy *bufferCopyIn = nullptr
    );

    void fill(
        uint32_t value,
        vk::DeviceSize offset,
        vk::DeviceSize size,
        const shared_ptr<CommandBuffer> &externalCommandBuffer = nullptr
    );

    void *map();
    template<typename T>
    inline T *map();
    void unmap();

    inline vk::PipelineStageFlags stage() const;
    inline vk::AccessFlags accessFlags() const;

public:
    inline operator vk::Buffer() const;

private:
    inline bool mustExecPipelineBarrier(
        vk::PipelineStageFlags dstStage,
        vk::AccessFlags dstAccessFlags
    );

    void pipelineBarrier(
        vk::CommandBuffer commandBuffer,
        vk::PipelineStageFlags dstStage,
        vk::AccessFlags dstAccessFlags
    );

private:
    const vk::DeviceSize m_size;
    const vk::BufferUsageFlags m_usage;

    vk::UniqueBuffer m_buffer;

    void *m_mapped = nullptr;

    bool m_dontFreeMemory = false;

    vk::PipelineStageFlags m_stage = vk::PipelineStageFlagBits::eTopOfPipe;
    vk::AccessFlags m_accessFlags;
};

/* Inline implementation */

vk::DeviceSize Buffer::size() const
{
    return m_size;
}
vk::BufferUsageFlags Buffer::usage() const
{
    return m_usage;
}

vk::PipelineStageFlags Buffer::stage() const
{
    return m_stage;
}
vk::AccessFlags Buffer::accessFlags() const
{
    return m_accessFlags;
}

template<typename T>
T *Buffer::map()
{
    return reinterpret_cast<T *>(map());
}

Buffer::operator vk::Buffer() const
{
    return *m_buffer;
}

}
