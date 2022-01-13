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

#include "MemoryObjectDescr.hpp"
#include "Buffer.hpp"
#include "Image.hpp"
#include "BufferView.hpp"
#include "Sampler.hpp"

namespace QmVk {

template<typename T>
static vector<shared_ptr<MemoryObjectBase>> toMemoryObjectBaseVector(const vector<shared_ptr<T>> &inObjects)
{
    vector<shared_ptr<MemoryObjectBase>> objects;
    objects.reserve(objects.size());
    for (auto &&inObject : inObjects)
        objects.push_back(inObject);
    return objects;
}

MemoryObjectDescr::MemoryObjectDescr(
    const vector<shared_ptr<Buffer>> &buffers,
    Access access)
    : m_type(Type::Buffer)
    , m_access(access)
    , m_objects(toMemoryObjectBaseVector(buffers))
    , m_descriptorTypeInfos(getBufferDescriptorTypeInfos())
{}
MemoryObjectDescr::MemoryObjectDescr(
    const vector<shared_ptr<Image>> &images,
    const shared_ptr<Sampler> &sampler,
    uint32_t plane)
    : m_type(Type::Image)
    , m_objects(toMemoryObjectBaseVector(images))
    , m_sampler(sampler)
    , m_plane(plane)
    , m_descriptorTypeInfos(getImageDescriptorTypeInfos())
{}
MemoryObjectDescr::MemoryObjectDescr(
    const vector<shared_ptr<Image>> &images,
    Access access,
    uint32_t plane)
    : m_type(Type::Image)
    , m_access(access)
    , m_objects(toMemoryObjectBaseVector(images))
    , m_plane(plane)
    , m_descriptorTypeInfos(getImageDescriptorTypeInfos())
{}
MemoryObjectDescr::MemoryObjectDescr(
    const vector<shared_ptr<BufferView>> &bufferViews,
    Access access)
    : m_type(Type::BufferView)
    , m_access(access)
    , m_objects(toMemoryObjectBaseVector(bufferViews))
    , m_descriptorTypeInfos(getBufferViewDescriptorTypeInfos())
{}

MemoryObjectDescr::MemoryObjectDescr(
    const shared_ptr<Buffer> &buffer,
    Access access)
    : m_type(Type::Buffer)
    , m_access(access)
    , m_objects({buffer})
    , m_descriptorTypeInfos(getBufferDescriptorTypeInfos())
{}
MemoryObjectDescr::MemoryObjectDescr(
    const shared_ptr<Image> &image,
    const shared_ptr<Sampler> &sampler,
    uint32_t plane)
    : m_type(Type::Image)
    , m_objects({image})
    , m_sampler(sampler)
    , m_plane(plane)
    , m_descriptorTypeInfos(getImageDescriptorTypeInfos())
{}
MemoryObjectDescr::MemoryObjectDescr(
    const shared_ptr<Image> &image,
    Access access,
    uint32_t plane)
    : m_type(Type::Image)
    , m_access(access)
    , m_objects({image})
    , m_plane(plane)
    , m_descriptorTypeInfos(getImageDescriptorTypeInfos())
{}
MemoryObjectDescr::MemoryObjectDescr(
    const shared_ptr<BufferView> &bufferView,
    Access access)
    : m_type(Type::BufferView)
    , m_access(access)
    , m_objects({bufferView})
    , m_descriptorTypeInfos(getBufferViewDescriptorTypeInfos())
{}

void MemoryObjectDescr::prepareImage(
    vk::CommandBuffer commandBuffer,
    vk::PipelineStageFlags pipelineStageFlags) const
{
    if (m_type != Type::Image)
        return;

    size_t descriptorInfosIdx = 0;
    for (auto &&object : m_objects)
    {
        auto image = static_pointer_cast<Image>(object);
        image->pipelineBarrier(
            commandBuffer,
            descriptorInfos()[descriptorInfosIdx].descrImgInfo.imageLayout,
            pipelineStageFlags,
            (m_access == Access::Write)
                ? vk::AccessFlagBits::eShaderWrite
                : vk::AccessFlagBits::eShaderRead
        );
        descriptorInfosIdx += (m_plane == ~0u)
            ? image->numPlanes()
            : 1
        ;
    }
}
void MemoryObjectDescr::finalizeImage(
    vk::CommandBuffer commandBuffer) const
{
    if (m_type != Type::Image || m_access != Access::Write)
        return;

    for (auto &&object : m_objects)
    {
        auto image = static_pointer_cast<Image>(object);
        image->maybeGenerateMipmaps(commandBuffer);
    }
}

MemoryObjectDescr::DescriptorTypeInfos MemoryObjectDescr::getBufferDescriptorTypeInfos() const
{
    if (m_access == Access::Write)
        throw vk::LogicError("Bad buffer access");

    DescriptorTypeInfos descriptorTypeInfos;

    auto &descriptorType = descriptorTypeInfos.first;

    auto &descriptorInfos = descriptorTypeInfos.second;
    descriptorInfos.reserve(m_objects.size());

    for (auto &&object : m_objects)
    {
        auto buffer = static_pointer_cast<Buffer>(object);
        auto type = (m_access == Access::Read || (m_access == Access::Undefined && (buffer->usage() & vk::BufferUsageFlagBits::eUniformBuffer)))
            ? vk::DescriptorType::eUniformBuffer
            : vk::DescriptorType::eStorageBuffer
        ;

        if (descriptorType.descriptorCount == 0)
            descriptorType.type = type;
        else if (descriptorType.type != type)
            throw vk::LogicError("Inconsistent buffer types");

        descriptorInfos.push_back({{*buffer, 0, buffer->size()}});
    }

    descriptorType.descriptorCount = descriptorInfos.size();
    return descriptorTypeInfos;
}
MemoryObjectDescr::DescriptorTypeInfos MemoryObjectDescr::getImageDescriptorTypeInfos() const
{
    if ((m_access == Access::Undefined) == !m_sampler)
        throw vk::LogicError("Bad image access");

    DescriptorTypeInfos descriptorTypeInfos;

    auto &descriptorType = descriptorTypeInfos.first;
    descriptorType.type = m_sampler
        ? vk::DescriptorType::eCombinedImageSampler
        : vk::DescriptorType::eStorageImage
    ;

    auto &descriptorInfos = descriptorTypeInfos.second;

    auto sampler = m_sampler
        ? *m_sampler
        : vk::Sampler()
    ;
    auto imageLayout = m_sampler
        ? vk::ImageLayout::eShaderReadOnlyOptimal
        : vk::ImageLayout::eGeneral
    ;

    for (auto &&object : m_objects)
    {
        auto image = static_pointer_cast<Image>(object);

        const uint32_t n = (m_plane != ~0u)
            ? m_plane + 1
            : image->numPlanes()
        ;
        uint32_t i = (m_plane != ~0u)
            ? m_plane
            : 0
        ;

        for (; i < n; ++i)
        {
            descriptorInfos.push_back(vk::DescriptorImageInfo(
                sampler,
                image->imageView(i),
                imageLayout
            ));
        }
    }

    descriptorType.descriptorCount = descriptorInfos.size();
    return descriptorTypeInfos;
}
MemoryObjectDescr::DescriptorTypeInfos MemoryObjectDescr::getBufferViewDescriptorTypeInfos() const
{
    if (m_access == Access::Undefined || m_access == Access::Write)
        throw vk::LogicError("Bad buffer view access");

    DescriptorTypeInfos descriptorTypeInfos;

    auto &descriptorType = descriptorTypeInfos.first;

    auto &descriptorInfos = descriptorTypeInfos.second;
    descriptorInfos.reserve(m_objects.size());

    for (auto &&object : m_objects)
    {
        auto bufferView = static_pointer_cast<BufferView>(object);
        auto type = (m_access == Access::Read)
            ? vk::DescriptorType::eUniformTexelBuffer
            : vk::DescriptorType::eStorageTexelBuffer
        ;

        if (descriptorType.descriptorCount == 0)
            descriptorType.type = type;
        else if (descriptorType.type != type)
            throw vk::LogicError("Inconsistent buffer view types");

        descriptorInfos.push_back({*bufferView});
    }

    descriptorType.descriptorCount = descriptorInfos.size();
    return descriptorTypeInfos;
}

bool MemoryObjectDescr::operator ==(const MemoryObjectDescr &other) const
{
    return (
        m_type == other.m_type &&
        m_objects == other.m_objects &&
        m_access == other.m_access &&
        m_sampler == other.m_sampler && m_plane == other.m_plane
    );
}

}
