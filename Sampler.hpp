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

class QMVK_EXPORT Sampler
{
public:
    static shared_ptr<Sampler> create(
        const shared_ptr<Device> &device,
        const vk::SamplerCreateInfo &createInfo = {},
        const vk::SamplerYcbcrConversionCreateInfo &ycbcrCreateInfo = {}
    );
    static shared_ptr<Sampler> createClampToEdge(
        const shared_ptr<Device> &device,
        vk::Filter filter = vk::Filter::eLinear,
        const vk::SamplerYcbcrConversionCreateInfo &ycbcrCreateInfo = {}
    );

public:
    Sampler(
        const shared_ptr<Device> &device,
        const vk::SamplerCreateInfo &createInfo,
        const vk::SamplerYcbcrConversionCreateInfo &ycbcrCreateInfo
    );
    ~Sampler();

private:
    void init();

public:
    inline shared_ptr<Device> device() const;

    inline vk::SamplerYcbcrConversion samplerYcbcr() const;

    inline const vk::SamplerCreateInfo &createInfo() const;
    inline const vk::SamplerYcbcrConversionCreateInfo &ycbcrCreateInfo() const;

public:
    inline operator vk::Sampler() const;

private:
    const shared_ptr<Device> m_device;

    vk::SamplerCreateInfo m_createInfo;
    vk::SamplerYcbcrConversionCreateInfo m_ycbcrCreateInfo;

    vk::UniqueSampler m_sampler;
    vk::UniqueSamplerYcbcrConversion m_samplerYcbcr;
};

/* Inline implementation */

shared_ptr<Device> Sampler::device() const
{
    return m_device;
}

vk::SamplerYcbcrConversion Sampler::samplerYcbcr() const
{
    return *m_samplerYcbcr;
}

const vk::SamplerCreateInfo &Sampler::createInfo() const
{
    return m_createInfo;
}
const vk::SamplerYcbcrConversionCreateInfo &Sampler::ycbcrCreateInfo() const
{
    return m_ycbcrCreateInfo;
}

Sampler::operator vk::Sampler() const
{
    return *m_sampler;
}

}
