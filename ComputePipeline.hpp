// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2025 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include "Pipeline.hpp"

namespace QmVk {

using namespace std;

class ShaderModule;

class QMVK_EXPORT ComputePipeline final : public Pipeline
{
public:
    static shared_ptr<ComputePipeline> create(
        const shared_ptr<Device> &device,
        const shared_ptr<ShaderModule> &shaderModule,
        uint32_t pushConstantsSize = 0,
        bool dispatchBase = false
    );

public:
    ComputePipeline(
        const shared_ptr<Device> &device,
        const shared_ptr<ShaderModule> &shaderModule,
        uint32_t pushConstantsSize,
        bool dispatchBase
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
    void recordCommandsCompute(
        const shared_ptr<CommandBuffer> &commandBuffer,
        const vk::Offset2D &baseGroup,
        const vk::Extent2D &groupCount
    );

    void recordCommands(
        const shared_ptr<CommandBuffer> &commandBuffer,
        const vk::Extent2D groupCount,
        bool doFinalizeObjects = false
    );
    void recordCommands(
        const shared_ptr<CommandBuffer> &commandBuffer,
        const vk::Offset2D &baseGroup,
        const vk::Extent2D groupCount,
        bool doFinalizeObjects = false
    );

private:
    const shared_ptr<ShaderModule> m_shaderModule;
    const bool m_dispatchBase = false;

    vk::Extent2D m_localWorkgroupSize;
};

/* Inline implementation */

vk::Extent2D ComputePipeline::localWorkGroupSize() const
{
    return m_localWorkgroupSize;
}

}
