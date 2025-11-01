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
#include "screen_logger.h" // --- NEW --- For M5StickC screen logging

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

// ---------------- GLOBAL SETTINGS ----------------
enum DisplayMode {
  MODE_TEXT,
  MODE_GRAPHICS,
  MODE_SNAKE // --- NEW ---
};
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
  initScreenLogger(); // --- NEW --- Initialize the screen logger
  logToScreen("Connecting Wi-Fi...");
  
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
    logToScreen("Wi-Fi Connected!");
    logToScreen("SSID: " + String(ssid));
    logToScreen("IP: " + WiFi.localIP().toString());
  } else {
    logToScreen("Wi-Fi Failed!");
  }

  // --- NEW --- Start Bluetooth
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

  logToScreen("Web Server Ready!");
  logToScreen("IP: " + WiFi.localIP().toString());
}

// ---------------- LOOP ----------------
void loop() {
  M5.update();
  handleScreenScroll();

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