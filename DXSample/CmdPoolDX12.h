#pragma once
#include <d3d12.h>
#include <wrl.h>

#include <cstdint>
#include <vector>

class CmdPoolDX12 {
  using ComPtr = Microsoft::WRL::ComPtr<ID3D12Object>;
  std::vector<ComPtr> alloc_;  // allocator per-frame
  std::vector<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>> list_;  // matching command list
  uint32_t frameCount_{};
  uint32_t currFrame_{};

 public:
  void init(ID3D12Device* dev, uint32_t frames) {
    frameCount_ = frames;
    alloc_.resize(frames);
    list_.resize(frames);
    for (uint32_t i = 0; i < frames; i++) {
      dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                  IID_PPV_ARGS(&alloc_[i]));
      dev->CreateCommandList(
          0, D3D12_COMMAND_LIST_TYPE_DIRECT,
          static_cast<ID3D12CommandAllocator*>(alloc_[i].Get()), nullptr,
          IID_PPV_ARGS(&list_[i]));
      list_[i]->Close();  // start closed
    }
  }
  /** returns an OPEN command list ready for recording */
  ID3D12GraphicsCommandList* begin(uint32_t frameIdx) {
    currFrame_ = frameIdx % frameCount_;
    auto* alloc =
        static_cast<ID3D12CommandAllocator*>(alloc_[currFrame_].Get());
    auto* cl = static_cast<ID3D12GraphicsCommandList*>(list_[currFrame_].Get());
    alloc->Reset();
    cl->Reset(alloc, nullptr);
    return cl;
  }
  /** returns the CLOSED command list for submission */
  ID3D12GraphicsCommandList* end() {
    auto* cl = static_cast<ID3D12GraphicsCommandList*>(list_[currFrame_].Get());
    cl->Close();
    return cl;
  }
};
