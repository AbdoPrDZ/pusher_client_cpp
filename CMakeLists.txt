#          Copyright Joe Coder 2004 - 2006.
#  Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          https://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.8)
project(PusherClient CXX)

set(VCPKG_DIR D:/Programs/vcpkg) # replace with your vcpkg path
set(CMAKE_TOOLCHAIN_FILE ${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake)
set(CMAKE_MODULE_PATH ${VCPKG_DIR}/scripts/buildsystems)

include_directories(${VCPKG_DIR}/installed/x64-windows/include)
link_directories(${VCPKG_DIR}/installed/x64-windows/lib)

add_subdirectory(PusherClient)
