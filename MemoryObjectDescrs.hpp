// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2022 Błażej Szczygieł
*/

#pragma once

#include "MemoryObjectDescr.hpp"

namespace QmVk {

using namespace std;

class MemoryObjectDescrs
{
    friend class hash<MemoryObjectDescrs>;
    friend class Pipeline;

public:
    MemoryObjectDescrs();
    MemoryObjectDescrs(const initializer_list<MemoryObjectDescr> &memoryObjects);
    MemoryObjectDescrs(const vector<MemoryObjectDescr> &memoryObjects);
    ~MemoryObjectDescrs();

public:
    void append(const MemoryObjectDescr &memoryObjectDescr);

    vector<vk::DescriptorPoolSize> fetchDescriptorTypes() const;
    vector<DescriptorInfo> fetchDescriptorInfos() const;

private:
    void prepareObjects(
        vk::CommandBuffer commandBuffer,
        vk::PipelineStageFlags pipelineStageFlags
    ) const;
    void finalizeObjects(
        vk::CommandBuffer commandBuffer
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
