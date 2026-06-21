// SPDX-License-Identifier: MIT
/*
   QmVk - simple Vulkan library created for QMPlay2
   Copyright (C) 2020-2026 Błażej Szczygieł
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

#include <vulkan/vulkan.hpp>

#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <cstring>
#include <memory>
#include <mutex>
#include <cmath>
#include <map>

namespace QmVk {

using namespace std;

}
