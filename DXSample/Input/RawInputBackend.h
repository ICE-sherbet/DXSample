#pragma once
#include <windows.h>

#include <array>
#include <atomic>
#include <bitset>
#include <cstdint>
#include <vector>


namespace base {

enum class PKey : uint16_t {
  None = 0x00,  // 仮想キー 0x00 は未使用

  /* 0x00-0x07 は未使用 */
  Back = VK_BACK,
  Tab = VK_TAB,

  /* 0x0C-0x0D */
  Enter = VK_RETURN,

  Shift = VK_SHIFT,
  Ctrl = VK_CONTROL,
  Alt = VK_MENU,
  Pause = VK_PAUSE,
  CapsLock = VK_CAPITAL,
  Esc = VK_ESCAPE,

  /* 0x20-0x2F */
  Space = VK_SPACE,
  PageUp = VK_PRIOR,
  PageDown = VK_NEXT,
  End = VK_END,
  Home = VK_HOME,
  Left = VK_LEFT,
  Up = VK_UP,
  Right = VK_RIGHT,
  Down = VK_DOWN,
  Print = VK_PRINT,
  Insert = VK_INSERT,
  DeleteKey = VK_DELETE,

  /* 0x30-0x39 数字 */
  D0 = '0',
  D1,
  D2,
  D3,
  D4,
  D5,
  D6,
  D7,
  D8,
  D9,

  /* 0x41-0x5A 英字 */
  A = 'A',
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,

  /* 0x60-0x69 テンキー数字 */
  Num0 = VK_NUMPAD0,
  Num1,
  Num2,
  Num3,
  Num4,
  Num5,
  Num6,
  Num7,
  Num8,
  Num9,

  /* 0x6A-0x6F テンキー記号 */
  NumMultiply = VK_MULTIPLY,
  NumAdd = VK_ADD,
  NumSubtract = VK_SUBTRACT,
  NumDecimal = VK_DECIMAL,
  NumDivide = VK_DIVIDE,

  /* 0x70-0x87 F1-F24 */
  F1 = VK_F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  F13,
  F14,
  F15,
  F16,
  F17,
  F18,
  F19,
  F20,
  F21,
  F22,
  F23,
  F24,

  LShift = VK_LSHIFT,
  RShift = VK_RSHIFT,
  LCtrl = VK_LCONTROL,
  RCtrl = VK_RCONTROL,
  LAlt = VK_LMENU,
  RAlt = VK_RMENU,

  MouseLeft = 0x100,
  MouseRight = 0x101,
  MouseMiddle = 0x102,

  Count = 0x103 
};

enum class PAxis : uint16_t {
  MouseX,
  MouseY,
  MDX,
  MDY,
  LX,
  LY,
  RX,
  RY,
  LT,
  RT,
  Count
};

using DeviceId = uint32_t;
constexpr DeviceId DEVICE_KEYBD = 1;
constexpr DeviceId DEVICE_MOUSE = 2;
constexpr DeviceId DEVICE_PAD0 = 4;

struct InputSnapshot {
  std::bitset<static_cast<size_t>(PKey::Count)> down;
  std::bitset<static_cast<size_t>(PKey::Count)> pressed;
  uint8_t mouseBtn = 0;   // bit0=L, bit1=R, bit2=M
  float axes[static_cast<size_t>(PAxis::Count)]{};
  uint32_t frame = 0;
};

class RawInputBackend {
 public:
  RawInputBackend(HWND hwnd);

  LRESULT HandleMsg(UINT msg, WPARAM wp, LPARAM lp);

  const InputSnapshot& GetCurrentSnapshot();
  void BeginFrame();

 private:
  void RegisterDevices(HWND hwnd);
  void ProcessRaw(const RAWINPUT& ri);

  InputSnapshot buffers_[2]{};
  std::atomic<uint32_t> writeIdx_{0};
  uint32_t frameCounter_ = 0;

  void UpdateXInput(InputSnapshot& dst);
};

}
