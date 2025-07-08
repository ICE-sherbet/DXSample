#include "InputConfig.h"
#include "nlohmann/json.hpp"
#include <Xinput.h>
#include <fstream>

namespace base {
//---------------------------------------------------------------
PKey keyFromString(std::string_view s) {
  if (s[0] == 'K' && std::isalpha(s[3]))
    return static_cast<PKey>(std::toupper(s[3]));
  if (s == "Space") return PKey::Space;
  if (s == "Ctrl") return PKey::Ctrl;
  if (s == "Shift") return PKey::Shift;
  if (s == "Alt") return PKey::Alt;

  return PKey::None;
}
PAxis axisFromString(std::string_view s) {
  if (s == "LX") return PAxis::LX;
  if (s == "LY") return PAxis::LY;
  if (s == "MouseX") return PAxis::MouseX;
  if (s == "MouseY") return PAxis::MouseY;
  if (s == "MDX") return PAxis::MDX;
  if (s == "MDY") return PAxis::MDY;
  if (s == "RT") return PAxis::RT;

  /* c’Ç‰Á c */
  return PAxis::MouseX;
}

//---------------------------------------------------------------
bool Config::load(const std::filesystem::path& fp, std::string* err) {
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

  //------------ bindings -------------------------------------
  for (auto& jb : j["bindings"]) {
    Binding b;
    const std::string kind = jb["kind"];
#define KIND(s, e) \
  if (kind == s) b.kind = Binding::Kind::e
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

    b.virtualName = jb["virtual"].get<std::string>();
    b.scale = jb.value("scale", 1.f);
    b.device = jb.value("device", DEV_KEYBD);

    if (jb.contains("axis"))
      b.axis = axisFromString(jb["axis"].get<std::string>());

    if (jb.contains("key"))
      b.keys.push_back(keyFromString(jb["key"].get<std::string>()));
    if (jb.contains("keys"))
      for (auto& k : jb["keys"])
        b.keys.push_back(keyFromString(k.get<std::string>()));

    b.threshold = jb.value("threshold", 0.25f);
    b.maxGapFrames = jb.value("maxGapFrames", 12);

    bindings.emplace_back(std::move(b));
  }

  //------------ composites -----------------------------------
  if (j.contains("composites"))
    for (auto& [name, obj] : j["composites"].items()) {
      Composite c;
      if (obj.contains("x")) c.x = obj["x"].get<std::string>();
      if (obj.contains("y")) c.y = obj["y"].get<std::string>();
      if (obj.contains("z")) c.z = obj["z"].get<std::string>();
      composites.emplace(name, std::move(c));
    }

  //------------ misc -----------------------------------------
  if (j.contains("deadzone"))
    for (auto& [k, v] : j["deadzone"].items())
      deadzone[axisFromString(k)] = v.get<float>();

  latchReleaseFrames = j.value("latchReleaseFrames", 8);
  accelThreshold = j.value("accelThreshold", 0.02f);
  return true;
}
}  // namespace base