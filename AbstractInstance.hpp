// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2022 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include <vulkan/vulkan.hpp>

#include <mutex>
#include <set>

namespace QmVk {

using namespace std;

class PhysicalDevice;
class Device;

class QMVK_EXPORT AbstractInstance : public vk::Instance, public enable_shared_from_this<AbstractInstance>
{
protected:
    AbstractInstance() = default;
    virtual ~AbstractInstance() = default;

protected:
    void init(PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr);

public:
    inline bool checkExtension(const char *extension) const;

    vector<shared_ptr<PhysicalDevice>> enumeratePhysicalDevices(bool compatibleOnly);

    shared_ptr<Device> createDevice(
        const shared_ptr<PhysicalDevice> &physicalDevice,
        vk::QueueFlags queueFlags,
        const vk::PhysicalDeviceFeatures2 &physicalDeviceFeatures,
        const vector<const char *> &physicalDeviceExtensions,
        uint32_t maxQueueCount
    );
    shared_ptr<Device> createDevice(
        const shared_ptr<PhysicalDevice> &physicalDevice,
        uint32_t queueFamilyIndex,
        const vk::PhysicalDeviceFeatures2 &physicalDeviceFeatures,
        const vector<const char *> &physicalDeviceExtensions,
        uint32_t maxQueueCount
    );
    void resetDevice(const shared_ptr<Device> &deviceToReset);
    shared_ptr<Device> device() const;

protected:
    virtual bool isCompatibleDevice(const shared_ptr<PhysicalDevice> &physicalDevice) const = 0;
    virtual void sortPhysicalDevices(vector<shared_ptr<PhysicalDevice>> &physicalDeivecs) const;

protected:
    set<string> m_extensions;

private:
    weak_ptr<Device> m_deviceWeak;
    mutable mutex m_deviceMutex;
};

/* Inline implementation */

bool AbstractInstance::checkExtension(const char *extension) const
{
    return (m_extensions.count(extension) > 0);
}

}
