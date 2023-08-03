// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2023 Błażej Szczygieł
*/

#include "Sampler.hpp"
#include "Device.hpp"

namespace QmVk {

shared_ptr<Sampler> Sampler::create(
    const shared_ptr<Device> &device,
    const vk::SamplerCreateInfo &createInfo)
{
    auto sampler = make_shared<Sampler>(
        device,
        createInfo,
        Priv()
    );
    sampler->init();
    return sampler;
}
shared_ptr<Sampler> Sampler::createClampToEdge(
    const shared_ptr<Device> &device,
    vk::Filter filter)
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
        Priv()
    );
    sampler->init();
    return sampler;
}

Sampler::Sampler(
    const shared_ptr<Device> &device,
    const vk::SamplerCreateInfo &createInfo,
    Priv)
    : m_device(device)
    , m_createInfo(createInfo)
{}
Sampler::~Sampler()
{}

void Sampler::init()
{
    m_sampler = m_device->createSamplerUnique(m_createInfo);
}

}
