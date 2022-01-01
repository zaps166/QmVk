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

class Device;

class QMVK_EXPORT DescriptorSetLayout
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
