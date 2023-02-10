// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2022 Błażej Szczygieł
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

    vk::DeviceSize deviceLocalAndHostVisibleSize = 0;
    vk::DeviceSize deviceLocalSize = 0;
    for (auto &&heapInfo : getMemoryHeapsInfo())
    {
        if (!heapInfo.deviceLocal)
            continue;

        if (heapInfo.hostVisible)
        {
            if (deviceLocalAndHostVisibleSize == 0)
                deviceLocalAndHostVisibleSize = heapInfo.size;
        }
        else
        {
            if (deviceLocalSize == 0)
                deviceLocalSize = heapInfo.size;
        }
    }
    m_hasFullHostVisibleDeviceLocal = (deviceLocalAndHostVisibleSize >= deviceLocalSize);

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

#ifdef QMVK_APPLY_MEMORY_PROPERTIES_QUIRKS
void PhysicalDevice::applyMemoryPropertiesQuirks(vk::PhysicalDeviceMemoryProperties &props) const
{
    const auto &physDevProps = properties();
    if (physDevProps.deviceType != vk::PhysicalDeviceType::eIntegratedGpu || physDevProps.vendorID != 0x1002 /* AMD */)
    {
        // Not an AMD integrated GPU
        return;
    }

    // Integrated AMD GPUs don't expose DeviceLocal flag when CPU has cached access.
    // Adding the DeviceLocal flag can prevent some copies and speed-up things,
    // because it's one and the same memory.

    constexpr auto hostFlags =
        vk::MemoryPropertyFlagBits::eHostVisible  |
        vk::MemoryPropertyFlagBits::eHostCoherent |
        vk::MemoryPropertyFlagBits::eHostCached
    ;

    const uint32_t nHeaps = props.memoryHeapCount;
    const uint32_t nTypes = props.memoryTypeCount;

    if (nHeaps <= 1)
        return;

    unordered_set<uint32_t> heapIndexes(nHeaps - 1);

    for (uint32_t i = 0; i < nTypes; ++i)
    {
        auto &memoryType = props.memoryTypes[i];

        if ((memoryType.propertyFlags & hostFlags) != hostFlags)
        {
            // Not a host coherent cached memory
            continue;
        }

        if (memoryType.propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
        {
            // We already have it, nothing to do
            return;
        }

        if (memoryType.heapIndex < nHeaps)
        {
            heapIndexes.insert(memoryType.heapIndex);
        }
    }

    for (auto &&heapIndex : heapIndexes)
    {
        props.memoryHeaps[heapIndex].flags |= vk::MemoryHeapFlagBits::eDeviceLocal;
        for (uint32_t i = 0; i < nTypes; ++i)
        {
            if (props.memoryTypes[i].heapIndex == heapIndex)
                props.memoryTypes[i].propertyFlags |= vk::MemoryPropertyFlagBits::eDeviceLocal;
        }
    }
}
#endif

vector<PhysicalDevice::MemoryHeap> PhysicalDevice::getMemoryHeapsInfo() const
{
    vk::PhysicalDeviceMemoryProperties2 props;
    vk::PhysicalDeviceMemoryBudgetPropertiesEXT budget;

    if (m_instance->checkExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
    {
        tie(props, budget) = getMemoryProperties2KHR<
            decltype(props),
            decltype(budget)
        >().get<
            decltype(props),
            decltype(budget)
        >();
    }
    else
    {
        props = getMemoryProperties();
    }
#ifdef QMVK_APPLY_MEMORY_PROPERTIES_QUIRKS
    applyMemoryPropertiesQuirks(props.memoryProperties);
#endif

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
        memoryHeaps[i].deviceLocal = static_cast<bool>(props.memoryProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal);
        memoryHeaps[i].multiInstance = static_cast<bool>(props.memoryProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eMultiInstance);
    }
    for (uint32_t i = 0; i < props.memoryProperties.memoryTypeCount; ++i)
    {
        if (props.memoryProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)
            memoryHeaps[props.memoryProperties.memoryTypes[i].heapIndex].hostVisible = true;
    }

    return memoryHeaps;
}

PhysicalDevice::MemoryType PhysicalDevice::findMemoryType(
    const MemoryPropertyFlags &memoryPropertyFlags,
    uint32_t memoryTypeBits,
    uint32_t heap) const
{
    MemoryType result;

    auto memoryProperties = getMemoryProperties();
#ifdef QMVK_APPLY_MEMORY_PROPERTIES_QUIRKS
    applyMemoryPropertiesQuirks(memoryProperties);
#endif
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
