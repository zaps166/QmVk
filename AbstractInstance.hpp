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

#include <vulkan/vulkan.hpp>

#include <mutex>
#include <set>

namespace QmVk {

using namespace std;

class PhysicalDevice;
class Device;

class AbstractInstance : public vk::Instance, public enable_shared_from_this<AbstractInstance>
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
