#include "ProvisioningHandoff.h"

#include <Preferences.h>

namespace {
constexpr char Namespace[] = "rw-prov";
constexpr char BootstrapNamespace[] = "rw-bootstrap";
}  // namespace

bool ProvisioningHandoff::isPending() {
  Preferences preferences;
  if (!preferences.begin(Namespace, true)) return false;
  const bool pending = preferences.getBool("pending", false);
  preferences.end();
  return pending;
}

bool ProvisioningHandoff::load(ProvisioningHandoff& handoff) {
  Preferences preferences;
  if (!preferences.begin(Namespace, true)) return false;
  const bool pending = preferences.getBool("pending", false);
  handoff.ssid = preferences.getString("ssid", "");
  handoff.password = preferences.getString("password", "");
  handoff.platformUrl = preferences.getString("platform", "");
  handoff.deviceToken = preferences.getString("device_token", "");
  handoff.releaseId = preferences.getULong64("release_id", 0);
  handoff.expectedVersion = preferences.getString("expected_ver", "");
  handoff.eventId = preferences.getString("event_id", "");
  preferences.end();
  return pending && !handoff.ssid.isEmpty() && !handoff.platformUrl.isEmpty() &&
         handoff.deviceToken.startsWith("rwd_") && handoff.releaseId != 0 &&
         !handoff.expectedVersion.isEmpty();
}

bool ProvisioningHandoff::saveEventId(const String& eventId) {
  Preferences preferences;
  if (!preferences.begin(Namespace, false)) return false;
  const size_t written = preferences.putString("event_id", eventId);
  preferences.end();
  return written == eventId.length();
}

bool ProvisioningHandoff::clear() {
  Preferences preferences;
  if (!preferences.begin(Namespace, false)) return false;
  const bool cleared = preferences.clear();
  preferences.end();
  return cleared;
}

bool ProvisioningHandoff::clearBootstrap() {
  Preferences preferences;
  if (!preferences.begin(BootstrapNamespace, false)) return false;
  const bool cleared = preferences.clear();
  preferences.end();
  return cleared;
}
