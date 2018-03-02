#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MQ135.h"

#if (SSD1306_LCDHEIGHT != 48)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

ESP8266WebServer server(80);
MQ135 sensor = MQ135(A0);

// SCL GPIO5
// SDA GPIO4
#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);

const int transitionTime = 30 * 1000;

void showData(String caption, String caption2, String value, String units) {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.println(caption);
  display.setCursor(0,10);
  display.println(caption2);
  display.setTextSize(2);
  display.setCursor(0,20);
  display.println(value);
  display.setTextSize(1);
  display.setCursor(0,40);
  display.println(units);
  display.display();
}

void showDataSmall(String caption, String caption2, String value, String units) {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.println(caption);
  display.setCursor(0,10);
  display.println(value);
  display.setTextSize(1);
  display.setCursor(0,30);
  display.println(units);
  display.display();
}

void handleNotFound() 
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}


String prepareHtmlPage()
{
  String message = "<!DOCTYPE HTML>";
  message += "<html><pre>";
  message += "A0:  ";
  message += String(analogRead(A0));
  message += "<br/>";
  message += "ppm:  ";
  message += String(sensor.getPPM());
  message += "<br/>";
  message += "RZERO0:  ";
  message += String(sensor.getRZero());
  message += "</pre></html>";
  message += "\r\n";
  
  return message;
}

void handleRoot() {
  server.send(200, "text/html", prepareHtmlPage());
}

void setup()
{
  Serial.begin(115200);
  while (!Serial) {};

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)

  display.display();


  WiFiManager wifiManager;
  wifiManager.setTimeout(60);

  while (!wifiManager.autoConnect("AirQualityAP")) { };

  Serial.println("connected...yeey :)");
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();

  display.clearDisplay();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("air-quality")) {
    Serial.println("MDNS responder started");
  }

  
  server.on("/", handleRoot);

  server.on("/info", [](){
    server.send(200, "text/plain", "Air Quality Node sensor v 0.1b");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  ArduinoOTA.handle();
  server.handleClient();

  if (millis() % transitionTime < transitionTime / 3) {
    showData("A0", "is", String(analogRead(A0)), "");
  } else if ((millis() % transitionTime > transitionTime / 3) && (millis() % transitionTime < transitionTime * 2 / 3)) {
    showData("Pollution", "is", String(sensor.getPPM()), "ppm");
  } else if (millis() % transitionTime > transitionTime * 2 / 3) {
    showData("RZERO", "is", String(sensor.getRZero()), "ohm");
  }

  delay(100);
}
