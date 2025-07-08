// main.cpp  ── Win32 ウィンドウ生成＋入力ループ最小サンプル
// ──────────────────────────────────────────────
#include <windows.h>

#include <string>

#include "Input/InputConfig.h"
#include "Input/RawInputBackend.h"  // Part-1
#include "Input/VirtualInput.h"     // Part-2

#pragma comment(lib, "comctl32.lib")

// ──────────────────────────────────────────────
//  1. グローバル
// ──────────────────────────────────────────────
static base::RawInputBackend* g_raw = nullptr;
static base::VirtualInput* g_vi = nullptr;

// ──────────────────────────────────────────────
//  2. 窓プロシージャ
// ──────────────────────────────────────────────
LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  // RawInput はバックエンドへ
  if (g_raw && m == WM_INPUT) return g_raw->handleMsg(m, w, l);

  switch (m) {
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProc(h, m, w, l);
}

// ──────────────────────────────────────────────
//  3. エントリポイント WinMain
// ──────────────────────────────────────────────
int APIENTRY wWinMain(HINSTANCE hi, HINSTANCE, LPWSTR, int) {
  // ――― 3-A.  window class 登録 ―――
  const wchar_t CLASS_NAME[] = L"InputSampleWnd";

  WNDCLASSEX wc{sizeof(wc)};
  wc.style = CS_OWNDC;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hi;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszClassName = CLASS_NAME;
  if (!RegisterClassEx(&wc)) return 0;

  // ――― 3-B.  window 作成 ―――
  HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Input System Sample",
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                             1280, 720, nullptr, nullptr, hi, nullptr);

  if (!hwnd) return 0;
  ShowWindow(hwnd, SW_SHOWDEFAULT);

  // ――― 3-C.  入力システム初期化 ―――
  base::RawInputBackend raw(hwnd);
  g_raw = &raw;

  base::Config cfg;
  cfg.load(L"bindings.json");  // Part-2 形式の JSON
  base::VirtualInput vi(cfg);
  g_vi = &vi;
  float cameraYaw = 0;
  float cameraPitch = 0;
  // ――― 3-D.  メインループ ―――
  MSG msg{};
  while (msg.message != WM_QUIT) {
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    // 1) RawInputBackend から最新スナップショット取得
    const base::Snapshot& snap = raw.acquire();

    // 2) VirtualInput 更新
    vi.beginFrame();
    vi.applySnapshot(snap);
    vi.endFrame();

    // 3) 例: 移動ベクトルとショートカット確認
    base::Vec2 move = vi.getVector2("Move");
    if (move.x || move.y) {
      std::wstring msg = L"Move: ";
      msg += std::to_wstring(move.x);
      msg += L" : ";
      msg += std::to_wstring(move.y);
      msg += L"\n";

      OutputDebugString(msg.c_str());
    }

    base::Vec2 look = vi.getVector2("Look");  // MouseΔ & PadRX/RY 合成済み
    cameraYaw += look.x;                       // 左右
    cameraPitch += look.y;                     // 上下
    {
      std::wstring msg = L"Camera: ";
      msg += std::to_wstring(cameraYaw);
      msg += L" : ";
      msg += std::to_wstring(cameraPitch);
      msg += L"\n";
      OutputDebugString(msg.c_str());
    }
    if (vi.getAction("Save").pressed) {
      MessageBox(hwnd, L"Save action triggered!", L"Info", MB_OK);
    }

    raw.BeginFrame();


    // 4) 簡易 vsync 的 sleep
    Sleep(16);  // ≒60 fps
  }
  return static_cast<int>(msg.wParam);
}
