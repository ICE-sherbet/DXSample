#pragma once
#include "Device.h"

namespace base::graphics {
template <Backend B>
class DeviceImpl : public Device {
  typename ApiTraits<B>::Native n_;
  FrameHandle f_{};

 public:
  explicit DeviceImpl(const DeviceDesc& d) { ApiTraits<B>::init(n_, d); }
  FrameHandle BeginFrame() override { return ++f_; }
  void EndFrame(FrameHandle) override {}
  void Present(FrameHandle) override { ApiTraits<B>::present(n_); }
  uint32_t BeginCmd(FrameHandle, Queue q) override {
    return ApiTraits<B>::beginCmd(n_, q);
  }
  void Submit(uint32_t cl) override { ApiTraits<B>::submit(n_, cl); }
};
}  // namespace base::graphics
