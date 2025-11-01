#include <NimBLEDevice.h>
#include "snake_game.h" // For snake controls and restart
#include "screen_logger.h" // Minimal on-screen status only

static NimBLEAddress addr("a0:5a:59:70:72:a3", 0);
uint16_t lastBtns;
bool isConnected = false;
static NimBLEClient* gClient = nullptr; // keep client alive
// --- END ---

// (Debug mode: no client callbacks used)

// --- Debug onInput + control mapping ---
void onInput(NimBLERemoteCharacteristic*, uint8_t* d, size_t l, bool) {
  if (l < 6) return;
  uint16_t b = (d[5] << 8) | d[4];
  if (b == lastBtns) return;
  lastBtns = b;

  Serial.println("--- Button Event ---");
  Serial.printf("Raw Hex: d[4]=0x%02X, d[5]=0x%02X\n", d[4], d[5]);

  switch (d[4]) {
    case 0x00: Serial.println("UP");    break;
    case 0x22: Serial.println("RIGHT"); break;
    case 0x44: Serial.println("DOWN");  break;
    case 0x66: Serial.println("LEFT");  break;
  }

  if (d[5] & 0x01) { Serial.println("A"); screenScrollUp(); }
  if (d[5] & 0x02) { Serial.println("B"); screenScrollDown(); }
  if (d[5] & 0x08) { Serial.println("X");
    // Cycle modes: Text -> Graphics -> Snake -> Text (with engine hard reset)
    matrix.displaySuspend(true);
    matrix.displayClear();
    matrix.displayReset();
    if (currentMode == MODE_TEXT) {
      currentMode = MODE_GRAPHICS;
      logToScreen("Mode: Graphics");
      renderPixelBufferToMatrix();
      matrix.displaySuspend(false);
    } else if (currentMode == MODE_GRAPHICS) {
      currentMode = MODE_SNAKE;
      logToScreen("Mode: Snake");
      matrix.displaySuspend(false);
      resetGame(); // draws first frame now
    } else {
      currentMode = MODE_TEXT;
      logToScreen("Mode: Text");
      matrix.displaySuspend(false);
      if (currentMsg != "") {
        matrix.displayText(currentMsg.c_str(), currentAlign, currentSpeed, currentPause, currentEffect, currentEffectOut);
      }
    }
  }
  if (d[5] & 0x10) { Serial.println("Y"); if (isGameOver) resetGame(); }

  // Snake control only in snake mode
  if (currentMode == MODE_SNAKE) {
    switch (d[4]) {
      case 0x00: setSnakeDirection(DIR_UP);    break;
      case 0x44: setSnakeDirection(DIR_DOWN);  break;
      case 0x66: setSnakeDirection(DIR_LEFT);  break;
      case 0x22: setSnakeDirection(DIR_RIGHT); break;
    }
  }
}

// --- Minimal initBLE: connect and subscribe to HID report ---
void initBLE() {
  logToScreen("BLE Debug Mode");
  Serial.println("Initializing BLE in Debug Mode...");
  NimBLEDevice::init("");
  gClient = NimBLEDevice::createClient();

  logToScreen("Connecting controller...");
  if (!gClient->connect(addr)) {
    Serial.println("Connection failed.");
    logToScreen("BLE Connection Failed!");
    return;
  }

  logToScreen("Controller Connected!");
  Serial.println("Controller Connected, searching for HID service...");
  
  // Prefer HID service 0x1812, else scan all
  NimBLERemoteService* hidSvc = nullptr;
  for (auto* s : gClient->getServices(true)) {
    std::string su = s->getUUID().toString();
    if (su.find("1812") != std::string::npos) { hidSvc = s; break; }
  }

  bool subscribedAny = false;

  auto subscribeCharIfInput = [&](NimBLERemoteCharacteristic* ch) {
    // Check Report Reference descriptor 0x2908 to ensure Input (type 0x01)
  NimBLERemoteDescriptor* repRef = ch->getDescriptor(NimBLEUUID((uint16_t)0x2908));
    bool isInput = false;
    if (repRef) {
      std::string v = repRef->readValue();
      if (v.size() >= 2) {
        uint8_t reportId = (uint8_t)v[0];
        uint8_t reportType = (uint8_t)v[1];
        isInput = (reportType == 0x01);
        Serial.printf("ReportRef: id=%u type=%u on handle %u\n", reportId, reportType, ch->getHandle());
      }
    }
    if (!repRef) {
      Serial.printf("No ReportRef desc on handle %u, subscribing as fallback...\n", ch->getHandle());
    }
    if ((isInput || !repRef) && ch->canNotify()) {
      if (ch->subscribe(true, onInput)) {
        Serial.printf("Subscribed to 2a4d handle %u\n", ch->getHandle());
        subscribedAny = true;
      }
    }
  };

  // If HID service found, set Protocol Mode to Report (0x01) and subscribe to input reports
  if (hidSvc) {
    Serial.println("HID service 0x1812 found");
    for (auto* ch : hidSvc->getCharacteristics(true)) {
      std::string cu = ch->getUUID().toString();
      if (cu.find("2a4e") != std::string::npos) { // Protocol Mode
        uint8_t reportMode = 0x01;
        if (ch->canWriteNoResponse() || ch->canWrite()) {
          bool ok = ch->writeValue(&reportMode, 1, true);
          Serial.printf("Set Protocol Mode to Report: %s\n", ok ? "ok" : "fail");
        }
      }
    }
    for (auto* ch : hidSvc->getCharacteristics(true)) {
      std::string cu = ch->getUUID().toString();
      if (cu.find("2a4d") != std::string::npos) {
        subscribeCharIfInput(ch);
      }
    }
  }

  // Fallback: scan all services for 2a4d if not subscribed yet
  if (!subscribedAny) {
    Serial.println("Falling back to subscribe any 2a4d with notify");
    for (auto* s : gClient->getServices(true)) {
      for (auto* ch : s->getCharacteristics(true)) {
        if (ch->getUUID().toString().find("2a4d") != std::string::npos) {
          subscribeCharIfInput(ch);
        }
      }
    }
  }

  if (subscribedAny) {
    logToScreen("Subscribed to HID!");
  } else {
    Serial.println("Could not subscribe to any HID Input report.");
    logToScreen("HID Char not found!");
  }
}