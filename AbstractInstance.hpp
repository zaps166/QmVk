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
    AbstractInstance() = default;
    virtual ~AbstractInstance() = default;

    PFN_vkGetInstanceProcAddr loadVulkanLibrary(const string &vulkanLibrary = {});
    PFN_vkGetInstanceProcAddr setVulkanLibrary(const shared_ptr<vk::DynamicLoader> &dl);
    void initDispatchLoaderDynamic(PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, vk::Instance instance = {});

    unordered_set<string> getAllInstanceLayers();

    void fetchAllExtensions();
    vector<const char *> filterAvailableExtensions(
        const  vector<const char *> &wantedExtensions
    );

public:
    inline shared_ptr<vk::DynamicLoader> getDl() const;

    inline const vk::DispatchLoaderDynamic &dld() const;

    inline bool isVk10();
    uint32_t version();

    inline const auto &enabledExtensions() const;
    inline bool checkExtension(const char *extension) const;

    vector<shared_ptr<PhysicalDevice>> enumeratePhysicalDevices(bool compatibleOnly, bool sort = true);

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
    shared_ptr<vk::DynamicLoader> m_dl;
    vk::DispatchLoaderDynamic m_dld;
    weak_ptr<Device> m_deviceWeak;
    mutable mutex m_deviceMutex;
};

/* Inline implementation */

shared_ptr<vk::DynamicLoader> AbstractInstance::getDl() const
{
    return m_dl;
}

const vk::DispatchLoaderDynamic &AbstractInstance::dld() const
{
    return m_dld;
}

bool AbstractInstance::isVk10()
{
    return (dld().vkEnumerateInstanceVersion == nullptr);
}

const auto &AbstractInstance::enabledExtensions() const
{
    return m_extensions;
}
bool AbstractInstance::checkExtension(const char *extension) const
{
    return (m_extensions.count(extension) > 0);
}

}
