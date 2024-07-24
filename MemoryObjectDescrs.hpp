// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include "MemoryObjectDescr.hpp"

#include <functional>

namespace QmVk {

using namespace std;

class QMVK_EXPORT MemoryObjectDescrs
{
    friend class hash<MemoryObjectDescrs>;
    friend class Pipeline;

    using Callback = function<void(const shared_ptr<MemoryObjectBase> &)>;

public:
    MemoryObjectDescrs();
    MemoryObjectDescrs(const initializer_list<MemoryObjectDescr> &memoryObjects);
    MemoryObjectDescrs(const vector<MemoryObjectDescr> &memoryObjects);
    ~MemoryObjectDescrs();

public:
    void append(const MemoryObjectDescr &memoryObjectDescr);

    vector<DescriptorType> fetchDescriptorTypes() const;
    vector<DescriptorInfo> fetchDescriptorInfos() const;

    void iterateMemoryObjects(const Callback &callback) const;

private:
    void prepareObjects(
        vk::CommandBuffer commandBuffer,
        vk::PipelineStageFlags pipelineStageFlags
    ) const;
    void finalizeObjects(
        vk::CommandBuffer commandBuffer,
        bool genMipmapsOnWrite,
        bool resetPipelineStageFlags
    ) const;

public:
    bool operator ==(const MemoryObjectDescrs &other) const;

private:
    shared_ptr<vector<MemoryObjectDescr>> m_memoryObjects;
};

}

namespace std {

template <>
class hash<QmVk::MemoryObjectDescrs>
{
public:
    size_t operator ()(const QmVk::MemoryObjectDescrs &k) const
    {
        return hash<decltype(k.m_memoryObjects)>{}(k.m_memoryObjects);
    }
};

}
