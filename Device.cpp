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

#include "Device.hpp"
#include "AbstractInstance.hpp"
#include "PhysicalDevice.hpp"
#include "MemoryPropertyFlags.hpp"
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
