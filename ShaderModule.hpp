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

#include <vulkan/vulkan.hpp>

namespace QmVk {

using namespace std;

class Device;

class QMVK_EXPORT ShaderModule
{
    struct Priv {};

public:
    static shared_ptr<ShaderModule> create(
        const shared_ptr<Device> &device,
        vk::ShaderStageFlagBits stage,
        const vector<uint32_t> &data
    );

public:
    ShaderModule(
        const shared_ptr<Device> &device,
        vk::ShaderStageFlagBits stage,
        Priv
    );
    ~ShaderModule();

private:
    void init(const vector<uint32_t> &data);

public:
    inline vk::ShaderStageFlagBits stage() const;

    vk::PipelineShaderStageCreateInfo getPipelineShaderStageCreateInfo(
        const vk::SpecializationInfo &specializationInfo
    ) const;

private:
    const shared_ptr<Device> m_device;
    const vk::ShaderStageFlagBits m_stage;

    vk::UniqueShaderModule m_shaderModule;
};

/* Inline implementation */

vk::ShaderStageFlagBits ShaderModule::stage() const
{
    return m_stage;
}

}
