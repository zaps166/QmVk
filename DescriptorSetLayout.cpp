// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2023 Błażej Szczygieł
*/

#include "DescriptorSetLayout.hpp"
#include "Device.hpp"

namespace QmVk {

shared_ptr<DescriptorSetLayout> DescriptorSetLayout::create(
    const shared_ptr<Device> &device,
    const vector<vk::DescriptorPoolSize> &descriptorTypes)
{
    auto descriptorSetLayout = make_shared<DescriptorSetLayout>(
        device,
        descriptorTypes,
        Priv()
    );
    descriptorSetLayout->init();
    return descriptorSetLayout;
}

DescriptorSetLayout::DescriptorSetLayout(
    const shared_ptr<Device> &device,
    const vector<vk::DescriptorPoolSize> &descriptorTypes,
    Priv)
    : m_device(device)
    , m_descriptorTypes(descriptorTypes)
{
}
DescriptorSetLayout::~DescriptorSetLayout()
{
}

void DescriptorSetLayout::init()
{
    vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings;
    descriptorSetLayoutBindings.reserve(m_descriptorTypes.size());
    for (uint32_t i = 0; i < m_descriptorTypes.size(); ++i)
    {
        vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding;
        descriptorSetLayoutBinding.binding = i;
        descriptorSetLayoutBinding.descriptorType = m_descriptorTypes[i].type;
        descriptorSetLayoutBinding.descriptorCount = m_descriptorTypes[i].descriptorCount;
        descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eAll;
        descriptorSetLayoutBindings.push_back(descriptorSetLayoutBinding);
    }
    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    descriptorSetLayoutCreateInfo.bindingCount = descriptorSetLayoutBindings.size();
    descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings.data();
    m_descriptorSetLayout = m_device->createDescriptorSetLayoutUnique(descriptorSetLayoutCreateInfo);
}

}
