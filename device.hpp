#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <optional>
#include <string>

#include "mvke.hpp"

namespace MVKE {
  class Device {
  public:
    Device(MVKE::Instance &inst);
    ~Device();
    MVKE::QueueFamilies findFamilies() const;
    
    const vk::Device &device() const;
    const vk::PhysicalDevice &physDevice() const;
  private:
    std::vector<vk::PhysicalDevice> chooseDeviceGroup() const;
    unsigned rateDevice(const vk::PhysicalDevice &d) const;
    void createLogicalDevice(std::vector<vk::PhysicalDevice> group);
    bool checkDeviceExtSupport(const vk::PhysicalDevice &d) const;
    MVKE::QueueFamilies findFamilies(const vk::PhysicalDevice &d) const;

    MVKE::Instance &mInst;

    vk::PhysicalDevice mPhysDevice;
    vk::UniqueDevice mDevice;

    static const std::vector<const char *> sExtensions;
  };
}