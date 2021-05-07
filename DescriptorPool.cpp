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

#include "DescriptorPool.hpp"
#include "DescriptorSetLayout.hpp"
#include "Device.hpp"

namespace QmVk {

shared_ptr<DescriptorPool> DescriptorPool::create(
    const shared_ptr<DescriptorSetLayout> &descriptorSetLayout,
    uint32_t max)
{
    auto descriptorPool = make_shared<DescriptorPool>(
        descriptorSetLayout,
        max,
        Priv()
    );
    descriptorPool->init();
    return descriptorPool;
}

DescriptorPool::DescriptorPool(
    const shared_ptr<DescriptorSetLayout> &descriptorSetLayout,
    uint32_t max,
    Priv)
    : m_descriptorSetLayout(descriptorSetLayout)
    , m_max(max)
{
}
DescriptorPool::~DescriptorPool()
{
}

void DescriptorPool::init()
{
    auto descriptorPoolSizes = m_descriptorSetLayout->descriptorTypes();
    auto device = m_descriptorSetLayout->device();

    if (m_max > 1)
    {
        for (auto &&descriptorPoolSize : descriptorPoolSizes)
            descriptorPoolSize.descriptorCount *= m_max;
    }

    vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
    descriptorPoolCreateInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    descriptorPoolCreateInfo.maxSets = m_max;
    descriptorPoolCreateInfo.poolSizeCount = descriptorPoolSizes.size();
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();
    m_descriptorPool = device->createDescriptorPoolUnique(descriptorPoolCreateInfo);
}

}
