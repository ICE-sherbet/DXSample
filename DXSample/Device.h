#pragma once
#include <memory>

#include "RendererEnum.h"
#include "RendererHandle.h"

namespace base::graphics {
template <Backend B>
struct ApiTraits;

struct DeviceDesc {
  Backend backend;
  void* window;
  bool debug;
};
class Device {
 public:
  virtual ~Device() = default;
  virtual FrameHandle BeginFrame() = 0;
  virtual void EndFrame(FrameHandle) = 0;
  virtual void Present(FrameHandle) = 0;
  virtual uint32_t BeginCmd(FrameHandle, Queue q) = 0;
  virtual void Submit(uint32_t cl) = 0;
};
std::shared_ptr<Device> CreateDevice(const DeviceDesc&);
}  // namespace rhi
