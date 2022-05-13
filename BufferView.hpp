/*
    QmVk - simple Vulkan library created for QMPlay2
    Copyright (C) 2020-2022  Błażej Szczygieł

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

#include "MemoryObjectBase.hpp"

namespace QmVk {

using namespace std;

class CommandBuffer;
class Buffer;

class BufferView : public MemoryObjectBase
{
    struct Priv {};

public:
    static shared_ptr<BufferView> create(
        const shared_ptr<Buffer> &buffer,
        vk::Format format,
        vk::DeviceSize offset,
        vk::DeviceSize range
    );

public:
    BufferView(
        const shared_ptr<Buffer> &buffer,
        vk::Format format,
        vk::DeviceSize offset,
        vk::DeviceSize range,
        Priv
    );
    ~BufferView();

private:
    void init();

public:
    inline shared_ptr<Buffer> buffer() const;

    inline vk::Format format() const;
    inline vk::DeviceSize offset() const;
    inline vk::DeviceSize size() const;

    void copyTo(
        const shared_ptr<BufferView> &dstBufferView,
        const shared_ptr<CommandBuffer> &externalCommandBuffer = nullptr
    );

public:
    inline operator vk::BufferView() const;

private:
    const shared_ptr<Buffer> m_buffer;
    const vk::Format m_format;
    const vk::DeviceSize m_offset;
    const vk::DeviceSize m_range;

    vk::UniqueBufferView m_bufferView;
};

/* Inline implementation */

shared_ptr<Buffer> BufferView::buffer() const
{
    return m_buffer;
}

vk::Format BufferView::format() const
{
    return m_format;
}
vk::DeviceSize BufferView::offset() const
{
    return m_offset;
}
vk::DeviceSize BufferView::size() const
{
    return m_range;
}

BufferView::operator vk::BufferView() const
{
    return *m_bufferView;
}

}
