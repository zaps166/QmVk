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

class QMVK_EXPORT RenderPass
{
    struct Priv {};

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
        vk::Format format,
        Priv
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
