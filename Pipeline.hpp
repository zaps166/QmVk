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

#include "MemoryObjectDescrs.hpp"

#include <map>

namespace QmVk {

using namespace std;

class Device;
class MemoryObjectDescr;
class DescriptorSetLayout;
class DescriptorPool;
class DescriptorSet;
class CommandBuffer;

class Pipeline
{
protected:
    Pipeline(
        const shared_ptr<Device> &device,
        const vk::ShaderStageFlags pushConstantsShaderStageFlags,
        const vk::PipelineStageFlags &objectsPipelineStageFlags,
        uint32_t pushConstantsSize
    );
    ~Pipeline();

protected:
    virtual void createPipeline() = 0;

    void setCustomSpecializationData(
        const vector<uint32_t> &data,
        vk::ShaderStageFlagBits shaderStageFlag
    );

    vk::SpecializationInfo getSpecializationInfo(
        vk::ShaderStageFlagBits shaderStageFlag,
        vector<vk::SpecializationMapEntry> &specializationMapEntries,
        vector<uint32_t> &specializationData
    ) const;

    void pushConstants(
        const shared_ptr<CommandBuffer> &commandBuffer
    );
    void bindObjects(
        const shared_ptr<CommandBuffer> &commandBuffer,
        vk::PipelineBindPoint pipelineBindPoint
    );

public:
    template<typename T>
    inline T *pushConstants();

    void createDescriptorSetFromPool(const shared_ptr<DescriptorPool> &descriptorPool);
    void setMemoryObjects(const MemoryObjectDescrs &memoryObjects);

    void prepare();

    void prepareObjects(
        const shared_ptr<CommandBuffer> &commandBuffer,
        const MemoryObjectDescrs &memoryObjects
    );
    void prepareObjects(
        const shared_ptr<CommandBuffer> &commandBuffer
    );

    void finalizeObjects(
        const shared_ptr<CommandBuffer> &commandBuffer,
        const MemoryObjectDescrs &memoryObjects
    );
    void finalizeObjects(
        const shared_ptr<CommandBuffer> &commandBuffer
    );

protected:
    const shared_ptr<Device> m_device;
    const vk::ShaderStageFlags m_pushConstantsShaderStageFlags;
    const vk::PipelineStageFlags m_objectsPipelineStageFlags;

    map<vk::ShaderStageFlagBits, vector<uint32_t>> m_customSpecializationData;

    vector<uint8_t> m_pushConstants;
    MemoryObjectDescrs m_memoryObjects;

    bool m_mustUpdateDescriptorInfos = false;
    bool m_mustRecreate = true;

    shared_ptr<DescriptorSetLayout> m_descriptorSetLayout;
    shared_ptr<DescriptorSet> m_descriptorSet;

    vk::UniquePipelineLayout m_pipelineLayout;
    vk::UniquePipeline m_pipeline;
};

/* Inline implementation */

template<typename T>
T *Pipeline::pushConstants()
{
    return reinterpret_cast<T *>(m_pushConstants.data());
}

}
