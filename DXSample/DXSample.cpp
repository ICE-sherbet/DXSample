#include <windows.h>

#include <string>

#include "Input/InputConfig.h"
#include "Input/RawInputBackend.h"
#include "Input/VirtualInput.h"

#pragma comment(lib, "comctl32.lib")

static base::RawInputBackend* g_raw = nullptr;
static base::VirtualInput* g_vi = nullptr;

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  if (g_raw && m == WM_INPUT) return g_raw->HandleMsg(m, w, l);

  switch (m) {
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProc(h, m, w, l);
}

int APIENTRY wWinMain(HINSTANCE hi, HINSTANCE, LPWSTR, int) {
  const wchar_t CLASS_NAME[] = L"InputSampleWnd";

  WNDCLASSEX wc{sizeof(wc)};
  wc.style = CS_OWNDC;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hi;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszClassName = CLASS_NAME;
  if (!RegisterClassEx(&wc)) return 0;

  HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Input System Sample",
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                             1280, 720, nullptr, nullptr, hi, nullptr);

  if (!hwnd) return 0;
  ShowWindow(hwnd, SW_SHOWDEFAULT);

  base::RawInputBackend raw(hwnd);
  g_raw = &raw;

  base::InputConfig cfg;
  cfg.Load(L"bindings.json");
  base::VirtualInput vi(cfg);
  g_vi = &vi;
  float cameraYaw = 0;
  float cameraPitch = 0;
  MSG msg{};
  while (msg.message != WM_QUIT) {
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    const base::InputSnapshot& snap = raw.GetCurrentSnapshot();

    vi.BeginFrame();
    vi.ApplySnapshot(snap);
    vi.EndFrame();

    base::Float2 move = vi.GetVector2("Move");
    if (move.x || move.y) {
      std::wstring msg = L"Move: ";
      msg += std::to_wstring(move.x);
      msg += L" : ";
      msg += std::to_wstring(move.y);
      msg += L"\n";

      OutputDebugString(msg.c_str());
    }

    base::Float2 look = vi.GetVector2("Look");
    cameraYaw += look.x;
    cameraPitch += look.y;
    {
      std::wstring msg = L"Camera: ";
      msg += std::to_wstring(cameraYaw);
      msg += L" : ";
      msg += std::to_wstring(cameraPitch);
      msg += L"\n";
      OutputDebugString(msg.c_str());
    }
    if (vi.GetAction("Save").pressed) {
      MessageBox(hwnd, L"Save action triggered!", L"Info", MB_OK);
    }

    raw.BeginFrame();

    Sleep(16);  // ≒60 fps
  }
  return static_cast<int>(msg.wParam);
}
