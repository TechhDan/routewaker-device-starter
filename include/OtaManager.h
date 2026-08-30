#pragma once

#include <Arduino.h>
#include <functional>

class OtaManager {
 public:
  enum class Result { Disabled, NoUpdate, Installed, Failed };
  using ProgressCallback = std::function<void(size_t received, size_t total)>;

  void configure(const String& platformUrl, const String& deviceToken);
  bool isConfigured() const;
  bool reportPendingInstallation();
  bool reportInstallation(uint64_t releaseId, const String& eventId);
  bool sendHeartbeat();
  Result checkAndInstall(const ProgressCallback& progress = nullptr);
  const String& availableVersion() const;
  const String& lastError() const;

 private:
  struct Manifest {
    uint32_t releaseId = 0;
    String version;
    String artifactUrl;
    String sha256;
    size_t artifactSize = 0;
  };

  bool fetchManifest(Manifest& manifest, bool& updateAvailable);
  bool downloadAndStage(const Manifest& manifest,
                        const ProgressCallback& progress);
  bool sendEvent(uint64_t releaseId, const char* eventType,
                 const String& failureCode = "",
                 const String& failureMessage = "",
                 const String& eventId = "");
  String endpoint(const char* path) const;

  String availableVersion_;
  String lastError_;
  String platformUrl_;
  String deviceToken_;
};
