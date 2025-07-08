#include "RawInputBackend.h"

#pragma comment(lib, "xinput.lib")

#include <xinput.h>

#include <algorithm>
#include <cmath>

using namespace base;
static PKey Unify(PKey pk) {
  switch (pk) {
    case PKey::LShift:
    case PKey::RShift:
      return PKey::Shift;
    case PKey::LCtrl:
    case PKey::RCtrl:
      return PKey::Ctrl;
    case PKey::LAlt:
    case PKey::RAlt:
      return PKey::Alt;
    default:
      return pk;
  }
}
RawInputBackend::RawInputBackend(HWND hwnd) { registerDevices(hwnd); }

LRESULT RawInputBackend::handleMsg(UINT msg, WPARAM wp, LPARAM lp) {
  if (msg == WM_INPUT) {
    UINT sz = 0;
    ::GetRawInputData(reinterpret_cast<HRAWINPUT>(lp), RID_INPUT, nullptr, &sz,
                      sizeof(RAWINPUTHEADER));
    std::vector<uint8_t> buf(sz);
    if (::GetRawInputData(reinterpret_cast<HRAWINPUT>(lp), RID_INPUT,
                          buf.data(), &sz, sizeof(RAWINPUTHEADER)) == sz) {
      const RAWINPUT& ri = *reinterpret_cast<RAWINPUT*>(buf.data());
      processRaw(ri);
    }
    return 0;
  }
  return DefWindowProc(nullptr, msg, wp, lp);
}

void RawInputBackend::registerDevices(HWND hwnd) {
  RAWINPUTDEVICE rid[2]{};

  rid[0].usUsagePage = 0x01;
  rid[0].usUsage = 0x06;
  rid[0].dwFlags = RIDEV_INPUTSINK;
  rid[0].hwndTarget = hwnd;

  rid[1].usUsagePage = 0x01;
  rid[1].usUsage = 0x02;
  rid[1].dwFlags = RIDEV_INPUTSINK;
  rid[1].hwndTarget = hwnd;

  if (!::RegisterRawInputDevices(rid, 2, sizeof(rid[0]))) {
    // TODO: ÉGÉâÅ[èàóù
  }
}

void RawInputBackend::processRaw(const RAWINPUT& ri) {
  Snapshot& wr = buffers_[0];

  switch (ri.header.dwType) {
    case RIM_TYPEKEYBOARD: {
      const RAWKEYBOARD& kb = ri.data.keyboard;
      const bool keyUp = kb.Flags & RI_KEY_BREAK;
      PKey pk = Unify(static_cast<PKey>(kb.VKey));
      size_t idx = static_cast<size_t>(pk);
      bool keyDown = !(kb.Flags & RI_KEY_BREAK);
      if (keyDown && !wr.down.test(idx)) wr.pressed.set(idx);

      wr.down.set(idx, keyDown);
      break;
    }
    case RIM_TYPEMOUSE: {
      const RAWMOUSE& m = ri.data.mouse;
      if (m.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) wr.mouseBtn |= 1;
      if (m.usButtonFlags & RI_MOUSE_BUTTON_1_UP) wr.mouseBtn &= ~1;
      if (m.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) wr.mouseBtn |= 2;
      if (m.usButtonFlags & RI_MOUSE_BUTTON_2_UP) wr.mouseBtn &= ~2;
      if (m.lLastX || m.lLastY) {
        wr.axes[(size_t)PAxis::MouseX] =
            std::clamp(m.lLastX / 500.f, -1.f, 1.f);
        wr.axes[(size_t)PAxis::MouseY] =
            std::clamp(-m.lLastY / 500.f, -1.f, 1.f);

        wr.axes[(size_t)PAxis::MDX] += m.lLastX;
        wr.axes[(size_t)PAxis::MDY] += m.lLastY;
      }
      break;
    }
  }
}

const Snapshot& RawInputBackend::acquire() {
  Snapshot& wr = buffers_[0];
  updateXInput(wr);

  wr.frame = ++frameCounter_;
  writeIdx_.store(1 - writeIdx_, std::memory_order_release);

  return buffers_[0];
}
void RawInputBackend::BeginFrame() {
  Snapshot& wr = buffers_[0];
  wr.axes[(size_t)PAxis::MDX] = 0;
  wr.axes[(size_t)PAxis::MDY] = 0;
}
void RawInputBackend::updateXInput(Snapshot& dst) {
  XINPUT_STATE xi{};
  if (XInputGetState(0, &xi) == ERROR_SUCCESS) {
    auto& g = xi.Gamepad;
    dst.down.set(VK_PAD_A, g.wButtons & XINPUT_GAMEPAD_A);
    dst.down.set(VK_PAD_B, g.wButtons & XINPUT_GAMEPAD_B);

    dst.axes[(size_t)PAxis::LX] = normThumb(g.sThumbLX);
    dst.axes[(size_t)PAxis::LY] = -normThumb(g.sThumbLY);
    dst.axes[(size_t)PAxis::RX] = normThumb(g.sThumbRX);
    dst.axes[(size_t)PAxis::RY] = -normThumb(g.sThumbRY);
    dst.axes[(size_t)PAxis::LT] = normTrig(g.bLeftTrigger);
    dst.axes[(size_t)PAxis::RT] = normTrig(g.bRightTrigger);
  }
}

float RawInputBackend::normThumb(SHORT v) {
  return (std::fabs(v) < 7849) ? 0.f : v / 32768.f;
}
float RawInputBackend::normTrig(BYTE v) { return (v < 30) ? 0.f : v / 255.f; }