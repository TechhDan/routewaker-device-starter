#include "DisplayManager.h"

#include <qrcode.h>

#include "config.h"

namespace {
constexpr uint16_t BackgroundColor = TFT_WHITE;
constexpr uint16_t BrandGreen = 0x1DEB;
constexpr uint16_t TextColor = 0x10C4;
constexpr uint8_t QrVersion = 4;
constexpr uint8_t QrQuietZoneModules = 4;
constexpr int16_t QrModuleSize = 4;
}  // namespace

void DisplayManager::begin() {
#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
#endif

  display_.init();
  display_.setRotation(1);
  display_.invertDisplay(true);
  display_.setTextWrap(false);
  Serial.println("[DISPLAY] ILI9341 display initialized");
}

void DisplayManager::showSplash(const String& hardwareId) {
  Serial.println("[DISPLAY] Splash screen");

  display_.fillScreen(BackgroundColor);
  display_.setTextDatum(MC_DATUM);
  display_.setTextColor(TextColor, BackgroundColor);
  display_.setTextSize(4);
  display_.drawString(Config::PRODUCT_NAME, display_.width() / 2,
                      (display_.height() / 2) - 18);
  display_.setTextSize(2);
  display_.drawString("Hardware ID", display_.width() / 2,
                      (display_.height() / 2) + 28);
  display_.setTextColor(BrandGreen, BackgroundColor);
  display_.drawString(hardwareId, display_.width() / 2,
                      (display_.height() / 2) + 52);
}

void DisplayManager::showProvisioning(const String& accessPointName,
                                      const IPAddress& portalIp) {
  Serial.println("[DISPLAY] Wi-Fi setup screen");
  Serial.printf("[DISPLAY] Connect to AP: %s\n", accessPointName.c_str());
  Serial.printf("[DISPLAY] Then open: http://%s\n", portalIp.toString().c_str());
  Serial.printf("[DISPLAY] QR payload: WIFI:T:nopass;S:%s;;\n", accessPointName.c_str());

  display_.fillScreen(BackgroundColor);
  display_.setTextColor(TextColor, BackgroundColor);
  display_.setTextDatum(TC_DATUM);
  display_.setTextSize(2);
  display_.drawString("Wi-Fi Setup", 78, 24);
  display_.setTextSize(1);
  display_.drawString("Scan to connect", 78, 68);
  display_.drawString("to setup network", 78, 84);
  display_.drawString("Then open", 78, 132);
  display_.drawString(portalIp.toString(), 78, 148);

  const String qrPayload =
      "WIFI:T:nopass;S:" + accessPointName + ";;";
  drawQrCode(qrPayload, 237, display_.height() / 2);
}

void DisplayManager::showConnected(const char* firmwareVersion,
                                   const IPAddress& deviceIp,
                                   const String& hardwareId) {
  Serial.println("[DISPLAY] Connected/version screen");
  Serial.println(Config::PRODUCT_NAME);
  Serial.println("Firmware Version");
  Serial.println(majorVersionLabel(firmwareVersion));
  Serial.printf("Wi-Fi: Connected (%s)\n", deviceIp.toString().c_str());
  Serial.printf("Hardware ID: %s\n", hardwareId.c_str());

  display_.fillScreen(BackgroundColor);
  display_.setTextDatum(MC_DATUM);
  display_.setTextColor(TextColor, BackgroundColor);
  display_.setTextSize(3);
  display_.drawString(Config::PRODUCT_NAME, display_.width() / 2, 48);
  display_.setTextSize(2);
  display_.drawString("Firmware Version", display_.width() / 2, 100);
  display_.setTextColor(BrandGreen, BackgroundColor);
  display_.setTextSize(4);
  display_.drawString(majorVersionLabel(firmwareVersion), display_.width() / 2,
                      150);
  display_.setTextColor(TextColor, BackgroundColor);
  display_.setTextSize(1);
  display_.drawString("Wi-Fi: Connected  " + deviceIp.toString(),
                      display_.width() / 2, 205);
  display_.drawString("Hardware ID: " + hardwareId, display_.width() / 2, 222);
}

void DisplayManager::showOtaStatus(const String& title, const String& detail) {
  display_.fillScreen(BackgroundColor);
  display_.setTextDatum(MC_DATUM);
  display_.setTextColor(TextColor, BackgroundColor);
  display_.setTextSize(3);
  display_.drawString(title, display_.width() / 2, 92);
  display_.setTextSize(2);
  display_.drawString(detail, display_.width() / 2, 142);
}

void DisplayManager::showOtaProgress(size_t received, size_t total) {
  const uint8_t percent = total == 0 ? 0 : (received * 100U) / total;
  display_.fillRect(20, 155, display_.width() - 40, 54, BackgroundColor);
  display_.drawRect(20, 160, display_.width() - 40, 20, TextColor);
  const int16_t progressWidth =
      ((display_.width() - 44) * static_cast<uint32_t>(percent)) / 100U;
  display_.fillRect(22, 162, progressWidth, 16, BrandGreen);
  display_.setTextDatum(MC_DATUM);
  display_.setTextColor(TextColor, BackgroundColor);
  display_.setTextSize(2);
  display_.drawString(String(percent) + "%", display_.width() / 2, 198);
}

void DisplayManager::drawQrCode(const String& value, int16_t centerX,
                                int16_t centerY) {
  QRCode qrCode;
  uint8_t qrData[qrcode_getBufferSize(QrVersion)];
  qrcode_initText(&qrCode, qrData, QrVersion, ECC_LOW, value.c_str());

  const int16_t renderedSize =
      (qrCode.size + (QrQuietZoneModules * 2)) * QrModuleSize;
  const int16_t originX = centerX - (renderedSize / 2);
  const int16_t originY = centerY - (renderedSize / 2);

  display_.fillRect(originX, originY, renderedSize, renderedSize, TFT_WHITE);
  for (uint8_t y = 0; y < qrCode.size; ++y) {
    for (uint8_t x = 0; x < qrCode.size; ++x) {
      if (qrcode_getModule(&qrCode, x, y)) {
        display_.fillRect(
            originX + ((x + QrQuietZoneModules) * QrModuleSize),
            originY + ((y + QrQuietZoneModules) * QrModuleSize), QrModuleSize,
            QrModuleSize, TFT_BLACK);
      }
    }
  }
}

String DisplayManager::majorVersionLabel(const char* semanticVersion) const {
  String version(semanticVersion);
  const int dotPosition = version.indexOf('.');
  if (dotPosition >= 0) {
    version.remove(dotPosition);
  }
  return "v" + version;
}
