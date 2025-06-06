// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2025 Błażej Szczygieł
*/

#include "ShaderModule.hpp"
#include "Device.hpp"

namespace QmVk {

shared_ptr<ShaderModule> ShaderModule::create(
    const shared_ptr<Device> &device,
    vk::ShaderStageFlagBits stage,
    const vector<uint32_t> &data)
{
    auto shaderModuleCreateInfo = make_shared<ShaderModule>(
        device,
        stage
    );
    shaderModuleCreateInfo->init(data);
    return shaderModuleCreateInfo;
}

ShaderModule::ShaderModule(
    const shared_ptr<Device> &device,
    vk::ShaderStageFlagBits stage)
    : m_device(device)
    , m_stage(stage)
{}
ShaderModule::~ShaderModule()
{}

void ShaderModule::init(const vector<uint32_t> &data)
{
    vk::ShaderModuleCreateInfo createInfo;
    createInfo.codeSize = data.size() * sizeof(uint32_t);
    createInfo.pCode = data.data();

    m_shaderModule = m_device->createShaderModuleUnique(createInfo, nullptr, m_device->dld());
}

vk::PipelineShaderStageCreateInfo ShaderModule::getPipelineShaderStageCreateInfo(
    const vk::SpecializationInfo &specializationInfo) const
{
    return vk::PipelineShaderStageCreateInfo(
        vk::PipelineShaderStageCreateFlags(),
        m_stage,
        *m_shaderModule,
        "main",
        &specializationInfo
    );
}

}
