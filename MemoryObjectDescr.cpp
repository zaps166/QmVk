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
    const shared_ptr<Buffer> &buffer)
    : m_type(Type::Buffer)
    , m_object(buffer)
    , m_descriptorTypeInfos(getBufferDescriptorTypeInfos())
{}
MemoryObjectDescr::MemoryObjectDescr(
    const shared_ptr<Image> &image,
    const shared_ptr<Sampler> &sampler,
    uint32_t plane)
    : m_type(Type::Image)
    , m_object(image)
    , m_imageAccess(ImageAccess::ReadSampler)
    , m_sampler(sampler)
    , m_plane(plane)
    , m_descriptorTypeInfos(getImageDescriptorTypeInfos())
{}
MemoryObjectDescr::MemoryObjectDescr(
    const shared_ptr<Image> &image,
    ImageAccess imageAccess,
    uint32_t plane)
    : m_type(Type::Image)
    , m_object(image)
    , m_imageAccess(imageAccess)
    , m_plane(plane)
    , m_descriptorTypeInfos(getImageDescriptorTypeInfos())
{
    if (imageAccess != ImageAccess::Read && imageAccess != ImageAccess::Write)
        throw vk::LogicError("Bad image access");
}
MemoryObjectDescr::MemoryObjectDescr(
    const shared_ptr<BufferView> &bufferView)
    : m_type(Type::BufferView)
    , m_object(bufferView)
    , m_descriptorTypeInfos(getBufferViewDescriptorTypeInfos())
{}

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
        (m_imageAccess == ImageAccess::Write)
            ? vk::AccessFlagBits::eShaderWrite
            : vk::AccessFlagBits::eShaderRead
    );
}
void MemoryObjectDescr::finalizeImage(
    vk::CommandBuffer commandBuffer) const
{
    if (m_type != Type::Image || m_imageAccess != ImageAccess::Write)
        return;

    static_pointer_cast<Image>(m_object)->maybeGenerateMipmaps(commandBuffer);
}

MemoryObjectDescr::DescriptorTypeInfos MemoryObjectDescr::getBufferDescriptorTypeInfos() const
{
    auto buffer = static_pointer_cast<Buffer>(m_object);
    auto type = buffer->isUniform()
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
    auto type = (m_imageAccess == ImageAccess::ReadSampler)
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

    vk::Sampler sampler = (m_imageAccess == ImageAccess::ReadSampler && m_sampler)
        ? *m_sampler
        : vk::Sampler()
    ;
    vk::ImageLayout imageLayout = (m_imageAccess == ImageAccess::ReadSampler)
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
    return {
        {vk::DescriptorType::eUniformTexelBuffer, 1},
        {{*static_pointer_cast<BufferView>(m_object)}},
    };
}

bool MemoryObjectDescr::operator ==(const MemoryObjectDescr &other) const
{
    return (
        m_type == other.m_type &&
        m_object == other.m_object &&
        m_imageAccess == other.m_imageAccess && m_sampler == other.m_sampler && m_plane == other.m_plane
    );
}

}
