// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2023 Błażej Szczygieł
*/

#include "AbstractInstance.hpp"
#include "PhysicalDevice.hpp"

vk::DispatchLoaderDynamic vk::defaultDispatchLoaderDynamic;

namespace QmVk {

void AbstractInstance::init(PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr)
{
    if (!vkGetInstanceProcAddr)
    {
        static unique_ptr<vk::DynamicLoader> dyld;
        static mutex dyldMutex;

        lock_guard<mutex> locker(dyldMutex);
        if (!dyld)
        {
            try
            {
                dyld = make_unique<vk::DynamicLoader>();
            }
            catch (const runtime_error &e)
            {
                throw vk::InitializationFailedError(e.what());
            }
        }

        vkGetInstanceProcAddr = dyld->getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        if (!vkGetInstanceProcAddr)
            throw vk::InitializationFailedError("Unable to get \"vkGetInstanceProcAddr\"");
    }
    if (*this)
        vk::defaultDispatchLoaderDynamic.init(*this, vkGetInstanceProcAddr);
    else
        vk::defaultDispatchLoaderDynamic.init(vkGetInstanceProcAddr);
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
    vk::QueueFlags queueFlags,
    const vk::PhysicalDeviceFeatures2 &physicalDeviceFeatures,
    const vector<const char *> &physicalDeviceExtensions,
    uint32_t maxQueueCount)
{
    return createDevice(
        physicalDevice,
        physicalDevice->getQueueFamilyIndex(queueFlags),
        physicalDeviceFeatures,
        physicalDeviceExtensions,
        maxQueueCount
    );
}
shared_ptr<Device> AbstractInstance::createDevice(
    const shared_ptr<PhysicalDevice> &physicalDevice,
    uint32_t queueFamilyIndex,
    const vk::PhysicalDeviceFeatures2 &physicalDeviceFeatures,
    const vector<const char *> &physicalDeviceExtensions,
    uint32_t maxQueueCount)
{
    auto device = physicalDevice->createDevice(
        queueFamilyIndex,
        physicalDeviceFeatures,
        physicalDevice->filterAvailableExtensions(physicalDeviceExtensions),
        maxQueueCount
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
