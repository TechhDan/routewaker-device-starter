#include "DeviceConfiguration.h"

#include <Preferences.h>

namespace {
constexpr char Namespace[] = "rw-device";
}  // namespace

bool DeviceConfiguration::isValid() const {
  return !platformUrl.isEmpty() && deviceToken.startsWith("rwd_");
}

bool DeviceConfiguration::load(DeviceConfiguration& configuration) {
  Preferences preferences;
  if (!preferences.begin(Namespace, true)) return false;
  configuration.platformUrl = preferences.getString("platform", "");
  configuration.deviceToken = preferences.getString("device_token", "");
  preferences.end();
  return configuration.isValid();
}

bool DeviceConfiguration::save(const String& platformUrl,
                               const String& deviceToken) {
  DeviceConfiguration candidate{platformUrl, deviceToken};
  if (!candidate.isValid()) return false;
  Preferences preferences;
  if (!preferences.begin(Namespace, false)) return false;
  const bool saved = preferences.putString("platform", platformUrl) ==
                         platformUrl.length() &&
                     preferences.putString("device_token", deviceToken) ==
                         deviceToken.length();
  preferences.end();
  DeviceConfiguration stored;
  return saved && load(stored) && stored.platformUrl == platformUrl &&
         stored.deviceToken == deviceToken;
}
