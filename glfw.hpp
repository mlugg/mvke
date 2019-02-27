#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>

namespace MVKE {
  class GLFW {
  public:
    GLFW(int width, int height, std::string title, bool &resizeVal);
    ~GLFW();
    std::vector<const char *> getVkExtensions() const;
    bool isOpen() const;
    void update();
    void initSurface(vk::Instance &vkInst);

    vk::SurfaceKHR surface() const;
    const vk::Extent2D getSize() const;

    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
  private:
    GLFWwindow *mWindow;
    VkSurfaceKHR mSurface;
    bool &mResizeVal;
    vk::Instance mInst;
  };
}