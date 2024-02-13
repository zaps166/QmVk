// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include "MemoryObjectBase.hpp"

namespace QmVk {

using namespace std;

class CommandBuffer;
class Buffer;

class QMVK_EXPORT BufferView : public MemoryObjectBase
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
