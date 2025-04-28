// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2025 Błażej Szczygieł
*/

#include "Buffer.hpp"
#include "Device.hpp"
#include "MemoryPropertyFlags.hpp"
#include "CommandBuffer.hpp"

namespace QmVk {

shared_ptr<Buffer> Buffer::create(
    const shared_ptr<Device> &device,
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    const MemoryPropertyFlags &memoryPropertyFlags)
{
    auto buffer = make_shared<Buffer>(
        device,
        size,
        usage
    );
    buffer->init(&memoryPropertyFlags);
    return buffer;
}
shared_ptr<Buffer> Buffer::createVerticesWrite(
    const shared_ptr<Device> &device,
    vk::DeviceSize size,
    bool requireDeviceLocal,
    uint32_t heap)
{
    MemoryPropertyFlags memoryPropertyFlags;
    vk::BufferUsageFlags transferUsage;

    if (requireDeviceLocal)
    {
        memoryPropertyFlags.required = vk::MemoryPropertyFlagBits::eDeviceLocal;
        memoryPropertyFlags.notWanted = vk::MemoryPropertyFlagBits::eHostVisible;

        transferUsage = vk::BufferUsageFlagBits::eTransferDst;
    }
    else
    {
        memoryPropertyFlags.required = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
        memoryPropertyFlags.optional = vk::MemoryPropertyFlagBits::eDeviceLocal;

        transferUsage = vk::BufferUsageFlagBits::eTransferSrc;
    }
    memoryPropertyFlags.heap = heap;

    return create(
        device,
        size,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer | transferUsage,
        memoryPropertyFlags
    );
}
shared_ptr<Buffer> Buffer::createUniformWrite(
    const shared_ptr<Device> &device,
    vk::DeviceSize size,
    uint32_t heap)
{
    MemoryPropertyFlags memoryPropertyFlags;
    memoryPropertyFlags.required = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    memoryPropertyFlags.optional = vk::MemoryPropertyFlagBits::eDeviceLocal;
    memoryPropertyFlags.heap = heap;
    return create(
        device,
        size,
        vk::BufferUsageFlagBits::eUniformBuffer,
        memoryPropertyFlags
    );
}
shared_ptr<Buffer> Buffer::createUniformTexelBuffer(
    const shared_ptr<Device> &device,
    vk::DeviceSize size,
    uint32_t heap)
{
    MemoryPropertyFlags memoryPropertyFlags;
    memoryPropertyFlags.required = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    memoryPropertyFlags.optional = vk::MemoryPropertyFlagBits::eHostCached;
    memoryPropertyFlags.heap = heap;
    return create(
        device,
        size,
        vk::BufferUsageFlagBits::eUniformTexelBuffer,
        memoryPropertyFlags
    );
}

shared_ptr<Buffer> Buffer::createFromDeviceMemory(
    const shared_ptr<Device> &device,
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::DeviceMemory deviceMemory,
    vk::MemoryPropertyFlags memoryPropertyFlags,
    vk::UniqueBuffer *bufferIn)
{
    auto buffer = make_shared<Buffer>(
        device,
        size,
        usage
    );
    buffer->m_memoryPropertyFlags = memoryPropertyFlags;
    buffer->m_deviceMemory.push_back(deviceMemory);
    buffer->m_dontFreeMemory = true;
    if (bufferIn)
        buffer->m_buffer = move(*bufferIn);
    buffer->init(nullptr);
    return buffer;
}

Buffer::Buffer(
    const shared_ptr<Device> &device,
    vk::DeviceSize size,
    vk::BufferUsageFlags usage)
    : MemoryObject(device)
    , m_size(size)
    , m_usage(usage)
{}
Buffer::~Buffer()
{
    unmap();
    if (m_dontFreeMemory)
        m_deviceMemory.clear();
}

void Buffer::init(const MemoryPropertyFlags *userMemoryPropertyFlags)
{
    if (!m_buffer)
    {
        const auto &enabledQueues = m_device->queues();

        vk::BufferCreateInfo bufferCreateInfo;
        bufferCreateInfo.size = m_size;
        bufferCreateInfo.usage = m_usage;
        if (enabledQueues.size() > 1)
        {
            bufferCreateInfo.sharingMode = vk::SharingMode::eConcurrent;
            bufferCreateInfo.queueFamilyIndexCount = enabledQueues.size();
            bufferCreateInfo.pQueueFamilyIndices = enabledQueues.data();
        }

        m_buffer = m_device->createBufferUnique(bufferCreateInfo, nullptr, dld());
    }

    m_memoryRequirements = m_device->getBufferMemoryRequirements(*this, dld());
    if (userMemoryPropertyFlags && m_deviceMemory.empty())
        allocateMemory(*userMemoryPropertyFlags);

    m_device->bindBufferMemory(*this, deviceMemory(), 0, dld());
}

void Buffer::copyTo(
    const shared_ptr<Buffer> &dstBuffer,
    const shared_ptr<CommandBuffer> &externalCommandBuffer,
    const vk::BufferCopy *bufferCopyIn)
{
    if (!(m_usage & vk::BufferUsageFlagBits::eTransferSrc))
        throw vk::LogicError("Source buffer is not flagged as transfer source");
    if (!(dstBuffer->m_usage & vk::BufferUsageFlagBits::eTransferDst))
        throw vk::LogicError("Destination buffer is not flagged as transfer destination");

    if (bufferCopyIn)
    {
        if (bufferCopyIn->srcOffset + bufferCopyIn->size > size())
            throw vk::LogicError("Source buffer overflow");
        if (bufferCopyIn->dstOffset + bufferCopyIn->size > dstBuffer->size())
            throw vk::LogicError("Destination buffer overflow");
    }

    auto copyCommands = [&](vk::CommandBuffer commandBuffer) {
        pipelineBarrier(
            commandBuffer,
            vk::PipelineStageFlagBits::eTransfer,
            vk::AccessFlagBits::eTransferRead
        );
        dstBuffer->pipelineBarrier(
            commandBuffer,
            vk::PipelineStageFlagBits::eTransfer,
            vk::AccessFlagBits::eTransferWrite
        );

        if (bufferCopyIn)
        {
            commandBuffer.copyBuffer(
                *this,
                *dstBuffer,
                *bufferCopyIn,
                dld()
            );
        }
        else
        {
            vk::BufferCopy bufferCopy;
            bufferCopy.size = min(size(), dstBuffer->size());
            commandBuffer.copyBuffer(
                *this,
                *dstBuffer,
                bufferCopy,
                dld()
            );
        }
    };

    if (externalCommandBuffer)
    {
        externalCommandBuffer->storeData(shared_from_this());
        externalCommandBuffer->storeData(dstBuffer);
        copyCommands(*externalCommandBuffer);
    }
    else
    {
        internalCommandBuffer()->execute(copyCommands);
    }
}

void Buffer::fill(
    uint32_t value,
    vk::DeviceSize offset,
    vk::DeviceSize size,
    const shared_ptr<CommandBuffer> &externalCommandBuffer)
{
    if (!(m_usage & vk::BufferUsageFlagBits::eTransferDst))
        throw vk::LogicError("Buffer is not flagged as transfer destination");

    if (offset + size > this->size())
        throw vk::LogicError("Buffer overflow");

    auto fillCommands = [&](vk::CommandBuffer commandBuffer) {
        pipelineBarrier(
            commandBuffer,
            vk::PipelineStageFlagBits::eTransfer,
            vk::AccessFlagBits::eTransferWrite
        );
        commandBuffer.fillBuffer(*m_buffer, offset, size, value, dld());
    };

    if (externalCommandBuffer)
    {
        externalCommandBuffer->storeData(shared_from_this());
        fillCommands(*externalCommandBuffer);
    }
    else
    {
        internalCommandBuffer()->execute(fillCommands);
    }
}

void *Buffer::map()
{
    if (!m_mapped)
        m_mapped = m_device->mapMemory(deviceMemory(), 0, memorySize(), {}, dld());

    return m_mapped;
}
void Buffer::unmap()
{
    if (!m_mapped)
        return;

    m_device->unmapMemory(deviceMemory(), dld());
    m_mapped = nullptr;
}

inline bool Buffer::mustExecPipelineBarrier(
    vk::PipelineStageFlags dstStage,
    vk::AccessFlags dstAccessFlags)
{
    if (m_stage != dstStage || m_accessFlags != dstAccessFlags)
        return true;
    if ((m_accessFlags & vk::AccessFlagBits::eShaderRead) && (m_accessFlags & vk::AccessFlagBits::eShaderWrite))
        return true;
    return false;
}

void Buffer::pipelineBarrier(
    vk::CommandBuffer commandBuffer,
    vk::PipelineStageFlags dstStage,
    vk::AccessFlags dstAccessFlags)
{
    if (!mustExecPipelineBarrier(dstStage, dstAccessFlags))
        return;

    vk::BufferMemoryBarrier barrier(
        m_accessFlags,
        dstAccessFlags,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        *m_buffer,
        0,
        size()
    );
    commandBuffer.pipelineBarrier(
        m_stage,
        dstStage,
        vk::DependencyFlags(),
        0,
        nullptr,
        1,
        &barrier,
        0,
        nullptr,
        dld()
    );

    m_stage = dstStage;
    m_accessFlags = dstAccessFlags;
}

}
