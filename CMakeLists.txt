cmake_minimum_required(VERSION 3.20)

# 使用 vcpkg
set(
    CMAKE_TOOLCHAIN_FILE
    "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "vcpkg toolchain file"
)

set(
    VCPKG_TARGET_TRIPLET
    "x64-windows"
    CACHE STRING "vcpkg target triplet"
)

project(CV_Homework LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

find_package(OpenCV CONFIG REQUIRED)

message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV_DIR: ${OpenCV_DIR}")

add_subdirectory(HW1)
add_subdirectory(HW2)
add_subdirectory(HW3)
add_subdirectory(HW4)
add_subdirectory(DIP_HW2)