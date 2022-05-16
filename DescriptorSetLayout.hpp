// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2022 Błażej Szczygieł
*/

#pragma once

#include <vulkan/vulkan.hpp>

namespace QmVk {

using namespace std;

class Device;

class DescriptorSetLayout
{
    struct Priv {};

public:
    static shared_ptr<DescriptorSetLayout> create(
        const shared_ptr<Device> &device,
        const vector<vk::DescriptorPoolSize> &descriptorTypes
    );

public:
    DescriptorSetLayout(
        const shared_ptr<Device> &device,
        const vector<vk::DescriptorPoolSize> &descriptorTypes,
        Priv
    );
    ~DescriptorSetLayout();

private:
    void init();

public:
    inline shared_ptr<Device> device() const;

    inline bool isEmpty() const;
    inline const vector<vk::DescriptorPoolSize> &descriptorTypes() const;

public:
    inline operator const vk::DescriptorSetLayout *() const;

private:
    const shared_ptr<Device> m_device;
    const vector<vk::DescriptorPoolSize> m_descriptorTypes;

    vk::UniqueDescriptorSetLayout m_descriptorSetLayout;
};

/* Inline implementation */

shared_ptr<Device> DescriptorSetLayout::device() const
{
    return m_device;
}

bool DescriptorSetLayout::isEmpty() const
{
    return m_descriptorTypes.empty();
}
const vector<vk::DescriptorPoolSize> &DescriptorSetLayout::descriptorTypes() const
{
    return m_descriptorTypes;
}

DescriptorSetLayout::operator const vk::DescriptorSetLayout *() const
{
    return &*m_descriptorSetLayout;
}

}
