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

class QMVK_EXPORT Semaphore
{
    struct Priv {};

public:
    static shared_ptr<Semaphore> create(
        const shared_ptr<Device> &device
    );
    static shared_ptr<Semaphore> createExport(
        const shared_ptr<Device> &device,
        vk::ExternalSemaphoreHandleTypeFlagBits handleType
    );

public:
    Semaphore(
        const shared_ptr<Device> &device,
        vk::ExternalSemaphoreHandleTypeFlagBits *handleType,
        Priv
    );
    ~Semaphore();

private:
    void init();

public:
    inline shared_ptr<Device> device() const;

    int exportFD();

#ifdef VK_USE_PLATFORM_WIN32_KHR
    HANDLE exportWin32Handle();
#endif

public:
    inline operator const vk::Semaphore &() const;
    inline operator const vk::Semaphore *() const;

private:
    const shared_ptr<Device> m_device;
    const unique_ptr<vk::ExternalSemaphoreHandleTypeFlagBits> m_handleType;

    vk::UniqueSemaphore m_semaphore;
};

/* Inline implementation */

shared_ptr<Device> Semaphore::device() const
{
    return m_device;
}

Semaphore::operator const vk::Semaphore &() const
{
    return *m_semaphore;
}
Semaphore::operator const vk::Semaphore *() const
{
    return &*m_semaphore;
}

}
