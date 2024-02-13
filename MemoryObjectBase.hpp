// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#pragma once

#include "QmVkExport.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace QmVk {

using namespace std;

class Device;

class QMVK_EXPORT MemoryObjectBase
{
public:
    template<typename T>
    static inline T aligned(const T value, const T alignment);

public:
    class CustomData
    {
    public:
        virtual ~CustomData() = default;
    };

protected:
    inline MemoryObjectBase(const shared_ptr<Device> &device);
    virtual ~MemoryObjectBase() = default;

public:
    inline shared_ptr<Device> device() const;

    template<typename T>
    inline T *customData();
    inline void setCustomData(unique_ptr<CustomData> &&customData);

protected:
    const shared_ptr<Device> m_device;

    unique_ptr<CustomData> m_customData;
};

/* Inline implementation */

template<typename T>
T MemoryObjectBase::aligned(const T value, const T alignment)
{
    return (value + alignment - 1 ) & ~(alignment - 1);
}

MemoryObjectBase::MemoryObjectBase(const shared_ptr<Device> &device)
    : m_device(device)
{
}

shared_ptr<Device> MemoryObjectBase::device() const
{
    return m_device;
}

template<typename T>
T *MemoryObjectBase::customData()
{
    return reinterpret_cast<T *>(m_customData.get());
}
void MemoryObjectBase::setCustomData(unique_ptr<CustomData> &&customData)
{
    m_customData = move(customData);
}

}
