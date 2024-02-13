// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include "MemoryObjectBase.hpp"

namespace QmVk {

using namespace std;

class MemoryPropertyFlags;
class PhysicalDevice;
class CommandBuffer;

class QMVK_EXPORT MemoryObject : public MemoryObjectBase
{
public:
    using FdDescriptors = vector<pair<int, size_t>>;

#ifdef VK_USE_PLATFORM_WIN32_KHR
    using Win32Handles = vector<pair<HANDLE, vk::DeviceSize>>;
#endif

protected:
    MemoryObject(
        const shared_ptr<Device> &device,
        vk::ExternalMemoryHandleTypeFlags exportMemoryTypes = {}
    );
    ~MemoryObject();

public:
    void importFD(
        const FdDescriptors &descriptors,
        vk::ExternalMemoryHandleTypeFlagBits handleType
    );

#ifdef VK_USE_PLATFORM_WIN32_KHR
    void importWin32Handle(
        const Win32Handles &handles,
        vk::ExternalMemoryHandleTypeFlagBits handleType
    );
#endif

protected:
    void allocateMemory(
        const MemoryPropertyFlags &userMemoryPropertyFlags,
        void *allocateInfoPNext = nullptr
    );

protected:
    shared_ptr<CommandBuffer> internalCommandBuffer();

public:
    inline uint32_t deviceMemoryCount() const;
    inline vk::DeviceMemory deviceMemory(uint32_t idx = 0) const;

    inline vk::DeviceSize memorySize() const;

    inline bool isDeviceLocal() const;
    inline bool isHostVisible() const;
    inline bool isHostCoherent() const;
    inline bool isHostCached() const;

    inline auto exportMemoryTypes() const;

    int exportMemoryFd(vk::ExternalMemoryHandleTypeFlagBits type);

#ifdef VK_USE_PLATFORM_WIN32_KHR
    HANDLE exportMemoryWin32(vk::ExternalMemoryHandleTypeFlagBits type);
#endif

protected:
    const shared_ptr<PhysicalDevice> m_physicalDevice;

    const vk::ExternalMemoryHandleTypeFlags m_exportMemoryTypes;

    vk::MemoryRequirements m_memoryRequirements;
    vk::MemoryPropertyFlags m_memoryPropertyFlags;

    vector<vk::DeviceMemory> m_deviceMemory;

private:
    shared_ptr<CommandBuffer> m_internalCommandBuffer;
};

/* Inline implementation */

uint32_t MemoryObject::deviceMemoryCount() const
{
    return m_deviceMemory.size();
}
vk::DeviceMemory MemoryObject::deviceMemory(uint32_t idx) const
{
    return m_deviceMemory[idx];
}

vk::DeviceSize MemoryObject::memorySize() const
{
    return m_memoryRequirements.size;
}

bool MemoryObject::isDeviceLocal() const
{
    return bool(m_memoryPropertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal);
}
bool MemoryObject::isHostVisible() const
{
    return bool(m_memoryPropertyFlags & vk::MemoryPropertyFlagBits::eHostVisible);
}
bool MemoryObject::isHostCoherent() const
{
    return bool(m_memoryPropertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent);
}
bool MemoryObject::isHostCached() const
{
    return bool(m_memoryPropertyFlags & vk::MemoryPropertyFlagBits::eHostCached);
}

auto MemoryObject::exportMemoryTypes() const
{
    return m_exportMemoryTypes;
}

}
