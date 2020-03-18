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

#ifdef VK_USE_PLATFORM_WIN32_KHR
#   ifdef QMVK_LIBRARY
#       define QMVK_EXPORT __declspec(dllexport)
#   else
#       define QMVK_EXPORT __declspec(dllimport)
#   endif
#else
#   define QMVK_EXPORT __attribute__((visibility("default")))
#endif
