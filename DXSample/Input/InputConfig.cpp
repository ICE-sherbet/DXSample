#include "InputConfig.h"
#include "nlohmann/json.hpp"
#include <Xinput.h>
#include <fstream>

namespace base {

// TODO: ハッシュを使うなど高速な検索を実装する
PKey KeyFromString(std::string_view s) {
  if (s[0] == 'K' && std::isalpha(s[3]))
    return static_cast<PKey>(std::toupper(s[3]));
  if (s == "Space") return PKey::Space;
  if (s == "Ctrl") return PKey::Ctrl;
  if (s == "Shift") return PKey::Shift;
  if (s == "Alt") return PKey::Alt;

  return PKey::None;
}
PAxis AxisFromString(std::string_view s) {
  if (s == "LX") return PAxis::LX;
  if (s == "LY") return PAxis::LY;
  if (s == "MouseX") return PAxis::MouseX;
  if (s == "MouseY") return PAxis::MouseY;
  if (s == "MDX") return PAxis::MDX;
  if (s == "MDY") return PAxis::MDY;
  if (s == "RT") return PAxis::RT;

  return PAxis::MouseX;
}

bool InputConfig::Load(const std::filesystem::path& fp, std::string* err) {
  std::ifstream ifs(fp);
  if (!ifs) {
    if (err) *err = "file open fail";
    return false;
  }

  
  nlohmann::json j;
  ifs >> j;

  bindings.clear();
  composites.clear();
  deadzone.clear();

  for (auto& jbindings : j["bindings"]) {
    InputBinding b;
    const std::string kind = jbindings["kind"];
#define KIND(s, e) \
  if (kind == s) b.kind = InputBinding::Kind::e
    KIND("DigitalToAxis", DigitalToAxis);
    else KIND("DigitalToAction", DigitalToAction);
    else KIND("AnalogToAxis", AnalogToAxis);
    else KIND("AnalogToAction", AnalogToAction);
    else KIND("ChordToAction", ChordToAction);
    else KIND("SequenceToAction", SequenceToAction);
    else {
      if (err) *err = "kind typo";
      return false;
    }

    b.virtualName = jbindings["virtual"].get<std::string>();
    b.scale = jbindings.value("scale", 1.f);
    b.device = jbindings.value("device", DEVICE_KEYBD);

    if (jbindings.contains("axis"))
      b.axis = AxisFromString(jbindings["axis"].get<std::string>());

    if (jbindings.contains("key"))
      b.keys.push_back(KeyFromString(jbindings["key"].get<std::string>()));
    if (jbindings.contains("keys"))
      for (auto& k : jbindings["keys"])
        b.keys.push_back(KeyFromString(k.get<std::string>()));

    b.threshold = jbindings.value("threshold", 0.25f);
    b.maxGapFrames = jbindings.value("maxGapFrames", 12);

    bindings.emplace_back(std::move(b));
  }

  if (j.contains("composites"))
    for (auto& [name, obj] : j["composites"].items()) {
      InputComposite c;
      if (obj.contains("x")) c.x = obj["x"].get<std::string>();
      if (obj.contains("y")) c.y = obj["y"].get<std::string>();
      if (obj.contains("z")) c.z = obj["z"].get<std::string>();
      composites.emplace(name, std::move(c));
    }

  if (j.contains("deadzone"))
    for (auto& [k, v] : j["deadzone"].items())
      deadzone[AxisFromString(k)] = v.get<float>();

  latchReleaseFrames = j.value("latchReleaseFrames", 8);
  accelThreshold = j.value("accelThreshold", 0.02f);
  return true;
}
}