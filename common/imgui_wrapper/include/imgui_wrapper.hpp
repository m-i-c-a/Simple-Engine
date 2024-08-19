#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "implot.h"

class GLFWwindow;

namespace imgui_wrapper
{
    void init(GLFWwindow* glfw_window, VkFormat color_attachment_format);

    void destroy();
};