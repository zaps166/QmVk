// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include "DescriptorSetLayout.hpp"

namespace QmVk {

using namespace std;

class DescriptorSetLayout;
class DescriptorInfo;
class DescriptorPool;
class Device;

class QMVK_EXPORT DescriptorSet
{
    struct Priv {};

public:
    static shared_ptr<DescriptorSet> create(
        const shared_ptr<DescriptorPool> &descriptorPool
    );

public:
    DescriptorSet(
        const shared_ptr<DescriptorPool> &descriptorPool,
        Priv
    );
    ~DescriptorSet();

private:
    void init();

public:
    inline shared_ptr<DescriptorPool> descriptorPool() const;

    void updateDescriptorInfos(const vector<DescriptorInfo> &descriptorInfos);

public:
    inline operator vk::DescriptorSet() const;

private:
    const shared_ptr<DescriptorPool> m_descriptorPool;

    vk::UniqueDescriptorSet m_descriptorSet;
};

/* Inline implementation */

shared_ptr<DescriptorPool> DescriptorSet::descriptorPool() const
{
    return m_descriptorPool;
}

DescriptorSet::operator vk::DescriptorSet() const
{
    return *m_descriptorSet;
}

}
