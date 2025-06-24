#include "ApiTraitsDX12.h"

#pragma comment(lib, "dxgi.lib")
namespace base::graphics {

static Microsoft::WRL::ComPtr<ID3D12Device> gDev;
void ApiTraits<Backend::DX12>::init(Native& n, const DeviceDesc& d) {
  Microsoft::WRL::ComPtr<IDXGIFactory7> f;
  CreateDXGIFactory2(0, IID_PPV_ARGS(&f));
  f->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                IID_PPV_ARGS(&gDev));
  D3D12_COMMAND_QUEUE_DESC qd{};
  gDev->CreateCommandQueue(&qd, IID_PPV_ARGS(&n.q));
  DXGI_SWAP_CHAIN_DESC1 scd{};
  scd.Width = 640;
  scd.Height = 480;
  scd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  scd.SampleDesc.Count = 1;
  scd.BufferCount = 2;
  scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  Microsoft::WRL::ComPtr<IDXGISwapChain1> sc1;
  f->CreateSwapChainForHwnd(n.q.Get(), (HWND)d.window, &scd, nullptr, nullptr,
                            &sc1);
  sc1.As(&n.swap);
  n.cmdPool.init(gDev.Get(), /*frames*/ 3);
}
void ApiTraits<Backend::DX12>::present(Native& n) { n.swap->Present(1, 0); }

uint32_t ApiTraits<Backend::DX12>::beginCmd(Native& n, Queue) {
  auto* cl = n.cmdPool.begin(n.swap->GetCurrentBackBufferIndex());

  // ─── ここでフレーム共通設定（ビューポート等） ───
  D3D12_VIEWPORT vp{0, 0, 640, 480, 0, 1};
  D3D12_RECT sc{0, 0, 640, 480};
  cl->RSSetViewports(1, &vp);
  cl->RSSetScissorRects(1, &sc);

  return 0;  // 返値はダミー (複数 CL の拡張余地)
}

void ApiTraits<Backend::DX12>::submit(Native& n, uint32_t) {
  ID3D12CommandList* cl[] = {n.cmdPool.end()};
  n.q->ExecuteCommandLists(1, cl);

  /* optional fence sync 略 */
}
}  // namespace base::graphics