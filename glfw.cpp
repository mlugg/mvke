#include "glfw.hpp"

#include <vector>

void MVKE::GLFW::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
  auto *glfw = reinterpret_cast<MVKE::GLFW *>(glfwGetWindowUserPointer(window));
  glfw->mResizeVal = true;
}

MVKE::GLFW::GLFW(int width, int height, std::string title, bool &resizeVal) : mResizeVal(resizeVal) {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  mWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

  glfwSetWindowUserPointer(mWindow, this);
  glfwSetFramebufferSizeCallback(mWindow, framebufferResizeCallback);
}

MVKE::GLFW::~GLFW() {
  vkDestroySurfaceKHR(mInst, mSurface, nullptr);
  glfwDestroyWindow(mWindow);
  glfwTerminate();
}

void MVKE::GLFW::initSurface(vk::Instance &vkInst) {
  mInst = vkInst;
  
  if (glfwCreateWindowSurface(VkInstance(vkInst), mWindow, nullptr, &mSurface) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create window surface!");
  }
}

std::vector<const char *> MVKE::GLFW::getVkExtensions() const {
  uint32_t count = 0;
  const char **exts;

  exts = glfwGetRequiredInstanceExtensions(&count);

  return std::vector<const char *>(exts, exts + count);
}

bool MVKE::GLFW::isOpen() const {
  return !glfwWindowShouldClose(mWindow);
}

void MVKE::GLFW::update() {
  glfwPollEvents();
}

const vk::Extent2D MVKE::GLFW::getSize() const {
  int width, height;
  glfwGetFramebufferSize(mWindow, &width, &height);

  return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

vk::SurfaceKHR MVKE::GLFW::surface() const { return vk::SurfaceKHR(mSurface); }