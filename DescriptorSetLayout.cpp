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
