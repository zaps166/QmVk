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

#include "Image.hpp"
#include "PhysicalDevice.hpp"
#include "Device.hpp"
#include "MemoryPropertyFlags.hpp"
#include "DescriptorInfo.hpp"
#include "CommandBuffer.hpp"
#include "Buffer.hpp"
#ifdef QMVK_USE_IMAGE_BUFFER_VIEW
#   include "BufferView.hpp"
#endif

#include <functional>
#include <cmath>

namespace QmVk {

bool Image::checkFormatSampledImage(
    const shared_ptr<PhysicalDevice> &physicalDevice,
    vk::Format fmt,
    bool linear)
{
    const auto &formatProperties = physicalDevice->getFormatPropertiesCached(fmt);
    const auto flags = vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eSampledImageFilterLinear;
    if (linear)
        return static_cast<bool>(formatProperties.linearTilingFeatures & flags);
    return static_cast<bool>(formatProperties.optimalTilingFeatures & flags);
}

uint32_t Image::getNumPlanes(vk::Format format)
{
    switch (format)
    {
        case vk::Format::eG8B8R82Plane420Unorm:
        case vk::Format::eG8B8R82Plane422Unorm:
        case vk::Format::eG16B16R162Plane420Unorm:
        case vk::Format::eG16B16R162Plane422Unorm:
            return 2;
        case vk::Format::eG8B8R83Plane420Unorm:
        case vk::Format::eG8B8R83Plane422Unorm:
        case vk::Format::eG8B8R83Plane444Unorm:
        case vk::Format::eG16B16R163Plane420Unorm:
        case vk::Format::eG16B16R163Plane422Unorm:
        case vk::Format::eG16B16R163Plane444Unorm:
            return 3;
        default:
            break;
    }
    return 1;
}

vk::ExternalMemoryProperties Image::getExternalMemoryProperties(
    const shared_ptr<PhysicalDevice> &physicalDevice,
    vk::ExternalMemoryHandleTypeFlagBits externalMemoryType,
    vk::Format realFmt,
    bool linear)
{
    vk::PhysicalDeviceExternalImageFormatInfo externalImageFormatInfo;
    externalImageFormatInfo.handleType = externalMemoryType;

    vk::PhysicalDeviceImageFormatInfo2 imageFormatInfo;
    imageFormatInfo.type = vk::ImageType::e2D;
    imageFormatInfo.format = realFmt;
    imageFormatInfo.tiling = linear
        ? vk::ImageTiling::eLinear
        : vk::ImageTiling::eOptimal
    ;
    imageFormatInfo.usage =
        vk::ImageUsageFlagBits::eTransferSrc
    ;
    if (checkFormatSampledImage(physicalDevice, realFmt, linear))
        imageFormatInfo.usage |= vk::ImageUsageFlagBits::eSampled;
    imageFormatInfo.pNext = &externalImageFormatInfo;

    return physicalDevice->getImageFormatProperties2KHR<
        vk::ImageFormatProperties2,
        vk::ExternalImageFormatProperties
    >(imageFormatInfo).get<
        vk::ExternalImageFormatProperties
    >().externalMemoryProperties;
}

shared_ptr<Image> Image::createOptimal(
    const shared_ptr<Device> &device,
    const vk::Extent2D &size,
    vk::Format fmt,
    bool useMipMaps,
    bool storage,
    vk::ExternalMemoryHandleTypeFlags exportMemoryTypes,
    uint32_t heap)
{
    auto image = make_shared<Image>(
        device,
        size,
        fmt,
        0,
        false,
        useMipMaps,
        storage,
        false,
        exportMemoryTypes,
        Priv()
    );
    image->init(MemoryPropertyPreset::PreferNoHostAccess, heap);
    return image;
}
shared_ptr<Image> Image::createLinear(
    const shared_ptr<Device> &device,
    const vk::Extent2D &size,
    vk::Format fmt,
    MemoryPropertyPreset memoryPropertyPreset,
    uint32_t paddingHeight,
    bool useMipMaps,
    bool storage,
    vk::ExternalMemoryHandleTypeFlags exportMemoryTypes,
    uint32_t heap)
{
    auto image = make_shared<Image>(
        device,
        size,
        fmt,
        paddingHeight,
        true,
        useMipMaps,
        storage,
        false,
        exportMemoryTypes,
        Priv()
    );
    image->init(memoryPropertyPreset, heap);
    return image;
}

shared_ptr<Image> Image::createExternalImport(
    const shared_ptr<Device> &device,
    const vk::Extent2D &size,
    vk::Format fmt,
    bool linear,
    vk::ExternalMemoryHandleTypeFlags exportMemoryTypes,
    ImageCreateInfoCallback imageCreateInfoCallback)
{
    auto image = make_shared<Image>(
        device,
        size,
        fmt,
        0,
        linear,
        false,
        false,
        true,
        exportMemoryTypes,
        Priv()
    );
    image->init(MemoryPropertyPreset::PreferNoHostAccess, ~0u, imageCreateInfoCallback);
    return image;
}

Image::Image(
    const shared_ptr<Device> &device,
    const vk::Extent2D &size,
    vk::Format fmt,
    uint32_t paddingHeight,
    bool linear,
    bool useMipmaps,
    bool storage,
    bool externalImport,
    vk::ExternalMemoryHandleTypeFlags exportMemoryTypes,
    Priv)
    : MemoryObject(device, exportMemoryTypes)
    , m_wantedSize(size)
    , m_wantedPaddingHeight(paddingHeight)
    , m_wantedFormat(fmt)
    , m_linear(linear)
    , m_useMipMaps(useMipmaps)
    , m_storage(storage)
    , m_externalImport(externalImport)
    , m_numPlanes(getNumPlanes(m_wantedFormat))
{}
Image::~Image()
{
    unmap();
}

void Image::init(
    MemoryPropertyPreset memoryPropertyPreset,
    uint32_t heap,
    ImageCreateInfoCallback imageCreateInfoCallback)
{
    if (!m_externalImport && m_useMipMaps)
    {
        m_mipLevels = getMipLevels(m_wantedSize);
        m_mipLevelsLimit = m_mipLevels;
    }

    m_sizes.resize(m_numPlanes);
    m_paddingHeights.resize(m_numPlanes);
    m_formats.resize(m_numPlanes);
    m_subresourceLayouts.resize(m_numPlanes);
    m_images.resize(m_numPlanes);
    m_imageViews.resize(m_numPlanes);

    function<vk::Extent2D(const vk::Extent2D &)> getChromaPlaneSizeFn;
    switch (m_wantedFormat)
    {
        case vk::Format::eG8B8R82Plane420Unorm:
        case vk::Format::eG16B16R162Plane420Unorm:
        case vk::Format::eG8B8R83Plane420Unorm:
        case vk::Format::eG16B16R163Plane420Unorm:
            getChromaPlaneSizeFn = [](const vk::Extent2D &size) {
                return vk::Extent2D((size.width + 1) / 2, (size.height + 1) / 2);
            };
            break;
        case vk::Format::eG8B8R82Plane422Unorm:
        case vk::Format::eG16B16R162Plane422Unorm:
        case vk::Format::eG8B8R83Plane422Unorm:
        case vk::Format::eG16B16R163Plane422Unorm:
            getChromaPlaneSizeFn = [](const vk::Extent2D &size) {
                return vk::Extent2D((size.width + 1) / 2, size.height);
            };
            break;
        case vk::Format::eG8B8R83Plane444Unorm:
        case vk::Format::eG16B16R163Plane444Unorm:
            getChromaPlaneSizeFn = [](const vk::Extent2D &size) {
                return size;
            };
            break;
        default:
            break;
    }
    m_sizes[0] = m_wantedSize;
    m_paddingHeights[0] = m_wantedPaddingHeight;
    if (getChromaPlaneSizeFn)
    {
        for (uint32_t i = 1; i < m_numPlanes; ++i)
        {
            m_sizes[i] = getChromaPlaneSizeFn(m_wantedSize);
            if (m_wantedPaddingHeight > 0)
                m_paddingHeights[i] = getChromaPlaneSizeFn(vk::Extent2D(0, m_wantedPaddingHeight)).height;
        }
    }

    switch (m_wantedFormat)
    {
        case vk::Format::eG8B8R83Plane420Unorm:
        case vk::Format::eG8B8R83Plane422Unorm:
        case vk::Format::eG8B8R83Plane444Unorm:
            m_formats[0] = vk::Format::eR8Unorm;
            m_formats[1] = vk::Format::eR8Unorm;
            m_formats[2] = vk::Format::eR8Unorm;
            break;
        case vk::Format::eG8B8R82Plane420Unorm:
        case vk::Format::eG8B8R82Plane422Unorm:
            m_formats[0] = vk::Format::eR8Unorm;
            m_formats[1] = vk::Format::eR8G8Unorm;
            break;
        case vk::Format::eG16B16R163Plane420Unorm:
        case vk::Format::eG16B16R163Plane422Unorm:
        case vk::Format::eG16B16R163Plane444Unorm:
            m_formats[0] = vk::Format::eR16Unorm;
            m_formats[1] = vk::Format::eR16Unorm;
            m_formats[2] = vk::Format::eR16Unorm;
            break;
        case vk::Format::eG16B16R162Plane420Unorm:
        case vk::Format::eG16B16R162Plane422Unorm:
            m_formats[0] = vk::Format::eR16Unorm;
            m_formats[1] = vk::Format::eR16G16Unorm;
            break;
        default:
            m_formats[0] = m_wantedFormat;
            break;
    }

    auto checkSampledImage = [this] {
        for (uint32_t i = 0; i < m_numPlanes; ++i)
        {
            if (!checkFormatSampledImage(m_physicalDevice, m_formats[i], m_linear))
                return false;
        }
        return true;
    };

    vk::ImageUsageFlags imageUsageFlags =
        vk::ImageUsageFlagBits::eTransferSrc
    ;
    if (checkSampledImage())
    {
        imageUsageFlags |= vk::ImageUsageFlagBits::eSampled;
        m_sampled = true;
    }
    if (!m_externalImport)
        imageUsageFlags |= vk::ImageUsageFlagBits::eTransferDst;
    if (m_storage)
        imageUsageFlags |= vk::ImageUsageFlagBits::eStorage;

    for (uint32_t i = 0; i < m_numPlanes; ++i)
    {
        vk::ImageCreateInfo imageCreateInfo;
        imageCreateInfo.imageType = vk::ImageType::e2D;
        imageCreateInfo.format = m_formats[i];
        imageCreateInfo.extent = vk::Extent3D(m_sizes[i], 1);
        imageCreateInfo.mipLevels = m_mipLevels;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
        imageCreateInfo.tiling = m_linear
            ? vk::ImageTiling::eLinear
            : vk::ImageTiling::eOptimal
        ;
        imageCreateInfo.usage = imageUsageFlags;
        imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

        vk::ExternalMemoryImageCreateInfo externalMemoryImageCreateInfo;
        if (m_exportMemoryTypes)
        {
            externalMemoryImageCreateInfo.handleTypes = m_exportMemoryTypes;
            imageCreateInfo.pNext = &externalMemoryImageCreateInfo;
        }

        if (imageCreateInfoCallback)
            imageCreateInfoCallback(i, imageCreateInfo);

        m_images[i] = m_device->createImageUnique(imageCreateInfo);
    }

    allocateAndBindMemory(memoryPropertyPreset, heap);
}
void Image::allocateAndBindMemory(MemoryPropertyPreset memoryPropertyPreset, uint32_t heap)
{
    vector<vk::DeviceSize> memoryOffsets(m_numPlanes);

    for (uint32_t i = 0; i < m_numPlanes; ++i)
    {
        vk::DeviceSize paddingBytes = 0;

        memoryOffsets[i] = m_memoryRequirements.size;

        if (m_linear)
        {
            m_subresourceLayouts[i] = m_device->getImageSubresourceLayout(
                *m_images[i],
                vk::ImageSubresource(vk::ImageAspectFlagBits::eColor)
            );
            paddingBytes = m_subresourceLayouts[i].rowPitch * m_paddingHeights[i];
        }

        const auto memoryRequirements = m_device->getImageMemoryRequirements(*m_images[i]);
        const auto memoryRequirementsSize = aligned(memoryRequirements.size + paddingBytes, memoryRequirements.alignment);

        m_memoryRequirements.size += memoryRequirementsSize;
        m_memoryRequirements.alignment = max(m_memoryRequirements.alignment, memoryRequirements.alignment);
        m_memoryRequirements.memoryTypeBits |= memoryRequirements.memoryTypeBits;

        m_subresourceLayouts[i].offset = memoryOffsets[i];
        if (!m_linear)
            m_subresourceLayouts[i].size = memoryRequirementsSize;
    }

    if (m_externalImport)
        return;

#ifdef QMVK_USE_IMAGE_BUFFER_VIEW
        vk::BufferCreateInfo bufferCreateInfo;
        bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformTexelBuffer | vk::BufferUsageFlagBits::eStorageTexelBuffer;
        bufferCreateInfo.size = m_memoryRequirements.size;
        m_uniqueBuffer = m_device->createBufferUnique(bufferCreateInfo);

        m_memoryRequirements.alignment = max(
            m_memoryRequirements.alignment,
            m_device->getBufferMemoryRequirements(*m_uniqueBuffer).alignment
        );
        m_memoryRequirements.size = aligned(m_memoryRequirements.size, m_memoryRequirements.alignment);
#endif

    MemoryPropertyFlags memoryPropertyFlags;
    switch (memoryPropertyPreset)
    {
        case MemoryPropertyPreset::PreferNoHostAccess:
            memoryPropertyFlags.required =
                vk::MemoryPropertyFlagBits::eDeviceLocal
            ;
            memoryPropertyFlags.notWanted =
                vk::MemoryPropertyFlagBits::eHostVisible
            ;
            break;
        case MemoryPropertyPreset::PreferCachedOrNoHostAccess:
            memoryPropertyFlags.required =
                vk::MemoryPropertyFlagBits::eDeviceLocal
            ;
            memoryPropertyFlags.optional =
                vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent |
                vk::MemoryPropertyFlagBits::eHostCached
            ;
            break;
        case MemoryPropertyPreset::PreferHostAccess:
            memoryPropertyFlags.required =
                vk::MemoryPropertyFlagBits::eDeviceLocal
            ;
            memoryPropertyFlags.optional =
                vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent |
                vk::MemoryPropertyFlagBits::eHostCached
            ;
            memoryPropertyFlags.optionalFallback =
                vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent
            ;
            break;
        case MemoryPropertyPreset::PreferCachedHostOnly:
            memoryPropertyFlags.required =
                vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent
            ;
            memoryPropertyFlags.optional =
                vk::MemoryPropertyFlagBits::eHostCached
            ;
            if (m_physicalDevice->hasFullHostVisibleDeviceLocal())
            {
                memoryPropertyFlags.optionalFallback =
                    vk::MemoryPropertyFlagBits::eDeviceLocal
                ;
            }
            break;
        case MemoryPropertyPreset::PreferHostOnly:
            memoryPropertyFlags.required =
                vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent
            ;
            memoryPropertyFlags.optional =
                vk::MemoryPropertyFlagBits::eDeviceLocal
            ;
            memoryPropertyFlags.optionalFallback =
                vk::MemoryPropertyFlagBits::eHostCached
            ;
            break;
    }
    memoryPropertyFlags.heap = heap;
    allocateMemory(memoryPropertyFlags);

    for (uint32_t i = 0; i < m_numPlanes; ++i)
        m_device->bindImageMemory(*m_images[i], deviceMemory(), memoryOffsets[i]);
    createImageViews();
}

void Image::finishImport(const vector<vk::DeviceSize> &offsets, vk::DeviceSize globalOffset)
{
    for (uint32_t i = 0; i < m_numPlanes; ++i)
    {
        m_device->bindImageMemory(
            *m_images[i],
            deviceMemory(min<uint32_t>(i, deviceMemoryCount() - 1)),
            offsets[i] + globalOffset
        );
    }
    createImageViews();
}

void Image::createImageViews()
{
    if (!m_storage && !m_sampled)
        return;

    for (uint32_t i = 0; i < m_numPlanes; ++i)
    {
        vk::ImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo.image = *m_images[i];
        imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
        imageViewCreateInfo.format = m_formats[i];
        imageViewCreateInfo.subresourceRange = getImageSubresourceRange();
        m_imageViews[i] = m_device->createImageViewUnique(imageViewCreateInfo);
    }
}

#ifdef QMVK_USE_IMAGE_BUFFER_VIEW
shared_ptr<BufferView> Image::bufferView(uint32_t plane)
{
    if (m_bufferViews.empty())
    {
        if (!m_uniqueBuffer)
            return nullptr;

        auto buffer = Buffer::createFromDeviceMemory(
            m_device,
            memorySize(),
            vk::BufferUsageFlagBits::eUniformTexelBuffer | vk::BufferUsageFlagBits::eStorageTexelBuffer,
            deviceMemory(),
            m_memoryPropertyFlags,
            &m_uniqueBuffer
        );

        m_bufferViews.reserve(m_numPlanes);
        for (uint32_t i = 0; i < m_numPlanes; ++i)
        {
            m_bufferViews.push_back(BufferView::create(
                buffer,
                format(i),
                planeOffset(i),
                memorySize(i)
            ));
        }
    }
    return m_bufferViews[plane];
}
#endif

void Image::importFD(
    const FdDescriptors &descriptors,
    const vector<vk::DeviceSize> &offsets,
    vk::ExternalMemoryHandleTypeFlagBits handleType)
{
    if (m_numPlanes != offsets.size())
        throw vk::LogicError("Offsets count and planes count missmatch");

    MemoryObject::importFD(descriptors, handleType);

    finishImport(offsets);
}

#ifdef VK_USE_PLATFORM_WIN32_KHR
void Image::importWin32Handle(
    const vector<HANDLE> &rawHandles,
    const vector<vk::DeviceSize> &offsets,
    vk::ExternalMemoryHandleTypeFlagBits handleType,
    vk::DeviceSize globalOffset)
{
    if (m_numPlanes != offsets.size())
        throw vk::LogicError("Offsets count and planes count missmatch");

    vector<vk::DeviceSize> imageSizes;
    imageSizes.resize(rawHandles.size());
    for (uint32_t i = 0; i < m_numPlanes; ++i)
    {
        auto size = m_device->getImageMemoryRequirements(*m_images[i]).size;
        if (i < imageSizes.size())
            imageSizes[i] = size + globalOffset;
        else
            imageSizes.back() += size;
    }

    Win32Handles handles;
    handles.reserve(rawHandles.size());
    for (size_t i = 0; i < rawHandles.size(); ++i)
    {
        handles.emplace_back(
            rawHandles[i],
            imageSizes[i]
        );
    }
    MemoryObject::importWin32Handle(handles, handleType);

    finishImport(offsets, globalOffset);
}
#endif

bool Image::setMipLevelsLimitForSize(const vk::Extent2D &size)
{
    const uint32_t mipLevels = getMipLevels(size);
    m_mipLevelsLimit = (mipLevels - 1 < m_mipLevels)
        ? min(m_mipLevels - mipLevels + 2, m_mipLevels)
        : 1
    ;
    return (m_mipLevelsLimit > m_mipLevelsGenerated);
}

void *Image::map(uint32_t plane)
{
    if (!m_mapped)
        m_mapped = m_device->mapMemory(deviceMemory(), 0, memorySize());

    if (plane == ~0u)
        return m_mapped;

    return reinterpret_cast<uint8_t *>(m_mapped) + planeOffset(plane);
}
void Image::unmap()
{
    if (!m_mapped)
        return;

    m_device->unmapMemory(deviceMemory());
    m_mapped = nullptr;
}

void Image::copyTo(
    const shared_ptr<Image> &dstImage,
    const shared_ptr<CommandBuffer> &externalCommandBuffer)
{
    if (dstImage->m_externalImport)
        throw vk::LogicError("Can't copy to externally imported memory");

    if (m_numPlanes != dstImage->m_numPlanes)
        throw vk::LogicError("Source image and destination image planes count missmatch");

    if (m_formats != dstImage->m_formats)
        throw vk::LogicError("Source image and destination image format missmatch");

    auto copyCommands = [&](vk::CommandBuffer commandBuffer) {
        pipelineBarrier(
            commandBuffer,
            vk::ImageLayout::eTransferSrcOptimal,
            vk::PipelineStageFlagBits::eTransfer,
            vk::AccessFlagBits::eTransferRead
        );
        dstImage->pipelineBarrier(
            commandBuffer,
            vk::ImageLayout::eTransferDstOptimal,
            vk::PipelineStageFlagBits::eTransfer,
            vk::AccessFlagBits::eTransferWrite
        );

        for (uint32_t i = 0; i < m_numPlanes; ++i)
        {
            vk::ImageCopy region;
            region.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            region.srcSubresource.layerCount = 1;
            region.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            region.dstSubresource.layerCount = 1;
            region.extent = vk::Extent3D(
                min(m_sizes[i].width,  dstImage->m_sizes[i].width),
                min(m_sizes[i].height, dstImage->m_sizes[i].height),
                1
            );

            commandBuffer.copyImage(
                *m_images[i],
                m_imageLayout,
                *dstImage->m_images[i],
                dstImage->m_imageLayout,
                region
            );
        }

        dstImage->maybeGenerateMipmaps(commandBuffer);
    };

    if (externalCommandBuffer)
    {
        externalCommandBuffer->storeData(shared_from_this());
        externalCommandBuffer->storeData(dstImage);
        copyCommands(*externalCommandBuffer);
    }
    else
    {
        internalCommandBuffer()->execute(copyCommands);
    }
}

void Image::maybeGenerateMipmaps(const shared_ptr<CommandBuffer> &commandBuffer)
{
    if (maybeGenerateMipmaps(*commandBuffer))
        commandBuffer->storeData(shared_from_this());
}

bool Image::maybeGenerateMipmaps(vk::CommandBuffer commandBuffer)
{
    if (m_mipLevels <= 1)
        return false;

    vk::ImageSubresourceRange imageSubresourceRange = getImageSubresourceRange(1);

    auto mipSizes = m_sizes;

    vk::ImageLayout imageLayout = m_imageLayout;
    vk::PipelineStageFlags stage = m_stage;
    vk::AccessFlags accessFlags = m_accessFlags;

    m_mipLevelsGenerated = 1;

    for (uint32_t l = 1; l < m_mipLevels; ++l)
    {
        imageSubresourceRange.baseMipLevel = l - 1;
        pipelineBarrier(
            commandBuffer,
            imageLayout,
            vk::ImageLayout::eTransferSrcOptimal,
            stage,
            vk::PipelineStageFlagBits::eTransfer,
            accessFlags,
            vk::AccessFlagBits::eTransferRead,
            imageSubresourceRange,
            false
        );

        imageSubresourceRange.baseMipLevel = l;
        pipelineBarrier(
            commandBuffer,
            m_imageLayout,
            vk::ImageLayout::eTransferDstOptimal,
            m_stage,
            vk::PipelineStageFlagBits::eTransfer,
            m_accessFlags,
            vk::AccessFlagBits::eTransferWrite,
            imageSubresourceRange,
            false
        );

        imageLayout = vk::ImageLayout::eTransferDstOptimal;
        stage = vk::PipelineStageFlagBits::eTransfer;
        accessFlags = vk::AccessFlagBits::eTransferWrite;

        if (l >= m_mipLevelsLimit)
            continue;

        for (uint32_t i = 0; i < m_numPlanes; ++i)
        {
            auto &mipWidth = reinterpret_cast<int32_t &>(mipSizes[i].width);
            auto &mipHeight = reinterpret_cast<int32_t &>(mipSizes[i].height);

            const auto mipPrevWidth = mipWidth;
            const auto mipPrevHeight = mipHeight;

            if (mipWidth > 1)
                mipWidth /= 2;
            if (mipHeight > 1)
                mipHeight /= 2;

            vk::ImageBlit blit;
            blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.srcSubresource.mipLevel = l - 1;
            blit.srcSubresource.layerCount = 1;
            blit.srcOffsets[1] = vk::Offset3D(mipPrevWidth, mipPrevHeight, 1);
            blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.dstSubresource.mipLevel = l;
            blit.dstSubresource.layerCount = 1;
            blit.dstOffsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);

            commandBuffer.blitImage(
                *m_images[i],
                vk::ImageLayout::eTransferSrcOptimal,
                *m_images[i],
                vk::ImageLayout::eTransferDstOptimal,
                1,
                &blit,
                vk::Filter::eLinear
            );
        }

        ++m_mipLevelsGenerated;
    }

    imageSubresourceRange.baseMipLevel = m_mipLevels - 1;
    pipelineBarrier(
        commandBuffer,
        imageLayout,
        vk::ImageLayout::eTransferSrcOptimal,
        stage,
        vk::PipelineStageFlagBits::eTransfer,
        accessFlags,
        vk::AccessFlagBits::eTransferRead,
        imageSubresourceRange,
        true
    );

    return true;
}

uint32_t Image::getMipLevels(const vk::Extent2D &inSize) const
{
    const auto size = (m_numPlanes == 1)
        ? max(inSize.width, inSize.height)
        : max((inSize.width + 1) / 2, (inSize.height + 1) / 2)
    ;
    return static_cast<uint32_t>(log2(size)) + 1;
}

vk::ImageSubresourceRange Image::getImageSubresourceRange(uint32_t mipLevels) const
{
    vk::ImageSubresourceRange imageSubresourceRange;
    imageSubresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageSubresourceRange.levelCount = (mipLevels == ~0u) ? m_mipLevels : mipLevels;
    imageSubresourceRange.layerCount = 1;
    return imageSubresourceRange;
}

inline bool Image::mustExecPipelineBarrier(
    vk::ImageLayout newLayout,
    vk::PipelineStageFlags dstStage,
    vk::AccessFlags dstAccessFlags)
{
    return (m_imageLayout != newLayout || m_stage != dstStage || m_accessFlags != dstAccessFlags);
}

void Image::pipelineBarrier(
    vk::CommandBuffer commandBuffer,
    vk::ImageLayout dstImageLayout,
    vk::PipelineStageFlags dstStage,
    vk::AccessFlags dstAccessFlags)
{
    pipelineBarrier(
        commandBuffer,
        m_imageLayout,
        dstImageLayout,
        m_stage,
        dstStage,
        m_accessFlags,
        dstAccessFlags,
        getImageSubresourceRange(),
        true
    );
}
void Image::pipelineBarrier(
    vk::CommandBuffer commandBuffer,
    vk::ImageLayout srcImageLayout,
    vk::ImageLayout dstImageLayout,
    vk::PipelineStageFlags srcStage,
    vk::PipelineStageFlags dstStage,
    vk::AccessFlags srcAccessFlags,
    vk::AccessFlags dstAccessFlags,
    const vk::ImageSubresourceRange &imageSubresourceRange,
    bool updateVariables)
{
    if (!mustExecPipelineBarrier(dstImageLayout, dstStage, dstAccessFlags))
        return;

    for (auto &&image : m_images)
    {
        vk::ImageMemoryBarrier barrier(
            srcAccessFlags,
            dstAccessFlags,
            srcImageLayout,
            dstImageLayout,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            *image,
            imageSubresourceRange
        );
        commandBuffer.pipelineBarrier(
            srcStage,
            dstStage,
            vk::DependencyFlags(),
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier
        );
    }

    if (updateVariables)
    {
        m_imageLayout = dstImageLayout;
        m_stage = dstStage;
        m_accessFlags = dstAccessFlags;
    }
}

}
