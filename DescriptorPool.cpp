// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2025 Błażej Szczygieł
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
        max
    );
    descriptorPool->init();
    return descriptorPool;
}

DescriptorPool::DescriptorPool(
    const shared_ptr<DescriptorSetLayout> &descriptorSetLayout,
    uint32_t max)
    : m_descriptorSetLayout(descriptorSetLayout)
    , m_max(max)
{
}
DescriptorPool::~DescriptorPool()
{
}

void DescriptorPool::init()
{
    const auto &descriptorTypes = m_descriptorSetLayout->descriptorTypes();
    auto device = m_descriptorSetLayout->device();

    vector<vk::DescriptorPoolSize> descriptorPoolSizes(
        descriptorTypes.begin(),
        descriptorTypes.end()
    );

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
    m_descriptorPool = device->createDescriptorPoolUnique(descriptorPoolCreateInfo, nullptr, device->dld());
}

}
