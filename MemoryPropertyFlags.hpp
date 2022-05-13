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

#include <vulkan/vulkan.hpp>

namespace QmVk {

using namespace std;

class MemoryPropertyFlags
{
public:
    MemoryPropertyFlags() = default;
    inline MemoryPropertyFlags(
        vk::MemoryPropertyFlagBits requiredFlag
    );
    inline MemoryPropertyFlags(
        vk::MemoryPropertyFlags requiredFlags
    );
    ~MemoryPropertyFlags() = default;

public:
    vk::MemoryPropertyFlags required;
    vk::MemoryPropertyFlags optional;
    vk::MemoryPropertyFlags optionalFallback;
    vk::MemoryPropertyFlags notWanted;
    uint32_t heap = ~0;
};

/* Inline implementation */

MemoryPropertyFlags::MemoryPropertyFlags(
        vk::MemoryPropertyFlagBits requiredFlag)
    : required(requiredFlag)
{}

MemoryPropertyFlags::MemoryPropertyFlags(
        vk::MemoryPropertyFlags requiredFlags)
    : required(requiredFlags)
{}

}
