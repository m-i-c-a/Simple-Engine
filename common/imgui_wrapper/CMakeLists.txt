add_library(imgui_wrapper STATIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui_wrapper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/src/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/src/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/src/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/src/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/src/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/src/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/src/imgui_impl_vulkan.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/src/implot_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/src/implot_items.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/src/implot.cpp
    )

target_include_directories(imgui_wrapper PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/include  
    ${CMAKE_CURRENT_SOURCE_DIR}/include  
    vk_core_INCLUDE_DIRS)

target_link_libraries(imgui_wrapper PRIVATE vk_core)