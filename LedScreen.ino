#include <WiFi.h>
#include <ESPAsyncWebServer.h> // For WebSockets and async operations
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <M5StickCPlus2.h>
#include <ArduinoJson.h> // Required for WebSocket parsing
#include <pgmspace.h>
#include <NimBLEDevice.h>
#include "webpage.h" 
#include "snake_game.h"  // --- NEW --- To share game functions
#include "ui_manager.h" // --- NEW --- For M5StickC UI system

// ---------------- LED MATRIX SETUP ----------------
#define HARDWARE_TYPE MD_MAX72XX::ICSTATION_HW
#define MAX_DEVICES 4 // 4 modules = 32 columns
#define DATA_PIN 0
#define CLK_PIN 26
#define CS_PIN 25

MD_Parola matrix = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// ---------------- WIFI SETUP ----------------
const char* ssid = "Sheryians HQ";
const char* password = "Unique@1011";

// ---------------- WEB SERVER & WEBSOCKETS ----------------
AsyncWebServer server(80); 
AsyncWebSocket ws("/ws"); 
#include "input_events.h"

// ---------------- GLOBAL SETTINGS ----------------
// DisplayMode enum is declared in ui_manager.h
DisplayMode currentMode = MODE_TEXT;

String currentMsg = "WELCOME TO SRC - ";
int currentSpeed = 70;
int currentBrightness = 10;
textEffect_t currentEffect = PA_SCROLL_LEFT;
textEffect_t currentEffectOut = PA_SCROLL_LEFT;
bool currentInvert = false; 
textPosition_t currentAlign = PA_RIGHT; // Default to "Left" (PA_RIGHT)
int currentPause = 0; 
int currentSpacing = 1; 
int currentScrollSpacing = 3;
bool displayOn = true;   

const int MATRIX_WIDTH = MAX_DEVICES * 8; 
const int MATRIX_HEIGHT = 8;
bool pixelBuffer[MATRIX_WIDTH][MATRIX_HEIGHT]; 

// --- NEW --- Prototypes for functions in other .ino files
extern void initBLE();
extern void snakeGameLoop();
extern void resetGame();
extern void initializePixelBuffer();
extern void handleRoot(AsyncWebServerRequest *request);
extern void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);



// ---------------- SETUP ----------------
void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);
  initUI(); // Initialize the UI system
  
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 30) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("IP Address: "); Serial.println(WiFi.localIP());
    setWiFiConnected(true);
  } else {
    Serial.println("Wi-Fi Failed!");
    setWiFiConnected(false);
  }

  // Start Bluetooth
  initBLE();

  matrix.begin();
  matrix.setIntensity(currentBrightness); 
  matrix.setInvert(currentInvert);
  matrix.setCharSpacing(currentSpacing);
  matrix.setScrollSpacing(currentScrollSpacing);
  matrix.displayShutdown(!displayOn);
  
  matrix.displayClear();
  matrix.displayText(currentMsg.c_str(), currentAlign, currentSpeed, currentPause, currentEffect, currentEffectOut);

  initializePixelBuffer();

  server.on("/", HTTP_GET, handleRoot); 
  ws.onEvent(onWsEvent); 
  server.addHandler(&ws); 
  server.begin();

  Serial.println("Web Server Ready!");
}

// ---------------- LOOP ----------------
volatile uint32_t gInputEvents = 0; // defined here
static void processInputEvents() {
  uint32_t ev = gInputEvents;
  if (!ev) return;
  gInputEvents = 0; // clear all captured events

  // Top-level
  if (ev & EV_HOME) uiHome();
  if (ev & EV_MENU) uiMenu();

  // Menus (Apps/Power/Settings/Text Settings)
  if (currentUIState == UI_APPS || currentUIState == UI_POWER_MENU ||
      currentUIState == UI_SETTINGS || currentUIState == UI_TEXT_SETTINGS) {
    if (ev & EV_NAV_UP)   uiNavigateUp();
    if (ev & EV_NAV_DOWN) uiNavigateDown();
    if (ev & EV_SELECT)   uiSelect();
    if (ev & EV_BACK)     uiBack();
    // Value adjustments everywhere via X/Y
    if (ev & EV_INC)      uiIncrease();
    if (ev & EV_DEC)      uiDecrease();
  }

  // App running
  if (currentUIState == UI_APP_RUNNING) {
    if (ev & EV_BACK) uiBack();
    // Open text settings from select while in text mode
    if ((ev & EV_SELECT) && currentMode == MODE_TEXT) {
      openTextSettings();
      return;
    }
    if (currentMode == MODE_SNAKE) {
      extern bool isGameOver;
      if ((ev & EV_SNAKE_RESTART) && isGameOver) {
        resetGame();
      } else {
        if (ev & EV_SNAKE_UP)    setSnakeDirection(DIR_UP);
        if (ev & EV_SNAKE_DOWN)  setSnakeDirection(DIR_DOWN);
        if (ev & EV_SNAKE_LEFT)  setSnakeDirection(DIR_LEFT);
        if (ev & EV_SNAKE_RIGHT) setSnakeDirection(DIR_RIGHT);
      }
    }
  }
}
unsigned long lastBatteryUpdate = 0;
const unsigned long BATTERY_UPDATE_INTERVAL = 30000; // Update every 30 seconds

void loop() {
  M5.update();
  // Map onboard M5 buttons to events
  static bool pwrLongSent = false;
  if (M5.BtnA.wasPressed()) postInputEvent(EV_NAV_UP);
  // In Settings/TextSettings, BtnB short = Back; otherwise Down
  if (M5.BtnB.wasPressed()) {
    if (currentUIState == UI_SETTINGS || currentUIState == UI_TEXT_SETTINGS) postInputEvent(EV_BACK);
    else postInputEvent(EV_NAV_DOWN);
  }
  if (M5.BtnA.pressedFor(700)) postInputEvent(EV_HOME);
  if (M5.BtnB.pressedFor(700)) postInputEvent(EV_BACK);
  if (M5.BtnPWR.wasPressed()) postInputEvent(EV_SELECT);
  if (M5.BtnPWR.pressedFor(1200)) { if (!pwrLongSent) { postInputEvent(EV_MENU); pwrLongSent = true; } }
  if (!M5.BtnPWR.isPressed()) pwrLongSent = false;

  processInputEvents();
  
  // Update battery level periodically
  unsigned long now = millis();
  if (now - lastBatteryUpdate >= BATTERY_UPDATE_INTERVAL) {
    updateBatteryLevel();
    lastBatteryUpdate = now;
  }
  
  // UI now updates only on events to avoid flicker; no periodic full redraw here

  switch (currentMode) {
    case MODE_TEXT:
      // Animate text only in this mode
      if (matrix.displayAnimate()) {
        if (currentEffect != PA_PRINT && currentMsg != "") {
          matrix.displayReset();
        }
      }
      break;

    case MODE_SNAKE:
      // Run snake game loop only in this mode
      snakeGameLoop();
      break;

    case MODE_GRAPHICS:
      // Do nothing here. The display is static and updated on demand.
      break;
  }
}