cmake_minimum_required(VERSION 3.15)
project(vkapiwin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_TOOLCHAIN_FILE "C:/Dev/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

set(VULKAN_SDK "C:/VulkanSDK/1.4.313.0")
set(GLM_ROOT "${VULKAN_SDK}/Include/glm")

set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/Learning/Source")
set(INC "${CMAKE_CURRENT_SOURCE_DIR}/Learning/Include")

include_directories(
    ${VULKAN_SDK}/Include
    ${GLM_ROOT}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${INC}
    ${SRC}
)

find_package(SDL2 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME}
    ${SRC}/main.cpp
    ${SRC}/VulkanApp.cpp
    ${SRC}/VulkanCore.cpp
    ${SRC}/VulkanDevice.cpp
    ${SRC}/VulkanSwapChain.cpp
    ${SRC}/VulkanRenderer.cpp
    ${SRC}/VulkanUtils.cpp
    ${SRC}/VulkanLogger.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan SDL2::SDL2)
