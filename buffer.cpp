#include "buffer.hpp"

#include "device.hpp"

MVKE::Buffer::Accessor::Accessor(MVKE::Buffer &buf, uint64_t offset, uint64_t size) : mBuf(buf), mOffset(offset), mSize(size), mData(buf.map_buffer(offset, size)) {}
MVKE::Buffer::Accessor::~Accessor() { mBuf.unmap_buffer(mData, mOffset, mSize); }
MVKE::Buffer::Accessor::operator void *() { return mData; }

MVKE::Buffer::Buffer(MVKE::Instance &inst, uint64_t size, vk::BufferUsageFlags usage, VmaMemoryUsage memUsage) : mInst(inst) {
  VmaAllocationCreateInfo createInfo = {};
  createInfo.usage = memUsage;

  vk::BufferCreateInfo bufferInfo(
    vk::BufferCreateFlags(),
    size,
    usage,
    vk::SharingMode::eExclusive
  );

  vmaCreateBuffer(
    mInst.mAllocator,
    reinterpret_cast<VkBufferCreateInfo *>(&bufferInfo),
    &createInfo,
    reinterpret_cast<VkBuffer *>(&mBuffer),
    &mAllocation,
    &mInfo
  );
}

MVKE::Buffer::~Buffer() {
  vmaDestroyBuffer(mInst.mAllocator, mBuffer, mAllocation);
}

MVKE::Buffer::Accessor MVKE::Buffer::map(uint64_t offset, uint64_t size) {
  if (size + offset > mInfo.size) throw std::runtime_error("Attempt to map space outside buffer!");
  return MVKE::Buffer::Accessor(*this, mInfo.offset + offset, size);
}

const vk::Buffer &MVKE::Buffer::buffer() const {
  return mBuffer;
}

const vk::DeviceMemory MVKE::Buffer::memory() const {
  return {mInfo.deviceMemory};
}

uint64_t MVKE::Buffer::size() const {
  return mInfo.size;
}

MVKE::HighPerformanceBuffer::HighPerformanceBuffer(MVKE::Instance &inst, uint64_t size, vk::BufferUsageFlags usage)
: MVKE::Buffer(inst, size, usage, VMA_MEMORY_USAGE_GPU_ONLY) {}

void *MVKE::HighPerformanceBuffer::map_buffer(uint64_t offset, uint64_t size) {
  throw std::runtime_error("Cannot map buffer in GPU local memory!");
}

void MVKE::HighPerformanceBuffer::unmap_buffer(void *data, uint64_t offset, uint64_t size) {
  throw std::runtime_error("Cannot unmap buffer in GPU local memory!");
}

MVKE::MappableBuffer::MappableBuffer(MVKE::Instance &inst, uint64_t size, vk::BufferUsageFlags usage)
: MVKE::Buffer(inst, size, usage, VMA_MEMORY_USAGE_CPU_ONLY) {}

void *MVKE::MappableBuffer::map_buffer(uint64_t offset, uint64_t size) {
  return mInst.mDevice->device().mapMemory(mInfo.deviceMemory, offset, size);
}

void MVKE::MappableBuffer::unmap_buffer(void *data, uint64_t offset, uint64_t size) {
  mInst.mDevice->device().unmapMemory(mInfo.deviceMemory);
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
  bufs[0]->copyBuffer(mStaging->buffer(), mBuffer, {{0, offset, size}});
  bufs[0]->end();

  mInst.mQueues.graphics.submit({{0, nullptr, nullptr, 1, &bufs[0].get(), 0, nullptr}}, vk::Fence());
  mInst.mQueues.graphics.waitIdle();

  delete mStaging;
  mStaging = nullptr;
}