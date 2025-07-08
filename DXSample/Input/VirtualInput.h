#pragma once
#include <bitset>
#include <cmath>
#include <string>
#include <string_view>
#include <unordered_map>

#include "InputConfig.h"
#include "RawInputBackend.h"

namespace base {

// ───────────────────────────────────────────────────────────────
//  1.  Runtime value containers
// ───────────────────────────────────────────────────────────────
struct Vec2 {
  float x = 0, y = 0;
};
inline Vec2 clamp1(Vec2 v) {
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

// ───────────────────────────────────────────────────────────────
class VirtualInput {
 public:
  explicit VirtualInput(const Config& cfg) : cfg_(cfg) {}

  //---------------- frame API ----------------
  void beginFrame();
  void applySnapshot(const Snapshot& snap);  // main work
  void endFrame();

  //---------------- query API ----------------
  float getAxis(std::string_view n) const {
    return axes_.at(std::string(n)).value;
  }
  Vec2 getVector2(std::string_view n) const;
  const Action& getAction(std::string_view n) const {
    return actions_.at(std::string(n));
  }

 private:
  //---------------- helpers ------------------
  void handleBinding(const Binding&, const Snapshot&);
  void updAxis(const Binding&, float raw, int frame);
  void updActionDigital(const Binding&, bool down);
  void updActionAnalog(const Binding&, float raw);
  void updChord(const Binding&, const Snapshot&, int frame);
  void updSequence(const Binding&, const Snapshot&, int frame);

  //-------------------------------------------
  const Config& cfg_;
  int frame_ = 0;

  std::unordered_map<std::string, Axis> axes_;
  std::unordered_map<std::string, Action> actions_;
  std::unordered_map<std::string, ChordState> chord_;
  std::unordered_map<std::string, SeqState> seq_;
};

// ───────────────────────────────────────────────────────────────
//  2.  Implementation
// ───────────────────────────────────────────────────────────────
inline void VirtualInput::beginFrame() {
  for (auto& [_, ax] : axes_) ax.value = 0;
  for (auto& [_, ac] : actions_) {
    ac.pressed = ac.released = false;
  }
}

inline void VirtualInput::applySnapshot(const Snapshot& snap) {
  frame_ = snap.frame;
  for (const Binding& b : cfg_.bindings) handleBinding(b, snap);

  // clamp axes
  for (auto& [_, ax] : axes_) ax.value = std::clamp(ax.value, -1.f, 1.f);
}

inline void VirtualInput::endFrame() {}

//------------------------------ vector -------------------------
inline Vec2 VirtualInput::getVector2(std::string_view n) const {
  auto it = cfg_.composites.find(std::string(n));
  if (it == cfg_.composites.end()) return {};
  const Composite& c = it->second;
  Vec2 v;
  if (c.x) v.x = axes_.at(*c.x).value;
  if (c.y) v.y = axes_.at(*c.y).value;
  return clamp1(v);
}

//---------------------- dispatcher -----------------------------
inline void VirtualInput::handleBinding(const Binding& b,
                                        const Snapshot& snap) {
  switch (b.kind) {
    case Binding::Kind::DigitalToAxis: {
      bool d = snap.down.test(static_cast<size_t>(b.keys[0]));
      updAxis(b, d ? b.scale : 0.f, frame_);
      break;
    }
    case Binding::Kind::AnalogToAxis: {
      float raw = snap.axes[(size_t)*b.axis] * b.scale;
      updAxis(b, raw, frame_);
      break;
    }
    case Binding::Kind::DigitalToAction: {
      bool d = snap.down.test(static_cast<size_t>(b.keys[0]));
      updActionDigital(b, d);
      break;
    }
    case Binding::Kind::AnalogToAction: {
      float r = snap.axes[(size_t)*b.axis] * b.scale;
      updActionAnalog(b, r);
      break;
    }
    case Binding::Kind::ChordToAction:
      updChord(b, snap, frame_);
      break;
    case Binding::Kind::SequenceToAction:
      updSequence(b, snap, frame_);
      break;
  }
}

//---------------- Axis update w/ latch -------------------------
inline void VirtualInput::updAxis(const Binding& b, float raw, int frame) {
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

//-------------- Digital / Analog Action -----------------------
inline void VirtualInput::updActionDigital(const Binding& b, bool down) {
  Action& a = actions_[b.virtualName];
  if (a.dev == UINT32_MAX && down) a.dev = b.device;
  if (a.dev != b.device) return;
  a.pressed = down && !a.down;
  a.released = !down && a.down;
  a.down = down;
  if (!down) a.dev = UINT32_MAX;
}
inline void VirtualInput::updActionAnalog(const Binding& b, float raw) {
  bool down = raw >= b.threshold;
  updActionDigital(b, down);
}

//-------------- Chord -----------------------------------------
void VirtualInput::updChord(const Binding& b, const Snapshot& s, int f) {
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

//-------------- Sequence --------------------------------------
inline void VirtualInput::updSequence(const Binding& b, const Snapshot& s,
                                      int f) {
  auto& st = seq_[b.virtualName];
  if (!s.pressed.test(static_cast<size_t>(b.keys[st.idx]))) return;

  if (f - st.lastFrame > b.maxGapFrames) st.idx = 0;  // タイムアウト
  ++st.idx;
  st.lastFrame = f;

  if (st.idx == b.keys.size()) {
    Action& a = actions_[b.virtualName];
    a.pressed = a.down = true;
    st.idx = 0;
  }
}

}  // namespace in
