#include <WiFi.h>
#include <WiFiMulti.h>
#include <M5StickCPlus2.h>
#include <ESPping.h>
#include "include/config_private.h" //Change config.h to config_private.h after downloading a file and fill WiFi SSID and password, otherwise 

int currentMode = 0;
bool isConnected=false;//--->every function will check if wifiConnected()

void showMenu();
void runMode(int mode);
void displayBattery();
//modes 
void showConfig(); //2
void netScanner(); //0
void wifiScanner(); //1

//helpers for scanner/selection/portscan
void discoverDevices();              
int selectDevice();                  
void portScan(uint32_t targetIPv4, bool fullScan);  
String ipv4ToString(uint32_t ip);    
uint32_t ipToUint32(const IPAddress &ip);
IPAddress uint32ToIP(uint32_t v);

// signal bars helper (returns a string with 0..4 bars "▂▄▆█")
String getSignalBarsString(int rssi) {
  // English comment: return a string composed of characters representing signal strength.
  if (rssi >= -50) return "▂▄▆█";
  else if (rssi >= -60) return "▂▄▆";
  else if (rssi >= -70) return "▂▄";
  else if (rssi >= -80) return "▂";
  else return " ";
}

// =============================
// RSSI -> distance (calibrated)
// =============================
// English comment: calibrated constants derived from user's measurements.
const float CALIB_TX_DBM = -39.242647115718064; // calibrated tx at 1m
const float CALIB_N = 2.8224303224420297;      // calibrated path-loss exponent

// English comment: convert RSSI (dBm) to approximate distance in meters
float rssiToMeters(int rssi) {
  if (rssi == 0) return -1.0;
  float expv = (CALIB_TX_DBM - (float)rssi) / (10.0 * CALIB_N);
  float d = pow(10.0, expv);
  return d;
}

// English comment: format distance to readable string
String formatDistance(float meters) {
  if (meters < 0) return String("?-m");
  if (meters < 0.1) { // <10cm -> mm
    int mm = (int)round(meters * 1000.0);
    return String(mm) + "mm";
  } else if (meters < 1.0) { // 10cm..1m -> cm
    int cm = (int)round(meters * 100.0);
    return String(cm) + "cm";
  } else if (meters < 10.0) { // 1m..10m -> 1 decimal
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1fm", meters);
    return String(buf);
  } else { // >=10m -> integer meters
    char buf[16];
    snprintf(buf, sizeof(buf), "%.0fm", meters);
    return String(buf);
  }
}

bool wifiConnected(){
  if (WiFi.status() != WL_CONNECTED){
    M5.Lcd.setTextColor(TFT_RED, TFT_PURPLE);
    return false;
  }
  else return true;
}

const int MAX_FOUND = 48;
uint32_t foundIPv4[MAX_FOUND];
int foundCount = 0;

void setup(){
  M5.begin();
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setRotation(1);
  M5.Lcd.println("Network Sentinel");

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  M5.Lcd.print("Connecting to Wi-Fi");
  int timeout = 0;
  while(WiFi.status() != WL_CONNECTED && timeout < 20){
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
  
  if(M5.BtnA.wasPressed()){
    currentMode++;
    if(currentMode>2)currentMode = 0;
    showMenu();
  }

  if(M5.BtnB.wasPressed()){
    runMode(currentMode);
    showMenu();
  }
}

void showMenu(){
  uint16_t bgColor;
  uint16_t textColor;
  uint16_t highlightColor;

  if(currentMode == 0){
    bgColor = BLACK;
    textColor = WHITE;
    highlightColor = WHITE;
  } 
  else if(currentMode == 1){
    bgColor = TFT_NAVY;
    textColor = TFT_CYAN;
    highlightColor = TFT_CYAN;
  } 
  else { // currentMode == 2
    bgColor = TFT_PURPLE;
    textColor = TFT_ORANGE;
    highlightColor = TFT_ORANGE;
  }

  M5.Lcd.fillScreen(bgColor);
  M5.Lcd.setTextColor(textColor, bgColor);
  M5.Lcd.setTextSize(1);

  M5.Lcd.setCursor(10, 0);
  M5.Lcd.println("==== MENU ====");
  displayBattery();

  M5.Lcd.setCursor(0, 30);

  if(currentMode==0){
    M5.Lcd.setTextColor(highlightColor, bgColor);
    M5.Lcd.println("> Network Sentinel");
    M5.Lcd.setTextColor(textColor, bgColor);
    M5.Lcd.println("  WiFi Scanner");
    M5.Lcd.println("  WiFi Config & Status");
  } 
  else if (currentMode==1){
    M5.Lcd.setTextColor(textColor, bgColor);
    M5.Lcd.println("  Network Sentinel");
    M5.Lcd.setTextColor(highlightColor, bgColor);
    M5.Lcd.println("> WiFi Scanner");
    M5.Lcd.setTextColor(textColor, bgColor);
    M5.Lcd.println("  WiFi Config & Status");
  }
  else {
    M5.Lcd.setTextColor(textColor, bgColor);
    M5.Lcd.println("  Network Sentinel");
    M5.Lcd.println("  WiFi Scanner");
    M5.Lcd.setTextColor(highlightColor, bgColor);
    M5.Lcd.println("> WiFi Config & Status");
  }
}

void runMode(int mode){
  switch (mode){
    case 0:
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setTextSize(1);
      M5.Lcd.println("Network Sentinel");
      netScanner();
      delay(1000);
      break;

    case 1:
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setTextSize(1);
      M5.Lcd.println("WiFi Scanner");
      wifiScanner();
      delay(1000);
      break;

    case 2:
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setTextSize(1);
      M5.Lcd.println("WiFi config...");
      showConfig();
      delay(1000);
      break;
  }
}

//--------------------------------MAIN FUNCTIONS----------------------------------------
void displayBattery(){
  int vol = StickCP2.Power.getBatteryVoltage();
  float batPercent = (vol - 3300) / 9.0;
  if (batPercent < 0) batPercent = 0;
  if (batPercent > 100) batPercent = 100;

  StickCP2.Display.setCursor(180, 0);
  StickCP2.Display.printf("Bat: %.0f%%", batPercent);
}

// English comment: draw simple vertical bars at given cursor - not used in compact line mode but kept if needed
void wifiSignalBarsDraw(int rssi, int x, int y, uint16_t color, uint16_t bg) {
  int bars = 0;
  if (rssi >= -50) bars = 4;
  else if (rssi >= -60) bars = 3;
  else if (rssi >= -70) bars = 2;
  else if (rssi >= -80) bars = 1;
  // 4 small rects increasing height
  M5.Lcd.fillRect(x, y - 4, 4, 4, (bars >= 1) ? color : bg);
  M5.Lcd.fillRect(x + 5, y - 8, 4, 8, (bars >= 2) ? color : bg);
  M5.Lcd.fillRect(x + 10, y - 12, 4, 12, (bars >= 3) ? color : bg);
  M5.Lcd.fillRect(x + 15, y - 16, 4, 16, (bars >= 4) ? color : bg);
}

// =============================
// Mode 1: WiFi Scanner (two-line per network layout)
// shows 5 networks per page: SSID (line1) | line2: bars ▂▄▆█ | dBm | distance
// A -> next page (wraparound), B -> back to menu
// After exiting scanner reconnects to CONFIG SSID from config_private.h
// =============================
void wifiScanner() {
  // ensure STA mode and fresh scan
  WiFi.mode(WIFI_STA);
  // run scan (this will temporarily break the current association)
  int n = WiFi.scanNetworks();
  if (n <= 0) {
    M5.Lcd.fillScreen(BLACK);
    displayBattery();
    M5.Lcd.setCursor(0, 30);
    M5.Lcd.setTextColor(TFT_RED, BLACK);
    M5.Lcd.println("No networks found.");
    M5.Lcd.println("\nPress B to return.");
    while (true) {
      M5.update();
      if (M5.BtnB.wasPressed()) {
        // reconnect to configured network before returning
        WiFi.scanDelete();
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        int rt = 0;
        while(WiFi.status() != WL_CONNECTED && rt < 10){ delay(200); rt++; }
        isConnected = wifiConnected();
        return;
      }
      delay(50);
    }
  }

  const int perPage = 5;
  int page = 0;
  int totalPages = (n + perPage - 1) / perPage;

  while (true) {
    M5.Lcd.fillScreen(BLACK);
    // top: battery
    displayBattery();
    // header line below battery (start a bit lower so it doesn't overlap)
    M5.Lcd.setCursor(0, 20);
    M5.Lcd.setTextColor(TFT_CYAN, BLACK);
    M5.Lcd.printf("WiFi Scanner (found: %d)\n\n", n);

    int start = page * perPage;
    int end = min(start + perPage, n);

    for (int i = start; i < end; ++i) {
      // Line 1: SSID (print trimmed to 30 chars to avoid overflow)
      char ssidbuf[36];
      snprintf(ssidbuf, sizeof(ssidbuf), "%.30s", WiFi.SSID(i).c_str());
      M5.Lcd.setTextColor(WHITE, BLACK);
      M5.Lcd.printf("%s\n", ssidbuf);

      // Line 2: bars, dBm, distance
      int rssi = WiFi.RSSI(i);
      String bars = getSignalBarsString(rssi);
      float meters = rssiToMeters(rssi);
      String dist = formatDistance(meters);

      M5.Lcd.setTextColor(TFT_YELLOW, BLACK);
      M5.Lcd.printf("  %s  ", bars.c_str());
      M5.Lcd.setTextColor(TFT_ORANGE, BLACK);
      M5.Lcd.printf("%+4ddBm ", rssi);
      M5.Lcd.setTextColor(TFT_CYAN, BLACK);
      M5.Lcd.printf("%5s\n\n", dist.c_str()); // extra blank line between entries
    }

    M5.Lcd.setTextColor(TFT_GREEN, BLACK);
    M5.Lcd.printf("Page %d/%d  A:Next  B:Back", page + 1, totalPages);

    // wait for input: A -> next (wrap), B -> back
    while (true) {
      M5.update();
      if (M5.BtnA.wasPressed()) {
        page++;
        if (page >= totalPages) page = 0;
        break;
      }
      if (M5.BtnB.wasPressed()) {
        // cleanup scan data and reconnect to configured network
        WiFi.scanDelete();
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        int rt = 0;
        while(WiFi.status() != WL_CONNECTED && rt < 10){ delay(200); rt++; }
        isConnected = wifiConnected();
        return;
      }
      delay(50);
    }
  }
}

// =============================
// Mode 0: Net scanner
// =============================
void netScanner(){
  isConnected = wifiConnected();

  if(!isConnected){
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(1);
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
  M5.Lcd.setTextSize(1);
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
      M5.Lcd.setTextSize(1);
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

//discovery implementation(calculates subnet from mask)
void discoverDevices() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(0,0);
  M5.Lcd.println("Discovering devices\n");

  IPAddress localIP = WiFi.localIP();
  IPAddress subnet = WiFi.subnetMask();

  uint32_t ipNum = ipToUint32(localIP);
  uint32_t maskNum = ipToUint32(subnet);
  uint32_t netNum = ipNum & maskNum;
  uint32_t broadcast = netNum | (~maskNum);

  //limit extremes(avoid iterating 0 or 255 if mask covers full ranges)
  uint32_t start = netNum + 1;
  uint32_t end = broadcast - 1;

  //safety: cap max hosts iterated to avoid insane ranges
  uint32_t maxHostsToScan = 4096; //safety cap
  uint32_t hostsRange = (end >= start) ? (end - start + 1) : 0;
  if (hostsRange > maxHostsToScan) {
    //fallback to /24 centered on current octets
    uint8_t a = localIP[0], b = localIP[1], c = localIP[2];
    start = ipToUint32(IPAddress(a, b, c, 1));
    end = ipToUint32(IPAddress(a, b, c, 254));
  }

  //clear previous results
  foundCount = 0;

  //UI header
  M5.Lcd.print("Local IP: ");
  M5.Lcd.println(localIP);
  M5.Lcd.print("Subnet: ");
  M5.Lcd.println(subnet);
  M5.Lcd.println();
  M5.Lcd.println("Scanning (BtnB abort)\n");

  //scan parameters
  const int pingAttempts   = 1;    // number of ping attempts (keep it small)
  const int tcpTimeoutMs   = 180;  // TCP fallback timeout (ms)
  const int smallDelay     = 8;    // responsiveness between hosts (ms)
  uint32_t scanned = 0;
  uint32_t totalToScan = (end >= start) ? (end - start + 1) : 0;

  //iterate hosts
  for (uint32_t cur = start; cur <= end; ++cur) {
    M5.update();
    if (M5.BtnB.wasPressed()) {
      M5.Lcd.println("\nScan aborted");
      break;
    }

    //skip self
    if (cur == ipNum) { scanned++; continue; }

    IPAddress target = uint32ToIP(cur);

    bool alive = false;
    //ICMP ping by IPAddress (ESP32Ping)
    alive = Ping.ping(target, pingAttempts);

    //fallback: quick TCP connect to common ports
    if (!alive) {
      WiFiClient sock;
      // try 80, 443, 22
      if (sock.connect(target, 80, tcpTimeoutMs)) { alive = true; sock.stop(); }
      else if (sock.connect(target, 443, tcpTimeoutMs)) { alive = true; sock.stop(); }
      else if (sock.connect(target, 22, tcpTimeoutMs)) { alive = true; sock.stop(); }
    }

    if (alive) {
      if (foundCount < MAX_FOUND) foundIPv4[foundCount] = cur;
      foundCount++;
    }

    scanned++;
    //progress update occasionally so user sees activity
    if ((scanned % 12) == 0 || scanned == totalToScan) {
      M5.Lcd.print("Scanned: ");
      M5.Lcd.print(scanned);
      M5.Lcd.print("/");
      M5.Lcd.println(totalToScan);
    }

    // allow quick UI handling and small rest
    M5.update();
    if (M5.BtnB.wasPressed()) {
      M5.Lcd.println("\nScan aborted");
      break;
    }

    delay(smallDelay);
  }

  //summary
  M5.Lcd.println();
  M5.Lcd.print("Found: ");
  M5.Lcd.println(foundCount);
  M5.Lcd.println();
  delay(300);
}

//interactive selection UI — returns index into foundIPv4 or -1 if cancelled
int selectDevice() {
  if (foundCount == 0) return -1;

  const int perPage = 6;
  int page = 0;
  int cursor = 0;

  int totalPages = (foundCount + perPage - 1) / perPage;

  while (true) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(1);
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


void portScan(uint32_t targetIPv4, bool fullScan) {
  IPAddress target = uint32ToIP(targetIPv4);
  // popular ports list
  const int popularPorts[] = {22, 23, 80, 443, 8080, 8443, 3306, 3389};
  const int npop = sizeof(popularPorts) / sizeof(popularPorts[0]);

  // for all ports
  const int startPort = 1;
  const int endPort = 65535;

  const int tcpTimeoutMs = 100; // small timeout to speed up full scan
  const int workers = 4; // number of parallel "workers"
  uint32_t totalPorts = fullScan ? (endPort - startPort + 1) : npop;

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0,0);
  M5.Lcd.printf("Port scan: %s\n", ipv4ToString(targetIPv4).c_str());
  M5.Lcd.println(fullScan ? "Mode: ALL (may take long)" : "Mode: POPULAR");
  M5.Lcd.println("Open ports will be shown. BtnB abort.");

  // prepare port iterator state for workers
  uint32_t nextPortIndex = 0; // index into the port sequence
  uint32_t scanned = 0;

  // helper lambda to get port at index
  auto portAt = [&](uint32_t idx) -> int {
    if (!fullScan) return popularPorts[idx];
    return (int)(startPort + idx);
  };

  // workers array: each iteration we'll try up to 'workers' ports (sequentially, fast timeouts)
  while (nextPortIndex < totalPorts) {
    // check abort
    M5.update();
    if (M5.BtnB.wasPressed()) {
      M5.Lcd.println("\nScan aborted");
      return;
    }

    // For each worker, pick a port and try connect
    for (int w = 0; w < workers && nextPortIndex < totalPorts; ++w) {
      int p = portAt(nextPortIndex);
      nextPortIndex++;

      // attempt connect with small timeout
      WiFiClient sock;
      bool opened = false;
      if (sock.connect(target, p, tcpTimeoutMs)) {
        opened = true;
        sock.stop();
      }

      scanned++;
      // update progress occasionally
      if (scanned % 50 == 0 || scanned == totalPorts) {
        M5.Lcd.fillRect(0, 90, 160, 24, BLACK);
        M5.Lcd.setCursor(0,90);
        M5.Lcd.printf("Progress: %d/%d", scanned, (int)totalPorts);
      }

      if (opened) {
        M5.Lcd.setCursor(0, 60);
        M5.Lcd.setTextColor(TFT_GREEN, BLACK);
        M5.Lcd.printf("Port %d OPEN\n", p);
        // restore text color for progress
        M5.Lcd.setTextColor(WHITE, BLACK);
      }

      // quick UI update and abort check
      M5.update();
      if (M5.BtnB.wasPressed()) {
        M5.Lcd.println("\nScan aborted");
        return;
      }
    } // end workers loop

    // small sleep to not starve CPU / give network stack small break
    delay(10);
  } // end while ports

  M5.Lcd.println("\nDone. Press B to return");
  while (!M5.BtnB.wasPressed()) {
    M5.update();
    delay(50);
  }
}

// =============================
// Mode 2: Show config
// =============================
void showConfig(){
  M5.Lcd.fillScreen(TFT_PURPLE);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(1);
  M5.Lcd.println("WiFi Configuration\n");

  isConnected = wifiConnected();

  if(isConnected==false){
    M5.Lcd.println("WiFi Error! See config_private.h");
    M5.Lcd.println("\nPress A or B to return");
    while(true){
      M5.update();
      if(M5.BtnA.wasPressed() || M5.BtnB.wasPressed()){
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
  int rssi = WiFi.RSSI();
  M5.Lcd.print("RSSI: ");
  M5.Lcd.printf("%d dBm  ", rssi);
 
  M5.Lcd.setTextColor(TFT_CYAN, TFT_PURPLE);
  M5.Lcd.print(getSignalBarsString(rssi));
  M5.Lcd.println();

  M5.Lcd.setTextColor(TFT_CYAN, TFT_PURPLE);
  M5.Lcd.print("MAC: ");
  M5.Lcd.println(WiFi.macAddress());

  M5.Lcd.setTextColor(TFT_WHITE, TFT_PURPLE);
  M5.Lcd.print("Ping: ");
  IPAddress testIP(8,8,8,8);
  bool pingSuccess = false;
  for(int i=0;i<3;i++){
    if(Ping.ping(testIP, 1)){
      pingSuccess = true;
      break;
    }
    delay(200);
  }
  if(pingSuccess) M5.Lcd.println("OK");
  else M5.Lcd.println("FAILED");

  M5.Lcd.println();
  M5.Lcd.println("Press BtnA or BtnB to go back");

  while(true){
    M5.update();
    if(M5.BtnA.wasPressed()||M5.BtnB.wasPressed()){
      return;
    }
    delay(50);
  }
}

String ipv4ToString(uint32_t ip){
  IPAddress a = uint32ToIP(ip);
  return String(a[0]) + "." + String(a[1]) + "." + String(a[2]) + "." + String(a[3]);
}

uint32_t ipToUint32(const IPAddress &ip){
  return (uint32_t)ip[0] << 24 | (uint32_t)ip[1] << 16 | (uint32_t)ip[2] << 8 | (uint32_t)ip[3];
}

IPAddress uint32ToIP(uint32_t v){
  return IPAddress((v >> 24) & 0xFF, (v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF);
}
