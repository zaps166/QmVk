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

#include <vulkan/vulkan.hpp>

namespace QmVk {

using namespace std;

class Device;

class Sampler
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
