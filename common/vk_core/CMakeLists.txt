add_library(vk_core STATIC src/vk_core.cpp include/vk_core.hpp)

target_include_directories(vk_core PUBLIC $ENV{VULKAN_SDK}/include)
target_include_directories(vk_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(vk_core PRIVATE $ENV{VULKAN_SDK}/lib/libvulkan.so)

set(vk_core_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)