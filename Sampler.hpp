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
    static shared_ptr<Sampler> createLinear(
        const shared_ptr<Device> &device
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

Sampler::operator vk::Sampler() const
{
    return *m_sampler;
}

}
