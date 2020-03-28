/*
    QmVk - simple Vulkan library created for QMPlay2
    Copyright (C) 2020  Błażej Szczygieł

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Buffer.hpp"
#include "PhysicalDevice.hpp"
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
        usage,
        Buffer::Priv()
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
    memoryPropertyFlags.required = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostCached;
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
    vk::DeviceMemory deviceMemory)
{
    auto buffer = make_shared<Buffer>(
        device,
        size,
        usage,
        Buffer::Priv()
    );
    buffer->m_deviceMemory.push_back(deviceMemory);
    buffer->m_dontFreeMemory = true;
    buffer->init(nullptr);
    return buffer;
}

Buffer::Buffer(
    const shared_ptr<Device> &device,
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    Priv)
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
    vk::BufferCreateInfo bufferCreateInfo;
    bufferCreateInfo.size = m_size;
    bufferCreateInfo.usage = m_usage;
    m_buffer = m_device->createBufferUnique(bufferCreateInfo);

    if (userMemoryPropertyFlags && m_deviceMemory.empty())
    {
        m_memoryRequirements = m_device->getBufferMemoryRequirements(*this);
        allocateMemory(*userMemoryPropertyFlags);
    }

    m_device->bindBufferMemory(*this, deviceMemory(), 0);
}

void Buffer::copyTo(
    const shared_ptr<Buffer> &dstBuffer,
    vk::CommandBuffer externalCommandBuffer)
{
    if (!(m_usage & vk::BufferUsageFlagBits::eTransferSrc))
        throw vk::LogicError("Source buffer is not flagged as transfer source");
    if (!(dstBuffer->m_usage & vk::BufferUsageFlagBits::eTransferDst))
        throw vk::LogicError("Destination buffer is not flagged as transfer destination");

    auto copyCommands = [&](vk::CommandBuffer commandBuffer) {
        vk::BufferCopy bufferCopy;
        bufferCopy.size = min(size(), dstBuffer->size());
        commandBuffer.copyBuffer(
            *this,
            *dstBuffer,
            bufferCopy
        );
    };

    if (externalCommandBuffer)
        copyCommands(externalCommandBuffer);
    else
        internalCommandBuffer()->execute(copyCommands);
}

void *Buffer::map()
{
    if (!m_mapped)
        m_mapped = m_device->mapMemory(deviceMemory(), 0, memorySize());

    return m_mapped;
}
void Buffer::unmap()
{
    if (!m_mapped)
        return;

    m_device->unmapMemory(deviceMemory());
    m_mapped = nullptr;
}

}
