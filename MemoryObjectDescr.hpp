// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include "DescriptorInfo.hpp"
#include "DescriptorType.hpp"
#include "MemoryObjectBase.hpp"

namespace QmVk {

using namespace std;

class Buffer;
class BufferView;
#ifndef QMVK_NO_GRAPHICS
class Image;
class Sampler;
#endif

class MemoryObjectDescr
{
    friend class MemoryObjectDescrs;

    using DescriptorTypeInfos = pair<DescriptorType, vector<DescriptorInfo>>;

public:
    enum class Type
    {
        Buffer,
        BufferView,
        Image,
    };
    enum class Access
    {
        Read,
        Write,
        Storage,
        StorageRead,
        StorageWrite,
    };

    using BufferRange = pair<vk::DeviceSize, vk::DeviceSize>;

public:
    MemoryObjectDescr(
        const vector<shared_ptr<Buffer>> &buffers,
        Access access = Access::Read,
        const vector<BufferRange> &ranges = {}
    );
#ifndef QMVK_NO_GRAPHICS
    MemoryObjectDescr(
        const vector<shared_ptr<Image>> &images,
        const shared_ptr<Sampler> &sampler,
        uint32_t plane = ~0u
    );
    MemoryObjectDescr(
        const vector<shared_ptr<Image>> &images,
        Access access = Access::Read,
        uint32_t plane = ~0u
    );
#endif
    MemoryObjectDescr(
        const vector<shared_ptr<BufferView>> &bufferViews,
        Access access = Access::Read
    );

    MemoryObjectDescr(
        const shared_ptr<Buffer> &buffer,
        Access access = Access::Read,
        const BufferRange &range = {}
    );
#ifndef QMVK_NO_GRAPHICS
    MemoryObjectDescr(
        const shared_ptr<Image> &image,
        const shared_ptr<Sampler> &sampler,
        uint32_t plane = ~0u
    );
    MemoryObjectDescr(
        const shared_ptr<Image> &image,
        Access access = Access::Read,
        uint32_t plane = ~0u
    );
#endif
    MemoryObjectDescr(
        const shared_ptr<BufferView> &bufferView,
        Access access = Access::Read
    );

public:
    inline const DescriptorType &descriptorType() const;
    inline const vector<DescriptorInfo> &descriptorInfos() const;

private:
    void prepareObject(
        vk::CommandBuffer commandBuffer,
        vk::PipelineStageFlags pipelineStageFlags
    ) const;
    void finalizeObject(
        vk::CommandBuffer commandBuffer
    ) const;

private:
    DescriptorTypeInfos getBufferDescriptorTypeInfos(const vector<BufferRange> &ranges) const;
#ifndef QMVK_NO_GRAPHICS
    DescriptorTypeInfos getImageDescriptorTypeInfos() const;
#endif
    DescriptorTypeInfos getBufferViewDescriptorTypeInfos() const;

public:
    bool operator ==(const MemoryObjectDescr &other) const;

private:
    Type m_type;
    Access m_access;
    vector<shared_ptr<MemoryObjectBase>> m_objects;

#ifndef QMVK_NO_GRAPHICS
    // Image
    shared_ptr<Sampler> m_sampler;
    uint32_t m_plane = ~0u;
#endif

    // Not used in comparison except buffer ranges
    DescriptorTypeInfos m_descriptorTypeInfos;
};

/* Inline implementation */

const DescriptorType &MemoryObjectDescr::descriptorType() const
{
    return m_descriptorTypeInfos.first;
}
const vector<DescriptorInfo> &MemoryObjectDescr::descriptorInfos() const
{
    return m_descriptorTypeInfos.second;
}

}
