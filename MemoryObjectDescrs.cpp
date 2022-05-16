// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2022 Błażej Szczygieł
*/

#include "MemoryObjectDescrs.hpp"

namespace QmVk {

MemoryObjectDescrs::MemoryObjectDescrs()
    : m_memoryObjects(make_shared<vector<MemoryObjectDescr>>())
{}
MemoryObjectDescrs::MemoryObjectDescrs(const initializer_list<MemoryObjectDescr> &memoryObjects)
    : m_memoryObjects(make_shared<vector<MemoryObjectDescr>>(memoryObjects))
{}
MemoryObjectDescrs::MemoryObjectDescrs(const vector<MemoryObjectDescr> &memoryObjects)
    : m_memoryObjects(make_shared<vector<MemoryObjectDescr>>(memoryObjects))
{}
MemoryObjectDescrs::~MemoryObjectDescrs()
{}

void MemoryObjectDescrs::append(const MemoryObjectDescr &memoryObjectDescr)
{
    m_memoryObjects->push_back(memoryObjectDescr);
}

vector<vk::DescriptorPoolSize> MemoryObjectDescrs::fetchDescriptorTypes() const
{
    vector<vk::DescriptorPoolSize> descriptorTypes;
    descriptorTypes.reserve(m_memoryObjects->size());
    for (auto &&memoryObject : *m_memoryObjects)
        descriptorTypes.push_back(memoryObject.descriptorType());
    return descriptorTypes;
}
vector<DescriptorInfo> MemoryObjectDescrs::fetchDescriptorInfos() const
{
    vector<DescriptorInfo> descriptorInfos;
    for (auto &&memoryObject : *m_memoryObjects)
    {
        for (auto &&descriptorInfo : memoryObject.descriptorInfos())
            descriptorInfos.push_back(descriptorInfo);
    }
    return descriptorInfos;
}

void MemoryObjectDescrs::prepareObjects(
    vk::CommandBuffer commandBuffer,
    vk::PipelineStageFlags pipelineStageFlags) const
{
    for (auto &&memoryObject : *m_memoryObjects)
        memoryObject.prepareObject(commandBuffer, pipelineStageFlags);
}
void MemoryObjectDescrs::finalizeObjects(
    vk::CommandBuffer commandBuffer) const
{
    for (auto &&memoryObject : *m_memoryObjects)
        memoryObject.finalizeObject(commandBuffer);
}

bool MemoryObjectDescrs::operator ==(const MemoryObjectDescrs &other) const
{
    return (*m_memoryObjects == *other.m_memoryObjects);
}

}
