// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2025 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include <vulkan/vulkan.hpp>

namespace QmVk {

using namespace std;

class QMVK_EXPORT MemoryPropertyFlags
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
