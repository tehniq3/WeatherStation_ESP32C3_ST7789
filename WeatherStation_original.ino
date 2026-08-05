// https://www.youtube.com/watch?v=Boz2X6QPp44&t=8s
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "time.h"

#define TFT_SCK      4
#define TFT_MOSI     6
#define TFT_CS       7
#define TFT_DC       3
#define TFT_RST     10

#define MY_SKY_BLUE  0x867D  
#define MY_TEXT_DARK 0x0000  
#define SHADOW_COLOR 0x52AA 
#define WHITE_COLOR  0xFFFF

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST);

const char* ssid = "111111";
const char* password = "11111111";
const char* weatherApiKey = "11111111111";

WiFiClientSecure client;

float lastTemp = 0, lastWs = 0, lastPres = 0;
int lastHumidity = 0;
String lastWeatherMain = "N/A";
int lastForecast[5] = {0,0,0,0,0};
String lastFIcons[5] = {"","","","",""};

void setup() {
  Serial.begin(115200);
  tft.init(240, 280);
  tft.setRotation(1); 
  tft.fillScreen(MY_SKY_BLUE);
  tft.setTextSize(2);
  tft.setCursor(60, 110);
  tft.print("Connecting...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  configTime(7200, 3600, "pool.ntp.org");
  client.setInsecure();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) { updateWeatherAndForecast(); }
  delay(300000); 
}

void drawAdvancedIcon(int x, int y, String type) {
  int ox = 2; int oy = 2;
  if (type == "Clear") {
    tft.fillCircle(x+ox, y+oy, 7, SHADOW_COLOR);
    tft.fillCircle(x, y, 7, 0xFFE0);
    tft.drawCircle(x, y, 7, 0xFD20);
    for(int i=0; i<360; i+=45) {
      float rad = i * 0.01745;
      tft.drawLine(x+cos(rad)*9, y+sin(rad)*9, x+cos(rad)*12, y+sin(rad)*12, 0xFFE0);
    }
  } 
  else if (type == "Clouds") {
    tft.fillCircle(x-5+ox, y+2+oy, 6, SHADOW_COLOR);
    tft.fillCircle(x+5+ox, y+2+oy, 6, SHADOW_COLOR);
    tft.fillCircle(x+ox, y-3+oy, 7, SHADOW_COLOR);
    tft.fillCircle(x-5, y+2, 6, WHITE_COLOR);
    tft.fillCircle(x+5, y+2, 6, WHITE_COLOR);
    tft.fillCircle(x, y-3, 7, WHITE_COLOR);
  } 
  else if (type == "Rain" || type == "Drizzle") {
    tft.fillRoundRect(x-8+ox, y-5+oy, 16, 8, 4, SHADOW_COLOR);
    tft.fillRoundRect(x-8, y-5, 16, 8, 4, 0x7BEF);
    tft.drawLine(x-3, y+5, x-5, y+10, 0x001F);
    tft.drawLine(x+2, y+5, x, y+10, 0x001F);
  } 
  else if (type == "Snow") {
    tft.fillCircle(x+ox, y+oy, 5, SHADOW_COLOR);
    for(int i=0; i<360; i+=60) {
      float rad = i * 0.01745;
      tft.drawLine(x, y, x + (int)(cos(rad)*10), y + (int)(sin(rad)*10), WHITE_COLOR);
    }
  } 
  else if (type == "Thunderstorm") {
    tft.fillRoundRect(x-8+ox, y-5+oy, 16, 8, 4, SHADOW_COLOR);
    tft.fillRoundRect(x-8, y-5, 16, 8, 4, 0x4208);
    tft.drawLine(x+1, y+3, x-2, y+12, 0xFFE0);
  }
  else {
    tft.drawFastHLine(x-10+ox, y+oy, 20, SHADOW_COLOR);
    tft.drawFastHLine(x-10, y, 20, WHITE_COLOR);
  }
}

void drawWiFiIcon(int x, int y, uint16_t color) {
  int baseY = y + 14; 
  for (int i = 1; i <= 4; i++) {
    tft.drawCircle(x, baseY, i*4, color);
    tft.drawCircle(x, baseY, i*4 - 1, color); 
  }
  tft.fillCircle(x, baseY, 2, color);
  tft.fillRect(x - 20, baseY + 1, 40, 20, MY_SKY_BLUE); 
  tft.fillTriangle(x-22, baseY+2, x-2, baseY, x-22, baseY-22, MY_SKY_BLUE);
  tft.fillTriangle(x+22, baseY+2, x+2, baseY, x+22, baseY-22, MY_SKY_BLUE);
}

void updateWeatherAndForecast() {
  HTTPClient http;
  String url = "https://api.openweathermap.org/data/2.5/weather?q=Lviv&appid=" + String(weatherApiKey) + "&units=metric";
  http.begin(client, url);
  if (http.GET() == 200) {
    JsonDocument doc; deserializeJson(doc, http.getStream());
    lastTemp = doc["main"]["temp"];
    lastWs = doc["wind"]["speed"];
    lastHumidity = doc["main"]["humidity"];
    lastPres = (float)doc["main"]["pressure"] * 0.750062;
    lastWeatherMain = doc["weather"][0]["main"].as<String>();
  }
  http.end();

  String fUrl = "https://api.openweathermap.org/data/2.5/forecast?q=Lviv&appid=" + String(weatherApiKey) + "&units=metric";
  http.begin(client, fUrl);
  if (http.GET() == 200) {
    JsonDocument fDoc; deserializeJson(fDoc, http.getStream());
    for(int i=0; i<5; i++) {
      int idx = (i * 8) + 7;
      lastForecast[i] = fDoc["list"][idx]["main"]["temp"];
      lastFIcons[i] = fDoc["list"][idx]["weather"][0]["main"].as<String>();
    }
  }
  http.end();
  drawUI();
}

void drawUI() {
  tft.fillScreen(MY_SKY_BLUE);
  int W = 280; 
  int16_t x1, y1; uint16_t w, h;
  struct tm ti;
  bool timeValid = getLocalTime(&ti);

  // HEADER
  tft.setTextSize(2); tft.setTextColor(MY_TEXT_DARK);
  tft.setCursor(18, 10); tft.print("LVIV");
  drawAdvancedIcon(85, 17, lastWeatherMain);

  int timeX = W - 80;
  if (timeValid) {
    drawWiFiIcon(timeX - 25, 10, MY_TEXT_DARK);
    tft.setCursor(timeX, 10);
    tft.printf("%02d:%02d", ti.tm_hour, ti.tm_min);
  }
  tft.drawFastHLine(10, 38, W-20, MY_TEXT_DARK);

  // --- ДОДАНО: ДАТА ТА ДЕНЬ ТИЖНЯ ПІД ГОДИННИКОМ ---
  if (timeValid) {
    tft.setTextSize(1);
    tft.setTextColor(MY_TEXT_DARK);
    const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    char dateBuffer[40];
    sprintf(dateBuffer, "%s, %d %s", days[ti.tm_wday], ti.tm_mday, months[ti.tm_mon]);
    
    tft.getTextBounds(dateBuffer, 0, 0, &x1, &y1, &w, &h);
    // Вирівнювання по правому краю під годинником
    tft.setCursor(W - w - 15, 45); 
    tft.print(dateBuffer);
  }

  // MAIN TEMP + DEGREE SHADOW
  tft.setTextSize(8); 
  String tempStr = String((int)round(lastTemp));
  tft.getTextBounds(tempStr.c_str(), 0, 0, &x1, &y1, &w, &h);
  int tempX = (W - w) / 2;
  
  tft.setTextColor(SHADOW_COLOR); // Тінь
  tft.setCursor(tempX + 3, 68); tft.print(tempStr);
  tft.setTextSize(3); tft.print("o");
  
  tft.setTextColor(WHITE_COLOR); // Основний
  tft.setTextSize(8); tft.setCursor(tempX, 65); tft.print(tempStr);
  tft.setTextSize(3); tft.print("o");

  tft.setTextSize(2); tft.setTextColor(MY_TEXT_DARK);
  tft.getTextBounds(lastWeatherMain.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((W - w) / 2, 135); tft.print(lastWeatherMain);

  // PARAMS WITH SHADOWS
  int colX = 15; int sY = 50;
  tft.setTextSize(1); tft.setTextColor(MY_TEXT_DARK); tft.setCursor(colX, sY); tft.print("WIND m/s");
  tft.setTextSize(2);
  tft.setCursor(colX+1, sY+13); tft.setTextColor(SHADOW_COLOR); tft.print(String(lastWs, 1)); 
  tft.setCursor(colX, sY+12); tft.setTextColor(WHITE_COLOR); tft.print(String(lastWs, 1)); 
  
  tft.setTextSize(1); tft.setTextColor(MY_TEXT_DARK); tft.setCursor(colX, sY+38); tft.print("HUMID %");
  tft.setTextSize(2);
  tft.setCursor(colX+1, sY+51); tft.setTextColor(SHADOW_COLOR); tft.print(lastHumidity); 
  tft.setCursor(colX, sY+50); tft.setTextColor(WHITE_COLOR); tft.print(lastHumidity); 
  
  tft.setTextSize(1); tft.setTextColor(MY_TEXT_DARK); tft.setCursor(colX, sY+76); tft.print("PRESS");
  tft.setTextSize(2);
  tft.setCursor(colX+1, sY+89); tft.setTextColor(SHADOW_COLOR); tft.print((int)lastPres); 
  tft.setCursor(colX, sY+88); tft.setTextColor(WHITE_COLOR); tft.print((int)lastPres); 

  // FORECAST
  tft.drawFastHLine(10, 165, W-20, MY_TEXT_DARK);
  const char* daysShort[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  for(int i=0; i<5; i++) {
    int cX = (i * (W/5)) + (W/10);
    int dIdx = (timeValid) ? (ti.tm_wday + i + 1) % 7 : 0; 
    tft.setTextSize(2); tft.setTextColor(MY_TEXT_DARK);
    tft.setCursor(cX - 15, 175); tft.print(daysShort[dIdx]);
    drawAdvancedIcon(cX, 200, lastFIcons[i]);
    
    String fT = String(lastForecast[i]);
    tft.getTextBounds(fT.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(cX - (w/2) + 1, 215); tft.setTextColor(SHADOW_COLOR); tft.print(fT); 
    tft.setCursor(cX - (w/2), 214); tft.setTextColor(WHITE_COLOR); tft.print(fT); 
  }
}
