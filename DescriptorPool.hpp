// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2022 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include <vulkan/vulkan.hpp>

namespace QmVk {

using namespace std;

class DescriptorSetLayout;
class Device;

class QMVK_EXPORT DescriptorPool
{
    struct Priv {};

public:
    static shared_ptr<DescriptorPool> create(
        const shared_ptr<DescriptorSetLayout> &descriptorSetLayout,
        uint32_t max = 1u
    );

public:
    DescriptorPool(
        const shared_ptr<DescriptorSetLayout> &descriptorSetLayout,
        uint32_t max,
        Priv
    );
    ~DescriptorPool();

private:
    void init();

public:
    inline shared_ptr<DescriptorSetLayout> descriptorSetLayout() const;
    inline uint32_t max() const;

public:
    inline operator vk::DescriptorPool() const;

private:
    const shared_ptr<DescriptorSetLayout> m_descriptorSetLayout;
    const uint32_t m_max;

    vk::UniqueDescriptorPool m_descriptorPool;
};

/* Inline implementation */

shared_ptr<DescriptorSetLayout> DescriptorPool::descriptorSetLayout() const
{
    return m_descriptorSetLayout;
}
uint32_t DescriptorPool::max() const
{
    return m_max;
}

DescriptorPool::operator vk::DescriptorPool() const
{
    return *m_descriptorPool;
}

}
