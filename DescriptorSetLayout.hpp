// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2025 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include "DescriptorType.hpp"

#include <memory>

namespace QmVk {

using namespace std;

class Device;

class QMVK_EXPORT DescriptorSetLayout
{
public:
    static shared_ptr<DescriptorSetLayout> create(
        const shared_ptr<Device> &device,
        const vector<DescriptorType> &descriptorTypes
    );

public:
    DescriptorSetLayout(
        const shared_ptr<Device> &device,
        const vector<DescriptorType> &descriptorTypes
    );
    ~DescriptorSetLayout();

private:
    void init();

public:
    inline shared_ptr<Device> device() const;

    inline bool isEmpty() const;
    inline const vector<DescriptorType> &descriptorTypes() const;

public:
    inline operator const vk::DescriptorSetLayout *() const;

private:
    const shared_ptr<Device> m_device;
    const vector<DescriptorType> m_descriptorTypes;

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
const vector<DescriptorType> &DescriptorSetLayout::descriptorTypes() const
{
    return m_descriptorTypes;
}

DescriptorSetLayout::operator const vk::DescriptorSetLayout *() const
{
    return &*m_descriptorSetLayout;
}

}
