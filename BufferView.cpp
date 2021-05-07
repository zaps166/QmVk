/*
    QmVk - simple Vulkan library created for QMPlay2
    Copyright (C) 2020-2021  Błażej Szczygieł

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

#include "BufferView.hpp"
#include "Device.hpp"
#include "Buffer.hpp"

namespace QmVk {

shared_ptr<BufferView> BufferView::create(
    const shared_ptr<Buffer> &buffer,
    vk::Format format,
    vk::DeviceSize offset,
    vk::DeviceSize range)
{
    auto bufferView = make_shared<BufferView>(
        buffer,
        format,
        offset,
        range,
        Priv()
    );
    bufferView->init();
    return bufferView;
}

BufferView::BufferView(
    const shared_ptr<Buffer> &buffer,
    vk::Format format,
    vk::DeviceSize offset,
    vk::DeviceSize range,
    Priv)
    : MemoryObjectBase(buffer->device())
    , m_buffer(buffer)
    , m_format(format)
    , m_offset(offset)
    , m_range(range)
{
}
BufferView::~BufferView()
{
    m_customData.reset();
}

void BufferView::init()
{
    vk::BufferViewCreateInfo bufferViewCreateInfo;
    bufferViewCreateInfo.buffer = *m_buffer;
    bufferViewCreateInfo.format = m_format;
    bufferViewCreateInfo.offset = m_offset;
    bufferViewCreateInfo.range = m_range;
    m_bufferView = m_device->createBufferViewUnique(bufferViewCreateInfo);
}

}
