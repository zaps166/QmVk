// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
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

        auto memoryTypeBits = m_device->getMemoryFdPropertiesKHR(
            handleType,
            import.fd
        ).memoryTypeBits;
        if (memoryTypeBits == 0 && m_device->physicalDevice()->properties().vendorID == 0x1002)
        {
            // Workaround for AMD GPUs on Mesa 20.1
            memoryTypeBits = 1;
        }

        tie(alloc.memoryTypeIndex, m_memoryPropertyFlags) = m_physicalDevice->findMemoryType(
            memoryTypeBits
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

    auto allocateMemoryInternal = [this, &allocateInfo](const MemoryPropertyFlags &userMemoryPropertyFlags) {
        tie(allocateInfo.memoryTypeIndex, m_memoryPropertyFlags) = m_physicalDevice->findMemoryType(
            userMemoryPropertyFlags,
            m_memoryRequirements.memoryTypeBits,
            userMemoryPropertyFlags.heap
        );

        m_deviceMemory.push_back(m_device->allocateMemory(allocateInfo));
    };

    try
    {
        allocateMemoryInternal(userMemoryPropertyFlags);
    }
    catch (const vk::OutOfDeviceMemoryError &e)
    {
        const auto isRequiredDeviceLocal =
            userMemoryPropertyFlags.required & vk::MemoryPropertyFlagBits::eDeviceLocal
        ;
        const auto isRequiredHostVisible =
            userMemoryPropertyFlags.required & vk::MemoryPropertyFlagBits::eHostVisible
        ;

        const auto isOptionalDeviceLocal =
            (userMemoryPropertyFlags.optional & vk::MemoryPropertyFlagBits::eDeviceLocal) ||
            (userMemoryPropertyFlags.optionalFallback & vk::MemoryPropertyFlagBits::eDeviceLocal)
        ;
        const auto isOptionalHostVisible =
            (userMemoryPropertyFlags.optional & vk::MemoryPropertyFlagBits::eHostVisible) ||
            (userMemoryPropertyFlags.optionalFallback & vk::MemoryPropertyFlagBits::eHostVisible)
        ;

        if ((isRequiredDeviceLocal && isRequiredHostVisible)
                || (isRequiredDeviceLocal && !isOptionalHostVisible)
                || (isRequiredHostVisible && !isOptionalDeviceLocal))
        {
            throw e;
        }

        auto userMemoryPropertyFlagsNew = userMemoryPropertyFlags;
        if (isOptionalDeviceLocal)
        {
            userMemoryPropertyFlagsNew.optional &=
                ~vk::MemoryPropertyFlagBits::eDeviceLocal
            ;
            userMemoryPropertyFlagsNew.optionalFallback &=
                ~vk::MemoryPropertyFlagBits::eDeviceLocal
            ;
        }
        if (isOptionalHostVisible)
        {
            userMemoryPropertyFlagsNew.optional &=
                ~(vk::MemoryPropertyFlagBits::eHostVisible |
                  vk::MemoryPropertyFlagBits::eHostCoherent |
                  vk::MemoryPropertyFlagBits::eHostCached)
            ;
            userMemoryPropertyFlagsNew.optionalFallback &=
                ~(vk::MemoryPropertyFlagBits::eHostVisible |
                  vk::MemoryPropertyFlagBits::eHostCoherent |
                  vk::MemoryPropertyFlagBits::eHostCached)
            ;
        }
        allocateMemoryInternal(userMemoryPropertyFlagsNew);
    }
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
