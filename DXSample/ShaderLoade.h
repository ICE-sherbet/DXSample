#pragma once
#include <windows.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <wrl.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <vector>
#pragma comment(lib, "dxgi.lib")

inline std::vector<std::byte> readFile(const std::filesystem::path& p) {
  std::ifstream f(p, std::ios::binary | std::ios::ate);
  if (!f) throw std::runtime_error("shader file not found");

  const std::streamsize sz = f.tellg();
  std::vector<std::byte> buf(static_cast<size_t>(sz));
  f.seekg(0, std::ios::beg);
  f.read(reinterpret_cast<char*>(buf.data()), sz);
  return buf;
}

inline std::vector<std::byte> compileDXC(const std::filesystem::path& hlsl,
                                         const std::wstring_view entry,
                                         const std::wstring_view target,
                                         const std::wstring& defines = L"") {
  using Microsoft::WRL::ComPtr;
  ComPtr<IDxcCompiler3> comp;
  const IID a;
  DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&comp));
  ComPtr<IDxcUtils> utils;
  DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
  ComPtr<IDxcIncludeHandler> incl;
  utils->CreateDefaultIncludeHandler(&incl);

  auto src = readFile(hlsl);
  DxcBuffer buf{src.data(), src.size(), DXC_CP_ACP};

  std::vector<LPCWSTR> args;
  args.push_back(L"-Qembed_debug");
  args.push_back(L"-Zpr");
  args.push_back(std::wstring(L"-E").append(entry).c_str());
  args.push_back(std::wstring(L"-T").append(target).c_str());
  if (!defines.empty()) args.push_back(defines.c_str());

  ComPtr<IDxcResult> res;
  comp->Compile(&buf, args.data(), (UINT)args.size(), incl.Get(),
                IID_PPV_ARGS(&res));
  ComPtr<IDxcBlob> blob;
  res->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&blob), nullptr);
  return {(std::byte*)blob->GetBufferPointer(),
          (std::byte*)blob->GetBufferPointer() + blob->GetBufferSize()};
}

/* acquire VS or PS : if cache hit ÅÀ read, else compile+write */
inline std::vector<std::byte> getShaderBinary(
    const std::string& shaderName,  // "pbr"
    const std::string& stage,       // "vs" / "ps"
    const std::wstring& entry,      // L"VSMain"
    const std::wstring& target,     // L"vs_6_7"
    const std::wstring& defines = L"") {
  namespace fs = std::filesystem;
  fs::path bin = std::format("cache/{}_{}.dxil", shaderName, stage);
  if (fs::exists(bin)) return readFile(bin);

  fs::create_directories("cache");
  fs::path hlsl = std::format("shaders/{}.hlsl", shaderName);
  auto dxil = compileDXC(hlsl, entry, target, defines);
  return dxil;
}
