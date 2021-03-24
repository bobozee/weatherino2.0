//CPU:
#include <Arduino.h>
//WiFi:
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
//Devices:
#include <dht11.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//Other:
#include <sstream>


#define POWER_LED_PIN D7
#define DHT11_PIN D4
#define ANALOG_PIN A0
#define BUTTON_PIN D6
#define BUTTON_PIN_NUMBER 12

#define LOCALPORT 2390

#define SSID "Erpix"
#define PASSWORD "Blackforest72"

int millisAtStart, hour, minute, second, day, month, humidity, temp, windspeed, light;
char packetBuffer[255];
volatile boolean btnIsPressed = false;
WiFiUDP udp;
NTPClient timeClient(udp, "de.pool.ntp.org", 7200);
dht11 dhtsens;
Adafruit_SSD1306 display(128, 64, &Wire);

void drawtext(String text, int line, int size) {
  display.setTextSize(size);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(0, line);
  display.print(text);
}

void broadcast(String data) {
  display.clearDisplay();
  IPAddress broadcastIp = WiFi.localIP();
  broadcastIp[3] = 255;
  Serial.print("Broadcasting on: ");
  Serial.println(broadcastIp);
  drawtext("Broadcasting on:", 2, 1);
  display.setCursor(0, 14);
  display.print(broadcastIp);
  if (btnIsPressed) {
    display.clearDisplay();
  }
  display.display();
  char broadcastBuffer[data.length() + 1];
  strcpy(broadcastBuffer, data.c_str());
  udp.beginPacket(broadcastIp, LOCALPORT);
  udp.write(broadcastBuffer);
  udp.endPacket();

  millisAtStart = millis();
}

int getMonth(unsigned long epochTime) {
  time_t rawtime = epochTime;
  struct tm * ti;
  ti = localtime (&rawtime);
  int month = (ti->tm_mon + 1) < 10 ? 0 + (ti->tm_mon + 1) : (ti->tm_mon + 1);
  
  return month;
}

int getDay(unsigned long epochTime) {
  time_t rawtime = epochTime;
  struct tm * ti;
  ti = localtime (&rawtime);
  int day = (ti->tm_mday) < 10 ? 0 + (ti->tm_mday) : (ti->tm_mday);
  
  return day;
}

void startWifi(const char* ssid, const char* password) {
  boolean connected = false;
  while (!connected) {
    delay(1); //required in order to prevent the WDT from resetting the chip
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.print("....");
    boolean esc = false;
    while (!esc) {
      int initialmillis = millis();
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(1); //required in order to prevent the WDT from resetting the chip
        if (millis() - initialmillis >= 10000) {
          Serial.println("Timeout");
          esc = true;
        }
      }
      esc = true;
    }
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
    }
  }
}

void changeBtnState() {
  if (btnIsPressed) {
    btnIsPressed = false;
  } else {
    btnIsPressed = true;
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  pinMode(POWER_LED_PIN, OUTPUT);
  pinMode(DHT11_PIN, INPUT);
  pinMode(ANALOG_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);

  delay(250);

  Serial.println();
  Serial.println("Startup complete.");
  Serial.println("~~~~~~~~~~~~~~~~~");
  digitalWrite(POWER_LED_PIN, HIGH);
  display.display();
  display.clearDisplay();
  drawtext("Starting...", 2, 1);
  display.display();

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN_NUMBER), changeBtnState, FALLING);

  millisAtStart = millis();
  startWifi(SSID, PASSWORD);
  udp.begin(LOCALPORT);
  timeClient.begin();
  Serial.println("Done");
}

void loop() {
  delay(1000);
  display.clearDisplay();
  {
    std::string stringbuffer;
    std::stringstream streambuffer;
    if (!timeClient.update()) {
      Serial.println("failed");
    }
    hour = timeClient.getHours();
    minute = timeClient.getMinutes();
    second = timeClient.getSeconds();
    day = getDay(timeClient.getEpochTime());
    month = getMonth(timeClient.getEpochTime());
    streambuffer << "Time: " << hour << ":" << minute << ":" << second << ", " << day << "." << month;
    stringbuffer = streambuffer.str();
    Serial.println(stringbuffer.c_str());
    drawtext(stringbuffer.c_str(), 2, 1);
  }

  {
    std::string stringbuffer;
    std::stringstream streambuffer;
    dhtsens.read(DHT11_PIN);
    humidity = dhtsens.humidity;
    streambuffer << "H/T: " << humidity << "%, " << temp << " C";
    stringbuffer = streambuffer.str();
    Serial.println(stringbuffer.c_str());
    drawtext(stringbuffer.c_str(), 14, 1);
  }

  {
    std::string stringbuffer;
    std::stringstream streambuffer;
    //windspeed = analogRead(analog);
    streambuffer << "Windspeed: " << windspeed << " km/h";
    stringbuffer = streambuffer.str();
    Serial.println(stringbuffer.c_str());
    drawtext(stringbuffer.c_str(), 26, 1);
  }

  {
    std::string stringbuffer;
    std::stringstream streambuffer;
    light = analogRead(ANALOG_PIN);
    streambuffer << "Light: " << light << " PhU";
    stringbuffer = streambuffer.str();
    Serial.println(stringbuffer.c_str());
    drawtext(stringbuffer.c_str(), 38, 1);
  }

  if (WiFi.status() != WL_CONNECTED) {
    drawtext("Connecting...", 50, 1);
    startWifi(SSID, PASSWORD);
    timeClient.begin();
    udp.begin(LOCALPORT);
  } else {
    std::string stringbuffer;
    std::stringstream streambuffer;
    streambuffer << "(" << WiFi.localIP().toString().c_str() << ")";
    stringbuffer = streambuffer.str();
    Serial.println(stringbuffer.c_str());
    drawtext(stringbuffer.c_str(), 50, 1);
  }
  if (btnIsPressed) {
    display.clearDisplay();
  }
  display.display();
  Serial.println();
  if (millis() - millisAtStart >= 60000) {
    std::string stringbuffer;
    std::stringstream streambuffer;
    streambuffer << second << minute << hour << day << month << humidity << temp << windspeed << light;
    stringbuffer = streambuffer.str();
    String values = stringbuffer.c_str();
    broadcast(values);
  }
}