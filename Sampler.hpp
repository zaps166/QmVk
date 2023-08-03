// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2023 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include <vulkan/vulkan.hpp>

namespace QmVk {

using namespace std;

class Device;

class QMVK_EXPORT Sampler
{
    struct Priv {};

public:
    static shared_ptr<Sampler> create(
        const shared_ptr<Device> &device,
        const vk::SamplerCreateInfo &createInfo = {}
    );
    static shared_ptr<Sampler> createClampToEdge(
        const shared_ptr<Device> &device,
        vk::Filter filter = vk::Filter::eLinear
    );

public:
    Sampler(
        const shared_ptr<Device> &device,
        const vk::SamplerCreateInfo &createInfo,
        Priv
    );
    ~Sampler();

private:
    void init();

public:
    inline float maxLod() const;

    inline const vk::SamplerCreateInfo &createInfo() const;

public:
    inline operator vk::Sampler() const;

private:
    const shared_ptr<Device> m_device;
    const vk::SamplerCreateInfo m_createInfo;

    vk::UniqueSampler m_sampler;
};

/* Inline implementation */

float Sampler::maxLod() const
{
    return m_createInfo.maxLod;
}

const vk::SamplerCreateInfo &Sampler::createInfo() const
{
    return m_createInfo;
}

Sampler::operator vk::Sampler() const
{
    return *m_sampler;
}

}
