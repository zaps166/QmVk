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

#include "Pipeline.hpp"
#include "Device.hpp"
#include "MemoryObjectDescr.hpp"
#include "DescriptorPool.hpp"
#include "DescriptorSet.hpp"
#include "DescriptorInfo.hpp"
#include "CommandBuffer.hpp"

namespace QmVk {

Pipeline::Pipeline(
    const shared_ptr<Device> &device,
    const vk::PipelineStageFlags &imagePipelineStageFlags,
    uint32_t pushConstantsSize)
    : m_device(device)
    , m_imagePipelineStageFlags(imagePipelineStageFlags)
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
        vk::ShaderStageFlagBits::eAll,
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
        m_mustRecreate = false;

        vk::PushConstantRange pushConstantRange;
        pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eAll;
        pushConstantRange.offset = 0;
        pushConstantRange.size = m_pushConstants.size();

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
    }
}

void Pipeline::prepareImages(
    const shared_ptr<CommandBuffer> &commandBuffer,
    const MemoryObjectDescrs &memoryObjects)
{
    memoryObjects.prepareImages(*commandBuffer, m_imagePipelineStageFlags);
}
void Pipeline::prepareImages(
    const shared_ptr<CommandBuffer> &commandBuffer)
{
    prepareImages(commandBuffer, m_memoryObjects);
}

void Pipeline::finalizeImages(
    const shared_ptr<CommandBuffer> &commandBuffer,
    const MemoryObjectDescrs &memoryObjects)
{
    memoryObjects.finalizeImages(*commandBuffer);
}
void Pipeline::finalizeImages(
    const shared_ptr<CommandBuffer> &commandBuffer)
{
    finalizeImages(commandBuffer, m_memoryObjects);
}

}
