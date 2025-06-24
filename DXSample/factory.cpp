#include "ApiTraitsDX12.h"
#include "DeviceImplCore.inl"

namespace base::graphics {
template
class DeviceImpl<Backend::DX12>;

std::shared_ptr<Device> CreateDevice(const DeviceDesc& d) {
  return std::make_shared<DeviceImpl<Backend::DX12>>(d);
}
}  // namespace rhi
