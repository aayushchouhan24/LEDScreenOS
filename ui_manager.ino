#include <M5StickCPlus2.h>
#include <WiFi.h>
#include <MD_Parola.h>
#include "ui_manager.h"

// UI State
UIState currentUIState = UI_HOME;
int selectedAppIndex = 0;
int selectedPowerOption = 0;
int selectedSettingIndex = 0;
int selectedTextSettingIndex = 0;
bool wifiConnected = false;
bool btConnected = false;
int batteryLevel = 0;

// Screen dimensions
const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 135;
const int STATUS_BAR_HEIGHT = 16;
const int CONTENT_Y = STATUS_BAR_HEIGHT + 2;

// App names
const char* appNames[] = {"Snake Game", "Text Display", "Graphics", "Settings"};
const int APP_COUNT = 4;

// Power options
const char* powerOptions[] = {"Reboot", "Power Off"};
const int POWER_OPTION_COUNT = 2;

// Externals from main sketch controlling LED matrix text
extern MD_Parola matrix;
extern int currentSpeed;
extern int currentBrightness;
extern textEffect_t currentEffect;
extern textEffect_t currentEffectOut;
extern bool currentInvert;
extern textPosition_t currentAlign;
extern int currentPause;
extern int currentSpacing;
extern int currentScrollSpacing;
extern bool displayOn;
extern String currentMsg;

void initUI() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextSize(1);
  currentUIState = UI_HOME;
  updateBatteryLevel();
  updateUI();
}

void updateUI() {
  M5.Lcd.startWrite();
  M5.Lcd.fillScreen(BLACK);
  drawStatusBar();
  
  switch (currentUIState) {
    case UI_HOME:
      drawHomeScreen();
      break;
    case UI_APPS:
      drawAppsMenu();
      break;
    case UI_POWER_MENU:
      drawPowerMenu();
      break;
    case UI_APP_RUNNING:
      // Minimal UI when app is running
      break;
    case UI_SETTINGS:
      drawSettingsMenu();
      break;
    case UI_TEXT_SETTINGS:
      drawTextSettingsMenu();
      break;
  }
  M5.Lcd.endWrite();
}

void drawStatusBar() {
  M5.Lcd.startWrite();
  M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, STATUS_BAR_HEIGHT, TFT_DARKGREY);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_DARKGREY);
  M5.Lcd.setTextSize(1);
  
  // Battery icon and percentage (right side)
  M5.Lcd.setCursor(SCREEN_WIDTH - 55, 4);
  if (batteryLevel > 75) M5.Lcd.print("█");
  else if (batteryLevel > 50) M5.Lcd.print("▓");
  else if (batteryLevel > 25) M5.Lcd.print("▒");
  else M5.Lcd.print("░");
  M5.Lcd.printf(" %d%%", batteryLevel);
  
  // WiFi status (middle-right)
  M5.Lcd.setCursor(SCREEN_WIDTH - 100, 4);
  if (wifiConnected) {
    M5.Lcd.setTextColor(TFT_GREEN, TFT_DARKGREY);
    M5.Lcd.print("WiFi");
  } else {
    M5.Lcd.setTextColor(TFT_RED, TFT_DARKGREY);
    M5.Lcd.print("WiFi");
  }
  
  // BT status (middle-left)
  M5.Lcd.setCursor(SCREEN_WIDTH - 150, 4);
  if (btConnected) {
    M5.Lcd.setTextColor(TFT_CYAN, TFT_DARKGREY);
    M5.Lcd.print("BT");
  } else {
    M5.Lcd.setTextColor(TFT_RED, TFT_DARKGREY);
    M5.Lcd.print("BT");
  }
  
  // Title (left)
  M5.Lcd.setTextColor(TFT_WHITE, TFT_DARKGREY);
  M5.Lcd.setCursor(4, 4);
  M5.Lcd.print("LED OS");
  M5.Lcd.endWrite();
}

void drawHomeScreen() {
  M5.Lcd.setTextColor(TFT_WHITE, BLACK);
  M5.Lcd.setTextSize(2);
  
  int y = CONTENT_Y + 10;
  
  // Title
  M5.Lcd.setCursor(10, y);
  M5.Lcd.setTextColor(TFT_ORANGE, BLACK);
  M5.Lcd.println("Home");
  y += 20;
  
  // IP Address
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(TFT_CYAN, BLACK);
  M5.Lcd.setCursor(10, y);
  M5.Lcd.print("IP: ");
  M5.Lcd.setTextColor(TFT_WHITE, BLACK);
  if (wifiConnected) {
    M5.Lcd.println(WiFi.localIP().toString());
  } else {
    M5.Lcd.println("Not Connected");
  }
  y += 14;
  
  // Current Mode
  M5.Lcd.setTextColor(TFT_CYAN, BLACK);
  M5.Lcd.setCursor(10, y);
  M5.Lcd.print("Mode: ");
  M5.Lcd.setTextColor(TFT_WHITE, BLACK);
  if (currentMode == MODE_TEXT) M5.Lcd.println("Text");
  else if (currentMode == MODE_GRAPHICS) M5.Lcd.println("Graphics");
  else if (currentMode == MODE_SNAKE) M5.Lcd.println("Snake");
  y += 14;
  
  // Free RAM
  M5.Lcd.setTextColor(TFT_CYAN, BLACK);
  M5.Lcd.setCursor(10, y);
  M5.Lcd.print("RAM: ");
  M5.Lcd.setTextColor(TFT_WHITE, BLACK);
  M5.Lcd.printf("%d KB\n", ESP.getFreeHeap() / 1024);
  y += 14;
  
  // Instructions
  y += 10;
  M5.Lcd.setTextColor(TFT_YELLOW, BLACK);
  M5.Lcd.setCursor(10, y);
  M5.Lcd.println("Press HOME for Apps");
  y += 12;
  M5.Lcd.setCursor(10, y);
  M5.Lcd.println("Press MENU for Power");
}

void drawAppsMenu() {
  M5.Lcd.setTextColor(TFT_WHITE, BLACK);
  M5.Lcd.setTextSize(2);
  
  int y = CONTENT_Y + 5;
  
  // Title
  M5.Lcd.setCursor(10, y);
  M5.Lcd.setTextColor(TFT_ORANGE, BLACK);
  M5.Lcd.println("Apps");
  y += 22;
  
  // App list
  M5.Lcd.setTextSize(1);
  for (int i = 0; i < APP_COUNT; i++) {
    M5.Lcd.setCursor(15, y);
    
    if (i == selectedAppIndex) {
      M5.Lcd.fillRect(10, y - 2, SCREEN_WIDTH - 20, 14, TFT_BLUE);
      M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
      M5.Lcd.print("> ");
    } else {
      M5.Lcd.setTextColor(TFT_WHITE, BLACK);
      M5.Lcd.print("  ");
    }
    
    M5.Lcd.println(appNames[i]);
    y += 16;
  }
  
  // Instructions
  y = SCREEN_HEIGHT - 24;
  M5.Lcd.setTextColor(TFT_YELLOW, BLACK);
  M5.Lcd.setCursor(10, y);
  M5.Lcd.println("D-Pad:Move  A:Enter  B:Back  X:-  Y:+");
}

void drawPowerMenu() {
  M5.Lcd.setTextColor(TFT_WHITE, BLACK);
  M5.Lcd.setTextSize(2);
  
  int y = CONTENT_Y + 20;
  
  // Title
  M5.Lcd.setCursor(10, y);
  M5.Lcd.setTextColor(TFT_RED, BLACK);
  M5.Lcd.println("Power Menu");
  y += 30;
  
  // Options
  M5.Lcd.setTextSize(1);
  for (int i = 0; i < POWER_OPTION_COUNT; i++) {
    M5.Lcd.setCursor(15, y);
    
    if (i == selectedPowerOption) {
      M5.Lcd.fillRect(10, y - 2, SCREEN_WIDTH - 20, 14, TFT_RED);
      M5.Lcd.setTextColor(TFT_WHITE, TFT_RED);
      M5.Lcd.print("> ");
    } else {
      M5.Lcd.setTextColor(TFT_WHITE, BLACK);
      M5.Lcd.print("  ");
    }
    
    M5.Lcd.println(powerOptions[i]);
    y += 20;
  }
  
  // Instructions
  y = SCREEN_HEIGHT - 24;
  M5.Lcd.setTextColor(TFT_YELLOW, BLACK);
  M5.Lcd.setCursor(10, y);
  M5.Lcd.println("D-Pad:Move  A:Enter  B:Back  X:-  Y:+");
}

void drawSettingsMenu() {
  M5.Lcd.setTextColor(TFT_WHITE, BLACK);
  M5.Lcd.setTextSize(2);
  int y = CONTENT_Y + 5;
  M5.Lcd.setCursor(10, y);
  M5.Lcd.setTextColor(TFT_ORANGE, BLACK);
  M5.Lcd.println("Settings");
  y += 22;
  M5.Lcd.setTextSize(1);

  for (int i = 0; i < 2; i++) {
    M5.Lcd.setCursor(15, y);
    if (i == selectedSettingIndex) {
      M5.Lcd.fillRect(10, y - 2, SCREEN_WIDTH - 20, 14, TFT_BLUE);
      M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
      M5.Lcd.print("> ");
    } else {
      M5.Lcd.setTextColor(TFT_WHITE, BLACK);
      M5.Lcd.print("  ");
    }
    if (i == 0) {
      M5.Lcd.printf("Brightness: %d\n", currentBrightness);
    } else {
      M5.Lcd.printf("Invert: %s\n", currentInvert ? "ON" : "OFF");
    }
    y += 16;
  }

  y = SCREEN_HEIGHT - 24;
  M5.Lcd.setTextColor(TFT_YELLOW, BLACK);
  M5.Lcd.setCursor(10, y);
  M5.Lcd.println("D-Pad:Move  A:Enter  B:Back  X:-  Y:+");
}

void drawTextSettingsMenu() {
  M5.Lcd.setTextColor(TFT_WHITE, BLACK);
  M5.Lcd.setTextSize(2);
  int y = CONTENT_Y + 5;
  M5.Lcd.setCursor(10, y);
  M5.Lcd.setTextColor(TFT_ORANGE, BLACK);
  M5.Lcd.println("Text Settings");
  y += 22;
  M5.Lcd.setTextSize(1);

  const char* names[] = {
    "Speed", "Brightness", "Effect", "Align", "Invert",
    "Pause", "Spacing", "Scroll Spacing", "Display On"
  };
  const int COUNT = 9;

  for (int i = 0; i < COUNT; i++) {
    M5.Lcd.setCursor(15, y);
    if (i == selectedTextSettingIndex) {
      M5.Lcd.fillRect(10, y - 2, SCREEN_WIDTH - 20, 14, TFT_BLUE);
      M5.Lcd.setTextColor(TFT_WHITE, TFT_BLUE);
      M5.Lcd.print("> ");
    } else {
      M5.Lcd.setTextColor(TFT_WHITE, BLACK);
      M5.Lcd.print("  ");
    }

    if (i == 0) M5.Lcd.printf("%s: %d\n", names[i], currentSpeed);
    else if (i == 1) M5.Lcd.printf("%s: %d\n", names[i], currentBrightness);
    else if (i == 2) M5.Lcd.printf("%s: %d\n", names[i], (int)currentEffect);
    else if (i == 3) M5.Lcd.printf("%s: %d\n", names[i], (int)currentAlign);
    else if (i == 4) M5.Lcd.printf("%s: %s\n", names[i], currentInvert?"ON":"OFF");
    else if (i == 5) M5.Lcd.printf("%s: %d\n", names[i], currentPause);
    else if (i == 6) M5.Lcd.printf("%s: %d\n", names[i], currentSpacing);
    else if (i == 7) M5.Lcd.printf("%s: %d\n", names[i], currentScrollSpacing);
    else if (i == 8) M5.Lcd.printf("%s: %s\n", names[i], displayOn?"YES":"NO");
    y += 16;
  }

  y = SCREEN_HEIGHT - 24;
  M5.Lcd.setTextColor(TFT_YELLOW, BLACK);
  M5.Lcd.setCursor(10, y);
  M5.Lcd.println("D-Pad:Move  A:Next  B:Exit  X:-  Y:+");
}
// Navigation functions
void uiNavigateUp() {
  if (currentUIState == UI_APPS) {
    selectedAppIndex = (selectedAppIndex - 1 + APP_COUNT) % APP_COUNT;
    updateUI();
  } else if (currentUIState == UI_POWER_MENU) {
    selectedPowerOption = (selectedPowerOption - 1 + POWER_OPTION_COUNT) % POWER_OPTION_COUNT;
    updateUI();
  } else if (currentUIState == UI_SETTINGS) {
    selectedSettingIndex = (selectedSettingIndex - 1 + 2) % 2;
    updateUI();
  } else if (currentUIState == UI_TEXT_SETTINGS) {
    // Increase current field value
    switch (selectedTextSettingIndex) {
      case 0: currentSpeed = min(200, currentSpeed + 5); break;
      case 1: currentBrightness = min(15, currentBrightness + 1); matrix.setIntensity(currentBrightness); break;
      case 2: currentEffect = (textEffect_t)((int)currentEffect + 1); break;
      case 3: currentAlign = (textPosition_t)((int)currentAlign + 1); break;
      case 4: currentInvert = true; break;
      case 5: currentPause = min(2000, currentPause + 50); break;
      case 6: currentSpacing = min(5, currentSpacing + 1); break;
      case 7: currentScrollSpacing = min(10, currentScrollSpacing + 1); break;
      case 8: displayOn = true; break;
    }
    matrix.setInvert(currentInvert);
    matrix.setCharSpacing(currentSpacing);
    matrix.setScrollSpacing(currentScrollSpacing);
    matrix.setIntensity(currentBrightness);
    matrix.displayShutdown(!displayOn);
    if (currentMode == MODE_TEXT) {
      if (currentMsg.length()) matrix.displayText(currentMsg.c_str(), currentAlign, currentSpeed, currentPause, currentEffect, currentEffect);
      else matrix.displayReset();
    }
    updateUI();
  }
}

void uiNavigateDown() {
  if (currentUIState == UI_APPS) {
    selectedAppIndex = (selectedAppIndex + 1) % APP_COUNT;
    updateUI();
  } else if (currentUIState == UI_POWER_MENU) {
    selectedPowerOption = (selectedPowerOption + 1) % POWER_OPTION_COUNT;
    updateUI();
  } else if (currentUIState == UI_SETTINGS) {
    selectedSettingIndex = (selectedSettingIndex + 1) % 2;
    updateUI();
  } else if (currentUIState == UI_TEXT_SETTINGS) {
    // Decrease current field value
    switch (selectedTextSettingIndex) {
      case 0: currentSpeed = max(0, currentSpeed - 5); break;
      case 1: currentBrightness = max(0, currentBrightness - 1); matrix.setIntensity(currentBrightness); break;
      case 2: currentEffect = (textEffect_t)max(0, (int)currentEffect - 1); break;
      case 3: currentAlign = (textPosition_t)max(0, (int)currentAlign - 1); break;
      case 4: currentInvert = false; break;
      case 5: currentPause = max(0, currentPause - 50); break;
      case 6: currentSpacing = max(0, currentSpacing - 1); break;
      case 7: currentScrollSpacing = max(0, currentScrollSpacing - 1); break;
      case 8: displayOn = false; break;
    }
    matrix.setInvert(currentInvert);
    matrix.setCharSpacing(currentSpacing);
    matrix.setScrollSpacing(currentScrollSpacing);
    matrix.setIntensity(currentBrightness);
    matrix.displayShutdown(!displayOn);
    if (currentMode == MODE_TEXT) {
      if (currentMsg.length()) matrix.displayText(currentMsg.c_str(), currentAlign, currentSpeed, currentPause, currentEffect, currentEffect);
      else matrix.displayReset();
    }
    updateUI();
  }
}

void uiNavigateLeft() {
  // Future: horizontal navigation
}

void uiNavigateRight() {
  // Future: horizontal navigation
}

void uiSelect() {
  if (currentUIState == UI_APPS) {
    // Launch selected app
    currentUIState = UI_APP_RUNNING;
    extern void resetGame();
    
    switch (selectedAppIndex) {
      case APP_SNAKE:
        currentMode = MODE_SNAKE;
        resetGame();
        M5.Lcd.fillScreen(BLACK);
        drawStatusBar();
        M5.Lcd.setTextColor(TFT_GREEN, BLACK);
        M5.Lcd.setCursor(10, CONTENT_Y + 10);
        M5.Lcd.setTextSize(2);
        M5.Lcd.println("Snake Game");
        M5.Lcd.setTextSize(1);
        M5.Lcd.setCursor(10, CONTENT_Y + 35);
        M5.Lcd.setTextColor(TFT_YELLOW, BLACK);
        M5.Lcd.println("Press B to exit");
        break;
      case APP_TEXT:
        currentMode = MODE_TEXT;
        M5.Lcd.fillScreen(BLACK);
        drawStatusBar();
        M5.Lcd.setTextColor(TFT_MAGENTA, BLACK);
        M5.Lcd.setCursor(10, CONTENT_Y + 10);
        M5.Lcd.setTextSize(2);
        M5.Lcd.println("Text Mode");
        M5.Lcd.setTextSize(1);
        M5.Lcd.setCursor(10, CONTENT_Y + 35);
        M5.Lcd.setTextColor(TFT_YELLOW, BLACK);
        M5.Lcd.println("Use Web UI to control");
        M5.Lcd.setCursor(10, CONTENT_Y + 50);
        M5.Lcd.println("Press B to exit");
        break;
      case APP_GRAPHICS:
        currentMode = MODE_GRAPHICS;
        M5.Lcd.fillScreen(BLACK);
        drawStatusBar();
        M5.Lcd.setTextColor(TFT_CYAN, BLACK);
        M5.Lcd.setCursor(10, CONTENT_Y + 10);
        M5.Lcd.setTextSize(2);
        M5.Lcd.println("Graphics Mode");
        M5.Lcd.setTextSize(1);
        M5.Lcd.setCursor(10, CONTENT_Y + 35);
        M5.Lcd.setTextColor(TFT_YELLOW, BLACK);
        M5.Lcd.println("Use Web UI to draw");
        M5.Lcd.setCursor(10, CONTENT_Y + 50);
        M5.Lcd.println("Press B to exit");
        break;
      case APP_SETTINGS:
        currentUIState = UI_SETTINGS;
        selectedSettingIndex = 0;
        updateUI();
        return;
    }
  } else if (currentUIState == UI_POWER_MENU) {
    if (selectedPowerOption == 0) {
      // Reboot
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setTextColor(TFT_ORANGE, BLACK);
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(50, 50);
      M5.Lcd.println("Rebooting...");
      delay(1000);
      ESP.restart();
    } else {
      // Power off
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setTextColor(TFT_RED, BLACK);
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(30, 50);
      M5.Lcd.println("Powering off...");
      delay(1000);
      M5.Power.powerOff();
    }
  }
  else if (currentUIState == UI_SETTINGS) {
    // Enter performs default action: toggle Invert or step Brightness up
    if (selectedSettingIndex == 0) {
      currentBrightness = (currentBrightness + 1) > 15 ? 15 : (currentBrightness + 1);
      matrix.setIntensity(currentBrightness);
    } else {
      currentInvert = !currentInvert;
      matrix.setInvert(currentInvert);
    }
    updateUI();
  }
  else if (currentUIState == UI_TEXT_SETTINGS) {
    // Move to next field
    selectedTextSettingIndex = (selectedTextSettingIndex + 1) % 9;
    updateUI();
  }
}

void uiBack() {
  if (currentUIState == UI_APPS || currentUIState == UI_POWER_MENU) {
    currentUIState = UI_HOME;
    updateUI();
  } else if (currentUIState == UI_APP_RUNNING) {
    currentUIState = UI_HOME;
    updateUI();
  } else if (currentUIState == UI_SETTINGS) {
    // Back exits settings
    currentUIState = UI_HOME;
    updateUI();
  } else if (currentUIState == UI_TEXT_SETTINGS) {
    currentUIState = UI_APP_RUNNING;
    updateUI();
  }
}

void uiHome() {
  if (currentUIState == UI_HOME) {
    currentUIState = UI_APPS;
    selectedAppIndex = 0;
    updateUI();
  } else if (currentUIState == UI_APP_RUNNING) {
    // Home from app goes to home screen
    currentUIState = UI_HOME;
    updateUI();
  }
}

void uiMenu() {
  if (currentUIState == UI_HOME || currentUIState == UI_APP_RUNNING) {
    currentUIState = UI_POWER_MENU;
    selectedPowerOption = 0;
    updateUI();
  }
}

void openTextSettings() {
  currentUIState = UI_TEXT_SETTINGS;
  selectedTextSettingIndex = 0;
  updateUI();
}

// Increase/decrease helpers driven by Y/X buttons
void uiIncrease() {
  if (currentUIState == UI_SETTINGS) {
    if (selectedSettingIndex == 0) {
      currentBrightness = min(15, currentBrightness + 1);
      matrix.setIntensity(currentBrightness);
    } else {
      currentInvert = true;
      matrix.setInvert(currentInvert);
    }
    updateUI();
  } else if (currentUIState == UI_TEXT_SETTINGS) {
    switch (selectedTextSettingIndex) {
      case 0: currentSpeed = min(200, currentSpeed + 5); break;
      case 1: currentBrightness = min(15, currentBrightness + 1); matrix.setIntensity(currentBrightness); break;
      case 2: currentEffect = (textEffect_t)((int)currentEffect + 1); break;
      case 3: currentAlign = (textPosition_t)((int)currentAlign + 1); break;
      case 4: currentInvert = true; break;
      case 5: currentPause = min(2000, currentPause + 50); break;
      case 6: currentSpacing = min(5, currentSpacing + 1); break;
      case 7: currentScrollSpacing = min(10, currentScrollSpacing + 1); break;
      case 8: displayOn = true; break;
    }
    matrix.setInvert(currentInvert);
    matrix.setCharSpacing(currentSpacing);
    matrix.setScrollSpacing(currentScrollSpacing);
    matrix.setIntensity(currentBrightness);
    matrix.displayShutdown(!displayOn);
    if (currentMode == MODE_TEXT) {
      if (currentMsg.length()) matrix.displayText(currentMsg.c_str(), currentAlign, currentSpeed, currentPause, currentEffect, currentEffect);
      else matrix.displayReset();
    }
    updateUI();
  }
}

void uiDecrease() {
  if (currentUIState == UI_SETTINGS) {
    if (selectedSettingIndex == 0) {
      currentBrightness = max(0, currentBrightness - 1);
      matrix.setIntensity(currentBrightness);
    } else {
      currentInvert = false;
      matrix.setInvert(currentInvert);
    }
    updateUI();
  } else if (currentUIState == UI_TEXT_SETTINGS) {
    switch (selectedTextSettingIndex) {
      case 0: currentSpeed = max(0, currentSpeed - 5); break;
      case 1: currentBrightness = max(0, currentBrightness - 1); matrix.setIntensity(currentBrightness); break;
      case 2: currentEffect = (textEffect_t)max(0, (int)currentEffect - 1); break;
      case 3: currentAlign = (textPosition_t)max(0, (int)currentAlign - 1); break;
      case 4: currentInvert = false; break;
      case 5: currentPause = max(0, currentPause - 50); break;
      case 6: currentSpacing = max(0, currentSpacing - 1); break;
      case 7: currentScrollSpacing = max(0, currentScrollSpacing - 1); break;
      case 8: displayOn = false; break;
    }
    matrix.setInvert(currentInvert);
    matrix.setCharSpacing(currentSpacing);
    matrix.setScrollSpacing(currentScrollSpacing);
    matrix.setIntensity(currentBrightness);
    matrix.displayShutdown(!displayOn);
    if (currentMode == MODE_TEXT) {
      if (currentMsg.length()) matrix.displayText(currentMsg.c_str(), currentAlign, currentSpeed, currentPause, currentEffect, currentEffect);
      else matrix.displayReset();
    }
    updateUI();
  }
}

// Status updates
void setWiFiConnected(bool connected) {
  wifiConnected = connected;
  if (currentUIState != UI_APP_RUNNING) {
    drawStatusBar();
  }
}

void setBTConnected(bool connected) {
  btConnected = connected;
  if (currentUIState != UI_APP_RUNNING) {
    drawStatusBar();
  }
}

void updateBatteryLevel() {
  batteryLevel = M5.Power.getBatteryLevel();
  if (currentUIState != UI_APP_RUNNING) {
    drawStatusBar();
  }
}
