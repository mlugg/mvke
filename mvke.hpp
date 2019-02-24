#pragma once

#include <string>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <optional>

#include "glfw.hpp"
#include "geometry.hpp"

#define MVKE_MAJOR 0
#define MVKE_MINOR 1
#define MVKE_PATCH 0

namespace MVKE {
  struct QueueFamilies {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    bool isComplete() {
      return graphics && present;
    }
  };

  class Device;
  class Swapchain;
  class Pipeline;
  class Buffer;
  class MappableBuffer;
  class StagedBuffer;

  class Instance {
    friend MVKE::Device;
    friend MVKE::Swapchain;
    friend MVKE::Pipeline;
    friend MVKE::Buffer;
    friend MVKE::MappableBuffer;
    friend MVKE::StagedBuffer;
  public:
    Instance(std::string appName, unsigned major, unsigned minor, unsigned patch);
    void mainLoop();
  private:
    vk::UniqueInstance mVkInst;
    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> mDbgMessenger;
    MVKE::GLFW *mWindow;
    std::shared_ptr<MVKE::Device> mDevice;

    struct {
      vk::Queue graphics;
      vk::Queue present;
    } mQueues;

    std::shared_ptr<MVKE::Swapchain> mSwapchain;

    std::shared_ptr<MVKE::Pipeline> mPipeline;

    std::shared_ptr<vk::DispatchLoaderDynamic> mDynamicLoader;

    static const std::vector<const char *> sValidation;

#ifndef NDEBUG
    static const bool sEnableValidation = true;
#else
    static const bool sEnableValidation = false;
#endif

    const std::vector<MVKE::Vertex> vertices = {
        {{-0.7f, -0.7f}, {1.0f, 0.0f, 1.0f}},
        {{0.7f, -0.7f}, {0.0f, 0.0f, 1.0f}},
        {{-0.7f, 0.7f}, {0.0f, 0.0f, 1.0f}},

        {{-0.7f, 0.7f}, {0.0f, 0.0f, 1.0f}},
        {{0.7f, -0.7f}, {0.0f, 0.0f, 1.0f}},
        {{0.7f, 0.7f}, {1.0f, 0.0f, 1.0f}},
    };

    vk::UniqueCommandPool mCommandPool;
    std::vector<vk::UniqueCommandBuffer> mCommandBuffers;

    std::vector<vk::UniqueSemaphore> mImageAvailable;
    std::vector<vk::UniqueSemaphore> mReaderFinished;
    std::vector<vk::UniqueFence> mInFlight;

    size_t mCurrentFrame = 0;

    bool mFramebufferResized = false;

    std::shared_ptr<MVKE::Buffer> mVertexBuffer;

    void drawFrame();

    void recreateSwapchain();

    void initCommandBuffers();
  };
}