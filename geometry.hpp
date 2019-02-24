#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include <array>

namespace MVKE {
  struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription getBindingDescription();
    static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions();
  };

  struct Triangle {
    Vertex a;
    Vertex b;
    Vertex c;
  };
}