#pragma once
#include <bitset>
#include <cmath>
#include <string>
#include <string_view>
#include <unordered_map>

#include "InputConfig.h"
#include "RawInputBackend.h"

namespace base {

struct Float2 {
  float x = 0, y = 0;
};
inline Float2 clamp1(Float2 v) {
  float len = v.x * v.x + v.y * v.y;
  if (len > 1.f) {
    len = std::sqrt(len);
    v.x /= len;
    v.y /= len;
  }
  return v;
}

struct Axis {
  float value = 0, prevRaw = 0;
  DeviceId dev = UINT32_MAX;
  int rel = -1;
};
struct Action {
  bool down = false, pressed = false, released = false;
  DeviceId dev = UINT32_MAX;
};

struct ChordState {
  bool active = false;
};
struct SeqState {
  size_t idx = 0;
  int lastFrame = -99999;
};

class VirtualInput {
 public:
  explicit VirtualInput(const InputConfig& cfg) : cfg_(cfg) {}

  void BeginFrame();
  void ApplySnapshot(const InputSnapshot& snap);
  void EndFrame();

  float GetAxis(std::string_view n) const {
    return axes_.at(std::string(n)).value;
  }
  Float2 GetVector2(std::string_view n) const;
  const Action& GetAction(std::string_view n) const {
    return actions_.at(std::string(n));
  }

 private:
  void HandleBinding(const InputBinding&, const InputSnapshot&);
  void UpdateAxis(const InputBinding&, float raw, int frame);
  void UpdateActionDigital(const InputBinding&, bool down);
  void UpdateActionAnalog(const InputBinding&, float raw);
  void UpdateChord(const InputBinding&, const InputSnapshot&, int frame);
  void UpdateSequence(const InputBinding&, const InputSnapshot&, int frame);

  const InputConfig& cfg_;
  int frame_ = 0;

  std::unordered_map<std::string, Axis> axes_;
  std::unordered_map<std::string, Action> actions_;
  std::unordered_map<std::string, ChordState> chord_;
  std::unordered_map<std::string, SeqState> seq_;
};

void VirtualInput::BeginFrame() {
  for (auto& [_, ax] : axes_) ax.value = 0;
  for (auto& [_, ac] : actions_) {
    ac.pressed = ac.released = false;
  }
}

void VirtualInput::ApplySnapshot(const InputSnapshot& snap) {
  frame_ = snap.frame;
  for (const InputBinding& b : cfg_.bindings) HandleBinding(b, snap);

  for (auto& [_, ax] : axes_) ax.value = std::clamp(ax.value, -1.f, 1.f);
}

inline void VirtualInput::EndFrame() {}

inline Float2 VirtualInput::GetVector2(std::string_view n) const {
  auto it = cfg_.composites.find(std::string(n));
  if (it == cfg_.composites.end()) return {};
  const InputComposite& c = it->second;
  Float2 v;
  if (c.x) v.x = axes_.at(*c.x).value;
  if (c.y) v.y = axes_.at(*c.y).value;
  return clamp1(v);
}

inline void VirtualInput::HandleBinding(const InputBinding& b,
                                        const InputSnapshot& snap) {
  switch (b.kind) {
    case InputBinding::Kind::DigitalToAxis: {
      bool d = snap.down.test(static_cast<size_t>(b.keys[0]));
      UpdateAxis(b, d ? b.scale : 0.f, frame_);
      break;
    }
    case InputBinding::Kind::AnalogToAxis: {
      float raw = snap.axes[(size_t)*b.axis] * b.scale;
      UpdateAxis(b, raw, frame_);
      break;
    }
    case InputBinding::Kind::DigitalToAction: {
      bool d = snap.down.test(static_cast<size_t>(b.keys[0]));
      UpdateActionDigital(b, d);
      break;
    }
    case InputBinding::Kind::AnalogToAction: {
      float r = snap.axes[(size_t)*b.axis] * b.scale;
      UpdateActionAnalog(b, r);
      break;
    }
    case InputBinding::Kind::ChordToAction:
      UpdateChord(b, snap, frame_);
      break;
    case InputBinding::Kind::SequenceToAction:
      UpdateSequence(b, snap, frame_);
      break;
  }
}

inline void VirtualInput::UpdateAxis(const InputBinding& b, float raw, int frame) {
  Axis& ax = axes_[b.virtualName];
  float dz = 0.f;
  if (b.axis && cfg_.deadzone.contains(*b.axis)) dz = cfg_.deadzone.at(*b.axis);
  float in = (std::fabs(raw) < dz) ? 0.f : raw;

  if (ax.dev == UINT32_MAX && in != 0) ax.dev = b.device;
  if (ax.dev == b.device) ax.value += in;

  float accel = std::fabs(in - ax.prevRaw);
  if (ax.dev == b.device) {
    bool stationary = (in == 0 && accel <= cfg_.accelThreshold);
    if (stationary) {
      if (ax.rel == -1)
        ax.rel = frame;
      else if (frame - ax.rel >= cfg_.latchReleaseFrames) {
        ax.dev = UINT32_MAX;
        ax.rel = -1;
      }
    } else
      ax.rel = -1;
  }
  ax.prevRaw = ax.value;
}

inline void VirtualInput::UpdateActionDigital(const InputBinding& b, bool down) {
  Action& a = actions_[b.virtualName];
  if (a.dev == UINT32_MAX && down) a.dev = b.device;
  if (a.dev != b.device) return;
  a.pressed = down && !a.down;
  a.released = !down && a.down;
  a.down = down;
  if (!down) a.dev = UINT32_MAX;
}
inline void VirtualInput::UpdateActionAnalog(const InputBinding& b, float raw) {
  bool down = raw >= b.threshold;
  UpdateActionDigital(b, down);
}

void VirtualInput::UpdateChord(const InputBinding& b, const InputSnapshot& s, int f) {
  auto& st = chord_[b.virtualName];
  bool allDown = true;
  for (PKey k : b.keys) allDown &= s.down.test(static_cast<size_t>(k));

  Action& a = actions_[b.virtualName];

  if (allDown && !st.active) {
    a.pressed = a.down = true;
    st.active = true;
  } else if (!allDown && st.active) {
    a.released = true;
    a.down = false;
    st.active = false;
  }
}

inline void VirtualInput::UpdateSequence(const InputBinding& b, const InputSnapshot& s,
                                      int f) {
  auto& st = seq_[b.virtualName];
  if (!s.pressed.test(static_cast<size_t>(b.keys[st.idx]))) return;

  if (f - st.lastFrame > b.maxGapFrames) st.idx = 0;
  ++st.idx;
  st.lastFrame = f;

  if (st.idx == b.keys.size()) {
    Action& a = actions_[b.virtualName];
    a.pressed = a.down = true;
    st.idx = 0;
  }
}

}
