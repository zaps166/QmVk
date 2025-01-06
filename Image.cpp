// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2025 Błażej Szczygieł
*/

#include "Image.hpp"
#include "AbstractInstance.hpp"
#include "PhysicalDevice.hpp"
#include "Device.hpp"
#include "MemoryPropertyFlags.hpp"
#include "CommandBuffer.hpp"
#ifdef QMVK_USE_IMAGE_BUFFER_VIEW
#   include "Buffer.hpp"
#   include "BufferView.hpp"
#endif

#include <cmath>

namespace QmVk {

bool Image::checkImageFormat(
    const shared_ptr<PhysicalDevice> &physicalDevice,
    vk::Format fmt,
    bool linear,
    vk::FormatFeatureFlags flags)
{
    const auto &formatProperties = physicalDevice->getFormatPropertiesCached(fmt);
    if (linear)
        return ((formatProperties.linearTilingFeatures & flags) == flags);
    return ((formatProperties.optimalTilingFeatures & flags) == flags);
}

vk::ImageAspectFlagBits Image::getImageAspectFlagBits(uint32_t plane)
{
    switch (plane)
    {
        case 0:
            return vk::ImageAspectFlagBits::ePlane0;
        case 1:
            return vk::ImageAspectFlagBits::ePlane1;
        case 2:
            return vk::ImageAspectFlagBits::ePlane2;
    }
    return vk::ImageAspectFlagBits::eColor;
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
    if (checkImageFormat(physicalDevice, realFmt, linear, vk::FormatFeatureFlagBits::eSampledImage))
        imageFormatInfo.usage |= vk::ImageUsageFlagBits::eSampled;
    imageFormatInfo.pNext = &externalImageFormatInfo;

    // This requires Vulkan 1.1 or extension
    if (physicalDevice->instance()->checkExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
    {
        return physicalDevice->getImageFormatProperties2KHR<
            vk::ImageFormatProperties2,
            vk::ExternalImageFormatProperties
        >(imageFormatInfo, physicalDevice->dld()).get<
            vk::ExternalImageFormatProperties
        >().externalMemoryProperties;
    }
    else
    {
        return physicalDevice->getImageFormatProperties2<
            vk::ImageFormatProperties2,
            vk::ExternalImageFormatProperties
        >(imageFormatInfo, physicalDevice->dld()).get<
            vk::ExternalImageFormatProperties
        >().externalMemoryProperties;
    }
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
        false,
        exportMemoryTypes,
        Priv()
    );
    image->init({}, ~0u, imageCreateInfoCallback);
    return image;
}

shared_ptr<Image> Image::createFromImage(
    const shared_ptr<Device> &device,
    vector<vk::Image> &&vkImages,
    const vk::Extent2D &size,
    vk::Format fmt,
    bool linear,
    uint32_t mipLevels)
{
    auto image = make_shared<Image>(
        device,
        size,
        fmt,
        0,
        linear,
        false,
        false,
        false,
        true,
        vk::ExternalMemoryHandleTypeFlags(),
        Priv()
    );
    if (image->m_numImages != vkImages.size())
        throw vk::LogicError("Number of images doesn't match");
    if (mipLevels > 1)
        image->m_mipLevels = mipLevels;
    image->m_images = move(vkImages);
    image->init({}, ~0u);
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
    bool externalImage,
    vk::ExternalMemoryHandleTypeFlags exportMemoryTypes,
    Priv)
    : MemoryObject(device, exportMemoryTypes)
    , m_wantedSize(size)
    , m_wantedPaddingHeight(paddingHeight)
    , m_mainFormat(fmt)
    , m_linear(linear)
    , m_useMipMaps(useMipmaps)
    , m_storage(storage)
    , m_externalImport(externalImport)
    , m_externalImage(externalImage)
    , m_numPlanes(getNumPlanes(m_mainFormat))
    , m_ycbcr(
        m_numPlanes > 1 &&
        !m_exportMemoryTypes &&
        !m_useMipMaps &&
        !m_storage &&
        !m_externalImport &&
        m_device->hasYcbcr() &&
        checkImageFormat(
            m_physicalDevice,
            m_mainFormat,
            m_linear,
            m_externalImage
              ? vk::FormatFeatureFlagBits::eTransferSrc
              : vk::FormatFeatureFlagBits::eTransferSrc | vk::FormatFeatureFlagBits::eDisjoint
        )
    )
    , m_numImages(m_ycbcr ? 1 : getNumPlanes(m_mainFormat))
{}
Image::~Image()
{
    unmap();
    for (auto &&imageView : m_imageViews)
        m_device->destroyImageView(imageView, nullptr, dld());
    if (!m_externalImage)
    {
        for (auto &&image : m_images)
            m_device->destroyImage(image, nullptr, dld());
    }
}

void Image::init(
    MemoryPropertyPreset memoryPropertyPreset,
    uint32_t heap,
    ImageCreateInfoCallback imageCreateInfoCallback)
{
    if (m_useMipMaps)
    {
        m_mipLevels = getMipLevels(m_wantedSize);
        m_mipLevelsLimit = m_mipLevels;
    }

    m_sizes.resize(m_numPlanes);
    m_paddingHeights.resize(m_numPlanes);
    m_formats.resize(m_numPlanes);
    m_subresourceLayouts.resize(m_numPlanes);
    if (!m_externalImage)
        m_images.resize(m_numImages);
    m_imageViews.resize(m_numPlanes);

    function<vk::Extent2D(const vk::Extent2D &)> getChromaPlaneSizeFn;
    switch (m_mainFormat)
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

    switch (m_mainFormat)
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
            m_formats[0] = m_mainFormat;
            break;
    }

    m_sampled = true;
    for (uint32_t i = 0; i < m_numPlanes; ++i)
    {
        if (!checkImageFormat(m_physicalDevice, m_formats[i], m_linear, vk::FormatFeatureFlagBits::eSampledImage))
        {
            m_sampled = false;
            break;
        }
        if (m_storage && !checkImageFormat(m_physicalDevice, m_formats[i], m_linear, vk::FormatFeatureFlagBits::eStorageImage))
            throw vk::LogicError("Storage image is not supported");
    }

    m_sampledYcbcr = true;
    if (!m_ycbcr || !checkImageFormat(m_physicalDevice, m_mainFormat, m_linear, vk::FormatFeatureFlagBits::eSampledImage))
        m_sampledYcbcr = false;

    vk::ImageUsageFlags imageUsageFlags =
        vk::ImageUsageFlagBits::eTransferSrc
    ;
    if (m_sampled || m_sampledYcbcr)
        imageUsageFlags |= vk::ImageUsageFlagBits::eSampled;

    if (m_externalImage)
    {
        if (m_linear)
            fetchSubresourceLayouts();
        return; // Importing image ends here
    }

    if (!m_externalImport)
        imageUsageFlags |= vk::ImageUsageFlagBits::eTransferDst;
    if (m_storage)
        imageUsageFlags |= vk::ImageUsageFlagBits::eStorage;

    const auto &enabledQueues = m_device->queues();
    for (uint32_t i = 0; i < m_numImages; ++i)
    {
        vk::ImageCreateInfo imageCreateInfo;
        if (m_ycbcr)
            imageCreateInfo.flags |= vk::ImageCreateFlagBits::eMutableFormat | vk::ImageCreateFlagBits::eDisjoint;
        imageCreateInfo.imageType = vk::ImageType::e2D;
        imageCreateInfo.format = m_ycbcr ? m_mainFormat : m_formats[i];
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
        if (enabledQueues.size() > 1)
        {
            imageCreateInfo.sharingMode = vk::SharingMode::eConcurrent;
            imageCreateInfo.queueFamilyIndexCount = enabledQueues.size();
            imageCreateInfo.pQueueFamilyIndices = enabledQueues.data();
        }

        vk::ExternalMemoryImageCreateInfo externalMemoryImageCreateInfo;
        if (m_exportMemoryTypes)
        {
            externalMemoryImageCreateInfo.handleTypes = m_exportMemoryTypes;
            imageCreateInfo.pNext = &externalMemoryImageCreateInfo;
        }

        if (imageCreateInfoCallback)
            imageCreateInfoCallback(i, imageCreateInfo);

        m_images[i] = m_device->createImage(imageCreateInfo, nullptr, dld());
    }

    allocateAndBindMemory(memoryPropertyPreset, heap);
}
void Image::allocateAndBindMemory(MemoryPropertyPreset memoryPropertyPreset, uint32_t heap)
{
    vector<vk::DeviceSize> memoryOffsets(m_numPlanes);

    if (m_linear)
        fetchSubresourceLayouts();

    for (uint32_t i = 0; i < m_numPlanes; ++i)
    {
        vk::DeviceSize paddingBytes = 0;

        memoryOffsets[i] = m_memoryRequirements.size;

        if (m_linear)
            paddingBytes = m_subresourceLayouts[i].rowPitch * m_paddingHeights[i];

        vk::MemoryRequirements2 memoryRequirements2;
        auto &memoryRequirements = memoryRequirements2.memoryRequirements;

        if (m_ycbcr)
        {
            vk::ImagePlaneMemoryRequirementsInfo imagePlaneMemReqInfo;
            imagePlaneMemReqInfo.planeAspect = getImageAspectFlagBits(i);

            vk::ImageMemoryRequirementsInfo2 imageMemoryRequirementsInfo2;
            imageMemoryRequirementsInfo2.image = m_images[0];
            imageMemoryRequirementsInfo2.pNext = &imagePlaneMemReqInfo;

            memoryRequirements2 = m_device->getImageMemoryRequirements2KHR(imageMemoryRequirementsInfo2, dld());
        }
        else
        {
            memoryRequirements = m_device->getImageMemoryRequirements(m_images[i], dld());
        }
        const auto memoryRequirementsSize = aligned(memoryRequirements.size + paddingBytes, memoryRequirements.alignment);

        m_memoryRequirements.size += memoryRequirementsSize;
        m_memoryRequirements.alignment = max(m_memoryRequirements.alignment, memoryRequirements.alignment);
        m_memoryRequirements.memoryTypeBits |= memoryRequirements.memoryTypeBits;

        m_subresourceLayouts[i].offset = memoryOffsets[i];
        if (!m_linear)
            m_subresourceLayouts[i].size = memoryRequirementsSize;
    }

    if (m_externalImport)
        return; // Importing external handler ends here

#ifdef QMVK_USE_IMAGE_BUFFER_VIEW
    if (m_linear)
    {
        vk::BufferCreateInfo bufferCreateInfo;
        bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformTexelBuffer | vk::BufferUsageFlagBits::eStorageTexelBuffer;
        bufferCreateInfo.size = m_memoryRequirements.size;
        m_uniqueBuffer = m_device->createBufferUnique(bufferCreateInfo, nullptr, dld());

        m_memoryRequirements.alignment = max(
            m_memoryRequirements.alignment,
            m_device->getBufferMemoryRequirements(*m_uniqueBuffer, dld()).alignment
        );
        m_memoryRequirements.size = aligned(m_memoryRequirements.size, m_memoryRequirements.alignment);
    }
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

    if (m_ycbcr)
    {
        vector<vk::BindImagePlaneMemoryInfo> bindImagePlaneMemInfos(m_numPlanes);
        vector<vk::BindImageMemoryInfo> bindImageMemInfos(m_numPlanes);
        for (uint32_t i = 0; i < m_numPlanes; ++i)
        {
            bindImagePlaneMemInfos[i].planeAspect = getImageAspectFlagBits(i);

            bindImageMemInfos[i].image = m_images[0];
            bindImageMemInfos[i].memory = deviceMemory();
            bindImageMemInfos[i].memoryOffset = memoryOffsets[i];
            bindImageMemInfos[i].pNext = &bindImagePlaneMemInfos[i];
        }
        m_device->bindImageMemory2KHR(bindImageMemInfos, dld());
    }
    else for (uint32_t i = 0; i < m_numImages; ++i)
    {
        m_device->bindImageMemory(m_images[i], deviceMemory(), memoryOffsets[i], dld());
    }
}

void Image::finishImport(const vector<vk::DeviceSize> &offsets, vk::DeviceSize globalOffset)
{
    for (uint32_t i = 0; i < m_numImages; ++i)
    {
        m_device->bindImageMemory(
            m_images[i],
            deviceMemory(min<uint32_t>(i, deviceMemoryCount() - 1)),
            offsets[i] + globalOffset,
            dld()
        );
    }
}

void Image::recreateImageViews(vk::SamplerYcbcrConversion samperYcbcr)
{
    for (auto &&imageView : m_imageViews)
    {
        m_device->destroyImageView(imageView, nullptr, dld());
        imageView = nullptr;
    }

    m_hasImageViews = false;
    m_samperYcbcr = nullptr;

    if (samperYcbcr && m_ycbcr)
    {
        if (!m_storage && !m_sampledYcbcr)
            return;

        vk::SamplerYcbcrConversionInfo samplerYcbcrInfo(samperYcbcr);

        vk::ImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo.image = m_images[0];
        imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
        imageViewCreateInfo.format = m_mainFormat;
        imageViewCreateInfo.subresourceRange = getImageSubresourceRange();
        imageViewCreateInfo.pNext = &samplerYcbcrInfo;
        m_imageViews[0] = m_device->createImageView(imageViewCreateInfo, nullptr, dld());

        m_samperYcbcr = samperYcbcr;
    }
    else
    {
        if (!m_storage && !m_sampled)
            return;

        for (uint32_t i = 0; i < m_numPlanes; ++i)
        {
            vk::ImageViewCreateInfo imageViewCreateInfo;
            imageViewCreateInfo.image = m_images[m_ycbcr ? 0 : i];
            imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
            imageViewCreateInfo.format = m_formats[i];
            imageViewCreateInfo.subresourceRange = getImageSubresourceRange(~0u, m_ycbcr ? i : ~0u);
            m_imageViews[i] = m_device->createImageView(imageViewCreateInfo, nullptr, dld());
        }
    }

    m_hasImageViews = true;
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
    if (!m_externalImport)
        throw vk::LogicError("Importing FD requires external import");

    if (m_numImages != offsets.size())
        throw vk::LogicError("Offsets count and images count missmatch");

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
    if (m_numImages != offsets.size())
        throw vk::LogicError("Offsets count and images count missmatch");

    vector<vk::DeviceSize> imageSizes;
    imageSizes.resize(rawHandles.size());
    for (uint32_t i = 0; i < m_numImages; ++i)
    {
        auto size = m_device->getImageMemoryRequirements(m_images[i], dld()).size;
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
    {
        if (m_externalImport || m_externalImage)
            throw vk::LogicError("Can't map externally imported memory or image");

        m_mapped = m_device->mapMemory(deviceMemory(), 0, memorySize(), {}, dld());
    }

    if (plane == ~0u)
        return m_mapped;

    return reinterpret_cast<uint8_t *>(m_mapped) + planeOffset(plane);
}
void Image::unmap()
{
    if (!m_mapped)
        return;

    m_device->unmapMemory(deviceMemory(), dld());
    m_mapped = nullptr;
}

void Image::copyTo(
    const shared_ptr<Image> &dstImage,
    const shared_ptr<CommandBuffer> &externalCommandBuffer)
{
    if (dstImage->m_externalImport || dstImage->m_externalImage)
        throw vk::LogicError("Can't copy to externally imported memory or image");

    if (m_numPlanes != dstImage->m_numPlanes)
        throw vk::LogicError("Source image and destination image planes count missmatch");

    if (m_mainFormat != dstImage->m_mainFormat)
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
            region.srcSubresource.aspectMask = getImageAspectFlagBits(m_ycbcr ? i : ~0u);
            region.srcSubresource.layerCount = 1;
            region.dstSubresource.aspectMask = getImageAspectFlagBits(dstImage->m_ycbcr ? i : ~0u);
            region.dstSubresource.layerCount = 1;
            region.extent = vk::Extent3D(
                min(m_sizes[i].width,  dstImage->m_sizes[i].width),
                min(m_sizes[i].height, dstImage->m_sizes[i].height),
                1
            );

            commandBuffer.copyImage(
                m_images[m_ycbcr ? 0 : i],
                m_imageLayout,
                dstImage->m_images[dstImage->m_ycbcr ? 0 : i],
                dstImage->m_imageLayout,
                region,
                dld()
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

void Image::fetchSubresourceLayouts()
{
    for (uint32_t i = 0; i < m_numPlanes; ++i)
    {
        m_subresourceLayouts[i] = m_device->getImageSubresourceLayout(
            m_images[m_ycbcr ? 0 : i],
            vk::ImageSubresource(getImageAspectFlagBits(m_ycbcr ? i : ~0u)),
            dld()
        );
    }
}

bool Image::maybeGenerateMipmaps(vk::CommandBuffer commandBuffer)
{
    if (!m_useMipMaps || m_mipLevels <= 1)
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

        for (uint32_t i = 0; i < m_numImages; ++i)
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
                m_images[i],
                vk::ImageLayout::eTransferSrcOptimal,
                m_images[i],
                vk::ImageLayout::eTransferDstOptimal,
                1,
                &blit,
                vk::Filter::eLinear,
                dld()
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
    const auto size = (m_numImages == 1)
        ? max(inSize.width, inSize.height)
        : max((inSize.width + 1) / 2, (inSize.height + 1) / 2)
    ;
    return static_cast<uint32_t>(log2(size)) + 1;
}

vk::ImageSubresourceRange Image::getImageSubresourceRange(uint32_t mipLevels, uint32_t plane) const
{
    vk::ImageSubresourceRange imageSubresourceRange;
    imageSubresourceRange.aspectMask = getImageAspectFlagBits(plane);
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
            image,
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
            &barrier,
            dld()
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
