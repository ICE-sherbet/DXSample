// DXSample.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include <iostream>
#include "ShaderLoade.h"
int main()
{
    std::cout << "Hello World!\n";
}
/*
#include <Windows.h>

#include <fstream>

#include "../rhi/public/rhi_device.hpp"
#include "../rhi/public/rhi_enums.hpp"
#include "make_sphere.hpp"

using namespace rhi;
static std::vector<std::byte> readBin(const char* p) {
  std::ifstream f(p, std::ios::binary);
  return {std::istreambuf_iterator<char>(f), {}};
}
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
  HWND hwnd = CreateWindowA("STATIC", "Sphere", WS_OVERLAPPEDWINDOW, 100, 100,
                            800, 600, nullptr, nullptr, nullptr, nullptr);
  ShowWindow(hwnd, SW_SHOW);

  auto dev = CreateDevice({Backend::DX12, hwnd, true});
  std::vector<Vtx> vtx;
  std::vector<uint16_t> idx;
  makeSphere(32, 16, vtx, idx);

  auto& impl = *static_cast<DeviceImpl<Backend::DX12>*>(dev.get());
  auto& nat = impl.native_;

  // Create vertex / index buffers
  BufferHandle vb = ApiTraits<Backend::DX12>::createBuffer(
      nat, {vtx.size() * sizeof(Vtx), BufferDesc::Vertex}, vtx.data());
  BufferHandle ib = ApiTraits<Backend::DX12>::createBuffer(
      nat, {idx.size() * 2, BufferDesc::Index}, idx.data());

  // Constant buffer (mvp + camPos)
  struct CB {
    DirectX::XMFLOAT4X4 mvp;
    DirectX::XMFLOAT4 cam;
  } cbData{};
  cbData.cam = {0, 0, 5, 0};
  BufferHandle cb = ApiTraits<Backend::DX12>::createBuffer(
      nat, {256, BufferDesc::Const}, &cbData);
  DescriptorIndex cbv = ApiTraits<Backend::DX12>::ReserveCBV(nat, cb, 256);

  // Pipeline
  auto vsBin = readBin("$(Configuration)/sphere_vs.dxil");  // adjust path
  auto psBin = readBin("$(Configuration)/sphere_ps.dxil");
  auto pso = ApiTraits<Backend::DX12>::createPipeline(
      nat, vsBin.data(), vsBin.size(), psBin.data(), psBin.size(),
      Format::RGBA8, Format::D32);

  // main loop
  MSG m;
  for (;;) {
    while (PeekMessage(&m, nullptr, 0, 0, PM_REMOVE)) {
      if (m.message == WM_QUIT) return 0;
      TranslateMessage(&m);
      DispatchMessage(&m);
    }
    auto f = dev->BeginFrame();
    auto cl = dev->BeginCmd(f, Queue::Graphics);

    ID3D12GraphicsCommandList* cmd = nat.cmdPool.current();
    // VB/IB views
    D3D12_VERTEX_BUFFER_VIEW vbv{
        nat.buffPool.at(vb.v).native->GetGPUVirtualAddress(),
        UINT(vtx.size() * sizeof(Vtx)), sizeof(Vtx)};
    D3D12_INDEX_BUFFER_VIEW ibv{
        nat.buffPool.at(ib.v).native->GetGPUVirtualAddress(),
        UINT(idx.size() * 2), DXGI_FORMAT_R16_UINT};

    cmd->SetPipelineState(nat.psoPool.at(pso.v).native.Get());
    cmd->SetGraphicsRootSignature(nat.psoPool.at(pso.v).rs.Get());
    cmd->SetGraphicsRootConstantBufferView(
        0, nat.buffPool.at(cb.v).native->GetGPUVirtualAddress());
    cmd->IASetVertexBuffers(0, 1, &vbv);
    cmd->IASetIndexBuffer(&ibv);
    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->DrawIndexedInstanced((UINT)idx.size(), 1, 0, 0, 0);

    ApiTraits<Backend::DX12>::transitionRTtoPresent(
        nat, cmd, nat.swap->GetCurrentBackBufferIndex());
    dev->Submit(cl);
    dev->EndFrame(f);
    dev->Present(f);
  }
}
// プログラムの実行: Ctrl + F5 または [デバッグ] > [デバッグなしで開始] メニュー
// プログラムのデバッグ: F5 または [デバッグ] > [デバッグの開始] メニュー

// 作業を開始するためのヒント: 
//    1. ソリューション エクスプローラー ウィンドウを使用してファイルを追加/管理します 
//   2. チーム エクスプローラー ウィンドウを使用してソース管理に接続します
//   3. 出力ウィンドウを使用して、ビルド出力とその他のメッセージを表示します
//   4. エラー一覧ウィンドウを使用してエラーを表示します
//   5. [プロジェクト] > [新しい項目の追加] と移動して新しいコード ファイルを作成するか、[プロジェクト] > [既存の項目の追加] と移動して既存のコード ファイルをプロジェクトに追加します
//   6. 後ほどこのプロジェクトを再び開く場合、[ファイル] > [開く] > [プロジェクト] と移動して .sln ファイルを選択します

*/