// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#include "MemoryObjectBase.hpp"
#include "Device.hpp"

namespace QmVk {

QmVk::MemoryObjectBase::MemoryObjectBase(const shared_ptr<Device> &device)
    : m_device(device)
    , m_dld(m_device->dld())
{
}

}
