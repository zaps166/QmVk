// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include <vulkan/vulkan.hpp>

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>

namespace QmVk {

using namespace std;

class PhysicalDevice;
class MemoryPropertyFlags;
class Queue;

class QMVK_EXPORT Device : public vk::Device, public enable_shared_from_this<Device>
{
    friend class PhysicalDevice;
    struct Priv {};

public:
    Device(
        const shared_ptr<PhysicalDevice> &physicalDevice,
        Priv
    );
    ~Device();

private:
    void init(
        const vk::PhysicalDeviceFeatures2 &features,
        const vector<const char *> &extensions,
        const vector<pair<uint32_t, uint32_t>> &queuesFamilyIn // {family index, max count}
    );

public:
    inline shared_ptr<PhysicalDevice> physicalDevice() const;

    inline const auto &enabledExtensions() const;
    inline bool hasExtension(const char *extensionName) const;

    inline const auto &queues() const;

    inline uint32_t numQueueFamilies() const;
    inline uint32_t queueFamilyIndex(uint32_t logicalQueueFamilyIndex) const;
    inline uint32_t numQueues(uint32_t queueFamilyIndex) const;

    shared_ptr<Queue> queue(uint32_t queueFamilyIndex, uint32_t index);
    inline shared_ptr<Queue> firstQueue();

private:
    const shared_ptr<PhysicalDevice> m_physicalDevice;

    unordered_set<string> m_enabledExtensions;

    vector<uint32_t> m_queues;

    mutex m_queueMutex;
    unordered_map<uint32_t, vector<weak_ptr<Queue>>> m_weakQueues;
};

/* Inline implementation */

shared_ptr<PhysicalDevice> Device::physicalDevice() const
{
    return m_physicalDevice;
}

const auto &Device::enabledExtensions() const
{
    return m_enabledExtensions;
}
bool Device::hasExtension(const char *extensionName) const
{
    return (m_enabledExtensions.count(extensionName) > 0);
}

const auto &Device::queues() const
{
    return m_queues;
}

uint32_t Device::numQueueFamilies() const
{
    return m_queues.size();
}
uint32_t Device::queueFamilyIndex(uint32_t logicalQueueFamilyIndex) const
{
    return m_queues.at(logicalQueueFamilyIndex);
}
uint32_t QmVk::Device::numQueues(uint32_t queueFamilyIndex) const
{
    return m_weakQueues.at(queueFamilyIndex).size();
}

shared_ptr<Queue> Device::firstQueue()
{
    return queue(queueFamilyIndex(0), 0);
}

}
