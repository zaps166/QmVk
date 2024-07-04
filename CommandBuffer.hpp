// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include <vulkan/vulkan.hpp>

#include <functional>
#include <memory>

namespace QmVk {

using namespace std;

class MemoryObjectDescrs;
class MemoryObjectBase;
class DescriptorSet;
class Queue;

class QMVK_EXPORT CommandBuffer : public vk::CommandBuffer
{
    struct Priv {};
    struct StoredData;

public:
    using Callback = function<void()>;
    using CommandCallback = function<void(vk::CommandBuffer)>;

public:
    static shared_ptr<CommandBuffer> create(
        const shared_ptr<Queue> &queue
    );

public:
    CommandBuffer(
        const shared_ptr<Queue> &queue,
        Priv
    );
    ~CommandBuffer();

private:
    void init();

public:
    inline shared_ptr<Queue> queue() const;
    inline const vk::DispatchLoaderDynamic &dld() const;

    void storeData(
        const MemoryObjectDescrs &memoryObjects,
        const shared_ptr<DescriptorSet> &descriptorSet
    );
    void storeData(
        const shared_ptr<MemoryObjectBase> &memoryObjectBase
    );
    void resetStoredData();

    void resetAndBegin();
    void endSubmitAndWait(
        vk::SubmitInfo &&submitInfo = vk::SubmitInfo()
    );
    void endSubmitAndWait(
        bool lock,
        const Callback &callback,
        vk::SubmitInfo &&submitInfo
    );

    void execute(const CommandCallback &callback);

private:
    const shared_ptr<Queue> m_queue;
    const vk::DispatchLoaderDynamic &m_dld;

    vk::UniqueCommandPool m_commandPool;

    unique_ptr<StoredData> m_storedData;
    bool m_resetNeeded = false;
};

/* Inline implementation */

shared_ptr<Queue> CommandBuffer::queue() const
{
    return m_queue;
}
const vk::DispatchLoaderDynamic &CommandBuffer::dld() const
{
    return m_dld;
}

}
