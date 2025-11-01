#include <M5StickCPlus2.h>
#include <vector>
#include "screen_logger.h"

// --- Configuration ---
const int MAX_LOG_LINES = 50; // Max number of log lines to store in memory
const int FONT_HEIGHT = 16;   // Height of a single line of text with size 2
const int HEADER_HEIGHT = 10; // Space for the top header
const int MAX_LINES_ON_SCREEN = (135 - HEADER_HEIGHT) / FONT_HEIGHT; // M5StickCPlus is 135px tall

// --- Global Variables ---
std::vector<String> logBuffer;
int scrollOffset = 0; // How many lines we've scrolled down from the top
bool screenNeedsRedraw = true;
static bool loggerEnabled = true;

// --- Function Implementations ---

void drawScreen() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(TFT_WHITE, BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("Logs: %d/%d | Scroll: %d", logBuffer.size(), MAX_LOG_LINES, scrollOffset);
  M5.Lcd.drawFastHLine(0, 8, M5.Lcd.width(), TFT_DARKGREY);

  M5.Lcd.setTextSize(2);
  int numLinesToDraw = std::min((int)logBuffer.size(), MAX_LINES_ON_SCREEN);
  
  for (int i = 0; i < numLinesToDraw; i++) {
    int bufferIndex = scrollOffset + i;
    if (bufferIndex < logBuffer.size()) {
      M5.Lcd.setCursor(0, HEADER_HEIGHT + (i * FONT_HEIGHT));
      M5.Lcd.print(logBuffer[bufferIndex]);
    }
  }
  screenNeedsRedraw = false;
}

void initScreenLogger() {
  logBuffer.reserve(MAX_LOG_LINES);
  logToScreen("Logger Initialized.");
}

void logToScreen(const String& message) {
  if (!loggerEnabled) return;
  // Add message to buffer
  if (logBuffer.size() >= MAX_LOG_LINES) {
    logBuffer.erase(logBuffer.begin()); // Remove the oldest message
  }
  logBuffer.push_back(message);

  // Auto-scroll to the bottom if we are not manually scrolled up
  if (scrollOffset == (int)logBuffer.size() - MAX_LINES_ON_SCREEN -1 || logBuffer.size() <= MAX_LINES_ON_SCREEN) {
      scrollOffset = std::max(0, (int)logBuffer.size() - MAX_LINES_ON_SCREEN);
  }
  
  screenNeedsRedraw = true;
}

void handleScreenScroll() {
  bool scrolled = false;
  // Scroll Down (view older messages) with BtnA
  if (M5.BtnA.wasPressed()) {
    if (scrollOffset > 0) {
      scrollOffset--;
      scrolled = true;
    }
  }
  // Scroll Up (view newer messages) with BtnB
  if (M5.BtnB.wasPressed()) {
    if (scrollOffset < (int)logBuffer.size() - MAX_LINES_ON_SCREEN) {
      scrollOffset++;
      scrolled = true;
    }
  }

  if (scrolled) {
    screenNeedsRedraw = true;
  }

  if (screenNeedsRedraw) {
    drawScreen();
  }
}

// --- NEW: Programmatic scroll controls for BLE A/B buttons ---
void screenScrollUp() {
  if (scrollOffset > 0) {
    scrollOffset--;
    screenNeedsRedraw = true;
  }
}

void screenScrollDown() {
  if (scrollOffset < (int)logBuffer.size() - MAX_LINES_ON_SCREEN) {
    scrollOffset++;
    screenNeedsRedraw = true;
  }
}

void setLoggerEnabled(bool enabled) {
  loggerEnabled = enabled;
}
