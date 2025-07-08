#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "RawInputBackend.h"

namespace base {

struct Binding {
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
  DeviceId device = DEV_KEYBD;

  std::vector<PKey> keys;
  std::optional<PAxis> axis;

  float threshold = 0.25f;

  int maxGapFrames = 12;
};


struct Composite {
  std::optional<std::string> x, y, z;
};

struct Config {
  std::vector<Binding> bindings;
  std::unordered_map<std::string, Composite> composites;
  std::unordered_map<PAxis, float> deadzone;
  int latchReleaseFrames = 8;
  float accelThreshold = 0.02f;

  bool load(const std::filesystem::path& fp, std::string* err = nullptr);
};

PKey keyFromString(std::string_view);
PAxis axisFromString(std::string_view);

}  // namespace base
