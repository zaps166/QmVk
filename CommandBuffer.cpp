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
{}
CommandBuffer::~CommandBuffer()
{}

void CommandBuffer::init()
{
    const auto device = m_queue->device();

    vk::CommandPoolCreateInfo commandPoolCreateInfo;
    commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPoolCreateInfo.queueFamilyIndex = m_queue->queueFamilyIndex();
    m_commandPool = device->createCommandPoolUnique(commandPoolCreateInfo);

    vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.commandPool = *m_commandPool;
    commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
    commandBufferAllocateInfo.commandBufferCount = 1;
    static_cast<vk::CommandBuffer &>(*this) = device->allocateCommandBuffers(commandBufferAllocateInfo)[0];
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
void CommandBuffer::resetStoredData()
{
    m_storedData.reset();
}

void CommandBuffer::resetAndBegin()
{
    if (m_resetNeeded)
    {
        reset(vk::CommandBufferResetFlags());
        resetStoredData();
    }
    begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
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

    end();

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
