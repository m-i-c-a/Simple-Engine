add_executable(vsync 
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/Pipeline.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/FrameResources.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/Stats.cpp 
    ${imgui_SOURCES})

target_include_directories(vsync PRIVATE 
    ${vk_core_INCLUDE_DIRS} 
    ${imgui_wrapper_INCLUDE_DIRS}
    ${nvtx_INCLUDE_DIRS})

target_link_libraries(vsync PRIVATE 
    vk_core
    imgui_wrapper 
    glfw)