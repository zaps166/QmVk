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

class Device;

class MemoryObjectBase
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
