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

void MemoryObjectDescrs::prepareImages(
    vk::CommandBuffer commandBuffer,
    vk::PipelineStageFlags pipelineStageFlags) const
{
    for (auto &&memoryObject : *m_memoryObjects)
        memoryObject.prepareImage(commandBuffer, pipelineStageFlags);
}
void MemoryObjectDescrs::finalizeImages(
    vk::CommandBuffer commandBuffer) const
{
    for (auto &&memoryObject : *m_memoryObjects)
        memoryObject.finalizeImage(commandBuffer);
}

bool MemoryObjectDescrs::operator ==(const MemoryObjectDescrs &other) const
{
    return (*m_memoryObjects == *other.m_memoryObjects);
}

}
