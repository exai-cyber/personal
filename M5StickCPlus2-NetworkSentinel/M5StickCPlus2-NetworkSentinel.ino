#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WiFiGeneric.h>
#include <WiFiMulti.h>
#include <WiFiSTA.h>
#include <WiFiScan.h>
#include <WiFiServer.h>
#include <WiFiType.h>
#include <WiFiUdp.h>
#include <M5StickCPlus2.h>
#include "include/config.h"
#include "include/config_private.h"

void setup() {
  M5.begin();
  M5.Lcd.setTextSize(2);
  M5.Lcd.println("Hello M5StickCPlus2!");
}

void loop() {
  M5.update();
}
