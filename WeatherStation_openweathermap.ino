/*
Material de inspiratie: https://youtu.be/Boz2X6QPp44?si=FaGkp2SKgjmNLSZn (multumesc lui Igor Molodets pentru sketch)
Program generat cu AI pe baza cerintelor mele, Nicu FLORICA (niq_ro)
- Fundal negru (ascunde defectul fizic al ecranului).
- simbol Wi-Fi lângă Craiova (fără să taie din litere).
- Oră mare, portocalie, cu "puncte care bat" (fără să mai redeseneze tot ecranul la fiecare jumătate de secundă).
- Data mai mare (Size 2), centrată în dreapta, sub oră.
- Temperatura mutată mai jos, cu °C atașat.
- Prognoza pe 5 zile calculată corect (Temperatura MAXIMĂ a zilei).
 */


#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "time.h"

// --- TEMA INTUNECATĂ ---
#define MY_SKY_BLUE  0x0000
#define MY_TEXT_DARK 0xFFFF
#define SHADOW_COLOR 0x5A5A
#define WHITE_COLOR  0xFD20

TFT_eSPI tft = TFT_eSPI();

const char* ssid = "networkname";
const char* password = "password";
const char* weatherApiKey = "openweathermap_APIkey";

WiFiClientSecure client;

float lastTemp = 0, lastWs = 0, lastPres = 0;
int lastHumidity = 0;
String lastWeatherMain = "N/A";
int lastForecast[5] = {0,0,0,0,0};
String lastFIcons[5] = {"","","","",""};

unsigned long lastWeatherUpdate = 0;
const long weatherInterval = 300000;
unsigned long lastClockUpdate = 0;
const long clockInterval = 500;
bool showColon = true;

void setup() {
  Serial.begin(115200);
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);

  tft.init(); 
  tft.setRotation(0); 
  tft.fillScreen(MY_SKY_BLUE);
  
  tft.setTextSize(2);
  tft.setTextColor(MY_TEXT_DARK);
  tft.setCursor(50, 110);
  tft.print("Connecting...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  
  configTime(7200, 3600, "pool.ntp.org");
  client.setInsecure();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastWeatherUpdate >= weatherInterval || lastWeatherUpdate == 0) {
    if (WiFi.status() == WL_CONNECTED) { updateWeatherAndForecast(); }
    lastWeatherUpdate = currentMillis;
  }

  if (currentMillis - lastClockUpdate >= clockInterval) {
    drawClock();
    showColon = !showColon;
    lastClockUpdate = currentMillis;
  }
}

void drawClock() {
  struct tm ti;
  if (!getLocalTime(&ti)) return;

  tft.fillRect(120, 0, 120, 30, MY_SKY_BLUE);
  tft.setTextSize(3); 
  tft.setTextColor(WHITE_COLOR);
  
  char timeStr[6];
  if (showColon) {
    sprintf(timeStr, "%02d:%02d", ti.tm_hour, ti.tm_min);
  } else {
    sprintf(timeStr, "%02d %02d", ti.tm_hour, ti.tm_min);
  }
  
  int timeW = tft.textWidth(timeStr);
  tft.setCursor(240 - timeW - 5, 5); 
  tft.print(timeStr);
}

void drawAdvancedIcon(int x, int y, String type) {
  int ox = 2; 
  int oy = 2;
  
  if (type == "Clear") {
    tft.fillCircle(x+ox, y+oy, 7, SHADOW_COLOR);
    tft.fillCircle(x, y, 7, 0xFFE0);
    tft.drawCircle(x, y, 7, 0xFD20);
    for(int i=0; i<360; i+=45) {
      float rad = i * 0.01745;
      int x1 = x + cos(rad)*9;
      int y1 = y + sin(rad)*9;
      int x2 = x + cos(rad)*12;
      int y2 = y + sin(rad)*12;
      tft.drawLine(x1, y1, x2, y2, 0xFFE0);
    }
  } 
  else if (type == "Clouds") {
    tft.fillCircle(x-5+ox, y+2+oy, 6, SHADOW_COLOR);
    tft.fillCircle(x+5+ox, y+2+oy, 6, SHADOW_COLOR);
    tft.fillCircle(x+ox, y-3+oy, 7, SHADOW_COLOR);
    tft.fillCircle(x-5, y+2, 6, 0xC618); 
    tft.fillCircle(x+5, y+2, 6, 0xC618);
    tft.fillCircle(x, y-3, 7, 0xC618);
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
      int x_end = x + (int)(cos(rad)*10);
      int y_end = y + (int)(sin(rad)*10);
      tft.drawLine(x, y, x_end, y_end, MY_TEXT_DARK);
    }
  } 
  else if (type == "Thunderstorm") {
    tft.fillRoundRect(x-8+ox, y-5+oy, 16, 8, 4, SHADOW_COLOR);
    tft.fillRoundRect(x-8, y-5, 16, 8, 4, 0x4208);
    tft.drawLine(x+1, y+3, x-2, y+12, 0xFFE0);
  }
  else {
    tft.drawFastHLine(x-10+ox, y+oy, 20, SHADOW_COLOR);
    tft.drawFastHLine(x-10, y, 20, MY_TEXT_DARK);
  }
}

void drawWiFiIconSmall(int x, int y, uint16_t color) {
  int baseY = y + 10; 
  for (int i = 1; i <= 3; i++) {
    tft.drawCircle(x, baseY, i*3, color);
    tft.drawCircle(x, baseY, i*3 - 1, color); 
  }
  tft.fillCircle(x, baseY, 2, color);
  tft.fillRect(x - 15, baseY + 1, 30, 15, MY_SKY_BLUE); 
  tft.fillTriangle(x-17, baseY+2, x-2, baseY, x-17, baseY-17, MY_SKY_BLUE);
  tft.fillTriangle(x+17, baseY+2, x+2, baseY, x+17, baseY-17, MY_SKY_BLUE);
}

void updateWeatherAndForecast() {
  HTTPClient http;
  String url = "https://api.openweathermap.org/data/2.5/weather?q=Craiova&appid=" + String(weatherApiKey) + "&units=metric";
  http.begin(client, url);
  if (http.GET() == 200) {
    JsonDocument doc; 
    deserializeJson(doc, http.getStream());
    lastTemp = doc["main"]["temp"];
    lastWs = doc["wind"]["speed"];
    lastHumidity = doc["main"]["humidity"];
    lastPres = (float)doc["main"]["pressure"] * 0.750062;
    lastWeatherMain = doc["weather"][0]["main"].as<String>();
  }
  http.end();

  String fUrl = "https://api.openweathermap.org/data/2.5/forecast?q=Craiova&appid=" + String(weatherApiKey) + "&units=metric";
  http.begin(client, fUrl);
  if (http.GET() == 200) {
    JsonDocument fDoc; 
    deserializeJson(fDoc, http.getStream());
    
    for(int i=0; i<5; i++) {
      float maxTemp = -100.0; 
      String noonIcon = "Clear";
      
      for(int j=0; j<8; j++) {
        int idx = (i * 8) + j;
        if(!fDoc["list"][idx].isNull()) {
          float currentTemp = fDoc["list"][idx]["main"]["temp"].as<float>();
          if(currentTemp > maxTemp) { maxTemp = currentTemp; }
          if(j == 4) { noonIcon = fDoc["list"][idx]["weather"][0]["main"].as<String>(); }
        }
      }
      lastForecast[i] = (int)round(maxTemp); 
      lastFIcons[i] = noonIcon;              
    }
  }
  http.end();
  drawUI(); 
}

void drawUI() {
  tft.fillScreen(MY_SKY_BLUE);
  int W = 240; 
  int16_t x1, y1; uint16_t w, h;
  struct tm ti;
  bool timeValid = getLocalTime(&ti);

  // --- HEADER STANGA ---
  tft.setTextSize(2); 
  tft.setTextColor(MY_TEXT_DARK);
  tft.setCursor(5, 10); 
  tft.print("CRAIOVA");
  
  int orasW = tft.textWidth("CRAIOVA");
  int wifiX = 5 + orasW + 20; 
  drawWiFiIconSmall(wifiX, 10, MY_TEXT_DARK);

  // Linie de separare
  tft.drawFastHLine(10, 50, W-20, SHADOW_COLOR);

  // --- DATA SUB LINIE ---
  if (timeValid) {
    tft.setTextSize(2); 
    tft.setTextColor(SHADOW_COLOR);
    const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    char dateBuffer[16];
    sprintf(dateBuffer, "%s, %d %s", days[ti.tm_wday], ti.tm_mday, months[ti.tm_mon]);
    
    int dateW = tft.textWidth(dateBuffer);
    tft.setCursor(W - dateW - 5, 55); 
    tft.print(dateBuffer);
  }

  // --- TEMPERATURĂ ---
  tft.setTextSize(8); 
  String tempStr = String((int)round(lastTemp));
  int tempW = tft.textWidth(tempStr);
  int tempX = (W - tempW) / 2;
  
  tft.setTextColor(SHADOW_COLOR); 
  tft.setTextSize(8); tft.setCursor(tempX + 3, 88); tft.print(tempStr);
  tft.setTextSize(5); tft.print("\xF7"); tft.print("C");
  
  tft.setTextColor(WHITE_COLOR); 
  tft.setTextSize(8); tft.setCursor(tempX, 85); tft.print(tempStr);
  tft.setTextSize(5); tft.print("\xF7"); tft.print("C");

  // --- STAREA VREMII ---
  tft.setTextSize(2); 
  tft.setTextColor(MY_TEXT_DARK);
  int mainW = tft.textWidth(lastWeatherMain);
  int iconW = 24; 
  int space = 5; 
  int totalW = iconW + space + mainW;
  int startX = (W - totalW) / 2; 
  
  drawAdvancedIcon(startX + 12, 158, lastWeatherMain);
  tft.setCursor(startX + iconW + space, 150); 
  tft.print(lastWeatherMain);

  // --- PARAMETRI ---
  int colX = 15; int sY = 60;
  tft.setTextSize(1); tft.setTextColor(SHADOW_COLOR); tft.setCursor(colX, sY); tft.print("WIND m/s");
  tft.setTextSize(2);
  tft.setCursor(colX+1, sY+13); tft.setTextColor(SHADOW_COLOR); tft.print(String(lastWs, 1)); 
  tft.setCursor(colX, sY+12); tft.setTextColor(MY_TEXT_DARK); tft.print(String(lastWs, 1)); 
  
  tft.setTextSize(1); tft.setTextColor(SHADOW_COLOR); tft.setCursor(colX, sY+38); tft.print("HUMID %");
  tft.setTextSize(2);
  tft.setCursor(colX+1, sY+51); tft.setTextColor(SHADOW_COLOR); tft.print(lastHumidity); 
  tft.setCursor(colX, sY+50); tft.setTextColor(MY_TEXT_DARK); tft.print(lastHumidity); 
  
  tft.setTextSize(1); tft.setTextColor(SHADOW_COLOR); tft.setCursor(colX, sY+76); tft.print("PRESS");
  tft.setTextSize(2);
  tft.setCursor(colX+1, sY+89); tft.setTextColor(SHADOW_COLOR); tft.print((int)lastPres); 
  tft.setCursor(colX, sY+88); tft.setTextColor(MY_TEXT_DARK); tft.print((int)lastPres); 

  // --- PROGNOZA 5 ZILE ---
  tft.drawFastHLine(10, 172, W-20, SHADOW_COLOR); 
  const char* daysShort[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  for(int i=0; i<5; i++) {
    int cX = (i * (W/5)) + (W/10);
    int dIdx = (timeValid) ? (ti.tm_wday + i + 1) % 7 : 0; 
    tft.setTextSize(2); tft.setTextColor(SHADOW_COLOR); 
    tft.setCursor(cX - 15, 180); tft.print(daysShort[dIdx]);
    drawAdvancedIcon(cX, 205, lastFIcons[i]);
    
    String fT = String(lastForecast[i]);
    int fW = tft.textWidth(fT);
    tft.setCursor(cX - (fW/2) + 1, 220); tft.setTextColor(SHADOW_COLOR); tft.print(fT); 
    tft.setCursor(cX - (fW/2), 219); tft.setTextColor(WHITE_COLOR); tft.print(fT); 
  }
}
