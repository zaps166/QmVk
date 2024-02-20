// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#pragma once

#include <vulkan/vulkan.hpp>

namespace QmVk {

using namespace std;

class DescriptorType : public vk::DescriptorPoolSize
{
#ifndef QMVK_NO_GRAPHICS
public:
    bool operator ==(const DescriptorType &other) const
    {
        return
               type == other.type
            && descriptorCount == other.descriptorCount
            && immutableSamplers == other.immutableSamplers
        ;
    }
    bool operator !=(const DescriptorType &other) const
    {
        return !this->operator==(other);
    }

public:
    vector<vk::Sampler> immutableSamplers;
#endif
};

}
