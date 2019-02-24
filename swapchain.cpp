#include "swapchain.hpp"
#include "pipeline.hpp"
#include "mvke.hpp"
#include "device.hpp"

bool MVKE::Swapchain::adequate(const vk::PhysicalDevice &dev, MVKE::Instance &inst) {
  auto details = querySupport(dev, inst);

  return !details.formats.empty() && !details.presentModes.empty();
}

MVKE::SwapchainSupportDetails MVKE::Swapchain::querySupport(const vk::PhysicalDevice &dev, MVKE::Instance &inst) {
  MVKE::SwapchainSupportDetails details;
  
  details.capabilities = dev.getSurfaceCapabilitiesKHR(inst.mWindow->surface());
  details.formats = dev.getSurfaceFormatsKHR(inst.mWindow->surface());
  details.presentModes = dev.getSurfacePresentModesKHR(inst.mWindow->surface());

  return details;
}

MVKE::Swapchain::Swapchain(MVKE::Instance &inst) : mInst(inst) {
  SwapchainSupportDetails details = querySupport(mInst.mDevice->physDevice(), mInst);

  mSurfaceFormat = chooseSurfaceFormat(details.formats);
  mPresentMode = choosePresentMode(details.presentModes);
  mExtent = chooseExtent(details.capabilities);

  uint32_t imageCount = details.capabilities.minImageCount + 1;

  if (details.capabilities.maxImageCount > 0) {
    imageCount = std::min(imageCount, details.capabilities.maxImageCount);
  }

  vk::SwapchainCreateInfoKHR createInfo(
    vk::SwapchainCreateFlagsKHR(),
    inst.mWindow->surface(),
    imageCount,
    mSurfaceFormat.format,
    mSurfaceFormat.colorSpace,
    mExtent,
    1,
    vk::ImageUsageFlagBits::eColorAttachment,
    vk::SharingMode::eExclusive,
    0,
    nullptr,
    details.capabilities.currentTransform,
    vk::CompositeAlphaFlagBitsKHR::eOpaque,
    mPresentMode,
    VK_TRUE
  );

  QueueFamilies families = mInst.mDevice->findFamilies();

  uint32_t familiesArr[] = {*families.graphics, *families.present};

  if (families.graphics != families.present) {
    createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = familiesArr;
  }

  mSwapchain = mInst.mDevice->device().createSwapchainKHRUnique(createInfo);

  initImages();
}

vk::SurfaceFormatKHR MVKE::Swapchain::chooseSurfaceFormat(std::vector<vk::SurfaceFormatKHR> &available) {
  if (available.size() == 1 && available[0].format == vk::Format::eUndefined) {
    return {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
  }

  for (const auto &format : available) {
    if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return format;
    }
  }

  return available[0];
}

vk::PresentModeKHR MVKE::Swapchain::choosePresentMode(std::vector<vk::PresentModeKHR> &available) {
  vk::PresentModeKHR best = vk::PresentModeKHR::eFifo;

  for (const auto &mode : available) {
    if (mode == vk::PresentModeKHR::eMailbox) {
      return mode;
    } else if (mode == vk::PresentModeKHR::eImmediate) {
      best = mode;
    }
  }
  
  return best;
}

vk::Extent2D MVKE::Swapchain::chooseExtent(vk::SurfaceCapabilitiesKHR &capabilities) {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }

  vk::Extent2D actual = mInst.mWindow->getSize();

  actual.width = std::clamp(actual.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
  actual.height = std::clamp(actual.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

  return actual;
}

void MVKE::Swapchain::initImages() {
  mImages = mInst.mDevice->device().getSwapchainImagesKHR(*mSwapchain);
  mImageViews.reserve(mImages.size());
  for (auto &img : mImages) {
    vk::ImageViewCreateInfo createInfo(
      vk::ImageViewCreateFlags(),
      img,
      vk::ImageViewType::e2D,
      mSurfaceFormat.format,
      vk::ComponentMapping(),
      vk::ImageSubresourceRange(
        vk::ImageAspectFlagBits::eColor,
        0,
        1,
        0,
        1
      )
    );
    mImageViews.push_back(mInst.mDevice->device().createImageViewUnique(createInfo));
  }
}

void MVKE::Swapchain::initFramebuffers() {
  mFramebuffers.reserve(mImageViews.size());

  for (auto &view : mImageViews) {
    vk::FramebufferCreateInfo framebufferInfo(
      vk::FramebufferCreateFlags(),
      mInst.mPipeline->renderPass(),
      1,
      &view.get(),
      mExtent.width,
      mExtent.height,
      1
    );

    mFramebuffers.push_back(mInst.mDevice->device().createFramebufferUnique(framebufferInfo));
  }
}

const vk::SwapchainKHR &MVKE::Swapchain::swapchain() const { return *mSwapchain; }
const vk::Extent2D &MVKE::Swapchain::extent() const { return mExtent; }
const vk::SurfaceFormatKHR &MVKE::Swapchain::surfaceFormat() const { return mSurfaceFormat; }
const std::vector<vk::UniqueFramebuffer> &MVKE::Swapchain::framebuffers() const { return mFramebuffers; }