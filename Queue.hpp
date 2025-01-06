// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2025 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
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
    inline const vk::DispatchLoaderDynamic &dld() const;

    inline uint32_t queueFamilyIndex() const;
    inline uint32_t queueIndex() const;

    unique_lock<mutex> lock();

    void submitCommandBuffer(vk::SubmitInfo &&submitInfo);
    void waitForCommandsFinished();

private:
    const shared_ptr<Device> m_device;
    const vk::DispatchLoaderDynamic &m_dld;
    const uint32_t m_queueFamilyIndex;
    const uint32_t m_queueIndex;

    bool m_fenceResetNeeded = false;
    vk::UniqueFence m_fence;

    mutex m_mutex;
};

/* Inline implementation */

shared_ptr<Device> Queue::device() const
{
    return m_device;
}
const vk::DispatchLoaderDynamic &Queue::dld() const
{
    return m_dld;
}

uint32_t Queue::queueFamilyIndex() const
{
    return m_queueFamilyIndex;
}
uint32_t Queue::queueIndex() const
{
    return m_queueIndex;
}

}
