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

#include "Pipeline.hpp"

namespace QmVk {

using namespace std;

class ShaderModule;

class QMVK_EXPORT ComputePipeline final : public Pipeline
{
    struct Priv {};

public:
    static shared_ptr<ComputePipeline> create(
        const shared_ptr<Device> &device,
        const shared_ptr<ShaderModule> &shaderModule,
        uint32_t pushConstantsSize = 0
    );

public:
    ComputePipeline(
        const shared_ptr<Device> &device,
        const shared_ptr<ShaderModule> &shaderModule,
        uint32_t pushConstantsSize,
        Priv
    );
    ~ComputePipeline();

private:
    void createPipeline() override;

public:
    void setCustomSpecializationData(const vector<uint32_t> &data);

    bool setLocalWorkgroupSize(const vk::Extent2D &localWorkgroupSize);

    inline vk::Extent2D localWorkGroupSize() const;
    vk::Extent2D groupCount(const vk::Extent2D &size) const;

    void recordCommandsInit(const shared_ptr<CommandBuffer> &commandBuffer);
    void recordCommandsCompute(
        const shared_ptr<CommandBuffer> &commandBuffer,
        const vk::Extent2D &groupCount
    );

    void recordCommands(
        const shared_ptr<CommandBuffer> &commandBuffer,
        const vk::Extent2D groupCount,
        bool doFinalizeImages = false
    );

private:
    const shared_ptr<ShaderModule> m_shaderModule;

    vk::Extent2D m_localWorkgroupSize;
};

/* Inline implementation */

vk::Extent2D ComputePipeline::localWorkGroupSize() const
{
    return m_localWorkgroupSize;
}

}
