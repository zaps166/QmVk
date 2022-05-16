// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2022 Błażej Szczygieł
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
    : Pipeline(createInfo.device, vk::ShaderStageFlagBits::eAllGraphics, vk::PipelineStageFlagBits::eFragmentShader, createInfo.pushConstantsSize)
    , m_vertexShaderModule(move(createInfo.vertexShaderModule))
    , m_fragmentShaderModule(move(createInfo.fragmentShaderModule))
    , m_renderPass(move(createInfo.renderPass))
    , m_size(createInfo.size)
    , m_vertexBindingDescrs(move(createInfo.vertexBindingDescrs))
    , m_vertexAttrDescrs(move(createInfo.vertexAttrDescrs))
{
    if (createInfo.colorBlendAttachment)
    {
        m_colorBlendAttachment = *createInfo.colorBlendAttachment;
    }
    else
    {
        m_colorBlendAttachment.blendEnable = true;
        m_colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        m_colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        m_colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
        m_colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        m_colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        m_colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
        m_colorBlendAttachment.colorWriteMask =
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA
        ;
    }

    if (createInfo.inputAssembly)
    {
        m_inputAssembly = *createInfo.inputAssembly;
    }
    else
    {
        m_inputAssembly.topology = vk::PrimitiveTopology::eTriangleStrip;
    }

    if (createInfo.rasterizer)
    {
        m_rasterizer = *createInfo.rasterizer;
    }
    else
    {
        m_rasterizer.cullMode = vk::CullModeFlagBits::eFront;
        m_rasterizer.lineWidth = 1.0f;
    }
}
GraphicsPipeline::~GraphicsPipeline()
{}

void GraphicsPipeline::createPipeline()
{
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

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &m_colorBlendAttachment;

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
    pipelineInfo.pInputAssemblyState = &m_inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &m_rasterizer;
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
