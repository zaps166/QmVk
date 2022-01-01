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

#include "QmVkExport.hpp"

#include <vulkan/vulkan.hpp>

namespace QmVk {

using namespace std;

class Device;
class Queue;
class RenderPass;
class Semaphore;

class QMVK_EXPORT SwapChain
{
    struct Priv {};

public:
    struct CreateInfo
    {
        shared_ptr<Device> device;
        shared_ptr<Queue> queue;
        shared_ptr<RenderPass> renderPass;
        vk::SurfaceKHR surface;
        vk::Extent2D fallbackSize;
        vector<vk::PresentModeKHR> presentModes;
        vk::UniqueSwapchainKHR oldSwapChain;
        uint32_t imageCount = 0;
#ifdef VK_USE_PLATFORM_WIN32_KHR
        vk::FullScreenExclusiveEXT exclusiveFullScreen = vk::FullScreenExclusiveEXT::eDefault;
#endif
    };

public:
    static vk::Format getSurfaceFormat(
        const vector<vk::SurfaceFormatKHR> &surfaceFormats,
        const vector<vk::Format> &wantedFormats,
        vk::ColorSpaceKHR colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear
    );

public:
    static shared_ptr<SwapChain> create(CreateInfo &createInfo);

public:
    SwapChain(CreateInfo &createInfo, Priv);
    ~SwapChain();

private:
    void init(CreateInfo &createInfo);

public:
    inline vk::Extent2D size() const;

    inline bool maybeSuboptimal() const;

    inline vk::Framebuffer frameBuffer(uint32_t idx) const;

    vk::SubmitInfo getSubmitInfo() const;

    uint32_t acquireNextImage(bool *suboptimal = nullptr);
    void present(uint32_t imageIdx, bool *suboptimal = nullptr);

    vk::UniqueSwapchainKHR take();

private:
    const shared_ptr<Device> m_device;
    const shared_ptr<Queue> m_queue;
    const shared_ptr<RenderPass> m_renderPass;
    const vk::SurfaceKHR m_surface;
    vk::UniqueSwapchainKHR m_oldSwapChain;

    vk::Extent2D m_size;

    bool m_maybeSuboptimal = false;

    vk::UniqueSwapchainKHR m_swapChain;

    vector<vk::UniqueImageView> m_swapChainImageViews;
    vector<vk::UniqueFramebuffer> m_frameBuffers;

    shared_ptr<Semaphore> m_imageAvailableSem;
    shared_ptr<Semaphore> m_renderFinishedSem;
};

/* Inline implementation */

vk::Extent2D SwapChain::size() const
{
    return m_size;
}

bool SwapChain::maybeSuboptimal() const
{
    return m_maybeSuboptimal;
}

vk::Framebuffer SwapChain::frameBuffer(uint32_t idx) const
{
    return *m_frameBuffers[idx];
}

}
