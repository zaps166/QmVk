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
class Device;

class QMVK_EXPORT AbstractInstance : public vk::Instance, public enable_shared_from_this<AbstractInstance>
{
protected:
    // Functions must be called only once and are not thread-safe
    static PFN_vkGetInstanceProcAddr loadVulkanLibrary(const string &vulkanLibrary = {});
    static void initDispatchLoaderDynamic(PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, vk::Instance instance = {});

public:
    static const vk::DispatchLoaderDynamic &getDispatchLoaderDynamic();

    static bool isVk10();
    static uint32_t version();

protected:
    AbstractInstance() = default;
    virtual ~AbstractInstance() = default;

    unordered_set<string> getAllInstanceLayers();

    void fetchAllExtensions();
    vector<const char *> filterAvailableExtensions(
        const  vector<const char *> &wantedExtensions
    );

public:
    inline const auto &enabledExtensions() const;
    inline bool checkExtension(const char *extension) const;

    vector<shared_ptr<PhysicalDevice>> enumeratePhysicalDevices(bool compatibleOnly);

    shared_ptr<Device> createDevice(
        const shared_ptr<PhysicalDevice> &physicalDevice,
        const vk::PhysicalDeviceFeatures2 &physicalDeviceFeatures,
        const vector<const char *> &physicalDeviceExtensions,
        const vector<pair<uint32_t, uint32_t>> &queuesFamily
    );
    void resetDevice(const shared_ptr<Device> &deviceToReset);
    shared_ptr<Device> device() const;

protected:
    virtual bool isCompatibleDevice(const shared_ptr<PhysicalDevice> &physicalDevice) const = 0;
    virtual void sortPhysicalDevices(vector<shared_ptr<PhysicalDevice>> &physicalDeivecs) const;

protected:
    unordered_set<string> m_extensions;

private:
    weak_ptr<Device> m_deviceWeak;
    mutable mutex m_deviceMutex;
};

/* Inline implementation */

const auto &AbstractInstance::enabledExtensions() const
{
    return m_extensions;
}
bool AbstractInstance::checkExtension(const char *extension) const
{
    return (m_extensions.count(extension) > 0);
}

}
