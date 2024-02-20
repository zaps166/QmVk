// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#include "AbstractInstance.hpp"
#include "PhysicalDevice.hpp"

vk::DispatchLoaderDynamic vk::defaultDispatchLoaderDynamic;

namespace QmVk {

PFN_vkGetInstanceProcAddr AbstractInstance::loadVulkanLibrary(const string &vulkanLibrary)
{
    static unique_ptr<vk::DynamicLoader> g_dyld;

    g_dyld.reset();

    try
    {
        g_dyld = make_unique<vk::DynamicLoader>(vulkanLibrary);
    }
    catch (const runtime_error &e)
    {
        throw vk::InitializationFailedError(e.what());
    }

    auto vkGetInstanceProcAddr = g_dyld->getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    if (!vkGetInstanceProcAddr)
        throw vk::InitializationFailedError("Unable to get \"vkGetInstanceProcAddr\"");

    return vkGetInstanceProcAddr;
}
void AbstractInstance::initDispatchLoaderDynamic(PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, vk::Instance instance)
{
    if (instance)
        vk::defaultDispatchLoaderDynamic.init(instance, vkGetInstanceProcAddr);
    else
        vk::defaultDispatchLoaderDynamic.init(vkGetInstanceProcAddr);
}

const vk::DispatchLoaderDynamic &AbstractInstance::getDispatchLoaderDynamic()
{
    return vk::defaultDispatchLoaderDynamic;
}

void AbstractInstance::fetchAllExtensions()
{
    const auto instanceExtensionProperties = [] {
        vector<vk::ExtensionProperties> instanceExtensionProperties;
        uint32_t propertyCount = 0;
        auto result = vk::enumerateInstanceExtensionProperties(nullptr, &propertyCount, static_cast<vk::ExtensionProperties *>(nullptr));
        if (result == vk::Result::eSuccess && propertyCount > 0)
        {
            instanceExtensionProperties.resize(propertyCount);
            result = vk::enumerateInstanceExtensionProperties(nullptr, &propertyCount, instanceExtensionProperties.data());
            if (result != vk::Result::eSuccess && result != vk::Result::eIncomplete)
                propertyCount = 0;
            if (propertyCount != instanceExtensionProperties.size())
                instanceExtensionProperties.resize(propertyCount);
        }
        return instanceExtensionProperties;
    }();
    m_extensions.reserve(instanceExtensionProperties.size());
    for (auto &&instanceExtensionProperty : instanceExtensionProperties)
        m_extensions.insert(instanceExtensionProperty.extensionName);
}
vector<const char *> AbstractInstance::filterAvailableExtensions(
    const vector<const char *> &wantedExtensions)
{
    vector<const char *> availableWantedExtensions;
    availableWantedExtensions.reserve(wantedExtensions.size());
    for (auto &&wantedExtension : wantedExtensions)
    {
        if (checkExtension(wantedExtension))
        {
            availableWantedExtensions.push_back(wantedExtension);
            if (availableWantedExtensions.size() == wantedExtensions.size())
                break;
        }
    }
    return availableWantedExtensions;
}

vector<shared_ptr<PhysicalDevice>> AbstractInstance::enumeratePhysicalDevices(bool compatibleOnly)
{
    const auto physicalDevices = [this] {
        const auto self = static_cast<vk::Instance *>(this);
        vector<vk::PhysicalDevice> physicalDevices;
        uint32_t physicalDevicesCount = 0;
        auto result = self->enumeratePhysicalDevices(&physicalDevicesCount, static_cast<vk::PhysicalDevice *>(nullptr));
        if (result == vk::Result::eSuccess && physicalDevicesCount > 0)
        {
            physicalDevices.resize(physicalDevicesCount);
            result = self->enumeratePhysicalDevices(&physicalDevicesCount, physicalDevices.data());
            if (result != vk::Result::eSuccess && result != vk::Result::eIncomplete)
                physicalDevicesCount = 0;
            if (physicalDevicesCount != physicalDevices.size())
                physicalDevices.resize(physicalDevicesCount);
        }
        return physicalDevices;
    }();

    vector<shared_ptr<PhysicalDevice>> physicalDeviceInstances;
    physicalDeviceInstances.reserve(physicalDevices.size());

    for (auto &&vkPhysicalDevice : physicalDevices)
    {
        auto physicalDevice = make_shared<PhysicalDevice>(
            shared_from_this(),
            vkPhysicalDevice,
            PhysicalDevice::Priv()
        );
        physicalDevice->init();
        if (!compatibleOnly || isCompatibleDevice(physicalDevice))
            physicalDeviceInstances.push_back(physicalDevice);
    }

    if (physicalDeviceInstances.empty())
        throw vk::InitializationFailedError("No compatible devices found");

    if (physicalDeviceInstances.size() > 1)
        sortPhysicalDevices(physicalDeviceInstances);

    return physicalDeviceInstances;
}

shared_ptr<Device> AbstractInstance::createDevice(
    const shared_ptr<PhysicalDevice> &physicalDevice,
    const vk::PhysicalDeviceFeatures2 &physicalDeviceFeatures,
    const vector<const char *> &physicalDeviceExtensions,
    const vector<pair<uint32_t, uint32_t>> &queuesFamily)
{
    auto device = physicalDevice->createDevice(
        physicalDeviceFeatures,
        physicalDevice->filterAvailableExtensions(physicalDeviceExtensions),
        queuesFamily
    );

    lock_guard<mutex> locker(m_deviceMutex);
    m_deviceWeak = device;
    return device;
}
void AbstractInstance::resetDevice(const shared_ptr<Device> &deviceToReset)
{
    if (!deviceToReset)
        return;

    lock_guard<mutex> locker(m_deviceMutex);
    if (m_deviceWeak.lock() == deviceToReset)
        m_deviceWeak.reset();
}
shared_ptr<Device> AbstractInstance::device() const
{
    lock_guard<mutex> locker(m_deviceMutex);
    return m_deviceWeak.lock();
}

void AbstractInstance::sortPhysicalDevices(vector<shared_ptr<PhysicalDevice>> &physicalDeivecs) const
{
    (void)physicalDeivecs;
}

}
