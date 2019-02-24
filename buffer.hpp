#pragma once

#include "mvke.hpp"
#include <vulkan/vulkan.hpp>

namespace MVKE {
  class Buffer {
    class Accessor {
      friend Buffer;
      Accessor(Buffer &buf, uint64_t offset, uint64_t size);
    public:
      operator void *();
      ~Accessor();
    private:
      Buffer &mBuf;
      uint64_t mOffset;
      uint64_t mSize;
      void *mData;
    };

    friend Accessor;

  protected:
    virtual void *map_buffer(uint64_t offset, uint64_t size) = 0;
    virtual void unmap_buffer(void *data, uint64_t offset, uint64_t size) = 0;

  public:
    Buffer(MVKE::Instance &inst, uint64_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props);
    virtual ~Buffer() = default;
    Accessor map(uint64_t offset, uint64_t size);
    const vk::Buffer &buffer() const;
    const vk::DeviceMemory &memory() const;
    uint64_t size() const;

  protected:
    MVKE::Instance &mInst;
    uint64_t mSize;
    vk::UniqueBuffer mBuffer;
    vk::UniqueDeviceMemory mMemory;

  private:
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
  };

  class HighPerformanceBuffer : public Buffer {
  public:
    HighPerformanceBuffer(MVKE::Instance &inst, uint64_t size, vk::BufferUsageFlags usage);
  protected:
    virtual void *map_buffer(uint64_t offset, uint64_t size);
    virtual void unmap_buffer(void *data, uint64_t offset, uint64_t size);
  };

  class MappableBuffer : public Buffer {
  public:
    MappableBuffer(MVKE::Instance &inst, uint64_t size, vk::BufferUsageFlags usage);
  protected:
    virtual void *map_buffer(uint64_t offset, uint64_t size);
    virtual void unmap_buffer(void *data, uint64_t offset, uint64_t size);
  };


  class StagedBuffer : public HighPerformanceBuffer {
  public:
    StagedBuffer(MVKE::Instance &inst, uint64_t size, vk::BufferUsageFlags usage);
  protected:
    virtual void *map_buffer(uint64_t offset, uint64_t size);
    virtual void unmap_buffer(void *data, uint64_t offset, uint64_t size);
  
    MappableBuffer *mStaging = nullptr;
  };
}