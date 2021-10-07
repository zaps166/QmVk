/*
    QmVk - simple Vulkan library created for QMPlay2
    Copyright (C) 2020-2021  Błażej Szczygieł

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

#include "Queue.hpp"
#include "Device.hpp"
#include "PhysicalDevice.hpp"

namespace QmVk {

shared_ptr<Queue> Queue::create(
    const shared_ptr<Device> &device,
    const uint32_t m_queueFamilyIndex,
    const uint32_t m_queueIndex)
{
    auto queue = make_shared<Queue>(
        device,
        m_queueFamilyIndex,
        m_queueIndex,
        Priv()
    );
    queue->init();
    return queue;
}

Queue::Queue(
    const shared_ptr<Device> &device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex,
    Priv)
    : m_device(device)
    , m_queueFamilyIndex(queueFamilyIndex)
    , m_queueIndex(queueIndex)
{}
Queue::~Queue()
{}

void Queue::init()
{
    static_cast<vk::Queue &>(*this) = m_device->getQueue(m_queueFamilyIndex, m_queueIndex);
    m_fence = m_device->createFenceUnique(vk::FenceCreateInfo());
}

unique_lock<mutex> Queue::lock()
{
    return unique_lock<mutex>(m_mutex);
}

void Queue::submitCommandBuffer(vk::SubmitInfo &&submitInfo)
{
    if (m_fenceResetNeeded)
    {
        m_device->resetFences(*m_fence);
        m_fenceResetNeeded = false;
    }
    submit(submitInfo, *m_fence);
    m_fenceResetNeeded = true;
}
void Queue::waitForCommandsFinished()
{
    auto result = m_device->waitForFences(
        *m_fence,
        true,
#ifdef QMVK_WAIT_TIMEOUT_MS
        QMVK_WAIT_TIMEOUT_MS * static_cast<uint64_t>(1e6)
#else
        numeric_limits<uint64_t>::max()
#endif
    );
    if (result == vk::Result::eTimeout)
        throw vk::SystemError(vk::make_error_code(result), "vkWaitForFences");
}

}
