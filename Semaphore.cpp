// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#include "Semaphore.hpp"
#include "Device.hpp"

namespace QmVk {

shared_ptr<Semaphore> Semaphore::create(
    const shared_ptr<Device> &device)
{
    auto semaphore = make_shared<Semaphore>(
        device,
        nullptr,
        Priv()
    );
    semaphore->init();
    return semaphore;
}
shared_ptr<Semaphore> Semaphore::createExport(
    const shared_ptr<Device> &device,
    vk::ExternalSemaphoreHandleTypeFlagBits handleType)
{
    auto semaphore = make_shared<Semaphore>(
        device,
        &handleType,
        Priv()
    );
    semaphore->init();
    return semaphore;
}

Semaphore::Semaphore(
    const shared_ptr<Device> &device,
    vk::ExternalSemaphoreHandleTypeFlagBits *handleType,
    Priv)
    : m_device(device)
    , m_handleType(handleType ? make_unique<vk::ExternalSemaphoreHandleTypeFlagBits>(*handleType) : nullptr)
{}
Semaphore::~Semaphore()
{}

void Semaphore::init()
{
    vk::ExportSemaphoreCreateInfo exportCreateInfo;
    vk::SemaphoreCreateInfo createInfo;
    if (m_handleType)
    {
        exportCreateInfo.handleTypes = *m_handleType;
        createInfo.pNext = &exportCreateInfo;
    }
    m_semaphore = m_device->createSemaphoreUnique(createInfo, nullptr, m_device->dld());
}

int Semaphore::exportFD()
{
    vk::SemaphoreGetFdInfoKHR semaphoreGetFdInfo;
    semaphoreGetFdInfo.semaphore = *m_semaphore;
    semaphoreGetFdInfo.handleType = *m_handleType;
    return m_device->getSemaphoreFdKHR(semaphoreGetFdInfo, m_device->dld());
}

#ifdef VK_USE_PLATFORM_WIN32_KHR
HANDLE Semaphore::exportWin32Handle()
{
    vk::SemaphoreGetWin32HandleInfoKHR semaphoreGetWin32HandleInfo;
    semaphoreGetWin32HandleInfo.semaphore = *m_semaphore;
    semaphoreGetWin32HandleInfo.handleType = *m_handleType;
    return m_device->getSemaphoreWin32HandleKHR(semaphoreGetWin32HandleInfo, m_device->dld());
}
#endif

}
