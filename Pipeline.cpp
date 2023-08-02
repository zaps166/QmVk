// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2023 Błażej Szczygieł
*/

#include "Pipeline.hpp"
#include "PhysicalDevice.hpp"
#include "Device.hpp"
#include "DescriptorPool.hpp"
#include "DescriptorSet.hpp"
#include "DescriptorInfo.hpp"
#include "CommandBuffer.hpp"

namespace QmVk {

Pipeline::Pipeline(
    const shared_ptr<Device> &device,
    const vk::ShaderStageFlags pushConstantsShaderStageFlags,
    const vk::PipelineStageFlags &objectsPipelineStageFlags,
    uint32_t pushConstantsSize)
    : m_device(device)
    , m_pushConstantsShaderStageFlags(pushConstantsShaderStageFlags)
    , m_objectsPipelineStageFlags(objectsPipelineStageFlags)
    , m_pushConstants(pushConstantsSize)
{}
Pipeline::~Pipeline()
{}

void Pipeline::setCustomSpecializationData(
    const vector<uint32_t> &data,
    vk::ShaderStageFlagBits shaderStageFlag)
{
    auto &customSpecializationData = m_customSpecializationData[shaderStageFlag];
    if (customSpecializationData != data)
    {
        m_mustRecreate = true;
        customSpecializationData = data;
    }
}

vk::SpecializationInfo Pipeline::getSpecializationInfo(
    vk::ShaderStageFlagBits shaderStageFlag,
    vector<vk::SpecializationMapEntry> &specializationMapEntries,
    vector<uint32_t> &specializationData) const
{
    constexpr uint32_t constantSize = sizeof(uint32_t);
    const uint32_t initialCount = specializationData.size();

    for (uint32_t i = 0; i < initialCount; ++i)
        specializationMapEntries.emplace_back(i, i * constantSize, constantSize);

    auto customSpecializationDataIt = m_customSpecializationData.find(shaderStageFlag);
    if (customSpecializationDataIt != m_customSpecializationData.end())
    {
        for (uint32_t i = 0; i < customSpecializationDataIt->second.size(); ++i)
        {
            const uint32_t o = i + initialCount;
            specializationMapEntries.emplace_back(o, o * constantSize, constantSize);
            specializationData.push_back(customSpecializationDataIt->second[i]);
        }
    }

    return vk::SpecializationInfo(
        specializationMapEntries.size(),
        specializationMapEntries.data(),
        specializationData.size() * constantSize,
        specializationData.data()
    );
}

void Pipeline::pushConstants(
    const shared_ptr<CommandBuffer> &commandBuffer)
{
    if (m_pushConstants.empty())
        return;

    commandBuffer->pushConstants(
        *m_pipelineLayout,
        m_pushConstantsShaderStageFlags,
        0,
        m_pushConstants.size(),
        m_pushConstants.data()
    );
}
void Pipeline::bindObjects(
    const shared_ptr<CommandBuffer> &commandBuffer,
    vk::PipelineBindPoint pipelineBindPoint)
{
    commandBuffer->bindPipeline(pipelineBindPoint, *m_pipeline);
    if (m_descriptorSet)
    {
        commandBuffer->storeData(
            m_memoryObjects,
            m_descriptorSet
        );
        commandBuffer->bindDescriptorSets(
            pipelineBindPoint,
            *m_pipelineLayout,
            0,
            {*m_descriptorSet},
            {}
        );
    }
}

void Pipeline::createDescriptorSetFromPool(const shared_ptr<DescriptorPool> &descriptorPool)
{
    m_descriptorSet.reset();
    if (descriptorPool)
    {
        m_descriptorSet = DescriptorSet::create(descriptorPool);
        m_mustUpdateDescriptorInfos = true;
    }
}
void Pipeline::setMemoryObjects(const MemoryObjectDescrs &memoryObjects)
{
    if (m_memoryObjects == memoryObjects)
        return;

    m_mustUpdateDescriptorInfos = true;
    m_memoryObjects = memoryObjects;
}

void Pipeline::prepare()
{
    const auto descriptorTypes = m_memoryObjects.fetchDescriptorTypes();
    bool descriptorSetLayoutFromDescriptorSet = false;

    if (m_descriptorSet)
    {
        auto descriptorSetLayout = m_descriptorSet->descriptorPool()->descriptorSetLayout();
        descriptorSetLayoutFromDescriptorSet = (descriptorSetLayout == m_descriptorSetLayout);
        if (descriptorSetLayout->descriptorTypes() != descriptorTypes)
        {
            if (descriptorSetLayoutFromDescriptorSet)
                m_descriptorSetLayout.reset();
            m_descriptorSet.reset();
        }
    }

    if (!descriptorSetLayoutFromDescriptorSet)
    {
        if (m_descriptorSetLayout && m_descriptorSetLayout->descriptorTypes() != descriptorTypes)
            m_descriptorSetLayout.reset();
    }

    if (!m_descriptorSetLayout)
    {
        m_descriptorSetLayout = m_descriptorSet
            ? m_descriptorSet->descriptorPool()->descriptorSetLayout()
            : DescriptorSetLayout::create(m_device, descriptorTypes)
        ;
        m_mustRecreate = true;
    }

    if (!m_descriptorSetLayout->isEmpty())
    {
        if (!m_descriptorSet)
        {
            m_descriptorSet = DescriptorSet::create(DescriptorPool::create(m_descriptorSetLayout));
            m_mustUpdateDescriptorInfos = true;
        }
        if (m_mustUpdateDescriptorInfos)
        {
            m_mustUpdateDescriptorInfos = false;
            m_descriptorSet->updateDescriptorInfos(m_memoryObjects.fetchDescriptorInfos());
        }
    }

    if (m_mustRecreate)
    {
        vk::PushConstantRange pushConstantRange;
        pushConstantRange.stageFlags = m_pushConstantsShaderStageFlags;
        pushConstantRange.offset = 0;
        pushConstantRange.size = m_pushConstants.size();

        const auto maxPushConstantsSize = m_device->physicalDevice()->limits().maxPushConstantsSize;
        if (m_pushConstants.size() > m_device->physicalDevice()->limits().maxPushConstantsSize)
            throw vk::LogicError("Push constants size exceeded: " + to_string(m_pushConstants.size()) + " > " + to_string(maxPushConstantsSize));

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        if (!m_descriptorSetLayout->isEmpty())
        {
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = *m_descriptorSetLayout;
        }
        if (pushConstantRange.size > 0)
        {
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        }
        m_pipelineLayout = m_device->createPipelineLayoutUnique(pipelineLayoutInfo);

        createPipeline();
        m_mustRecreate = false;
    }
}

void Pipeline::prepareObjects(
    const shared_ptr<CommandBuffer> &commandBuffer,
    const MemoryObjectDescrs &memoryObjects)
{
    memoryObjects.prepareObjects(*commandBuffer, m_objectsPipelineStageFlags);
}
void Pipeline::prepareObjects(
    const shared_ptr<CommandBuffer> &commandBuffer)
{
    prepareObjects(commandBuffer, m_memoryObjects);
}

void Pipeline::finalizeObjects(
    const shared_ptr<CommandBuffer> &commandBuffer,
    const MemoryObjectDescrs &memoryObjects)
{
    memoryObjects.finalizeObjects(*commandBuffer);
}
void Pipeline::finalizeObjects(
    const shared_ptr<CommandBuffer> &commandBuffer)
{
    finalizeObjects(commandBuffer, m_memoryObjects);
}

}
