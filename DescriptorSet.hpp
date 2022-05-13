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

#include "DescriptorSetLayout.hpp"

namespace QmVk {

using namespace std;

class DescriptorSetLayout;
class DescriptorInfo;
class DescriptorPool;
class Device;

class DescriptorSet
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
