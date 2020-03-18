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

#include "GraphicsPipeline.hpp"
#include "PhysicalDevice.hpp"
#include "Device.hpp"
#include "DescriptorSet.hpp"
#include "ShaderModule.hpp"
#include "RenderPass.hpp"
#include "CommandBuffer.hpp"

namespace QmVk {

shared_ptr<GraphicsPipeline> GraphicsPipeline::create(CreateInfo &createInfo)
{
    auto graphicsPipeline = make_shared<GraphicsPipeline>(createInfo, Priv());
    return graphicsPipeline;
}

GraphicsPipeline::GraphicsPipeline(CreateInfo &createInfo, Priv)
    : Pipeline(createInfo.device, vk::PipelineStageFlagBits::eFragmentShader, createInfo.pushConstantsSize)
    , m_vertexShaderModule(move(createInfo.vertexShaderModule))
    , m_fragmentShaderModule(move(createInfo.fragmentShaderModule))
    , m_renderPass(move(createInfo.renderPass))
    , m_size(createInfo.size)
    , m_vertexBindingDescrs(move(createInfo.vertexBindingDescrs))
    , m_vertexAttrDescrs(move(createInfo.vertexAttrDescrs))
{}
GraphicsPipeline::~GraphicsPipeline()
{}

void GraphicsPipeline::createPipeline()
{
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleStrip;

    vk::Viewport viewport;
    viewport.width = m_size.width;
    viewport.height = m_size.height;

    vk::Rect2D scissor;
    scissor.extent = m_size;

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.cullMode = vk::CullModeFlagBits::eFront;
    rasterizer.lineWidth = 1.0f;

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.blendEnable = true;
    colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
    colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
    colorBlendAttachment.colorWriteMask = (vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    const vk::ShaderStageFlagBits specializationShaderStageFlagBits[2] {
        vk::ShaderStageFlagBits::eVertex,
        vk::ShaderStageFlagBits::eFragment,
    };
    vector<vk::SpecializationMapEntry> specializationMapEntries[2];
    vector<uint32_t> specializationData[2];
    vk::SpecializationInfo specializationInfo[2];
    for (uint32_t i = 0; i < 2; ++i)
    {
        specializationInfo[i] = getSpecializationInfo(
            specializationShaderStageFlagBits[i],
            specializationMapEntries[i],
            specializationData[i]
        );
    }

    vk::PipelineShaderStageCreateInfo shaderStages[] = {
        m_vertexShaderModule->getPipelineShaderStageCreateInfo(specializationInfo[0]),
        m_fragmentShaderModule->getPipelineShaderStageCreateInfo(specializationInfo[1]),
    };

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexBindingDescriptionCount = m_vertexBindingDescrs.size();
    vertexInputInfo.pVertexBindingDescriptions = m_vertexBindingDescrs.data();
    vertexInputInfo.vertexAttributeDescriptionCount = m_vertexAttrDescrs.size();
    vertexInputInfo.pVertexAttributeDescriptions = m_vertexAttrDescrs.data();

    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = *m_pipelineLayout;
    pipelineInfo.renderPass = *m_renderPass;
    m_pipeline = m_device->createGraphicsPipelineUnique(nullptr, pipelineInfo);
}

void GraphicsPipeline::setCustomSpecializationDataVertex(const vector<uint32_t> &data)
{
    setCustomSpecializationData(data, vk::ShaderStageFlagBits::eVertex);
}
void GraphicsPipeline::setCustomSpecializationDataFragment(const vector<uint32_t> &data)
{
    setCustomSpecializationData(data, vk::ShaderStageFlagBits::eFragment);
}

void GraphicsPipeline::recordCommands(const shared_ptr<CommandBuffer> &commandBuffer)
{
    pushConstants(commandBuffer);
    bindObjects(commandBuffer, vk::PipelineBindPoint::eGraphics);
}

}
