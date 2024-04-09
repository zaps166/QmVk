// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#include "MemoryObjectDescrs.hpp"

#ifndef NDEBUG
#   include <unordered_map>
#endif

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

vector<DescriptorType> MemoryObjectDescrs::fetchDescriptorTypes() const
{
    vector<DescriptorType> descriptorTypes;
    descriptorTypes.reserve(m_memoryObjects->size());
    for (auto &&memoryObjectDescr : *m_memoryObjects)
        descriptorTypes.push_back(memoryObjectDescr.descriptorType());
    return descriptorTypes;
}
vector<DescriptorInfo> MemoryObjectDescrs::fetchDescriptorInfos() const
{
    vector<DescriptorInfo> descriptorInfos;
    for (auto &&memoryObjectDescr : *m_memoryObjects)
    {
        for (auto &&descriptorInfo : memoryObjectDescr.descriptorInfos())
            descriptorInfos.push_back(descriptorInfo);
    }
    return descriptorInfos;
}

void MemoryObjectDescrs::prepareObjects(
    vk::CommandBuffer commandBuffer,
    vk::PipelineStageFlags pipelineStageFlags) const
{
#ifndef NDEBUG
    unordered_map<MemoryObjectBase *, MemoryObjectDescr::Access> accessMap;
    for (auto &&memoryObjectDescr : *m_memoryObjects)
    {
        for (auto &&memoryObject : memoryObjectDescr.m_objects)
        {
            auto it = accessMap.find(memoryObject.get());
            if (it == accessMap.end())
            {
                accessMap[memoryObject.get()] = memoryObjectDescr.m_access;
            }
            else if (it->second != memoryObjectDescr.m_access)
            {
                throw vk::LogicError("Different access to the same memory object");
            }
        }
    }
#endif
    for (auto &&memoryObjectDescr : *m_memoryObjects)
        memoryObjectDescr.prepareObject(commandBuffer, pipelineStageFlags);
}
void MemoryObjectDescrs::finalizeObjects(
    vk::CommandBuffer commandBuffer,
    bool genMipmapsOnWrite,
    bool resetPipelineStageFlags) const
{
    for (auto &&memoryObjectDescr : *m_memoryObjects)
        memoryObjectDescr.finalizeObject(commandBuffer, genMipmapsOnWrite, resetPipelineStageFlags);
}

bool MemoryObjectDescrs::operator ==(const MemoryObjectDescrs &other) const
{
    return (*m_memoryObjects == *other.m_memoryObjects);
}

}
