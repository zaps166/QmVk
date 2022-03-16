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

#include "QmVkExport.hpp"

#include "DescriptorInfo.hpp"
#include "MemoryObjectBase.hpp"

namespace QmVk {

using namespace std;

class Buffer;
class Image;
class BufferView;
class Sampler;

class QMVK_EXPORT MemoryObjectDescr
{
    friend class MemoryObjectDescrs;

    using DescriptorTypeInfos = pair<vk::DescriptorPoolSize, vector<DescriptorInfo>>;

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
    MemoryObjectDescr(
        const vector<shared_ptr<BufferView>> &bufferViews,
        Access access = Access::Read
    );

    MemoryObjectDescr(
        const shared_ptr<Buffer> &buffer,
        Access access = Access::Read,
        const BufferRange &range = {}
    );
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
    MemoryObjectDescr(
        const shared_ptr<BufferView> &bufferView,
        Access access = Access::Read
    );

public:
    inline const vk::DescriptorPoolSize &descriptorType() const;
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
    DescriptorTypeInfos getImageDescriptorTypeInfos() const;
    DescriptorTypeInfos getBufferViewDescriptorTypeInfos() const;

public:
    bool operator ==(const MemoryObjectDescr &other) const;

private:
    Type m_type;
    Access m_access;
    vector<shared_ptr<MemoryObjectBase>> m_objects;

    // Image
    shared_ptr<Sampler> m_sampler;
    uint32_t m_plane = ~0u;

    // Not used in comparison
    DescriptorTypeInfos m_descriptorTypeInfos;
};

/* Inline implementation */

const vk::DescriptorPoolSize &MemoryObjectDescr::descriptorType() const
{
    return m_descriptorTypeInfos.first;
}
const vector<DescriptorInfo> &MemoryObjectDescr::descriptorInfos() const
{
    return m_descriptorTypeInfos.second;
}

}
