#include "geometry.hpp"

vk::VertexInputBindingDescription MVKE::Vertex::getBindingDescription() {
  return {
    0,
    sizeof (Vertex),
    vk::VertexInputRate::eVertex
  };
}

std::array<vk::VertexInputAttributeDescription, 2> MVKE::Vertex::getAttributeDescriptions() {
  using VIAD = vk::VertexInputAttributeDescription;
  return {
    VIAD {0, 0, vk::Format::eR32G32Sfloat, offsetof (Vertex, pos)},
    VIAD {1, 0, vk::Format::eR32G32B32Sfloat, offsetof (Vertex, color)},
  };
}