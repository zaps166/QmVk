// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#include "MemoryObjectDescr.hpp"
#include "Buffer.hpp"
#include "BufferView.hpp"
#ifndef QMVK_NO_GRAPHICS
#   include "Image.hpp"
#   include "Sampler.hpp"
#endif

namespace QmVk {

template<typename T>
static vector<weak_ptr<MemoryObjectBase>> toMemoryObjectBaseVector(const vector<shared_ptr<T>> &inObjects)
{
    vector<weak_ptr<MemoryObjectBase>> objects;
    objects.reserve(objects.size());
    for (auto &&inObject : inObjects)
        objects.push_back(inObject);
    return objects;
}

MemoryObjectDescr::MemoryObjectDescr(
    const vector<shared_ptr<Buffer>> &buffers,
    Access access,
    const vector<BufferRange> &ranges)
    : m_type(Type::Buffer)
    , m_access(access)
    , m_objects(toMemoryObjectBaseVector(buffers))
    , m_descriptorTypeInfos(getBufferDescriptorTypeInfos(ranges))
{}
#ifndef QMVK_NO_GRAPHICS
MemoryObjectDescr::MemoryObjectDescr(
    const vector<shared_ptr<Image>> &images,
    const shared_ptr<Sampler> &sampler,
    uint32_t plane)
    : m_type(Type::Image)
    , m_access(Access::Read)
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
#endif
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
    Access access,
    const BufferRange &range)
    : m_type(Type::Buffer)
    , m_access(access)
    , m_objects({buffer})
    , m_descriptorTypeInfos(getBufferDescriptorTypeInfos({range}))
{}
#ifndef QMVK_NO_GRAPHICS
MemoryObjectDescr::MemoryObjectDescr(
    const shared_ptr<Image> &image,
    const shared_ptr<Sampler> &sampler,
    uint32_t plane)
    : m_type(Type::Image)
    , m_access(Access::Read)
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
#endif
MemoryObjectDescr::MemoryObjectDescr(
    const shared_ptr<BufferView> &bufferView,
    Access access)
    : m_type(Type::BufferView)
    , m_access(access)
    , m_objects({bufferView})
    , m_descriptorTypeInfos(getBufferViewDescriptorTypeInfos())
{}

void MemoryObjectDescr::prepareObject(
    vk::CommandBuffer commandBuffer,
    vk::PipelineStageFlags pipelineStageFlags) const
{
    vk::AccessFlags accessFlag = {};
    switch (m_access)
    {
        case Access::Read:
        case Access::StorageRead:
            accessFlag = vk::AccessFlagBits::eShaderRead;
            break;
        case Access::Write:
        case Access::StorageWrite:
            accessFlag = vk::AccessFlagBits::eShaderWrite;
            break;
        case Access::Storage:
            accessFlag = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
            break;
    }

#ifndef QMVK_NO_GRAPHICS
    size_t descriptorInfosIdx = 0;
#endif
    for (auto &&objectWeak : m_objects)
    {
        auto object = objectWeak.lock();
        assert(object);

        switch (m_type)
        {
            case Type::Buffer:
            case Type::BufferView:
            {
                auto buffer = (m_type == Type::BufferView)
                    ? static_pointer_cast<BufferView>(object)->buffer()
                    : static_pointer_cast<Buffer>(object)
                ;
                buffer->pipelineBarrier(
                    commandBuffer,
                    pipelineStageFlags,
                    accessFlag
                );
                break;
            }
            case Type::Image:
            {
#ifndef QMVK_NO_GRAPHICS
                auto image = static_pointer_cast<Image>(object);
                image->pipelineBarrier(
                    commandBuffer,
                    descriptorInfos()[descriptorInfosIdx].descrImgInfo.imageLayout,
                    pipelineStageFlags,
                    accessFlag
                );
                descriptorInfosIdx += (m_plane == ~0u && !image->samplerYcbcr())
                    ? image->numPlanes()
                    : 1
                ;
#endif
                break;
            }
        }
    }
}
void MemoryObjectDescr::finalizeObject(
    vk::CommandBuffer commandBuffer,
    bool genMipmapsOnWrite,
    bool resetPipelineStageFlags) const
{
    if (!genMipmapsOnWrite && !resetPipelineStageFlags)
        return;

    for (auto &&objectWeak : m_objects)
    {
        auto object = objectWeak.lock();
        assert(object);

        switch (m_type)
        {
            case Type::Buffer:
            case Type::BufferView:
            {
                if (resetPipelineStageFlags)
                {
                    auto buffer = (m_type == Type::BufferView)
                        ? static_pointer_cast<BufferView>(object)->buffer()
                        : static_pointer_cast<Buffer>(object)
                    ;
                    buffer->pipelineBarrier(
                        commandBuffer,
                        vk::PipelineStageFlagBits::eBottomOfPipe,
                        vk::AccessFlags()
                    );
                }
                break;
            }
            case Type::Image:
            {
#ifndef QMVK_NO_GRAPHICS
                auto image = static_pointer_cast<Image>(object);
                if (genMipmapsOnWrite && m_access == Access::Write)
                {
                    image->maybeGenerateMipmaps(commandBuffer);
                }
                if (resetPipelineStageFlags)
                {
                    image->pipelineBarrier(
                        commandBuffer,
                        image->imageLayout(),
                        vk::PipelineStageFlagBits::eBottomOfPipe,
                        vk::AccessFlags()
                    );
                }
#endif
                break;
            }
        }
    }
}

MemoryObjectDescr::DescriptorTypeInfos MemoryObjectDescr::getBufferDescriptorTypeInfos(const vector<BufferRange> &ranges) const
{
    if (m_access == Access::Write)
        throw vk::LogicError("Bad buffer access");

    DescriptorTypeInfos descriptorTypeInfos;

    auto &descriptorType = descriptorTypeInfos.first;

    auto &descriptorInfos = descriptorTypeInfos.second;
    descriptorInfos.reserve(m_objects.size());

    size_t i = 0;
    for (auto &&objectWeak : m_objects)
    {
        auto object = objectWeak.lock();
        assert(object);

        auto buffer = static_pointer_cast<Buffer>(object);
        auto type = (m_access == Access::Read)
            ? vk::DescriptorType::eUniformBuffer
            : vk::DescriptorType::eStorageBuffer
        ;

        if (descriptorType.descriptorCount == 0)
            descriptorType.type = type;
        else if (descriptorType.type != type)
            throw vk::LogicError("Inconsistent buffer types");

        vk::DeviceSize offset = 0;
        vk::DeviceSize size = buffer->size();
        if (ranges.size() > i && ranges[i].second > 0)
        {
            if (ranges[i].first + ranges[i].second > size)
                throw vk::LogicError("Buffer range exceeds the buffer size");

            offset = ranges[i].first;
            size = ranges[i].second;
        }

        descriptorInfos.push_back({{
            *buffer,
            offset,
            size,
        }});

        ++i;
    }

    descriptorType.descriptorCount = descriptorInfos.size();
    return descriptorTypeInfos;
}
#ifndef QMVK_NO_GRAPHICS
MemoryObjectDescr::DescriptorTypeInfos MemoryObjectDescr::getImageDescriptorTypeInfos() const
{
    if (m_access == Access::Storage || m_access == Access::StorageRead || m_access == Access::StorageWrite || (m_sampler && m_access != Access::Read))
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

    vk::SamplerYcbcrConversion samplerYcbcr;
    if (m_sampler)
        samplerYcbcr = m_sampler->samplerYcbcr();
    if (samplerYcbcr && m_plane != ~0u)
        throw vk::LogicError("YCbCr descriptor must not have plane specified");

    for (auto &&objectWeak : m_objects)
    {
        auto object = objectWeak.lock();
        assert(object);

        auto image = static_pointer_cast<Image>(object);

        if (!image->hasImageViews() || samplerYcbcr != image->samplerYcbcr())
            image->recreateImageViews(samplerYcbcr);

        if (samplerYcbcr && !image->samplerYcbcr())
            throw vk::LogicError("Image is not YCbCr");

        const uint32_t n = (m_plane != ~0u)
            ? m_plane + 1
            : samplerYcbcr
              ? 1
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

    if (samplerYcbcr)
        descriptorType.immutableSamplers.resize(descriptorType.descriptorCount, *m_sampler);

    return descriptorTypeInfos;
}
#endif
MemoryObjectDescr::DescriptorTypeInfos MemoryObjectDescr::getBufferViewDescriptorTypeInfos() const
{
    if (m_access == Access::Write)
        throw vk::LogicError("Bad buffer view access");

    DescriptorTypeInfos descriptorTypeInfos;

    auto &descriptorType = descriptorTypeInfos.first;

    auto &descriptorInfos = descriptorTypeInfos.second;
    descriptorInfos.reserve(m_objects.size());

    for (auto &&objectWeak : m_objects)
    {
        auto object = objectWeak.lock();
        assert(object);

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
    auto compareObjects = [](const vector<weak_ptr<MemoryObjectBase>> &a, const vector<weak_ptr<MemoryObjectBase>> &b) {
        const size_t size = a.size();
        if (size != b.size())
            return false;

        for (size_t i = 0; i < size; ++i)
        {
            if (a[i].lock() != b[i].lock())
                return false;
        }

        return true;
    };
    bool ret =
           m_type == other.m_type
        && m_access == other.m_access
        && compareObjects(m_objects, other.m_objects)
#ifndef QMVK_NO_GRAPHICS
        && m_sampler == other.m_sampler && m_plane == other.m_plane
#endif
    ;
    if (ret && m_type == Type::Buffer)
    {
        const auto size = m_descriptorTypeInfos.second.size();
        for (size_t i = 0; i < size; ++i)
        {
            const auto &descriptorInfo = m_descriptorTypeInfos.second[i];
            const auto &descriptorInfoOther = other.m_descriptorTypeInfos.second[i];
            if (descriptorInfo.descrBuffInfo.offset != descriptorInfoOther.descrBuffInfo.offset)
            {
                ret = false;
                break;
            }
            if (descriptorInfo.descrBuffInfo.range != descriptorInfoOther.descrBuffInfo.range)
            {
                ret = false;
                break;
            }
        }
    }
    return ret;
}

}
