// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2024 Błażej Szczygieł
*/

#if defined(QMVK_NO_EXPORT)
#   define QMVK_EXPORT
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
#   ifdef QMVK_LIBRARY
#       define QMVK_EXPORT __declspec(dllexport)
#   else
#       define QMVK_EXPORT __declspec(dllimport)
#   endif
#else
#   define QMVK_EXPORT __attribute__((visibility("default")))
#endif
