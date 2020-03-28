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

#include "MemoryObjectDescr.hpp"
#include "Buffer.hpp"
#include "Image.hpp"
#include "BufferView.hpp"
#include "Sampler.hpp"

namespace QmVk {

MemoryObjectDescr::MemoryObjectDescr(
    const shared_ptr<Buffer> &buffer,
    Access access)
    : m_type(Type::Buffer)
    , m_access(access)
    , m_object(buffer)
    , m_descriptorTypeInfos(getBufferDescriptorTypeInfos())
{}
MemoryObjectDescr::MemoryObjectDescr(
    const shared_ptr<Image> &image,
    const shared_ptr<Sampler> &sampler,
    uint32_t plane)
    : m_type(Type::Image)
    , m_object(image)
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
    , m_object(image)
    , m_plane(plane)
    , m_descriptorTypeInfos(getImageDescriptorTypeInfos())
{
    if (m_access == Access::Undefined)
        throw vk::LogicError("Bad image access");
}
MemoryObjectDescr::MemoryObjectDescr(
    const shared_ptr<BufferView> &bufferView,
    Access access)
    : m_type(Type::BufferView)
    , m_access(access)
    , m_object(bufferView)
    , m_descriptorTypeInfos(getBufferViewDescriptorTypeInfos())
{
    if (m_access == Access::Undefined)
        throw vk::LogicError("Bad buffer view access");
}

void MemoryObjectDescr::prepareImage(
    vk::CommandBuffer commandBuffer,
    vk::PipelineStageFlags pipelineStageFlags) const
{
    if (m_type != Type::Image)
        return;

    static_pointer_cast<Image>(m_object)->pipelineBarrier(
        commandBuffer,
        descriptorInfos()[0].descrImgInfo.imageLayout,
        pipelineStageFlags,
        (m_access == Access::Write)
            ? vk::AccessFlagBits::eShaderWrite
            : vk::AccessFlagBits::eShaderRead
    );
}
void MemoryObjectDescr::finalizeImage(
    vk::CommandBuffer commandBuffer) const
{
    if (m_type != Type::Image || m_access != Access::Write)
        return;

    static_pointer_cast<Image>(m_object)->maybeGenerateMipmaps(commandBuffer);
}

MemoryObjectDescr::DescriptorTypeInfos MemoryObjectDescr::getBufferDescriptorTypeInfos() const
{
    auto buffer = static_pointer_cast<Buffer>(m_object);
    auto type = (m_access == Access::Read || (m_access == Access::Undefined && (buffer->usage() & vk::BufferUsageFlagBits::eUniformBuffer)))
        ? vk::DescriptorType::eUniformBuffer
        : vk::DescriptorType::eStorageBuffer
    ;
    return {
        {type, 1},
        {{{*buffer, 0, buffer->size()}}},
    };
}
MemoryObjectDescr::DescriptorTypeInfos MemoryObjectDescr::getImageDescriptorTypeInfos() const
{
    auto type = m_sampler
        ? vk::DescriptorType::eCombinedImageSampler
        : vk::DescriptorType::eStorageImage
    ;
    uint32_t descriptorTypeCount = 0;

    auto image = static_pointer_cast<Image>(m_object);

    const uint32_t n = (m_plane != ~0u)
        ? m_plane + 1
        : image->numPlanes()
    ;
    uint32_t i = (m_plane != ~0u)
        ? m_plane
        : 0
    ;

    auto sampler = m_sampler
        ? *m_sampler
        : vk::Sampler()
    ;
    auto imageLayout = m_sampler
        ? vk::ImageLayout::eShaderReadOnlyOptimal
        : vk::ImageLayout::eGeneral
    ;

    vector<DescriptorInfo> descriptorInfos;
    for (; i < n; ++i)
    {
        descriptorInfos.push_back(vk::DescriptorImageInfo(
            sampler,
            image->imageView(i),
            imageLayout
        ));
        ++descriptorTypeCount;
    }

    return {
        {type, descriptorTypeCount},
        descriptorInfos,
    };
}
MemoryObjectDescr::DescriptorTypeInfos MemoryObjectDescr::getBufferViewDescriptorTypeInfos() const
{
    auto descriptorType = (m_access == Access::Read)
        ? vk::DescriptorType::eUniformTexelBuffer
        : vk::DescriptorType::eStorageTexelBuffer
    ;
    return {
        {descriptorType, 1},
        {{*static_pointer_cast<BufferView>(m_object)}},
    };
}

bool MemoryObjectDescr::operator ==(const MemoryObjectDescr &other) const
{
    return (
        m_type == other.m_type &&
        m_object == other.m_object &&
        m_access == other.m_access &&
        m_sampler == other.m_sampler && m_plane == other.m_plane
    );
}

}
