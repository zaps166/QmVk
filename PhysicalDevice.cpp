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

#include "PhysicalDevice.hpp"
#include "AbstractInstance.hpp"
#include "MemoryPropertyFlags.hpp"
#include "Device.hpp"

#include <cmath>

namespace QmVk {

PhysicalDevice::PhysicalDevice(
        const shared_ptr<AbstractInstance> &instance,
        vk::PhysicalDevice physicalDevice,
        Priv)
    : vk::PhysicalDevice(physicalDevice)
    , m_instance(instance)
{}
PhysicalDevice::~PhysicalDevice()
{}

void PhysicalDevice::init()
{
    for (auto &&extensionProperty : enumerateDeviceExtensionProperties())
        m_extensionProperties.insert(static_cast<string>(extensionProperty.extensionName));

    if (m_instance->checkExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
    {
        tie(m_properties, m_pciBusInfo) = getProperties2KHR<
            decltype(m_properties),
            decltype(m_pciBusInfo)
        >().get<
            decltype(m_properties),
            decltype(m_pciBusInfo)
        >();

        m_hasMemoryBudget = checkExtension(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
        m_hasPciBusInfo = checkExtension(VK_EXT_PCI_BUS_INFO_EXTENSION_NAME);
    }
    else
    {
        m_properties = getProperties();
    }

    const uint32_t localWorkgroupSizeSqr = pow(2.0, floor(log2(sqrt(limits().maxComputeWorkGroupInvocations))));
    m_localWorkgroupSize = vk::Extent2D(
        min(localWorkgroupSizeSqr, limits().maxComputeWorkGroupSize[0]),
        min(localWorkgroupSizeSqr, limits().maxComputeWorkGroupSize[1])
    );
}

vector<const char *> PhysicalDevice::filterAvailableExtensions(
    const vector<const char *> &wantedExtensions) const
{
    vector<const char *> availableWantedExtensions;
    availableWantedExtensions.reserve(wantedExtensions.size());
    for (auto &&wantedExtension : wantedExtensions)
    {
        if (m_extensionProperties.count(wantedExtension) > 0)
        {
            availableWantedExtensions.push_back(wantedExtension);
            if (availableWantedExtensions.size() == wantedExtensions.size())
                break;
        }
    }
    return availableWantedExtensions;
}

bool PhysicalDevice::checkExtensions(
    const vector<const char *> &wantedExtensions) const
{
    size_t foundExtensions = 0;
    for (auto &&wantedExtension : wantedExtensions)
    {
        if (m_extensionProperties.count(wantedExtension) > 0)
        {
            ++foundExtensions;
            if (foundExtensions == wantedExtensions.size())
                return true;
        }
    }
    return false;
}

shared_ptr<Device> PhysicalDevice::createDevice(
    uint32_t queueFamilyIndex,
    const vk::PhysicalDeviceFeatures2 &features,
    const vector<const char *> &extensions,
    uint32_t maxQueueCount)
{
    auto device = make_shared<Device>(
        shared_from_this(),
        queueFamilyIndex,
        Device::Priv()
    );
    device->init(features, extensions, maxQueueCount);
    return device;
}

vector<PhysicalDevice::MemoryHeap> PhysicalDevice::getMemoryHeapsInfo() const
{
    vk::PhysicalDeviceMemoryProperties2 props;
    vk::PhysicalDeviceMemoryBudgetPropertiesEXT budget;

    tie(props, budget) = getMemoryProperties2<
        decltype(props),
        decltype(budget)
    >().get<
        decltype(props),
        decltype(budget)
    >();

    vector<MemoryHeap> memoryHeaps(props.memoryProperties.memoryHeapCount);
    for (uint32_t i = 0; i < props.memoryProperties.memoryHeapCount; ++i)
    {
        memoryHeaps[i].idx = i;
        memoryHeaps[i].size = props.memoryProperties.memoryHeaps[i].size;
        if (m_hasMemoryBudget)
        {
            memoryHeaps[i].budget = budget.heapBudget[i];
            memoryHeaps[i].usage = budget.heapUsage[i];
        }
        else
        {
            memoryHeaps[i].budget = memoryHeaps[i].size;
            memoryHeaps[i].usage = 0;
        }
        memoryHeaps[i].deviceLocal = bool(props.memoryProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal);
        memoryHeaps[i].multiInstance = bool(props.memoryProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eMultiInstance);
    }

    return memoryHeaps;
}

PhysicalDevice::MemoryType PhysicalDevice::findMemoryType(
    const MemoryPropertyFlags &memoryPropertyFlags,
    uint32_t memoryTypeBits,
    uint32_t heap) const
{
    MemoryType result;

    const auto memoryProperties = getMemoryProperties();
    bool optionalFallbackFound = false;
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        if (heap != ~0u && memoryProperties.memoryTypes[i].heapIndex != heap)
            continue;

        if (!(memoryTypeBits & (1u << i)))
            continue;

        const auto currMemoryPropertyFlags = memoryProperties.memoryTypes[i].propertyFlags;
        const auto required = memoryPropertyFlags.required;
        if ((currMemoryPropertyFlags & required) == required)
        {
            const MemoryType currResult = {i, currMemoryPropertyFlags};
            bool doBreak = false;

            const auto optional = memoryPropertyFlags.optional;
            const auto optionalFallback = memoryPropertyFlags.optionalFallback;
            const auto notWanted = memoryPropertyFlags.notWanted;

            auto getFlagsWithoutNotWanted = [&] {
                return (currMemoryPropertyFlags & ~notWanted);
            };

            if (optional || optionalFallback)
            {
                auto testFlags = [&](vk::MemoryPropertyFlags flags) {
                    return ((getFlagsWithoutNotWanted() & flags) == flags);
                };

                if (optional && testFlags(optional))
                {
                    result = currResult;
                    break;
                }
                if (optionalFallback && !optionalFallbackFound && testFlags(optionalFallback))
                {
                    result = currResult;
                    optionalFallbackFound = true;
                }
            }
            else if (notWanted)
            {
                if (getFlagsWithoutNotWanted() == currMemoryPropertyFlags)
                {
                    result = currResult;
                    break;
                }
            }
            else
            {
                doBreak = true;
            }

            if (!result.second)
                result = currResult;

            if (doBreak)
                break;
        }
    }

    if (!result.second)
        throw vk::InitializationFailedError("Cannot find specified memory type");

    return result;
}
PhysicalDevice::MemoryType PhysicalDevice::findMemoryType(
    uint32_t memoryTypeBits) const
{
    return findMemoryType(MemoryPropertyFlags(), memoryTypeBits);
}

uint32_t PhysicalDevice::getQueueFamilyIndex(
    vk::QueueFlags queueFlags,
    bool matchExactly) const
{
    const auto queueFamilies = getQueueFamilyProperties();
    for (uint32_t i = 0; i < queueFamilies.size(); ++i)
    {
        auto &&props = queueFamilies[i];
        if (props.queueCount < 1)
            continue;
        if ((matchExactly && props.queueFlags == queueFlags) || (!matchExactly && (props.queueFlags & queueFlags)))
            return i;
    }
    throw vk::InitializationFailedError("Cannot find specified queue family index");
}

string PhysicalDevice::linuxPCIPath() const
{
    if (!m_hasPciBusInfo)
        return string();

    char out[13];
    snprintf(
        out,
        sizeof(out),
        "%.4x:%.2x:%.2x.%1x",
        m_pciBusInfo.pciDomain,
        m_pciBusInfo.pciBus,
        m_pciBusInfo.pciDevice,
        m_pciBusInfo.pciFunction
    );
    return out;
}

const vk::FormatProperties &PhysicalDevice::getFormatPropertiesCached(vk::Format fmt)
{
    auto it = m_formatProperties.find(fmt);
    if (it == m_formatProperties.end())
    {
        m_formatProperties[fmt] = getFormatProperties(fmt);
        it = m_formatProperties.find(fmt);
    }
    return it->second;
}

}
