// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2025 Błażej Szczygieł
*/

#include "Device.hpp"
#include "AbstractInstance.hpp"
#include "PhysicalDevice.hpp"
#include "Queue.hpp"

#include <cstring>

namespace QmVk {

Device::Device(const shared_ptr<PhysicalDevice> &physicalDevice)
    : m_physicalDevice(physicalDevice)
    , m_dld(m_physicalDevice->dld())
{}
Device::~Device()
{
    if (*this)
        destroy(nullptr, dld());
}

void Device::init(const vk::PhysicalDeviceFeatures2 &features,
    const vector<const char *> &extensions,
    const vector<pair<uint32_t, uint32_t>> &queuesFamilyIn)
{
    vector<pair<uint32_t, uint32_t>> queuesFamily;
    queuesFamily.reserve(queuesFamilyIn.size());

    // Remove possible duplicates
    for (uint32_t i = 0; i < queuesFamilyIn.size(); ++i)
    {
        auto it = find_if(queuesFamily.begin(), queuesFamily.end(), [queueFamilyIndex = queuesFamilyIn[i].first](auto &&p) {
            return (p.first == queueFamilyIndex);
        });
        if (it == queuesFamily.end())
        {
            queuesFamily.push_back(queuesFamilyIn[i]);
        }
    }

    vector<vk::DeviceQueueCreateInfo> queueCreateInfos(queuesFamily.size());
    vector<vector<float>> queuePriorities(queuesFamily.size());

    m_queues.reserve(queuesFamily.size());

    for (uint32_t i = 0; i < queuesFamily.size(); ++i)
    {
        const uint32_t queueFamilyIndex = queuesFamily[i].first;
        const uint32_t queueCount = min(
            queuesFamily[i].second,
            m_physicalDevice->getQueueProps(queueFamilyIndex).count
        );

        queuePriorities[i].resize(queueCount, 1.0f / queueCount);
        queueCreateInfos[i] = {
            vk::DeviceQueueCreateFlags(),
            queueFamilyIndex,
            queueCount,
            queuePriorities[i].data()
        };

        m_queues.push_back(queueFamilyIndex);

        m_weakQueues[queueFamilyIndex].resize(queueCount);
    }

    m_enabledExtensions.reserve(extensions.size());
    for (auto &&extension : extensions)
        m_enabledExtensions.insert(extension);

    const auto instance = m_physicalDevice->instance();
    const bool hasPhysDevs2Props = !instance->isVk10() || instance->checkExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.enabledExtensionCount = extensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = extensions.data();
    if (hasPhysDevs2Props)
        deviceCreateInfo.pNext = &features;
    else
        deviceCreateInfo.pEnabledFeatures = &features.features;
    static_cast<vk::Device &>(*this) = m_physicalDevice->createDevice(deviceCreateInfo, nullptr, dld());

    if (hasPhysDevs2Props)
    {
        const auto version = m_physicalDevice->version();
        const bool hasV11 = (version.first > 1 || version.second >= 1);
        const bool hasV13 = (version.first > 1 || version.second >= 3);

        const bool ycbcr = (hasV11 || hasExtension(VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME));
        const bool sync2 = (hasV13 || hasExtension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME));

        auto pNext = reinterpret_cast<vk::BaseOutStructure *>(features.pNext);
        while (pNext)
        {
            switch (pNext->sType)
            {
                case vk::StructureType::ePhysicalDeviceSamplerYcbcrConversionFeatures:
                    if (ycbcr && reinterpret_cast<vk::PhysicalDeviceSamplerYcbcrConversionFeatures *>(pNext)->samplerYcbcrConversion)
                        m_hasYcbcr = true;
                    break;
                case vk::StructureType::ePhysicalDeviceSynchronization2FeaturesKHR:
                    if (sync2 && reinterpret_cast<vk::PhysicalDeviceSynchronization2FeaturesKHR *>(pNext)->synchronization2)
                        m_hasSync2 = true;
                    break;
                default:
                    break;
            }
            pNext = pNext->pNext;
        }
    }
}

shared_ptr<Queue> Device::queue(uint32_t queueFamilyIndex, uint32_t index)
{
    lock_guard<mutex> locker(m_queueMutex);
    auto &weakQueue = m_weakQueues.at(queueFamilyIndex).at(index);
    auto queue = weakQueue.lock();
    if (!queue)
    {
        queue = Queue::create(
            shared_from_this(),
            queueFamilyIndex,
            index
        );
        weakQueue = queue;
    }
    return queue;
}

}
