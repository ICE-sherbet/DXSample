#pragma once
#include <cstdint>
namespace base::graphics {
template <class Tag>
struct Handle {
  uint32_t v{0};
};
using BufferHandle = Handle<struct Buf>;
using FrameHandle = uint32_t;
}