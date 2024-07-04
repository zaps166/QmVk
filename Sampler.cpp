// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#include "Sampler.hpp"
#include "Device.hpp"

namespace QmVk {

shared_ptr<Sampler> Sampler::create(
    const shared_ptr<Device> &device,
    const vk::SamplerCreateInfo &createInfo,
    const vk::SamplerYcbcrConversionCreateInfo &ycbcrCreateInfo)
{
    auto sampler = make_shared<Sampler>(
        device,
        createInfo,
        ycbcrCreateInfo,
        Priv()
    );
    sampler->init();
    return sampler;
}
shared_ptr<Sampler> Sampler::createClampToEdge(
    const shared_ptr<Device> &device,
    vk::Filter filter,
    const vk::SamplerYcbcrConversionCreateInfo &ycbcrCreateInfo)
{
    vk::SamplerCreateInfo createInfo;
    createInfo.magFilter = filter;
    createInfo.minFilter = filter;
    createInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    createInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    createInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    createInfo.mipmapMode = (filter == vk::Filter::eNearest)
        ? vk::SamplerMipmapMode::eNearest
        : vk::SamplerMipmapMode::eLinear
    ;
    createInfo.maxLod = numeric_limits<float>::max();

    auto sampler = make_shared<Sampler>(
        device,
        createInfo,
        ycbcrCreateInfo,
        Priv()
    );
    sampler->init();
    return sampler;
}

Sampler::Sampler(
    const shared_ptr<Device> &device,
    const vk::SamplerCreateInfo &createInfo,
    const vk::SamplerYcbcrConversionCreateInfo &ycbcrCreateInfo,
    Priv)
    : m_device(device)
    , m_createInfo(createInfo)
    , m_ycbcrCreateInfo(ycbcrCreateInfo)
{}
Sampler::~Sampler()
{}

void Sampler::init()
{
    vk::SamplerYcbcrConversionInfo samplerYcbcrInfo;

    if (m_ycbcrCreateInfo.format != vk::Format::eUndefined)
    {
        m_samplerYcbcr = m_device->createSamplerYcbcrConversionKHRUnique(m_ycbcrCreateInfo, nullptr, m_device->dld());

        samplerYcbcrInfo.pNext = m_createInfo.pNext;
        m_createInfo.pNext = &samplerYcbcrInfo;

        samplerYcbcrInfo.conversion = *m_samplerYcbcr;
    }

    m_sampler = m_device->createSamplerUnique(m_createInfo, nullptr, m_device->dld());

    m_createInfo.pNext = nullptr;
    m_ycbcrCreateInfo.pNext = nullptr;
}

}
