#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include "Device.h"
#include "CmdPoolDX12.h"

namespace base::graphics {
template <>
struct ApiTraits<Backend::DX12> {
  struct Native {
    Microsoft::WRL::ComPtr<IDXGISwapChain4> swap;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> q;
    CmdPoolDX12 cmdPool;
  };
  static void init(Native&, const DeviceDesc&);
  static void present(Native&);
  static uint32_t beginCmd(Native& n, Queue);
  static void submit(Native& n, uint32_t);
};
}  // namespace rhi
