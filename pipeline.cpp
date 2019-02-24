#include "pipeline.hpp"
#include "swapchain.hpp"
#include "device.hpp"
#include "geometry.hpp"

#include <vector>
#include <fstream>

std::vector<char> readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file!");
  }

  size_t filesize = file.tellg();

  std::vector<char> buf(filesize);

  file.seekg(0);
  file.read(buf.data(), filesize);
  file.close();

  return buf;
}

MVKE::Pipeline::Pipeline(MVKE::Instance &inst) : mInst(inst) {
  initRenderPass();

  auto vertCode = readFile("build/shaders/vert.spv");
  auto fragCode = readFile("build/shaders/frag.spv");

  mVertShader = createShader(vertCode);
  mFragShader = createShader(fragCode);

  vk::PipelineShaderStageCreateInfo shaderStages[] = {
    {
      vk::PipelineShaderStageCreateFlags(),
      vk::ShaderStageFlagBits::eVertex,
      *mVertShader,
      "main"
    }, {
      vk::PipelineShaderStageCreateFlags(),
      vk::ShaderStageFlagBits::eFragment,
      *mFragShader,
      "main"
    }
  };

  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  vk::PipelineVertexInputStateCreateInfo vertexInputInfo(
    vk::PipelineVertexInputStateCreateFlags(),
    1,
    &bindingDescription,
    attributeDescriptions.size(),
    attributeDescriptions.data()
  );

  vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
    vk::PipelineInputAssemblyStateCreateFlags(),
    vk::PrimitiveTopology::eTriangleList,
    VK_FALSE
  );

  vk::Extent2D swapchainExtent = mInst.mSwapchain->extent();

  vk::Viewport viewport(
    0.0f,
    0.0f,
    swapchainExtent.width,
    swapchainExtent.height,
    0.0f,
    1.0f
  );

  vk::Rect2D scissor(
    {0, 0},
    swapchainExtent
  );

  vk::PipelineViewportStateCreateInfo viewportState(
    vk::PipelineViewportStateCreateFlags(),
    1,
    &viewport,
    1,
    &scissor
  );

  vk::PipelineRasterizationStateCreateInfo rasterizer(
    vk::PipelineRasterizationStateCreateFlags(),
    VK_FALSE,
    VK_FALSE,
    vk::PolygonMode::eFill,
    vk::CullModeFlagBits::eBack,
    vk::FrontFace::eClockwise,
    VK_FALSE,
    0.0f,
    0.0f,
    0.0f,
    1.0f
  );

  vk::PipelineMultisampleStateCreateInfo multisampling(
    vk::PipelineMultisampleStateCreateFlags(),
    vk::SampleCountFlagBits::e1,
    VK_FALSE,
    1.0f,
    nullptr,
    VK_FALSE,
    VK_FALSE
  );

  using ComponentFlag = vk::ColorComponentFlagBits;
  vk::PipelineColorBlendAttachmentState colorBlendAttachment(
    VK_FALSE,
    vk::BlendFactor::eOne,
    vk::BlendFactor::eZero,
    vk::BlendOp::eAdd,
    vk::BlendFactor::eOne,
    vk::BlendFactor::eZero,
    vk::BlendOp::eAdd,
    ComponentFlag::eR | ComponentFlag::eG | ComponentFlag::eB | ComponentFlag::eA
  );

  vk::PipelineColorBlendStateCreateInfo colorBlending(
    vk::PipelineColorBlendStateCreateFlags(),
    VK_FALSE,
    vk::LogicOp::eCopy,
    1,
    &colorBlendAttachment,
    {0, 0, 0, 0}
  );

  vk::PipelineLayoutCreateInfo layoutInfo(
    vk::PipelineLayoutCreateFlags(),
    0,
    nullptr,
    0,
    nullptr
  );

  mLayout = mInst.mDevice->device().createPipelineLayoutUnique(layoutInfo);

  vk::GraphicsPipelineCreateInfo pipelineInfo(
    vk::PipelineCreateFlags(),
    2,
    shaderStages,
    &vertexInputInfo,
    &inputAssembly,
    nullptr,
    &viewportState,
    &rasterizer,
    &multisampling,
    nullptr,
    &colorBlending,
    nullptr,
    *mLayout,
    *mRenderPass,
    0,
    vk::Pipeline(),
    -1
  );

  mPipeline = mInst.mDevice->device().createGraphicsPipelineUnique(vk::PipelineCache(), pipelineInfo);
}

vk::UniqueShaderModule MVKE::Pipeline::createShader(const std::vector<char> &code) {
  vk::ShaderModuleCreateInfo createInfo(
    vk::ShaderModuleCreateFlags(),
    code.size(),
    reinterpret_cast<const uint32_t *>(code.data())
  );

  return mInst.mDevice->device().createShaderModuleUnique(createInfo);
}

void MVKE::Pipeline::initRenderPass() {
  vk::AttachmentDescription colorAttachment(
    vk::AttachmentDescriptionFlags(),
    mInst.mSwapchain->surfaceFormat().format,
    vk::SampleCountFlagBits::e1,
    vk::AttachmentLoadOp::eClear,
    vk::AttachmentStoreOp::eStore,
    vk::AttachmentLoadOp::eDontCare,
    vk::AttachmentStoreOp::eDontCare,
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::ePresentSrcKHR
  );

  vk::AttachmentReference colorAttachmentRef(
    0,
    vk::ImageLayout::eColorAttachmentOptimal
  );

  vk::SubpassDescription subpass(
    vk::SubpassDescriptionFlags(),
    vk::PipelineBindPoint::eGraphics,
    0,
    nullptr,
    1,
    &colorAttachmentRef
  );

  vk::RenderPassCreateInfo renderPassInfo(
    vk::RenderPassCreateFlags(),
    1,
    &colorAttachment,
    1,
    &subpass
  );

  vk::SubpassDependency dependency(
    VK_SUBPASS_EXTERNAL,
    0,
    vk::PipelineStageFlagBits::eColorAttachmentOutput,
    vk::PipelineStageFlagBits::eColorAttachmentOutput,
    vk::AccessFlags(),
    vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite
  );

  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  mRenderPass = mInst.mDevice->device().createRenderPassUnique(renderPassInfo);
}

const vk::RenderPass &MVKE::Pipeline::renderPass() const { return *mRenderPass; }
const vk::Pipeline &MVKE::Pipeline::pipeline() const { return *mPipeline; }