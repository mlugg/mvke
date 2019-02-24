#include "mvke.hpp"

#include <vector>
#include <iostream>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <chrono>

#include <string>

#include "device.hpp"
#include "swapchain.hpp"
#include "pipeline.hpp"
#include "buffer.hpp"

#define MAX_CONCURRENT_FRAMES (2)

const std::vector<const char *> MVKE::Instance::sValidation = {
  "VK_LAYER_LUNARG_standard_validation",
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {
  std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

MVKE::Instance::Instance(std::string appName, unsigned major, unsigned minor, unsigned patch) {
  mWindow = new MVKE::GLFW(800, 600, appName, mFramebufferResized);
  std::vector<const char *> extensions;
  std::vector<const char *> layers;

  for (const char *&ext : mWindow->getVkExtensions()) {
    extensions.push_back(ext);
  }

  if (sEnableValidation) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    for (const char *const &layer : sValidation) {
      layers.push_back(layer);
    }
  }

  auto appInfo = vk::ApplicationInfo(
    appName.c_str(),
    VK_MAKE_VERSION(major, minor, patch),
    "MVKE",
    VK_MAKE_VERSION(MVKE_MAJOR, MVKE_MINOR, MVKE_PATCH),
    VK_API_VERSION_1_1
  );

  mVkInst = vk::createInstanceUnique(
    vk::InstanceCreateInfo(
      vk::InstanceCreateFlags(),
      &appInfo,
      layers.size(),
      layers.data(),
      extensions.size(),
      extensions.data()
    )
  );

  mDynamicLoader = std::make_shared<vk::DispatchLoaderDynamic>(*mVkInst);

  if (sEnableValidation) {
    using MsgSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    using MsgType = vk::DebugUtilsMessageTypeFlagBitsEXT;

    mDbgMessenger = mVkInst->createDebugUtilsMessengerEXTUnique(
      vk::DebugUtilsMessengerCreateInfoEXT(
        vk::DebugUtilsMessengerCreateFlagsEXT(),
        MsgSeverity::eError | MsgSeverity::eWarning | MsgSeverity::eVerbose,
        MsgType::eGeneral | MsgType::eValidation | MsgType::ePerformance,
        debugCallback
      ),
      nullptr,
      *mDynamicLoader
    );
  }

  mWindow->initSurface(*mVkInst);

  mDevice = std::make_shared<MVKE::Device>(*this);

  mSwapchain = std::make_shared<MVKE::Swapchain>(*this);
  mPipeline = std::make_shared<MVKE::Pipeline>(*this);
  mSwapchain->initFramebuffers();

  QueueFamilies families = mDevice->findFamilies();

  mCommandPool = mDevice->device().createCommandPoolUnique({
    vk::CommandPoolCreateFlags(),
    *families.graphics
  });

  mVertexBuffer = std::make_shared<MVKE::StagedBuffer>(*this, vertices.size() * sizeof vertices[0], vk::BufferUsageFlagBits::eVertexBuffer);

  memcpy(mVertexBuffer->map(0, mVertexBuffer->size()), vertices.data(), mVertexBuffer->size());

  initCommandBuffers();
 
  mImageAvailable.reserve(MAX_CONCURRENT_FRAMES);
  mReaderFinished.reserve(MAX_CONCURRENT_FRAMES);
  mInFlight.reserve(MAX_CONCURRENT_FRAMES);

  for (size_t i = 0; i < MAX_CONCURRENT_FRAMES; ++i) {
    mImageAvailable.push_back(mDevice->device().createSemaphoreUnique(vk::SemaphoreCreateInfo()));
    mReaderFinished.push_back(mDevice->device().createSemaphoreUnique(vk::SemaphoreCreateInfo()));
    mInFlight.push_back(mDevice->device().createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)));
  }
}

void MVKE::Instance::mainLoop() {
  using frame = std::chrono::duration<int64_t, std::ratio<1, 120000>>;

  auto cur = std::chrono::high_resolution_clock::now();
  auto prev = cur;

  auto diff = cur - prev;

  while (mWindow->isOpen()) {
    while (cur - prev < frame{1}) {
      cur = std::chrono::high_resolution_clock::now();
    }

    diff = cur - prev;
    std::cout << "ms/f: " << diff.count() / 1000000.0f << std::endl;

    prev = cur;

    mWindow->update();
    drawFrame();
  }

  mDevice->device().waitIdle();
}

void MVKE::Instance::drawFrame() {
  mDevice->device().waitForFences(*mInFlight[mCurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

  uint32_t imageIndex;

  try {
    imageIndex = mDevice->device().acquireNextImageKHR(mSwapchain->swapchain(), std::numeric_limits<uint64_t>::max(), *mImageAvailable[mCurrentFrame], vk::Fence()).value;
  } catch (vk::OutOfDateKHRError &e) {
    recreateSwapchain();
    return;
  }

  mDevice->device().resetFences(*mInFlight[mCurrentFrame]);

  vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

  vk::SubmitInfo submitInfo(
    1,
    &mImageAvailable[mCurrentFrame].get(),
    waitStages,
    1,
    &mCommandBuffers[imageIndex].get(),
    1,
    &mReaderFinished[mCurrentFrame].get()
  );

  mQueues.graphics.submit(submitInfo, *mInFlight[mCurrentFrame]);

  vk::PresentInfoKHR presentInfo(
    1,
    &mReaderFinished[mCurrentFrame].get(),
    1,
    &mSwapchain->swapchain(),
    &imageIndex
  );

  try {
    if (mQueues.graphics.presentKHR(presentInfo) == vk::Result::eSuboptimalKHR) {
      mFramebufferResized = true;
    }
  } catch (vk::OutOfDateKHRError &e) {
    mFramebufferResized = true;
  }

  if (mFramebufferResized) {
    mFramebufferResized = false;
    recreateSwapchain();
    return;
  }

  mCurrentFrame = (mCurrentFrame + 1) % MAX_CONCURRENT_FRAMES;
}

void MVKE::Instance::recreateSwapchain() {
  mDevice->device().waitIdle();

  mSwapchain.reset();
  mPipeline.reset();

  mSwapchain = std::make_shared<MVKE::Swapchain>(*this);
  mPipeline = std::make_shared<MVKE::Pipeline>(*this);
  mSwapchain->initFramebuffers();

  QueueFamilies families = mDevice->findFamilies();

  mCommandBuffers.clear();
  mVertexBuffer.reset();

  mCommandPool = mDevice->device().createCommandPoolUnique({
    vk::CommandPoolCreateFlags(),
    *families.graphics
  });

  mVertexBuffer = std::make_shared<MVKE::StagedBuffer>(*this, vertices.size() * sizeof vertices[0], vk::BufferUsageFlagBits::eVertexBuffer);

  memcpy(mVertexBuffer->map(0, mVertexBuffer->size()), vertices.data(), mVertexBuffer->size());

  initCommandBuffers();
}


void MVKE::Instance::initCommandBuffers() {
  vk::BufferCreateInfo bufferInfo(
    vk::BufferCreateFlags(),
    vertices.size() * sizeof vertices[0],
    vk::BufferUsageFlagBits::eVertexBuffer,
    vk::SharingMode::eExclusive
  );

  vk::CommandBufferAllocateInfo allocInfo(
    *mCommandPool,
    vk::CommandBufferLevel::ePrimary,
    mSwapchain->framebuffers().size()
  );

  mCommandBuffers = mDevice->device().allocateCommandBuffersUnique(allocInfo);

  for (size_t i = 0; i < mCommandBuffers.size(); ++i) {
    vk::CommandBufferBeginInfo beginInfo(
      vk::CommandBufferUsageFlagBits::eSimultaneousUse,
      nullptr
    );

    mCommandBuffers[i]->begin(beginInfo);

    vk::ClearValue clearColor(vk::ClearColorValue(std::array<float, 4UL>{0.0f, 0.0f, 0.0f, 1.0f}));
    vk::RenderPassBeginInfo renderPassInfo(
      mPipeline->renderPass(),
      *mSwapchain->framebuffers()[i],
      vk::Rect2D({0, 0}, mSwapchain->extent()),
      1,
      &clearColor
    );

    mCommandBuffers[i]->beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    mCommandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline->pipeline());
    mCommandBuffers[i]->bindVertexBuffers(0, {mVertexBuffer->buffer()}, {0});
    mCommandBuffers[i]->draw(vertices.size(), 1, 0, 0);
    mCommandBuffers[i]->endRenderPass();

    mCommandBuffers[i]->end();
  }
}