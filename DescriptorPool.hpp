/*
    QmVk - simple Vulkan library created for QMPlay2
    Copyright (C) 2020-2022  Błażej Szczygieł

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
