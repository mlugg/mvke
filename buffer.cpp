#include "buffer.hpp"

#include "device.hpp"

MVKE::Buffer::Accessor::Accessor(MVKE::Buffer &buf, uint64_t offset, uint64_t size) : mBuf(buf), mOffset(offset), mSize(size), mData(buf.map_buffer(offset, size)) {}
MVKE::Buffer::Accessor::~Accessor() { mBuf.unmap_buffer(mData, mOffset, mSize); }
MVKE::Buffer::Accessor::operator void *() { return mData; }

MVKE::Buffer::Buffer(MVKE::Instance &inst, uint64_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props) : mInst(inst), mSize(size) {
  mBuffer = mInst.mDevice->device().createBufferUnique({
    vk::BufferCreateFlags(),
    size,
    usage,
    vk::SharingMode::eExclusive
  });

  auto requirements = mInst.mDevice->device().getBufferMemoryRequirements(*mBuffer);

  mMemory = mInst.mDevice->device().allocateMemoryUnique({
    requirements.size,
    findMemoryType(requirements.memoryTypeBits, props)
  });

  mInst.mDevice->device().bindBufferMemory(*mBuffer, *mMemory, 0);
}

MVKE::Buffer::Accessor MVKE::Buffer::map(uint64_t offset, uint64_t size) {
  return MVKE::Buffer::Accessor(*this, offset, size);
}

const vk::Buffer &MVKE::Buffer::buffer() const {
  return *mBuffer;
}

const vk::DeviceMemory &MVKE::Buffer::memory() const {
  return *mMemory;
}

uint64_t MVKE::Buffer::size() const {
  return mSize;
}

uint32_t MVKE::Buffer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
  auto memProps = mInst.mDevice->physDevice().getMemoryProperties();
  for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
    if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("Failed to find suitable memory type!");
}

MVKE::HighPerformanceBuffer::HighPerformanceBuffer(MVKE::Instance &inst, uint64_t size, vk::BufferUsageFlags usage)
: MVKE::Buffer(inst, size, usage, vk::MemoryPropertyFlagBits::eDeviceLocal) {}

void *MVKE::HighPerformanceBuffer::map_buffer(uint64_t offset, uint64_t size) {
  throw std::runtime_error("Cannot map buffer in GPU local memory!");
}

void MVKE::HighPerformanceBuffer::unmap_buffer(void *data, uint64_t offset, uint64_t size) {
  throw std::runtime_error("Cannot unmap buffer in GPU local memory!");
}

MVKE::MappableBuffer::MappableBuffer(MVKE::Instance &inst, uint64_t size, vk::BufferUsageFlags usage)
: MVKE::Buffer(inst, size, usage, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) {}

void *MVKE::MappableBuffer::map_buffer(uint64_t offset, uint64_t size) {
  return mInst.mDevice->device().mapMemory(*mMemory, offset, size);
}

void MVKE::MappableBuffer::unmap_buffer(void *data, uint64_t offset, uint64_t size) {
  mInst.mDevice->device().unmapMemory(*mMemory);
}

MVKE::StagedBuffer::StagedBuffer(MVKE::Instance &inst, uint64_t size, vk::BufferUsageFlags usage)
: MVKE::HighPerformanceBuffer(inst, size, usage | vk::BufferUsageFlagBits::eTransferDst) {}//, mFastBuffer(inst, size, usage | vk::BufferUsageFlagBits::eTransferDst) {}

void *MVKE::StagedBuffer::map_buffer(uint64_t offset, uint64_t size) {
  if (mStaging) return nullptr;

  mStaging = new MVKE::MappableBuffer(mInst, size, vk::BufferUsageFlagBits::eTransferSrc);
  return mInst.mDevice->device().mapMemory(mStaging->memory(), offset, size);
}

void MVKE::StagedBuffer::unmap_buffer(void *data, uint64_t offset, uint64_t size) {
  mInst.mDevice->device().unmapMemory(mStaging->memory());

  auto bufs = mInst.mDevice->device().allocateCommandBuffersUnique({*mInst.mCommandPool, vk::CommandBufferLevel::ePrimary, 1});

  bufs[0]->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  bufs[0]->copyBuffer(mStaging->buffer(), *mBuffer, {{0, offset, size}});
  bufs[0]->end();

  mInst.mQueues.graphics.submit({{0, nullptr, nullptr, 1, &bufs[0].get(), 0, nullptr}}, vk::Fence());
  mInst.mQueues.graphics.waitIdle();

  delete mStaging;
  mStaging = nullptr;
}