/*
    QmVk - simple Vulkan library created for QMPlay2
    Copyright (C) 2020-2021  Błażej Szczygieł

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
shared_ptr<Sampler> Sampler::createLinear(
    const shared_ptr<Device> &device)
{
    vk::SamplerCreateInfo createInfo;
    createInfo.magFilter = vk::Filter::eLinear;
    createInfo.minFilter = vk::Filter::eLinear;
    createInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    createInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    createInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    createInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
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
