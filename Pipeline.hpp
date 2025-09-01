// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2025 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

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

class QMVK_EXPORT Pipeline
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
        const MemoryObjectDescrs &memoryObjects,
        bool genMipmapsOnWrite,
        bool resetPipelineStageFlags
    );
    void finalizeObjects(
        const shared_ptr<CommandBuffer> &commandBuffer,
        bool genMipmapsOnWrite,
        bool resetPipelineStageFlags
    );

protected:
    const shared_ptr<Device> m_device;
    const vk::detail::DispatchLoaderDynamic &m_dld;
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
