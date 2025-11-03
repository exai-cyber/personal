#include <M5Unified.h>
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
