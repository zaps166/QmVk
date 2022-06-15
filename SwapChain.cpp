// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2022 Błażej Szczygieł
*/

#include "SwapChain.hpp"
#include "PhysicalDevice.hpp"
#include "Device.hpp"
#include "Queue.hpp"
#include "RenderPass.hpp"
#include "Semaphore.hpp"

namespace QmVk {

static constexpr vk::PipelineStageFlags g_waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

vk::Format SwapChain::getSurfaceFormat(
    const vector<vk::SurfaceFormatKHR> &surfaceFormats,
    const vector<vk::Format> &wantedFormats,
    vk::ColorSpaceKHR colorSpace)
{
    if (surfaceFormats.empty())
        return vk::Format::eUndefined;

    auto isFormatSupported = [&](vk::Format format) {
        return find_if(surfaceFormats.begin(), surfaceFormats.end(), [&](const vk::SurfaceFormatKHR &surfaceFormat) {
            return (surfaceFormat.colorSpace == colorSpace && surfaceFormat.format == format);
        }) != surfaceFormats.end();
    };

    for (auto &&wantedFormat : wantedFormats)
    {
        if (isFormatSupported(wantedFormat))
            return wantedFormat;
    }

    return surfaceFormats[0].format;
}

shared_ptr<SwapChain> SwapChain::create(CreateInfo &createInfo)
{
    auto swapChain = make_shared<SwapChain>(createInfo, Priv());
    swapChain->init(createInfo);
    return swapChain;
}

SwapChain::SwapChain(CreateInfo &createInfo, Priv)
    : m_device(move(createInfo.device))
    , m_queue(move(createInfo.queue))
    , m_renderPass(move(createInfo.renderPass))
    , m_surface(move(createInfo.surface))
    , m_oldSwapChain(move(createInfo.oldSwapChain))
{}
SwapChain::~SwapChain()
{}

void SwapChain::init(CreateInfo &createInfo)
{
    const auto physicalDevice = m_device->physicalDevice();

    const auto surfaceCapabilities = physicalDevice->getSurfaceCapabilitiesKHR(m_surface);
    const auto availPresentModes = physicalDevice->getSurfacePresentModesKHR(m_surface);

    m_size = (surfaceCapabilities.currentExtent.width == numeric_limits<uint32_t>::max())
        ? createInfo.fallbackSize
        : surfaceCapabilities.currentExtent
    ;

    auto presentMode = vk::PresentModeKHR::eFifo;
    for (auto &&presentModeIt : createInfo.presentModes)
    {
        if (find(availPresentModes.begin(), availPresentModes.end(), presentModeIt) != availPresentModes.end())
        {
            presentMode = presentModeIt;
            break;
        }
    }

#ifdef VK_USE_PLATFORM_WIN32_KHR
    const bool exclusiveFullScreen =
        createInfo.exclusiveFullScreen != vk::FullScreenExclusiveEXT::eDefault &&
        m_device->enabledExtensions().count(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME) > 0
    ;
#endif

    if (createInfo.imageCount == 0 && presentMode == vk::PresentModeKHR::eMailbox)
        createInfo.imageCount = 3;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    else if (exclusiveFullScreen && createInfo.imageCount == 0 && surfaceCapabilities.minImageCount == 1)
        createInfo.imageCount = 2;
#endif
    createInfo.imageCount = max(createInfo.imageCount, surfaceCapabilities.minImageCount);
    if (surfaceCapabilities.maxImageCount > 0)
        createInfo.imageCount = min(createInfo.imageCount, surfaceCapabilities.maxImageCount);

    vk::CompositeAlphaFlagBitsKHR compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    if (!(surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque))
    {
        if (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
            compositeAlpha = vk::CompositeAlphaFlagBitsKHR::ePreMultiplied;
        else if (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied)
            compositeAlpha = vk::CompositeAlphaFlagBitsKHR::ePostMultiplied;
        else if (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit)
            compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eInherit;
    }

    vk::SurfaceTransformFlagBitsKHR preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    if (!(surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity))
        preTransform = surfaceCapabilities.currentTransform;
    else if (preTransform != surfaceCapabilities.currentTransform)
        m_maybeSuboptimal = true;

    vk::SwapchainCreateInfoKHR vkCreateInfo;
    vkCreateInfo.surface = m_surface;
    vkCreateInfo.minImageCount = createInfo.imageCount;
    vkCreateInfo.imageFormat = m_renderPass->format();
    vkCreateInfo.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    vkCreateInfo.imageExtent = m_size;
    vkCreateInfo.imageArrayLayers = 1;
    vkCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    vkCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
    vkCreateInfo.preTransform = preTransform;
    vkCreateInfo.compositeAlpha = compositeAlpha;
    vkCreateInfo.presentMode = presentMode;
    vkCreateInfo.clipped = true;
    vkCreateInfo.oldSwapchain = *m_oldSwapChain;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    vk::SurfaceFullScreenExclusiveInfoEXT surfaceFullScreenExclusiveInfo;
    if (exclusiveFullScreen)
    {
        surfaceFullScreenExclusiveInfo.fullScreenExclusive = createInfo.exclusiveFullScreen;
        vkCreateInfo.pNext = &surfaceFullScreenExclusiveInfo;
    }
#endif
    m_swapChain = m_device->createSwapchainKHRUnique(vkCreateInfo);

    m_oldSwapChain.reset();

    const auto swapChainImages = m_device->getSwapchainImagesKHR(*m_swapChain);
    for (auto &&swapChainImage : swapChainImages)
    {
        vk::ImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo.image = swapChainImage;
        imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
        imageViewCreateInfo.format = m_renderPass->format();
        imageViewCreateInfo.components = vk::ComponentMapping();
        imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        m_swapChainImageViews.push_back(m_device->createImageViewUnique(imageViewCreateInfo));

        vk::FramebufferCreateInfo framebufferCreateInfo;
        framebufferCreateInfo.renderPass = *m_renderPass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = &m_swapChainImageViews.back().get();
        framebufferCreateInfo.width = m_size.width;
        framebufferCreateInfo.height = m_size.height;
        framebufferCreateInfo.layers = 1;
        m_frameBuffers.push_back(m_device->createFramebufferUnique(framebufferCreateInfo));
    }

    m_imageAvailableSem = Semaphore::create(m_device);
    m_renderFinishedSem = Semaphore::create(m_device);
}

vk::SubmitInfo SwapChain::getSubmitInfo() const
{
    vk::SubmitInfo submitInfo;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = *m_imageAvailableSem;
    submitInfo.pWaitDstStageMask = &g_waitStage;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = *m_renderFinishedSem;
    return submitInfo;
}

uint32_t SwapChain::acquireNextImage(bool *suboptimal)
{
    const auto nextImageResult = m_device->acquireNextImageKHR(
        *m_swapChain,
#ifdef QMVK_WAIT_TIMEOUT_MS
        QMVK_WAIT_TIMEOUT_MS * static_cast<uint64_t>(1e6),
#else
        numeric_limits<uint64_t>::max(),
#endif
        *m_imageAvailableSem,
        vk::Fence()
    );
    if (nextImageResult.result == vk::Result::eSuboptimalKHR)
    {
        if (suboptimal)
            *suboptimal = true;
    }
    else if (nextImageResult.result == vk::Result::eTimeout)
    {
        throw vk::SystemError(vk::make_error_code(nextImageResult.result), "vkAcquireNextImageKHR");
    }
    return nextImageResult.value;
}
void SwapChain::present(uint32_t imageIdx, bool *suboptimal)
{
    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = *m_renderFinishedSem;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &*m_swapChain;
    presentInfo.pImageIndices = &imageIdx;
    if (m_queue->presentKHR(presentInfo) == vk::Result::eSuboptimalKHR)
    {
        if (suboptimal)
            *suboptimal = true;
    }
}

vk::UniqueSwapchainKHR SwapChain::take()
{
    return move(m_swapChain);
}

}
