#pragma once

#include <vulkan/vulkan.hpp>
#include "mvke.hpp"

namespace MVKE {
  class Pipeline {
  public:
    Pipeline(MVKE::Instance &inst);
    const vk::RenderPass &renderPass() const;
    const vk::Pipeline &pipeline() const;
  private:
    MVKE::Instance &mInst;
    
    vk::UniqueShaderModule createShader(const std::vector<char> &code);
    vk::UniqueShaderModule mVertShader;
    vk::UniqueShaderModule mFragShader;
    vk::UniquePipelineLayout mLayout;
    vk::UniqueRenderPass mRenderPass;
    vk::UniquePipeline mPipeline;

    void initRenderPass();
  };
}