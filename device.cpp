#include "device.hpp"
#include "mvke.hpp"
#include "swapchain.hpp"
#include "pipeline.hpp"

#include <map>
#include <set>
#include <string>

const std::vector<const char *> MVKE::Device::sExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

MVKE::Device::Device(MVKE::Instance &inst) : mInst(inst) {
  auto group = chooseDeviceGroup();
  createLogicalDevice(group);
}

void MVKE::Device::createLogicalDevice(std::vector<vk::PhysicalDevice> group) {
  mPhysDevice = group[0];
  auto families = findFamilies(mPhysDevice);

  auto queueInfos = std::vector<vk::DeviceQueueCreateInfo>();

  std::set<uint32_t> uniqueFamilies = {*families.graphics, *families.present};

  float priority = 1.0f;

  for (uint32_t family : uniqueFamilies) {
    queueInfos.push_back(
      vk::DeviceQueueCreateInfo(
        vk::DeviceQueueCreateFlags(),
        family,
        1,
        &priority
      )
    );
  }

  vk::PhysicalDeviceFeatures features;

  std::vector<const char *> layers;

  if (MVKE::Instance::sEnableValidation) {
    for (const char *const &l : MVKE::Instance::sValidation) {
      layers.push_back(l);
    }
  }

  vk::DeviceCreateInfo createInfo(
    vk::DeviceCreateFlags(),
    queueInfos.size(),
    queueInfos.data(),
    layers.size(),
    layers.data(),
    sExtensions.size(),
    sExtensions.data(),
    &features
  );

  vk::DeviceGroupDeviceCreateInfo deviceGroupInfo(group.size(), group.data());

  createInfo.pNext = &deviceGroupInfo;

  mDevice = group[0].createDeviceUnique(createInfo);

  mInst.mQueues.graphics = mDevice->getQueue(families.graphics.value(), 0);
  mInst.mQueues.present = mDevice->getQueue(families.present.value(), 0);
}

std::vector<vk::PhysicalDevice> MVKE::Device::chooseDeviceGroup() const {
  auto groups = mInst.mVkInst->enumeratePhysicalDeviceGroups();
  std::multimap<unsigned, const vk::PhysicalDeviceGroupProperties &> candidates;
  for (const auto &g : groups) {
    unsigned score = 0;

    for (size_t i = 0; i < g.physicalDeviceCount; ++i) {
      score += rateDevice(g.physicalDevices[i]);
    }

    candidates.insert(std::make_pair(score, g));
  }

  if (candidates.rbegin()->first > 0) {
    auto group = candidates.rbegin()->second;
    return std::vector<vk::PhysicalDevice>(group.physicalDevices, group.physicalDevices + group.physicalDeviceCount);
  } else {
    throw std::runtime_error("No suitable physical devices detected!");
  }
}

MVKE::QueueFamilies MVKE::Device::findFamilies(const vk::PhysicalDevice &d) const {
  MVKE::QueueFamilies found;

  auto families = d.getQueueFamilyProperties();

  int i = 0;
  for (const auto &f : families) {
    if (f.queueCount > 0 && f.queueFlags & vk::QueueFlagBits::eGraphics) {
      found.graphics = i;
    }

    if (f.queueCount > 0 && d.getSurfaceSupportKHR(i, mInst.mWindow->surface())) {
      found.present = i;
    }

    if (found.isComplete()) break;

    ++i;
  }

  return found;
}

unsigned MVKE::Device::rateDevice(const vk::PhysicalDevice &d) const {
  if (!findFamilies(d).isComplete()) return 0;
  if (!checkDeviceExtSupport(d)) return 0;
  if (!MVKE::Swapchain::adequate(d, mInst)) return 0;
  
  return 1;
}

bool MVKE::Device::checkDeviceExtSupport(const vk::PhysicalDevice &d) const {
  std::set<std::string> required(sExtensions.begin(), sExtensions.end());
  
  for (const auto &ext : d.enumerateDeviceExtensionProperties()) {
    required.erase(ext.extensionName);
  }

  return required.empty();
}

MVKE::QueueFamilies MVKE::Device::findFamilies() const {
  return findFamilies(mPhysDevice);
}

const vk::Device &MVKE::Device::device() const { return *mDevice; }
const vk::PhysicalDevice &MVKE::Device::physDevice() const { return mPhysDevice; }