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

#pragma once

#include "QmVkExport.hpp"

#include "MemoryObjectDescr.hpp"

namespace QmVk {

using namespace std;

class QMVK_EXPORT MemoryObjectDescrs
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
    void prepareImages(
        vk::CommandBuffer commandBuffer,
        vk::PipelineStageFlags pipelineStageFlags
    ) const;
    void finalizeImages(
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
