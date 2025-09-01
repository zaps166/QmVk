// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2025 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include <vulkan/vulkan.hpp>

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <map>

namespace QmVk {

using namespace std;

class MemoryPropertyFlags;
class AbstractInstance;
class Device;

class QMVK_EXPORT PhysicalDevice : public vk::PhysicalDevice, public enable_shared_from_this<PhysicalDevice>
{
    friend class AbstractInstance;

public:
    using MemoryType = pair<uint32_t, vk::MemoryPropertyFlags>;

    struct MemoryHeap
    {
        uint32_t idx;
        vk::DeviceSize size;
        vk::DeviceSize budget;
        vk::DeviceSize usage;
        bool deviceLocal;
        bool multiInstance;
        bool hostVisible;
    };

    struct QueueProps
    {
        vk::QueueFlags flags;
        uint32_t familyIndex;
        uint32_t count;
    };

public:
    PhysicalDevice(
        const shared_ptr<AbstractInstance> &instance,
        vk::PhysicalDevice physicalDevice
    );
    ~PhysicalDevice();

private:
    void init();

public:
    inline pair<uint16_t, uint16_t> version() const;
    inline bool isVk10() const;

    inline const auto &extensionProperties() const;

    inline const auto &properties() const;
    inline const auto &limits() const;

    inline const auto &pciBusInfo() const;

    inline bool hasMemoryBudget() const;
    inline bool hasPciBusInfo() const;

    inline bool hasFullHostVisibleDeviceLocal() const;

    inline vk::Extent2D localWorkgroupSize() const;

    inline bool isGpu() const;

    vector<const char *> filterAvailableExtensions(
        const vector<const char *> &wantedExtensions
    ) const;

    inline bool checkExtension(const char *extension) const;
    bool checkExtensions(
        const vector<const char *> &wantedExtensions
    ) const;

    using vk::PhysicalDevice::createDevice;
    shared_ptr<Device> createDevice(
        const vk::PhysicalDeviceFeatures2 &features,
        const vector<const char *> &extensions,
        const vector<pair<uint32_t, uint32_t>> &queuesFamily
    );

    inline shared_ptr<AbstractInstance> instance() const;
    inline const vk::detail::DispatchLoaderDynamic &dld() const;

#ifdef QMVK_APPLY_MEMORY_PROPERTIES_QUIRKS
    void applyMemoryPropertiesQuirks(vk::PhysicalDeviceMemoryProperties &props) const;
#endif

    vector<MemoryHeap> getMemoryHeapsInfo() const;

    MemoryType findMemoryType(
        const MemoryPropertyFlags &memoryPropertyFlags,
        uint32_t memoryTypeBits = ~0u,
        uint32_t heap = ~0u
    ) const;
    MemoryType findMemoryType(
        uint32_t memoryTypeBits
    ) const;

    // {family index, count}
    vector<pair<uint32_t, uint32_t>> getQueuesFamily(
        vk::QueueFlags queueFlags,
        bool tryExcludeGraphics = false,
        bool firstOnly = false,
        bool exceptionOnFail = false
    ) const;

    inline const QueueProps &getQueueProps(uint32_t queueFamilyIndex) const;

    string linuxPCIPath() const;

    const vk::FormatProperties &getFormatPropertiesCached(vk::Format fmt);

private:
    const shared_ptr<AbstractInstance> m_instance;
    const vk::detail::DispatchLoaderDynamic &m_dld;

    unordered_set<string> m_extensionProperties;

    vk::PhysicalDeviceProperties2 m_properties;
    vk::PhysicalDevicePCIBusInfoPropertiesEXT m_pciBusInfo;

    bool m_hasMemoryBudget = false;
    bool m_hasPciBusInfo = false;

    bool m_hasFullHostVisibleDeviceLocal = false;

    vk::Extent2D m_localWorkgroupSize;

    map<uint32_t, QueueProps> m_queues;

    mutex m_formatPropertiesMutex;
    unordered_map<vk::Format, vk::FormatProperties> m_formatProperties;
};

/* Inline implementation */

pair<uint16_t, uint16_t> PhysicalDevice::version() const
{
    return {
        VK_VERSION_MAJOR(m_properties.properties.apiVersion),
        VK_VERSION_MINOR(m_properties.properties.apiVersion),
    };
}
bool PhysicalDevice::isVk10() const
{
    const auto v = version();
    return (v.first <= 1 && v.second == 0);
}

const auto &PhysicalDevice::extensionProperties() const
{
    return m_extensionProperties;
}

const auto &PhysicalDevice::properties() const
{
    return m_properties.properties;
}
const auto &PhysicalDevice::limits() const
{
    return properties().limits;
}

const auto &PhysicalDevice::pciBusInfo() const
{
    return m_pciBusInfo;
}

bool PhysicalDevice::hasMemoryBudget() const
{
    return m_hasMemoryBudget;
}
bool PhysicalDevice::hasPciBusInfo() const
{
    return m_hasPciBusInfo;
}

bool PhysicalDevice::hasFullHostVisibleDeviceLocal() const
{
    return m_hasFullHostVisibleDeviceLocal;
}

vk::Extent2D PhysicalDevice::localWorkgroupSize() const
{
    return m_localWorkgroupSize;
}

bool PhysicalDevice::isGpu() const
{
    const auto type = properties().deviceType;
    return (type != vk::PhysicalDeviceType::eOther && type != vk::PhysicalDeviceType::eCpu);
}

bool PhysicalDevice::checkExtension(const char *extension) const
{
    return (m_extensionProperties.count(extension) > 0);
}

shared_ptr<AbstractInstance> PhysicalDevice::instance() const
{
    return m_instance;
}
const vk::detail::DispatchLoaderDynamic &PhysicalDevice::dld() const
{
    return m_dld;
}

const PhysicalDevice::QueueProps &PhysicalDevice::getQueueProps(uint32_t queueFamilyIndex) const
{
    return m_queues.at(queueFamilyIndex);
}

}
