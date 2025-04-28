// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2025 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include "MemoryObject.hpp"

#include <functional>

namespace QmVk {

using namespace std;

#ifdef QMVK_USE_IMAGE_BUFFER_VIEW
class BufferView;
#endif

class QMVK_EXPORT Image : public MemoryObject, public enable_shared_from_this<Image>
{
    friend class MemoryObjectDescr;

public:
    enum class MemoryPropertyPreset
    {
        PreferNoHostAccess,
        PreferCachedOrNoHostAccess,
        PreferHostAccess,
        PreferCachedHostOnly,
        PreferHostOnly,
    };

    using ImageCreateInfoCallback = function<void(uint32_t plane, vk::ImageCreateInfo &imageCreateInfo)>;

public:
    static bool checkImageFormat(
        const shared_ptr<PhysicalDevice> &physicalDevice,
        vk::Format fmt,
        bool linear,
        vk::FormatFeatureFlags flags
    );

    static vk::ImageAspectFlagBits getImageAspectFlagBits(uint32_t plane);

    static uint32_t getNumPlanes(vk::Format format);

    static vk::ExternalMemoryProperties getExternalMemoryProperties(
        const shared_ptr<PhysicalDevice> &physicalDevice,
        vk::ExternalMemoryHandleTypeFlagBits externalMemoryType,
        vk::Format realFmt,
        bool linear
    );

public:
    static shared_ptr<Image> createOptimal(
        const shared_ptr<Device> &device,
        const vk::Extent2D &size,
        vk::Format fmt,
        bool useMipMaps = false,
        bool storage = false,
        vk::ExternalMemoryHandleTypeFlags exportMemoryTypes = {},
        uint32_t heap = ~0u
    );
    static shared_ptr<Image> createLinear(
        const shared_ptr<Device> &device,
        const vk::Extent2D &size,
        vk::Format fmt,
        MemoryPropertyPreset memoryPropertyPreset = MemoryPropertyPreset::PreferCachedHostOnly,
        uint32_t paddingHeight = 0,
        bool useMipMaps = false,
        bool storage = false,
        vk::ExternalMemoryHandleTypeFlags exportMemoryTypes = {},
        uint32_t heap = ~0u
    );

    static shared_ptr<Image> createExternalImport(
        const shared_ptr<Device> &device,
        const vk::Extent2D &size,
        vk::Format fmt,
        bool linear,
        vk::ExternalMemoryHandleTypeFlags exportMemoryTypes,
        ImageCreateInfoCallback imageCreateInfoCallback = nullptr
    );

    static shared_ptr<Image> createFromImage(
        const shared_ptr<Device> &device,
        vector<vk::Image> &&vkImages,
        const vk::Extent2D &size,
        vk::Format fmt,
        bool linear,
        uint32_t mipLevels = 1
    );

public:
    Image(
        const shared_ptr<Device> &device,
        const vk::Extent2D &size,
        vk::Format fmt,
        uint32_t paddingHeight,
        bool linear,
        bool useMipmaps,
        bool storage,
        bool externalImport,
        bool externalImage,
        vk::ExternalMemoryHandleTypeFlags exportMemoryTypes
    );
    ~Image();

private:
    void init(
        MemoryPropertyPreset memoryPropertyPreset,
        uint32_t heap = ~0u,
        ImageCreateInfoCallback imageCreateInfoCallback = nullptr
    );
    void allocateAndBindMemory(MemoryPropertyPreset memoryPropertyPreset, uint32_t heap);

    void finishImport(const vector<vk::DeviceSize> &offsets, vk::DeviceSize globalOffset = 0u);

public:
    void recreateImageViews(vk::SamplerYcbcrConversion samperYcbcr = nullptr);

#ifdef QMVK_USE_IMAGE_BUFFER_VIEW
    shared_ptr<BufferView> bufferView(uint32_t plane = 0);
#endif

    void importFD(
        const FdDescriptors &descriptors,
        const vector<vk::DeviceSize> &offsets,
        vk::ExternalMemoryHandleTypeFlagBits handleType
    );

#ifdef VK_USE_PLATFORM_WIN32_KHR
    void importWin32Handle(
        const vector<HANDLE> &rawHandles,
        const vector<vk::DeviceSize> &offsets,
        vk::ExternalMemoryHandleTypeFlagBits handleType,
        vk::DeviceSize globalOffset = 0u
    );
#endif

public:
    inline vk::Extent2D size() const;
    inline vk::Extent2D size(uint32_t plane) const;

    inline uint32_t paddingHeight() const;
    inline uint32_t paddingHeight(uint32_t plane) const;

    inline vk::Format format() const;
    inline vk::Format format(uint32_t plane) const;

    inline bool isLinear() const;
    inline bool useMipmaps() const;
    inline bool isStorage() const;
    inline bool isExternalImport() const;
    inline bool isExternalImage() const;
    inline bool isYcbcr() const;

    inline uint32_t numPlanes() const;
    inline uint32_t numImages() const;

    inline bool isSampled() const;
    inline bool isSampledYcbcr() const;

    inline bool hasImageViews() const;
    inline vk::SamplerYcbcrConversion samplerYcbcr() const;

    inline vk::ImageView imageView(uint32_t plane = 0) const;

    inline uint32_t mipLevels() const;

    using MemoryObject::memorySize;
    inline vk::DeviceSize memorySize(uint32_t plane) const;
    inline vk::DeviceSize planeOffset(uint32_t plane = 0) const;
    inline vk::DeviceSize linesize(uint32_t plane = 0) const;

    bool setMipLevelsLimitForSize(const vk::Extent2D &size);

    void *map(uint32_t plane = ~0u);
    template<typename T>
    inline T *map(uint32_t plane = ~0u);
    void unmap();

    void copyTo(
        const shared_ptr<Image> &dstImage,
        const shared_ptr<CommandBuffer> &externalCommandBuffer = nullptr
    );

    void maybeGenerateMipmaps(const shared_ptr<CommandBuffer> &commandBuffer);

    // Modify only on external image
    inline vk::ImageLayout &imageLayout();
    inline vk::PipelineStageFlags &stage();
    inline vk::AccessFlags &accessFlags();

private:
    void fetchSubresourceLayouts();

    bool maybeGenerateMipmaps(vk::CommandBuffer commandBuffer);

    uint32_t getMipLevels(const vk::Extent2D &inSize) const;

    vk::ImageSubresourceRange getImageSubresourceRange(uint32_t mipLevels = ~0u, uint32_t plane = ~0u) const;

    inline bool mustExecPipelineBarrier(
        vk::ImageLayout dstImageLayout,
        vk::PipelineStageFlags dstStage,
        vk::AccessFlags dstAccessFlags
    );

    void pipelineBarrier(
        vk::CommandBuffer commandBuffer,
        vk::ImageLayout newLayout,
        vk::PipelineStageFlags dstStage,
        vk::AccessFlags dstAccessFlags
    );
    void pipelineBarrier(
        vk::CommandBuffer commandBuffer,
        vk::ImageLayout srcImageLayout,
        vk::ImageLayout dstImageLayout,
        vk::PipelineStageFlags srcStage,
        vk::PipelineStageFlags dstStage,
        vk::AccessFlags srcAccessFlags,
        vk::AccessFlags dstAccessFlags,
        const vk::ImageSubresourceRange &imageSubresourceRange,
        bool updateVariables
    );

private:
    const vk::Extent2D m_wantedSize;
    const uint32_t m_wantedPaddingHeight;
    const vk::Format m_mainFormat;
    const bool m_linear;
    const bool m_useMipMaps;
    const bool m_storage;
    const bool m_externalImport;
    const bool m_externalImage;
    const uint32_t m_numPlanes;
    const bool m_ycbcr;
    const uint32_t m_numImages;

    bool m_sampled = false;
    bool m_sampledYcbcr = false;

    bool m_hasImageViews = false;
    vk::SamplerYcbcrConversion m_samperYcbcr = nullptr;

    vector<vk::Extent2D> m_sizes;
    vector<uint32_t> m_paddingHeights;
    vector<vk::Format> m_formats;

    uint32_t m_mipLevels = 1;
    uint32_t m_mipLevelsLimit = 1;
    uint32_t m_mipLevelsGenerated = 1;

    vector<vk::SubresourceLayout> m_subresourceLayouts;

    vector<vk::Image> m_images;
    vector<vk::ImageView> m_imageViews;

#ifdef QMVK_USE_IMAGE_BUFFER_VIEW
    vk::UniqueBuffer m_uniqueBuffer;
    vector<shared_ptr<BufferView>> m_bufferViews;
#endif

    void *m_mapped = nullptr;

    vk::ImageLayout m_imageLayout = vk::ImageLayout::eUndefined;
    vk::PipelineStageFlags m_stage = vk::PipelineStageFlagBits::eTopOfPipe;
    vk::AccessFlags m_accessFlags;
};

/* Inline Implementation */

vk::Extent2D Image::size() const
{
    return m_wantedSize;
}
vk::Extent2D Image::size(uint32_t plane) const
{
    return m_sizes[plane];
}

uint32_t Image::paddingHeight() const
{
    return m_wantedPaddingHeight;
}
uint32_t Image::paddingHeight(uint32_t plane) const
{
    return m_paddingHeights[plane];
}

vk::Format Image::format() const
{
    return m_mainFormat;
}
vk::Format Image::format(uint32_t plane) const
{
    return m_formats[plane];
}

bool Image::isLinear() const
{
    return m_linear;
}
bool Image::useMipmaps() const
{
    return m_useMipMaps;
}
bool Image::isStorage() const
{
    return m_storage;
}
bool Image::isExternalImport() const
{
    return m_externalImport;
}
bool Image::isExternalImage() const
{
    return m_externalImage;
}
bool Image::isYcbcr() const
{
    return m_ycbcr;
}

uint32_t Image::numPlanes() const
{
    return m_numPlanes;
}
uint32_t Image::numImages() const
{
    return m_numImages;
}

bool Image::isSampled() const
{
    return m_sampled;
}
bool Image::isSampledYcbcr() const
{
    return m_sampledYcbcr;
}

bool Image::hasImageViews() const
{
    return m_hasImageViews;
}
vk::SamplerYcbcrConversion Image::samplerYcbcr() const
{
    return m_samperYcbcr;
}

vk::ImageView Image::imageView(uint32_t plane) const
{
    return m_imageViews[plane];
}

uint32_t Image::mipLevels() const
{
    return m_mipLevels;
}

vk::DeviceSize Image::memorySize(uint32_t plane) const
{
    return m_subresourceLayouts[plane].size;
}
vk::DeviceSize Image::planeOffset(uint32_t plane) const
{
    return m_subresourceLayouts[plane].offset;
}
vk::DeviceSize Image::linesize(uint32_t plane) const
{
    return m_subresourceLayouts[plane].rowPitch;
}

inline vk::ImageLayout &Image::imageLayout()
{
    return m_imageLayout;
}
inline vk::PipelineStageFlags &Image::stage()
{
    return m_stage;
}
inline vk::AccessFlags &Image::accessFlags()
{
    return m_accessFlags;
}

template<typename T>
T *Image::map(uint32_t plane)
{
    return reinterpret_cast<T *>(map(plane));
}

}
