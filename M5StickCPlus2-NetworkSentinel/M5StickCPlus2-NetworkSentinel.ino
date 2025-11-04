/*New ideas:
-ping detector
*/
#include <WiFi.h>
#include <WiFiMulti.h>
#include <M5StickCPlus2.h>
#include <ESPping.h>
#include "include/config_private.h" //Change config.h to config_private.h after downloading a file and fill WiFi SSID and password, otherwise 

int currentMode = 0;
bool isConnected=false;//--->every function will check if wifiConnected()

void showMenu();
void runMode(int mode);
//modes 
void runConnectivityTest(); //2
void showConfig(); //1
void netScanner(); //0

//helpers for scanner/selection/portscan
void discoverDevices();              //performs scan and fills foundIPv4[]
int selectDevice();                  //interactive chooser, returns index or -1
void portScan(uint32_t targetIPv4, bool fullScan);  //simple TCP port scan on chosen IP (only open ports)
String ipv4ToString(uint32_t ip);    //helper to format IP
uint32_t ipToUint32(const IPAddress &ip);
IPAddress uint32ToIP(uint32_t v);

bool wifiConnected(){
  if (WiFi.status() != WL_CONNECTED){
    M5.Lcd.setTextColor(TFT_RED, TFT_PURPLE);
    return false;
  }
  else return true;
}

// storage for discovered hosts
const int MAX_FOUND = 48;
uint32_t foundIPv4[MAX_FOUND];
int foundCount = 0;

void setup(){
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
  while(WiFi.status() != WL_CONNECTED && timeout < 20){ //20*500ms=10s
    delay(500);
    M5.Lcd.print(".");
    timeout++;
  }

  isConnected=wifiConnected();
  delay(1000);
  showMenu();
}

void loop() {
  M5.update();

  //A change mode
  if(M5.BtnA.wasPressed()){
    currentMode++;
    if(currentMode>2)currentMode = 0;
    showMenu();
  }

  //B select mode
  if(M5.BtnB.wasPressed()){
    runMode(currentMode);
    showMenu();
  }
}

void showMenu(){
  //Different colors for modes
  uint16_t bgColor;
  uint16_t textColor;
  uint16_t highlightColor;

  if(currentMode == 0){
    bgColor = BLACK;
    textColor = WHITE;
    highlightColor = WHITE;
  } 
  else if(currentMode == 1){
    bgColor = TFT_PURPLE;
    textColor = TFT_ORANGE;
    highlightColor = TFT_ORANGE;
  } 
  else if (currentMode == 2){
    bgColor = TFT_CYAN;
    textColor = TFT_NAVY; 
    highlightColor = TFT_RED;
  }

  M5.Lcd.fillScreen(bgColor);
  M5.Lcd.setTextColor(textColor, bgColor);

  M5.Lcd.setCursor(10, 0);
  M5.Lcd.println("==== MENU ====");

  M5.Lcd.setCursor(0, 30);

  if(currentMode==0){
    M5.Lcd.setTextColor(highlightColor, bgColor);
    M5.Lcd.println("> Network Sentinel");
    M5.Lcd.setTextColor(textColor, bgColor);
    M5.Lcd.println("  Config & WiFi");
    M5.Lcd.println("  Connectivity Test");
  } 
  else if (currentMode==1){
    M5.Lcd.setTextColor(textColor, bgColor);
    M5.Lcd.println("  Network Sentinel");
    M5.Lcd.setTextColor(highlightColor, bgColor);
    M5.Lcd.println("> Config & WiFi");
    M5.Lcd.setTextColor(textColor, bgColor);
    M5.Lcd.println("  Connectivity Test");
  } 
  else if(currentMode==2){
    M5.Lcd.setTextColor(textColor, bgColor);
    M5.Lcd.println("  Network Sentinel");
    M5.Lcd.println("  Config & WiFi");
    M5.Lcd.setTextColor(highlightColor, bgColor);
    M5.Lcd.println("> Connectivity Test");
  }
}


void runMode(int mode){
  switch (mode){
    case 0:
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.println("Network Sentinel");
      netScanner();
      delay(1000);
      break;

    case 1:
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.println("WiFi config...");
      showConfig();
      delay(1000);
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
void netScanner(){
  isConnected = wifiConnected();

  if(!isConnected){
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.println("WiFi not connected");
    M5.Lcd.println("\nPress A to return");
    while (true){
      M5.update();
      if (M5.BtnA.wasPressed()) return;
      delay(50);
    }
  }

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(TFT_GREEN, BLACK);
  M5.Lcd.println("Network Scanner");
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.print("Local IP: ");
  M5.Lcd.println(WiFi.localIP());
  M5.Lcd.print("Subnet:  ");
  M5.Lcd.println(WiFi.subnetMask());
  M5.Lcd.println();
  delay(800);

  discoverDevices();

  if (foundCount == 0) {
    M5.Lcd.println("\nNo devices found");
    M5.Lcd.println("Press A to return");
    while (true) {
      M5.update();
      if (M5.BtnA.wasPressed()) return;
      delay(50);
    }
  }

  int idx = selectDevice();
  if(idx >= 0 && idx < foundCount) {
    int choice = 0;
    while (true) {
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0,0);
      M5.Lcd.println("Choose port scan:");
      M5.Lcd.println();
      if (choice == 0) {
        M5.Lcd.setTextColor(TFT_GREEN, BLACK);
        M5.Lcd.println("> A) Popular ports");
        M5.Lcd.setTextColor(WHITE, BLACK);
        M5.Lcd.println("  B) All ports (1-65535)");
      } else {
        M5.Lcd.setTextColor(WHITE, BLACK);
        M5.Lcd.println("  A) Popular ports");
        M5.Lcd.setTextColor(TFT_GREEN, BLACK);
        M5.Lcd.println("> B) All ports (1-65535)");
      }
      M5.update();
      while (true) {
        M5.update();
        if (M5.BtnA.wasPressed()) { choice = (choice + 1) % 2; break; }
        if (M5.BtnB.wasPressed()) { portScan(foundIPv4[idx], (choice == 1)); goto after_scan_choice; }
        delay(50);
      }
    }
  }
after_scan_choice:
  ;
}

void discoverDevices() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0,0);
  M5.Lcd.println("Discovering devices\n");

  IPAddress localIP = WiFi.localIP();
  IPAddress subnet = WiFi.subnetMask();

  uint32_t ipNum = ipToUint32(localIP);
  uint32_t maskNum = ipToUint32(subnet);
  uint32_t netNum = ipNum & maskNum;
  uint32_t broadcast = netNum | (~maskNum);

  uint32_t start = netNum + 1;
  uint32_t end = broadcast - 1;

  uint32_t maxHostsToScan = 4096; 
  uint32_t hostsRange = (end >= start) ? (end - start + 1) : 0;
  if (hostsRange > maxHostsToScan) {
    uint8_t a = localIP[0], b = localIP[1], c = localIP[2];
    start = ipToUint32(IPAddress(a, b, c, 1));
    end = ipToUint32(IPAddress(a, b, c, 254));
  }

  foundCount = 0;

  M5.Lcd.print("Local IP: ");
  M5.Lcd.println(localIP);
  M5.Lcd.print("Subnet: ");
  M5.Lcd.println(subnet);
  M5.Lcd.println();
  M5.Lcd.println("Scanning (BtnB abort)\n");

  const int pingAttempts   = 1;
  const int tcpTimeoutMs   = 150;
  const int smallDelay     = 6;
  uint32_t scanned = 0;
  uint32_t totalToScan = (end >= start) ? (end - start + 1) : 0;

  for (uint32_t cur = start; cur <= end; ++cur) {
    M5.update();
    if (M5.BtnB.wasPressed()) {
      M5.Lcd.println("\nScan aborted");
      return;
    }

    if (cur == ipNum) { scanned++; }

    IPAddress target = uint32ToIP(cur);
    bool alive = Ping.ping(target, pingAttempts);

    if (!alive) {
      WiFiClient sock;
      if (sock.connect(target, 80, tcpTimeoutMs)) { alive = true; sock.stop(); }
      else if (sock.connect(target, 443, tcpTimeoutMs)) { alive = true; sock.stop(); }
      else if (sock.connect(target, 22, tcpTimeoutMs)) { alive = true; sock.stop(); }
    }

    if (alive) {
      if (foundCount < MAX_FOUND) foundIPv4[foundCount] = cur;
      foundCount++;
    }

    scanned++;
    if ((scanned % 4) == 0 || scanned == totalToScan) {
      M5.Lcd.setCursor(0, 70);
      M5.Lcd.setTextColor(TFT_YELLOW, BLACK);
      M5.Lcd.printf("Progress: %u/%u   ", scanned, totalToScan);
    }

    delay(smallDelay);
  }

  M5.Lcd.println();
  M5.Lcd.print("Found: ");
  M5.Lcd.println(foundCount);
  M5.Lcd.println();
  delay(300);
}

int selectDevice() {
  if (foundCount == 0) return -1;

  const int perPage = 6;
  int page = 0;
  int cursor = 0;

  int totalPages = (foundCount + perPage - 1) / perPage;

  while (true) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.printf("Select device (%d)\n\n", foundCount);

    int start = page * perPage;
    int end = min(start + perPage, foundCount);
    for (int i = start; i < end; ++i) {
      if (i == cursor) {
        M5.Lcd.setTextColor(TFT_GREEN, BLACK);
        M5.Lcd.print("> ");
      } else {
        M5.Lcd.setTextColor(WHITE, BLACK);
        M5.Lcd.print("  ");
      }
      M5.Lcd.println(ipv4ToString(foundIPv4[i]));
    }

    M5.Lcd.setTextColor(TFT_YELLOW, BLACK);
    M5.Lcd.printf("\nA-next  B-select  Pg %d/%d", page+1, totalPages);

    //wait for input - A moves cursor, B selects
    while (true) {
      M5.update();
      if (M5.BtnA.wasPressed()) {
        cursor++;
        if (cursor >= foundCount) cursor = 0;
        page = cursor / perPage;
        break;
      }
      if (M5.BtnB.wasPressed()) {
        return cursor;
      }
      delay(50);
    }
  }

  return -1;
}

//simple TCP port scanner for selected IP (only open ports)
// fullScan: false = popular ports, true = all ports 1..65535
void portScan(uint32_t targetIPv4, bool fullScan) {
  IPAddress target = uint32ToIP(targetIPv4);

  const int popularPorts[] = {22, 23, 80, 443, 8080, 8443, 3306, 3389};
  const int npop = sizeof(popularPorts)/sizeof(popularPorts[0]);

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0,0);
  M5.Lcd.printf("Port scan: %s\n\n", ipv4ToString(targetIPv4).c_str());

  bool anyOpen = false;

  if (!fullScan) {
    for (int i = 0; i < npop; ++i) {
      int p = popularPorts[i];
      WiFiClient sock;
      if (sock.connect(target, p, 200)) {
        M5.Lcd.printf("Port %d: OPEN\n", p);
        sock.stop();
        anyOpen = true;
      }
      M5.update();
      if (M5.BtnB.wasPressed()) { M5.Lcd.println("\nScan aborted"); break; }
      delay(40);
    }
  } else {
    uint32_t p = 1;
    const uint32_t LAST = 65535;
    uint32_t shown = 0;
    for (p = 1; p <= LAST; ++p) {
      WiFiClient sock;
      if (sock.connect(target, p, 120)) {
        M5.Lcd.printf("Port %u: OPEN\n", p);
        sock.stop();
        anyOpen = true;
      }
      if ((p % 50) == 0) {
        M5.Lcd.setCursor(0, 120);
        M5.Lcd.setTextColor(TFT_YELLOW, BLACK);
        M5.Lcd.printf("Ports scanned: %u/%u   ", p, LAST);
        shown = p;
      }
      M5.update();
      if (M5.BtnB.wasPressed()) { M5.Lcd.println("\nScan aborted"); break; }
      if ((p % 16) == 0) delay(10);
    }
    if (shown > 0) { M5.Lcd.setCursor(0, 120); M5.Lcd.printf("Ports scanned: %u/%u   ", shown, LAST); }
  }

  if (!anyOpen) M5.Lcd.println("No open ports detected.");

  M5.Lcd.println("\nPress B to return");
  while (!M5.BtnB.wasPressed()) {
    M5.update();
    delay(50);
  }
}

// =============================
// Mode 1: Show config
// =============================
void showConfig(){
  M5.Lcd.fillScreen(TFT_PURPLE);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.println("WiFi Configuration\n");

  isConnected = wifiConnected();

  if(isConnected==false){
    M5.Lcd.println("WiFi Error! See config_private.h");
    M5.Lcd.println("\nPress A to return");
    while (true) {
      M5.update();
      if (M5.BtnA.wasPressed()) {
        return;
      }
      delay(50);
    }
  }

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

  while (true) {
    M5.update();
    if (M5.BtnA.wasPressed()) {
      return;
    }
    delay(50);
  }
}

// =============================
// Mode 2: Connectivity Test
// =============================
void runConnectivityTest(){
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("Connectivity Test\n");

  isConnected = wifiConnected();

  if(isConnected==false){
    M5.Lcd.println("Wi-Fi: Not connected");
    M5.Lcd.println("\nPress B to return to menu");
    while (!M5.BtnB.wasPressed()) {
      M5.update();
      delay(50);
    }
    return;
  }
  delay(1000);
  M5.Lcd.print("IP: ");
  M5.Lcd.println(WiFi.localIP());
  M5.Lcd.println("\nTesting...");

  WiFiClient client;
  unsigned long start = millis();

  if(client.connect("8.8.8.8", 53)){
    unsigned long elapsed = millis() - start;
    M5.Lcd.setTextColor(TFT_GREEN, BLACK);
    M5.Lcd.println("Internet: OK");
    M5.Lcd.print("Latency: ");
    M5.Lcd.print(elapsed);
    M5.Lcd.println(" ms");
    client.stop();
  } 
  else{
    M5.Lcd.setTextColor(TFT_RED, BLACK);
    M5.Lcd.println("Internet: FAIL");
  }

  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.println("\nPress B to return to menu");
  while (!M5.BtnB.wasPressed()) {
    M5.update();
    delay(50);
  }
}

// ----------------------- Helper functions -----------------------

String ipv4ToString(uint32_t ip) {
  IPAddress a = uint32ToIP(ip);
  String s = String(a[0]) + "." + String(a[1]) + "." + String(a[2]) + "." + String(a[3]);
  return s;
}

uint32_t ipToUint32(const IPAddress &ip) {
  return ((uint32_t)ip[0] << 24) | ((uint32_t)ip[1] << 16) | ((uint32_t)ip[2] << 8) | ((uint32_t)ip[3]);
}

IPAddress uint32ToIP(uint32_t v) {
  uint8_t a = (v >> 24) & 0xFF;
  uint8_t b = (v >> 16) & 0xFF;
  uint8_t c = (v >> 8) & 0xFF;
  uint8_t d = v & 0xFF;
  return IPAddress(a, b, c, d);
}
