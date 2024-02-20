// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#include "Device.hpp"
#include "AbstractInstance.hpp"
#include "PhysicalDevice.hpp"
#include "Queue.hpp"

#include <cstring>

namespace QmVk {

Device::Device(
    const shared_ptr<PhysicalDevice> &physicalDevice,
    uint32_t queueFamilyIndex,
    Priv)
    : m_physicalDevice(physicalDevice)
    , m_queueFamilyIndex(queueFamilyIndex)
{}
Device::~Device()
{
    if (*this)
        destroy();
}

void Device::init(
    const vk::PhysicalDeviceFeatures2 &features,
    const vector<const char *> &extensions,
    uint32_t maxQueueCount)
{
    const uint32_t queueCount = min(
        maxQueueCount,
        m_physicalDevice->getQueueFamilyProperties().at(m_queueFamilyIndex).queueCount
    );

    const vector<float> queuePriorities(queueCount, 1.0f);
    vk::DeviceQueueCreateInfo queueCreateInfo(
        vk::DeviceQueueCreateFlags(),
        m_queueFamilyIndex,
        queueCount,
        queuePriorities.data()
    );

    m_enabledExtensions.reserve(extensions.size());
    for (auto &&extension : extensions)
        m_enabledExtensions.insert(extension);

    m_weakQueues.resize(queueCount);

    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = extensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = extensions.data();
    if (m_physicalDevice->instance()->checkExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
        deviceCreateInfo.pNext = &features;
    else
        deviceCreateInfo.pEnabledFeatures = &features.features;
    static_cast<vk::Device &>(*this) = m_physicalDevice->createDevice(deviceCreateInfo, nullptr);
}

shared_ptr<Queue> Device::queue(uint32_t index)
{
    lock_guard<mutex> locker(m_queueMutex);
    auto queue = m_weakQueues.at(index).lock();
    if (!queue)
    {
        queue = Queue::create(
            shared_from_this(),
            m_queueFamilyIndex,
            index
        );
        m_weakQueues[index] = queue;
    }
    return queue;
}

}
