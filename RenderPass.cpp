// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2023 Błażej Szczygieł
*/

#include "RenderPass.hpp"
#include "Device.hpp"

namespace QmVk {

shared_ptr<RenderPass> RenderPass::create(
    const shared_ptr<Device> &device,
    vk::Format format,
    vk::ImageLayout finalLayout,
    bool clear)
{
    auto renderPass = make_shared<RenderPass>(
        device,
        format,
        Priv()
    );
    renderPass->init(finalLayout, clear);
    return renderPass;
}

RenderPass::RenderPass(
    const shared_ptr<Device> &device,
    vk::Format format,
    Priv)
    : m_device(device)
    , m_format(format)
{}
RenderPass::~RenderPass()
{}

void RenderPass::init(vk::ImageLayout finalLayout, bool clear)
{
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = m_format;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = finalLayout;

    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    vk::RenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    m_renderPass = m_device->createRenderPassUnique(renderPassCreateInfo);
}

}
