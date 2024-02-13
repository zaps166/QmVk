// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include <vulkan/vulkan.hpp>

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
        uint32_t queueFamilyIndex,
        Priv
    );
    ~Device();

private:
    void init(
        const vk::PhysicalDeviceFeatures2 &features,
        const vector<const char *> &extensions,
        uint32_t maxQueueCount
    );

public:
    inline shared_ptr<PhysicalDevice> physicalDevice() const;

    inline const auto &enabledExtensions() const;
    inline bool hasExtension(const char *extensionName) const;

    inline uint32_t numQueues() const;
    shared_ptr<Queue> queue(uint32_t index = 0);

private:
    const shared_ptr<PhysicalDevice> m_physicalDevice;
    const uint32_t m_queueFamilyIndex;

    unordered_set<string> m_enabledExtensions;

    mutex m_queueMutex;
    vector<weak_ptr<Queue>> m_weakQueues;
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

uint32_t Device::numQueues() const
{
    return m_weakQueues.size();
}

}
