/*
    QmVk - simple Vulkan library created for QMPlay2
    Copyright (C) 2020-2021  Błażej Szczygieł

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
        stage,
        Priv()
    );
    shaderModuleCreateInfo->init(data);
    return shaderModuleCreateInfo;
}

ShaderModule::ShaderModule(
    const shared_ptr<Device> &device,
    vk::ShaderStageFlagBits stage,
    Priv)
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

    m_shaderModule = m_device->createShaderModuleUnique(createInfo);
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
