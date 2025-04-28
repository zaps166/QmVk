// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2025 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace QmVk {

using namespace std;

class Device;

class QMVK_EXPORT RenderPass
{
public:
    static shared_ptr<RenderPass> create(
        const shared_ptr<Device> &device,
        vk::Format format,
        vk::ImageLayout finalLayout,
        bool clear
    );

public:
    RenderPass(
        const shared_ptr<Device> &device,
        vk::Format format
    );
    ~RenderPass();

private:
    void init(vk::ImageLayout finalLayout, bool clear);

public:
    inline vk::Format format() const;

public:
    inline operator vk::RenderPass() const;

private:
    const shared_ptr<Device> m_device;
    const vk::Format m_format;

    vk::UniqueRenderPass m_renderPass;
};

/* Inline implementation */

vk::Format RenderPass::format() const
{
    return m_format;
}

RenderPass::operator vk::RenderPass() const
{
    return *m_renderPass;
}

}
