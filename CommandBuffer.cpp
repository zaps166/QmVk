// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#include "CommandBuffer.hpp"
#include "Device.hpp"
#include "Queue.hpp"
#include "DescriptorSet.hpp"
#include "MemoryObjectDescrs.hpp"

#include <unordered_set>

namespace QmVk {

struct CommandBuffer::StoredData
{
    unordered_set<MemoryObjectDescrs> memoryObjects;
    unordered_set<shared_ptr<DescriptorSet>> descriptorSets;
    unordered_set<shared_ptr<MemoryObjectBase>> memoryObjectsBase;
};

shared_ptr<CommandBuffer> CommandBuffer::create(
    const shared_ptr<Queue> &queue)
{
    auto commandBuffer = make_shared<CommandBuffer>(
        queue,
        Priv()
    );
    commandBuffer->init();
    return commandBuffer;
}

CommandBuffer::CommandBuffer(
    const shared_ptr<Queue> &queue,
    Priv)
    : m_queue(queue)
    , m_dld(m_queue->dld())
{}
CommandBuffer::~CommandBuffer()
{}

void CommandBuffer::init()
{
    const auto device = m_queue->device();

    vk::CommandPoolCreateInfo commandPoolCreateInfo;
    commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPoolCreateInfo.queueFamilyIndex = m_queue->queueFamilyIndex();
    m_commandPool = device->createCommandPoolUnique(commandPoolCreateInfo, nullptr, dld());

    vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.commandPool = *m_commandPool;
    commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
    commandBufferAllocateInfo.commandBufferCount = 1;
    static_cast<vk::CommandBuffer &>(*this) = device->allocateCommandBuffers(commandBufferAllocateInfo, dld())[0];
}

void CommandBuffer::storeData(
    const MemoryObjectDescrs &memoryObjects,
    const shared_ptr<DescriptorSet> &descriptorSet)
{
    if (!m_storedData)
        m_storedData = make_unique<StoredData>();

    m_storedData->memoryObjects.insert(memoryObjects);
    m_storedData->descriptorSets.insert(descriptorSet);
}
void CommandBuffer::storeData(
    const shared_ptr<MemoryObjectBase> &memoryObjectBase)
{
    if (!m_storedData)
        m_storedData = make_unique<StoredData>();

    m_storedData->memoryObjectsBase.insert(memoryObjectBase);
}
void CommandBuffer::resetStoredData()
{
    if (!m_storedData)
        return;

    m_storedData->memoryObjects.clear();
    m_storedData->descriptorSets.clear();
    m_storedData->memoryObjectsBase.clear();
}

void CommandBuffer::resetAndBegin()
{
    if (m_resetNeeded)
    {
        reset(vk::CommandBufferResetFlags(), dld());
        resetStoredData();
    }
    begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit), dld());
    m_resetNeeded = true;
}
void CommandBuffer::endSubmitAndWait(
    vk::SubmitInfo &&submitInfo)
{
    endSubmitAndWait(true, nullptr, move(submitInfo));
}
void CommandBuffer::endSubmitAndWait(
    bool lock,
    const Callback &callback,
    vk::SubmitInfo &&submitInfo)
{
    unique_lock<mutex> queueLock;

    end(dld());

    if (lock)
        queueLock = m_queue->lock();

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &*this;
    m_queue->submitCommandBuffer(move(submitInfo));

    if (callback)
        callback();

    m_queue->waitForCommandsFinished();

    resetStoredData();
}

void CommandBuffer::execute(const CommandCallback &callback)
{
    resetAndBegin();
    callback(*this);
    endSubmitAndWait();
}

}
