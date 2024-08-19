add_executable(gui ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ${imgui_SOURCES})
target_include_directories(gui PRIVATE ${vk_core_INCLUDE_DIRS} ${imgui_wrapper_INCLUDE_DIRS})
target_link_libraries(gui PRIVATE vk_core imgui_wrapper glfw)