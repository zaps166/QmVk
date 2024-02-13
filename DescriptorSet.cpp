// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#include "DescriptorSet.hpp"
#include "DescriptorInfo.hpp"
#include "DescriptorPool.hpp"
#include "Device.hpp"

namespace QmVk {

shared_ptr<DescriptorSet> DescriptorSet::create(
    const shared_ptr<DescriptorPool> &descriptorPool)
{
    auto descriptor = make_shared<DescriptorSet>(
        descriptorPool,
        Priv()
    );
    descriptor->init();
    return descriptor;
}

DescriptorSet::DescriptorSet(
    const shared_ptr<DescriptorPool> &descriptorPool,
    Priv)
    : m_descriptorPool(descriptorPool)
{}
DescriptorSet::~DescriptorSet()
{}

void DescriptorSet::init()
{
    auto descriptorSetLayout = m_descriptorPool->descriptorSetLayout();

    if (descriptorSetLayout->isEmpty())
        return;

    auto device = descriptorSetLayout->device();

    vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.descriptorPool = *m_descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = *descriptorSetLayout;
    m_descriptorSet = move(device->allocateDescriptorSetsUnique(descriptorSetAllocateInfo)[0]);
}

void DescriptorSet::updateDescriptorInfos(const vector<DescriptorInfo> &descriptorInfos)
{
    auto descriptorSetLayout = m_descriptorPool->descriptorSetLayout();
    auto device = descriptorSetLayout->device();

    const auto &descriptorTypes = descriptorSetLayout->descriptorTypes();

    vector<vk::WriteDescriptorSet> writeDescriptorSets;
    writeDescriptorSets.resize(descriptorInfos.size());
    for (uint32_t t = 0, i = 0; t < descriptorTypes.size(); ++t)
    {
        const uint32_t arrSize = descriptorTypes[t].descriptorCount;
        for (uint32_t e = 0; e < arrSize; ++e, ++i)
        {
            vk::WriteDescriptorSet &writeDescriptorSet = writeDescriptorSets[i];
            writeDescriptorSet.dstSet = *m_descriptorSet;
            writeDescriptorSet.dstBinding = t;
            writeDescriptorSet.dstArrayElement = e;
            writeDescriptorSet.descriptorCount = 1;
            writeDescriptorSet.descriptorType = descriptorTypes[t].type;
            switch (descriptorInfos[i].type)
            {
                case DescriptorInfo::Type::DescriptorImageInfo:
                    writeDescriptorSet.pImageInfo = &descriptorInfos[i].descrImgInfo;
                    break;
                case DescriptorInfo::Type::DescriptorBufferInfo:
                    writeDescriptorSet.pBufferInfo = &descriptorInfos[i].descrBuffInfo;
                    break;
                case DescriptorInfo::Type::BufferView:
                    writeDescriptorSet.pTexelBufferView = &descriptorInfos[i].bufferView;
                    break;
            }
        }
    }

    device->updateDescriptorSets(writeDescriptorSets, nullptr);
}

}
