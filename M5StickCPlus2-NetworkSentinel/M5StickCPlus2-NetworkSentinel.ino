/*New ideas:
-ping detector
*/
#include <WiFi.h>
#include <WiFiMulti.h>
#include <M5StickCPlus2.h>
#include "include/config_private.h" //Change config.h to config_private.h after downloading a file and fill WiFi SSID and password, otherwise 

int currentMode = 0;

void showMenu();
void runMode(int mode);
//modes 
void runConnectivityTest(); //2
void showConfig(); //1
void netScanner(); //0

void setup() {
  M5.begin();
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setRotation(1);
  M5.Lcd.println("Network Sentinel");

  //Connecting with WiFi provided in private config
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  M5.Lcd.print("Connecting to Wi-Fi");
   int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) { // 20 * 500ms = 10s
    delay(500);
    M5.Lcd.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    M5.Lcd.println("\nConnected!");
    M5.Lcd.print("IP: ");
    M5.Lcd.println(WiFi.localIP());
  } else {
    M5.Lcd.println("\nWi-Fi Failed!");
  }

  delay(1500);
  showMenu();
}

void loop() {
  M5.update();

  // Przycisk A zmiana opcji
  if(M5.BtnA.wasPressed()){
    currentMode++;
    if(currentMode>2)currentMode = 0;
    showMenu();
  }

  // Przycisk B wybór
  if(M5.BtnB.wasPressed()){
    runMode(currentMode);
    showMenu();
  }
}

void showMenu() {
  //Different colors for modes
  uint16_t bgColor;
  uint16_t textColor;
  uint16_t highlightColor;

  if (currentMode == 0) {
    bgColor = BLACK;
    textColor = WHITE;
    highlightColor = WHITE;
  } else if (currentMode == 1) {
    bgColor = TFT_PURPLE;
    textColor = TFT_ORANGE;
    highlightColor = TFT_ORANGE;
  } else if (currentMode == 2) {
    bgColor = TFT_CYAN;
    textColor = TFT_NAVY; 
    highlightColor = TFT_RED;
  }

  M5.Lcd.fillScreen(bgColor);
  M5.Lcd.setTextColor(textColor, bgColor);

  M5.Lcd.setCursor(10, 0);
  M5.Lcd.println("== MENU ==");

  M5.Lcd.setCursor(0, 30);

  if (currentMode == 0) {
    M5.Lcd.setTextColor(highlightColor, bgColor);
    M5.Lcd.println("> Network Sentinel");
    M5.Lcd.setTextColor(textColor, bgColor);
    M5.Lcd.println("  Config & WiFi");
    M5.Lcd.println("  Connectivity Test");
  } else if (currentMode == 1) {
    M5.Lcd.setTextColor(textColor, bgColor);
    M5.Lcd.println("  Network Sentinel");
    M5.Lcd.setTextColor(highlightColor, bgColor);
    M5.Lcd.println("> Config & WiFi");
    M5.Lcd.setTextColor(textColor, bgColor);
    M5.Lcd.println("  Connectivity Test");
  } else if (currentMode == 2) {
    M5.Lcd.setTextColor(textColor, bgColor);
    M5.Lcd.println("  Network Sentinel");
    M5.Lcd.println("  Config & WiFi");
    M5.Lcd.setTextColor(highlightColor, bgColor);
    M5.Lcd.println("> Connectivity Test");
  }
}


void runMode(int mode) {
  switch (mode) {
    case 0:
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.println("Network Sentinel");
      // TODO: Main function that will scan network like nmap
      netScanner();
      delay(2000);
      break;

    case 1:
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.println("WiFi config...");
      showConfig();
      delay(2000);
      break;

    case 2:
      runConnectivityTest();
      break;
  }
}

//--------------------------------MAIN FUNCTIONS----------------------------------------

// =============================
// Mode 0: Net scanner
// =============================
//For now it will be simple enumeration of local ip addresses that answers for ICMP ping
void netScanner(){
  ;
}

//---------------------------------------------------------------------------------------

// =============================
// Mode 1: Show config
// =============================
// SSID | LOCAL IP | RSSI(Signal strength in dBm) |  MAC address
void showConfig() {
  M5.Lcd.fillScreen(TFT_PURPLE);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.println("WiFi Configuration\n");

  if (WiFi.status() != WL_CONNECTED) {
    M5.Lcd.setTextColor(TFT_RED, TFT_PURPLE);
    M5.Lcd.println("Status: Not connected");
    delay(2000);
    return;
  }

  // Displaying all info in different colors
  M5.Lcd.setTextColor(TFT_ORANGE, TFT_PURPLE);
  M5.Lcd.print("SSID: ");
  M5.Lcd.println(WiFi.SSID());

  M5.Lcd.setTextColor(TFT_GREEN, TFT_PURPLE);
  M5.Lcd.print("IP: ");
  M5.Lcd.println(WiFi.localIP());

  M5.Lcd.setTextColor(TFT_YELLOW, TFT_PURPLE);
  M5.Lcd.print("RSSI: ");
  M5.Lcd.print(WiFi.RSSI());
  M5.Lcd.println(" dBm");

  M5.Lcd.setTextColor(TFT_CYAN, TFT_PURPLE);
  M5.Lcd.print("MAC: ");
  M5.Lcd.println(WiFi.macAddress());

  M5.Lcd.setTextColor(TFT_WHITE, TFT_PURPLE);
  M5.Lcd.println();
  M5.Lcd.println("Press BtnA to go back");
  delay(10000);
}


//---------------------------------------------------------------------------------------
// =============================
// Mode 2: Connectivity Test
// =============================
// It works by checking connection with Google DNS(8.8.8.8 on port 53[DNS])
void runConnectivityTest() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("Connectivity Test\n");

  if (WiFi.status() != WL_CONNECTED) {
    M5.Lcd.println("Wi-Fi: Not connected");
    delay(5000);
    return;
  }
  delay(1000);
  M5.Lcd.print("IP: ");
  M5.Lcd.println(WiFi.localIP());
  M5.Lcd.println("\nTesting...");

  WiFiClient client;
  unsigned long start = millis();

  if (client.connect("8.8.8.8", 53)) {
    unsigned long elapsed = millis() - start;
    M5.Lcd.setTextColor(TFT_GREEN, BLACK);
    M5.Lcd.println("Internet: OK");
    M5.Lcd.print("Latency: ");
    M5.Lcd.print(elapsed);
    M5.Lcd.println(" ms");
    client.stop();
    delay(15000);
  } else {
    M5.Lcd.setTextColor(TFT_RED, BLACK);
    M5.Lcd.println("Internet: FAIL");
    delay(5000);  
  }

  M5.Lcd.println("\nPress B to return to menu");
  while (!M5.BtnB.wasPressed()) {
    M5.update();
    delay(50);
  }
}

