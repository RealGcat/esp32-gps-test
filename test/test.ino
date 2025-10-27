#include <Arduino.h>
#include <stdio.h>
#include <U8g2lib.h>
#include "ATGM336H_GPS.h"
 
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
 
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 22, /* data=*/ 21, /* reset=*/ U8X8_PIN_NONE);
 
// 保存上一次显示的GPS数据，用于变化检测
struct PreviousGPSDisplay {
  char UTCTime[11];
  char latitude[11];
  char N_S[2];
  char longitude[12];
  char E_W[2];
  bool isUsefull;
  bool hasData;
} prevDisplay;
 
// 比较GPS数据是否发生变化
bool isGPSDataChanged() {
  if (!prevDisplay.hasData) {
    return true;
  }
  
  if (strcmp(prevDisplay.UTCTime, Save_Data.UTCTime) != 0) return true;
  if (strcmp(prevDisplay.latitude, Save_Data.latitude) != 0) return true;
  if (strcmp(prevDisplay.N_S, Save_Data.N_S) != 0) return true;
  if (strcmp(prevDisplay.longitude, Save_Data.longitude) != 0) return true;
  if (strcmp(prevDisplay.E_W, Save_Data.E_W) != 0) return true;
  if (prevDisplay.isUsefull != Save_Data.isUsefull) return true;
  
  return false;
}
 
// 保存当前GPS数据到上一次显示状态
void saveCurrentGPSData() {
  strcpy(prevDisplay.UTCTime, Save_Data.UTCTime);
  strcpy(prevDisplay.latitude, Save_Data.latitude);
  strcpy(prevDisplay.N_S, Save_Data.N_S);
  strcpy(prevDisplay.longitude, Save_Data.longitude);
  strcpy(prevDisplay.E_W, Save_Data.E_W);
  prevDisplay.isUsefull = Save_Data.isUsefull;
  prevDisplay.hasData = true;
}

bool convertUTCToBeijingTime(const char* utcTime, char* output, size_t outputSize) {
  if (output == NULL || utcTime == NULL || outputSize < 9) {
    return false;
  }

  for (int i = 0; i < 6; ++i) {
    char c = utcTime[i];
    if (c == '\0' || c < '0' || c > '9') {
      return false;
    }
  }

  int hour = (utcTime[0] - '0') * 10 + (utcTime[1] - '0');
  int minute = (utcTime[2] - '0') * 10 + (utcTime[3] - '0');
  int second = (utcTime[4] - '0') * 10 + (utcTime[5] - '0');

  if (hour > 23 || minute > 59 || second > 59) {
    return false;
  }

  int totalSeconds = hour * 3600 + minute * 60 + second;
  totalSeconds = (totalSeconds + 8 * 3600) % (24 * 3600);

  int beijingHour = totalSeconds / 3600;
  int beijingMinute = (totalSeconds % 3600) / 60;
  int beijingSecond = totalSeconds % 60;

  output[0] = '0' + (beijingHour / 10);
  output[1] = '0' + (beijingHour % 10);
  output[2] = ':';
  output[3] = '0' + (beijingMinute / 10);
  output[4] = '0' + (beijingMinute % 10);
  output[5] = ':';
  output[6] = '0' + (beijingSecond / 10);
  output[7] = '0' + (beijingSecond % 10);
  output[8] = '\0';

  return true;
}

bool convertCoordinateToDMS(const char* coordinate, char hemisphere, char* output, size_t outputSize) {
  if (coordinate == NULL || output == NULL || outputSize < 16) {
    return false;
  }

  if (coordinate[0] == '\0') {
    output[0] = '\0';
    return false;
  }

  double rawValue = atof(coordinate);
  if (rawValue < 0.0) {
    rawValue = -rawValue;
  }

  int degrees = rawValue / 100.0;
  double minutesFull = rawValue - degrees * 100.0;

  int minutes = minutesFull;
  double seconds = (minutesFull - minutes) * 60.0;

  if (seconds >= 59.995) {
    seconds = 0.0;
    minutes += 1;
    if (minutes >= 60) {
      minutes = 0;
      degrees += 1;
    }
  }

  char hemisphereChar = hemisphere;
  if (hemisphereChar >= 'a' && hemisphereChar <= 'z') {
    hemisphereChar -= ('a' - 'A');
  }
  if (hemisphereChar != 'N' && hemisphereChar != 'S' &&
      hemisphereChar != 'E' && hemisphereChar != 'W') {
    hemisphereChar = ' ';
  }

  int written = snprintf(output, outputSize, "%d°%02d'%05.2f\"%c", degrees, minutes, seconds, hemisphereChar);
  if (written < 0 || (size_t)written >= outputSize) {
    output[0] = '\0';
    return false;
  }

  return true;
}
 
void setup() {
  Serial.begin(115200);
  
  u8g2.begin();
  u8g2.enableUTF8Print();
  
  GPS_BDS_Init();
  
  // 初始化上一次显示状态
  memset(&prevDisplay, 0, sizeof(prevDisplay));
  prevDisplay.hasData = false;
}
 
void loop() {
  gpsRead();    //获取GPS数据
    parseGpsBuffer();//解析GPS数据
 
  // 判断解析是否成功
    if (Save_Data.isParseData)
    {
        Save_Data.isParseData = false;
        
        // 检查GPS数据是否发生变化
        if (isGPSDataChanged())
        {
            // 数据有变化，需要更新显示
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_wqy12_t_gb2312);
            u8g2.setFontDirection(0);
            
            // 世界标准时间，即格林威治时间，全球根据所在时区调整。
            u8g2.setCursor(0, 12);
            char beijingTime[9];
            if (convertUTCToBeijingTime(Save_Data.UTCTime, beijingTime, sizeof(beijingTime)))
            {
                u8g2.print("北京时间=");
                u8g2.println(beijingTime);
            }
            else
            {
                u8g2.print("UTC时间=");
                u8g2.println(Save_Data.UTCTime);
            }

            if(Save_Data.isUsefull)
            {
                // 打印经纬度信息 东西、南北半球信息
                char latitudeDMS[24];
                char longitudeDMS[24];
                bool hasLatitudeDMS = convertCoordinateToDMS(Save_Data.latitude, Save_Data.N_S[0], latitudeDMS, sizeof(latitudeDMS));
                bool hasLongitudeDMS = convertCoordinateToDMS(Save_Data.longitude, Save_Data.E_W[0], longitudeDMS, sizeof(longitudeDMS));

                u8g2.setCursor(0, 24);
                u8g2.print("纬度 = ");     
                if (hasLatitudeDMS)
                {
                    u8g2.println(latitudeDMS);
                }
                else
                {
                    u8g2.println(Save_Data.latitude);
                }

                u8g2.setCursor(0, 36);
                u8g2.print("经度 = ");
                if (hasLongitudeDMS)
                {
                    u8g2.println(longitudeDMS);
                }
                else
                {
                    u8g2.println(Save_Data.longitude);
                }

                u8g2.setCursor(0, 48);
                u8g2.print("N_S = ");
                u8g2.println(Save_Data.N_S);
                u8g2.setCursor(0, 60);
                u8g2.print("E_W = ");
                u8g2.println(Save_Data.E_W);
            }
            else
            {
                u8g2.setCursor(0, 24);
                u8g2.println("GPS DATA isn't usefull!");
            }
            
            // 保存当前数据作为下一次比较的基准
            saveCurrentGPSData();
            
            // 只在数据变化时才发送缓冲区到屏幕
            u8g2.sendBuffer();
        }
        // 如果数据没有变化，则跳过所有显示更新操作
    }
}
 
