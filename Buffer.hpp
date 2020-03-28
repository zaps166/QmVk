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

#pragma once

#include "QmVkExport.hpp"

#include "MemoryObject.hpp"

namespace QmVk {

using namespace std;

class QMVK_EXPORT Buffer : public MemoryObject
{
    Buffer(const Buffer &) = delete;

    struct Priv {};

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

public:
    Buffer(
        const shared_ptr<Device> &device,
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        Priv
    );
    ~Buffer();

private:
    void init(const MemoryPropertyFlags &userMemoryPropertyFlags);

public:
    inline vk::DeviceSize size() const;

    inline bool isUniform() const;

    void copyTo(
        const shared_ptr<Buffer> &dstBuffer,
        vk::CommandBuffer externalCommandBuffer
    );

    void *map();
    template<typename T>
    inline T *map();
    void unmap();

public:
    inline operator vk::Buffer() const;

private:
    const vk::DeviceSize m_size;
    const vk::BufferUsageFlags m_usage;

    vk::UniqueBuffer m_buffer;

    void *m_mapped = nullptr;
};

/* Inline implementation */

vk::DeviceSize Buffer::size() const
{
    return m_size;
}

bool Buffer::isUniform() const
{
    return bool(m_usage & vk::BufferUsageFlagBits::eUniformBuffer);
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
