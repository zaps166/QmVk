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

#include "MemoryObject.hpp"
#include "PhysicalDevice.hpp"
#include "Device.hpp"
#include "MemoryPropertyFlags.hpp"
#include "CommandBuffer.hpp"

namespace QmVk {

MemoryObject::MemoryObject(
    const shared_ptr<Device> &device,
    vk::ExternalMemoryHandleTypeFlags exportMemoryTypes)
    : MemoryObjectBase(device)
    , m_physicalDevice(device->physicalDevice())
    , m_exportMemoryTypes(exportMemoryTypes)
{}
MemoryObject::~MemoryObject()
{
    m_customData.reset();
    for (auto &&deviceMemory : m_deviceMemory)
        m_device->freeMemory(deviceMemory);
}

void MemoryObject::importFD(
    const FdDescriptors &descriptors,
    vk::ExternalMemoryHandleTypeFlagBits handleType)
{
    if (!m_deviceMemory.empty())
        throw vk::LogicError("Memory already allocated");

    m_deviceMemory.reserve(descriptors.size());
    for (auto &&descriptor : descriptors)
    {
        vk::ImportMemoryFdInfoKHR import;
        import.handleType = handleType;
        import.fd = descriptor.first;

        vk::MemoryAllocateInfo alloc;
        alloc.allocationSize = descriptor.second;
        alloc.pNext = &import;

        tie(alloc.memoryTypeIndex, m_memoryPropertyFlags) = m_physicalDevice->findMemoryType(
            m_device->getMemoryFdPropertiesKHR(
                handleType,
                import.fd
            ).memoryTypeBits
        );

        m_deviceMemory.push_back(m_device->allocateMemory(alloc));
    }
}

#ifdef VK_USE_PLATFORM_WIN32_KHR
void MemoryObject::importWin32Handle(
    const Win32Handles &handles,
    vk::ExternalMemoryHandleTypeFlagBits handleType)
{
    if (!m_deviceMemory.empty())
        throw vk::LogicError("Memory already allocated");

    m_deviceMemory.reserve(handles.size());
    for (auto &&handle : handles)
    {
        vk::ImportMemoryWin32HandleInfoKHR import;
        import.handleType = handleType;
        import.handle = handle.first;

        vk::MemoryAllocateInfo alloc;
        alloc.allocationSize = handle.second;
        alloc.pNext = &import;

        tie(alloc.memoryTypeIndex, m_memoryPropertyFlags) = m_physicalDevice->findMemoryType(
            m_device->getMemoryWin32HandlePropertiesKHR(
                import.handleType,
                import.handle
            ).memoryTypeBits
        );

        m_deviceMemory.push_back(m_device->allocateMemory(alloc));
    }
}
#endif

void MemoryObject::allocateMemory(
    const MemoryPropertyFlags &userMemoryPropertyFlags,
    void *allocateInfoPNext)
{
    vk::ExportMemoryAllocateInfo exportMemoryAllocateInfo(m_exportMemoryTypes);
    if (m_exportMemoryTypes)
    {
        exportMemoryAllocateInfo.pNext = allocateInfoPNext;
        allocateInfoPNext = &exportMemoryAllocateInfo;
    }

    vk::MemoryAllocateInfo allocateInfo;
    allocateInfo.allocationSize = m_memoryRequirements.size;
    allocateInfo.pNext = allocateInfoPNext;

    tie(allocateInfo.memoryTypeIndex, m_memoryPropertyFlags) = m_physicalDevice->findMemoryType(
        userMemoryPropertyFlags,
        m_memoryRequirements.memoryTypeBits,
        userMemoryPropertyFlags.heap
    );

    m_deviceMemory.push_back(m_device->allocateMemory(allocateInfo));
}

shared_ptr<CommandBuffer> MemoryObject::internalCommandBuffer()
{
    if (!m_internalCommandBuffer)
        m_internalCommandBuffer = CommandBuffer::create(m_device->queue());
    return m_internalCommandBuffer;
}

int MemoryObject::exportMemoryFd(vk::ExternalMemoryHandleTypeFlagBits type)
{
    if (!(m_exportMemoryTypes & type))
        throw vk::LogicError("Specified memory export is not initialized");

    vk::MemoryGetFdInfoKHR memFdInfo;
    memFdInfo.memory = deviceMemory();
    memFdInfo.handleType = type;
    return m_device->getMemoryFdKHR(memFdInfo);
}

#ifdef VK_USE_PLATFORM_WIN32_KHR
HANDLE MemoryObject::exportMemoryWin32(vk::ExternalMemoryHandleTypeFlagBits type)
{
    if (!(m_exportMemoryTypes & type))
        throw vk::LogicError("Specified memory export is not initialized");

    vk::MemoryGetWin32HandleInfoKHR memWin32Info;
    memWin32Info.memory = deviceMemory();
    memWin32Info.handleType = type;
    return m_device->getMemoryWin32HandleKHR(memWin32Info);
}
#endif

}
