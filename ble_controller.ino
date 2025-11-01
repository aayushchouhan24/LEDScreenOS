#include <NimBLEDevice.h>
#include <cstring>
#include "snake_game.h" // For snake controls and restart
#include "ui_manager.h" // For UI state/types
#include "input_events.h" // For posting events to main loop

static NimBLEAddress addr("a0:5a:59:70:72:a3", 0);
// Track last packet/buttons to reduce spam and detect edges
static uint8_t lastData[10] = {0};
static uint16_t lastButtons = 0;
static bool lastA = false, lastB = false, lastX = false, lastY = false, lastLB = false, lastRB = false;
static bool lastDU = false, lastDD = false, lastDL = false, lastDR = false;
static bool lastMenu = false, lastSelect = false, lastHome = false;
static bool lastL3 = false, lastR3 = false;
bool isConnected = false;
static NimBLEClient* gClient = nullptr; // keep client alive

// Controller button masks (from your data)
#define BTN_DPAD_UP    0x00
#define BTN_DPAD_RIGHT 0x22
#define BTN_DPAD_DOWN  0x44
#define BTN_DPAD_LEFT  0x66
#define BTN_DPAD_NEUTRAL 0x88

#define BTN_A  0x01
#define BTN_B  0x02
#define BTN_X  0x08
#define BTN_Y  0x10
#define BTN_LB 0x40
#define BTN_RB 0x80
#define BTN_SELECT 0x04
#define BTN_MENU   0x08

// (Debug mode: no client callbacks used)

// --- Controller input handler with full 10-byte decode and UI navigation ---
void onInput(NimBLERemoteCharacteristic*, uint8_t* data, size_t len, bool) {
  if (len < 9) return; // Need at least first 9 bytes

  // Collapse face+dpad into 16-bit for quick change detect (upper byte is data[5])
  uint16_t buttons = (data[5] << 8) | data[4];
  if (memcmp(data, lastData, len) == 0 && buttons == lastButtons) return;
  memcpy(lastData, data, len);
  lastButtons = buttons;

  auto logChange = [](const char* name, bool current, bool& last) {
    if (current != last) {
      last = current;
      Serial.printf("%s %s\n", name, current ? "pressed" : "released");
    }
  };

  // Face buttons from upper byte
  bool A  = buttons & 0x0100;
  bool B  = buttons & 0x0200;
  bool X  = buttons & 0x0800;
  bool Y  = buttons & 0x1000;
  bool LB = buttons & 0x4000;
  bool RB = buttons & 0x8000;

  // Menu cluster / stick buttons
  bool Menu   = data[6] & 0x08;
  bool Select = data[6] & 0x04;
  bool Home   = data[6] & 0x10;
  bool L3     = data[6] & 0x01;
  bool R3     = data[6] & 0x40;

  // D-Pad hat (lower nibble)
  uint8_t hat = data[4] & 0x0F;
  bool up=false, down=false, left=false, right=false;
  switch (hat) {
    case 0: up = true; break;
    case 1: up = true; right = true; break;
    case 2: right = true; break;
    case 3: down = true; right = true; break;
    case 4: down = true; break;
    case 5: down = true; left = true; break;
    case 6: left = true; break;
    case 7: up = true; left = true; break;
    default: break;
  }

  // Optional logs on edges
  logChange("A", A, lastA); logChange("B", B, lastB); logChange("X", X, lastX); logChange("Y", Y, lastY);
  logChange("LB", LB, lastLB); logChange("RB", RB, lastRB);
  logChange("Menu", Menu, lastMenu); logChange("Select", Select, lastSelect); logChange("Home", Home, lastHome);
  logChange("L3", L3, lastL3); logChange("R3", R3, lastR3);
  logChange("D-Up", up, lastDU); logChange("D-Down", down, lastDD); logChange("D-Left", left, lastDL); logChange("D-Right", right, lastDR);

  // Triggers (analog)
  uint8_t rt = data[7];
  uint8_t lt = data[8];
  if (lt > 10) Serial.printf("LT: %u\n", lt);
  if (rt > 10) Serial.printf("RT: %u\n", rt);

  // Sticks
  int lx = (int)data[0] - 128;
  int ly = (int)data[1] - 128;
  int rx = (int)data[2] - 128;
  int ry = (int)data[3] - 128;
  if (abs(lx) > 5 || abs(ly) > 5) Serial.printf("Left Stick:  X=%d Y=%d\n", lx, ly);
  if (abs(rx) > 5 || abs(ry) > 5) Serial.printf("Right Stick: X=%d Y=%d\n", rx, ry);

  // --- Post events for main loop processing (avoid UI work in BLE task) ---
  if (Home)           { postInputEvent(EV_HOME); }
  if (Menu)           { postInputEvent(EV_MENU); }

  // Navigation and selection/back
  // Use ONLY D-Pad for navigation
  if (up)             { postInputEvent(EV_NAV_UP); }
  if (down)           { postInputEvent(EV_NAV_DOWN); }
  // A=Enter, B=Back
  if (A)              { postInputEvent(EV_SELECT); }
  if (B)              { postInputEvent(EV_BACK); }
  // X=Decrease, Y=Increase
  if (X)              { postInputEvent(EV_DEC); }
  if (Y)              { postInputEvent(EV_INC); }

  // Snake/game related
  if (up)             { postInputEvent(EV_SNAKE_UP); }
  if (down)           { postInputEvent(EV_SNAKE_DOWN); }
  if (left)           { postInputEvent(EV_SNAKE_LEFT); }
  if (right)          { postInputEvent(EV_SNAKE_RIGHT); }
}

// --- Minimal initBLE: connect and subscribe to HID report ---
void initBLE() {
  setBTConnected(false);
  Serial.println("Initializing BLE in Debug Mode...");
  NimBLEDevice::init("");
  gClient = NimBLEDevice::createClient();

  if (!gClient->connect(addr)) {
    Serial.println("Connection failed.");
    return;
  }

  setBTConnected(true);
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
    Serial.println("✅ Subscribed to HID!");
  } else {
    Serial.println("❌ Could not subscribe to any HID Input report.");
    setBTConnected(false);
  }
}