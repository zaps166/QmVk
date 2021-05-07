/*
    QmVk - simple Vulkan library created for QMPlay2
    Copyright (C) 2020-2021  Błażej Szczygieł

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

#include "MemoryObject.hpp"

namespace QmVk {

using namespace std;

class BufferView;

class QMVK_EXPORT Image : public MemoryObject, public enable_shared_from_this<Image>
{
    friend class MemoryObjectDescr;
    struct Priv {};

public:
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
        bool useMipMaps,
        bool storage,
        vk::ExternalMemoryHandleTypeFlags exportMemoryTypes = {},
        uint32_t heap = ~0u
    );
    static shared_ptr<Image> createLinear(
        const shared_ptr<Device> &device,
        const vk::Extent2D &size,
        vk::Format fmt,
        uint32_t paddingHeight = 0,
        bool deviceLocal = false,
        vk::ExternalMemoryHandleTypeFlags exportMemoryTypes = {},
        uint32_t heap = ~0u
    );

    static shared_ptr<Image> createExternalImport(
        const shared_ptr<Device> &device,
        const vk::Extent2D &size,
        vk::Format fmt,
        bool linear,
        vk::ExternalMemoryHandleTypeFlags exportMemoryTypes
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
        vk::ExternalMemoryHandleTypeFlags exportMemoryTypes,
        Priv
    );
    ~Image();

private:
    void init(bool deviceLocal = false, uint32_t heap = ~0u);
    void allocateAndBindMemory(bool deviceLocal, uint32_t heap);

    void finishImport(const vector<vk::DeviceSize> &offsets, vk::DeviceSize globalOffset = 0u);

    void createImageViews();

public:
    shared_ptr<BufferView> bufferView(uint32_t plane = 0);

public:
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

    inline uint32_t numPlanes() const;

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

private:
    bool maybeGenerateMipmaps(vk::CommandBuffer commandBuffer);

    uint32_t getMipLevels(const vk::Extent2D &inSize) const;

    vk::ImageSubresourceRange getImageSubresourceRange(uint32_t mipLevels = ~0u) const;

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
    const vk::Format m_wantedFormat;
    const bool m_linear;
    const bool m_useMipMaps;
    const bool m_storage;
    const bool m_externalImport;
    const uint32_t m_numPlanes;

    vector<vk::Extent2D> m_sizes;
    vector<uint32_t> m_paddingHeights;
    vector<vk::Format> m_formats;

    uint32_t m_mipLevels = 1;
    uint32_t m_mipLevelsLimit = 1;
    uint32_t m_mipLevelsGenerated = 1;

    vector<vk::SubresourceLayout> m_subresourceLayouts;

    vector<vk::UniqueImage> m_images;
    vector<vk::UniqueImageView> m_imageViews;

    vector<shared_ptr<BufferView>> m_bufferViews;

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
    return m_wantedFormat;
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

uint32_t Image::numPlanes() const
{
    return m_numPlanes;
}

vk::ImageView Image::imageView(uint32_t plane) const
{
    return *m_imageViews[plane];
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

template<typename T>
T *Image::map(uint32_t plane)
{
    return reinterpret_cast<T *>(map(plane));
}

}
