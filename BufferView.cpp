// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2023 Błażej Szczygieł
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

void BufferView::copyTo(
    const shared_ptr<BufferView> &dstBufferView,
    const shared_ptr<CommandBuffer> &externalCommandBuffer)
{
    vk::BufferCopy bufferCopy;
    bufferCopy.srcOffset = m_offset;
    bufferCopy.dstOffset = dstBufferView->offset();
    bufferCopy.size = min(m_range, dstBufferView->m_range);
    m_buffer->copyTo(
        dstBufferView->buffer(),
        externalCommandBuffer,
        &bufferCopy
    );
}

}
