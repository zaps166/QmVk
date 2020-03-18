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

#include "ComputePipeline.hpp"
#include "PhysicalDevice.hpp"
#include "Device.hpp"
#include "DescriptorSet.hpp"
#include "ShaderModule.hpp"
#include "CommandBuffer.hpp"

#include <cmath>

namespace QmVk {

shared_ptr<ComputePipeline> ComputePipeline::create(
    const shared_ptr<Device> &device,
    const shared_ptr<ShaderModule> &shaderModule,
    uint32_t pushConstantsSize)
{
    auto computePipeline = make_shared<ComputePipeline>(
        device,
        shaderModule,
        pushConstantsSize,
        Priv()
    );
    return computePipeline;
}

ComputePipeline::ComputePipeline(
    const shared_ptr<Device> &device,
    const shared_ptr<ShaderModule> &shaderModule,
    uint32_t pushConstantsSize,
    Priv)
    : Pipeline(device, vk::PipelineStageFlagBits::eComputeShader, pushConstantsSize)
    , m_shaderModule(shaderModule)
{}
ComputePipeline::~ComputePipeline()
{}

void ComputePipeline::createPipeline()
{
    if (m_localWorkgroupSize.width == 0 || m_localWorkgroupSize.height == 0)
        m_localWorkgroupSize = m_device->physicalDevice()->localWorkgroupSize();

    vector<vk::SpecializationMapEntry> specializationMapEntries;
    vector<uint32_t> specializationData {
        m_localWorkgroupSize.width,
        m_localWorkgroupSize.height,
        1,
    };
    vk::SpecializationInfo specializationInfo = getSpecializationInfo(
        vk::ShaderStageFlagBits::eCompute,
        specializationMapEntries,
        specializationData
    );

    vk::ComputePipelineCreateInfo pipelineCreateInfo;
    pipelineCreateInfo.stage = m_shaderModule->getPipelineShaderStageCreateInfo(specializationInfo);
    pipelineCreateInfo.layout = *m_pipelineLayout;
    m_pipeline = m_device->createComputePipelineUnique(nullptr, pipelineCreateInfo);
}

void ComputePipeline::setCustomSpecializationData(const vector<uint32_t> &data)
{
    Pipeline::setCustomSpecializationData(data, vk::ShaderStageFlagBits::eCompute);
}

bool ComputePipeline::setLocalWorkgroupSize(const vk::Extent2D &localWorkgroupSize)
{
    vk::Extent2D newLocalWorkgroupSize;

    if (localWorkgroupSize.width > 0 && localWorkgroupSize.height > 0)
    {
        const auto &limits = m_device->physicalDevice()->limits();

        if (localWorkgroupSize.width > limits.maxComputeWorkGroupSize[0])
            return false;
        if (localWorkgroupSize.height > limits.maxComputeWorkGroupSize[1])
            return false;

        if (localWorkgroupSize.width * localWorkgroupSize.height > limits.maxComputeWorkGroupInvocations)
            return false;

        newLocalWorkgroupSize = localWorkgroupSize;
    }
    else
    {
        newLocalWorkgroupSize = m_device->physicalDevice()->localWorkgroupSize();
    }

    if (m_localWorkgroupSize == newLocalWorkgroupSize)
        return true;

    m_localWorkgroupSize = newLocalWorkgroupSize;
    m_mustRecreate = true;
    return true;
}
void ComputePipeline::setSize(const vk::Extent2D &size)
{
    m_size = size;
}

void ComputePipeline::recordCommandsInit(const shared_ptr<CommandBuffer> &commandBuffer)
{
    prepareImages(commandBuffer);
    bindObjects(commandBuffer, vk::PipelineBindPoint::eCompute);
}
void ComputePipeline::recordCommandsCompute(const shared_ptr<CommandBuffer> &commandBuffer)
{
    pushConstants(commandBuffer);
    commandBuffer->dispatch(
        ceil(static_cast<double>(m_size.width)  / static_cast<double>(m_localWorkgroupSize.width)),
        ceil(static_cast<double>(m_size.height) / static_cast<double>(m_localWorkgroupSize.height)),
        1
    );
}
void ComputePipeline::recordCommandsFinalize(const shared_ptr<CommandBuffer> &commandBuffer)
{
    finalizeImages(commandBuffer);
}

void ComputePipeline::recordCommands(
    const shared_ptr<CommandBuffer> &commandBuffer,
    bool finalizeImages)
{
    recordCommandsInit(commandBuffer);
    recordCommandsCompute(commandBuffer);
    if (finalizeImages)
        recordCommandsFinalize(commandBuffer);
}

}
