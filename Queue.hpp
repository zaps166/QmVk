/*
    QmVk - simple Vulkan library created for QMPlay2
    Copyright (C) 2020  Błażej Szczygieł

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

#include <mutex>

namespace QmVk {

using namespace std;

class Device;

class QMVK_EXPORT Queue : public vk::Queue
{
    struct Priv {};

public:
    static shared_ptr<Queue> create(
        const shared_ptr<Device> &device,
        uint32_t queueFamilyIndex,
        uint32_t queueIndex
    );

public:
    Queue(
        const shared_ptr<Device> &device,
        uint32_t queueFamilyIndex,
        uint32_t queueIndex,
        Priv
    );
    ~Queue();

private:
    void init();

public:
    inline shared_ptr<Device> device() const;

    inline uint32_t queueFamilyIndex() const;

    unique_lock<mutex> lock();

    void submitCommandBuffer(vk::SubmitInfo &&submitInfo);
    void waitForCommandsFinished();

private:
    const shared_ptr<Device> m_device;
    const uint32_t m_queueFamilyIndex;
    const uint32_t m_queueIndex;

    bool m_fakeDeviceLost = false;

    bool m_fenceResetNeeded = false;
    vk::UniqueFence m_fence;

    mutex m_mutex;
};

/* Inline implementation */

shared_ptr<Device> Queue::device() const
{
    return m_device;
}

uint32_t Queue::queueFamilyIndex() const
{
    return m_queueFamilyIndex;
}

}
