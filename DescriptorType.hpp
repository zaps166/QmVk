// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2026 Błażej Szczygieł
*/

#pragma once

#include "QmVkCommon.hpp"

namespace QmVk {

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
