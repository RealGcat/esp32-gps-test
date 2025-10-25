#include <Arduino.h>
#include <U8g2lib.h>
#include <stdio.h>
#include <string.h>
#include "ATGM336H_GPS.h"

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 22, /* data=*/ 21, /* reset=*/ U8X8_PIN_NONE);

namespace {
  constexpr uint8_t kScreenWidth = 128;
  constexpr uint8_t kScreenHeight = 64;
  constexpr uint8_t kTileWidth = kScreenWidth / 8;
  constexpr uint8_t kLineHeight = 16;

  enum DisplayLine : uint8_t {
    kUtcLine,
    kLatitudeLine,
    kNsLine,
    kLongitudeLine,
    kEwLine,
    kStatusLine,
    kDisplayLineCount
  };

  const uint8_t kLineBaseline[kDisplayLineCount] = {12, 24, 36, 48, 60, 64};
  char lastLineContent[kDisplayLineCount][32];

  void copyLineContent(DisplayLine line, const char *text) {
    strncpy(lastLineContent[line], text, sizeof(lastLineContent[line]) - 1);
    lastLineContent[line][sizeof(lastLineContent[line]) - 1] = '\0';
  }

  void drawLine(DisplayLine line, const char *text) {
    uint8_t top = kLineBaseline[line] > kLineHeight ? static_cast<uint8_t>(kLineBaseline[line] - kLineHeight) : 0;
    uint8_t height = kLineHeight;
    if (top + height > kScreenHeight) {
      height = kScreenHeight - top;
    }

    u8g2.setDrawColor(0);
    u8g2.drawBox(0, top, kScreenWidth, height);
    u8g2.setDrawColor(1);

    if (text[0] != '\0') {
      u8g2.setCursor(0, kLineBaseline[line]);
      u8g2.print(text);
    }

    uint8_t tileY = top / 8;
    uint8_t tileHeight = (height + 7) / 8;
    if (tileHeight == 0) {
      tileHeight = 1;
    }
    u8g2.updateDisplayArea(0, tileY, kTileWidth, tileHeight);
  }

  void updateLine(DisplayLine line, const char *text) {
    if (strncmp(lastLineContent[line], text, sizeof(lastLineContent[line])) == 0) {
      return;
    }

    drawLine(line, text);
    copyLineContent(line, text);
  }
}

void setup() {
  Serial.begin(115200);

  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.setFontDirection(0);
  u8g2.clearBuffer();
  u8g2.sendBuffer();
  u8g2.setDrawColor(1);

  GPS_BDS_Init();
}

void loop() {
  gpsRead();
  parseGpsBuffer();

  if (!Save_Data.isParseData) {
    return;
  }

  Save_Data.isParseData = false;

  Save_Data.UTCTime[sizeof(Save_Data.UTCTime) - 1] = '\0';
  Save_Data.latitude[sizeof(Save_Data.latitude) - 1] = '\0';
  Save_Data.N_S[sizeof(Save_Data.N_S) - 1] = '\0';
  Save_Data.longitude[sizeof(Save_Data.longitude) - 1] = '\0';
  Save_Data.E_W[sizeof(Save_Data.E_W) - 1] = '\0';

  char lineBuffer[32];
  snprintf(lineBuffer, sizeof(lineBuffer), "UTCTime=%s", Save_Data.UTCTime);
  updateLine(kUtcLine, lineBuffer);

  if (Save_Data.isUsefull) {
    char latBuffer[32];
    snprintf(latBuffer, sizeof(latBuffer), "latitude = %s", Save_Data.latitude);
    updateLine(kLatitudeLine, latBuffer);

    char nsBuffer[16];
    snprintf(nsBuffer, sizeof(nsBuffer), "N_S = %s", Save_Data.N_S);
    updateLine(kNsLine, nsBuffer);

    char lonBuffer[32];
    snprintf(lonBuffer, sizeof(lonBuffer), "longitude = %s", Save_Data.longitude);
    updateLine(kLongitudeLine, lonBuffer);

    char ewBuffer[16];
    snprintf(ewBuffer, sizeof(ewBuffer), "E_W = %s", Save_Data.E_W);
    updateLine(kEwLine, ewBuffer);

    updateLine(kStatusLine, "");
    Save_Data.isUsefull = false;
  } else {
    updateLine(kLatitudeLine, "");
    updateLine(kNsLine, "");
    updateLine(kLongitudeLine, "");
    updateLine(kEwLine, "");
    updateLine(kStatusLine, "GPS DATA isn't usefull!");
  }
}
