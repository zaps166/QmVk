/*
    QmVk - simple Vulkan library created for QMPlay2
    Copyright (C) 2020  Błażej Szczygieł

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

class QMVK_EXPORT DescriptorInfo
{
public:
    enum class Type
    {
        DescriptorBufferInfo,
        DescriptorImageInfo,
        BufferView,
    };

    inline DescriptorInfo(const vk::DescriptorBufferInfo &descrInfo)
        : type(Type::DescriptorBufferInfo)
        , descrBuffInfo(descrInfo)
    {}
    inline DescriptorInfo(const vk::DescriptorImageInfo &descrInfo)
        : type(Type::DescriptorImageInfo)
        , descrImgInfo(descrInfo)
    {}
    inline DescriptorInfo(vk::BufferView bufferView)
        : type(Type::BufferView)
        , bufferView(bufferView)
    {}

    Type type;

    vk::DescriptorBufferInfo descrBuffInfo;
    vk::DescriptorImageInfo descrImgInfo;
    vk::BufferView bufferView;
};

}
