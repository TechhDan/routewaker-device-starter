#include "OtaManager.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <Update.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <mbedtls/sha256.h>

#include "config.h"

namespace {
constexpr char OtaPreferencesNamespace[] = "routewaker-ota";
constexpr char PendingReleaseKey[] = "release";
constexpr char PendingVersionKey[] = "version";
constexpr char PendingEventKey[] = "event_id";

bool validSha256(const String& value) {
  if (value.length() != 64) return false;
  for (size_t index = 0; index < value.length(); ++index) {
    const char character = value[index];
    if (!isxdigit(static_cast<unsigned char>(character))) return false;
  }
  return true;
}

String sha256Hex(const uint8_t digest[32]) {
  char output[65];
  for (size_t index = 0; index < 32; ++index) {
    snprintf(output + (index * 2), 3, "%02x", digest[index]);
  }
  output[64] = '\0';
  return String(output);
}

String eventUuid() {
  uint8_t bytes[16];
  esp_fill_random(bytes, sizeof(bytes));
  bytes[6] = (bytes[6] & 0x0F) | 0x40;
  bytes[8] = (bytes[8] & 0x3F) | 0x80;
  char output[37];
  snprintf(output, sizeof(output),
           "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-"
           "%02x%02x%02x%02x%02x%02x",
           bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5],
           bytes[6], bytes[7], bytes[8], bytes[9], bytes[10], bytes[11],
           bytes[12], bytes[13], bytes[14], bytes[15]);
  return String(output);
}

void configureTls(WiFiClientSecure& client) {
  if (strlen(Config::OTA_ROOT_CA) > 0) {
    client.setCACert(Config::OTA_ROOT_CA);
  } else {
    client.setInsecure();
    Serial.println("[OTA] WARNING: TLS certificate verification is disabled");
  }
}

template <typename Client>
bool beginRequest(HTTPClient& http, Client& client, const String& url) {
  http.setConnectTimeout(Config::OTA_REQUEST_TIMEOUT_MS);
  http.setTimeout(Config::OTA_REQUEST_TIMEOUT_MS);
  return http.begin(client, url);
}

void addApiHeaders(HTTPClient& http, const String& deviceToken) {
  http.addHeader("Accept", "application/json");
  http.addHeader("Authorization", "Bearer " + deviceToken);
}
}  // namespace

void OtaManager::configure(const String& platformUrl, const String& deviceToken) {
  platformUrl_ = platformUrl;
  deviceToken_ = deviceToken;
}

bool OtaManager::isConfigured() const {
  return !platformUrl_.isEmpty() && deviceToken_.startsWith("rwd_");
}

bool OtaManager::reportInstallation(uint64_t releaseId, const String& eventId) {
  return sendEvent(releaseId, "installation_succeeded", "", "", eventId);
}

bool OtaManager::reportPendingInstallation() {
  if (!isConfigured()) return true;

  Preferences preferences;
  if (!preferences.begin(OtaPreferencesNamespace, false)) return false;
  const uint64_t releaseId = preferences.getULong64(PendingReleaseKey, 0);
  const String expectedVersion = preferences.getString(PendingVersionKey, "");
  String pendingEventId = preferences.getString(PendingEventKey, "");
  preferences.end();

  if (releaseId == 0) return true;
  if (expectedVersion != Config::FIRMWARE_VERSION) {
    Serial.printf("[OTA] Pending version %s did not boot; running %s\n",
                  expectedVersion.c_str(), Config::FIRMWARE_VERSION);
    return false;
  }

  if (pendingEventId.isEmpty()) {
    pendingEventId = eventUuid();
    if (!preferences.begin(OtaPreferencesNamespace, false)) return false;
    const bool saved = preferences.putString(PendingEventKey, pendingEventId) ==
                       pendingEventId.length();
    preferences.end();
    if (!saved) return false;
  }
  if (!sendEvent(releaseId, "installation_succeeded", "", "",
                 pendingEventId)) return false;
  if (preferences.begin(OtaPreferencesNamespace, false)) {
    preferences.clear();
    preferences.end();
  }
  Serial.printf("[OTA] Confirmed installation of version %s\n",
                Config::FIRMWARE_VERSION);
  return true;
}

OtaManager::Result OtaManager::checkAndInstall(
    const ProgressCallback& progress) {
  availableVersion_ = "";
  lastError_ = "";
  if (!isConfigured()) return Result::Disabled;
  if (!sendHeartbeat()) return Result::Failed;

  Manifest manifest;
  bool updateAvailable = false;
  if (!fetchManifest(manifest, updateAvailable)) return Result::Failed;
  if (!updateAvailable) return Result::NoUpdate;

  availableVersion_ = manifest.version;
  sendEvent(manifest.releaseId, "download_started");
  if (!downloadAndStage(manifest, progress)) {
    sendEvent(manifest.releaseId, "installation_failed", "ota_install_failed",
              lastError_);
    return Result::Failed;
  }
  Preferences preferences;
  if (!preferences.begin(OtaPreferencesNamespace, false)) {
    const esp_err_t recovery =
        esp_ota_set_boot_partition(esp_ota_get_running_partition());
    lastError_ = recovery == ESP_OK
                     ? "Unable to save pending update state; update not activated."
                     : "Unable to save pending update state or restore the current boot image.";
    return Result::Failed;
  }
  const String pendingEventId = eventUuid();
  const bool releaseSaved =
      preferences.putULong64(PendingReleaseKey, manifest.releaseId) ==
      sizeof(manifest.releaseId);
  const bool versionSaved =
      preferences.putString(PendingVersionKey, manifest.version) ==
      manifest.version.length();
  const bool eventSaved =
      preferences.putString(PendingEventKey, pendingEventId) ==
      pendingEventId.length();
  if (!releaseSaved || !versionSaved || !eventSaved) {
    preferences.clear();
    preferences.end();
    const esp_err_t recovery =
        esp_ota_set_boot_partition(esp_ota_get_running_partition());
    lastError_ = recovery == ESP_OK
                     ? "Unable to save all pending update state; update not activated."
                     : "Unable to save pending update state or restore the current boot image.";
    return Result::Failed;
  }
  preferences.end();
  sendEvent(manifest.releaseId, "download_completed");
  return Result::Installed;
}

const String& OtaManager::availableVersion() const { return availableVersion_; }

const String& OtaManager::lastError() const { return lastError_; }

bool OtaManager::sendHeartbeat() {
  JsonDocument document;
  document["firmwareVersion"] = Config::FIRMWARE_VERSION;
  String body;
  serializeJson(document, body);

  HTTPClient http;
  const String url = endpoint("/api/device/heartbeat");
  WiFiClient plainClient;
  WiFiClientSecure secureClient;
  int statusCode = -1;
  if (url.startsWith("https://")) {
    configureTls(secureClient);
    if (!beginRequest(http, secureClient, url)) return false;
  } else {
    if (!beginRequest(http, plainClient, url)) return false;
  }
  addApiHeaders(http, deviceToken_);
  http.addHeader("Content-Type", "application/json");
  statusCode = http.POST(body);
  http.end();
  if (statusCode < 200 || statusCode >= 300) {
    lastError_ = "Heartbeat failed (" + String(statusCode) + ").";
    return false;
  }
  Serial.println("[OTA] Heartbeat accepted");
  return true;
}

bool OtaManager::fetchManifest(Manifest& manifest, bool& updateAvailable) {
  updateAvailable = false;
  HTTPClient http;
  const String url = endpoint("/api/device/firmware-update");
  WiFiClient plainClient;
  WiFiClientSecure secureClient;
  int statusCode = -1;
  String response;
  if (url.startsWith("https://")) {
    configureTls(secureClient);
    if (!beginRequest(http, secureClient, url)) return false;
  } else {
    if (!beginRequest(http, plainClient, url)) return false;
  }
  addApiHeaders(http, deviceToken_);
  statusCode = http.GET();
  if (statusCode > 0) response = http.getString();
  http.end();

  if (statusCode != HTTP_CODE_OK) {
    lastError_ = "Update check failed (" + String(statusCode) + ").";
    return false;
  }

  JsonDocument document;
  if (deserializeJson(document, response)) {
    lastError_ = "The update response was not valid JSON.";
    return false;
  }
  updateAvailable = document["updateAvailable"] | false;
  if (!updateAvailable) return true;

  JsonObject release = document["firmwareRelease"];
  manifest.releaseId = release["id"].as<uint64_t>();
  manifest.version = String(release["version"] | "");
  manifest.artifactUrl = String(release["artifactUrl"] | "");
  manifest.artifactSize = release["artifactSize"] | 0;
  manifest.sha256 = String(release["sha256"] | "");
  manifest.sha256.toLowerCase();

  const esp_partition_t* partition = esp_ota_get_next_update_partition(nullptr);
  if (release.isNull() || manifest.releaseId == 0 || manifest.version.isEmpty() ||
      manifest.version == Config::FIRMWARE_VERSION ||
      manifest.artifactUrl.isEmpty() || manifest.artifactSize == 0 ||
      !validSha256(manifest.sha256) || partition == nullptr ||
      manifest.artifactSize > partition->size) {
    lastError_ = "The update manifest is incomplete or incompatible.";
    return false;
  }
  Serial.printf("[OTA] Update available: %s (%u bytes)\n",
                manifest.version.c_str(), manifest.artifactSize);
  return true;
}

bool OtaManager::downloadAndStage(const Manifest& manifest,
                                  const ProgressCallback& progress) {
  HTTPClient http;
  http.setConnectTimeout(Config::OTA_REQUEST_TIMEOUT_MS);
  http.setTimeout(Config::OTA_DOWNLOAD_TIMEOUT_MS);
  int statusCode = -1;
  WiFiClient* stream = nullptr;
  WiFiClient plainClient;
  WiFiClientSecure secureClient;

  if (manifest.artifactUrl.startsWith("https://")) {
    configureTls(secureClient);
    if (!http.begin(secureClient, manifest.artifactUrl)) return false;
  } else if (!http.begin(plainClient, manifest.artifactUrl)) {
    return false;
  }
  addApiHeaders(http, deviceToken_);
  statusCode = http.GET();
  if (statusCode != HTTP_CODE_OK) {
    lastError_ = "Firmware download failed (" + String(statusCode) + ").";
    http.end();
    return false;
  }
  stream = http.getStreamPtr();

  if (!Update.begin(manifest.artifactSize, U_FLASH)) {
    lastError_ = String("Unable to stage update: ") + Update.errorString();
    http.end();
    return false;
  }

  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  mbedtls_sha256_starts_ret(&sha, 0);
  uint8_t buffer[2048];
  size_t received = 0;
  unsigned long lastDataAt = millis();
  bool failed = false;
  while (received < manifest.artifactSize) {
    const size_t available = stream->available();
    if (available == 0) {
      if (!http.connected() ||
          millis() - lastDataAt > Config::OTA_DOWNLOAD_TIMEOUT_MS) {
        lastError_ = "Firmware download was interrupted.";
        failed = true;
        break;
      }
      delay(1);
      continue;
    }
    const size_t wanted = min(sizeof(buffer),
                              min(available, manifest.artifactSize - received));
    const int count = stream->readBytes(buffer, wanted);
    if (count <= 0) continue;
    lastDataAt = millis();
    mbedtls_sha256_update_ret(&sha, buffer, count);
    if (Update.write(buffer, count) != static_cast<size_t>(count)) {
      lastError_ = String("Flash write failed: ") + Update.errorString();
      failed = true;
      break;
    }
    received += count;
    if (progress) progress(received, manifest.artifactSize);
  }

  uint8_t digest[32];
  mbedtls_sha256_finish_ret(&sha, digest);
  mbedtls_sha256_free(&sha);
  http.end();

  if (failed || received != manifest.artifactSize) {
    Update.abort();
    return false;
  }
  if (sha256Hex(digest) != manifest.sha256) {
    lastError_ = "Firmware SHA-256 verification failed.";
    Update.abort();
    return false;
  }
  if (!Update.end() || !Update.isFinished()) {
    lastError_ = String("Update activation failed: ") + Update.errorString();
    return false;
  }
  return true;
}

bool OtaManager::sendEvent(uint64_t releaseId, const char* eventType,
                           const String& failureCode,
                           const String& failureMessage,
                           const String& eventId) {
  JsonDocument document;
  document["eventId"] = eventId.isEmpty() ? eventUuid() : eventId;
  document["firmwareReleaseId"] = releaseId;
  document["eventType"] = eventType;
  if (!failureCode.isEmpty()) document["failureCode"] = failureCode;
  if (!failureMessage.isEmpty()) document["failureMessage"] = failureMessage;
  String body;
  serializeJson(document, body);

  HTTPClient http;
  const String url = endpoint("/api/device/firmware-update-events");
  WiFiClient plainClient;
  WiFiClientSecure secureClient;
  int statusCode = -1;
  if (url.startsWith("https://")) {
    configureTls(secureClient);
    if (!beginRequest(http, secureClient, url)) return false;
  } else {
    if (!beginRequest(http, plainClient, url)) return false;
  }
  addApiHeaders(http, deviceToken_);
  http.addHeader("Content-Type", "application/json");
  statusCode = http.POST(body);
  http.end();
  return statusCode >= 200 && statusCode < 300;
}

String OtaManager::endpoint(const char* path) const {
  String baseUrl(platformUrl_);
  while (baseUrl.endsWith("/")) baseUrl.remove(baseUrl.length() - 1);
  return baseUrl + path;
}
