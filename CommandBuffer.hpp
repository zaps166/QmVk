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

#include <functional>

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

    vk::UniqueCommandPool m_commandPool;

    unique_ptr<StoredData> m_storedData;
    bool m_resetNeeded = false;
};

}
