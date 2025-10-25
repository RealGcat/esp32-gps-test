#include <Arduino.h>
#include <U8g2lib.h>
#include "ATGM336H_GPS.h"

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 22, /* data=*/ 21, /* reset=*/ U8X8_PIN_NONE);


void setup() {
  Serial.begin(115200);
  
  u8g2.begin();
  u8g2.enableUTF8Print();
  
  GPS_BDS_Init();
}

void loop() {

  u8g2.setFont(u8g2_font_wqy12_t_gb2312);  // use chinese2
  u8g2.setFontDirection(0);
  //u8g2.clearBuffer();
  u8g2.setCursor(0, 12);

  gpsRead();	//获取GPS数据
	parseGpsBuffer();//解析GPS数据

  // 判断解析是否成功
	if (Save_Data.isParseData)
	{
		Save_Data.isParseData = false;
		
    // 世界标准时间，即格林威治时间，全球根据所在时区调整。
		u8g2.print("UTCTime=");
		u8g2.println(Save_Data.UTCTime);
    //u8g2.print(Save_Data.UTCTime);

		if(Save_Data.isUsefull)
		{
      // 打印经纬度信息 东西、南北半球信息
			Save_Data.isUsefull = false;
      u8g2.setCursor(0, 24);
			u8g2.print("latitude = ");     
			u8g2.println(Save_Data.latitude);
      u8g2.setCursor(0, 36);
			u8g2.print("N_S = ");
			u8g2.println(Save_Data.N_S);
      u8g2.setCursor(0, 48);
			u8g2.print("longitude = ");
			u8g2.println(Save_Data.longitude);
      u8g2.setCursor(0, 60);
			u8g2.print("E_W = ");
			u8g2.println(Save_Data.E_W);
      u8g2.sendBuffer();
      u8g2.clearBuffer();
		}
		else
		{
      u8g2.setCursor(0, 64);
			u8g2.println("GPS DATA isn't usefull!");
      u8g2.sendBuffer();
		}
		
	}

  u8g2.sendBuffer();

}