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
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
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
