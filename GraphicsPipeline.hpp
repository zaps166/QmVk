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

#include "Pipeline.hpp"

namespace QmVk {

using namespace std;

class ShaderModule;
class RenderPass;
class CommandBuffer;

class GraphicsPipeline final : public Pipeline
{
    struct Priv {};

public:
    struct CreateInfo
    {
        // Required
        shared_ptr<Device> device;
        shared_ptr<ShaderModule> vertexShaderModule;
        shared_ptr<ShaderModule> fragmentShaderModule;
        shared_ptr<RenderPass> renderPass;
        vk::Extent2D size;
        // Optional
        uint32_t pushConstantsSize = 0;
        vector<vk::VertexInputBindingDescription> vertexBindingDescrs;
        vector<vk::VertexInputAttributeDescription> vertexAttrDescrs;
        vk::PipelineColorBlendAttachmentState *colorBlendAttachment = nullptr;
        vk::PipelineInputAssemblyStateCreateInfo *inputAssembly = nullptr;
        vk::PipelineRasterizationStateCreateInfo *rasterizer = nullptr;
    };

public:
    static shared_ptr<GraphicsPipeline> create(CreateInfo &createInfo);

public:
    GraphicsPipeline(CreateInfo &createInfo, Priv);
    ~GraphicsPipeline();

private:
    void createPipeline() override;

public:
    inline vk::Extent2D size() const;

    void setCustomSpecializationDataVertex(const vector<uint32_t> &data);
    void setCustomSpecializationDataFragment(const vector<uint32_t> &data);

    void recordCommands(const shared_ptr<CommandBuffer> &commandBuffer);

private:
    const shared_ptr<ShaderModule> m_vertexShaderModule;
    const shared_ptr<ShaderModule> m_fragmentShaderModule;
    const shared_ptr<RenderPass> m_renderPass;
    const vk::Extent2D m_size;
    const vector<vk::VertexInputBindingDescription> m_vertexBindingDescrs;
    const vector<vk::VertexInputAttributeDescription> m_vertexAttrDescrs;
    vk::PipelineColorBlendAttachmentState m_colorBlendAttachment;
    vk::PipelineInputAssemblyStateCreateInfo m_inputAssembly;
    vk::PipelineRasterizationStateCreateInfo m_rasterizer;
};

/* Inline implementation */

vk::Extent2D GraphicsPipeline::size() const
{
    return m_size;
}

}
