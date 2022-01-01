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
    inline bool isCached() const;

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
bool MemoryObject::isCached() const
{
    return bool(m_memoryPropertyFlags & vk::MemoryPropertyFlagBits::eHostCached);
}

auto MemoryObject::exportMemoryTypes() const
{
    return m_exportMemoryTypes;
}

}
