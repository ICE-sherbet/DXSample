#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "RawInputBackend.h"

namespace base {

struct InputBinding {
  enum class Kind {
    DigitalToAxis,
    DigitalToAction,
    AnalogToAxis,
    AnalogToAction,
    ChordToAction,
    SequenceToAction
  };

  Kind kind{};
  std::string virtualName;
  float scale = 1.f;
  DeviceId device = DEVICE_KEYBD;

  std::vector<PKey> keys;
  std::optional<PAxis> axis;

  float threshold = 0.25f;

  int maxGapFrames = 12;
};


struct InputComposite {
  std::optional<std::string> x, y, z;
};

struct InputConfig {
  std::vector<InputBinding> bindings;
  std::unordered_map<std::string, InputComposite> composites;
  std::unordered_map<PAxis, float> deadzone;
  int latchReleaseFrames = 8;
  float accelThreshold = 0.02f;

  bool Load(const std::filesystem::path& fp, std::string* err = nullptr);
};

PKey KeyFromString(std::string_view);
PAxis AxisFromString(std::string_view);

}
