#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

#include "mvke.hpp"

namespace MVKE {
  struct SwapchainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
  };

  class Swapchain {
  public:
    Swapchain(MVKE::Instance &inst);
    void initFramebuffers();
    static bool adequate(const vk::PhysicalDevice &d, MVKE::Instance &inst);

    const vk::SwapchainKHR &swapchain() const;
    const vk::Extent2D &extent() const;
    const vk::SurfaceFormatKHR &surfaceFormat() const;
    const std::vector<vk::UniqueFramebuffer> &framebuffers() const;
  private:
    static MVKE::SwapchainSupportDetails querySupport(const vk::PhysicalDevice &dev, MVKE::Instance &inst);
    vk::SurfaceFormatKHR chooseSurfaceFormat(std::vector<vk::SurfaceFormatKHR> &available);
    vk::PresentModeKHR choosePresentMode(std::vector<vk::PresentModeKHR> &available);
    vk::Extent2D chooseExtent(vk::SurfaceCapabilitiesKHR &capabilities);
    void initImages();

    MVKE::Instance &mInst;

    vk::SurfaceFormatKHR mSurfaceFormat;
    vk::PresentModeKHR mPresentMode;
    vk::Extent2D mExtent;

    vk::UniqueSwapchainKHR mSwapchain;
    std::vector<vk::Image> mImages;
    std::vector<vk::UniqueImageView> mImageViews;
    std::vector<vk::UniqueFramebuffer> mFramebuffers;
  };
}