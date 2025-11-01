#include <M5StickCPlus2.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>
#include "screen_os.h"
#include "screen_logger.h"
#include "snake_game.h"

extern AsyncWebServer server; // declared in main ino
extern DisplayMode currentMode; // from LedScreen.ino
extern String currentMsg; extern int currentSpeed; extern int currentBrightness; extern textEffect_t currentEffect, currentEffectOut; extern textPosition_t currentAlign; extern int currentPause, currentSpacing, currentScrollSpacing; extern bool displayOn;
extern MD_Parola matrix; // display engine
extern void renderPixelBufferToMatrix();
extern void initializePixelBuffer();
extern const int MATRIX_WIDTH; extern const int MATRIX_HEIGHT;
extern bool isBLEConnected();

// --- OS state ---
static OSView view = VIEW_HOME;
static int appsIndex = 0; // selection in apps menu
static int powerIndex = 0; // selection in power menu
// Draw app state
static int drawX = MATRIX_WIDTH/2;
static int drawY = MATRIX_HEIGHT/2;
static bool drawPen = true; // draw while moving when true
// Text app state
static const char* kbRows[] = {
  "ABCDE",
  "FGHIJ",
  "KLMNO",
  "PQRST",
  "UVWXY",
  "Z _.,"
};
static const int kbRowCount = 6;
static String textInput = "";
static int kbR = 0, kbC = 0; // keyboard cursor
// Clock state
static unsigned long lastClock = 0;
// YT state
static const char* YT_CHANNEL_ID = "UC_x5XG1OV2P6uZZ5FSM9Ttw"; // default: Google Developers
static unsigned long lastYTFetch = 0;
static long ytCount = -1;

// --- Drawing helpers ---
static void drawTopBar() {
  // fixed bar y=0..11
  M5.Lcd.fillRect(0, 0, M5.Lcd.width(), 12, BLACK);
  M5.Lcd.drawFastHLine(0, 12, M5.Lcd.width(), TFT_DARKGREY);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE, BLACK);
  // WiFi icon
  bool wifi = WiFi.status() == WL_CONNECTED;
  M5.Lcd.setCursor(2, 2);
  M5.Lcd.print(wifi ? "📶" : "✖");
  // BT icon
  bool bt = isBLEConnected();
  M5.Lcd.setCursor(18, 2);
  M5.Lcd.print(bt ? "🅱" : "—");
  // Battery placeholder icon at right
  int w = M5.Lcd.width();
  M5.Lcd.drawRect(w-24, 2, 20, 8, WHITE);
  M5.Lcd.fillRect(w-4, 4, 3, 4, WHITE);
  // (no percentage to avoid platform-specific APIs)
}

static void clearContent() {
  // clear below bar
  M5.Lcd.fillRect(0, 13, M5.Lcd.width(), M5.Lcd.height()-13, BLACK);
}

static void drawHome() {
  clearContent();
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(CYAN, BLACK);
  M5.Lcd.setCursor(6, 18);
  M5.Lcd.print("LEDScreen OS");

  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE, BLACK);
  int y = 48;
  M5.Lcd.setCursor(6, y);   M5.Lcd.printf("IP: %s", WiFi.localIP().toString().c_str()); y+=12;
  M5.Lcd.setCursor(6, y);   M5.Lcd.printf("SSID: %s", WiFi.SSID().c_str()); y+=12;
  M5.Lcd.setCursor(6, y);   M5.Lcd.print("Apps: Press X"); y+=12;
  M5.Lcd.setCursor(6, y);   M5.Lcd.print("Power: Menu");
}

static const char* appsList[] = {
  "Draw", "Text", "Snake", "Clock", "YT Count"
};
static const int appsCount = sizeof(appsList)/sizeof(appsList[0]);

static void drawApps() {
  clearContent();
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(YELLOW, BLACK);
  M5.Lcd.setCursor(6, 18); M5.Lcd.print("Apps");
  M5.Lcd.setTextSize(1);
  int y = 42;
  for (int i=0;i<appsCount;i++) {
    M5.Lcd.setCursor(12, y);
    if (i==appsIndex) { M5.Lcd.setTextColor(BLACK, WHITE); } else { M5.Lcd.setTextColor(WHITE, BLACK); }
    M5.Lcd.print(appsList[i]);
    y+=14;
  }
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(6, M5.Lcd.height()-12);
  M5.Lcd.print("X=Open  B=Back  Menu=Power");
}

static void drawPower() {
  clearContent();
  const char* opts[] = {"Reboot", "Power Off", "Back"};
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(RED, BLACK);
  M5.Lcd.setCursor(6, 18); M5.Lcd.print("Power");
  M5.Lcd.setTextSize(1);
  int y=42;
  for (int i=0;i<3;i++) {
    M5.Lcd.setCursor(12, y);
    if (i==powerIndex) { M5.Lcd.setTextColor(BLACK, WHITE); } else { M5.Lcd.setTextColor(WHITE, BLACK); }
    M5.Lcd.print(opts[i]); y+=14;
  }
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(6, M5.Lcd.height()-12);
  M5.Lcd.print("X=Select  B=Back");
}

// --- Helpers for apps ---
static inline void setPixelWeb(int x_web, int y_web, bool state) {
  if (x_web < 0 || x_web >= MATRIX_WIDTH || y_web < 0 || y_web >= MATRIX_HEIGHT) return;
  pixelBuffer[x_web][y_web] = state;
  int x_hw = (MATRIX_WIDTH - 1) - x_web;
  int y_hw = y_web;
  matrix.displaySuspend(true);
  matrix.getGraphicObject()->setPoint(y_hw, x_hw, state);
  matrix.displaySuspend(false);
}

static void drawDrawUI() {
  clearContent();
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(GREEN, BLACK);
  M5.Lcd.setCursor(6, 18); M5.Lcd.print("Draw");
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(6, 42);
  M5.Lcd.printf("Cursor: %d,%d", drawX, drawY);
  M5.Lcd.setCursor(6, 58);
  M5.Lcd.printf("Pen: %s", drawPen ? "ON" : "OFF");
  M5.Lcd.setCursor(6, M5.Lcd.height()-26);
  M5.Lcd.print("X=Toggle Pen  Y=Clear");
  M5.Lcd.setCursor(6, M5.Lcd.height()-12);
  M5.Lcd.print("B=Back");
}

static void drawTextUI() {
  clearContent();
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(CYAN, BLACK);
  M5.Lcd.setCursor(6, 18); M5.Lcd.print("Text");
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE, BLACK);
  // Show current input
  M5.Lcd.setCursor(6, 42);
  M5.Lcd.print(textInput);
  // Draw keyboard grid
  int y = 70;
  for (int r=0; r<kbRowCount; r++) {
    const char* row = kbRows[r];
    int len = strlen(row);
    int x = 10;
    for (int c=0; c<len; c++) {
      if (r==kbR && c==kbC) {
        M5.Lcd.setTextColor(BLACK, WHITE);
      } else {
        M5.Lcd.setTextColor(WHITE, BLACK);
      }
      M5.Lcd.setCursor(x, y);
      M5.Lcd.print(row[c]);
      x += 12;
    }
    y += 14;
  }
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(6, M5.Lcd.height()-26); M5.Lcd.print("X=Add  Y=Backspace");
  M5.Lcd.setCursor(6, M5.Lcd.height()-12); M5.Lcd.print("Menu=Send  B=Back");
}

static void ensureTimeConfigured() {
  static bool timeInit = false;
  if (!timeInit && WiFi.status() == WL_CONNECTED) {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    timeInit = true;
  }
}

static void drawClockUI() {
  clearContent();
  ensureTimeConfigured();
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  char buf1[16], buf2[16];
  strftime(buf1, sizeof(buf1), "%H:%M:%S", &t);
  strftime(buf2, sizeof(buf2), "%Y-%m-%d", &t);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(10, 36); M5.Lcd.print(buf1);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 80); M5.Lcd.print(buf2);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(6, M5.Lcd.height()-12); M5.Lcd.print("B=Back");
}

static void fetchYTCount() {
  if (WiFi.status() != WL_CONNECTED) { ytCount = -1; return; }
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = String("https://api.socialcounts.org/youtube-live-subscriber-count/") + YT_CHANNEL_ID;
  if (!http.begin(client, url)) { ytCount = -1; return; }
  int code = http.GET();
  if (code == HTTP_CODE_OK) {
    StaticJsonDocument<2048> doc;
    DeserializationError err = deserializeJson(doc, http.getString());
    if (!err) {
      // Try common fields; adapt as needed
      if (doc["count"].is<long>()) ytCount = doc["count"].as<long>();
      else if (doc["subscriberCount"].is<long>()) ytCount = doc["subscriberCount"].as<long>();
      else if (doc["estimatedCount"].is<long>()) ytCount = doc["estimatedCount"].as<long>();
    } else {
      ytCount = -1;
    }
  } else {
    ytCount = -1;
  }
  http.end();
}

static void drawYTUI() {
  clearContent();
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(6, 18); M5.Lcd.print("YouTube Count");
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(10, 56);
  if (ytCount >= 0) M5.Lcd.print(ytCount);
  else M5.Lcd.print("--");
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(6, M5.Lcd.height()-26); M5.Lcd.print("X=Refresh");
  M5.Lcd.setCursor(6, M5.Lcd.height()-12); M5.Lcd.print("B=Back");
}

void initOS() {
  // Disable logger overlay and draw first frame
  setLoggerEnabled(false);
  M5.Lcd.fillScreen(BLACK);
  drawTopBar();
  drawHome();
}

void osLoop() {
  // draw top bar each loop to keep status fresh
  drawTopBar();
  switch(view) {
    case VIEW_HOME: break; // static
    case VIEW_APPS: break;
    case VIEW_POWER: break;
    case VIEW_APP_DRAW: break;
    case VIEW_APP_TEXT: break;
    case VIEW_APP_CLOCK:
      if (millis() - lastClock > 500) { lastClock = millis(); drawClockUI(); }
      break;
    case VIEW_APP_YT:
      if (millis() - lastYTFetch > 30000) { lastYTFetch = millis(); fetchYTCount(); drawYTUI(); }
      break;
    default: break;
  }
}

static void enterApp(OSView app) {
  view = app;
  // Configure matrix/app modes as needed
  if (app == VIEW_APP_SNAKE) {
    // enter snake on matrix
    matrix.displaySuspend(false);
    currentMode = MODE_SNAKE;
    resetGame();
  } else if (app == VIEW_APP_DRAW) {
    currentMode = MODE_GRAPHICS;
    matrix.displaySuspend(true);
    renderPixelBufferToMatrix();
    matrix.displaySuspend(false);
    drawDrawUI();
  } else if (app == VIEW_APP_TEXT) {
    currentMode = MODE_TEXT;
    if (currentMsg.length()) {
      matrix.displayText(currentMsg.c_str(), currentAlign, currentSpeed, currentPause, currentEffect, currentEffectOut);
    }
    textInput = ""; kbR = kbC = 0; drawTextUI();
  } else if (app == VIEW_APP_CLOCK) {
    drawClockUI();
  } else if (app == VIEW_APP_YT) {
    fetchYTCount(); drawYTUI();
  }
}

// --- Input routing ---
void osOnDpad(uint8_t dirCode) {
  if (view == VIEW_APPS) {
    if (dirCode == 0x00) { if (appsIndex>0) appsIndex--; drawApps(); }
    else if (dirCode == 0x44) { if (appsIndex<appsCount-1) appsIndex++; drawApps(); }
  } else if (view == VIEW_APP_SNAKE) {
    // route to snake controls
    if (dirCode == 0x00) setSnakeDirection(DIR_UP);
    else if (dirCode == 0x44) setSnakeDirection(DIR_DOWN);
    else if (dirCode == 0x66) setSnakeDirection(DIR_LEFT);
    else if (dirCode == 0x22) setSnakeDirection(DIR_RIGHT);
  } else if (view == VIEW_APP_DRAW) {
    int nx = drawX, ny = drawY;
    if (dirCode == 0x00) ny = max(0, ny-1);
    else if (dirCode == 0x44) ny = min(MATRIX_HEIGHT-1, ny+1);
    else if (dirCode == 0x66) nx = max(0, nx-1);
    else if (dirCode == 0x22) nx = min(MATRIX_WIDTH-1, nx+1);
    drawX = nx; drawY = ny;
    if (drawPen) setPixelWeb(drawX, drawY, true);
    drawDrawUI();
  } else if (view == VIEW_APP_TEXT) {
    // Move keyboard cursor
    if (dirCode == 0x00) { if (kbR>0) kbR--; }
    else if (dirCode == 0x44) { if (kbR<kbRowCount-1) kbR++; }
    else if (dirCode == 0x66) { if (kbC>0) kbC--; }
    else if (dirCode == 0x22) {
      int len = strlen(kbRows[kbR]);
      if (kbC < len-1) kbC++;
    }
    drawTextUI();
  } else if (view == VIEW_POWER) {
    if (dirCode == 0x00) { if (powerIndex>0) powerIndex--; drawPower(); }
    else if (dirCode == 0x44) { if (powerIndex<2) powerIndex++; drawPower(); }
  }
}

void osOnButtonX() {
  if (view == VIEW_HOME) { view = VIEW_APPS; drawApps(); return; }
  if (view == VIEW_APPS) {
    switch(appsIndex) {
      case 0: enterApp(VIEW_APP_DRAW); break;
      case 1: enterApp(VIEW_APP_TEXT); break;
      case 2: enterApp(VIEW_APP_SNAKE); break;
      case 3: view = VIEW_APP_CLOCK; /* TODO: implement clock */ break;
      case 4: view = VIEW_APP_YT; /* YT count */ break;
    }
    return;
  }
  if (view == VIEW_APP_DRAW) { drawPen = !drawPen; drawDrawUI(); return; }
  if (view == VIEW_APP_TEXT) {
    // add selected char
    char ch = kbRows[kbR][kbC];
    textInput += ch;
    drawTextUI();
    return;
  }
  if (view == VIEW_APP_YT) { fetchYTCount(); drawYTUI(); return; }
  if (view == VIEW_POWER) {
    if (powerIndex == 0) { ESP.restart(); }
    else if (powerIndex == 1) { esp_deep_sleep_start(); }
    else { view = VIEW_HOME; drawHome(); }
    return;
  }
}

void osOnButtonB() {
  if (view == VIEW_APPS || view == VIEW_POWER || view==VIEW_APP_CLOCK || view==VIEW_APP_YT) {
    view = VIEW_HOME; drawHome(); return;
  }
  // leaving apps returns to home and leaves matrix in previous state
  if (view == VIEW_APP_DRAW || view == VIEW_APP_TEXT || view == VIEW_APP_SNAKE) {
    view = VIEW_HOME; drawHome(); return;
  }
}

void osOnButtonY() {
  // In Snake app: restart if over
  if (view == VIEW_APP_SNAKE) {
    if (isGameOver) resetGame();
    return;
  }
  if (view == VIEW_APP_DRAW) {
    // Clear canvas
    initializePixelBuffer();
    matrix.displaySuspend(true);
    matrix.displayClear();
    matrix.displaySuspend(false);
    drawDrawUI();
    return;
  }
  if (view == VIEW_APP_TEXT) {
    // Backspace
    if (textInput.length() > 0) textInput.remove(textInput.length()-1);
    drawTextUI();
    return;
  }
}

void osOnMenu() {
  if (view == VIEW_APP_TEXT) {
    // Send text to matrix
    currentMsg = textInput;
    currentMode = MODE_TEXT;
    matrix.displayClear();
    if (currentMsg.length()) {
      matrix.displayText(currentMsg.c_str(), currentAlign, currentSpeed, currentPause, currentEffect, currentEffectOut);
    } else {
      matrix.displayReset();
    }
    // Small feedback on LCD
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(GREEN, BLACK);
    M5.Lcd.setCursor(6, 56);
    M5.Lcd.print("Sent to matrix");
    return;
  }
  view = VIEW_POWER; powerIndex=0; drawPower();
}

void osOnHome() {
  view = VIEW_APPS; appsIndex=0; drawApps();
}

// Use Menu button in Text app to send text to matrix
// Provide explicit hook via osOnMenu already: augment behavior here
void __attribute__((weak)) onTextSendToMatrix() {}
