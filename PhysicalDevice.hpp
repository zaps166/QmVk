/*
    QmVk - simple Vulkan library created for QMPlay2
    Copyright (C) 2020-2022  Błażej Szczygieł

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

#pragma once

#include "QmVkExport.hpp"

#include <vulkan/vulkan.hpp>

#include <unordered_map>
#include <unordered_set>

namespace std {

// Needed for old compilers (GCC < 6.1)
template<> struct hash<vk::Format>
{
    size_t operator ()(const vk::Format &t) const
    {
        return static_cast<size_t>(t);
    }
};

}

namespace QmVk {

using namespace std;

class MemoryPropertyFlags;
class AbstractInstance;
class Device;

class QMVK_EXPORT PhysicalDevice : public vk::PhysicalDevice, public enable_shared_from_this<PhysicalDevice>
{
    friend class AbstractInstance;
    struct Priv {};

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

public:
    PhysicalDevice(
        const shared_ptr<AbstractInstance> &instance,
        vk::PhysicalDevice physicalDevice,
        Priv
    );
    ~PhysicalDevice();

private:
    void init();

public:
    inline const auto &extensionProperties() const;

    inline const auto &properties() const;
    inline const auto &limits() const;

    inline const auto &pciBusInfo() const;

    inline bool hasMemoryBudget() const;
    inline bool hasPciBusInfo() const;

    inline bool hasFullHostVisibleDeviceLocal() const;

    inline vk::Extent2D localWorkgroupSize() const;

    vector<const char *> filterAvailableExtensions(
        const vector<const char *> &wantedExtensions
    ) const;

    inline bool checkExtension(const char *extension);
    bool checkExtensions(
        const vector<const char *> &wantedExtensions
    ) const;

    using vk::PhysicalDevice::createDevice;
    shared_ptr<Device> createDevice(
        uint32_t queueFamilyIndex,
        const vk::PhysicalDeviceFeatures2 &features,
        const vector<const char *> &extensions,
        uint32_t maxQueueCount
    );

    inline shared_ptr<AbstractInstance> instance() const;

    vector<MemoryHeap> getMemoryHeapsInfo() const;

    MemoryType findMemoryType(
        const MemoryPropertyFlags &memoryPropertyFlags,
        uint32_t memoryTypeBits = ~0u,
        uint32_t heap = ~0u
    ) const;
    MemoryType findMemoryType(
        uint32_t memoryTypeBits
    ) const;

    uint32_t getQueueFamilyIndex(
        vk::QueueFlags queueFlags,
        bool matchExactly = false
    ) const;

    string linuxPCIPath() const;

    const vk::FormatProperties &getFormatPropertiesCached(vk::Format fmt);

private:
    const shared_ptr<AbstractInstance> m_instance;

    unordered_set<string> m_extensionProperties;

    vk::PhysicalDeviceProperties2 m_properties;
    vk::PhysicalDevicePCIBusInfoPropertiesEXT m_pciBusInfo;

    bool m_hasMemoryBudget = false;
    bool m_hasPciBusInfo = false;

    bool m_hasFullHostVisibleDeviceLocal = false;

    vk::Extent2D m_localWorkgroupSize;

    unordered_map<vk::Format, vk::FormatProperties> m_formatProperties;
};

/* Inline implementation */

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

bool PhysicalDevice::checkExtension(const char *extension)
{
    return (m_extensionProperties.count(extension) > 0);
}

shared_ptr<AbstractInstance> PhysicalDevice::instance() const
{
    return m_instance;
}

}
